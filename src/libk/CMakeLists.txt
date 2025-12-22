set(TARGET libk)

cross_target(${TARGET})
target_link_libraries(${TARGET} util ebus)
