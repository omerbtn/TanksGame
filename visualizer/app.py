#!/usr/bin/env python3
import json
import os
import re
import subprocess
import sys
import threading
import time
from datetime import datetime
from pathlib import Path
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
try:
    from .replay_embed import ReplayView  # type: ignore
except Exception:
    from replay_embed import ReplayView  # type: ignore

PROJECT_ROOT = Path(__file__).resolve().parents[1]
VIS_ROOT = PROJECT_ROOT / ".vis" / "runs"
VIS_ROOT.mkdir(parents=True, exist_ok=True)


def find_simulator() -> Path:
    # Prefer build/bin, fallback to Simulator/
    ids = "322573304_322647603"  # keep in sync with project
    candidates = [
        PROJECT_ROOT / "build" / "bin" / f"simulator_{ids}",
        PROJECT_ROOT / "Simulator" / f"simulator_{ids}",
    ]
    for c in candidates:
        if c.exists():
            return c
    return candidates[0]


def get_library_extension() -> str:
    # Read from config_generated.h or config.ini fallback
    cfg_h = PROJECT_ROOT / "build" / "config_generated.h"
    if cfg_h.exists():
        text = cfg_h.read_text()
        m = re.search(r"shared_library_extension\", \"(\.[a-zA-Z0-9]+)\"", text)
        if m:
            return m.group(1)
    # fallback
    return ".so"


def latest_matching(folder: Path, pattern: re.Pattern) -> Path | None:
    best = None
    for p in folder.glob("*"):
        if not p.is_file():
            continue
        if pattern.match(p.name):
            if best is None or p.stat().st_mtime > best.stat().st_mtime:
                best = p
    return best


def run_simulator(args: list[str], cwd: Path, stdout_path: Path, stderr_path: Path) -> int:
    with open(stdout_path, "w") as out, open(stderr_path, "w") as err:
        proc = subprocess.Popen(args, cwd=str(cwd), stdout=out, stderr=err)
        return proc.wait()


def collect_traces(run_dir: Path, manifest: dict) -> list[dict]:
    # Build per-game entries: {map: <stem>, trace: <jsonl>, raw: <txt>}
    args = {k.strip(): v.strip() for k, v in (x.split('=', 1) for x in manifest.get("args", []) if '=' in x)}
    mode = manifest.get("mode", "")
    games: list[dict] = []
    if mode == "comparative":
        map_path = Path(args.get("game_map", ""))
        stems = [map_path.stem] if map_path.exists() else []
    elif mode == "competition":
        maps_folder = Path(args.get("game_maps_folder", ""))
        stems = [p.stem for p in maps_folder.glob("*.txt")]
    else:
        stems = []

    for stem in stems:
        candidates = []
        candidates += list(PROJECT_ROOT.glob(f"input_{stem}_*.txt"))
        candidates += list(PROJECT_ROOT.glob(f"{stem}_Algorithm_*_vs_*_*.txt"))
        if not candidates:
            continue
        latest = max(candidates, key=lambda p: p.stat().st_mtime)
        dest_log = run_dir / "traces" / latest.name
        dest_log.write_text(latest.read_text())
        # normalize
        try:
            from trace_parser import normalize_columns_log  # type: ignore
        except Exception:
            from .trace_parser import normalize_columns_log  # type: ignore
        jsonl_path = dest_log.with_suffix(".jsonl")
        with open(jsonl_path, "w") as out:
            for line in normalize_columns_log(dest_log):
                out.write(line + "\n")
        games.append({"map": stem, "trace": str(jsonl_path), "raw": str(dest_log)})
    return games


