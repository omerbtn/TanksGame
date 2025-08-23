#!/usr/bin/env python3
import sys
import argparse
import time
import json
from pathlib import Path
from typing import Optional, List, Tuple

import tkinter as tk
from tkinter import ttk, filedialog, messagebox

# Reuse parsing/logic from replay_ascii to avoid duplication
try:
    from .replay_ascii import parse_map, parse_actions_log, Tank, step_tanks
except Exception:
    from replay_ascii import parse_map, parse_actions_log, Tank, step_tanks


class ReplayGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TanksGame Replay")
        self.geometry("1200x900")
        try:
            self.attributes('-fullscreen', True)
        except Exception:
            pass

        self.map_path_var = tk.StringVar()
        self.actions_path_var = tk.StringVar()

        self.grid_lines: List[str] = []
        self.tanks_initial: List[Tank] = []
        self.rounds: List[List[str]] = []
        self.winner_line: Optional[str] = None

        self.playing: bool = False
        self.speed_fps: float = 3.0
        self.round_index: int = 0
        self.tanks_state: List[Tank] = []
        self._updating_timeline: bool = False
        self.shells: List[dict] = []  # {x,y,dx,dy,ttl,round_created}
        self.beams: List[dict] = []   # {cells: [(x,y),...], ttl}
        self.shell_trajectories: List[dict] = []  # {cells: [(x,y),...], ttl, color}

        self._build_ui()
        self._schedule_tick()

    def _build_ui(self):
        # Top bar: file pickers and load
        top = ttk.Frame(self)
        top.pack(side=tk.TOP, fill=tk.X, padx=10, pady=10)

        ttk.Label(top, text="Map:").pack(side=tk.LEFT)
        ttk.Entry(top, textvariable=self.map_path_var, width=60).pack(side=tk.LEFT, padx=5)
        ttk.Button(top, text="Browse", command=self._pick_map).pack(side=tk.LEFT)

        ttk.Label(top, text="  Actions/Verbose:").pack(side=tk.LEFT, padx=(15, 0))
        ttk.Entry(top, textvariable=self.actions_path_var, width=60).pack(side=tk.LEFT, padx=5)
        ttk.Button(top, text="Browse", command=self._pick_actions).pack(side=tk.LEFT)

        ttk.Button(top, text="Load", command=self._load).pack(side=tk.LEFT, padx=(15, 0))

        # Center: Canvas
        self.canvas = tk.Canvas(self, bg="#151515")
        self.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=10)
        # Re-render on resize so content appears immediately when the window lays out
        self.canvas.bind("<Configure>", lambda e: self._render())

        # Bottom controls
        bottom = ttk.Frame(self)
        bottom.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=10)

        self.play_btn = ttk.Button(bottom, text="Play", command=self._on_play)
        self.pause_btn = ttk.Button(bottom, text="Pause", command=self._on_pause)
        self.step_back_btn = ttk.Button(bottom, text="⟨", command=self._on_step_back)
        self.step_fwd_btn = ttk.Button(bottom, text="⟩", command=self._on_step_forward)
        self.restart_btn = ttk.Button(bottom, text="Restart", command=self._on_restart)
        self.play_btn.pack(side=tk.LEFT)
        self.pause_btn.pack(side=tk.LEFT, padx=(5, 10))
        self.step_back_btn.pack(side=tk.LEFT)
        self.step_fwd_btn.pack(side=tk.LEFT)
        self.restart_btn.pack(side=tk.LEFT, padx=(10, 0))

        ttk.Label(bottom, text="  Speed").pack(side=tk.LEFT, padx=(15, 4))
        self.speed_scale = ttk.Scale(bottom, from_=0.5, to=12, orient=tk.HORIZONTAL, command=self._on_speed_change)
        self.speed_scale.set(self.speed_fps)
        self.speed_scale.pack(side=tk.LEFT, fill=tk.X, expand=True)

        ttk.Label(bottom, text="  Round").pack(side=tk.LEFT, padx=(15, 4))
        self.timeline = ttk.Scale(bottom, from_=0, to=1, orient=tk.HORIZONTAL, command=self._on_scrub)
        self.timeline.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.status_var = tk.StringVar(value="Load a map and actions to start. Keys: Space play/pause, ←/→ step, R restart, F fullscreen, Esc pause")
        ttk.Label(bottom, textvariable=self.status_var).pack(side=tk.RIGHT)

        self.bind("<space>", lambda e: self._toggle_play())
        self.bind("<Left>", lambda e: self._on_step_back())
        self.bind("<Right>", lambda e: self._on_step_forward())
        self.bind("<Escape>", lambda e: self._on_pause())
        self.bind("r", lambda e: self._on_restart())
        self.bind("R", lambda e: self._on_restart())
        self.bind("f", lambda e: self._toggle_fullscreen())
        self.bind("F", lambda e: self._toggle_fullscreen())

    def _pick_map(self):
        path = filedialog.askopenfilename(
            title="Select Map File",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        if path:
            self.map_path_var.set(path)

    def _pick_actions(self):
        path = filedialog.askopenfilename(
            title="Select Actions/Verbose File",
            filetypes=[("Text files", "*.txt"), ("JSONL files", "*.jsonl"), ("All files", "*.*")]
        )
        if path:
            self.actions_path_var.set(path)

    def _load(self):
        map_path = Path(self.map_path_var.get())
        actions_path = Path(self.actions_path_var.get())
        
        if not map_path.exists():
            messagebox.showerror("Error", f"Map file not found: {map_path}")
            return
        if not actions_path.exists():
            messagebox.showerror("Error", f"Actions file not found: {actions_path}")
            return

        try:
            # Parse map and actions
            self.grid_lines, self.tanks_initial = parse_map(map_path)
            self.rounds, self.winner_line = self._load_actions_any(actions_path, len(self.tanks_initial))
            
            # Reset state
            self.round_index = 0
            self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
            self.shells = []
            self.beams = []
            self.shell_trajectories = []
            
            # Update timeline range
            self.timeline.configure(from_=0, to=len(self.rounds))
            self.timeline.set(0)
            
            # Start playing
            self.playing = True
            self._render()
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load files: {e}")
            print(f"Load error: {e}")

    def _on_play(self):
        # Always restart from the beginning when playing
        self.round_index = 0
        self.playing = True
        
        # Reset tank state to initial positions
        if hasattr(self, 'tanks_initial'):
            self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
            self.shells = []
            self.beams = []
            self.shell_trajectories = []
        
        # Update timeline to show we're at the beginning
        if hasattr(self, 'timeline'):
            self.timeline.set(0)

    def _on_pause(self):
        self.playing = False

    def _toggle_play(self):
        self.playing = not self.playing

    def _on_step_back(self):
        if self.round_index > 0:
            self._scrub_to_round(self.round_index - 1)

    def _on_step_forward(self):
        if self.round_index < len(self.rounds):
            self._scrub_to_round(self.round_index + 1)

    def _on_restart(self):
        self._on_play() # Re-use _on_play to reset state and timeline

    def _on_speed_change(self, _):
        self.speed_fps = self.speed_scale.get()

    def _on_scrub(self, _):
        if not self._updating_timeline:
            try:
                target_idx = int(self.timeline.get())
                self._scrub_to_round(target_idx)
            except ValueError:
                pass

    def _scrub_to_round(self, target_idx: int):
        if not self.rounds:
            return
        w = len(self.grid_lines[0]) if self.grid_lines else 0
        h = len(self.grid_lines)
        new_idx = max(0, min(target_idx, len(self.rounds)))
        self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
        self.shells = []
        self.beams = []
        self.shell_trajectories = []
        
        # Replay all rounds up to target
        for i in range(new_idx):
            cols = self.rounds[i]
            self._process_round(cols, w, h, i)
            
        self.round_index = new_idx
        # Reflect position on the timeline without reentry
        self._updating_timeline = True
        try:
            self.timeline.set(new_idx)
        finally:
            self._updating_timeline = False
        self._render()

    def _process_round(self, cols: List[str], w: int, h: int, round_idx: int):
        """Process a single round and update game state"""
        # Track kills in this step from tokens
        kills_in_step = [i for i, tok in enumerate(cols) if '(killed)' in tok]
        
        # Capture pre-step positions to compute beams
        pre_pos = [(t.x, t.y, t.deg, t.alive) for t in self.tanks_state]
        
        # Build shooter indices and ignore flags
        shooters: List[int] = []
        for i, token in enumerate(cols):
            base = token.replace('(ignored)', '').replace('(killed)', '').strip()
            if base == 'Shoot' and '(ignored)' not in token:
                shooters.append(i)
        
        # For each killed tank, try to match a shooter line-of-sight to explain kill
        for victim in kills_in_step:
            if victim < 0 or victim >= len(pre_pos):
                continue
            vx, vy, _, was_alive = pre_pos[victim]
            if not was_alive:
                continue
            matched = False
            for s in shooters:
                if s < 0 or s >= len(pre_pos):
                    continue
                sx, sy, sdeg, salive = pre_pos[s]
                if not salive:
                    continue
                dx, dy = _dir_vector_deg(sdeg)
                cx, cy = sx, sy
                path: List[tuple] = []
                # Trace until wall or bounds
                for _step in range(max(w, h)):
                    cx = (cx + dx) % w
                    cy = (cy + dy) % h
                    if 0 <= cy < h and 0 <= cx < w:
                        if self.grid_lines[cy][cx] == '#':
                            break
                        path.append((cx, cy))
                        if cx == vx and cy == vy:
                            self.beams.append({"cells": path, "ttl": 15})
                            print(f"[replay] beam shooter={s} -> victim={victim} cells={len(path)}")
                            matched = True
                            break
                    else:
                        break
                if matched:
                    break
        
        # Apply tank actions
        step_tanks(self.tanks_state, cols, w, h)
        
        # Spawn shells for any Shoot actions in this round
        for i, token in enumerate(cols):
            if i >= len(self.tanks_state):
                break
            base = token.replace('(ignored)', '').replace('(killed)', '').strip()
            if base == 'Shoot' and '(ignored)' not in token:
                t = self.tanks_state[i]
                if t.alive:
                    dx, dy = _dir_vector_deg(t.deg)
                    sx = (t.x + dx) % w
                    sy = (t.y + dy) % h
                    # Create shell with longer TTL and round tracking
                    shell = {
                        "x": sx, "y": sy, 
                        "dx": dx, "dy": dy, 
                        "ttl": max(w, h) * 2,  # Longer TTL for better visibility
                        "round_created": round_idx,
                        "player": t.player
                    }
                    self.shells.append(shell)
                    
                    # Create shell trajectory
                    trajectory = self._create_shell_trajectory(sx, sy, dx, dy, w, h, t.player)
                    self.shell_trajectories.append(trajectory)

    def _create_shell_trajectory(self, start_x: int, start_y: int, dx: int, dy: int, w: int, h: int, player: int) -> dict:
        """Create a shell trajectory from start position in direction"""
        trajectory = {"cells": [], "ttl": 20, "color": "#ffd24a" if player == 1 else "#ff6b4a"}
        
        cx, cy = start_x, start_y
        for _step in range(max(w, h) * 2):  # Longer trajectory
            cx = (cx + dx) % w
            cy = (cy + dy) % h
            if 0 <= cy < h and 0 <= cx < w:
                if self.grid_lines[cy][cx] == '#':
                    break  # Hit wall
                trajectory["cells"].append((cx, cy))
            else:
                break
                
        return trajectory

    def _schedule_tick(self):
        # Called periodically to advance animation when playing
        if self.playing and self.rounds and self.round_index < len(self.rounds):
            cols = self.rounds[self.round_index]
            w = len(self.grid_lines[0]) if self.grid_lines else 0
            h = len(self.grid_lines)
            
            print(f"[replay] tick={self.round_index} actions={cols}")
            
            # Process this round
            self._process_round(cols, w, h, self.round_index)
            
            # Advance all shells one cell; remove if hit wall or ttl over
            new_shells = []
            for s in self.shells:
                if s["ttl"] <= 0:
                    continue
                nx = (s["x"] + s["dx"]) % w
                ny = (s["y"] + s["dy"]) % h
                # stop on walls
                if 0 <= ny < h and 0 <= nx < w and self.grid_lines[ny][nx] == '#':
                    continue
                s["x"], s["y"] = nx, ny
                s["ttl"] -= 1
                new_shells.append(s)
            self.shells = new_shells
            
            # Decay beams and trajectories
            new_beams = []
            for b in self.beams:
                b["ttl"] -= 1
                if b["ttl"] > 0:
                    new_beams.append(b)
            self.beams = new_beams
            
            new_trajectories = []
            for t in self.shell_trajectories:
                t["ttl"] -= 1
                if t["ttl"] > 0:
                    new_trajectories.append(t)
            self.shell_trajectories = new_trajectories
            
            self.round_index += 1
            alive1 = sum(1 for t in self.tanks_state if t.player == 1 and t.alive)
            alive2 = sum(1 for t in self.tanks_state if t.player == 2 and t.alive)
            print(f"[replay] alive P1={alive1} P2={alive2}")
            
            self._render()
            
            if self.round_index >= len(self.rounds):
                self.playing = False
                print("[replay] finished playback; holding final board")
        
        delay_ms = max(20, int(1000 / max(0.5, self.speed_fps)))
        self.after(delay_ms, self._schedule_tick)

    def _load_actions_any(self, actions_p: Path, num_tanks: int) -> Tuple[List[List[str]], Optional[str]]:
        # Supports either original columnar logs or normalized jsonl
        rounds: List[List[str]] = []
        winner: Optional[str] = None
        if actions_p.suffix == ".jsonl":
            by_t: dict[int, List[str]] = {}
            max_t = -1
            with actions_p.open() as f:
                for line in f:
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    t = int(obj.get("t", 0))
                    idx = int(obj.get("tankIndex", 0))
                    act = str(obj.get("action", "DoNothing"))
                    if obj.get("ignored"):
                        act += " (ignored)"
                    if obj.get("killed"):
                        act += " (killed)"
                    if t not in by_t:
                        by_t[t] = ["DoNothing"] * num_tanks
                    if 0 <= idx < num_tanks:
                        by_t[t][idx] = act
                    if t > max_t:
                        max_t = t
            if max_t >= 0:
                for t in range(0, max_t + 1):
                    rounds.append(by_t.get(t, ["DoNothing"] * num_tanks))
            return rounds, winner
        else:
            return parse_actions_log(actions_p)

    def _render(self):
        if not self.grid_lines:
            return
            
        self.canvas.delete("all")
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
        
        # Draw grid
        for y in range(h):
            for x in range(w):
                x0 = ox + x * cell
                y0 = oy + y * cell
                x1 = x0 + cell
                y1 = y0 + cell
                
                ch = self.grid_lines[y][x]
                if ch == '#':
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#404040", outline="#202020")
                elif ch == '@':
                    self.canvas.create_oval(x0 + 2, y0 + 2, x1 - 2, y1 - 2, fill="#ff0000", outline="#800000")
                elif ch == ' ':
                    self.canvas.create_rectangle(x0, y0, x1, y1, fill="#151515", outline="#202020")
        
        # Draw shell trajectories (faint lines showing where shells will go)
        for t in self.shell_trajectories:
            if len(t["cells"]) > 1:
                points = []
                for bx, by in t["cells"]:
                    x0 = ox + bx * cell + cell // 2
                    y0 = oy + by * cell + cell // 2
                    points.extend([x0, y0])
                if len(points) >= 4:
                    self.canvas.create_line(points, fill=t["color"], width=1, dash=(2, 2))
        
        # Draw beams (shell trajectories explaining kills)
        for b in self.beams:
            for (bx, by) in b["cells"]:
                x0 = ox + bx * cell
                y0 = oy + by * cell
                self.canvas.create_oval(x0 + cell*0.35, y0 + cell*0.35, x0 + cell*0.65, y0 + cell*0.65,
                                        fill="#ffe37a", outline="")
        
        # Draw shells (moving projectiles)
        for s in self.shells:
            x0 = ox + s["x"] * cell
            y0 = oy + s["y"] * cell
            # Use player color for shells
            shell_color = "#ffd24a" if s["player"] == 1 else "#ff6b4a"
            self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, x0 + cell*0.7, y0 + cell*0.7,
                                    fill=shell_color, outline="#ffffff", width=2)
        
        # Draw tanks
        for idx, t in enumerate(self.tanks_state):
            if not t.alive:
                # Draw an X
                x0 = ox + t.x * cell
                y0 = oy + t.y * cell
                x1 = x0 + cell
                y1 = y0 + cell
                self.canvas.create_line(x0 + 4, y0 + 4, x1 - 4, y1 - 4, fill="#c05050", width=2)
                self.canvas.create_line(x0 + 4, y1 - 4, x1 - 4, y0 + 4, fill="#c05050", width=2)
                continue
            color = _tank_color(t.player)
            x0 = ox + t.x * cell
            y0 = oy + t.y * cell
            cx = x0 + cell / 2
            cy = y0 + cell / 2
            r = max(6, cell * 0.35)
            # Body
            self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline="#101010")
            # Barrel: small line indicating direction
            dx, dy = _dir_vector(t.deg)
            self.canvas.create_line(cx, cy, cx + dx * r, cy + dy * r, fill="#101010", width=3)
            self.canvas.create_text(cx, cy, text=str(idx+1), fill="#111111")

        # Status
        total = len(self.rounds)
        winner = self.winner_line if self.round_index >= total else None
        self.status_var.set(f"Round {self.round_index}/{total}  |  Tanks: {len(self.tanks_state)}  " + (f"|  {winner}" if winner else ""))
        
        # Help overlay
        help_lines = [
            "Space: Play/Pause",
            "Left/Right: Step",
            "R: Restart",
            "F: Toggle Fullscreen",
            "Slider: Speed, Round",
        ]
        hx, hy = 20, 20
        for i, txt in enumerate(help_lines):
            self.canvas.create_text(hx, hy + i*18, text=txt, anchor='nw', fill="#dddddd")

    def _toggle_fullscreen(self):
        try:
            cur = self.attributes('-fullscreen')
            self.attributes('-fullscreen', not bool(cur))
        except Exception:
            pass


