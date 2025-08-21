set(TARGET kernel)

cross_target(${TARGET})
target_link_libraries(${TARGET} libk libc cpu ebus drivers util)
cross_target_binary(${TARGET})
