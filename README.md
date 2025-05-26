# TanksGame

Omer Biton - 322573304

Arbel Katzir - 322647603

## Prerequisites

- CMake 3.10 or higher
- A C++20-compatible compiler (e.g. GCC 10+, Clang 11+, MSVC 2019+)
- Python 3 (for generating configuration headers)

## Build Instructions

1. **Create a build directory**:

   ```sh
   mkdir build
   cd build
   ```

2. **Run CMake to configure the project**:

   ```sh
   cmake ..
   ```

3. **Build the project**:

   ```sh
   cmake --build .
   ```

   This will generate the `tanks_game` executable in the root of the repository.

## Run the Game

From the root of the project (not inside `build/`), run:

```sh
./tanks_game <path_to_board_file>
```

## Input files

The structure of the input files is as following:

```
Width Height 

BOARD INFO
```

So the first line is used for indicating the dimensions of the board.
The next lines are the board itself.

The input and output files for demonstration are located in the resources folder.