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

extern main, printk
global _start, _gdt, _idt, _tss

%include    "pm.inc"

[SECTION .text]
ALIGN 4
[BITS 32]

_start:
    ; we are in protected-mode now

; reload GDT, segment regs
    ; set es temporarily to load new GDT
    mov ax, 0x10
    mov es, ax
    lgdt [es:_gdtr]
    mov ax, _sel_video
    mov gs, ax
    mov ax, _sel_data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, 0x90000 ; we store boot params on memory >= 0x90000

; make sure A20 is opened
    call    _func_check_a20

; setup IDT, TSS
    call    _func_setup_idt
    call    _func_setup_tss

    jmp _sel_code:_after_page_table

times   0x1000-($-$$)  db  0
_page0:
times   0x2000-($-$$)  db  0
_page1:
times   0x3000-($-$$)  db  0
_page2:
times   0x4000-($-$$)  db  0
_page3:

times   0x5000-($-$$)  db  0
_after_page_table:
; we can now safely override the code starting at 0x0000
; setup Paging
    call    _func_setup_paging

    ; 4. jump to main.c
    push 0 ; arg data
    push 0 ; argv
    push 0 ; argc
    push _return_from_main
    push main
    ret

_return_from_main:
    jmp $ ; main should not return


;-------------------------------------
; Functions
;-------------------------------------
_func_check_a20:
    xor eax, eax
.loop:
    inc eax
    mov eax, [0x000000]
    cmp eax, [0x100000]
    je .loop
    ret

_func_setup_idt:
    lidt [_idtr]
    ret

_func_setup_tss:
    mov ax, ds
    mov [_tss + 8], ax          ; ss0
    mov word [_tss + 102], 104  ; iobase

    mov eax, _tss
    mov [_gd_tss + 2], ax
    shr eax, 16
    mov [_gd_tss + 4], al
    mov [_gd_tss + 7], ah

    mov ax, _sel_tss
    ltr ax
    ret

_func_setup_paging:
    ; zero memory [0x0000, 0x5000)
    mov ecx, 1024 * 5
    xor eax, eax
    xor edi, edi
    cld
    rep stosd

    ; set PDE
    mov dword [0], _page0 + PG_DEFAULT
    mov dword [4], _page1 + PG_DEFAULT
    mov dword [8], _page2 + PG_DEFAULT
    mov dword [12], _page3 + PG_DEFAULT

    ; set PTE
    mov edi, _page3 + 4092
    mov eax, 0xfff000 + PG_DEFAULT
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

_SpuriousHandler:
SpuriousHandler equ _SpuriousHandler - $$
    mov ah, 0Ch
    mov al, '!'
    mov [gs:((80 * 0 + 75) * 2)], ax
    jmp $
    iretd

;-------------------------------------
; TSS & IDT & GDT
;-------------------------------------
ALIGN 4
_tss:
times   104 db 0

ALIGN 4
_idtr   dw 256 * 8 - 1
        dd _idt

_gdtr   dw 256 * 8 - 1
        dd _gdt

ALIGN 8
_idt:
%rep 256
    Gate        _sel_code, SpuriousHandler, 0, DA_386IGate
%endrep

_gdt:
_gd_null:       Descriptor        0,         0, 0
_gd_code:       Descriptor        0,   0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
_gd_data:       Descriptor        0,   0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K
_gd_video:      Descriptor  0B8000h,    0ffffh, DA_DRW | DA_DPL3
_gd_tss:        Descriptor        0,       67h, DA_386TSS
%rep 251
    Descriptor  0, 0, 0
%endrep

_sel_code        equ _gd_code - _gdt
_sel_data        equ _gd_data - _gdt
_sel_video       equ _gd_video - _gdt + SA_RPL3
_sel_tss         equ _gd_tss - _gdt

