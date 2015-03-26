;
; @file: setup.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-23
;

; Task of setup.bin:
; 1. check memory, get params
; 2. check hd, get params
; 3. cli
; 4. move kernel to memory 0x00000
; 5. fix 8259A
; 6. set temporary GDT, A20, enable Protected-Mode
; 7. jump to head part of kernel.bin

%include "const.inc"
%include "pm.inc"

SETUP_START:
    mov ax, cs
    mov ds, ax

    mov bp, Msg_Setup
    call    DispString

    ; 1. check memory, get params
    mov bp, Msg_MemChk
    call    DispString
    call    MemoryCheck
    mov bp, Msg_MemSize
    call    DispString

    mov bp, wEXMemSizeHigh
    call    DispWord
    mov bp, wEXMemSizeLow
    call    DispWord

    ; 2. check hd, get params

    ; 3. cli
    ; we can't call BIOS functions after this!
    cli

    ; 4. move kernel to memory 0x00000
    push ds
    mov ax, INIT_SEGMENT
    mov ds, ax
    mov al, [ds:509]
    movzx ax, al
    shl ax, 8
    mov cx, ax      ; ax = kernel size in words
    mov bx, KERNEL_SEGMENT
    mov ds, bx
    mov bx, 0x0000
    mov es, bx
    xor si, si
    xor di, di
    rep movsw       ; ds:si -> es:di
    pop ds

    ; 5. fix 8259A
    call    Init8259A

    ; 6. set temporary GDT, A20, enable Protected-Mode
    ; load GDT
    lgdt    [GdtPtr]
    ; enable A20, so we can access memory >= 1 MB
    in  al, 92h
    or  al, 00000010b
    out 92h, al
    ; set CR0, after this, CPU operates in protected-mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; 7. jump to head part of kernel.bin
    ; note the address translation has already changed,
    ; also note this jmp is a 32-bit instruction in 16-bit code segment.
    jmp dword SelectorFlatC:0x0000


;-------------------------------------
; Variables
wEXMemSizeLow       dw  0 ; extend memory (>= 1 MB) size in KB
wEXMemSizeHigh      dw  0 ; extend memory (>= 1 MB) size in KB

; used in DispString()
bCursorRow          db  3
bCursorCol          db  0
Msg_Setup:          db  "setup...", 0
Msg_MemChk:         db  "check memory...", 0
Msg_MemSize:        db  "extend memory size = 0x", 0
Msg_Ready:          db  "loading kernel...", 0

;-------------------------------------
; Temporary GDT
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
SelectorFlatC       equ LABEL_DESC_FLAT_C   - LABEL_GDT
SelectorFlatRW      equ LABEL_DESC_FLAT_RW  - LABEL_GDT
SelectorVideo       equ LABEL_DESC_VIDEO    - LABEL_GDT + SA_RPL3

;-------------------------------------
; Functions

; @MemoryCheck
MemoryCheck:
    mov ax, 0e801h
    int 15h
    jc .error
    test ax, ax
    jz .error

    test bx, bx
    jz .no_high
    mov cx, bx
    and cx, 03ffh
    shl cx, 6
    add cx, ax
    mov [ds:wEXMemSizeLow], cx
    shr bx, 10
    jnc .no_carry
    inc bx
.no_carry:
    mov [ds:wEXMemSizeHigh], bx
    jmp .end
.no_high:
    mov [ds:wEXMemSizeLow], ax
    mov word [ds:wEXMemSizeHigh], 0
.end:
    ret
.error:
    jmp $

Init8259A:
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

; @DispString
; in:
;   bp - String Offset
DispString:
    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [ds:bCursorRow]     ; DH - row of cursor (00h is top)
    inc dh
    mov dl, 0                   ; DL - col of cursor (00h is left)
.next_char:
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [ds:bp]             ; AL - character to write
    test al, al
    jz .end
    mov cx, 1                   ; CX - number of times to write
    int 10h
    inc bp
    inc dl
    jmp .next_char
.end:
    mov [ds:bCursorRow], dh
    mov [ds:bCursorCol], dl
    ret

; @DispWord
; in:
;   bp - Word to Display
DispWord:
    mov bx, 0007h               ; BH - page number
                                ; BL - 07h, black background, white character
    mov dh, [ds:bCursorRow]     ; DH - row of cursor (00h is top)
    mov dl, [ds:bCursorCol]     ; DL - col of cursor (00h is left)
    mov ah, 02h                 ; AH - 02h, Set Cursor Position
    int 10h

    inc bp
    mov ah, 0ah                 ; AH - 09h, Write Character
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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
    mov al, [ds:bp]             ; AL - character to write
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

    mov [ds:bCursorRow], dh
    mov [ds:bCursorCol], dl
    ret

