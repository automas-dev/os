# WIP - Driver

Loading drivers is a future problem. Drivers will be built into the kernel and
the kernel will know about each of them after compilation.

I have two versions of io drivers from previous branches. One (older) is an
entire plugin system with generic driver interfaces and everything. It's a bit
complex and I would change some of the structures and potentially simplify it.

The second (newer) is essentially the same as the current drivers with an io and
fs interface + plugin system. I'm not a huge fan of ...

```c
size_t read(ptr, buff, size);
size_t write(ptr, buff, size);

size_t read(ptr, buff, size, pos);
size_t write(ptr, buff, size, pos);

size_t seek(pos, anchor);
size_t tell();
```

What can't libc do?

- Allocate heap pages
- Buffers
  - read/write
  - size of
  - open/close?
- File
  - stat file
  - list dir
  - create/move/delete
  - permissions
- launch process
- yield
- time
- network?
- user input
- display output
- signals
- threads?

Types of buffers

- File (abstract) r/w
- Directory (abstract) r/w
- Filesystem (abstract) r/w or ro
- Disk r/w
- Ram
- VGA
- Pipe?

# System Call

Getting libc to be more independent.

It shouldn't call the kernel for malloc, it should just call page alloc and page
free. malloc should live in libc instead of memory_alloc.
