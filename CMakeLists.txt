cmake_minimum_required(VERSION 3.13)
project(TanksGame)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable strict compiler warnings
add_compile_options(-Wall -Wextra -Werror -pedantic)

# Enable debug symbols and set the debug type to gdb
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Set submitter IDs globally
set(SUBMITTER_IDS 322573304_322647603)

# Include common functionality
include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)

# Add subprojects
add_subdirectory(GameManager)
add_subdirectory(Algorithm)
add_subdirectory(Simulator)
