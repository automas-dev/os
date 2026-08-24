# Style Guide for Code

<!-- Table of Contents only links to level 2 headers -->
\[ [Header Guards](#header-guards) \]
\[ [Function Names](#function-names) \]
\[ [Memory Ownership](#memory-ownership) \]

## Header Guards

All C code in src/ should use traditional guard style with the file path
(excluding src prefix) in upper snake case separated with underscores. A
comment should be included on the `#endif` line with the same name.

The name should be the file path under include/ (ie.
src/kernel/include/drivers/ata.h should have a header guard `DRIVERS_ATA_H`).

**Example**

src/kernel/include/kernel/device/ata.h

```c
#ifndef KERNEL_DEVICE_ATA_H
#define KERNEL_DEVICE_ATA_H
...
#endif // KERNEL_DEVICE_ATA_H
```

All C++ code or C code outside src/ should use `#pragma once`

```c++
#pragma once
...
```

## Function Names

Function names should include the module (ie. path) as a prefix using snake
case.

## Global Variables

Global variables should not be shared between files, but for several systems it
makes sense to have an internal state (eg. next pid counter, pointer to kernel
struct, etc.). Global variables should be limited to a single file and should be
declared as static. Constants (eg. log level name lookup) can use a const type
with upper snake case name. Variables should use lower snake case and have a
double underscore (`__`) prefix.

## Memory Ownership

TODO

