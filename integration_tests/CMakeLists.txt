cmake_minimum_required(VERSION 3.13)
project(integration_tests)

include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)
setup_project_common()
ensure_config_generation()
ensure_usercommon()

# Try to find GTest, otherwise fetch it
include(FetchContent)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found system-wide. Fetching googletest...")
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

set(SUBMITTER_IDS 322573304_322647603)

add_executable(integration_tests
  test_integration_simulator.cpp
  test_integration_cli_args.cpp
  test_integration_coverage.cpp
)

target_include_directories(integration_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/Simulator/include
  ${CMAKE_SOURCE_DIR}/UserCommon/include
  ${CMAKE_SOURCE_DIR}/common
  ${CMAKE_SOURCE_DIR}/build
)

if(TARGET GTest::gtest AND TARGET GTest::gtest_main)
  target_link_libraries(integration_tests PRIVATE GTest::gtest GTest::gtest_main UserCommon)
else()
  target_link_libraries(integration_tests PRIVATE gtest gtest_main UserCommon)
endif()

add_dependencies(integration_tests simulator_${SUBMITTER_IDS})

gtest_discover_tests(integration_tests)


