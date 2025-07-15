# Function to find project root
function(find_project_root result_var)
    set(current_dir ${CMAKE_CURRENT_LIST_DIR})
    while(NOT EXISTS "${current_dir}/config.ini" AND NOT "${current_dir}" STREQUAL "/")
        get_filename_component(current_dir "${current_dir}" DIRECTORY)
    endwhile()
    
    if(NOT EXISTS "${current_dir}/config.ini")
        message(FATAL_ERROR "Could not find project root (config.ini not found)")
    endif()
    
    set(${result_var} ${current_dir} PARENT_SCOPE)
endfunction()

# Function to setup consistent build directories
# function(setup_consistent_build_dirs)
#     find_project_root(PROJECT_ROOT)
    
#     set(SHARED_BUILD_DIR "${PROJECT_ROOT}/build")
    
#     # Create necessary directories
#     file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}")
#     file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}/bin")
#     file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}/lib")
    
#     # Set consistent output directories for all projects
#     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/bin" PARENT_SCOPE)
#     set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
#     set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
    
#     # Also set the per-config versions
#     foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
#         set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/bin" PARENT_SCOPE)
#         set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
#         set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
#     endforeach()
# endfunction()

function(setup_consistent_build_dirs)
    find_project_root(PROJECT_ROOT)

    # Allow cmake to run in Simulator/build, but redirect outputs to root build/
    set(SHARED_BUILD_DIR "${PROJECT_ROOT}/build")

    # Create build dir if it doesn't exist
    file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}")
    file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}/bin")
    file(MAKE_DIRECTORY "${SHARED_BUILD_DIR}/lib")

    # Redirect only actual build artifacts
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/bin" PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)

    # Per-configuration variants
    foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/bin" PARENT_SCOPE)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "${SHARED_BUILD_DIR}/lib" PARENT_SCOPE)
    endforeach()
endfunction()


# Function to ensure config is generated (only once globally)
function(ensure_config_generation)
    if(NOT TARGET generate_config)
        find_project_root(PROJECT_ROOT)
        
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        
        set(CONFIG_INPUT "${PROJECT_ROOT}/config.ini")
        set(CONFIG_OUTPUT "${PROJECT_ROOT}/build/config_generated.h")
        
        add_custom_command(
            OUTPUT ${CONFIG_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_ROOT}/build
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_ROOT}/generate_config.py ${CONFIG_INPUT} ${CONFIG_OUTPUT}
            DEPENDS ${CONFIG_INPUT} ${PROJECT_ROOT}/generate_config.py
            COMMENT "Generating config_generated.h"
        )
        
        add_custom_target(generate_config DEPENDS ${CONFIG_OUTPUT})
    endif()
endfunction()

# Function to ensure UserCommon is built (only once globally)
function(ensure_usercommon)
    if(NOT TARGET UserCommon)
        find_project_root(PROJECT_ROOT)
        
        # Ensure config generation is set up first
        ensure_config_generation()
        
        # Build UserCommon with consistent output directory
        file(GLOB_RECURSE USERCOMMON_SOURCES CONFIGURE_DEPENDS "${PROJECT_ROOT}/UserCommon/src/*.cpp")
        add_library(UserCommon STATIC ${USERCOMMON_SOURCES})
        set_target_properties(UserCommon PROPERTIES 
            POSITION_INDEPENDENT_CODE ON
        )
        target_include_directories(UserCommon PUBLIC 
            ${PROJECT_ROOT}/UserCommon/include
            ${PROJECT_ROOT}/common
            ${PROJECT_ROOT}/build
        )
        
        add_dependencies(UserCommon generate_config)
    endif()
endfunction()

# Function to setup common project settings
function(setup_project_common)
    # Set submitter IDs if not already set
    if(NOT DEFINED SUBMITTER_IDS)
        set(SUBMITTER_IDS 322573304_322647603 PARENT_SCOPE)
    endif()
    
    # Set C++ standard if not already set
    if(NOT DEFINED CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
        set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    endif()
    
    # Set build type if not already set
    if(NOT DEFINED CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug PARENT_SCOPE)
        set(CMAKE_CXX_FLAGS_DEBUG "-g" PARENT_SCOPE)
    endif()
    
    # Enable strict compiler warnings
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    
    # Enable position independent code
    set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
    
    # Setup consistent build directories
    setup_consistent_build_dirs()
endfunction()
