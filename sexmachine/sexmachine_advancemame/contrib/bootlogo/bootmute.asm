; BOOTMUTE.EXE by Robert Palmqvist 2000
; Built on device driver code by Dr. GJC Lokhorst
; and altered versions by Donald Davis.

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
..stack 80h 				; seems enough

..code

assume ds:@code

deviceheader	dd -1h
attribute	dw 8004h		; char nul
strategy	dw offset strategy_init
interrupt	dw offset interrupt_init
		db "BOOTMUTE"

endresident:

mainproc proc near
        mov	ax,3000h		; get DOS version in al
	int	21h
	cmp	al,6			; DOS 6 or less?
	jbe	Dos6
	mov	dl,0e8h			; if DOS 7 or more, first byte of int 29h is 0e8h
	jmp	short cont
Dos6:	
        mov	dl,50h			; if DOS 6 or less, first byte of int 29h is 50h
cont:	
	xor	ax,ax
	mov	ds,ax
	lds	bx,dword ptr ds:[29h*4]	; int 29h
	cmp	byte ptr [bx],0cfh	; iret
	jne	turnoff
	mov	ax,0100h		; turn cursor on if toggling int 29h display on
	mov	cx,0506h		; DOS default cursor size
	int	10h
	call	toggle
	jmp 	short endmainproc	; don't clear screen if if toggling int 29h on
turnoff:
	cmp 	byte ptr [bx],dl	; push ax
	jne 	endmainproc
	mov 	ax,0100h		; turn cursor off if toggling INT 29h display off
	mov	cx,2000h
	int	10h
	call	toggle
	mov	ah,0fh			; clrscr from transient portion of COMMAND.COM 5.0
	int	10h
	cmp	al,03h
	jbe	label_112h
	cmp	al,07h
	jnz	endmainproc
label_112h:
	xor	ax,ax
	mov	ds,ax
	mov	dx,word ptr ds:[044ah]	; dl = cols
	mov	dh,byte ptr ds:[0484h]	; last line, or 0
	or	dh,dh
	jnz	label_127h
	mov	dh,24
label_127h:
	dec	dl
	push	dx
	mov	ah,0bh
	xor	bx,bx
	int	10h
	pop	dx
	xor	ax,ax
	mov	cx,ax
	mov	ah,06h
	mov	bh,07h
	xor	bl,bl
	int	10h
	mov	ah,0fh
	int	10h
	mov	ah,02h
	xor	dx,dx
	int	10h
endmainproc:
	ret
endp

toggle proc near
	cmp	dl,50h				; DOS 6 or less?
	ja	Dos7 
	xor	byte ptr [bx], (50h xor 0cfh)	; 50h if DOS or less
	jmp	short endtoggle
Dos7:	xor	byte ptr [bx], (0e8h xor 0cfh)	; 0e8h if DOS 7 or more
endtoggle:	
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
