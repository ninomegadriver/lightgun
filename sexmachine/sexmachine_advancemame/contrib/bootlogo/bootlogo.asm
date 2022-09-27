; BOOTLOGO.EXE by Robert Palmqvist 2000
; Built on device driver code by Dr. GJC Lokhorst
; and PCX code by Sugar Less (Paul Wroblewski).

pushall macro
	push ax
	push bx
	push cx
	push dx
	push di
	push si
	push es
	push ds
	push bp
	pushf
endm

popall macro
	popf
	pop bp
	pop ds
	pop es
	pop si
	pop di
	pop dx
	pop cx
	pop bx
	pop ax
endm

..model small
..stack 80h				; seems enough

..code

assume ds:@code

deviceheader	dd -1h
attribute	dw 8004h		; char nul
strategy	dw offset strategy_init
interrupt	dw offset interrupt_init
		db "BOOTLOGO"

endresident:

B		db ?
FileName	db "bootlogo.pcx",0

mainproc proc near
	mov	ax,0f00h		; get current video mode
	int	10h
	cmp	al,13h			; is it mode 13h?
	jnz	ShowLogo		; if not show logo
	mov	ax,3			; else restore text mode
	int	10h
	ret
ShowLogo:
	push	cs			; point DS to our segment
	pop	ds
	mov	ax,03d00h
	mov	dx,offset FileName
        int	21h			; try open file
        jnc	FileOk
	ret
FileOk:
        mov	bx,ax			; bx = file handle
        mov	ax,0a000h
        mov	es,ax			; es = video mem
        mov	di,0fa01h
        mov	ax,13h
        int	10h			; init vga mode 13h
        mov	ah,42h			; seek to eof-768
        xor	cx,cx			; (there may be a better way to do this...)
        xor	dx,dx
        mov	al,2
        int	21h
        sub	ax,300h
        xor	cx,cx
        mov	dx,ax
        mov	ah,42h
        mov	al,0
        int	21h
        mov	cx,300h			; load pcx palette
        push	di
Loop0:
        push	cx
        call	GetByte
        shr	al,2
        stosb
        pop	cx
        loop	Loop0
        pop	dx
        push	bx
        mov	ax,1012h
        xor	bx,bx
        mov	cx,100h
        int	10h
        pop	bx
        mov	ah,42h			; seek to start of pcx image
        xor	cx,cx
        mov	dx,80h
        mov	al,0h
        int	21h
        xor	di,di
Loop1:					; show pcx image
        call GetByte
        and  al,  0c0h
        cmp  al,  0c0h
        mov  al,  B
        je   Run
        stosb
        jmp  Cont
Run:
        sub  al,  0c0h
        push ax
        call GetByte
        pop  cx
        rep  stosb
Cont:
        cmp  di,  0fa00h
        jb   Loop1
        mov  ah,  3eh                   ; close file
        int  21h
        ret
GetByte:                                ; get one byte from file
        lea  dx,  B
        mov  ah,  3Fh
        mov  cx,  1
        int  21h
        mov  al,  B
	ret
endp

strategy_init proc far
	mov	word ptr cs:[keep_bx],bx
	mov	word ptr cs:[keep_es],es
	ret
endp

keep_bx:
	nop
	nop
keep_es:
	nop
	nop

interrupt_init proc far
	pushall
	call	mainproc
; adapted from EGA.SYS 5.0
	lds	bx,dword ptr cs:[keep_bx]
	xor	ax,ax
	mov	word ptr cs:[attribute],ax 
	mov	word ptr [bx+10h],cs
	mov	word ptr [bx+0eh],ax		; length of resident portion
	mov	byte ptr [bx+0dh],al		; nr of units
	mov	word ptr [bx+03h],810Ch		; done, general failure
	popall
	ret
endp

start:
	call	mainproc
	mov	ax,4c00h			; finish, exit code = OK
	int	21h
end start
