set(TARGET cinit)

cross_target(${TARGET})
target_link_libraries(${TARGET} libc)
