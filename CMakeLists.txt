cmake_minimum_required(VERSION 3.10)
project(tanks_game)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable strict compiler warnings
# add_compile_options(-Wall -Wextra -Werror -pedantic) TODO: restore

# Enable debug symbols and set the debug type to gdb
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Find Python
find_package(Python3 COMPONENTS Interpreter REQUIRED)

include(FetchContent)

# Download gtest at configure time
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# Make it available
FetchContent_MakeAvailable(googletest)

# Path to the config files
set(CONFIG_INPUT "${CMAKE_SOURCE_DIR}/config.ini")
set(CONFIG_OUTPUT "${CMAKE_SOURCE_DIR}/includes/config_generated.h")

# Command to generate config_generated.h
add_custom_command(
    OUTPUT ${CONFIG_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate_config.py ${CONFIG_INPUT} ${CONFIG_OUTPUT}
    DEPENDS ${CONFIG_INPUT} ${CMAKE_SOURCE_DIR}/generate_config.py
    COMMENT "Generating config_generated.h from config.ini"
)

# Tell CMake that config_generated.h is a dependency
add_custom_target(generate_config DEPENDS ${CONFIG_OUTPUT})

# Include header files
include_directories(${CMAKE_SOURCE_DIR} common includes includes/algorithms includes/types includes/factory includes/players)

# Collect all .cpp files in the main folder
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(tanks_game_lib STATIC ${SOURCES})

# Make sure tanks_game depends on the generated config
add_dependencies(tanks_game_lib generate_config)

# Include the generated header
target_include_directories(tanks_game_lib PUBLIC ${CMAKE_BINARY_DIR})

add_executable(tanks_game src/main.cpp)

target_link_libraries(tanks_game PRIVATE tanks_game_lib)

# Put the game binary in the project root
set_target_properties(tanks_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Enable testing
enable_testing()

file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

# Create the test executable
add_executable(tanks_game_tests ${TEST_SOURCES})

# Link the game library + GoogleTest into the tests
target_link_libraries(tanks_game_tests
  PRIVATE
    tanks_game_lib
    GTest::gtest_main
)

# Discover and register the tests automatically
include(GoogleTest)
gtest_discover_tests(tanks_game_tests)
