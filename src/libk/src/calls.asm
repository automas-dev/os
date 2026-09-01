[bits 32]

global send_call_noret
global send_call

send_call_noret:
send_call:
    push ebx
    mov eax, [esp+8]
    mov ebx, esp
    add ebx, 12

    int 48

    pop ebx
    ret
