from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple


Direction = {
    'U': (0, -1), 'UR': (1, -1), 'R': (1, 0), 'DR': (1, 1),
    'D': (0, 1), 'DL': (-1, 1), 'L': (-1, 0), 'UL': (-1, -1),
}


@dataclass
class Tank:
    player: int
    x: int
    y: int
    dir_key: str
    alive: bool = True


class Board:
    def __init__(self, grid: List[str]):
        self.height = len(grid)
        self.width = len(grid[0]) if grid else 0
        self.grid = grid  # list of strings for static walls/mines
        self.tanks: List[Tank] = []
        self._index_initial_tanks()

    def _index_initial_tanks(self):
        # Top-to-bottom, left-to-right
        for y, line in enumerate(self.grid):
            for x, ch in enumerate(line):
                if ch.isdigit():
                    self.tanks.append(Tank(player=int(ch), x=x, y=y, dir_key='R'))

    def step_action(self, tank_index: int, action: str):
        if tank_index < 0 or tank_index >= len(self.tanks):
            return
        t = self.tanks[tank_index]
        if not t.alive:
            return
        if action == 'MoveForward':
            dx, dy = Direction[t.dir_key]
            t.x = (t.x + dx) % self.width
            t.y = (t.y + dy) % self.height
        elif action == 'MoveBackward':
            dx, dy = Direction[t.dir_key]
            t.x = (t.x - dx) % self.width
            t.y = (t.y - dy) % self.height
        elif action in ('RotateLeft90', 'RotateRight90', 'RotateLeft45', 'RotateRight45'):
            self._rotate(t, action)
        elif action == 'Shoot':
            pass  # minimal version; shells omitted in first cut

    def _rotate(self, t: Tank, action: str):
        order = ['U', 'UR', 'R', 'DR', 'D', 'DL', 'L', 'UL']
        idx = order.index(t.dir_key)
        if action == 'RotateLeft90':
            idx = (idx - 2) % 8
        elif action == 'RotateRight90':
            idx = (idx + 2) % 8
        elif action == 'RotateLeft45':
            idx = (idx - 1) % 8
        elif action == 'RotateRight45':
            idx = (idx + 1) % 8
        t.dir_key = order[idx]
