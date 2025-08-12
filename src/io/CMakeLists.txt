set(TARGET io)

cross_target(${TARGET})
target_link_libraries(${TARGET} libk libc kernel)
