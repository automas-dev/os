# Programmable Interval Timer (PIT)

PIT has 3 channels

| Channel | Use       | Port |
| ------- | --------- | ---- |
| 0       | IRQ 0     | 0x40 |
| 1       | _Unused_  | 0x41 |
| 2       | Audio Out | 0x42 |

The mode / command port is 0x43.

Each channel port is used to interact with each channel doing tasks such as setting
the reload value, reading the current count, etc.

Each channel counts down, starting at the reload value, and when they reach 0
the output is switched (low -> high or high -> low). In most cases the channel
is then set to the reload value. 

TODO in what cases is the reload value not used when a channel reaches 0?

> [!NOTE] Reload Value
> The reload value of a channel is the value it will set it's count to after
> reaching zero. Depending on the mode, that reset can be triggered by various
> conditions.

## Mode / Command Register `0x43`

| Bits    | Usage                                                     |
| ------- | --------------------------------------------------------- |
| 7 and 6 | Select Channel                                            |
|         | 00 = Channel 0                                            |
|         | 01 = Channel 1                                            |
|         | 10 = Channel 2                                            |
|         | 11 = Read back command                                    |
| 5 and 4 | Access Mode                                               |
|         | 00 = Latch count value command                            |
|         | 01 = low byte only                                        |
|         | 10 = high byte only                                       |
|         | 11 = low and high bytes                                   |
| 3 to 1  | Operating Mode                                            |
|         | 000 = Mode 0 (interrupt on terminal count)                |
|         | 001 = Mode 1 (hardware re-triggerable one-shot)           |
|         | 010 = Mode 2 (rate generator)                             |
|         | 011 = Mode 3 (square wave generator)                      |
|         | 100 = Mode 4 (software triggered strobe)                  |
|         | 101 = Mode 5 (hardware triggered strobe)                  |
|         | 110 = Mode 2 (rate generator, same as 010)                |
|         | 111 = Mode 3 (square wave generator, same as 011)         |
| 0       | BCD / Binary Mode (0 = 16-bit binary, 1 = four digit BCD) |

## Modes

### Mode 0 - Interrupt on Terminal Count

_only channel 0_

This mode will wait until a software trigger to begin the countdown.

TODO does the countdown happen with the clock or on each sw trigger?

### Mode 1 - Hardware Re-triggerable One-Shot

_only channel 2_

### Mode 2 - Rate Generator

This works as a frequency divider. For channel 0, when the counter reaches 0 it
will trigger an irq 0 before resetting the counter to the reset value. This will
produce a steady sequence of "ticks" that can be counted to track time.

### Mode 3 - Square Wave Generator


### Mode 4 - Software Triggered Strobe


### Mode 5 - Hardware Triggered Strobe


## Read Back Command

The mode / command port uses different flags in read back mode (vs all other
channels). Bits 7 and 6 will both be set, because these are the channel bits.

Multiple counters can be read with a single command.

| Bits    | Usage                                      |
| ------- | ------------------------------------------ |
| 7 and 6 | 11                                         |
| 5       | Latch count flag (0 = latch, 1 = unlatch)  |
| 4       | Latch status flag (0 = latch, 1 = unlatch) |
| 3       | Read back channel 2 (1 = yes, 0 = no)      |
| 2       | Read back channel 1 (1 = yes, 0 = no)      |
| 1       | Read back channel 0 (1 = yes, 0 = no)      |
| 0       | _reserved, always 0_                       |

### Read Back Status

After sending the read back command, read the port for each channel that was
selected in the command.

| Bits    | Usage                                                     |
| ------- | --------------------------------------------------------- |
| 7       | Output pin state                                          |
| 6       | Null count flag                                           |
| 5 and 4 | Access Mode                                               |
|         | 00 = Latch count value command                            |
|         | 01 = low byte only                                        |
|         | 10 = high byte only                                       |
|         | 11 = low and high bytes                                   |
| 3 to 1  | Operating Mode                                            |
|         | 000 = Mode 0 (interrupt on terminal count)                |
|         | 001 = Mode 1 (hardware re-triggerable one-shot)           |
|         | 010 = Mode 2 (rate generator)                             |
|         | 011 = Mode 3 (square wave generator)                      |
|         | 100 = Mode 4 (software triggered strobe)                  |
|         | 101 = Mode 5 (hardware triggered strobe)                  |
|         | 110 = Mode 2 (rate generator, same as 010)                |
|         | 111 = Mode 3 (square wave generator, same as 011)         |
| 0       | BCD / Binary Mode (0 = 16-bit binary, 1 = four digit BCD) |

> [!TIP] Bits 0 - 5 are the same as the mode / command

# Real Time Clock (RTC)
