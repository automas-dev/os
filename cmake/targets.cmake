function(cross_target target)
    # Setup a STATIC library including src/*.c src/*.asm and include/*.h recursively
    file(GLOB_RECURSE SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
    file(GLOB_RECURSE HEADER_LIST ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
    file(GLOB_RECURSE ASM_SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm)

    add_library(${target} STATIC ${ASM_SOURCE_LIST} ${SOURCE_LIST} ${HEADER_LIST})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(${target} PROPERTIES NASM_OBJ_FORMAT elf)
endfunction()

function(cross_target_binary target)
    # Build an executable binary from a STATIC library target
    # ie. cross_target must be called first on target
    get_link_libraries(TARGET_LINKS ${target})

    set(TARGET_LINK_FILES)
    foreach(item IN ITEMS ${TARGET_LINKS})
        list(APPEND TARGET_LINK_FILES $<TARGET_FILE:${item}>)
    endforeach()

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin
        COMMAND ${CMAKE_LINKER}
        "-T${CMAKE_CURRENT_SOURCE_DIR}/link.ld"
        --oformat binary
        -o "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
        --start-group
        $<TARGET_FILE:${target}>
        ${TARGET_LINK_FILES}
        --end-group
        -nostdlib
        "-L${CROSS_PREFIX}/lib/gcc/i386-elf/12.2.0"
        -lgcc
        DEPENDS ${target} ${TARGET_LINK_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)

    add_custom_target(${target}_image ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}.elf
        COMMAND ${CMAKE_LINKER}
        "-T${CMAKE_CURRENT_SOURCE_DIR}/link.ld"
        -o "${CMAKE_CURRENT_BINARY_DIR}/${target}.elf"
        --start-group
        $<TARGET_FILE:${target}>
        ${TARGET_LINK_FILES}
        --end-group
        -nostdlib
        "-L${CROSS_PREFIX}/lib/gcc/i386-elf/12.2.0"
        -lgcc
        DEPENDS ${target} ${TARGET_LINK_FILES})

    add_custom_target(${target}_image_debug ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.elf)
endfunction()

# ------------------------------------------------------------------------------
# SUPPORT FUNCTIONS

# https://stackoverflow.com/a/39127212
function(get_link_libraries OUTPUT_LIST TARGET)
    get_target_property(IMPORTED ${TARGET} IMPORTED)
    list(APPEND VISITED_TARGETS ${TARGET})
    if(IMPORTED)
        get_target_property(LIBS ${TARGET} INTERFACE_LINK_LIBRARIES)
    else()
        get_target_property(LIBS ${TARGET} LINK_LIBRARIES)
    endif()
    set(LIB_FILES "")
    foreach(LIB ${LIBS})
        if(TARGET ${LIB})
            list(FIND VISITED_TARGETS ${LIB} VISITED)
            if(${VISITED} EQUAL -1)
                get_target_property(type ${LIB} TYPE)
                if(NOT ${type} STREQUAL "INTERFACE_LIBRARY")
                    get_link_libraries(LINK_LIB_TARGET ${LIB})
                    list(APPEND LIB_FILES ${LIB} ${LINK_LIB_TARGET})
                endif()
            endif()
        endif()
    endforeach()
    set(VISITED_TARGETS ${VISITED_TARGETS} PARENT_SCOPE)
    set(${OUTPUT_LIST} ${LIB_FILES} PARENT_SCOPE)
endfunction()
