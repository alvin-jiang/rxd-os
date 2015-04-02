;
; @file: string.asm
; @author: Alvin Jiang
; @mail: celsius.j@gmail.com
; @created time: 2015-03-26
;

[SECTION .text]

global memcpy
;global memmove
global memset
global strcpy
global strlen

;-------------------------------------
; void *  memcpy(void * dst, const void * src, size_t num);
;-------------------------------------
memcpy:
    cmp dword [esp + 12], 0
    je .end

    push    ebp
    mov ebp, esp
    push    esi
    push    edi
    push    ecx

    mov edi, [ebp + 8]  ; dst
    mov esi, [ebp + 12] ; src
    mov ecx, [ebp + 16] ; num
.loop:
    mov al, [ds:esi]
    mov byte [es:edi], al
    inc esi
    inc edi
    loop .loop

    mov eax, [ebp + 8] ; return dst
    pop ecx
    pop edi
    pop esi
    pop ebp
.end:
    ret

;-------------------------------------
; void *  memmove(void * destination, const void * source, size_t num);
;-------------------------------------
memmove:
    ret

;-------------------------------------
; void *  memset(void * ptr, int value, size_t num);
;-------------------------------------
memset:
    cmp dword [esp + 12], 0
    je .end

    push    ebp
    mov ebp, esp
    push    edi
    push    ecx
    push    edx

    mov edi, [ebp + 8]  ; ptr
    mov edx, [ebp + 12] ; value
    mov ecx, [ebp + 16] ; num
.loop:
    mov byte [es:edi], dl
    inc edi
    loop .loop

    mov eax, [ebp + 8] ; return ptr
    pop edx
    pop ecx
    pop edi
    pop ebp
.end:
    ret


;-------------------------------------
; char *  strcpy(char * destination, const char * source);
;-------------------------------------
strcpy:
    push ebp
    mov ebp, esp
    push esi
    push edi

    mov esi, [ebp + 12]
    mov edi, [ebp + 8]
.loop:
    cmp byte [esi], 0
    je .end
    mov al, [esi]
    mov [edi], al
    inc edi
    inc esi
    jmp .loop
.end:
    mov eax, [esp + 8]

    pop edi
    pop esi
    pop ebp
    ret

;-------------------------------------
; size_t  strlen(const char * str);
;-------------------------------------
strlen:
    push ebp
    mov ebp, esp
    push esi

    mov eax, 0
    mov esi, [ebp + 8]
.next:
    cmp byte [esi], 0
    je .end
    inc esi
    inc eax
    jmp .next
.end:
    pop esi
    pop ebp
    ret




