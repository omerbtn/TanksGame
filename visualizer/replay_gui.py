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
        self.shells: List[dict] = []  # {x,y,dx,dy,ttl}
        self.beams: List[dict] = []   # {cells: [(x,y),...], ttl}

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
        self.play_btn.pack(side=tk.LEFT)
        self.pause_btn.pack(side=tk.LEFT, padx=(5, 10))
        self.step_back_btn.pack(side=tk.LEFT)
        self.step_fwd_btn.pack(side=tk.LEFT)

        ttk.Label(bottom, text="  Speed").pack(side=tk.LEFT, padx=(15, 4))
        self.speed_scale = ttk.Scale(bottom, from_=0.5, to=12, orient=tk.HORIZONTAL, command=self._on_speed_change)
        self.speed_scale.set(self.speed_fps)
        self.speed_scale.pack(side=tk.LEFT, fill=tk.X, expand=True)

        ttk.Label(bottom, text="  Round").pack(side=tk.LEFT, padx=(15, 4))
        self.timeline = ttk.Scale(bottom, from_=0, to=1, orient=tk.HORIZONTAL, command=self._on_scrub)
        self.timeline.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.status_var = tk.StringVar(value="Load a map and actions to start. Keys: Space play/pause, ←/→ step, F fullscreen, Esc pause")
        ttk.Label(bottom, textvariable=self.status_var).pack(side=tk.RIGHT)

        self.bind("<space>", lambda e: self._toggle_play())
        self.bind("<Left>", lambda e: self._on_step_back())
        self.bind("<Right>", lambda e: self._on_step_forward())
        self.bind("<Escape>", lambda e: self._on_pause())
        self.bind("f", lambda e: self._toggle_fullscreen())
        self.bind("F", lambda e: self._toggle_fullscreen())

    def _pick_map(self):
        p = filedialog.askopenfilename(title="Select map file", initialdir=str(Path.cwd()))
        if p:
            self.map_path_var.set(p)

    def _pick_actions(self):
        p = filedialog.askopenfilename(title="Select actions log", initialdir=str(Path.cwd()))
        if p:
            self.actions_path_var.set(p)

    def _load(self):
        try:
            map_p = Path(self.map_path_var.get())
            actions_p = Path(self.actions_path_var.get())
            if not map_p.exists() or not actions_p.exists():
                messagebox.showerror("Error", "Please choose valid map and actions files")
                return
            self.grid_lines, self.tanks_initial = parse_map(map_p)
            self.verbose_states = []
            if actions_p.suffix.lower() == '.log' or actions_p.name.lower().endswith('_verbose.txt'):
                try:
                    from .verbose_parser import parse_verbose_board_log  # type: ignore
                except Exception:
                    from verbose_parser import parse_verbose_board_log  # type: ignore
                self.verbose_states = parse_verbose_board_log(actions_p)
                self.rounds, self.winner_line = [], None
            else:
                self.rounds, self.winner_line = self._load_actions_any(actions_p, len(self.tanks_initial))
            if not self.rounds and not self.verbose_states:
                messagebox.showerror("Error", "No rounds found in actions/verbose file")
                return
            # Initialize state
            self.round_index = 0
            self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
            self.shells = []
            self.playing = True  # auto-start
            self.timeline.configure(from_=0, to=len(self.rounds) if self.rounds else len(self.verbose_states))
            # Avoid recursion when setting timeline programmatically
            self._updating_timeline = True
            try:
                self.timeline.set(0)
            finally:
                self._updating_timeline = False
            self.status_var.set(f"Loaded. Rounds: {len(self.rounds) if self.rounds else len(self.verbose_states)}. Tanks: {len(self.tanks_state)} | Space: pause, ←/→: step, F: fullscreen")
            self._render()
        except Exception as e:
            messagebox.showerror("Load error", str(e))

    def _on_speed_change(self, _val):
        try:
            self.speed_fps = max(0.5, float(self.speed_scale.get()))
        except Exception:
            self.speed_fps = 3.0

    def _on_scrub(self, _val):
        if self._updating_timeline or not self.rounds:
            return
        target = int(float(self.timeline.get()))
        self._seek_to_round(target)

    def _on_play(self):
        if not self.rounds:
            return
        # If we're at the end, restart
        if self.round_index >= len(self.rounds):
            self.round_index = 0
            self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
            self._render()
        self.playing = True

    def _on_pause(self):
        self.playing = False

    def _toggle_play(self):
        if not self.rounds:
            return
        self.playing = not self.playing

    def _on_step_forward(self):
        if not self.rounds:
            return
        self.playing = False
        self._advance_round(1)

    def _on_step_back(self):
        if not self.rounds:
            return
        self.playing = False
        self._advance_round(-1)

    def _advance_round(self, delta: int):
        new_idx = self.round_index + delta
        self._seek_to_round(new_idx)

    def _seek_to_round(self, target_idx: int):
        if not self.rounds:
            return
        w = len(self.grid_lines[0]) if self.grid_lines else 0
        h = len(self.grid_lines)
        new_idx = max(0, min(target_idx, len(self.rounds)))
        self.tanks_state = [Tank(t.player, t.x, t.y, t.alive, t.deg) for t in self.tanks_initial]
        for i in range(new_idx):
            cols = self.rounds[i]
            step_tanks(self.tanks_state, cols, w, h)
        self.round_index = new_idx
        # Reflect position on the timeline without reentry
        self._updating_timeline = True
        try:
            self.timeline.set(new_idx)
        finally:
            self._updating_timeline = False
        self._render()

    def _schedule_tick(self):
        # Called periodically to advance animation when playing
        if self.playing and self.verbose_states and self.round_index < len(self.verbose_states):
            self.round_index += 1
            self._render()
            if self.round_index >= len(self.verbose_states):
                self.playing = False
            delay_ms = max(20, int(1000 / max(0.5, self.speed_fps)))
            self.after(delay_ms, self._schedule_tick)
            return
        if self.playing and self.rounds and self.round_index < len(self.rounds):
            cols = self.rounds[self.round_index]
            w = len(self.grid_lines[0]) if self.grid_lines else 0
            h = len(self.grid_lines)
            print(f"[replay] tick={self.round_index} actions={cols}")
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
                                self.beams.append({"cells": path, "ttl": 10})
                                print(f"[replay] beam shooter={s} -> victim={victim} cells={len(path)}")
                                matched = True
                                break
                        else:
                            break
                    if matched:
                        break
            step_tanks(self.tanks_state, cols, w, h)
            # Spawn shells for any Shoot actions in this round
            for i, token in enumerate(cols):
                if i >= len(self.tanks_state):
                    break
                base = token.replace('(ignored)', '').replace('(killed)', '').strip()
                if base == 'Shoot':
                    t = self.tanks_state[i]
                    dx, dy = _dir_vector_deg(t.deg)
                    sx = (t.x + dx) % w
                    sy = (t.y + dy) % h
                    self.shells.append({"x": sx, "y": sy, "dx": dx, "dy": dy, "ttl": max(w, h)})
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
            # Decay beams
            new_beams = []
            for b in self.beams:
                b["ttl"] -= 1
                if b["ttl"] > 0:
                    new_beams.append(b)
            self.beams = new_beams
            self.round_index += 1
            alive1 = sum(1 for t in self.tanks_state if t.player == 1 and t.alive)
            alive2 = sum(1 for t in self.tanks_state if t.player == 2 and t.alive)
            if kills_in_step:
                print(f"[replay] kills this tick: {kills_in_step}; alive P1={alive1} P2={alive2}")
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
        self.canvas.delete("all")
        if not self.grid_lines:
            return
        rows = len(self.grid_lines)
        cols = len(self.grid_lines[0]) if rows else 0
        pad = 10
        cw = max(8, (self.canvas.winfo_width() - 2 * pad) // max(1, cols))
        ch = max(8, (self.canvas.winfo_height() - 2 * pad) // max(1, rows))
        cell = min(cw, ch)
        ox = (self.canvas.winfo_width() - cols * cell) // 2
        oy = (self.canvas.winfo_height() - rows * cell) // 2

        # Draw grid (uniform wall color) and accent mines with a dot
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
        # If verbose states are loaded, overlay tokens directly from them (single-process, stable scrubbing)
        if hasattr(self, 'verbose_states') and self.verbose_states:
            idx = max(0, min(self.round_index, len(self.verbose_states) - 1))
            state = self.verbose_states[idx]
            for y in range(min(rows, len(state))):
                for x in range(min(cols, len(state[y]))):
                    token = state[y][x]
                    if not token:
                        continue
                    cx = ox + x * cell + cell / 2
                    cy = oy + y * cell + cell / 2
                    if '*' in token:
                        self.canvas.create_oval(cx - cell*0.15, cy - cell*0.15, cx + cell*0.15, cy + cell*0.15, fill="#ffd24a", outline="")
                    elif '1' in token or '2' in token:
                        player = 1 if '1' in token else 2
                        color = _tank_color(player)
                        r = max(6, cell * 0.35)
                        self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline="#101010")
                        arrow = None
                        for a in ('↑','↓','←','→','↗','↘','↙','↖'):
                            if a in token:
                                arrow = a
                                break
                        if arrow:
                            self.canvas.create_text(cx, cy, text=arrow, fill="#101010")
            total = len(self.verbose_states)
            self.status_var.set(f"Round {self.round_index}/{total}  |  Verbose playback")
            return

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
        # Draw beams (shell trajectories explaining kills)
        for b in self.beams:
            for (bx, by) in b["cells"]:
                x0 = ox + bx * cell
                y0 = oy + by * cell
                self.canvas.create_oval(x0 + cell*0.35, y0 + cell*0.35, x0 + cell*0.65, y0 + cell*0.65,
                                        fill="#ffe37a", outline="")
        # Draw shells
        for s in self.shells:
            x0 = ox + s["x"] * cell
            y0 = oy + s["y"] * cell
            self.canvas.create_oval(x0 + cell*0.3, y0 + cell*0.3, x0 + cell*0.7, y0 + cell*0.7,
                                    fill="#ffd24a", outline="")

        # Status
        total = len(self.rounds)
        winner = self.winner_line if self.round_index >= total else None
        self.status_var.set(f"Round {self.round_index}/{total}  |  Tanks: {len(self.tanks_state)}  " + (f"|  {winner}" if winner else ""))
        # Help overlay
        help_lines = [
            "Space: Play/Pause",
            "Left/Right: Step",
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
