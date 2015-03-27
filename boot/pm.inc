;-------------------------------------
; Constants & Marcos for Protected-Mode
;-------------------------------------

;-------------------------------------
; DESCRIPTOR
; DA_: Descriptor Attribute

; G - Granularity
DA_LIMIT_4K EQU     8000h

; B/D - Default operation size
; 0 = 16-bit seg, 1 = 32-bit seg
DA_32       EQU     4000h

; DPL
DA_DPL0     EQU     00h
DA_DPL1     EQU     20h
DA_DPL2     EQU     40h
DA_DPL3     EQU     60h

; Type
; for Data-Segment
; R: Readable W: Writable A: Accessed
DA_DR       EQU     90h
DA_DRW      EQU     92h
DA_DRWA     EQU     93h
; for Code-Segment
; C: Conforming R: Readable
DA_C        EQU     98h
DA_CR       EQU     9Ah
DA_CC       EQU     9Ch
DA_CCR      EQU     9Eh
; for System-Segment
DA_LDT      EQU     82h
DA_TaskGate EQU     85h
DA_386TSS   EQU     89h
DA_386CGate EQU     8Ch
DA_386IGate EQU     8Eh
DA_386TGate EQU     8Fh

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
SA_GDT      EQU 0
SA_LDT      EQU 4

; RPL - Requested Privilege Level
SA_RPL0     EQU 0
SA_RPL1     EQU 1
SA_RPL2     EQU 2
SA_RPL3     EQU 3

;-------------------------------------
; GATE MARCOS

; Gate      Selector(dw), Offset(dd), DCount(db), Attr(db)
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

