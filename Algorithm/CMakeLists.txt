cmake_minimum_required(VERSION 3.13)
project(Algorithm)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable strict compiler warnings
add_compile_options(-Wall -Wextra -Werror -pedantic)

# Include common functionality
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../cmake")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/Common.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/Common.cmake)
else()
    # Fallback for when building from root
    include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)
endif()

# Setup common project settings
setup_project_common()

# Ensure config generation and UserCommon are available
ensure_config_generation()
ensure_usercommon()

# Get project root for consistent paths
find_project_root(PROJECT_ROOT)

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create the shared library
add_library(Algorithm_${SUBMITTER_IDS} SHARED ${SOURCES})

# Remove prefix for .so files
set_target_properties(Algorithm_${SUBMITTER_IDS} PROPERTIES PREFIX "")

# Include headers
target_include_directories(Algorithm_${SUBMITTER_IDS}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
        ${PROJECT_ROOT}/build  # Consistent config location
)

# Link against UserCommon
target_link_libraries(Algorithm_${SUBMITTER_IDS} PRIVATE UserCommon)

# Copy output to project root
add_custom_command(TARGET Algorithm_${SUBMITTER_IDS} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Algorithm_${SUBMITTER_IDS}>
        ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Copying Algorithm library to project directory"
)

# TODO: Remove on submission!
# Copy .so to resources/algorithms after build
add_custom_command(TARGET Algorithm_${SUBMITTER_IDS} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${PROJECT_ROOT}/resources/algorithms
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Algorithm_${SUBMITTER_IDS}>
        ${PROJECT_ROOT}/resources/algorithms/
    COMMENT "Copying Algorithm library to resources directory"
)

# Ignore unresolved symbols only on non-Windows compilers
if(WIN32)
    message(FATAL_ERROR
        "This project relies on unresolved symbols being resolved at runtime (e.g., when loaded by the Simulator).\n"
        "However, Windows platforms — including both MSVC and MinGW — do NOT support this behavior.\n"
        "Please build and run this project on a non-Windows platform such as Linux."
    )
else()
    target_link_options(Algorithm_${SUBMITTER_IDS} PRIVATE 
        "-Wl,-undefined,dynamic_lookup"
    )
endif()
