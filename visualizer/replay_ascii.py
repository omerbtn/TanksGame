#!/usr/bin/env python3
"""
ASCII Replay visualizer for TanksGame.

Usage:
  python3 visualizer/replay_ascii.py --map resources/game_maps/input_c.txt \
                                     --actions input_c_Algorithm_...txt \
                                     [--delay 0.2] [--step]

Features:
  - Parses the map file to get board size and initial tank positions in the
    specified order (top-to-bottom, left-to-right) for both players.
  - Reads a columnar per-round actions log (as produced by the simulator).
  - Renders an ASCII animation of tank movement and rotation.
  - Respects killed state as indicated by the log ("(killed)" annotation).
  - If an action is annotated with "(ignored)", we do not apply its effect.
  - Shows shell trajectories and moving shells.

Notes:
  - We do not recompute legality of moves or hits. We trust the log file.
  - Orientation starts facing up for all tanks and changes via Rotate actions.
  - Movement is in 8 directions (N, NE, E, SE, S, SW, W, NW) by one cell.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple


RESET = "\x1b[0m"
DIM = "\x1b[2m"
RED = "\x1b[31m"
BLUE = "\x1b[34m"
YELLOW = "\x1b[33m"
GREEN = "\x1b[32m"
CYAN = "\x1b[36m"


@dataclass
class Tank:
    player: int  # 1 or 2
    x: int
    y: int
    alive: bool = True
    deg: int = 0  # 0=N, 45=NE, 90=E, ... clockwise


@dataclass
class Shell:
    x: int
    y: int
    dx: int
    dy: int
    ttl: int
    player: int


def clear():
    sys.stdout.write("\x1b[2J\x1b[H")
    sys.stdout.flush()


def parse_map(map_path: Path) -> Tuple[List[str], List[Tank]]:
    lines = map_path.read_text().splitlines()
    if len(lines) < 5:
        raise ValueError("Invalid map file: too few lines (need at least 5)")

    # Strictly follow assignment format:
    # Line 1: map name (ignored)
    # Line 2: MaxSteps = <NUM>
    # Line 3: NumShells = <NUM>
    # Line 4: Rows = <NUM>
    # Line 5: Cols = <NUM>
    # Lines 6+: map grid
    
    try:
        # Parse Line 4 (Rows) - may have spaces around =
        rows_line = lines[3] if len(lines) > 3 else ""
        m = re.search(r"\bRows\b\s*=\s*(\d+)", rows_line)
        if not m:
            raise ValueError(f"Line 4 must contain 'Rows = <NUM>', got: {rows_line}")
        rows = int(m.group(1))
        
        # Parse Line 5 (Cols) - may have spaces around =
        cols_line = lines[4] if len(lines) > 4 else ""
        m = re.search(r"\bCols\b\s*=\s*(\d+)", cols_line)
        if not m:
            raise ValueError(f"Line 5 must contain 'Cols = <NUM>', got: {cols_line}")
        cols = int(m.group(1))
        
    except (IndexError, ValueError) as e:
        raise ValueError(f"Invalid map format following assignment rules: {e}")

    # According to assignment: Lines 6+ (index 5+) contain the map grid
    grid_start = 5

    grid_lines: List[str] = []
    tanks: List[Tank] = []
    for r in range(rows):
        src_idx = grid_start + r
        if src_idx >= len(lines):
            break
        row_line = lines[src_idx].rstrip('\n').rstrip('\r')
        if len(row_line) < cols:
            row_line = row_line + ' ' * (cols - len(row_line))
        row_line = row_line[:cols]
        # Scan for tanks and normalize the grid to remove '1'/'2' markers
        for c, ch in enumerate(row_line):
            if ch == '1' or ch == '2':
                player = 1 if ch == '1' else 2
                # According to assignment rules: Player 1 faces LEFT (270°), Player 2 faces RIGHT (90°)
                initial_direction = 270 if player == 1 else 90
                tanks.append(Tank(player=player, x=c, y=r, deg=initial_direction))
        normalized = row_line.replace('1', ' ').replace('2', ' ')
        grid_lines.append(normalized)

    # Sort tanks by reading order (top->bottom, then left->right)
    tanks.sort(key=lambda t: (t.y, t.x))
    return grid_lines, tanks


def parse_actions_log(log_path: Path) -> Tuple[List[List[str]], str | None]:
    rounds: List[List[str]] = []
    winner: str | None = None
    with log_path.open('r') as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            if line.startswith('Player ') or line.startswith('Tie'):
                winner = line
                break
            # split by commas
            parts = [p.strip() for p in line.split(',')]
            rounds.append(parts)
    return rounds, winner


def parse_action_token(token: str) -> Tuple[str, bool, bool]:
    # Returns (base_action, ignored, killed)
    ignored = '(ignored)' in token
    killed = '(killed)' in token
    base = token
    base = base.replace('(ignored)', '').replace('(killed)', '').strip()
    return base, ignored, killed


def dir_to_arrow(deg: int) -> str:
    deg = (deg % 360 + 360) % 360
    mapping = {
        0: '^', 45: '⭧', 90: '>', 135: '⭨', 180: 'v', 225: '⭩', 270: '<', 315: '⭦'
    }
    return mapping.get(deg, '^')


def step_tanks(tanks: List[Tank], actions: List[str], width: int, height: int) -> List[Shell]:
    # Apply actions as described by the tokens; trust ignored/killed flags
    # We compute shot flash positions using pre-move orientation and position
    shells: List[Shell] = []
    to_kill: List[int] = []
    
    # First pass: compute rotations/moves and record shots
    for i, token in enumerate(actions):
        if i >= len(tanks):
            continue
        tank = tanks[i]
        if not tank.alive:
            continue
        base, ignored, killed = parse_action_token(token)
        if base.startswith('RotateLeft'):
            deg = int(base.replace('RotateLeft', '').replace('Rotate', ''))
            tank.deg = (tank.deg - deg) % 360
        elif base.startswith('RotateRight'):
            deg = int(base.replace('RotateRight', '').replace('Rotate', ''))
            tank.deg = (tank.deg + deg) % 360
        elif base in ('MoveForward', 'MoveBackward') and not ignored:
            dx, dy = direction_delta(tank.deg)
            if base == 'MoveBackward':
                dx, dy = -dx, -dy
            tank.x = (tank.x + dx) % width
            tank.y = (tank.y + dy) % height
        elif base == 'Shoot' and not ignored:
            dx, dy = direction_delta(tank.deg)
            sx = (tank.x + dx) % width
            sy = (tank.y + dy) % height
            # Create shell with longer TTL for better visibility
            shell = Shell(x=sx, y=sy, dx=dx, dy=dy, ttl=width * 2, player=tank.player)
            shells.append(shell)
        # GetBattleInfo, DoNothing: no position change
        if killed:
            to_kill.append(i)
    
    # Second pass: apply kills
    for i in to_kill:
        if 0 <= i < len(tanks):
            tanks[i].alive = False
    
    return shells


def direction_delta(deg: int) -> Tuple[int, int]:
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


def update_shells(shells: List[Shell], width: int, height: int, grid_lines: List[str]) -> List[Shell]:
    """Update shell positions and remove those that hit walls or expired"""
    new_shells = []
    for shell in shells:
        if shell.ttl <= 0:
            continue
        # Move shell
        nx = (shell.x + shell.dx) % width
        ny = (shell.y + shell.dy) % height
        # Stop on walls
        if 0 <= ny < height and 0 <= nx < width and grid_lines[ny][nx] == '#':
            continue
        shell.x, shell.y = nx, ny
        shell.ttl -= 1
        new_shells.append(shell)
    
    # Check for shell collisions - if two shells are in the same cell, both explode
    collision_positions = {}
    for i, shell in enumerate(new_shells):
        pos = (shell.x, shell.y)
        if pos in collision_positions:
            collision_positions[pos].append(i)
        else:
            collision_positions[pos] = [i]
    
    # Remove shells that collided (more than one shell in same position)
    shells_to_remove = set()
    for pos, shell_indices in collision_positions.items():
        if len(shell_indices) > 1:
            shells_to_remove.update(shell_indices)
    
    # Filter out collided shells
    final_shells = [shell for i, shell in enumerate(new_shells) if i not in shells_to_remove]
    return final_shells


def render(grid: List[str], tanks: List[Tank], shells: List[Shell], round_idx: int, winner: str | None):
    clear()
    print(f"Round: {round_idx}")
    if winner:
        print(DIM + winner + RESET)
    print()
    
    # Draw grid as list of chars we can overlay
    rows = len(grid)
    cols = len(grid[0]) if rows else 0
    canvas = [list(row) for row in grid]
    
    # Overlay shells as '*'
    for shell in shells:
        if 0 <= shell.y < rows and 0 <= shell.x < cols:
            # Use different colors for different players
            if shell.player == 1:
                canvas[shell.y][shell.x] = YELLOW + '*' + RESET
            else:
                canvas[shell.y][shell.x] = CYAN + '*' + RESET
    
    # Overlay tanks
    for t in tanks:
        ch = 'x' if not t.alive else dir_to_arrow(t.deg)
        color = BLUE if t.player == 1 else RED
        if 0 <= t.y < rows and 0 <= t.x < cols:
            canvas[t.y][t.x] = color + ch + RESET
    
    # Print
    for r in canvas:
        print(''.join(r))
    print()
    
    alive1 = sum(1 for t in tanks if t.player == 1 and t.alive)
    alive2 = sum(1 for t in tanks if t.player == 2 and t.alive)
    print(f"P1 alive: {alive1}    P2 alive: {alive2}")
    print(f"Shells in air: {len(shells)}")
    print("Controls: Ctrl+C to quit")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--map', required=True, type=Path)
    ap.add_argument('--actions', required=True, type=Path)
    ap.add_argument('--delay', type=float, default=0.2, help='seconds between frames')
    ap.add_argument('--step', action='store_true', help='wait for Enter each frame')
    args = ap.parse_args()

    grid, tanks = parse_map(args.map)
    rounds, winner_line = parse_actions_log(args.actions)

    # Validate columns
    num_tanks = len(tanks)
    if not rounds:
        print('No rounds found in actions file.', file=sys.stderr)
        sys.exit(1)
    for idx, r in enumerate(rounds[:5]):
        if len(r) != num_tanks:
            print(f"Warning: round {idx} column count {len(r)} != num tanks {num_tanks}", file=sys.stderr)

    # Initialize shells list
    all_shells: List[Shell] = []
    
    try:
        for i, cols in enumerate(rounds, start=1):
            # Get new shells from this round
            new_shells = step_tanks(tanks, cols, len(grid[0]), len(grid))
            all_shells.extend(new_shells)
            
            # Update existing shells
            all_shells = update_shells(all_shells, len(grid[0]), len(grid), grid)
            
            render(grid, tanks, all_shells, i, winner_line if i == len(rounds) else None)
            
            if args.step:
                input()
            else:
                time.sleep(args.delay)
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
