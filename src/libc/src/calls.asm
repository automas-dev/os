[bits 32]

global send_call_noret
global send_call

send_call_noret:
send_call:
    mov eax, [esp+4]
    mov ebx, esp
    add ebx, 8

    int 48

    ret
