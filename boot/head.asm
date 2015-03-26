;
; @file: head.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-25
;

; Task of head:
; 1. reset GDT
; 2. reset segment regs, check A20, setup IDT
; 3. setup Paging
; 4. jump to main.c

%include    "pm.inc"

extern _maink

[SECTION .s32]
ALIGN 32
[BITS 32]

global _start
_start:
    ; we are in protected-mode now

    ; 1. reset GDT
    mov ax, SelectorVideo
    mov gs, ax
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, SETUP_STACKTOP
    lgdt [GDTR]

    ; 2. reset segment regs, check A20, setup IDT
    mov ax, SelectorVideo
    mov gs, ax
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, SETUP_STACKTOP
    call    CheckA20
    call    SetupIDT

    jmp SelectorFlatC:AFTER_PAGE_TABLE

times   0x1000-($-$$)  db  0
Page0:
times   0x2000-($-$$)  db  0
Page1:
times   0x3000-($-$$)  db  0
Page2:
times   0x4000-($-$$)  db  0
Page3:

times   0x5000-($-$$)  db  0

AFTER_PAGE_TABLE:
    ; we can now safely override the code starting at 0x0000 
    ; 3. setup Paging
    call    SetupPaging

    ; 4. jump to main.c
    push 0 ; arg data
    push 0 ; argv
    push 0 ; argc
    push RETURN_FROM_MAIN
    push _maink
    ret

RETURN_FROM_MAIN:
    jmp $ ; main should not return


;-------------------------------------
; Functions
CheckA20:
    xor eax, eax
.loop:
    inc eax
    mov eax, [0x000000]
    cmp eax, [0x100000]
    je .loop
    ret

SetupIDT:
    lidt [LABEL_IDT]
    ret

SetupPaging:
    ; zero memory [0x0000, 0x5000)
    mov ecx, 1024 * 5
    xor eax, eax
    xor edi, edi
    cld
    rep stosd

    ; set PDE
    mov dword [0], Page0 + PG_ATTR
    mov dword [4], Page1 + PG_ATTR
    mov dword [8], Page2 + PG_ATTR
    mov dword [12], Page3 + PG_ATTR

    ; set PTE
    mov edi, Page3 + 4092
    mov eax, 0xfff000 + PG_ATTR
    std
.next_pde:
    stosd
    sub eax, 0x1000
    jge .next_pde

    ; enable paging
    mov eax, 0
    mov cr3, eax
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax
    ret

global TestFunc
TestFunc:
    mov ax, SelectorVideo
    mov gs, ax
    mov byte [gs:30], 'M'
    mov byte [gs:32], 'a'
    mov byte [gs:34], 'i'
    mov byte [gs:36], 'n'
    ret

_SpuriousHandler:
SpuriousHandler equ _SpuriousHandler - $$
    mov ah, 0Ch
    mov al, '!'
    mov [gs:((80 * 0 + 75) * 2)], ax
    jmp $
    iretd

[SECTION .pmdata]
ALIGN 32

IDTR    dw 256 * 8 - 1
        dd LABEL_IDT

GDTR    dw 256 * 8 - 1
        dd LABEL_GDT

;-------------------------------------
; IDT
LABEL_IDT:
%rep 256
    Gate    SelectorFlatC, SpuriousHandler, 0, DA_386IGate
%endrep

;-------------------------------------
; GDT
; descriptors
LABEL_GDT:
LABEL_DESC_NULL:        Descriptor             0,                    0, 0
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW | DA_DPL3
%rep 252
    Descriptor  0, 0, 0
%endrep

; selectors
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

;-------------------------------------
; Kernel Stack
times   100h    db  0
SETUP_STACKTOP  equ $ ; 栈顶

