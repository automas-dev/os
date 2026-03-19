set(DRIVE_BASE_DIR ${CMAKE_BINARY_DIR}/drive)
execute_process(COMMAND mkdir -p ${DRIVE_BASE_DIR})

macro(add_app target)
    cross_target(${target})
    target_link_libraries(${target} cinit libc)
    cross_target_binary(${target})

    # Build raw binary for execution in os
    add_custom_command(OUTPUT ${DRIVE_BASE_DIR}/${target}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin ${DRIVE_BASE_DIR}/${target}

        # Output from cross_target_binary
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin)

    # Build elf executable for future use
    add_custom_command(OUTPUT ${DRIVE_BASE_DIR}/${target}.elf
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${target}.elf ${DRIVE_BASE_DIR}/${target}.elf

        # Output from cross_target_binary
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.elf)

    add_custom_target(${target}_app
        DEPENDS ${DRIVE_BASE_DIR}/${target} ${DRIVE_BASE_DIR}/${target}.elf)

    list(APPEND APPS_TARGETS ${target} ${target}_app)
    set(APPS_TARGETS ${APPS_TARGETS} PARENT_SCOPE)
endmacro()

# Code to run before calling app main (includes __start entrypoint)
add_subdirectory(cinit)

# Programs loaded from disk
add_subdirectory(foo)
add_subdirectory(bar)
add_subdirectory(demo)
add_subdirectory(echo)
add_subdirectory(init)
add_subdirectory(ping)
add_subdirectory(pong)
add_subdirectory(shell)

message("App targets are ${APPS_TARGETS}")

add_custom_command(
    OUTPUT ${DRIVE_BASE_DIR}/data/
    COMMAND rm -rf ${DRIVE_BASE_DIR}/data/
    COMMAND mkdir -p ${DRIVE_BASE_DIR}/data/
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/drive/* ${DRIVE_BASE_DIR}/data
    DEPENDS ${CMAKE_SOURCE_DIR}/drive/)

add_custom_target(apps_data
    DEPENDS ${DRIVE_BASE_DIR}/data/)

# Delete files in build/apps/data so it can be removed
set_property(
    TARGET apps_data
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${DRIVE_BASE_DIR}/data/)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/apps.tar
    COMMAND tar -cf ${CMAKE_BINARY_DIR}/apps.tar *
    DEPENDS ${APPS_TARGETS} apps_data
    WORKING_DIRECTORY ${DRIVE_BASE_DIR})

add_custom_target(apps_image ALL
    DEPENDS ${CMAKE_BINARY_DIR}/apps.tar)
