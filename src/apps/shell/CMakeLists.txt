set(TARGET shell)

add_app(${TARGET})
target_link_libraries(${TARGET} drivers)
