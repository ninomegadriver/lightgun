        NAME    movup
;[]------------------------------------------------------------[]
;|      MOVUP.ASM -- helper code for DEVLOD.C                   |
;|      Copyright 1990 by Jim Kyle - All Rights Reserved        |
;[]------------------------------------------------------------[]

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
_TEXT   ENDS

_DATA   SEGMENT WORD PUBLIC 'DATA'
_DATA   ENDS

_BSS    SEGMENT WORD PUBLIC 'BSS'
_BSS    ENDS

DGROUP  GROUP   _TEXT, _DATA, _BSS

ASSUME  CS:_TEXT, DS:DGROUP

_TEXT   SEGMENT BYTE PUBLIC 'CODE'

;-----------------------------------------------------------------
;       movup( src, dst, nbytes )
;       src and dst are far pointers. area overlap is NOT okay
;-----------------------------------------------------------------
        PUBLIC  _movup

_movup  PROC      NEAR
        push    bp
        mov     bp, sp
        push    si
        push    di
        lds     si,[bp+4]               ; source
        les     di,[bp+8]               ; destination
        mov     bx,es                   ; save dest segment
        mov     cx,[bp+12]              ; byte count
        cld
        rep     movsb                   ; move everything to high ram
        mov     ss,bx                   ; fix stack segment ASAP
        mov     ds,bx                   ; adjust DS too
        pop     di
        pop     si
        mov     sp, bp
        pop     bp
        pop     dx                      ; Get return address
        push    bx                      ; Put segment up first
        push    dx                      ; Now a far address on stack
        retf
_movup  ENDP

;-------------------------------------------------------------------
;       copyptr( src, dst )
;       src and dst are far pointers.
;       moves exactly 4 bytes from src to dst.
;-------------------------------------------------------------------
        PUBLIC  _copyptr

_copyptr        PROC      NEAR
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        lds     si,[bp+4]               ; source
        les     di,[bp+8]               ; destination
        cld
        movsw
        movsw
        pop     ds
        pop     di
        pop     si
        mov     sp, bp
        pop     bp
        ret
_copyptr        ENDP

_TEXT   ENDS

        end
