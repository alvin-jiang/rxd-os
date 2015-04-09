
global fork

[SECTION .text]
ALIGN 4
[BITS 32]

fork:
    mov ecx, 0
    int 0x90
    ret
