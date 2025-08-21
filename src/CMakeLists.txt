add_subdirectory(util)

add_subdirectory(cpu)
add_subdirectory(libc)
add_subdirectory(ebus)

# These must be after libs for linking
add_subdirectory(kernel)
add_subdirectory(boot)
add_subdirectory(apps)
