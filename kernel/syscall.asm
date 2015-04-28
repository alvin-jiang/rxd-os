
global fork

[SECTION .text]
ALIGN 4
[BITS 32]

fork:
    mov ecx, 0
    mov eax, 0
    mov ebx, 0
    mov edx, 0
    int 0x90
    ret
