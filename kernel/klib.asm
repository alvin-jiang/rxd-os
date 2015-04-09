;
; @file: libk.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-26
;

[SECTION .text]

; export kernel lib functions
global printk

;-------------------------------------
; void printk(const char *s);
;-------------------------------------
printk:
    push ebp
    mov ebp, esp
    pushf
    cli

    ; DEBUG
    add esp, 4
    cmp esp, ebp
    jne $
    sub esp, 4

    mov esi, [ebp + 8]
    mov dx, word [0x90000]  ; dx = cursor pos
    mov al, 80
    mul dh
    add al, dl
    adc ah, 0
    shl ax, 1
    movzx eax, ax           ; eax = cursor offset

.loop:
    mov cl, byte [esi]
    cmp cl, 0
    je .end
    cmp cl, 0ah     ; '\n'
    je .no_print
    cmp cl, 0dh     ; '\r'
    je .no_print_0
    jmp .print
.no_print:
    inc dh
.no_print_0:
    mov dl, 0
    mov al, 80
    mul dh
    add al, dl
    adc ah, 0
    shl ax, 1
    movzx eax, ax
    jmp .next
.print:
    mov [gs:eax], cl
    add eax, 2
        ;cmp eax, 80 * 25 * 2
        ;jl .next
        ;mov dx, 0x0f00
.next:
    inc esi
    inc dl
    cmp dl, 80
    jl .loop
    mov dl, 0
    inc dh
    jmp .loop

.end:
    mov word [0x90000], dx
    call auto_scroll

    popf
    pop ebp
    ret

auto_scroll:
; dh - bottom line
    cmp dh, 24
    jl .end

    pushf
    cli
    push eax
    push edx
    push ecx

    xor eax, eax
    mov al, dh
    sub al, 23
    mov dl, 80
    mul dl
    mov cx, ax

    mov al, 0xd
    mov dx, 0x3d4
    out dx, al
    mov dx, 0x3d5
    mov al, cl
    out dx, al

    mov al, 0xc
    mov dx, 0x3d4
    out dx, al
    mov dx, 0x3d5
    mov al, ch
    out dx, al

    pop ecx
    pop edx
    pop eax
    popf
.end:
    ret

