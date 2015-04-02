;-------------------------------------
; Constants & Marcos for Protected-Mode
;-------------------------------------

;-------------------------------------
; DESCRIPTOR
; DA_: Descriptor Attribute

; G - Granularity
DA_LIMIT_4K equ     8000h

; B/D - Default operation size
; 0 = 16-bit seg, 1 = 32-bit seg
DA_32       equ     4000h

; DPL
DA_DPL0     equ     00h
DA_DPL1     equ     20h
DA_DPL2     equ     40h
DA_DPL3     equ     60h

; Type
; for Data-Segment
; R: Readable W: Writable A: Accessed
DA_DR       equ     90h
DA_DRW      equ     92h
DA_DRWA     equ     93h
; for Code-Segment
; C: Conforming R: Readable
DA_C        equ     98h
DA_CR       equ     9Ah
DA_CC       equ     9Ch
DA_CCR      equ     9Eh
; for System-Segment
DA_LDT      equ     82h
DA_TaskGate equ     85h
DA_386TSS   equ     89h
DA_386CGate equ     8Ch
DA_386IGate equ     8Eh
DA_386TGate equ     8Fh

;-------------------------------------
; DESCRIPTOR MARCOS

; Descriptor    Base(dd), Limit(dd), Attr(dw)
%macro Descriptor 3
    dw  %2 & 0FFFFh          
    dw  %1 & 0FFFFh          
    db  (%1 >> 16) & 0FFh        
    dw  ((%2 >> 8) & 0F00h) | (%3 & 0F0FFh)
    db  (%1 >> 24) & 0FFh        
%endmacro

;-------------------------------------
; SELECTOR
; SA_: Selector Attribute

; TI - Table Indicator
SA_GDT      equ 0
SA_LDT      equ 4

; RPL - Requested Privilege Level
SA_RPL0     equ 0
SA_RPL1     equ 1
SA_RPL2     equ 2
SA_RPL3     equ 3

;-------------------------------------
; GATE MARCOS

; Gate      Selector(dw), Offset(dd), ParamCount(db), Attr(db)
%macro Gate 4
    dw  (%2 & 0FFFFh)            
    dw  %1               
    dw  (%3 & 1Fh) | ((%4 << 8) & 0FF00h)
    dw  ((%2 >> 16) & 0FFFFh)        
%endmacro

;-------------------------------------
; PAGING

; P - Present
PG_P        equ 1
; R/W - Read/Write
PG_RWR      equ 0
PG_RWW      equ 2
; U/S - User/Supervisor
PG_USS      equ 0
PG_USU      equ 4

; default = Present + Read/Write + User
PG_DEFAULT  equ 7

