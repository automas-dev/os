set(TARGET drivers)

cross_target(${TARGET})
target_link_libraries(${TARGET} libc cpu ebus kernel)
