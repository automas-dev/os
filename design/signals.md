# WIP - Signals

Signals allow the kernel to communicate to the process using callbacks. To
handle communication form process to kernel, use [system calls](system_call.md).

TODO

- [ ] Signal callback signature
- [ ] Registering callback
- [ ] Sending signals from kernel

# Old

The `register_signals` call will hook a function in libc to receive all signals.
It will then store all registered callbacks of the process.

TODO - keyboard event