def _dir_vector(deg: int) -> Tuple[float, float]:
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

def _tank_color(player: int) -> str:
    # Single consistent color per player
    return "#2b7de9" if player == 1 else "#e25b3a"

def _dir_vector_deg(deg: int) -> Tuple[int, int]:
    d = (deg % 360 + 360) % 360
    if d == 0:
        return (0, -1)
    if d == 45:
        return (1, -1)
    if d == 90:
        return (1, 0)
    if d == 135:
        return (1, 1)
    if d == 180:
        return (0, 1)
    if d == 225:
        return (-1, 1)
    if d == 270:
        return (-1, 0)
    if d == 315:
        return (-1, -1)
    return (0, -1)


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument('--map', type=str, default='')
    ap.add_argument('--actions', type=str, default='')
    ap.add_argument('--help', action='store_true')
    args, _ = ap.parse_known_args()

    app = ReplayGUI()

    if args.help:
        print("Usage: python3 visualizer/replay_gui.py [--map MAP] [--actions ACTIONS]", file=sys.stderr)
    # If args provided, use them; otherwise prefill
    if args.map:
        app.map_path_var.set(args.map)
    if args.actions:
        app.actions_path_var.set(args.actions)

    if not app.map_path_var.get() or not app.actions_path_var.get():
        cwd = Path.cwd()
        try:
            candidates = sorted(cwd.glob("input_*_Algorithm_*_vs_*_*.txt"), key=lambda p: p.stat().st_mtime, reverse=True)
            if candidates and not app.actions_path_var.get():
                app.actions_path_var.set(str(candidates[0]))
        except Exception:
            pass
        common_map = Path("resources/game_maps/input_c.txt")
        if common_map.exists() and not app.map_path_var.get():
            app.map_path_var.set(str(common_map))

    # Auto-load if both provided
    if app.map_path_var.get() and app.actions_path_var.get():
        app.after(200, app._load)

    app.mainloop()


if __name__ == "__main__":
    main()
