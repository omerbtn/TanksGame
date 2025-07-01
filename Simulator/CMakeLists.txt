cmake_minimum_required(VERSION 3.10)
project(Simulator)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(simulator_${SUBMITTER_IDS} ${SOURCES})

target_link_libraries(simulator_${SUBMITTER_IDS} PRIVATE dl)
set_target_properties(simulator_${SUBMITTER_IDS} PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic"
)

add_dependencies(simulator_${SUBMITTER_IDS} generate_config)

# Include headers
target_include_directories(simulator_${SUBMITTER_IDS}
    PUBLIC
        ${PROJECT_ROOT}/include
        ${CMAKE_BINARY_DIR}
)

# Link against UserCommon
target_link_libraries(simulator_${SUBMITTER_IDS} PRIVATE UserCommon)

# Output to this project root directory
set_target_properties(simulator_${SUBMITTER_IDS} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}
)