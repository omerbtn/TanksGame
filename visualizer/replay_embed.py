#!/usr/bin/env python3
from __future__ import annotations

import tkinter as tk
from tkinter import ttk, messagebox
from pathlib import Path
from typing import List

try:
    from .verbose_parser import parse_verbose_board_log  # type: ignore
except Exception:
    from verbose_parser import parse_verbose_board_log  # type: ignore


class ReplayView(ttk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.canvas = tk.Canvas(self, bg="#151515")
        self.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        controls = ttk.Frame(self)
        controls.pack(side=tk.BOTTOM, fill=tk.X)
        self.play_btn = ttk.Button(controls, text="Play", command=self._on_play)
        self.pause_btn = ttk.Button(controls, text="Pause", command=self._on_pause)
        self.play_btn.pack(side=tk.LEFT)
        self.pause_btn.pack(side=tk.LEFT, padx=(6, 12))
        ttk.Label(controls, text="Speed").pack(side=tk.LEFT)
        self.speed_scale = ttk.Scale(controls, from_=0.5, to=12, orient=tk.HORIZONTAL, command=self._on_speed)
        self.speed_scale.set(3.0)
        self.speed_scale.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=8)

        self.status = ttk.Label(controls, text="")
        self.status.pack(side=tk.RIGHT)

        self.states: List[List[List[str]]] = []
        self.round_index: int = 0
        self.playing: bool = False
        self.speed_fps: float = 3.0
        self.cell_cache = {}
        self.canvas.bind("<Configure>", lambda e: self._render())
        self.after(150, self._tick)

    def load_verbose(self, verbose_path: Path):
        self.states = parse_verbose_board_log(verbose_path)
        if not self.states:
            messagebox.showerror("Replay", "No board states found in verbose log")
            return
        self.round_index = 0
        self.playing = True
        self._render()

    def load_from_actions(self, map_path: Path, actions_path: Path):
        """Load replay from map file and actions log (like replay_gui.py)"""
        try:
            # Import required modules
            try:
                from .replay_ascii import parse_map, parse_actions_log, Tank, step_tanks
            except Exception:
                from replay_ascii import parse_map, parse_actions_log, Tank, step_tanks
            
            # Store paths for scrubbing
            self.initial_map_path = map_path
            
            # Parse map and actions exactly like replay_gui.py
            self.grid_lines = parse_map(map_path)[0]  # grid lines only
            tanks = parse_map(map_path)[1]  # initial tanks
            rounds, winner = parse_actions_log(actions_path)
            
            # Store like replay_gui.py for proper tank rendering
            self.tanks_state = tanks  # Tank objects with x, y, alive, deg, player
            self.rounds = rounds
            self.winner_line = winner
            self.shells = []  # Will be populated per round
            self.beams = []   # Shell trajectories
            self.explosions = []  # Shell collision explosions
            
            # Initialize replay state
            self.round_index = 0
            self.playing = True
            self._render()
            
        except Exception as e:
            messagebox.showerror("Replay", f"Failed to load actions replay: {e}")
            print(f"[ReplayView] Failed to load actions: {e}")
            return

    def _on_play(self):
        # Handle both verbose states and tank-based replay
        if hasattr(self, 'states') and self.states:
            if self.round_index >= len(self.states):
                self.round_index = 0
            self.playing = True
        elif hasattr(self, 'rounds') and self.rounds:
            if self.round_index >= len(self.rounds):
                self.round_index = 0
            self.playing = True

    def _on_pause(self):
        self.playing = False

    def _on_speed(self, _):
        try:
            self.speed_fps = max(0.5, float(self.speed_scale.get()))
        except Exception:
            self.speed_fps = 3.0

    def _tick(self):
        if self.playing:
            if hasattr(self, 'states') and self.states and self.round_index < len(self.states):
                self.round_index += 1
                self._render()
                if self.round_index >= len(self.states):
                    self.playing = False
            elif hasattr(self, 'rounds') and self.rounds and self.round_index < len(self.rounds):
                self._advance_one_round()
                self.round_index += 1
                self._render()
                if self.round_index >= len(self.rounds):
                    self.playing = False
        delay_ms = max(20, int(1000 / max(0.5, self.speed_fps)))
        self.after(delay_ms, self._tick)

    def _advance_one_round(self):
        """Advance tanks by one round using step_tanks logic with proper shell management"""
        if not hasattr(self, 'rounds') or self.round_index >= len(self.rounds):
            return
        try:
            from .replay_ascii import step_tanks
        except Exception:
            from replay_ascii import step_tanks
        
        cols = self.rounds[self.round_index]
        w = len(self.grid_lines[0]) if self.grid_lines else 10
        h = len(self.grid_lines) if self.grid_lines else 10
        
        # Step tanks first
        step_tanks(self.tanks_state, cols, w, h)
        
        # Spawn new shells for any Shoot actions in this round
        for i, token in enumerate(cols):
            if i >= len(self.tanks_state):
                break
            base = token.replace('(ignored)', '').replace('(killed)', '').strip()
            if base == 'Shoot':
                t = self.tanks_state[i]
                dx, dy = self._dir_vector(t.deg)
                sx = int((t.x + dx) % w)
                sy = int((t.y + dy) % h)
                self.shells.append({"x": sx, "y": sy, "dx": dx, "dy": dy, "ttl": max(w, h)})
        
        # Advance all existing shells one cell; remove if hit wall or ttl over
        new_shells = []
        for s in self.shells:
            if s["ttl"] <= 0:
                continue
            nx = int((s["x"] + s["dx"]) % w)
            ny = int((s["y"] + s["dy"]) % h)
            # Stop on walls
            if 0 <= ny < h and 0 <= nx < w and self.grid_lines[ny][nx] == '#':
                continue
            s["x"], s["y"] = nx, ny
            s["ttl"] -= 1
            new_shells.append(s)
        
        # Check for shell-to-shell collisions - shells explode when they occupy same position
        collision_positions = set()
        surviving_shells = []
        
        # Group shells by position
        position_groups = {}
        for s in new_shells:
            pos = (s["x"], s["y"])
            if pos not in position_groups:
                position_groups[pos] = []
            position_groups[pos].append(s)
        
        # Keep only shells that are alone at their position
        for pos, shells_at_pos in position_groups.items():
            if len(shells_at_pos) == 1:
                # No collision, shell survives
                surviving_shells.extend(shells_at_pos)
            else:
                # Collision! All shells at this position explode
                collision_positions.add(pos)
                # Add explosion effect that lasts a few frames
                self.explosions.append({"x": pos[0], "y": pos[1], "ttl": 3})
                print(f"[Shell Collision] {len(shells_at_pos)} shells exploded at position {pos}")
        
        # Decay explosions
        self.explosions = [e for e in self.explosions if e["ttl"] > 0]
        for e in self.explosions:
            e["ttl"] -= 1
        
        self.shells = surviving_shells

    def _update_tank_state_to_round(self, target_round):
        """Update tank state to specific round (for scrubbing)"""
        if not hasattr(self, 'rounds') or not hasattr(self, 'initial_map_path'):
            return
        
        try:
            from .replay_ascii import parse_map, step_tanks
        except Exception:
            from replay_ascii import parse_map, step_tanks
        
        # Re-parse map to get fresh tank state
        _, fresh_tanks = parse_map(self.initial_map_path)
        
        # Reset tanks to initial state
        self.tanks_state = fresh_tanks
        self.shells = []  # Clear all shells
        self.explosions = []  # Clear explosions
        
        # Step through rounds up to target
        w = len(self.grid_lines[0]) if self.grid_lines else 10
        h = len(self.grid_lines) if self.grid_lines else 10
        for round_idx in range(min(target_round, len(self.rounds))):
            cols = self.rounds[round_idx]
            
            # Step tanks
            step_tanks(self.tanks_state, cols, w, h)
            
            # Spawn shells for any Shoot actions
            for i, token in enumerate(cols):
                if i >= len(self.tanks_state):
                    break
                base = token.replace('(ignored)', '').replace('(killed)', '').strip()
                if base == 'Shoot':
                    t = self.tanks_state[i]
                    dx, dy = self._dir_vector(t.deg)
                    sx = int((t.x + dx) % w)
                    sy = int((t.y + dy) % h)
                    self.shells.append({"x": sx, "y": sy, "dx": dx, "dy": dy, "ttl": max(w, h)})
            
            # Advance shells
            new_shells = []
            for s in self.shells:
                if s["ttl"] <= 0:
                    continue
                nx = int((s["x"] + s["dx"]) % w)
                ny = int((s["y"] + s["dy"]) % h)
                if 0 <= ny < h and 0 <= nx < w and self.grid_lines[ny][nx] == '#':
                    continue
                s["x"], s["y"] = nx, ny
                s["ttl"] -= 1
                new_shells.append(s)
            
            # Check for shell collisions during scrubbing too
            surviving_shells = []
            position_groups = {}
            for s in new_shells:
                pos = (s["x"], s["y"])
                if pos not in position_groups:
                    position_groups[pos] = []
                position_groups[pos].append(s)
            
            for pos, shells_at_pos in position_groups.items():
                if len(shells_at_pos) == 1:
                    surviving_shells.extend(shells_at_pos)
                # else: shells explode in collision
            
            self.shells = surviving_shells

    def _render(self):
        self.canvas.delete("all")
        
        # Handle both verbose states and tank-based rendering
        if hasattr(self, 'states') and self.states:
            # Verbose log rendering (existing logic)
            idx = max(0, min(self.round_index, len(self.states) - 1))
            state = self.states[idx]
            rows = len(state)
            cols = len(state[0]) if rows else 0
            if rows == 0 or cols == 0:
                return
            # ... (keep existing verbose rendering logic)
            self.status.configure(text=f"Round {idx+1}/{len(self.states)}")
            return
        
        # Tank-based rendering (like replay_gui.py)
        if not hasattr(self, 'grid_lines') or not self.grid_lines:
            self.canvas.create_text(
                self.canvas.winfo_width() // 2,
                self.canvas.winfo_height() // 2,
                text="No replay loaded",
                fill="#888888",
                font=("Arial", 14)
            )
            return
            
        rows = len(self.grid_lines)
        cols = len(self.grid_lines[0]) if rows else 0
        if rows == 0 or cols == 0:
            return
            
        pad = 10
        cw = max(8, (self.canvas.winfo_width() - 2 * pad) // max(1, cols))
        ch = max(8, (self.canvas.winfo_height() - 2 * pad) // max(1, rows))
        cell = min(cw, ch)
        ox = (self.canvas.winfo_width() - cols * cell) // 2
        oy = (self.canvas.winfo_height() - rows * cell) // 2

        # Draw grid background - exactly like replay_gui.py
        for y in range(rows):
            for x in range(cols):
                chv = self.grid_lines[y][x]
                if chv == '#':
                    color = "#7a7a7a"  # walls uniform
                elif chv == '@':
                    color = "#c9a227"  # mines
                else:
                    color = "#2a2a2a"  # floor
                self.canvas.create_rectangle(ox + x * cell, oy + y * cell, ox + (x + 1) * cell, oy + (y + 1) * cell,
                                           fill=color, outline="#1c1c1c")
                if chv == '@':
                    cx = ox + x * cell + cell / 2
                    cy = oy + y * cell + cell / 2
                    r = max(2, cell * 0.15)
                    self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill="#ffdd55", outline="")

        # Draw tanks - exactly like replay_gui.py
        if hasattr(self, 'tanks_state'):
            for idx, t in enumerate(self.tanks_state):
                if not t.alive:
                    # Draw an X for dead tanks
                    x0 = ox + t.x * cell
                    y0 = oy + t.y * cell
                    x1 = x0 + cell
                    y1 = y0 + cell
                    self.canvas.create_line(x0 + 4, y0 + 4, x1 - 4, y1 - 4, fill="#c05050", width=2)
                    self.canvas.create_line(x0 + 4, y1 - 4, x1 - 4, y0 + 4, fill="#c05050", width=2)
                    continue
                    
                # Tank color by player
                color = "#2b7de9" if t.player == 1 else "#e25b3a"
                x0 = ox + t.x * cell
                y0 = oy + t.y * cell
                cx = x0 + cell / 2
                cy = y0 + cell / 2
                r = max(6, cell * 0.35)
                
                # Tank body
                self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline="#101010")
                
                # Tank barrel (direction indicator)
                dx, dy = self._dir_vector(t.deg)
                self.canvas.create_line(cx, cy, cx + dx * r, cy + dy * r, fill="#101010", width=3)
                
                # Tank number (1-based)
                self.canvas.create_text(cx, cy, text=str(idx+1), fill="#111111")

        # Draw shells
        if hasattr(self, 'shells'):
            for s in self.shells:
                x0 = ox + s["x"] * cell
                y0 = oy + s["y"] * cell
                self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, x0 + cell*0.7, y0 + cell*0.7,
                                      fill="#ffd24a", outline="")

        # Draw explosions (from shell collisions)
        if hasattr(self, 'explosions'):
            for e in self.explosions:
                x0 = ox + e["x"] * cell
                y0 = oy + e["y"] * cell
                cx = x0 + cell / 2
                cy = y0 + cell / 2
                # Explosion effect - bright red/orange burst
                radius = max(8, cell * 0.6)
                self.canvas.create_oval(cx - radius, cy - radius, cx + radius, cy + radius,
                                      fill="#ff4444", outline="#ffaa00", width=2)
                # Inner bright core
                inner_radius = max(4, cell * 0.3)
                self.canvas.create_oval(cx - inner_radius, cy - inner_radius, cx + inner_radius, cy + inner_radius,
                                      fill="#ffff44", outline="")

        # Status
        if hasattr(self, 'rounds'):
            total = len(self.rounds)
            winner = self.winner_line if hasattr(self, 'winner_line') and self.round_index >= total else None
            tank_count = len(self.tanks_state) if hasattr(self, 'tanks_state') else 0
            self.status.configure(text=f"Round {self.round_index}/{total}  |  Tanks: {tank_count}  " + (f"|  {winner}" if winner else ""))
        else:
            self.status.configure(text="Ready")

    def _dir_vector(self, deg: int):
        """Convert degrees to direction vector for barrel rendering"""
        d = (deg % 360 + 360) % 360
        if d == 0:
            return (0.0, -1.0)
        if d == 45:
            return (0.707, -0.707)
        if d == 90:
            return (1.0, 0.0)
        if d == 135:
            return (0.707, 0.707)
        if d == 180:
            return (0.0, 1.0)
        if d == 225:
            return (-0.707, 0.707)
        if d == 270:
            return (-1.0, 0.0)
        if d == 315:
            return (-0.707, -0.707)
        return (0.0, -1.0)
