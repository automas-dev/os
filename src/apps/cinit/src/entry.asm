[bits 32]

extern  __cinit
extern  main

section .text.entry

global  __start
__start:
    ; The kernel places argc/argv at the top of the initial stack before
    ; jumping here (see process_set_entrypoint in the kernel):
    ; [esp+0]=argc, [esp+4]=argv. Forward them to __cinit via a normal cdecl
    ; call (push right-to-left).
    mov eax, [esp]
    mov ebx, [esp + 4]
    push ebx
    push eax
    call __cinit
    ; __cinit never returns (it calls proc_exit)