class RunTab(ttk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.sim_path = tk.StringVar(value=str(find_simulator()))
        self.mode = tk.StringVar(value="comparative")
        self.game_map = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "game_maps" / "input_a.txt")))
        self.game_maps_folder = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "game_maps")))
        self.game_managers_folder = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "game_managers")))
        self.game_manager_so = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "game_managers" / ("GameManager_322573304_322647603" + get_library_extension()))))
        self.algorithm1 = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "algorithms" / ("Algorithm_322573304_322647603" + get_library_extension()))))
        self.algorithm2 = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "algorithms" / ("Algorithm_322573304_322647603" + get_library_extension()))))
        self.algorithms_folder = tk.StringVar(value=str((PROJECT_ROOT / "resources" / "algorithms")))
        self.num_threads = tk.StringVar(value="4")
        self.verbose = tk.BooleanVar(value=False)

        self._build()

    def _build(self):
        row = 0
        ttk.Label(self, text="Simulator").grid(row=row, column=0, sticky="e")
        ttk.Entry(self, textvariable=self.sim_path, width=80).grid(row=row, column=1, sticky="we")
        ttk.Button(self, text="Browse", command=self._pick_sim).grid(row=row, column=2)
        row += 1

        ttk.Label(self, text="Mode").grid(row=row, column=0, sticky="e")
        ttk.Combobox(self, textvariable=self.mode, values=["comparative", "competition", "single"], width=20).grid(row=row, column=1, sticky="w")
        row += 1

        # Comparative inputs
        self._add_path(row, "Game map", self.game_map, self._pick_file)
        row += 1
        self._add_path(row, "Game managers folder", self.game_managers_folder, self._pick_dir)
        row += 1
        self._add_path(row, "Algorithm 1", self.algorithm1, self._pick_file)
        row += 1
        self._add_path(row, "Algorithm 2", self.algorithm2, self._pick_file)
        row += 1

        # Competition inputs
        self._add_path(row, "Game maps folder", self.game_maps_folder, self._pick_dir)
        row += 1
        self._add_path(row, "Game manager .so/.dylib", self.game_manager_so, self._pick_file)
        row += 1
        self._add_path(row, "Algorithms folder", self.algorithms_folder, self._pick_dir)
        row += 1

        ttk.Label(self, text="num_threads").grid(row=row, column=0, sticky="e")
        ttk.Entry(self, textvariable=self.num_threads, width=20).grid(row=row, column=1, sticky="w")
        ttk.Checkbutton(self, text="-verbose", variable=self.verbose).grid(row=row, column=2, sticky="w")
        row += 1

        ttk.Button(self, text="Run", command=self._on_run).grid(row=row, column=1, sticky="w")
        self.grid_columnconfigure(1, weight=1)

    def _add_path(self, row, label, var, picker):
        ttk.Label(self, text=label).grid(row=row, column=0, sticky="e")
        ttk.Entry(self, textvariable=var, width=80).grid(row=row, column=1, sticky="we")
        ttk.Button(self, text="Browse", command=lambda: picker(var)).grid(row=row, column=2)

    def _pick_sim(self):
        p = filedialog.askopenfilename(initialdir=str(PROJECT_ROOT))
        if p:
            self.sim_path.set(p)

    def _pick_file(self, var=None):
        p = filedialog.askopenfilename(initialdir=str(PROJECT_ROOT))
        if p and var:
            var.set(p)

    def _pick_dir(self, var=None):
        p = filedialog.askdirectory(initialdir=str(PROJECT_ROOT))
        if p and var:
            var.set(p)

    def _on_run(self):
        try:
            args = [self.sim_path.get()]
            mode = self.mode.get()
            if mode == "comparative":
                args += [
                    "-comparative",
                    f"game_map={self.game_map.get()}",
                    f"game_managers_folder={self.game_managers_folder.get()}",
                    f"algorithm1={self.algorithm1.get()}",
                    f"algorithm2={self.algorithm2.get()}",
                ]
            elif mode == "competition":
                # Ensure we have enough algorithms for competition mode
                self._ensure_competition_algorithms()
                args += [
                    "-competition",
                    f"game_maps_folder={self.game_maps_folder.get()}",
                    f"game_manager={self.game_manager_so.get()}",
                    f"algorithms_folder={self.algorithms_folder.get()}",
                ]
            else:
                args += [
                    "-single",
                    f"game_manager={self.game_manager_so.get()}",
                    f"game_map={self.game_map.get()}",
                    f"algorithm1={self.algorithm1.get()}",
                    f"algorithm2={self.algorithm2.get()}",
                ]

            if self.num_threads.get().strip():
                args.append(f"num_threads={self.num_threads.get().strip()}")
            if self.verbose.get():
                args.append("-verbose")

            run_id = datetime.now().strftime("%Y%m%d_%H%M%S")
            run_dir = VIS_ROOT / run_id
            (run_dir / "traces").mkdir(parents=True, exist_ok=True)
            stdout_path = run_dir / "stdout.log"
            stderr_path = run_dir / "stderr.log"

            def _work():
                print(f"[RunTab] Running simulator with args: {args}", flush=True)
                rc = run_simulator(args, PROJECT_ROOT, stdout_path, stderr_path)
                print(f"[RunTab] Simulator exit code: {rc}", flush=True)
                # Try to find and index the created output file
                comp_pat = re.compile(r"^comparative_results_\d{9}\.txt$")
                comp_out = latest_matching(PROJECT_ROOT / "GameManager", comp_pat)
                if comp_out is None:
                    comp_out = latest_matching(Path(self.game_managers_folder.get()), comp_pat)
                comp_manifest = None
                if comp_out and comp_out.exists():
                    comp_manifest = {
                        "type": "comparative",
                        "file": str(comp_out),
                    }
                print(f"[RunTab] comparative_output: {comp_manifest}", flush=True)

                # Competition
                compo_pat = re.compile(r"^competition_\d{9}\.txt$")
                compo_out = latest_matching(Path(self.algorithms_folder.get()), compo_pat)
                compo_manifest = None
                if compo_out and compo_out.exists():
                    compo_manifest = {
                        "type": "competition",
                        "file": str(compo_out),
                    }
                print(f"[RunTab] competition_output: {compo_manifest}", flush=True)

                manifest = {
                    "run_id": run_id,
                    "mode": mode,
                    "args": args[1:],
                    "created_at": time.time(),
                    "comparative_output": comp_manifest,
                    "competition_output": compo_manifest,
                    "games": [],  # will be filled by _collect_traces
                }
                (run_dir / "manifest.json").write_text(json.dumps(manifest, indent=2))
                print(f"[RunTab] Wrote manifest to {run_dir / 'manifest.json'}", flush=True)
                # Attempt to collect traces from column logs (input_<map>_*.txt)
                try:
                    games = self._collect_traces(run_dir, manifest)
                    manifest["games"] = games or []
                    (run_dir / "manifest.json").write_text(json.dumps(manifest, indent=2))
                    print(f"[RunTab] Collected {len(manifest['games'])} games", flush=True)
                except Exception as e:
                    # Non-fatal; just log to stderr file
                    with open(stderr_path, "a") as ef:
                        ef.write(f"\n[trace-collect-error] {e}\n")
                    print(f"[RunTab] Trace collection error: {e}", flush=True)

                if rc != 0:
                    err_txt = stderr_path.read_text(errors="ignore") if stderr_path.exists() else ""
                    messagebox.showerror("Run finished with errors",
                                         f"Return code: {rc}\nSee logs in: {run_dir}\n\n{err_txt[-800:]}" )
                else:
                    messagebox.showinfo("Run finished", f"Return code: {rc}\nManifest: {run_dir}")

            threading.Thread(target=_work, daemon=True).start()
        except Exception as e:
            messagebox.showerror("Error", str(e))

    # Backward-compatible delegator so older calls won't break
    def _collect_traces(self, run_dir: Path, manifest: dict):
        return collect_traces(run_dir, manifest)

    def _ensure_competition_algorithms(self):
        """Ensure at least 2 algorithms exist for competition mode by creating copies if needed"""
        algorithms_folder = Path(self.algorithms_folder.get())
        if not algorithms_folder.exists():
            return
        
        # Get the configured library extension
        ext = get_library_extension()
        
        # Find all algorithm files with the correct extension
        algo_files = list(algorithms_folder.glob(f"*{ext}"))
        print(f"[RunTab] Found {len(algo_files)} algorithm files with extension {ext}", flush=True)
        
        if len(algo_files) < 2:
            if len(algo_files) == 0:
                messagebox.showerror("Competition Error", 
                    f"No algorithm files found in {algorithms_folder}\n"
                    f"Expected files with extension: {ext}")
                return
            elif len(algo_files) == 1:
                # Create a copy of the single algorithm
                original = algo_files[0]
                copy_name = f"Algorithm_Copy_{original.stem}{ext}"
                copy_path = algorithms_folder / copy_name
                
                try:
                    import shutil
                    shutil.copy2(original, copy_path)
                    print(f"[RunTab] Created algorithm copy: {copy_path}", flush=True)
                    messagebox.showinfo("Competition Mode", 
                        f"Created algorithm copy for competition mode:\n{copy_name}\n\n"
                        f"Competition mode requires at least 2 algorithms.")
                except Exception as e:
                    messagebox.showerror("Copy Error", 
                        f"Failed to create algorithm copy: {e}")
                    return


