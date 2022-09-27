#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"

/* pc140x
   16 5x7 with space between char
   6000 .. 6027, 6067.. 6040
  603c: 3 STAT
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 HYP, 4 PRO, 5 RUN, 6 CAL
  607c: 0 E, 1 M, 2 (), 3 RAD, 4 G, 5 DE, 6 PRINT */

/* pc1421
   16 5x7 with space between char
   6000 .. 6027, 6067.. 6040
  603c: 3 RUN
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 BGN, 4 STAT, 5 FIN, 6 PRINT
  607c: 0 E, 1 M, 2 BAL, 3 INT, 4 PRN, 5 Sum-Sign, 6 PRO */

static struct {
	UINT8 reg[0x100];
} pc1401_lcd;

 READ8_HANDLER(pc1401_lcd_read)
{
	offset&=0xff;
	return pc1401_lcd.reg[offset];
}

WRITE8_HANDLER(pc1401_lcd_write)
{
	offset&=0xff;
	pc1401_lcd.reg[offset]=data;
}

static const POCKETC_FIGURE	line={ /* simple line */
	"11111",
	"11111",
	"11111e" 
};
static const POCKETC_FIGURE busy={ 
	"11  1 1  11 1 1",
	"1 1 1 1 1   1 1",
	"11  1 1  1  1 1",
	"1 1 1 1   1  1",
	"11   1  11   1e"
}, def={ 
	"11  111 111",
	"1 1 1   1",
	"1 1 111 11",
	"1 1 1   1",
	"11  111 1e" 
}, shift={
	" 11 1 1 1 111 111",
	"1   1 1 1 1    1",
	" 1  111 1 11   1",
	"  1 1 1 1 1    1",
	"11  1 1 1 1    1e" 
}, hyp={
	"1 1 1 1 11",
	"1 1 1 1 1 1",
	"111 1 1 11",
	"1 1  1  1",
	"1 1  1  1e" 
}, de={
	"11  111",
	"1 1 1",
	"1 1 111",
	"1 1 1",
	"11  111e"
}, g={
	" 11",
	"1",
	"1 1",
	"1 1",
	" 11e" 
}, rad={
	"11   1  11",
	"1 1 1 1 1 1",
	"11  111 1 1",
	"1 1 1 1 1 1",
	"1 1 1 1 11e"
}, braces={
	" 1 1",
	"1   1",
	"1   1",
	"1   1",
	" 1 1e"
}, m={
	"1   1",
	"11 11",
	"1 1 1",
	"1   1",
	"1   1e"
}, e={
	"111",
	"1",
	"111",
	"1",
	"111e" 
}, run={ 
	"11  1 1 1  1",
	"1 1 1 1 11 1",
	"11  1 1 1 11",
	"1 1 1 1 1  1",
	"1 1  1  1  1e" 
}, pro={ 
	"11  11   1  ",
	"1 1 1 1 1 1",
	"11  11  1 1",
	"1   1 1 1 1",
	"1   1 1  1e" 
}, japan={ 
	"  1  1  11   1  1  1",
	"  1 1 1 1 1 1 1 11 1",
	"  1 111 11  111 1 11",
	"1 1 1 1 1   1 1 1  1",
	" 1  1 1 1   1 1 1  1e" 
}, sml={ 
	" 11 1 1 1",
	"1   111 1",
	" 1  1 1 1",
	"  1 1 1 1",
	"11  1 1 111e" 
}, rsv={ 
	"11   11 1   1",
	"1 1 1   1   1",
	"11   1   1 1",
	"1 1   1  1 1",
	"1 1 11    1e" 
};

#define DOWN 57
#define RIGHT 114

VIDEO_UPDATE( pc1401 )
{
	int x, y, i, j;
	int color[2];
	/* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = Machine->pens[pocketc_colortable[CONTRAST][0]];
	color[1] = Machine->pens[pocketc_colortable[CONTRAST][1]];
    
    if (pc1401_portc&1) {
		for (x=RIGHT,y=DOWN,i=0; i<0x28;x+=2) {
			for (j=0; j<5;j++,i++,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1401_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=0x67; i>=0x40;x+=2) {
			for (j=0; j<5;j++,i--,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1401_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
    }

    pocketc_draw_special(bitmap,RIGHT+149,DOWN+24,line,
			 pc1401_lcd.reg[0x3c]&8?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT,DOWN-10,busy,
			 pc1401_lcd.reg[0x3d]&1?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+18,DOWN-10,def,
			 pc1401_lcd.reg[0x3d]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+43,DOWN-10,shift,
			 pc1401_lcd.reg[0x3d]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+63,DOWN-10,hyp,
			 pc1401_lcd.reg[0x3d]&8?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+38,DOWN+24,line,
			 pc1401_lcd.reg[0x3d]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+23,DOWN+24,line,
			 pc1401_lcd.reg[0x3d]&0x20?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+8,DOWN+24,line,
			 pc1401_lcd.reg[0x3d]&0x40?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+183,DOWN-10,e,
			 pc1401_lcd.reg[0x7c]&1?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+176,DOWN-10,m,
			 pc1401_lcd.reg[0x7c]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+168,DOWN-10,braces,
			 pc1401_lcd.reg[0x7c]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+138,DOWN-10,rad,
			 pc1401_lcd.reg[0x7c]&8?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+134,DOWN-10,g,
			 pc1401_lcd.reg[0x7c]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+126,DOWN-10,de,
			 pc1401_lcd.reg[0x7c]&0x20?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+165,DOWN+24,line,
			 pc1401_lcd.reg[0x7c]&0x40?color[1]:color[0]);

/*
  603c: 3 STAT
  603d: 0 BUSY, 1 DEF, 2 SHIFT, 3 HYP, 4 PRO, 5 RUN, 6 CAL
  607c: 0 E, 1 M, 2 (), 3 RAD, 4 G, 5 DE, 6 PRINT
*/
}

