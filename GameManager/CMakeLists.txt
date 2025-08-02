cmake_minimum_required(VERSION 3.13)
project(GameManager)

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
add_library(GameManager_${SUBMITTER_IDS} SHARED ${SOURCES})

# Remove prefix for .so files
set_target_properties(GameManager_${SUBMITTER_IDS} PROPERTIES PREFIX "")

# Include headers
target_include_directories(GameManager_${SUBMITTER_IDS}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_ROOT}/build  # Consistent config location
)

# Link against UserCommon
target_link_libraries(GameManager_${SUBMITTER_IDS} PRIVATE UserCommon)

# Copy output to project root
add_custom_command(TARGET GameManager_${SUBMITTER_IDS} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:GameManager_${SUBMITTER_IDS}>
        ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Copying GameManager library to project directory"
)

# TODO: Remove on submission!
# Copy .so to resources/game_managers after build
add_custom_command(TARGET GameManager_${SUBMITTER_IDS} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${PROJECT_ROOT}/resources/game_managers
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:GameManager_${SUBMITTER_IDS}>
        ${PROJECT_ROOT}/resources/game_managers/
    COMMENT "Copying GameManager library to resources directory"
)

# Ignore unresolved symbols only on non-Windows compilers
if(WIN32)
    message(FATAL_ERROR
        "This project relies on unresolved symbols being resolved at runtime (e.g., when loaded by the Simulator). "
        "However, Windows platforms — including both MSVC and MinGW — do NOT support this behavior. "
        "Please build and run this project on a non-Windows platform such as Linux."
    )
else()
    target_link_options(GameManager_${SUBMITTER_IDS} PRIVATE
        "-Wl,-undefined,dynamic_lookup"
    )
endif()
