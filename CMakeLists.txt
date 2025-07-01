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

# UserCommon shared library
file(GLOB_RECURSE USERCOMMON_SOURCES CONFIGURE_DEPENDS "UserCommon/src/*.cpp")
add_library(UserCommon STATIC ${USERCOMMON_SOURCES})
set_target_properties(UserCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(UserCommon PUBLIC 
    ${CMAKE_SOURCE_DIR}/UserCommon/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_BINARY_DIR}
)

# Generate config_generated.h from config.ini using generate_config.py
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CONFIG_INPUT "${CMAKE_SOURCE_DIR}/config.ini")
set(CONFIG_OUTPUT "${CMAKE_BINARY_DIR}/config_generated.h")

add_custom_command(
    OUTPUT ${CONFIG_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate_config.py ${CONFIG_INPUT} ${CONFIG_OUTPUT}
    DEPENDS ${CONFIG_INPUT} ${CMAKE_SOURCE_DIR}/generate_config.py
    COMMENT "Generating config_generated.h"
)

add_custom_target(generate_config DEPENDS ${CONFIG_OUTPUT})

add_dependencies(UserCommon generate_config)

# Add subprojects
add_subdirectory(GameManager)
add_subdirectory(Algorithm)
add_subdirectory(Simulator)