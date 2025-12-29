cmake_minimum_required(VERSION 3.15..3.20)

set(CPU_ARCH i386)

set(CROSS_PREFIX "$ENV{HOME}/.local/opt/cross")
set(CMAKE_TOOLCHAIN_FILE cmake/toolchain.cmake)

project(os VERSION 0.1.0 LANGUAGES C)

# set(CMAKE_VERBOSE_MAKEFILE on)

# ------------------------------------------------------------------------------
# NASM SETUP

# https://stackoverflow.com/a/56434253

enable_language(ASM_NASM)
if(CMAKE_ASM_NASM_COMPILER_LOADED)
    set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

    # Create a compile option that operates on ASM_NASM files
    # If the target has a property NASM_OBJ_FORMAT, use it, otherwise
    # use the environment variable CMAKE_ASM_NASM_OBJECT_FORMAT
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:ASM_NASM>:-f $<IF:$<BOOL:$<TARGET_PROPERTY:NASM_OBJ_FORMAT>>, \
        $<TARGET_PROPERTY:NASM_OBJ_FORMAT>, ${CMAKE_ASM_NASM_OBJECT_FORMAT}>>"
    )
endif()

# Create config.h with project version numbers
string(LENGTH "${PROJECT_SOURCE_DIR}/src/" FILE_PREFIX_LENGTH)
configure_file(cmake/config.h.in include/config.h)
include_directories(PRIVATE ${CMAKE_BINARY_DIR}/include)

include(cmake/targets.cmake)

# set KERNEL macro for build time
add_compile_options(-DKERNEL)

add_subdirectory(src)

# ------------------------------------------------------------------------------
# OS IMAGE

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/os-image.bin
    COMMAND cat $<TARGET_OBJECTS:boot> ${CMAKE_BINARY_DIR}/src/kernel/kernel.bin > ${CMAKE_BINARY_DIR}/os-image.bin
    COMMAND echo Size of os image is
    COMMAND du -h ${CMAKE_BINARY_DIR}/os-image.bin
    DEPENDS
    $<TARGET_OBJECTS:boot>
    ${CMAKE_BINARY_DIR}/src/kernel/kernel.bin
)

add_custom_target(os_image ALL
    DEPENDS ${CMAKE_BINARY_DIR}/os-image.bin
)
