add_subdirectory(util)
add_subdirectory(memory_alloc)

add_subdirectory(cpu)
add_subdirectory(libk)
add_subdirectory(libc)
add_subdirectory(io)
add_subdirectory(ebus)
add_subdirectory(drivers)
add_subdirectory(process)
add_subdirectory(system_call)

# These must be after libs for linking
add_subdirectory(kernel)
add_subdirectory(boot)
add_subdirectory(apps)
