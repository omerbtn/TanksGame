cmake_minimum_required(VERSION 3.10)
project(tanks_game)
set(CMAKE_BUILD_TYPE Debug)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include header files
include_directories(includes includes/algorithms includes/types)

# Collect all .cpp files in the main folder
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the executable
add_executable(tanks_game ${SOURCES})

# Ensure debug flags are set
set(CMAKE_CXX_FLAGS_DEBUG "-g")