resources/ directory
---------------------

This directory contains runtime resources used by the Simulator.

Subdirectories:
===============

1. algorithms/
   - Shared object (.so) files for algorithm implementations.
   - Includes both our algorithm (.so) files (copied here automatically during build)
     and .so files from other developers (from the shared Google Drive).

2. game_managers/
   - Shared object (.so) files for game manager implementations.
   - Includes both our game manager (.so) files (copied here automatically during build)
     and those from other developers (from the shared Google Drive).

3. game_maps/
   - Contains .txt files defining game maps used during simulation runs.

Note:
-----
Our `.so` files are automatically copied into the relevant folders during build.
This directory is meant to be shared or committed (with exceptions) so the simulator
can access all required assets in one place.