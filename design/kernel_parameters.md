# Kernel Parameters
<!-- Remember to update comment in cmake/config.h.in if this file moves -->

<!-- TODO add TOC if sections are added -->

The build system communicates with the kernel through a generated `config.h` (
generated from `cmake/config.h.in`).

| Parameter                              | Description                                                                           |
| -------------------------------------- | ------------------------------------------------------------------------------------- |
| PROJECT_NAME                           | Assigned in the root `CMakeLists.txt`                                                 |
| PROJECT_VERSION                        | Full semantic version from the root `CMakeLists.txt`                                  |
| PROJECT_VERSION_MAJOR                  | Semantic version major number                                                         |
| PROJECT_VERSION_MINOR                  | Semantic version minor number                                                         |
| PROJECT_VERSION_PATCH                  | Semantic version patch number                                                         |
| CPU_ARCH                               | CPU Architecture                                                                      |
| FILE_PREFIX_LENGTH                     | Length of absolute path to `src/`, used in logger macro to isolate relative file path |
| KERNEL_MEMORY_INTEGRITY_CHECKS_ENABLED | Perform extra checks during malloc operations                                         |
| KERNEL_MAX_STACK_SIZE_KB               | Default maximum size in KB a process' stack may grow to (seeds `process_t.max_stack_pages`, rounded up to a whole page; can be changed per-process at runtime via `process_set_max_stack_size_kb`) |
| PROJECT_DESCRIPTION                    | String that combines name, version and arguments to the kernel                        |
