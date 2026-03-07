# os

Hobby i386 kernel / operating system.

- [Active and planned work](notes.md)
- [Design concepts and implementation references](design/)

This project started by following the tutorials under
[https://github.com/cfenollosa/os-tutorial](https://github.com/cfenollosa/os-tutorial)
but has diverged quite a bit since. Some components are still mostly the same
(eg. [src/boot/](src/boot/)).

## Goals

The OS will be finished when all of the following are implemented.

1. Able to get and draw basic http web page
2. Compiler for at least Assembly or C
3. Playable Minecraft Clone

The following lists show the planned development stages and planned tasks to
achieve these goals.

### Near Near Term

- [x] Read extended kernel form floppy
- [x] Switch to protected mode
- [x] some stdio.h
- [x] printf
- [x] some string.h
- [x] keyboard driver
- [x] ata driver
- [x] terminal with command parsing
- [x] circular buffer + tests (builtin, needs separating)

### Near Term

- [ ] Kernel stats (stats command returns # disk io, keys, mounts, etc.)
- [ ] Detect stack overflows
- [ ] fs
  - [ ] Finish implementing fs functions
  - [ ] Implement file io
- [x] basic malloc (linked list)
- [x] malloc
  - [x] Actually manage memory
  - [x] Need to find end of kernel to not overflow at runtime (for malloc start)
  - [x] Detect max memory for malloc
  - [x] Do that fancy memory map
- [x] Paging
  - [x] Setup page dir and table
  - [x] Enter paging
- [x] Load user space application
- [ ] Ring 3
  - [x] Kernel Service Calls
- [ ] Date and Time
- [ ] Optional don't disable interrupts during interrupt (nested interrupts)
- [x] Better key event buffer (with mods) (maybe in addition to char buffer)
- [x] Change stdlib names with k prefix for namespace during testing
- [ ] Optimize disk read to check if area already in buffer

### Long Term

These tasks will be broken down further after completion of Near Term, when
there is a better foundation and design to work with.

- [ ] Switch to 2d graphics mode
- [x] Virtual memory pages / memory paging
- [ ] Memory permissions (eg. stack can't exec, code can't write)
- [ ] User level applications (might need 3rd level kernel from filesystem)
- [ ] Testing
- [ ] FAT or EXT2 filesystem driver
- [ ] Audio Driver
- [x] 64 bit support printf (needs libgcc)
- [ ] Text editor

### Long Long Term

These tasks will be broken down further after completion of Long Term, when
there is enough support for their development.

- [ ] Graphics card driver (pseudo opengl)
- [ ] Threading / multi-process
- [ ] (Maybe) Multiboot or GRUB?

## Setup

You will need to install the gcc i386 elf cross compiler. This project expects
this to be under `~/.local/opt/cross` but this can be changed by editing the
root `CMakeLists.txt`. For instructions of how to build the i386 elf compiler
from source, follow the instructions on the OS-Dev Wiki
[GCC_Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)

```sh
git clone git@github.com:automas-dev/os.git
cd os
make setup
```

### Building

The os image can be found at `build/os-image.bin`, with an elf for the kernel
at `build/src/kernel/kernel.elf` for debug symbols. Apps are combined in a
tar file at `build/apps.tar` which is mounted by the kernel at runtime.

```sh
make build
```

### Running

QEMU is used for cpu emulation and kernel execution. I'm using QEMU emulator
version 10.1.0 (Debian 1:10.1.0+ds-5ubuntu2.4) but other versions should work.
Logs will be written to stdout and `kernel.log` Qemu logs are written to
`qemu_logs.txt`

```sh
make run
```

Once running, use the `help` command to see what you can do.

Kernel log level can be adjusted in
[src/kernel/src/loader.c](src/kernel/src/loader.c) in the function `__start`

### Testing

```sh
make test
make test_cov
make test_cov DARK_MODE=ON
```

### Formatting / Linting

```sh
make lint
make format
```
