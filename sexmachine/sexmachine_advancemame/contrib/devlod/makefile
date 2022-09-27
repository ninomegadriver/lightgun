# makefile for DEVLOD.COM - last revised 05/23/90 - jk
# can substitute other assemblers for TASM

all: devlod.com

c0.obj  :       c0.asm
        tasm c0 /t/mx/la;

movup.obj:      movup.asm
        tasm movup /t/mx/la;

devlod.obj:     devlod.c
        bcc -c -ms devlod.c

devlod.com:     devlod.obj c0.obj movup.obj
        tlink /t c0 movup devlod /c/m,devlod

clean:
	del *.obj
	del *.map
	del *.lst

distclean: clean
	del devlod.com
