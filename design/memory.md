# Memory Map

## Real Mode

See [https://wiki.osdev.org/Memory_Map_(x86)](https://wiki.osdev.org/Memory_Map_(x86))
for reserved BIOS memory.

| start  | end     | size      | description                 |
| ------ | ------- | --------- | --------------------------- |
| 0x0000 | 0x4ff   | 1.25 KiB  | Unusable                    |
| 0x0500 | 0x00fff | 1.25 KiB  | Boot Parameters             |
| 0x1000 | 0x06fff | 24 KiB    | Stack (real mode), top down |
| 0x7000 | 0x07bff | 3 KiB     | Unused                      |
| 0x7c00 | 0x07dff | 512 bytes | Boot Sector                 |
| 0x7e00 | 0x9fbff | 607.5 KiB | Kernel (second stage)       |

> [!IMPORTANT]
> Kernel Size in Protected Mode
> Reserved memory in protected mode starts at 0x9fc00 while real mode starts at
> 0xa0000

### Boot Parameters

Boot parameters are set by the bootloader and passed to the second stage kernel
at address 0x500.

| start | size | description        |
| ----- | ---- | ------------------ |
| 0     | 2    | Low Memory Size    |
| 2     | 2    | Memory Entry Count |
| 4     | x    | Memory Map Entries |

x is the value of Memory Entry Count * 24

#### Memory Entry

There is a single memory entry for each region of memory. It is possible to have
out of order and overlapping regions. See [BIOS Function: INT 0x15, EAX =
0xE820](https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_EAX_.3D_0xE820)

| start | size | description  |
| ----- | ---- | ------------ |
| 0     | 8    | Base Address |
| 8     | 8    | Length       |
| 16    | 4    | Type         |
| 20    | 4    | Ext          |

Region Type can be one of the following

| Type | Description      | Usable   |
| ---- | ---------------- | -------- |
| 1    | Usable RAM       | Usable   |
| 2    | Reserved         | Reserved |
| 3    | ACPI Reclaimable | Usable   |
| 4    | ACPI NVS         | Reserved |
| 5    | Bad Memory       | Reserved |

ACPI 3.0 Extended Attributes

| bit    | description                  |
| ------ | ---------------------------- |
| 0      | entry should be ignored if 0 |
| 1      | entry is non-volatile if 1   |
| 2 - 31 | undefined                    |

## Protected Mode

| start   | end     | size      | description           |
| ------- | ------- | --------- | --------------------- |
| 0x00000 | 0x004ff | 1.25 KiB  | Unused                |
| 0x00500 | 0x00fff | 2.75 KiB  | Boot Parameters       |
| 0x01000 | 0x01fff | 4 KiB     | Page Directory        |
| 0x02000 | 0x02fff | 4 KiB     | ram region table      |
| 0x03000 | 0x06fff | 16 KiB    | Stack                 |
| 0x07000 | 0x07bff | 3 KiB     | Unused                |
| 0x07c00 | 0x07dff | 512 bytes | GDT                   |
| 0x07e00 | 0x9fbff | 607.5 KiB | Kernel (second stage) |
| ...     | ...     | ...       | ...                   |
| 0xb8000 | 0xb8fff | 0x01000   | VGA Memory            |

> [!IMPORTANT] Kernel Size in Protected Mode
> Reserved memory in protected mode starts at 0x9fc00 while real mode starts at
> 0xa0000

### Virtual Address Space (stage 2)

See https://wiki.osdev.org/Paging for information about page tables and
directory.

- The first page (0) is always null, accessing any address here will result in a
  (page fault?)
- Each page directory and table contain 1024 entries
- 0x1000 will always point to the active page directory
- 0x2000 will always point to the ram region table
- 0xb9000 will always point to the first ram region bitmasks
  - There are 512 sequential pages of ram bitmasks ending at 0x2b9fff
- _0x400000 is the first virtual address of the second page table_
  - It is suggested to keep kernel level memory bellow this address to allow
    user space application switching out the second+ page table
  - This can be used as the entry point address for all user space applications
  - This region is ~3.99 GB
- 0xffc00000 will always point to the first page table (kernel table)
  - This goes up to 0xffffffff as the last address in all of virtual space
  - Each of the 1024 tables from the page directory are stored here sequentially
  - The first table includes the null page and kernel memory mapping (see bellow)

| start      | end        | pages      | physical addr | description                                               |
| ---------- | ---------- | ---------- | ------------- | --------------------------------------------------------- |
| 0x00000000 | 0x00000fff | 0x00001    | 0x00000000    | null page (not present)                                   |
| 0x00001000 | 0x00001fff | 0x00001    | 0x00001000    | Page Directory                                            |
| 0x00002000 | 0x00002fff | 0x00001    | 0x00002000    | ram region table                                          |
| 0x00003000 | 0x00006fff | 0x00004    | 0x00003000    | Stack                                                     |
| 0x00007000 | 0x0009efff | 0x00098    | 0x00007000    | Kernel (from 0x7e00 to 0x9efff)                           |
| 0x0009f000 | 0x000b7fff | 0x00019    |               | _temp pages for mapping_                                  |
| 0x000b8000 | 0x000b8fff | 0x00001    | 0x000b8000    | VGA Memory                                                |
| 0x000b9000 | 0x000b9fff | 0x00001    |               | First page table (kernel's page) of any page directory    |
| 0x000ba000 | x - 1      | <= 0x00200 |               | ram region bitmasks                                       |
| x          | y - 1      |            |               | _free memory for kmalloc (remainder of first page table)_ |
| y          | 0x003fffff |            |               | _kernel stack (grows down)_                               |
| 0x00400000 | 0xffffffff | 0xffb00    |               | _free memory for user (second+ page tables)_              |

_Pages with a blank physical address are allocated form free physical memory._

### Virtual Address Space (Process)

Applications / processes require access to less of the memory than the kernel.
With the use of interrupts for system calls and page directories for each
process,

## Physical Allocator (`ram.h`)

Physical pages are tracked by a region table. A region table has one entry for
each region of free pages where the first page is a bitmask with one bit per
following page. The first bit is for the bitmask itself, which is included in
the total page count for the region. This first bit will always be 0, and the
number of free pages will always been 1 less than the total number of pages.

### Region Table

Regions are used to track all pages. Each region has up to 32768 continuous
pages (128 MiB) where the first page is a bitmask of all free pages in that
region. All regions are tracked by a Region Table which holds up to 512 region
entries. Each entry in the region table also includes a flag if the region is
present and counts of the total number of pages in the region and number of free
pages in the region.

| start | size | description     |
| ----- | ---- | --------------- |
| 0     | 4    | address + flags |
| 4     | 2    | page count      |
| 8     | 2    | free count      |

>[!IMPORTANT] Remember to mask the address
> The low 12 bits of each address are flags because the address is always page
> aligned. Remember to mask these bits when using the address.

> [!NOTE] Region size
> Each region can be up to 128 MiB (32768 pages)
>- Page size = 4096 (0x1000)
>- Bits per page = Page Size * 8 = 4096 * 8 = 32768 (0x8000)
>- Max pages per region = Bits per page = 32768 (0x8000) (includes bitmask page)
>- Max region size = Max pages per region * Page size = 32768 * 4096 = 134217728
>  (0x8000000) = 128 MiB

#### Flags

| flag | description                                                |
| ---- | ---------------------------------------------------------- |
| 0x1  | Present - this region table entry points to a valid region |

### Region Bitmask

The first page of each region (pointed to by the region table entry) is a
bitmask for the region showing which pages are free. Any bits outside of the
region should be set to 0 (ie. if the region is smaller than 128 MiB, bits for
any pages after the region end should be 0).

> [!IMPORTANT] Bitmask bit 1
> The first bit of the bitmask will always be 0 for the bitmask page itself.

## Paging Allocator (`memory.h`)

Paging allocator (aka `pmalloc` and `pfree`) is responsible for connecting the
physical memory allocator (ram) and page tables (mmu). This allocator keeps a
linked list of tables, 1022 entries each.

Each entry of the table describes a region of memory with address, flags and
size in bytes (page aligned).

### Memory Table

There are a total of 511 entries per table.

| start | size | description   |
| ----- | ---- | ------------- |
| 0     | 4    | next          |
| 4     | 4    | prev          |
| 8     | 4088 | entries x 511 |

### Memory Table Entries

The first 12 bits of a table entry are flags. The page number can be converted
to a memory address by shifting it left by 12 bits, or by using the max
(0xfffff000). Because the size is page aligned, the same strategy can be used
for getting the page number or memory address.

| start | size | description                            |
| ----- | ---- | -------------------------------------- |
| 0     | 12   | flags                                  |
| 12    | 20   | page no (address if flags masked away) |
| 32    | 32   | size in bytes (page aligned)           |

> [!IMPORTANT] Size is page aligned
> The size is page aligned such that a page no / address + size points to the
> next valid, page aligned, address.

> [!NOTE] Size has unused bits
> Because the size is page aligned, the first 12 bits should always be 0

#### Flags

All entries before the last must be present. The first entry that does not have
the present flag set will be treated as the end of all entries.

| flag | description                             |
| ---- | --------------------------------------- |
| 0x1  | Present - this is a valid entry         |
| 0x2  | Free (1 if memory is free, 0 if in use) |

### Algorithm

- Adjacent free memory entries can be merged.
- Large free memory entries can be split to allocate smaller requests
  - Maybe this is when a request is less than half of the free space?
- Memory tables can be moved and removed. Only a pointer to the first table
  should be stored. All other tables are reachable via the linked list.

TODO - everything else here
