set(TARGET system_call)

cross_target(${TARGET})
# TODO add all dependencies, currently using kernel
target_link_libraries(${TARGET} libk libc kernel)
