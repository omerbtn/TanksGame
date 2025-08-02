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

Pairing Policy:
==============
As permitted in the official forum clarification: https://moodle.tau.ac.il/mod/forum/discuss.php?d=108678, 
our implementation runs all valid player pairings, including reversed matchups. 
Specifically, for each unique pair of algorithms A and B, we execute two matches: one where A plays as Player 1 and B as Player 2, 
and one where B plays as Player 1 and A as Player 2. We ensure that each directional pairing is executed only once, 
and we avoid repeating any pairing with the same player order. This design complies with the allowed alternative pairing approach, 
provided proper documentation—which is hereby included.