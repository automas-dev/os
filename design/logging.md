# Logging

<!-- Table of Contents only links to level 2 headers -->
\[ [Usage](#usage) \]

The kernel logging system provides convenient, standardized and configurable
logging.

Features include

- Formatted log messages with time, level, file name / line number, service and message
- printf style string formatting
- Lazy evaluation of string formatting
- Macros to exclude log levels at compile time
- Runtime configurable log level
- File and level service names
- Compile time configurable file level logging level

> [!IMPORTANT]
> Currently all logging is written to serial port 1. No logs are retained in
> memory or on disk.

## Usage

Each C file should define `KLOG_SERVICE` as the first line in the file. This
must be defined before including `"kernel/logs.h"`. The service name should be
all uppercase with the component name and file path under the components src/
separated by `/` (eg. src/kernel/src/kernel/memory.c would be `KERNEL/MEMORY`
excluding the src/kernel/src prefix).

```c
#define KLOG_SERVICE "KERNEL/MEMORY"
#include "kernel/logs.h"
...
```

To modify the compile time log level, also define `KLOG_LEVEL` in the file
before including `"kernel/logs.h"`.

```c
#define KLOG_SERVICE "KERNEL/MEMORY"
#define KLOG_LEVEL KERNEL_LOG_LEVEL_TRACE  // default level is debug
#include "kernel/logs.h"
...
```

Logging macros use printf style string formatting. The message should not
include newlines (those are appended by the logging system during formatting).

```c
KLOG_INFO("message %u name %s", 42, "hello world");
```

The logging system is limited to the kernel and should not be used in
application or user space code.

| Level   | Macro          | Usage                                                                                                        |
| ------- | -------------- | ------------------------------------------------------------------------------------------------------------ |
| Error   | `KLOG_ERROR`   | An internal invariant was broken or an operation that should always succeed failed                           |
| Warning | `KLOG_WARNING` | The calling code passed bad or unexpected input, handled gracefully                                          |
| Info    | `KLOG_INFO`    | A resource's lifecycle changed (opened/closed, started/stopped)                                              |
| Debug   | `KLOG_DEBUG`   | Information that's useful during troubleshooting, including short context when catching a propagated failure |
| Trace   | `KLOG_TRACE`   | I want to know what code is executing or some value                                                          |

### When to use Error

Use Error when an internal invariant is broken or an operation that should
always succeed unexpectedly fails (eg. allocation failure, corrupted internal
state, a required kernel resource missing). Error signals a kernel bug or
unrecoverable resource exhaustion, not bad input from a caller. These messages
should be short but descriptive, avoiding too many format variables where
practical (ie. don't include all function args in the message if not relevant).

**Example**

```c
int memory_init(memory_t * mem, memory_alloc_pages_t alloc_pages_fn) {
    mem->first = alloc_pages_fn(1);
    if (!mem->first) {
        KLOG_ERROR("Failed to allocate first page");
        return -1;
    }
    ...
}
```

### When to use Warning

Use Warning in kernel functions when the calling code passes bad / invalid
inputs, and the function handles it gracefully (eg. null argument bad syscall
arguments). These messages should be short but descriptive, avoiding too many
format variables where practical (ie. don't include all function args in the
message if not relevant).

**Example**

```c
int malloc(size_t size) {
    if (!size) {
        KLOG_WARNING("Malloc called with size 0");
    }
    ...
}
```

### Logging error chains (propagation)

An error should be logged exactly once, as close to the origin of the fault as
possible, at whichever level (Error or Warning) applies per the sections above.
A function that merely forwards a failure from a callee (eg. by returning 0,
NULL, or a negative status code without changing its meaning) must not repeat an
Error or Warning log for the same failure.

A caller catching a propagated failure may add a `KLOG_DEBUG` line naming the
higher-level operation that failed, to keep a readable call chain in verbose
logs. It should not use the Error/Warning level.

**Example**

```c
int open_something() {
    int id = ata_device_open();
    if (!id) {
        KLOG_ERROR("Failed to open ata device");
        return 0;
    }
    return id;
}
int check_something(const char * path) {
    int id = open_something(path);
    if (!id) {
        // Optional context only, the real cause was already logged by open_something
        KLOG_DEBUG("check_something failed because open_something failed for %s", path);
        return 0;
    }
    ...
}
```

### When to use Info

Use Info for a resource's lifecycle transitions (eg. opening/closing a device,
init/teardown of a subsystem). Both sides of a matched lifecycle pair (open and
close, init and free) should use the same level. Reserve Info for lifecycle
events that happen a limited number of times, not high-frequency internal
operations.

**Example**

```c
KLOG_INFO("Loader Start");
KLOG_INFO("Kernel Start");
KLOG_INFO("Halting");

KLOG_INFO("Opening ATA drive 0");
KLOG_INFO("Closing ATA drive 0");
```

### When to use Debug

Use the debug level for state or decisions, and for the optional propagation
context line described above. These can include printf values when relevant but
logging many variables should use trace where possible. Avoid using in loops or
functions with high call frequency.

**Example**

```c
KLOG_DEBUG("Using 4 KiB pages for kernel heap");
```

### When to use Trace

Use the trace level for information useful during development or debugging, but
considered noise during normal operation. These log messages are pruned at
compile time by default so they do not incur any additional costs at runtime.

> [!IMPORTANT]
> Try to avoid calling calling expensive functions as parameters to trace level
> messages.

**Example**

```c
KLOG_TRACE("Freed temporary page %u with virtual address %p from physical address %p", i, vaddr, paddr);
```
