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

| Level   | Macro          | Usage                                               |
| ------- | -------------- | --------------------------------------------------- |
| Error   | `KLOG_ERROR`   | Something is wrong with the code                    |
| Warning | `KLOG_WARNING` | Someone is using the code incorrectly               |
| Info    | `KLOG_INFO`    | Information that's useful during normal operation   |
| Debug   | `KLOG_DEBUG`   | Information that's useful during troubleshooting    |
| Trace   | `KLOG_TRACE`   | I want to know what code is executing or some value |

### When to use Error

Use the Error level when the code must stop execution due to some condition.
This can be checking results of a function call or when performing input
validation. These messages should be short but descriptive avoiding format
variables where practical (ie. don't include all function args in the message).

Error messages typically mean something is wrong with the code.

Error should not be used when a return path is intended (eg. passing through
an error). If an error happens, there should only be 1 log message for it as
close to the error as possible. Any callers that receive the error should avoid
further logging if possible.

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
        // The error was already logged so no error log is needed. Use lower levels if needed.
        return 0;
    }
    ...
}
```

### When to use Warning

**TODO** this might get more use in place of ERROR

Use the Warning level for conditions that are unexpected or errors that do not
prevent further execution. This can be  These messages should be short but descriptive
avoiding format variables where practical (ie. don't include all function args
in the message).

Warning messages typically mean code is being used incorrectly.

**Example**

```c
int malloc(size_t size) {
    if (!size) {
        KLOG_WARNING("Malloc called with size 0");
    }
    ...
}
```

### When to use Info

**TODO** might need to split how DEBUG is being used so info gets more use

There aren't many examples of using the Info level. It's mostly reserved for
major state transitions or user events (eg. opening a connection or device).

**Example**

```c
KLOG_INFO("Loader Start");
KLOG_INFO("Kernel Start");
...
KLOG_INFO("Halting");
```

### When to use Debug

Use the debug level for state or decisions. These can include printf values when
relevant but logging many variables should use trace where possible. Avoid using
in loops or functions with high call frequency.

**Example**

```c
KLOG_DEBUG("vga init finished");
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