class ReplaysTab(ttk.Frame):
    def __init__(self, master):
        super().__init__(master)
        
        # Create adjustable paned window
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)
        
        # Left panel for runs/games list
        left = ttk.Frame(paned)
        paned.add(left, weight=1)
        
        # Right panel for replay viewer
        right = ttk.Frame(paned)
        paned.add(right, weight=3)
        
        self.run_list = tk.Listbox(left, width=28, exportselection=False, selectmode=tk.SINGLE)
        self.game_list = tk.Listbox(left, width=40, exportselection=False, selectmode=tk.SINGLE)
        self.run_list.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.game_list.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        btns = ttk.Frame(left)
        btns.pack(side=tk.TOP, fill=tk.X)
        self.refresh_btn = ttk.Button(btns, text="Refresh", command=self.refresh)
        self.refresh_btn.pack(side=tk.LEFT)

        # Embedded replay view (single-process)
        self.viewer = ReplayView(right)
        self.viewer.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.run_list.bind("<<ListboxSelect>>", self.on_run_selected)
        self.game_list.bind("<<ListboxSelect>>", self.on_game_selected)

        self.refresh()

    def refresh(self):
        self.run_list.delete(0, tk.END)
        if not VIS_ROOT.exists():
            return
        runs = sorted([p.name for p in VIS_ROOT.iterdir() if p.is_dir()])
        print(f"[ReplaysTab] Found runs: {runs}", flush=True)
        for r in runs:
            self.run_list.insert(tk.END, r)
        self.game_list.delete(0, tk.END)

    def on_run_selected(self, _):
        self.game_list.delete(0, tk.END)
        sel = self.run_list.curselection()
        if not sel:
            return
        run_id = self.run_list.get(sel[0])
        mpath = VIS_ROOT / run_id / "manifest.json"
        if not mpath.exists():
            return
        manifest = json.loads(mpath.read_text())
        print(f"[ReplaysTab] Selected run {run_id}. Manifest keys: {list(manifest.keys())}", flush=True)
        items = []
        for g in manifest.get("games", []):
            label = f"game: {g.get('map','?')} -> {Path(g.get('trace','')).name}"
            items.append(label)
        if not items:
            if manifest.get("comparative_output"):
                items.append("comparative: " + Path(manifest["comparative_output"]["file"]).name)
            if manifest.get("competition_output"):
                items.append("competition: " + Path(manifest["competition_output"]["file"]).name)
        if not items:
            items = ["(No indexed games yet)"]
        print(f"[ReplaysTab] Items for run {run_id}: {items}", flush=True)
        for it in items:
            self.game_list.insert(tk.END, it)

    def on_game_selected(self, _):
        """Automatically open replay when a game is selected"""
        self.open_replay()

    def open_replay(self):
        sel_run = self.run_list.curselection()
        sel_game = self.game_list.curselection()
        if not sel_run:
            messagebox.showinfo("Replay", "Please select a run first.")
            return
        if not sel_game:
            messagebox.showinfo("Replay", "Please select a game from the list.")
            return
        
        run_id = self.run_list.get(sel_run[0])
        mpath = VIS_ROOT / run_id / "manifest.json"
        if not mpath.exists():
            messagebox.showerror("Replay", f"Manifest not found for run {run_id}")
            return
            
        try:
            manifest = json.loads(mpath.read_text())
        except Exception as e:
            messagebox.showerror("Replay", f"Failed to read manifest: {e}")
            return
            
        chosen = self.game_list.get(sel_game[0])
        print(f"[ReplaysTab] Open replay - run_id: {run_id}, chosen: {chosen}", flush=True)
        if chosen.startswith("game:"):
            games = manifest.get("games", [])
            idx = self.game_list.curselection()[0]
            if idx < len(games):
                g = games[idx]
                actions_arg = g.get("raw") or g.get("trace")
                map_path = self._resolve_map_from_manifest(manifest, g.get("map", ""))
                print(f"[ReplaysTab] Game {idx}: map_path={map_path}, actions_arg={actions_arg}", flush=True)
                
                if not actions_arg:
                    messagebox.showerror("Replay", "No actions file found for this game.")
                    return
                if not map_path or not map_path.exists():
                    messagebox.showerror("Replay", f"Map file not found: {map_path}")
                    return
                if not Path(actions_arg).exists():
                    messagebox.showerror("Replay", f"Actions file not found: {actions_arg}")
                    return
                
                # If a verbose log is present for this run, prefer it
                # Look for a *_verbose.txt alongside the run traces
                run_dir = VIS_ROOT / run_id / "traces"
                verbose = None
                for cand in run_dir.glob("*_verbose.txt"):
                    verbose = cand
                    break
                if verbose and verbose.exists():
                    print(f"[ReplaysTab] Loading verbose replay: {verbose}", flush=True)
                    self.viewer.load_verbose(verbose)
                    return
                
                # Fallback: use actions-based replay in embedded viewer
                print(f"[ReplaysTab] No verbose log found, falling back to actions replay: map={map_path}, actions={actions_arg}", flush=True)
                self.viewer.load_from_actions(map_path, Path(actions_arg))
                return
            messagebox.showinfo("Replay", "Trace or map missing for this game.")
            return
        if chosen.startswith("comparative:") and manifest.get("comparative_output"):
            # Try to replay using latest input_<map>_*.txt as action columns
            args_map = self._args_to_dict(manifest.get("args", []))
            map_path = Path(args_map.get("game_map", ""))
            if not map_path.exists():
                messagebox.showinfo("Replay", "Map file not found in args; opening summary output instead.")
                self._open_path(manifest['comparative_output']['file'])
                return
            trace_file = self._find_latest_columns_trace(map_path.stem)
            if not trace_file:
                messagebox.showinfo("Replay", "Could not find an input_* trace. Opening summary output instead.")
                self._open_path(manifest['comparative_output']['file'])
                return
            try:
                cmd = [sys.executable, str(PROJECT_ROOT / "visualizer" / "replay_gui.py"), "--map", str(map_path), "--actions", str(trace_file)]
                print(f"[ReplaysTab] Launching replay_gui (fallback): {' '.join(cmd)}", flush=True)
                try:
                    subprocess.Popen(cmd, cwd=str(PROJECT_ROOT))
                except Exception:
                    fallback = ["/usr/bin/python3", str(PROJECT_ROOT / "visualizer" / "replay_gui.py"), "--map", str(map_path), "--actions", str(trace_file)]
                    print(f"[ReplaysTab] Fallback launch: {' '.join(fallback)}", flush=True)
                    subprocess.Popen(fallback, cwd=str(PROJECT_ROOT))
                messagebox.showinfo("Replay", f"Launched replay window for {map_path.stem}\n\nIf it doesn't appear, enable Terminal/Python in macOS Privacy & Security → Screen Recording/Accessibility.")
            except Exception as e:
                messagebox.showerror("Replay GUI error", str(e))
                print(f"[ReplaysTab] Replay GUI error (fallback): {e}", flush=True)
        elif chosen.startswith("competition:") and manifest.get("competition_output"):
            self._open_path(manifest['competition_output']['file'])
        else:
            messagebox.showinfo("Replay", "No replayable trace available yet.")
            print("[ReplaysTab] No replayable trace available.", flush=True)

    def _open_path(self, p: str):
        # cross-platform open
        try:
            if sys.platform == 'darwin':
                subprocess.Popen(['open', p])
            elif sys.platform.startswith('linux'):
                subprocess.Popen(['xdg-open', p])
            elif sys.platform.startswith('win'):
                os.startfile(p)  # type: ignore[attr-defined]
            else:
                messagebox.showinfo("Open", p)
        except Exception as e:
            messagebox.showerror("Open error", str(e))

    def _collect_traces(self, run_dir: Path, manifest: dict):
        # Build per-game entries: {map: <stem>, trace: <jsonl>}
        args = self._args_to_dict(manifest.get("args", []))
        mode = manifest.get("mode", "")
        games: list[dict] = []
        if mode == "comparative":
            map_path = Path(args.get("game_map", ""))
            stems = [map_path.stem] if map_path.exists() else []
        elif mode == "competition":
            maps_folder = Path(args.get("game_maps_folder", ""))
            stems = [p.stem for p in maps_folder.glob("*.txt")]
        else:
            stems = []

        print(f"[_collect_traces] mode={mode} stems={stems}", flush=True)
        for stem in stems:
            candidates = []
            candidates += list(PROJECT_ROOT.glob(f"input_{stem}_*.txt"))
            candidates += list(PROJECT_ROOT.glob(f"{stem}_Algorithm_*_vs_*_*.txt"))
            if not candidates:
                print(f"[_collect_traces] No candidates for stem={stem}", flush=True)
                continue
            latest = max(candidates, key=lambda p: p.stat().st_mtime)
            print(f"[_collect_traces] Picked latest for {stem}: {latest}", flush=True)
            dest_log = run_dir / "traces" / latest.name
            dest_log.write_text(latest.read_text())
            # normalize
            try:
                from trace_parser import normalize_columns_log  # type: ignore
            except Exception:
                from .trace_parser import normalize_columns_log  # type: ignore
            jsonl_path = dest_log.with_suffix(".jsonl")
            with open(jsonl_path, "w") as out:
                for line in normalize_columns_log(dest_log):
                    out.write(line + "\n")
            games.append({"map": stem, "trace": str(jsonl_path)})
        return games

    def _args_to_dict(self, args_list):
        d = {}
        for a in args_list:
            if '=' in a:
                k, v = a.split('=', 1)
                d[k.strip()] = v.strip()
        return d

    def _find_latest_columns_trace(self, map_stem: str) -> Path | None:
        # look for files like input_<map_stem>_*.txt in project root
        pats = list(PROJECT_ROOT.glob(f"input_{map_stem}_*.txt"))
        if not pats:
            return None
        pats.sort(key=lambda p: p.stat().st_mtime, reverse=True)
        return pats[0]

    def _resolve_map_from_manifest(self, manifest: dict, map_stem: str) -> Path | None:
        args = self._args_to_dict(manifest.get("args", []))
        mode = manifest.get("mode", "")
        if mode == "comparative":
            mp = Path(args.get("game_map", ""))
            return mp if mp.exists() else None
        if mode == "competition":
            folder = Path(args.get("game_maps_folder", ""))
            candidate = folder / f"{map_stem}.txt"
            return candidate if candidate.exists() else None
        return None

    def _load_map_grid(self, map_path: Path):
        lines = map_path.read_text().splitlines()
        if len(lines) < 6:
            raise ValueError("Invalid map file")
        # skip name line and read metadata
        # name=..., MaxSteps=, NumShells=, Rows=, Cols=, then grid
        meta = { }
        idx = 1
        for key in ("MaxSteps", "NumShells", "Rows", "Cols"):
            if idx >= len(lines):
                raise ValueError("Map metadata truncated")
            parts = lines[idx].split('=')
            if len(parts) != 2:
                raise ValueError("Bad map metadata line")
            meta[key] = int(parts[1]) if parts[1].isdigit() else parts[1]
            idx += 1
        rows = meta.get("Rows")
        cols = meta.get("Cols")
        grid = []
        for r in range(rows):
            if idx + r >= len(lines):
                break
            line = lines[idx + r].rstrip('\n').rstrip('\r')
            if len(line) < cols:
                line = line + ' ' * (cols - len(line))
            grid.append(line[:cols])
        return grid

    def _load_actions_from_columns(self, path: Path):
        try:
            # local import so app runs without pygame installed for non-replay use
            from trace_parser import normalize_columns_log  # type: ignore
        except Exception:
            from .trace_parser import normalize_columns_log  # type: ignore
        actions = [json.loads(s) for s in normalize_columns_log(path)]
        return actions

    def _launch_pygame(self, grid, actions):
        try:
            from pygame_view import run_pygame  # type: ignore
        except Exception:
            from .pygame_view import run_pygame  # type: ignore
        run_pygame(grid, actions)


def main():
    root = tk.Tk()
    root.title("TanksGame Visualizer")
    nb = ttk.Notebook(root)
    nb.pack(fill=tk.BOTH, expand=True)
    
    run_tab = RunTab(nb)
    replays_tab = ReplaysTab(nb)
    
    nb.add(run_tab, text="Run")
    nb.add(replays_tab, text="Replays")
    
    # Auto-refresh replays tab when selected
    def on_tab_changed(event):
        selected_tab = event.widget.nametowidget(event.widget.select())
        if selected_tab == replays_tab:
            replays_tab.refresh()
    
    nb.bind("<<NotebookTabChanged>>", on_tab_changed)
    root.mainloop()


if __name__ == "__main__":
    main()
