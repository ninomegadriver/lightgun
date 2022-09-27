        NAME    c0
;[]------------------------------------------------------------[]
;|      C0.ASM -- Start Up Code                                 |
;|        based on Turbo-C startup code, extensively modified   |
;[]------------------------------------------------------------[]

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
_TEXT   ENDS

_DATA   SEGMENT WORD PUBLIC 'DATA'
_DATA   ENDS

_BSS    SEGMENT WORD PUBLIC 'BSS'
_BSS    ENDS

DGROUP  GROUP   _TEXT, _DATA, _BSS

;       External References

EXTRN   _main : NEAR
EXTRN   _exit : NEAR

EXTRN   __stklen : WORD
EXTRN   __heaplen : WORD

PSPHigh         equ     00002h
PSPEnv          equ     0002ch

MINSTACK        equ     128     ; minimal stack size in words

;       At the start, DS, ES, and SS are all equal to CS

;/*-----------------------------------------------------*/
;/*     Start Up Code                                   */
;/*-----------------------------------------------------*/

_TEXT   SEGMENT BYTE PUBLIC 'CODE'

ASSUME  CS:_TEXT, DS:DGROUP

        ORG     100h

STARTX  PROC    NEAR

        mov     dx, cs          ; DX = GROUP Segment address
        mov     DGROUP@, dx
        mov     ah, 30h         ; get DOS version
        int     21h
        mov     bp, ds:[PSPHigh]; BP = Highest Memory Segment Addr
        mov     word ptr __heaptop, bp
        mov     bx, ds:[PSPEnv] ; BX = Environment Segment address
        mov     __version, ax   ; Keep major and minor version number
        mov     __psp, es       ; Keep Program Segment Prefix address

;       Determine the amount of memory that we need to keep

        mov     dx, ds          ; DX = GROUP Segment address
        sub     bp, dx          ; BP = remaining size in paragraphs
        mov     di, __stklen    ; DI = Requested stack size
;
; Make sure that the requested stack size is at least MINSTACK words.
;
        cmp     di, 2*MINSTACK  ; requested stack big enough ?
        jae     AskedStackOK    ; yes, use it
        mov     di, 2*MINSTACK  ; no,  use minimal value
        mov     __stklen, di    ; override requested stack size
AskedStackOK:
        add     di, offset DGROUP: edata
        jb      InitFailed      ; DATA segment can NOT be > 64 Kbytes
        add     di, __heaplen
        jb      InitFailed      ; DATA segment can NOT be > 64 Kbytes
        mov     cl, 4
        shr     di, cl          ; $$$ Do not destroy CL $$$
        inc     di              ; DI = DS size in paragraphs
        cmp     bp, di
        jnb     TooMuchRAM      ; Enough to run the program

;       All initialization errors arrive here

InitFailed:
        jmp     near ptr _abort

;       Set heap base and pointer

TooMuchRAM:
        mov     bx, di          ; BX = total paragraphs in DGROUP
        shl     di, cl          ; $$$ CX is still equal to 4 $$$
        add     bx, dx          ; BX = seg adr past DGROUP
        mov     __heapbase, bx
        mov     __brklvl, bx
;
;       Set the program stack down into RAM that will be kept.
;
        cli
        mov     ss, dx          ; DGROUP
        mov     sp, di          ; top of (reduced) program area
        sti

        mov     bx,__heaplen    ; set up heap top pointer
        add     bx,15
        shr     bx,cl           ; length in paragraphs
        add     bx,__heapbase
        mov     __heaptop, bx
;
;       Clear uninitialized data area to zeroes
;
        xor     ax, ax
        mov     es, cs:DGROUP@
        mov     di, offset DGROUP: bdata
        mov     cx, offset DGROUP: edata
        sub     cx, di
        rep     stosb
;
;       exit(main());
;
        call    _main           ; the real C program
        push    ax
        call    _exit           ; part of the C program too

;----------------------------------------------------------------
;       _exit()
;       Restore interrupt vector taken during startup.
;       Exit to DOS.
;----------------------------------------------------------------

        PUBLIC  __exit
__exit  PROC      NEAR
        push    ss
        pop     ds

;       Exit to DOS

ExitToDOS:
        mov     bp,sp
        mov     ah,4Ch
        mov     al,[bp+2]
        int     21h                     ; Exit to DOS

__exit  ENDP

STARTX  ENDP

;[]------------------------------------------------------------[]
;|      Miscellaneous functions                                 |
;[]------------------------------------------------------------[]

ErrorDisplay    PROC    NEAR
                mov     ah, 040h
                mov     bx, 2           ; stderr device
                int     021h
                ret
ErrorDisplay    ENDP

        PUBLIC  _abort
_abort  PROC      NEAR
                mov     cx, lgth_abortMSG
                mov     dx, offset DGROUP: abortMSG
MsgExit3        label   near
                push    ss
                pop     ds
                call    ErrorDisplay
CallExit3       label   near
                mov     ax, 3
                push    ax
                call    __exit          ; _exit(3);
_abort   ENDP

;       The DGROUP@ variable is used to reload DS with DGROUP

        PUBLIC  DGROUP@
DGROUP@     dw    ?

_TEXT   ENDS

;[]------------------------------------------------------------[]
;|      Start Up Data Area                                      |
;[]------------------------------------------------------------[]

_DATA   SEGMENT WORD PUBLIC 'DATA'

abortMSG        db      'Quitting program...', 13, 10
lgth_abortMSG   equ     $ - abortMSG

;
;                       Miscellaneous variables
;
        PUBLIC  __psp
        PUBLIC  __version
        PUBLIC  __osmajor
        PUBLIC  __osminor

__psp           dw      0
__version       label   word
__osmajor       db      0
__osminor       db      0

;       Memory management variables

        PUBLIC  ___heapbase
        PUBLIC  ___brklvl
        PUBLIC  ___heaptop
        PUBLIC  __heapbase
        PUBLIC  __brklvl
        PUBLIC  __heaptop

___heapbase     dw      DGROUP:edata
___brklvl       dw      DGROUP:edata
___heaptop      dw      DGROUP:edata
__heapbase      dw      0
__brklvl        dw      0
__heaptop       dw      0

_DATA   ENDS

_BSS    SEGMENT WORD PUBLIC 'BSS'

bdata   label   byte
edata   label   byte            ; mark top of used area

_BSS    ENDS

        END     STARTX
