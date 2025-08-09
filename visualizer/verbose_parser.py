#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path
from typing import List, Tuple, Dict, Any


def parse_verbose_board_log(path: Path) -> List[List[List[str]]]:
    """
    Parse a verbose GameManager board log into a list of board states.
    Each state is a 2D array of cell payload strings (e.g. '#', '@', ' ', '1←', '*↓').
    The log is expected to contain repeating blocks after lines that start with 'Game Board:'.
    """
    text = path.read_text(errors='ignore').splitlines()
    states: List[List[List[str]]] = []
    i = 0
    while i < len(text):
        line = text[i]
        if line.strip().startswith('Game Board:'):
            i += 1
            grid: List[List[str]] = []
            # Read until a blank line or until the next non-row marker
            while i < len(text):
                row = text[i].strip()
                if not row or not row.startswith('['):
                    break
                # Extract tokens inside []
                tokens = re.findall(r"\[(.*?)\]", row)
                # Normalize tokens to strings without padding
                normalized = [t.strip() for t in tokens]
                grid.append(normalized)
                i += 1
            if grid:
                states.append(grid)
            continue
        i += 1
    return states
