set(TARGET mocks)

file(GLOB_RECURSE SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE HEADER_LIST ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(${TARGET} STATIC ${SOURCE_LIST} ${HEADER_LIST})
target_compile_definitions(${TARGET} PRIVATE TESTING)
target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${TEST_TARGET_ROOT}/src/cpu/include
    ${TEST_TARGET_ROOT}/src/drivers/include
    ${TEST_TARGET_ROOT}/src/ebus/include
    ${TEST_TARGET_ROOT}/src/kernel/include
    ${TEST_TARGET_ROOT}/src/libc/include
    ${TEST_TARGET_ROOT}/src/libk/include
    ${TEST_TARGET_ROOT}/src/memory_alloc/include
    ${TEST_TARGET_ROOT}/src/process/include
    ${TEST_TARGET_ROOT}/src/util/include
)

target_precompile_headers(${TARGET} PUBLIC ${CMAKE_BINARY_DIR}/config.h)
target_link_libraries(${TARGET} PUBLIC fff)
