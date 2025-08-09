import sys
import pygame
from pathlib import Path
from typing import List

try:
    from replay_engine import Board  # when executed from folder
except Exception:
    # Allow running from project root without package context
    from .replay_engine import Board


CELL = 32
MARGIN = 2


def render_frame(screen, board: Board):
    screen.fill((20, 20, 20))
    # Draw grid
    for y in range(board.height):
        for x in range(board.width):
            rect = pygame.Rect(x * (CELL + MARGIN) + MARGIN, y * (CELL + MARGIN) + MARGIN, CELL, CELL)
            ch = board.grid[y][x]
            color = (40, 40, 40)
            if ch == '#':
                color = (90, 90, 90)
            elif ch == '@':
                color = (160, 120, 0)
            pygame.draw.rect(screen, color, rect)

    # Draw tanks
    font = pygame.font.SysFont(None, 18)
    for t in board.tanks:
        if not t.alive:
            continue
        rect = pygame.Rect(t.x * (CELL + MARGIN) + MARGIN, t.y * (CELL + MARGIN) + MARGIN, CELL, CELL)
        color = (0, 120, 255) if t.player % 2 == 1 else (255, 80, 0)
        pygame.draw.rect(screen, color, rect, border_radius=6)
        txt = font.render(str(t.player), True, (0, 0, 0))
        screen.blit(txt, (rect.x + 10, rect.y + 8))


def run_pygame(grid: List[str], actions: List[dict]):
    print(f"[pygame_view] Starting pygame. grid_rows={len(grid)} actions={len(actions)}", flush=True)
    pygame.init()
    board = Board(grid)
    w = board.width * (CELL + MARGIN) + MARGIN
    h = board.height * (CELL + MARGIN) + MARGIN
    screen = pygame.display.set_mode((w, h))
    clock = pygame.time.Clock()
    playing = True
    t = 0

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                return
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    playing = not playing
                if event.key == pygame.K_RIGHT:
                    t += 1
                if event.key == pygame.K_LEFT:
                    t = max(0, t - 1)

        if playing and t < len(actions):
            a = actions[t]
            idx = a.get('tankIndex', 0)
            act = a.get('action', 'DoNothing')
            print(f"[pygame_view] step t={t} tankIndex={idx} action={act}", flush=True)
            board.step_action(idx, act)
            t += 1

        render_frame(screen, board)
        pygame.display.flip()
        clock.tick(8)
