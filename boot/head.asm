;
; @file: head.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-25
;

; Task of head:
; 1. reset segment regs
; 2. reset GDT, check A20, set LDT
; 3. set IDT, 8259A
; 4. set TSS
; 5. set Paging

%include    "pm.inc"

[SECTION .s32]
ALIGN   32
[BITS   32]

global _start
_start:
    ; we are in protected-mode now
    ; 1. reset segment regs
    mov ax, SelectorVideo
    mov gs, ax
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, SETUP_STACKTOP

    ; 2. reset GDT, check A20, set LDT
    ; 3. set IDT, 8259A
    ; 4. set TSS
    ; 5. set Paging

    mov byte [gs:0], 'P'

    jmp $
    ;jmp SelectorFlatC:KERNEL_START  ; jump to kernel!

;-------------------------------------
; Functions

;-------------------------------------
; Data

[SECTION .data1]
ALIGN   32

;-------------------------------------
; GDT
LABEL_GDT:
LABEL_DESC_NULL:        Descriptor             0,                    0, 0
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW | DA_DPL3

GdtLen  equ $ - LABEL_GDT
GdtPtr  dw  GdtLen - 1
        dd  LABEL_GDT

;-------------------------------------
; GDT Selector
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

;-------------------------------------
; Kernel Stack
times   100h    db  0
SETUP_STACKTOP  equ $ ; 栈顶

