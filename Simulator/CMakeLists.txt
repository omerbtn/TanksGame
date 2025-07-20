cmake_minimum_required(VERSION 3.13)
project(Simulator)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common functionality
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/../cmake")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/Common.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/Common.cmake)
else()
    # Fallback for when building from root
    include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)
endif()

# Setup common project settings (this also sets up shared build dirs)
setup_project_common()

# Ensure config generation and UserCommon are available
ensure_config_generation()
ensure_usercommon()

# Get project root for consistent paths
find_project_root(PROJECT_ROOT)

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create the executable
add_executable(simulator_${SUBMITTER_IDS} ${SOURCES})

# Include headers
target_include_directories(simulator_${SUBMITTER_IDS}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_ROOT}/build  # Consistent config location
)

# Link against UserCommon
target_link_libraries(simulator_${SUBMITTER_IDS} PRIVATE UserCommon)

# Simulator-specific linking
target_link_libraries(simulator_${SUBMITTER_IDS} PRIVATE dl)
set_target_properties(simulator_${SUBMITTER_IDS} PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic"
)

# Copy output to project root
add_custom_command(TARGET simulator_${SUBMITTER_IDS} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:simulator_${SUBMITTER_IDS}>
        ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Copying Simulator executable to project directory"
)
