project(Algorithm)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(Algorithm_${SUBMITTER_IDS} SHARED ${SOURCES})

add_dependencies(Algorithm_${SUBMITTER_IDS} generate_config)

# Include headers
target_include_directories(Algorithm_${SUBMITTER_IDS}
    PUBLIC 
        ${PROJECT_ROOT}/include 
        ${CMAKE_BINARY_DIR}
)

# Link against UserCommon
target_link_libraries(Algorithm_${SUBMITTER_IDS} PRIVATE UserCommon)

# Output to this project root directory
set_target_properties(Algorithm_${SUBMITTER_IDS} PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}
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
