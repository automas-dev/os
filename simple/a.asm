[org 0x7e00]

start:
mov ah, 0x0e
mov al, 'A'
int 0x10
hlt

times 510-($-$$) db 0
dw 0xaa55
