; uint8_t port_byte_in(uint16_t port)
global port_byte_in
port_byte_in:
    mov dx, [esp+4]
    in al, dx
    ret

; void port_byte_out(uint16_t port, uint8_t data)
global port_byte_out
port_byte_out:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, al
    ret

; uint16_t port_word_in(uint16_t port)
global port_word_in
port_word_in:
    mov dx, [esp+4]
    in ax, dx
    ret

; void port_word_out(uint16_t port, uint16_t data)
global port_word_out
port_word_out:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, ax
    ret
