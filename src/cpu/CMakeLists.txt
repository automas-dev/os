set(TARGET cpu)

add_subdirectory(${CPU_ARCH}) # defined in root CMakeLists.txt
target_link_libraries(${TARGET} libk libc util kernel)
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
