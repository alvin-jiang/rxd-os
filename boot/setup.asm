;
; @file: setup.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-23
;

; Task of setup.bin:
; 1. get params: display, memory, hd
; 2. cli
; 4. move kernel to memory 0x00000
; 5. fix 8259A
; 6. set temporary GDT, A20, enable Protected-Mode
; 7. jump to head part of kernel.bin

INIT_SEGMENT        equ 0x9000
SETUP_SEGMENT       equ 0x9020
KERNEL_SEGMENT      equ 0x1000
%include "pm.inc"

SETUP_START:

; read & save cursor pos
    mov ah, 03h
    mov bh, 0
    int 10h
    mov dl, 0
    inc dh
    mov word [0], dx

    mov bp, Msg_Setup
    call    PrintLocalMsg
    mov bp, Msg_CheckParams
    call    PrintLocalMsg
    mov bp, Msg_MemSize
    call    PrintLocalMsg

; read & save extend memory size
    call    CheckMemory
    mov word [2], ax
    mov word [4], bx

    mov bp, 4
    call    PrintWord
    mov bp, 2
    call    PrintWord

; check hd, get params

; reset segment regs
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000 ; 0x9f200 < 0x9fc00

; disable interrupt
; we can't use BIOS functions after this!
; after enter protected-mode, we can use printk
    cli

; move kernel to memory 0x00000
    cld
    push ds
    push es
    mov ax, INIT_SEGMENT
    mov ds, ax
    mov cl, [ds:509]
    movzx cx, cl
    shl cx, 8       ; cx = kernel size in words
    mov bx, KERNEL_SEGMENT
    mov ds, bx
    mov bx, 0x0000
    mov es, bx
    xor si, si
    xor di, di
    rep movsw       ; ds:si -> es:di
    pop es
    pop ds

; fix 8259A compatibility problems
    call    Reset8259A

; set temporary GDT, open A20, enter protected-mode
    ; load GDT
    lgdt [GdtPtr]
    ; enable A20, so we can access memory >= 1 MB
    in  al, 92h
    or  al, 00000010b
    out 92h, al
    ; set CR0, after this, CPU operates in protected-mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

; jump to kernel head
    ; note address translation has already changed from now,
    ; also note this jmp is a 32-bit instruction in 16-bit code segment.
    jmp dword SelectorFlatC:0x0000


;-------------------------------------
; Variables
;-------------------------------------
wEXMemSizeLow       dw  0 ; extend memory (>= 1 MB) size in KB
wEXMemSizeHigh      dw  0 ; extend memory (>= 1 MB) size in KB

; used in PrintLocalMsg()
Msg_Setup:          db  "setup...", 0
Msg_CheckParams:    db  "check system params...", 0
Msg_MemSize:        db  "extend memory size = 0x", 0
Msg_Printk:          db  "hello printk...", 0

;-------------------------------------
; Temporary GDT
;-------------------------------------
LABEL_GDT:
LABEL_DESC_NULL:        Descriptor             0,                    0, 0
LABEL_DESC_FLAT_C:      Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor       0B8000h,               0ffffh, DA_DRW | DA_DPL3

GdtLen  equ $ - LABEL_GDT
GdtPtr  dw  GdtLen - 1
        dd  SETUP_SEGMENT * 16 + LABEL_GDT

;-------------------------------------
; Temporary GDT Selector
;-------------------------------------
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

;-------------------------------------
; Functions
;-------------------------------------
CheckMemory:
; out:
;   ax - low 16-bit of extend memory size in KB
;   bx - high 16-bit

    mov ax, 0e801h
    int 15h
    jc .error
    test ax, ax
    jz .error

    test bx, bx
    jz .end
    mov cx, bx
    and cx, 03ffh
    shl cx, 6
    add cx, ax
    mov ax, cx
    shr bx, 10
    jnc .end
    inc bx
.end:
    ret
.error:
    jmp $

Reset8259A:
    mov al, 011h
    out 020h, al    ; 主8259, ICW1.
    call    io_delay

    out 0A0h, al    ; 从8259, ICW1.
    call    io_delay

    mov al, 020h    ; IRQ0 对应中断向量 0x20
    out 021h, al    ; 主8259, ICW2.
    call    io_delay

    mov al, 028h    ; IRQ8 对应中断向量 0x28
    out 0A1h, al    ; 从8259, ICW2.
    call    io_delay

    mov al, 004h    ; IR2 对应从8259
    out 021h, al    ; 主8259, ICW3.
    call    io_delay

    mov al, 002h    ; 对应主8259的 IR2
    out 0A1h, al    ; 从8259, ICW3.
    call    io_delay

    mov al, 001h
    out 021h, al    ; 主8259, ICW4.
    call    io_delay

    out 0A1h, al    ; 从8259, ICW4.
    call    io_delay

    mov    al, 11111111b   ; 屏蔽主8259所有中断
    out 021h, al    ; 主8259, OCW1.
    call    io_delay

    mov al, 11111111b   ; 屏蔽从8259所有中断
    out 0A1h, al    ; 从8259, OCW1.
    call    io_delay

    ret

io_delay:
    nop
    nop
    nop
    nop
    ret

PrintLocalMsg:
; in:
;   bp - String Offset

    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [1]                 ; DH - row of cursor (00h is top)
    inc dh
    mov dl, 0                   ; DL - col of cursor (00h is left)
.next_char:
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp + 0x200]        ; AL - character to write
    test al, al
    jz .end
    mov cx, 1                   ; CX - number of times to write
    int 10h
    inc bp
    inc dl
    jmp .next_char
.end:
    mov word [0], dx
    ret

PrintWord:
; in:
;   bp - Word to Display

    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dx, word [0]            ; DH - row of cursor (00h is top)
                                ; DL - col of cursor (00h is left)
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h

    inc bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    shr al, 4
    add al, '0'
    cmp al, '9'
    jle disp_word_2
    add al, 7
disp_word_2:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    and al, 0fh
    add al, '0'
    cmp al, '9'
    jle disp_word_1
    add al, 7
disp_word_1:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    dec bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    shr al, 4
    add al, '0'
    cmp al, '9'
    jle disp_word_4
    add al, 7
disp_word_4:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [bp]                ; AL - character to write
    and al, 0fh
    add al, '0'
    cmp al, '9'
    jle disp_word_3
    add al, 7
disp_word_3:
    mov cx, 1                   ; CX - number of times to write
    int 10h
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    inc dl
    int 10h

    mov word [0], dx
    ret

