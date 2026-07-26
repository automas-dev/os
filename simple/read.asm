[org 0x7c00]

LOAD_ADDR equ 0x500
BOOT_DRIVE equ 0x80

start:
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ah, 0x02        ; BIOS read sectors
    mov al, 1           ; one sector
    mov ch, 0           ; cylinder 0
    mov cl, 2           ; sector 2
    mov dh, 0           ; head 0
    mov dl, BOOT_DRIVE  ; drive number
    mov bx, LOAD_ADDR   ; buffer address
    int 0x13

    jc hang

    jmp LOAD_ADDR

hang:
    hlt
    jmp hang

times 510-($-$$) db 0
dw 0xaa55
