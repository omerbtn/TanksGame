import json
import re
from pathlib import Path
from typing import Iterable


# Example adapter to normalize lines like:
# "MoveForward, RotateLeft90, ..." per tank columns

ACTION_RE = re.compile(r"(MoveForward|MoveBackward|RotateLeft90|RotateRight90|RotateLeft45|RotateRight45|Shoot|DoNothing|GetBattleInfo)")


def normalize_columns_log(path: Path) -> Iterable[str]:
    # Each line has comma-separated actions for tanks (columns by top-left to bottom-right order)
    # We emit json lines preserving flags:
    # {"t": tick, "tankIndex": k, "action": "MoveForward", "ignored": true/false, "killed": true/false}
    t = 0
    with path.open() as f:
        for line in f:
            parts = [p.strip() for p in line.strip().split(',')]
            for k, token in enumerate(parts):
                m = ACTION_RE.search(token)
                if not m:
                    continue
                action = m.group(1)
                ignored = "(ignored)" in token
                killed = "(killed)" in token
                yield json.dumps({"t": t, "tankIndex": k, "action": action, "ignored": ignored, "killed": killed})
            t += 1
