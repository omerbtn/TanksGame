cmake_minimum_required(VERSION 3.10)
project(tanks_game)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include header files
include_directories(includes includes/algorithms includes/types)

# Collect all .cpp files in the main folder
file(GLOB SOURCES "src/*.cpp")

# Create the executable
add_executable(tanks_game ${SOURCES})
