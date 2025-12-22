set(TARGET libc)

cross_target(${TARGET})
target_link_libraries(${TARGET} libk ebus)
