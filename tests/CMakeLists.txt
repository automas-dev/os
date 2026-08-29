cmake_minimum_required(VERSION 3.15..3.20)
project(os_test VERSION 0.1.0 LANGUAGES C CXX)

option(OS_TEST_COVERAGE "Generate gcov target for calculating code coverage" ON)
option(OS_TEST_COVERAGE_CAN_FAIL "Tests can fail for coverage" ON)
option(OS_TEST_COVERAGE_DARK "Use dark theme for html output" ON)

set(TEST_TARGET_ROOT ${CMAKE_SOURCE_DIR}/..)
set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_FLAGS -m32)
set(CMAKE_C_STANDARD 17)

# Set the global FFF_GCC_FUNCTION_ATTRIBUTES for config.h
set(FFF_GCC_FUNCTION_ATTRIBUTES "__attribute__((weak))")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)
include_directories(PRIVATE ${CMAKE_BINARY_DIR}/include)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if(OS_TEST_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    if(OS_TEST_COVERAGE_CAN_FAIL)
        set(GCOVR_ADDITIONAL_ARGS ${GCOVR_ADDITIONAL_ARGS} --html-high-threshold 100 --html-medium-threshold 90 --fail-under-line 100 --fail-under-branch 100)
    endif()
    if(OS_TEST_COVERAGE_DARK)
        set(GCOVR_ADDITIONAL_ARGS ${GCOVR_ADDITIONAL_ARGS} --html-theme github.dark-green)
    else()
        set(GCOVR_ADDITIONAL_ARGS ${GCOVR_ADDITIONAL_ARGS} --html-theme github.green)
    endif()
endif()

add_subdirectory(external)

# ------------------------------------------------------------------------------
# TESTS

include(CTest)
enable_testing()

add_subdirectory(src)

if(OS_TEST_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    setup_target_for_coverage_gcovr_html(
        NAME ${PROJECT_NAME}_coverage
        EXECUTABLE ctest
        EXECUTABLE_ARGS --output-on-failure
        DEPENDENCIES ${PROJECT_NAME}_all
        BASE_DIRECTORY ../
        EXCLUDE ${PROJECT_SOURCE_DIR}/* ${PROJECT_BINARY_DIR}/*
    )
endif()
