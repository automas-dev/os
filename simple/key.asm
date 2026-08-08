[org 0x7e00]

DATA          equ 0x500
STATE         equ DATA + 0x100
FIRST_NIBBLE  equ DATA + 0x101

start:
    mov si, DATA
    mov byte [STATE], 0

.next_key:
    mov ah, 0x00        ; BIOS: wait for keyboard input
    int 0x16            ; Read keypress; ASCII char in AL

    mov ah, 0x0E        ; BIOS teletype output
    int 0x10            ; Print character in AL

    cmp al, 0x0D        ; Enter key
    je .jump_to_data

    call parse_nibble

    cmp byte [STATE], 0
    jne .second_nibble

    mov byte [FIRST_NIBBLE], al
    mov byte [STATE], 1
    jmp .next_key

.second_nibble:
    mov byte [STATE], 0
    mov ah, [FIRST_NIBBLE]
    shl ah, 4
    or ah, al
    mov byte [si], ah
    inc si
    jmp .next_key

.jump_to_data:
    jmp DATA

parse_nibble:
    cmp al, '0'
    jb .done
    cmp al, '9'
    jbe .digit
    cmp al, 'a'
    jb .done
    cmp al, 'f'
    jbe .letter

.done:
    ret

.digit:
    sub al, '0'
    ret

.letter:
    sub al, 'a'
    add al, 10
    ret