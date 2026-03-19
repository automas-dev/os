extern  __cinit
extern  main

section .text.entry

global  __start
__start:
    call __cinit
    ; jmp  main
