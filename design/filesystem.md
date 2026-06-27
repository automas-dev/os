# Filesystem

- Blocks are 64 bytes each
- Block groups have 256 blocks (max addressable with 8 bits)

## Block Address (BA)

Each block is addressed as it's index in the block group and the index of the
block group. The address is 4 bytes where the low 8 bits are the block index in
a group, the next 22 bits are the group index and the last 2 bits are the block
type.

|    bits | name        |
| ------: | ----------- |
|   0 - 8 | block index |
|  9 - 30 | group index |
| 31 - 31 | type        |

The type show what the block type is.

| type | name        |
| ---: | ----------- |
|    1 | dnode block |
|    2 | inode block |
|    3 | data        |

Example to get decode an address

```c
block = loc & 0xff
group = (loc >> 8) & 0x3fff
type = loc >> 30
```

Example to encode an address

```text
address = block
address |= (group << 8)
address |= (type << 30)
```

C structs support bitfields to assign bits to fields. An example struct could
be.

```c
typedef struct {
    uint8_t block : 8;
    uint32_t group : 22;
    uint8_t type : 2;
} loc_t;
```

## Superblock

The root dnode is the first dnode in a dnode block. The root id is always 1 (0
is reserved to indicate invalid parent). root has a parent of 0 and a null ptr
for name.

| start | size | name                      |
| ----: | ---: | ------------------------- |
|     0 |    4 | magic 0x53467366 ("fsFS") |
|     4 |    4 | block_size (in bytes)     |
|     8 |    4 | block count per group     |
|    12 |    4 | block group count         |
|    16 |    4 | root dnode block address  |
|    20 |   44 | (unused)                  |

## Directory Node (dnode)

The name of a dnode is a single block as a 0 terminated string. Therefore the
max name length is 63 (block size - 1 (0 terminated)).

| start | size | name                                         |
| ----: | ---: | -------------------------------------------- |
|     0 |    4 | id                                           |
|     4 |    4 | parent id                                    |
|     8 |    4 | name str block address                       |
|    12 |    4 | additional (indirect) children block address |
|    16 |   16 | children (4 bytes for each = 4)              |

Children can be found by first collecting the list of children (should be
unique) and then searching each child block for nodes with self as parent.

### Children list

Each child in the list is 4 bytes as a block address for the node block that
contains the children.

| start | size | name                    |
| ----: | ---: | ----------------------- |
|     0 |    4 | node list block address |

### Indirect child block

The dnode block can point to an additional block to list more children. This
block is prefixed by the location of the next block or 0 if this is the last.
Followed by a list of children ids.

| start | size | name                                                 |
| ----: | ---: | ---------------------------------------------------- |
|     0 |    4 | next indirect child list block address or 0 for last |
|     4 |   60 | children (4 bytes for each = 15)                     |

## File Node (inode)

The name of a dnode is a single block as a 0 terminated string. Therefore the
max name length is 63 (block size - 1 (0 terminated)).

| start | size | name                                |
| ----: | ---: | ----------------------------------- |
|     0 |    4 | id                                  |
|     4 |    4 | parent id                           |
|     8 |    4 | size in bytes                       |
|    12 |    4 | name str block address              |
|    16 |    4 | additional (indirect) block address |
|    20 |   44 | direct blocks (4 bytes for each)    |

See the dnode docs for data blocks and indirect block address (called children
for dnode).

## Block Group

The first block of each group is it's header and contains a bitmask for free and
used blocks. A bit mask value of 1 is free while a value of 0 is used. The least
significant bit is always 0 for the block group header. Because each block group
has 256 blocks, the bitmask is 256 bits (32 bytes) long.

| start | size | name                             |
| ----: | ---: | -------------------------------- |
|     0 |   32 | bitmask for free and used blocks |

# Formatting

1. x Read disk size
2. x Calculate count of block groups
3. x Generate superblock
4. x Generate first block group
5. x Generate first dnode list
6. x Generate root dnode
7. Generate remaining block groups

# USTAR

| Type flag          | Meaning           |
| ------------------ | ----------------- |
| '0' or (ASCII NUL) | Normal file       |
| '1'                | Hard link         |
| '2'                | Symbolic link     |
| '3'                | Character device  |
| '4'                | Block device      |
| '5'                | Directory         |
| '6'                | Named pipe (FIFO) |

| Offset | Size | Description                                                |
| ------ | ---- | ---------------------------------------------------------- |
| 0      | 100  | File name                                                  |
| 100    | 8    | File mode                                                  |
| 108    | 8    | Owner's numeric user ID                                    |
| 116    | 8    | Group's numeric user ID                                    |
| 124    | 12   | File size in bytes (octal base)                            |
| 136    | 12   | Last modification time in numeric Unix time format (octal) |
| 148    | 8    | Checksum for header record                                 |
| 156    | 1    | Type flag                                                  |
| 157    | 100  | Name of linked file                                        |
| 257    | 6    | UStar indicator "ustar" then NUL                           |
| 263    | 2    | UStar version "00"                                         |
| 265    | 32   | Owner user name                                            |
| 297    | 32   | Owner group name                                           |
| 329    | 8    | Device major number                                        |
| 337    | 8    | Device minor number                                        |
| 345    | 155  | Filename prefix                                            |
