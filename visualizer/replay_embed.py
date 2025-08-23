#!/usr/bin/env python3
from __future__ import annotations

import tkinter as tk
from tkinter import ttk, messagebox
from pathlib import Path
from typing import List
import math

try:
    from .verbose_parser import parse_verbose_board_log  # type: ignore
except Exception:
    from verbose_parser import parse_verbose_board_log  # type: ignore

try:
    from .replay_ascii import Tank  # type: ignore
except Exception:
    from replay_ascii import Tank  # type: ignore


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
        # Reset dead tank tracking for verbose mode
        self._verbose_dead_tanks = set()
        self._render()

    def load_from_actions(self, map_path: Path, actions_path: Path):
        """Load replay from map file and actions log (like replay_gui.py)"""
        try:
            # Import required modules
            try:
                from .replay_ascii import parse_map, parse_actions_log, step_tanks, update_shells
            except Exception:
                from replay_ascii import parse_map, parse_actions_log, step_tanks, update_shells
            
            # Store paths for reloading
            self.initial_map_path = map_path
            self.initial_actions_path = actions_path
            
            # Parse map and actions exactly like replay_gui.py
            self.grid_lines = parse_map(map_path)[0]  # grid lines only
            tanks = parse_map(map_path)[1]  # initial tanks
            rounds, winner = parse_actions_log(actions_path)
            
            # Store like replay_gui.py for proper tank rendering
            self.tanks_initial = tanks  # Store initial tank positions
            self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in tanks]
            self.rounds = rounds
            self.winner_line = winner
            self.shells = []  # Will be populated per round
            self.beams = []   # Shell trajectories
            self.shell_explosions = []  # Shell collision explosions
            self.shell_trajectories = []  # Shell path previews
            self.tank_explosions = []  # Tank death explosions (temporary)
            
            # Initialize replay state
            self.round_index = 0
            self.playing = True
            self._render()
            
        except Exception as e:
            messagebox.showerror("Replay", f"Failed to load actions replay: {e}")
            print(f"[ReplayView] Failed to load actions: {e}")
            return

    def _on_play(self):
        # Reload the game from the stored paths
        if hasattr(self, 'initial_map_path') and hasattr(self, 'initial_actions_path'):
            # Reload the actions-based replay
            self.load_from_actions(self.initial_map_path, self.initial_actions_path)
        elif hasattr(self, 'states') and self.states:
            # Reload the verbose replay
            self.round_index = 0
            self.playing = True
            self._render()

    def _on_pause(self):
        self.playing = False

    def _on_speed(self, _):
        self.speed_fps = self.speed_scale.get()

    def _tick(self):
        if self.playing:
            if hasattr(self, 'states') and self.states:
                # Verbose mode
                if self.round_index < len(self.states):
                    self.round_index += 1
                    self._render()
                    if self.round_index >= len(self.states):
                        self.playing = False
            elif hasattr(self, 'rounds') and self.rounds:
                # Actions mode
                if self.round_index < len(self.rounds):
                    self._process_round()
                    self.round_index += 1
                    self._render()
                    if self.round_index >= len(self.rounds):
                        self.playing = False
                        
        delay_ms = max(16, int(1000 / max(0.5, self.speed_fps)))  # Cap at ~60 FPS for smoothness
        self.after(delay_ms, self._tick)

    def _process_round(self):
        """Process a single round for actions-based replay"""
        if not hasattr(self, 'rounds') or self.round_index >= len(self.rounds):
            return
            
        cols = self.rounds[self.round_index]
        w = len(self.grid_lines[0]) if self.grid_lines else 0
        h = len(self.grid_lines)
        
        # Process tank actions and create shells
        try:
            from .replay_ascii import step_tanks, update_shells
        except Exception:
            from replay_ascii import step_tanks, update_shells
            
        # Get new shells from this round and update tank positions
        new_shells = step_tanks(self.tanks_state, cols, w, h)
        
        # Add new shells to the list
        self.shells.extend(new_shells)
        
        # Store shell positions before update to detect collisions
        old_shell_positions = [(s.x, s.y) for s in self.shells]
        
        # Update existing shells using the proper update_shells function
        updated_shells = update_shells(self.shells, w, h, self.grid_lines)
        
        # Detect which shells were removed (exploded)
        self.shell_explosions = []
        if len(updated_shells) < len(self.shells):
            # Find positions where shells disappeared (collisions or wall hits)
            remaining_positions = set((s.x, s.y) for s in updated_shells)
            for i, shell in enumerate(self.shells):
                if (shell.x, shell.y) not in remaining_positions:
                    self.shell_explosions.append({
                        'x': shell.x,
                        'y': shell.y,
                        'player': shell.player
                    })
        
        self.shells = updated_shells
        
        # Track tank deaths for explosion effects
        for i, tank in enumerate(self.tanks_state):
            if not tank.alive:
                # Check if this tank just died this round
                tank_pos = (tank.x, tank.y)
                if not any(t.x == tank.x and t.y == tank.y and t.alive for t in self.tanks_state):
                    # Tank just died this round - add explosion effect
                    self.tank_explosions.append({
                        'x': tank.x,
                        'y': tank.y,
                        'player': tank.player,
                        'round': self.round_index
                    })
        
        # Remove old tank explosions (older than 1 round)
        self.tank_explosions = [exp for exp in self.tank_explosions 
                               if exp['round'] >= self.round_index - 1]
        
        # Create shell trajectories for visual effect
        self.shell_trajectories = []
        for shell in self.shells:
            trajectory = self._create_shell_trajectory(shell, w, h)
            if trajectory and len(trajectory) > 0:  # Only add if trajectory has cells
                self.shell_trajectories.append({
                    "cells": trajectory,
                    "player": shell.player
                })

    def _create_shell_trajectory(self, shell, width: int, height: int) -> List[tuple]:
        """Create a trajectory showing where a shell will go"""
        trajectory = []
        x, y = shell.x, shell.y
        dx, dy = shell.dx, shell.dy
        
        # Project shell path until it hits a wall or goes out of bounds
        # Don't include the starting position to avoid overlap with shell
        for _ in range(shell.ttl):
            nx = (x + dx) % width
            ny = (y + dy) % height
            
            # Stop if hitting a wall
            if 0 <= ny < height and 0 <= nx < width and self.grid_lines[ny][nx] == '#':
                break
                
            trajectory.append((nx, ny))
            x, y = nx, ny
            
        return trajectory

    def _render_shell_trajectories(self, ox: int, oy: int, cell: int):
        """Render shell trajectories as individual dashed lines for each shell"""
        for trajectory in self.shell_trajectories:
            if len(trajectory["cells"]) < 2:
                continue
                
            # Use player color for trajectories
            line_color = "#ffd24a" if trajectory["player"] == 1 else "#ff6b4a"
            
            # Draw dashed line connecting trajectory cells (but don't connect to shells)
            for i in range(len(trajectory["cells"]) - 1):
                x1, y1 = trajectory["cells"][i]
                x2, y2 = trajectory["cells"][i + 1]
                x0_1 = ox + x1 * cell
                y0_1 = oy + y1 * cell
                x0_2 = ox + x2 * cell
                y0_2 = oy + y2 * cell
                
                # Draw dashed line
                self.canvas.create_line(x0_1 + cell//2, y0_1 + cell//2, 
                                      x0_2 + cell//2, y0_2 + cell//2,
                                      fill=line_color, width=1, dash=(3, 3))

    def _render(self):
        self.canvas.delete("all")
        
        if hasattr(self, 'states') and self.states:
            self._render_verbose()
        elif hasattr(self, 'grid_lines') and self.grid_lines:
            self._render_actions()
        else:
            self.status.config(text="No data loaded")

    def _render_verbose(self):
        """Render verbose board states"""
        if self.round_index >= len(self.states):
            return
            
        state = self.states[self.round_index]
        rows = len(state)
        cols = len(state[0]) if rows else 0
        
        # Calculate cell size and offset
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            return
            
        cell = min(canvas_w // cols, canvas_h // rows)
        ox = (canvas_w - cols * cell) // 2
        oy = (canvas_h - rows * cell) // 2
        
        # First, fill the entire canvas with black background
        self.canvas.create_rectangle(0, 0, canvas_w, canvas_h, fill="#000000", outline="")
        
        # Draw grid only within the game map area
        if canvas_w > 1 and canvas_h > 1:
            # Draw vertical grid lines within game map
            for x in range(ox, ox + cols * cell + 1, cell):
                self.canvas.create_line(x, oy, x, oy + rows * cell, fill="#404040", width=1)
            
            # Draw horizontal grid lines within game map
            for y in range(oy, oy + rows * cell + 1, cell):
                self.canvas.create_line(ox, y, ox + cols * cell, y, fill="#404040", width=1)
        
        # Then draw the actual game map on top (without outlines since we have grid lines)
        for y in range(rows):
            for x in range(cols):
                if x >= len(state[y]):
                    continue
                token = state[y][x]
                x0 = ox + x * cell
                y0 = oy + y * cell
                x1 = x0 + cell
                y1 = y0 + cell
                
                if '*' in token:
                    # Shell
                    self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, x0 + cell*0.7, y0 + cell*0.7,
                                            fill="#ffd24a", outline="#ffffff", width=2)
                elif '1' in token or '2' in token:
                    # Tank
                    player = 1 if '1' in token else 2
                    color = "#2b7de9" if player == 1 else "#e25b3a"
                    cx = x0 + cell // 2
                    cy = y0 + cell // 2
                    r = max(6, cell * 0.35)
                    self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline="#101010")
                elif 'X' in token or 'x' in token:
                    # Dead tank - only show explosion if it just died
                    # For verbose mode, we'll show explosion for the first round we see the X
                    if not hasattr(self, '_verbose_dead_tanks'):
                        self._verbose_dead_tanks = set()
                    
                    tank_pos = (x, y)
                    if tank_pos not in self._verbose_dead_tanks:
                        # Tank just died - add to set and show explosion
                        self._verbose_dead_tanks.add(tank_pos)
                        
                        cx = x0 + cell // 2
                        cy = y0 + cell // 2
                        
                        # Large explosion - outer ring
                        explosion_size = cell * 1.2
                        self.canvas.create_oval(cx - explosion_size, cy - explosion_size, 
                                               cx + explosion_size, cy + explosion_size,
                                               fill="", outline="#ff6600", width=4)
                        
                        # Medium explosion - middle ring
                        mid_size = cell * 0.8
                        self.canvas.create_oval(cx - mid_size, cy - mid_size, 
                                               cx + mid_size, cy + mid_size,
                                               fill="#ffaa00", outline="#ff4400", width=3)
                        
                        # Small explosion - inner core
                        inner_size = cell * 0.4
                        self.canvas.create_oval(cx - inner_size, cy - inner_size, 
                                               cx + inner_size, cy + inner_size,
                                               fill="#ffdd00", outline="#ff8800", width=2)
                        
                        # Explosion particles (sparks)
                        for i in range(8):
                            angle = (i * 45) * 3.14159 / 180  # Convert to radians
                            spark_x = cx + int(explosion_size * 0.8 * math.cos(angle))
                            spark_y = cy + int(explosion_size * 0.8 * math.sin(angle))
                            self.canvas.create_oval(spark_x - 2, spark_y - 2, spark_x + 2, spark_y + 2,
                                                   fill="#ffff00", outline="#ffaa00")
                elif '#' in token:
                    # Wall
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#606060", outline="")
                elif '@' in token:
                    # Draw empty cell background first
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#151515", outline="")
                    # Mine - draw as a triangle pointing up
                    cx = x0 + cell // 2
                    cy = y0 + cell // 2
                    size = cell * 0.3
                    # Triangle pointing up
                    self.canvas.create_polygon(cx, cy - size, 
                                            cx - size, cy + size, 
                                            cx + size, cy + size,
                                            fill="#ff0000", outline="#800000", width=2)
                else:
                    # Empty space
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#151515", outline="")
        

        
        self.status.config(text=f"Round {self.round_index + 1}/{len(self.states)} | Verbose mode")

    def _render_actions(self):
        """Render actions-based replay"""
        w = len(self.grid_lines[0])
        h = len(self.grid_lines)
        
        # Calculate cell size and offset
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            return
            
        cell = min(canvas_w // w, canvas_h // h)
        ox = (canvas_w - w * cell) // 2
        oy = (canvas_h - h * cell) // 2
        
        # Fill the entire canvas with black background
        self.canvas.create_rectangle(0, 0, canvas_w, canvas_h, fill="#000000", outline="")
        
        # Draw grid only within the game map area
        if canvas_w > 1 and canvas_h > 1:
            # Draw vertical grid lines within game map
            for x in range(ox, ox + w * cell + 1, cell):
                self.canvas.create_line(x, oy, x, oy + h * cell, fill="#404040", width=1)
            
            # Draw horizontal grid lines within game map
            for y in range(oy, oy + h * cell + 1, cell):
                self.canvas.create_line(ox, y, ox + w * cell, y, fill="#404040", width=1)
        
        # Then draw the actual game map on top (without outlines since we have grid lines)
        for y in range(h):
            for x in range(w):
                x0 = ox + x * cell
                y0 = oy + y * cell
                x1 = x0 + cell
                y1 = y0 + cell
                
                ch = self.grid_lines[y][x]
                if ch == '#':
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#606060", outline="")
                elif ch == '@':
                    # Draw empty cell background first
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#151515", outline="")
                    # Mine - draw as a triangle pointing up
                    cx = x0 + cell // 2
                    cy = y0 + cell // 2
                    size = cell * 0.3
                    # Triangle pointing up
                    self.canvas.create_polygon(cx, cy - size, 
                                            cx - size, cy + size, 
                                            cx + size, cy + size,
                                            fill="#ff0000", outline="#800000", width=2)
                elif ch == ' ':
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#151515", outline="")
        
        # Shell trajectories disabled - just show moving shells
        
        # Draw shells (after trajectories so they appear on top)
        for s in self.shells:
            x0 = ox + s.x * cell
            y0 = oy + s.y * cell
            # Use player color for shells
            shell_color = "#ffd24a" if s.player == 1 else "#ff6b4a"
            self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, x0 + cell*0.7, y0 + cell*0.7,
                                    fill=shell_color, outline="#ffffff", width=2)
        
        # Draw shell explosions (when shells collide or hit walls)
        for explosion in self.shell_explosions:
            x0 = ox + explosion['x'] * cell
            y0 = oy + explosion['y'] * cell
            # Draw explosion effect - expanding circle
            self.canvas.create_oval(x0 + cell*0.1, y0 + cell*0.1, 
                                   x0 + cell*0.9, y0 + cell*0.9,
                                   fill="", outline="#ffaa00", width=3)
            # Inner explosion
            self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, 
                                   x0 + cell*0.7, y0 + cell*0.7,
                                   fill="#ffaa00", outline="#ff6600", width=2)
        
        # Draw tanks
        for idx, t in enumerate(self.tanks_state):
            if not t.alive:
                # Only show explosion if tank just died this round
                tank_explosion = next((exp for exp in self.tank_explosions 
                                     if exp['x'] == t.x and exp['y'] == t.y and exp['round'] == self.round_index), None)
                
                if tank_explosion:
                    # Draw tank explosion effect
                    x0 = ox + t.x * cell
                    y0 = oy + t.y * cell
                    cx = x0 + cell // 2
                    cy = y0 + cell // 2
                    
                    # Large explosion - outer ring
                    explosion_size = cell * 1.2
                    self.canvas.create_oval(cx - explosion_size, cy - explosion_size, 
                                           cx + explosion_size, cy + explosion_size,
                                           fill="", outline="#ff6600", width=4)
                    
                    # Medium explosion - middle ring
                    mid_size = cell * 0.8
                    self.canvas.create_oval(cx - mid_size, cy - mid_size, 
                                           cx + mid_size, cy + mid_size,
                                           fill="#ffaa00", outline="#ff4400", width=3)
                    
                    # Small explosion - inner core
                    inner_size = cell * 0.4
                    self.canvas.create_oval(cx - inner_size, cy - inner_size, 
                                           cx + inner_size, cy + inner_size,
                                           fill="#ffdd00", outline="#ff8800", width=2)
                    
                    # Explosion particles (sparks)
                    for i in range(8):
                        angle = (i * 45) * 3.14159 / 180  # Convert to radians
                        spark_x = cx + int(explosion_size * 0.8 * math.cos(angle))
                        spark_y = cy + int(explosion_size * 0.8 * math.sin(angle))
                        self.canvas.create_oval(spark_x - 2, spark_y - 2, spark_x + 2, spark_y + 2,
                                               fill="#ffff00", outline="#ffaa00")
                continue
            color = "#2b7de9" if t.player == 1 else "#e25b3a"
            x0 = ox + t.x * cell
            y0 = oy + t.y * cell
            cx = x0 + cell / 2
            cy = y0 + cell / 2
            r = max(6, cell * 0.35)
            # Body
            self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline="#101010")
            # Barrel: small line indicating direction
            dx, dy = self._dir_vector(t.deg)
            self.canvas.create_line(cx, cy, cx + dx * r, cy + dy * r, fill="#101010", width=3)
            self.canvas.create_text(cx, cy, text=str(idx+1), fill="#111111")
        
        total = len(self.rounds)
        winner = self.winner_line if self.round_index >= total else None
        status_text = f"Round {self.round_index}/{total} | Tanks: {len(self.tanks_state)}"
        if winner:
            status_text += f" | {winner}"
        self.status.config(text=status_text)

    def _dir_vector(self, deg: int):
        """Convert degrees to direction vector"""
        d = (deg % 360 + 360) % 360
        if d == 0:
            return (0.0, -1.0)
        elif d == 45:
            return (0.707, -0.707)
        elif d == 90:
            return (1.0, 0.0)
        elif d == 135:
            return (0.707, 0.707)
        elif d == 180:
            return (0.0, 1.0)
        elif d == 225:
            return (-0.707, 0.707)
        elif d == 270:
            return (-1.0, 0.0)
        elif d == 315:
            return (-0.707, -0.707)
        return (0.0, -1.0)
