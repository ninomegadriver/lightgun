#include "driver.h"
#include "sound/ym2151.h"
#include "sound/namco.h"

#define NAMCOS1_MAX_BANK 0x400

/* from vidhrdw */
READ8_HANDLER( namcos1_videoram_r );
WRITE8_HANDLER( namcos1_videoram_w );
WRITE8_HANDLER( namcos1_paletteram_w );
READ8_HANDLER( namcos1_spriteram_r );
WRITE8_HANDLER( namcos1_spriteram_w );
extern void namcos1_set_scroll_offsets( const int *bgx, const int *bgy, int negative, int optimize );
extern void namcos1_set_sprite_offsets( int x, int y );

UINT8 *namcos1_paletteram;

static UINT8 *namcos1_triram;
static UINT8 *s1ram;


/*******************************************************************************
*                                                                              *
*   BANK area handling                                                         *
*                                                                              *
*******************************************************************************/

/* Bank handler definitions */
typedef struct
{
	read8_handler bank_handler_r;
	write8_handler bank_handler_w;
	int           bank_offset;
	unsigned char *bank_pointer;
} bankhandler;

/* hardware elements of 1Mbytes physical memory space */
static bankhandler namcos1_bank_element[NAMCOS1_MAX_BANK];
static bankhandler namcos1_active_bank[16];

static READ8_HANDLER( bank1_r )  { return (*namcos1_active_bank[0].bank_handler_r )(offset + namcos1_active_bank[0].bank_offset); }
static READ8_HANDLER( bank2_r )  { return (*namcos1_active_bank[1].bank_handler_r )(offset + namcos1_active_bank[1].bank_offset); }
static READ8_HANDLER( bank3_r )  { return (*namcos1_active_bank[2].bank_handler_r )(offset + namcos1_active_bank[2].bank_offset); }
static READ8_HANDLER( bank4_r )  { return (*namcos1_active_bank[3].bank_handler_r )(offset + namcos1_active_bank[3].bank_offset); }
static READ8_HANDLER( bank5_r )  { return (*namcos1_active_bank[4].bank_handler_r )(offset + namcos1_active_bank[4].bank_offset); }
static READ8_HANDLER( bank6_r )  { return (*namcos1_active_bank[5].bank_handler_r )(offset + namcos1_active_bank[5].bank_offset); }
static READ8_HANDLER( bank7_r )  { return (*namcos1_active_bank[6].bank_handler_r )(offset + namcos1_active_bank[6].bank_offset); }
static READ8_HANDLER( bank8_r )  { return (*namcos1_active_bank[7].bank_handler_r )(offset + namcos1_active_bank[7].bank_offset); }
static READ8_HANDLER( bank9_r )  { return (*namcos1_active_bank[8].bank_handler_r )(offset + namcos1_active_bank[8].bank_offset); }
static READ8_HANDLER( bank10_r ) { return (*namcos1_active_bank[9].bank_handler_r )(offset + namcos1_active_bank[9].bank_offset); }
static READ8_HANDLER( bank11_r ) { return (*namcos1_active_bank[10].bank_handler_r)(offset + namcos1_active_bank[10].bank_offset); }
static READ8_HANDLER( bank12_r ) { return (*namcos1_active_bank[11].bank_handler_r)(offset + namcos1_active_bank[11].bank_offset); }
static READ8_HANDLER( bank13_r ) { return (*namcos1_active_bank[12].bank_handler_r)(offset + namcos1_active_bank[12].bank_offset); }
static READ8_HANDLER( bank14_r ) { return (*namcos1_active_bank[13].bank_handler_r)(offset + namcos1_active_bank[13].bank_offset); }
static READ8_HANDLER( bank15_r ) { return (*namcos1_active_bank[14].bank_handler_r)(offset + namcos1_active_bank[14].bank_offset); }
static READ8_HANDLER( bank16_r ) { return (*namcos1_active_bank[15].bank_handler_r)(offset + namcos1_active_bank[15].bank_offset); }

static WRITE8_HANDLER( bank1_w )  { (*namcos1_active_bank[0].bank_handler_w )(offset + namcos1_active_bank[0].bank_offset, data); }
static WRITE8_HANDLER( bank2_w )  { (*namcos1_active_bank[1].bank_handler_w )(offset + namcos1_active_bank[1].bank_offset, data); }
static WRITE8_HANDLER( bank3_w )  { (*namcos1_active_bank[2].bank_handler_w )(offset + namcos1_active_bank[2].bank_offset, data); }
static WRITE8_HANDLER( bank4_w )  { (*namcos1_active_bank[3].bank_handler_w )(offset + namcos1_active_bank[3].bank_offset, data); }
static WRITE8_HANDLER( bank5_w )  { (*namcos1_active_bank[4].bank_handler_w )(offset + namcos1_active_bank[4].bank_offset, data); }
static WRITE8_HANDLER( bank6_w )  { (*namcos1_active_bank[5].bank_handler_w )(offset + namcos1_active_bank[5].bank_offset, data); }
static WRITE8_HANDLER( bank7_w )  { (*namcos1_active_bank[6].bank_handler_w )(offset + namcos1_active_bank[6].bank_offset, data); }
static WRITE8_HANDLER( bank8_w )  { (*namcos1_active_bank[7].bank_handler_w )(offset + namcos1_active_bank[7].bank_offset, data); }
static WRITE8_HANDLER( bank9_w )  { (*namcos1_active_bank[8].bank_handler_w )(offset + namcos1_active_bank[8].bank_offset, data); }
static WRITE8_HANDLER( bank10_w ) { (*namcos1_active_bank[9].bank_handler_w )(offset + namcos1_active_bank[9].bank_offset, data); }
static WRITE8_HANDLER( bank11_w ) { (*namcos1_active_bank[10].bank_handler_w)(offset + namcos1_active_bank[10].bank_offset, data); }
static WRITE8_HANDLER( bank12_w ) { (*namcos1_active_bank[11].bank_handler_w)(offset + namcos1_active_bank[11].bank_offset, data); }
static WRITE8_HANDLER( bank13_w ) { (*namcos1_active_bank[12].bank_handler_w)(offset + namcos1_active_bank[12].bank_offset, data); }
static WRITE8_HANDLER( bank14_w ) { (*namcos1_active_bank[13].bank_handler_w)(offset + namcos1_active_bank[13].bank_offset, data); }
static WRITE8_HANDLER( bank15_w ) { (*namcos1_active_bank[14].bank_handler_w)(offset + namcos1_active_bank[14].bank_offset, data); }
static WRITE8_HANDLER( bank16_w ) { (*namcos1_active_bank[15].bank_handler_w)(offset + namcos1_active_bank[15].bank_offset, data); }

static const read8_handler ram_bank_handler_r[16] =
{
	MRA8_BANK1 ,MRA8_BANK2 ,MRA8_BANK3 ,MRA8_BANK4 ,
	MRA8_BANK5 ,MRA8_BANK6 ,MRA8_BANK7 ,MRA8_BANK8 ,
	MRA8_BANK9 ,MRA8_BANK10,MRA8_BANK11,MRA8_BANK12,
	MRA8_BANK13,MRA8_BANK14,MRA8_BANK15,MRA8_BANK16
};

static const write8_handler ram_bank_handler_w[16] =
{
	MWA8_BANK1 ,MWA8_BANK2 ,MWA8_BANK3 ,MWA8_BANK4 ,
	MWA8_BANK5 ,MWA8_BANK6 ,MWA8_BANK7 ,MWA8_BANK8 ,
	MWA8_BANK9 ,MWA8_BANK10,MWA8_BANK11,MWA8_BANK12,
	MWA8_BANK13,MWA8_BANK14,MWA8_BANK15,MWA8_BANK16
};

static const read8_handler io_bank_handler_r[16] =
{
	bank1_r, bank2_r, bank3_r, bank4_r,
	bank5_r, bank6_r, bank7_r, bank8_r,
	bank9_r, bank10_r, bank11_r, bank12_r,
	bank13_r, bank14_r, bank15_r, bank16_r
};

static const write8_handler io_bank_handler_w[16] =
{
	bank1_w, bank2_w, bank3_w, bank4_w,
	bank5_w, bank6_w, bank7_w, bank8_w,
	bank9_w, bank10_w, bank11_w, bank12_w,
	bank13_w, bank14_w, bank15_w, bank16_w
};



static WRITE8_HANDLER( namcos1_3dcs_w )
{
	if (offset & 1)	ui_popup("LEFT");
	else			ui_popup("RIGHT");
}



static int key_id,key_reg,key_rng,key_swap4_arg,key_swap4,key_bottom4,key_top4;
static UINT8 key[8];


static READ8_HANDLER( no_key_r )
{
	ui_popup("CPU #%d PC %08x: keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);
	return 0;
}

static WRITE8_HANDLER( no_key_w )
{
	ui_popup("CPU #%d PC %08x: keychip write %04x=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);
}


/*****************************************************************************

  Key custom type 1 (CUS136, CUS143, CUS144)

  16/8 bit division + key no.

*****************************************************************************/

/*
the startup checks are the same in the three games using this kind of chip,
apart from the [operating mode?] values written.

dspirit: (CUS136)
CPU #0 PC da06: keychip write 0000=01 [maybe to initialize chip?]
CPU #0 PC da09: keychip read 0003     [return key no.]
CPU #0 PC da14: keychip write 0003=f2 [operating mode?]
CPU #0 PC da1d: keychip write 0000=xx [ \ operands]
CPU #0 PC da22: keychip write 0001=xx [    "    " ]
CPU #0 PC da22: keychip write 0002=xx [    "    " ]
CPU #0 PC da25: keychip read 0001     [   division result (verified to be correct)]
CPU #0 PC da25: keychip read 0002     [    "           "  (verified to be correct)]
CPU #0 PC da28: keychip read 0000     [ /  "           "  (verified to be correct)]
CPU #0 PC daf6: keychip write 0003=c2 [operating mode?]
CPU #0 PC dafa: keychip write 0002=00-ff [???????]
CPU #0 PC db00: keychip read 0003     [???? - stored in 0001]
CPU #0 PC db07: keychip write 0001=82-ff [ \ ???????]
CPU #0 PC db07: keychip write 0002=8d    [ / ???????]
CPU #0 PC db0d: keychip read 0003     [???? - stored in 0002]
CPU #0 PC db14: keychip read 0001     [ \ ???????]
CPU #0 PC db14: keychip read 0002     [   ???????]
CPU #0 PC db17: keychip write 0001=xx [   ???????]
CPU #0 PC db17: keychip write 0002=xx [ / ???????]
CPU #0 PC db1d: keychip read 0001     [???? - stored in 0003]
CPU #0 PC db1d: keychip read 0002     [???? - stored in 0004]
CPU #0 PC db24: keychip write 0003=f2 [operating mode?]
---- end of common checks
CPU #0 PC 8191: keychip write 0003=01 [????]
CPU #0 PC 8196: keychip write 0000=40 [ \ operands]
CPU #0 PC 819c: keychip write 0002=00 [    "    " ]
CPU #0 PC 819f: keychip write 0001=04 [    "    " ]
CPU #0 PC 81a2: keychip read 0001     [   division result (verified to be correct)]
CPU #0 PC 81a2: keychip read 0002     [ /  "           "  (verified to be correct)]
if division result is wrong, reads again key no. from 03, and doesn't crash if it's correct

wldcourt: (CUS143)
CPU #0 PC db0a: keychip write 0000=01 [maybe to initialize chip?]
CPU #0 PC db0d: keychip read 0003     [return key no.]
CPU #0 PC db18: keychip write 0003=d9 [operating mode?]
CPU #0 PC db21: keychip write 0000=xx [ \ operands]
CPU #0 PC db26: keychip write 0001=xx [    "    " ]
CPU #0 PC db26: keychip write 0002=xx [    "    " ]
CPU #0 PC db29: keychip read 0001     [   division result (verified to be correct)]
CPU #0 PC db29: keychip read 0002     [    "           "  (verified to be correct)]
CPU #0 PC db2c: keychip read 0000     [ /  "           "  (verified to be correct)]
CPU #0 PC dbfa: keychip write 0003=db [operating mode?]
CPU #0 PC dbfe: keychip write 0002=00-ff [???????]
CPU #0 PC dc04: keychip read 0003     [???? - stored in 0001]
CPU #0 PC dc0b: keychip write 0001=82-ff [ \ ???????]
CPU #0 PC dc0b: keychip write 0002=8d    [ / ???????]
CPU #0 PC dc11: keychip read 0003     [???? - stored in 0002]
CPU #0 PC dc18: keychip read 0001     [ \ ???????]
CPU #0 PC dc18: keychip read 0002     [   ???????]
CPU #0 PC dc1b: keychip write 0001=xx [   ???????]
CPU #0 PC dc1b: keychip write 0002=xx [ / ???????]
CPU #0 PC dc21: keychip read 0001     [???? - stored in 0003]
CPU #0 PC dc21: keychip read 0002     [???? - stored in 0004]
CPU #0 PC dc28: keychip write 0003=d9 [operating mode?]
---- end of common checks
CPU #0 PC e31c: keychip read 0003     [return key no.]

blazer: (CUS144)
CPU #0 PC da0b: keychip write 0000=01 [maybe to initialize chip?]
CPU #0 PC da0e: keychip read 0003     [return key no.]
CPU #0 PC da19: keychip write 0003=b7 [operating mode?]
CPU #0 PC da22: keychip write 0000=12 [ \ operands]
CPU #0 PC da27: keychip write 0001=0a [    "    " ]
CPU #0 PC da27: keychip write 0002=95 [    "    " ]
CPU #0 PC da2a: keychip read 0001     [   division result (verified to be correct)]
CPU #0 PC da2a: keychip read 0002     [    "           "  (verified to be correct)]
CPU #0 PC da2d: keychip read 0000     [ /  "           "  (verified to be correct)]
CPU #0 PC dafb: keychip write 0003=b6 [operating mode?]
CPU #0 PC daff: keychip write 0002=00-ff [???????]
CPU #0 PC db05: keychip read 0003     [???? - stored in 0001]
CPU #0 PC db0c: keychip write 0001=82-ff [ \ ???????]
CPU #0 PC db0c: keychip write 0002=8d    [ / ???????]
CPU #0 PC db12: keychip read 0003     [???? - stored in 0002]
CPU #0 PC db19: keychip read 0001     [ \ ???????]
CPU #0 PC db19: keychip read 0002     [   ???????]
CPU #0 PC db1c: keychip write 0001=xx [   ???????]
CPU #0 PC db1c: keychip write 0002=xx [ / ???????]
CPU #0 PC db22: keychip read 0001     [???? - stored in 0003]
CPU #0 PC db22: keychip read 0002     [???? - stored in 0004]
CPU #0 PC db29: keychip write 0003=b7 [operating mode?]
---- end of common checks

puzlclub:
CPU #0 PC e017: keychip write 0003=35 [they probably used RAM instead of a key chip for this prototype]
CPU #0 PC e3d4: keychip read 0003     [AND #$37 = key no.]
*/
static READ8_HANDLER( key_type1_r )
{
//  logerror("CPU #%d PC %04x: keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);

	if (offset < 3)
	{
		int d = key[0];
		int n = (key[1] << 8) | key[2];
		int q,r;

		if (d)
		{
			q = n / d;
			r = n % d;
		}
		else
		{
			q = 0xffff;
			r = 0x00;
		}

		if (offset == 0) return r;
		if (offset == 1) return q >> 8;
		if (offset == 2) return q & 0xff;
	}
	else if (offset == 3)
		return key_id;

	return 0;
}

static WRITE8_HANDLER( key_type1_w )
{
//  logerror("CPU #%d PC %04x: keychip write %04x=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);

	if (offset < 4)
		key[offset] = data;
}


/*****************************************************************************

  Key custom type 2 (CUS151 - CUS155)

  16/16 bit division + key no.

*****************************************************************************/

/*
pacmania: (CUS151)
CPU #0 PC dac2: keychip write 0004=4b [operating mode?]
CPU #0 PC dac5: keychip read 0004     [AND #$37 = key no.]
CPU #0 PC dad9: keychip write 0000=xx
CPU #0 PC dad9: keychip write 0001=xx
CPU #0 PC dae0: keychip write 0002=xx
CPU #0 PC dae0: keychip write 0003=xx
CPU #0 PC dae4: keychip read 0000
CPU #0 PC dae4: keychip read 0001
CPU #0 PC daed: keychip read 0002
CPU #0 PC daed: keychip read 0003
CPU #1 PC f188: keychip write 0004=4b [operating mode?]
CPU #1 PC f2d0: keychip write 0000=01
CPU #1 PC f2d0: keychip write 0001=aa
CPU #1 PC f30d: keychip write 0002=xx
CPU #1 PC f30d: keychip write 0003=xx
CPU #1 PC f310: keychip read 0003
CPU #1 PC f55d: keychip write 0000=00
CPU #1 PC f55d: keychip write 0001=10
CPU #1 PC f562: keychip write 0004=4b [operating mode?]
CPU #1 PC f5d3: keychip write 0002=xx
CPU #1 PC f5d3: keychip write 0003=xx
CPU #1 PC f5d6: keychip read 0003
CPU #1 PC f5ef: keychip read 0001
CPU #1 PC cbc6: keychip write 0000=00
CPU #1 PC cbc6: keychip write 0001=02
CPU #1 PC cbdd: keychip write 0002=xx
CPU #1 PC cbdd: keychip write 0003=xx
CPU #1 PC cbe8: keychip read 0002
CPU #1 PC cbe8: keychip read 0003

mmaze: (CUS152)
CPU #0 PC 60f3: keychip write 0004=5b
CPU #0 PC 60fe: keychip write 0000=00
CPU #0 PC 60fe: keychip write 0001=01
CPU #0 PC 6101: keychip write 0002=00
CPU #0 PC 6101: keychip write 0003=01
CPU #0 PC 6104: keychip read 0004
CPU #0 PC 61ae: keychip write 0000=00
CPU #0 PC 61ae: keychip write 0001=33
CPU #0 PC 61b1: keychip write 0002=02
CPU #0 PC 61b1: keychip write 0003=00
CPU #0 PC 61b4: keychip read 0002
CPU #0 PC 61b4: keychip read 0003
CPU #0 PC 68fa: keychip write 0000=01
CPU #0 PC 68fa: keychip write 0001=00
CPU #0 PC 68fd: keychip write 0002=00
CPU #0 PC 68fd: keychip write 0003=00
CPU #0 PC 6900: keychip read 0002
CPU #0 PC 6900: keychip read 0003
CPU #0 PC bf6c: keychip write 0000=00
CPU #0 PC bf6c: keychip write 0001=20
CPU #0 PC bf6f: keychip write 0002=00
CPU #0 PC bf6f: keychip write 0003=00
CPU #0 PC bf72: keychip read 0002
CPU #0 PC bf72: keychip read 0003
CPU #0 PC 9576: keychip write 0000=00
CPU #0 PC 9576: keychip write 0001=14
CPU #0 PC 9579: keychip write 0002=00
CPU #0 PC 9579: keychip write 0003=00
CPU #0 PC 957c: keychip read 0002
CPU #0 PC 957c: keychip read 0003
CPU #0 PC 6538: keychip write 0000=00
CPU #0 PC 6538: keychip write 0001=15
CPU #0 PC 653b: keychip write 0002=00
CPU #0 PC 653b: keychip write 0003=01
CPU #0 PC 653e: keychip read 0002
CPU #0 PC 653e: keychip read 0003

galaga88: (CUS153)
CPU #0 PC db02: keychip write 0004=2d [operating mode?]
CPU #0 PC db05: keychip read 0004     [AND #$7F = key no.]
CPU #0 PC db17: keychip write 0000=xx
CPU #0 PC db17: keychip write 0001=xx
CPU #0 PC db1e: keychip write 0002=xx
CPU #0 PC db1e: keychip write 0003=xx
CPU #0 PC db22: keychip read 0000
CPU #0 PC db22: keychip read 0001
CPU #0 PC db2b: keychip read 0002
CPU #0 PC db2b: keychip read 0003
CPU #0 PC e136: keychip write 0004=0c

ws: (CUS154)
CPU #0 PC e052: keychip write 0004=d3 [operating mode?]
CPU #0 PC e82a: keychip read 0004     [AND #$3F = key no.]

bakutotu: (CUS155)
CPU #0 PC db38: keychip write 0004=03 [operating mode?]
CPU #0 PC db3b: keychip read 0004     [AND #$37 = key no.]
CPU #0 PC db4f: keychip write 0000=xx
CPU #0 PC db4f: keychip write 0001=xx
CPU #0 PC db56: keychip write 0002=xx
CPU #0 PC db56: keychip write 0003=xx
CPU #0 PC db5a: keychip read 0000
CPU #0 PC db5a: keychip read 0001
CPU #0 PC db63: keychip read 0002
CPU #0 PC db63: keychip read 0003
CPU #0 PC db70: keychip write 0004=0b
CPU #0 PC db77: keychip write 0000=ff
CPU #0 PC db77: keychip write 0001=ff
CPU #0 PC db7e: keychip write 0002=ff
CPU #0 PC db7e: keychip write 0003=ff
CPU #0 PC db82: keychip read 0000
CPU #0 PC db82: keychip read 0001
CPU #0 PC db8b: keychip read 0002
CPU #0 PC db8b: keychip read 0003
CPU #0 PC db95: keychip write 0004=03
CPU #0 PC e552: keychip read 0004     [AND #$3F = key no.]
CPU #0 PC e560: keychip write 0000=00
CPU #0 PC e560: keychip write 0001=73
CPU #0 PC e566: keychip write 0002=a4
CPU #0 PC e566: keychip write 0003=83
CPU #0 PC e569: keychip read 0002
CPU #0 PC e569: keychip read 0003
CPU #0 PC e574: keychip read 0000
CPU #0 PC e574: keychip read 0001

*/

static READ8_HANDLER( key_type2_r )
{
//  logerror("CPU #%d PC %04x: keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);

	if (offset < 4)
	{
		int d = (key[0] << 8) | key[1];
		int n = (key[2] << 8) | key[3];
		int q,r;

		if (d)
		{
			q = n / d;
			r = n % d;
		}
		else
		{
			q = 0xffff;
			r = 0x00;
		}

		if (offset == 0) return r >> 8;
		if (offset == 1) return r & 0xff;
		if (offset == 2) return q >> 8;
		if (offset == 3) return q & 0xff;
	}
	else if (offset == 4)
		return key_id;

	return 0;
}

static WRITE8_HANDLER( key_type2_w )
{
//  logerror("CPU #%d PC %04x: keychip write %04x=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);

	if (offset < 5)
		key[offset] = data;
}


/*****************************************************************************

  Key custom type 3 (CUS181 - CUS185, CUS308 - CUS311)

  Nibble swapper + rng + key no.

*****************************************************************************/

/*
splatter: (CUS181)
CPU #0 PC cd66: keychip write 003f=01 [maybe to initialize chip?]
CPU #0 PC cd78: keychip read 004f     [rng]
CPU #0 PC cd91: keychip read 003f     [key no.]
CPU #0 PC f90c: keychip read 0036     [key no.]
CPU #0 PC f91f: keychip read 0036     [key no.]

rompers: (CUS182)
CPU #0 PC b891: keychip read 0070     [key no.]

blastoff: (CUS183)
CPU #0 PC db4f: keychip read 0000     [key no.]
CPU #0 PC db55: keychip read 0070     [rng?]
CPU #0 PC db57: keychip write 0030=xx [ARG for 58]
CPU #0 PC db60: keychip write 0858=xx [debug leftover?]
CPU #0 PC db67: keychip read 0858     [debug leftover?]
CPU #0 PC db69: keychip read 0058     [ARG >> 4) | (ARG << 4]
CPU #0 PC e01a: keychip read 0000     [key no.]
CPU #0 PC e021: keychip read 0070     [rng?]
CPU #0 PC e024: keychip write 0030=xx [arg for 58]
CPU #0 PC e037: keychip read 0058     [ARG >> 4) | (ARG << 4]
CPU #0 PC 6989: keychip read 0000     [ignored]
CPU #0 PC 6989: keychip read 0001     [ignored]

ws89: (CUS184)
CPU #0 PC e050: keychip read 0020     [key no.]
CPU #0 PC e835: keychip read 0020     [key no.]

tankfrce: (CUS185)
CPU #0 PC c441: keychip read 0057     [key no.]
CPU #0 PC c444: keychip write 0017=b9 [ARG for 2b]
CPU #0 PC c447: keychip read 002b     [0xb0 | (ARG & 0x0f)]
CPU #0 PC f700: keychip read 0050     [key no.]

dangseed: (CUS308)
CPU #0 PC c628: keychip read 0067     [key no.]
CPU #0 PC c62b: keychip write 0057=34 [ARG for 03]
CPU #0 PC c62e: keychip read 0003     [0x30 | (ARG & 0x0f)]
CPU #1 PC feb8: keychip write 0050=xx [ARG for 41 and 01]
CPU #1 PC febb: keychip read 0041     [0x10 | (ARG >> 4)]   used for stars display
CPU #1 PC fec0: keychip read 0001     [0x10 | (ARG & 0x0f)] used for stars display
CPU #1 PC f74d: keychip write 0050=xx [ARG for 40 and 00]
CPU #1 PC f750: keychip read 0040     [0x00 | (ARG >> 4)]   used for score display
CPU #1 PC f755: keychip read 0000     [0x00 | (ARG & 0x0f)] used for score display
CPU #1 PC c310: keychip write 0050=xx [ARG for 00]
CPU #1 PC c31e: keychip read 0000     [0x00 | (ARG & 0x0f)] used for credits display

pistoldm: (CUS309)
CPU #0 PC c84a: keychip read 0017     [key no.]
CPU #0 PC c84d: keychip write 0007=35 [ARG for 43]
CPU #0 PC c850: keychip read 0043     [0x30 | (ARG & 0x0f)]
CPU #0 PC cdef: keychip read 0020     [rng] used for several random events

ws90: (CUS310)
CPU #0 PC c82f: keychip read 0047     [key no.]
CPU #0 PC c832: keychip write 007f=36 [ARG for 33]
CPU #0 PC c835: keychip read 0033     [0x30 | (ARG & 0x0f)]
CPU #0 PC e828: keychip read 0040     [key no.]

soukobdx: (CUS311)
CPU #0 PC ca90: keychip read 0027     [key no.]
CPU #0 PC ca93: keychip write 0007=37 [ARG for 43]
CPU #0 PC ca96: keychip read 0043     [0x30 | (ARG & 0x0f)]
CPU #0 PC e45a: keychip read 0030     [discarded]
*/

static READ8_HANDLER( key_type3_r )
{
	int op;

//  logerror("CPU #%d PC %04x: keychip read %04x\n",cpu_getactivecpu(),activecpu_get_pc(),offset);

	/* I need to handle blastoff's read from 0858. The game previously writes to 0858,
       using it as temporary storage, so maybe it expects to act as RAM, however
       it happens to work correctly also using the standard handling for 0058.
       The schematics don't show A11 being used, so I go for this handling.
      */
	op = (offset & 0x70) >> 4;

	if (op == key_reg)		return key_id;
	if (op == key_rng)		return mame_rand();
	if (op == key_swap4)	return (key[key_swap4_arg] << 4) | (key[key_swap4_arg] >> 4);
	if (op == key_bottom4)	return (offset << 4) | (key[key_swap4_arg] & 0x0f);
	if (op == key_top4)		return (offset << 4) | (key[key_swap4_arg] >> 4);

	ui_popup("CPU #%d PC %08x: keychip read %04x",cpu_getactivecpu(),activecpu_get_pc(),offset);

	return 0;
}

static WRITE8_HANDLER( key_type3_w )
{
//  logerror("CPU #%d PC %04x: keychip write %04x=%02x\n",cpu_getactivecpu(),activecpu_get_pc(),offset,data);

	key[(offset & 0x70) >> 4] = data;
}



/*******************************************************************************
*                                                                              *
*   Sound banking emulation (CUS121)                                           *
*                                                                              *
*******************************************************************************/

WRITE8_HANDLER( namcos1_sound_bankswitch_w )
{
	UINT8 *rom = memory_region(REGION_CPU3) + 0xc000;

	int bank = (data & 0x70) >> 4;
	memory_set_bankptr(17,rom + 0x4000 * bank);
}



/*******************************************************************************
*                                                                              *
*   Banking emulation (CUS117)                                                 *
*                                                                              *
*******************************************************************************/

static int mcu_patch_data;
static int namcos1_reset = 0;

WRITE8_HANDLER( namcos1_cpu_control_w )
{
//  logerror("reset control pc=%04x %02x\n",activecpu_get_pc(),data);
	if ((data & 1) ^ namcos1_reset)
	{
		mcu_patch_data = 0;
		namcos1_reset = data & 1;
	}

	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(2, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
}



WRITE8_HANDLER( namcos1_watchdog_w )
{
	static int wdog;

	wdog |= 1 << (cpu_getactivecpu());
	if (wdog == 7 || !namcos1_reset)
	{
		wdog = 0;
		watchdog_reset_w(0,0);
	}
}



static READ8_HANDLER( soundram_r )
{
	if (offset < 0x1000)
	{
		offset &= 0x3ff;

		/* CUS 30 */
		return namcos1_cus30_r(offset);
	}
	else
	{
		offset &= 0x7ff;

		/* shared ram */
		return namcos1_triram[offset];
	}
}

static WRITE8_HANDLER( soundram_w )
{
	if (offset < 0x1000)
	{
		offset &= 0x3ff;

		/* CUS 30 */
		namcos1_cus30_w(offset,data);
	}
	else
	{
		offset &= 0x7ff;

		/* shared ram */
		namcos1_triram[offset] = data;
		return;
	}
}

/* ROM handlers */

static WRITE8_HANDLER( rom_w )
{
	logerror("CPU #%d PC %04x: warning - write %02x to rom address %04x\n",cpu_getactivecpu(),activecpu_get_pc(),data,offset);
}

/* error handlers */
static READ8_HANDLER( unknown_r )
{
	logerror("CPU #%d PC %04x: warning - read from unknown chip\n",cpu_getactivecpu(),activecpu_get_pc() );
//  ui_popup("CPU #%d PC %04x: read from unknown chip",cpu_getactivecpu(),activecpu_get_pc() );
	return 0;
}

static WRITE8_HANDLER( unknown_w )
{
	logerror("CPU #%d PC %04x: warning - wrote to unknown chip\n",cpu_getactivecpu(),activecpu_get_pc() );
//  ui_popup("CPU #%d PC %04x: wrote to unknown chip",cpu_getactivecpu(),activecpu_get_pc() );
}

/* Main bankswitching routine */
static void set_bank(int banknum, bankhandler *handler)
{
	int bankstart = (banknum & 7) * 0x2000;
	int cpunum = (banknum >> 3) & 1;

	/* for BANK handlers , memory direct and OP-code base */
	if (handler->bank_pointer)
		memory_set_bankptr(banknum + 1, handler->bank_pointer);

	/* read handlers */
	if (!handler->bank_handler_r)
	{
		if (namcos1_active_bank[banknum].bank_handler_r)
			memory_install_read8_handler(cpunum, ADDRESS_SPACE_PROGRAM, bankstart, bankstart + 0x1fff, 0, 0, ram_bank_handler_r[banknum]);
	}
	else
	{
		if (!namcos1_active_bank[banknum].bank_handler_r)
			memory_install_read8_handler(cpunum, ADDRESS_SPACE_PROGRAM, bankstart, bankstart + 0x1fff, 0, 0, io_bank_handler_r[banknum]);
	}

	/* write handlers (except for the 0xe000-0xffff range) */
	if (bankstart != 0xe000)
	{
		if (!handler->bank_handler_w)
		{
			if (namcos1_active_bank[banknum].bank_handler_w)
				memory_install_write8_handler(cpunum, ADDRESS_SPACE_PROGRAM, bankstart, bankstart + 0x1fff, 0, 0, ram_bank_handler_w[banknum]);
		}
		else
		{
			if (!namcos1_active_bank[banknum].bank_handler_r)
				memory_install_write8_handler(cpunum, ADDRESS_SPACE_PROGRAM, bankstart, bankstart + 0x1fff, 0, 0, io_bank_handler_w[banknum]);
		}
	}

	/* Remember this bank handler */
	namcos1_active_bank[banknum] = *handler;
}

void namcos1_bankswitch(int cpu, offs_t offset, UINT8 data)
{
	static int chip[16];
	int bank = (cpu*8) + (( offset >> 9) & 0x07);

	if (offset & 1)
	{
		chip[bank] &= 0x0300;
		chip[bank] |= (data & 0xff);
	}
	else
	{
		chip[bank] &= 0x00ff;
		chip[bank] |= (data & 0x03) << 8;
	}

	set_bank(bank, &namcos1_bank_element[chip[bank]]);

	/* unmapped bank warning */
	if( namcos1_active_bank[bank].bank_handler_r == unknown_r)
	{
		logerror("CPU #%d PC %04x:warning unknown chip selected bank %x=$%04x\n", cpu , activecpu_get_pc(), bank , chip[bank] );
//          if (chip) ui_popup("CPU #%d PC %04x:unknown chip selected bank %x=$%04x", cpu , activecpu_get_pc(), bank , chip[bank] );
	}

	/* renew pc base */
//  change_pc(activecpu_get_pc());
}

WRITE8_HANDLER( namcos1_bankswitch_w )
{
//  logerror("cpu %d: namcos1_bankswitch_w offset %04x data %02x\n",cpu_getactivecpu(),offset,data);

	namcos1_bankswitch(cpu_getactivecpu(), offset, data);
}

/* Sub cpu set start bank port */
WRITE8_HANDLER( namcos1_subcpu_bank_w )
{
//  logerror("namcos1_subcpu_bank_w offset %04x data %02x\n",offset,data);

	/* Prepare code for CPU 1 */
	namcos1_bankswitch( 1, 0x0e00, 0x03 );
	namcos1_bankswitch( 1, 0x0e01, data );
}

/*******************************************************************************
*                                                                              *
*   Initialization                                                             *
*                                                                              *
*******************************************************************************/

static void namcos1_install_bank(int start,int end,read8_handler hr,write8_handler hw,
			  int offset,unsigned char *pointer)
{
	int i;
	for(i=start;i<=end;i++)
	{
		namcos1_bank_element[i].bank_handler_r = hr;
		namcos1_bank_element[i].bank_handler_w = hw;
		namcos1_bank_element[i].bank_offset    = offset;
		namcos1_bank_element[i].bank_pointer   = pointer;
		offset  += 0x2000;
		if(pointer) pointer += 0x2000;
	}
}



static void namcos1_build_banks(read8_handler key_r,write8_handler key_w)
{
	int i;

	/**** kludge alert ****/
	UINT8 *dummyrom = auto_malloc(0x2000);

	/* when the games want to reset because the test switch has been flipped (or
       because the protection checks failed!) they just set the top bits of bank #7
       to 0, effectively crashing and waiting for the watchdog to kick in.
       To avoid crashes in MAME, I prepare a dummy ROM containing just BRA -2 so
       the program doesn't start executing code in unmapped areas.
       Conveniently, the opcode for BRA -2 is 20 FE, and FE 20 FE is LDU $20FE,
       so misaligned entry points get immediatly corrected. */
	for (i = 0;i < 0x2000;i+=2)
	{
		dummyrom[i]   = 0x20;
		dummyrom[i+1] = 0xfe;
	}
	/* also provide a valid IRQ vector */
	dummyrom[0x1ff8] = 0xff;
	dummyrom[0x1ff9] = 0x00;

	/* clear all banks to unknown area */
	for(i = 0;i < NAMCOS1_MAX_BANK;i++)
		namcos1_install_bank(i,i,0,unknown_w,0,dummyrom);
	/**** end of kludge alert ****/


	/* 3D glasses */
	namcos1_install_bank(0x160,0x160,0,namcos1_3dcs_w,0,0);
	/* RAM 6 banks - palette */
	namcos1_install_bank(0x170,0x173,0,namcos1_paletteram_w,0,namcos1_paletteram);
	/* RAM 5 banks - videoram */
	namcos1_install_bank(0x178,0x17b,namcos1_videoram_r,namcos1_videoram_w,0,0);
	/* key chip bank */
	namcos1_install_bank(0x17c,0x17c,key_r,key_w,0,0);
	/* RAM 7 banks - display control, playfields, sprites */
	namcos1_install_bank(0x17e,0x17e,namcos1_spriteram_r,namcos1_spriteram_w,0,0);
	/* RAM 1 shared ram, PSG device */
	namcos1_install_bank(0x17f,0x17f,soundram_r,soundram_w,0,0);
	/* RAM 3 banks */
	namcos1_install_bank(0x180,0x183,0,0,0,s1ram);

	/* PRG0-PRG7 */
	{
		UINT8 *rom = memory_region(REGION_USER1);

		namcos1_install_bank(0x200,0x3ff,0,rom_w,0,rom);

		/* bit 16 of the address is inverted for PRG7 (and bits 17,18 just not connected) */
		for (i = 0x380000;i < 0x400000;i++)
		{
			if ((i & 0x010000) == 0)
			{
				UINT8 t = rom[i];
				rom[i] = rom[i + 0x010000];
				rom[i + 0x010000] = t;
			}
		}
	}
}

MACHINE_RESET( namcos1 )
{
	static bankhandler unknown_handler = { unknown_r, unknown_w, 0, NULL };
	int bank;

	/* Point all of our bankhandlers to the error handlers */
	for (bank = 0; bank < 2*8 ; bank++)
		set_bank(bank, &unknown_handler);

	/* Default MMU setup for Cpu 0 */
	namcos1_bankswitch(0, 0x0000, 0x01 ); /* bank0 = 0x180(RAM) - evidence: wldcourt */
	namcos1_bankswitch(0, 0x0001, 0x80 );
	namcos1_bankswitch(0, 0x0200, 0x01 ); /* bank1 = 0x180(RAM) - evidence: berabohm */
	namcos1_bankswitch(0, 0x0201, 0x80 );

	namcos1_bankswitch(0, 0x0e00, 0x03 ); /* bank7 = 0x3ff(PRG7) */
	namcos1_bankswitch(0, 0x0e01, 0xff );

	/* Default MMU setup for Cpu 1 */
	namcos1_bankswitch(1, 0x0000, 0x01 ); /* bank0 = 0x180(RAM) - evidence: wldcourt */
	namcos1_bankswitch(1, 0x0001, 0x80 );

	namcos1_bankswitch(1, 0x0e00, 0x03); /* bank7 = 0x3ff(PRG7) */
	namcos1_bankswitch(1, 0x0e01, 0xff);

	/* stop all CPUs */
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);

	/* mcu patch data clear */
	mcu_patch_data = 0;
	namcos1_reset = 0;
}



/*******************************************************************************
*                                                                              *
*   63701 MCU emulation (CUS64)                                                *
*                                                                              *
*******************************************************************************/

/*******************************************************************************
*                                                                              *
*   MCU banking emulation and patch                                            *
*                                                                              *
*******************************************************************************/

/* mcu banked rom area select */
WRITE8_HANDLER( namcos1_mcu_bankswitch_w )
{
	int addr;

	/* bit 2-7 : chip select line of ROM chip */
	switch (data & 0xfc)
	{
		case 0xf8: addr = 0x10000; data ^= 2; break;	/* bit 2 : ROM 0 (bit 1 is inverted in that case) */
		case 0xf4: addr = 0x30000; break;				/* bit 3 : ROM 1 */
		case 0xec: addr = 0x50000; break;				/* bit 4 : ROM 2 */
		case 0xdc: addr = 0x70000; break;				/* bit 5 : ROM 3 */
		case 0xbc: addr = 0x90000; break;				/* bit 6 : ROM 4 */
		case 0x7c: addr = 0xb0000; break;				/* bit 7 : ROM 5 */
		default:   addr = 0x10000; break;				/* illegal */
	}

	/* bit 0-1 : address line A15-A16 */
	addr += (data & 3) * 0x8000;

	memory_set_bankptr(20, memory_region(REGION_CPU4) + addr);
}



/* This point is very obscure, but i havent found any better way yet. */
/* Works with all games so far.                                       */

/* patch points of memory address */
/* CPU0/1 bank[17f][1000] */
/* CPU2   [7000]      */
/* CPU3   [c000]      */

/* This memory point should be set $A6 by anywhere, but         */
/* I found set $A6 only initialize in MCU                       */
/* This patch kill write this data by MCU case $A6 to xx(clear) */

WRITE8_HANDLER( namcos1_mcu_patch_w )
{
	//logerror("mcu C000 write pc=%04x data=%02x\n",activecpu_get_pc(),data);
	if (mcu_patch_data == 0xa6) return;
	mcu_patch_data = data;
	namcos1_triram[0] = data;
}



/*******************************************************************************
*                                                                              *
*   driver specific initialize routine                                         *
*                                                                              *
*******************************************************************************/
struct namcos1_specific
{
	/* keychip */
	read8_handler key_r;
	write8_handler key_w;
	int key_id;
	int key_reg1;
	int key_reg2;
	int key_reg3;
	int key_reg4;
	int key_reg5;
	int key_reg6;
};

static void namcos1_driver_init(const struct namcos1_specific *specific )
{
	const struct namcos1_specific no_key =
	{
		no_key_r,no_key_w
	};

	if (!specific) specific = &no_key;

	/* keychip id */
	key_id        = specific->key_id;
	/* for key type 3 */
	key_reg       = specific->key_reg1;
	key_rng       = specific->key_reg2;
	key_swap4_arg = specific->key_reg3;
	key_swap4     = specific->key_reg4;
	key_bottom4   = specific->key_reg5;
	key_top4      = specific->key_reg6;

	/* S1 RAM pointer set */
	s1ram = auto_malloc(0x8000);
	namcos1_triram = auto_malloc(0x800);
	namcos1_paletteram = auto_malloc(0x8000);

	/* Register volatile user memory for save state */
	state_save_register_global_pointer(s1ram, 0x8000);
	state_save_register_global_pointer(namcos1_triram, 0x800);
	state_save_register_global_pointer(namcos1_paletteram, 0x8000);

	/* Point mcu & sound shared RAM to destination */
	memory_set_bankptr( 18, namcos1_triram );
	memory_set_bankptr( 19, namcos1_triram );

	/* build bank elements */
	namcos1_build_banks(specific->key_r,specific->key_w);
}


/*******************************************************************************
*   Shadowland / Youkai Douchuuki specific                                     *
*******************************************************************************/
DRIVER_INIT( shadowld )
{
	namcos1_driver_init(NULL);
}

/*******************************************************************************
*   Dragon Spirit specific (CUS136)                                            *
*******************************************************************************/
DRIVER_INIT( dspirit )
{
	const struct namcos1_specific dspirit_specific=
	{
		key_type1_r,key_type1_w, 0x36
	};
	namcos1_driver_init(&dspirit_specific);
}

/*******************************************************************************
*   World Court specific (CUS143)                                              *
*******************************************************************************/
DRIVER_INIT( wldcourt )
{
	const struct namcos1_specific worldcourt_specific=
	{
		key_type1_r,key_type1_w, 0x35
	};
	namcos1_driver_init(&worldcourt_specific);
}

/*******************************************************************************
*   Blazer specific (CUS144)                                                   *
*******************************************************************************/
DRIVER_INIT( blazer )
{
	const struct namcos1_specific blazer_specific=
	{
		key_type1_r,key_type1_w, 0x13
	};
	namcos1_driver_init(&blazer_specific);
}

/*******************************************************************************
*   Puzzle Club specific                                                       *
*******************************************************************************/
DRIVER_INIT( puzlclub )
{
	const struct namcos1_specific puzlclub_specific=
	{
		key_type1_r,key_type1_w, 0x35
	};
	namcos1_driver_init(&puzlclub_specific);
}

/*******************************************************************************
*   Pac-Mania specific (CUS151)                                                *
*******************************************************************************/
DRIVER_INIT( pacmania )
{
	const struct namcos1_specific pacmania_specific=
	{
		key_type2_r,key_type2_w, 0x12
	};
	namcos1_driver_init(&pacmania_specific);
}

/*******************************************************************************
*   Alice in Wonderland / Marchen Maze specific (CUS152)                       *
*******************************************************************************/
DRIVER_INIT( alice )
{
	const struct namcos1_specific alice_specific=
	{
		key_type2_r,key_type2_w, 0x25
	};
	namcos1_driver_init(&alice_specific);
}

/*******************************************************************************
*   Galaga '88 specific (CUS153)                                               *
*******************************************************************************/
DRIVER_INIT( galaga88 )
{
	const struct namcos1_specific galaga88_specific=
	{
		key_type2_r,key_type2_w, 0x31
	};
	namcos1_driver_init(&galaga88_specific);
}

/*******************************************************************************
*   World Stadium specific (CUS154)                                            *
*******************************************************************************/
DRIVER_INIT( ws )
{
	const struct namcos1_specific ws_specific=
	{
		key_type2_r,key_type2_w, 0x07
	};
	namcos1_driver_init(&ws_specific);
}

/*******************************************************************************
*   Bakutotsu Kijuutei specific (CUS155)                                       *
*******************************************************************************/
DRIVER_INIT( bakutotu )
{
	const struct namcos1_specific bakutotu_specific=
	{
		key_type2_r,key_type2_w, 0x22
	};
	namcos1_driver_init(&bakutotu_specific);

#if 0
	// resolves CPU deadlocks caused by sloppy coding(see driver\namcos1.c)
	{
		UINT8 target[8] = {0x34,0x37,0x35,0x37,0x96,0x00,0x2e,0xed};
		UINT8 *rombase, *srcptr, *endptr, *scanptr;

		rombase = memory_region(REGION_USER1);
		srcptr = rombase + 0x1e000;
		endptr = srcptr + 0xa000;

		while ( (scanptr = memchr(srcptr, 0x34, endptr-srcptr)) )
		{
			if (!memcmp(scanptr, target, 8))
			{
				scanptr[7] = 0xfc;
				srcptr = scanptr + 8;

				logerror ("faulty loop patched at %06x\n", scanptr-rombase+7);
			}
			else
				srcptr = scanptr + 1;
		}
	}
#endif
}

/*******************************************************************************
*   Splatter House specific (CUS181)                                           *
*******************************************************************************/
DRIVER_INIT( splatter )
{
	const struct namcos1_specific splatter_specific=
	{
		key_type3_r,key_type3_w, 181, 3, 4,-1,-1,-1,-1
	};

	namcos1_driver_init(&splatter_specific);
}

/*******************************************************************************
*   Rompers specific (CUS182)                                                  *
*******************************************************************************/
DRIVER_INIT( rompers )
{
	const struct namcos1_specific rompers_specific=
	{
		key_type3_r,key_type3_w, 182, 7,-1,-1,-1,-1,-1
	};
	namcos1_driver_init(&rompers_specific);
}

/*******************************************************************************
*   Blast Off specific (CUS183)                                                *
*******************************************************************************/
DRIVER_INIT( blastoff )
{
	const struct namcos1_specific blastoff_specific=
	{
		key_type3_r,key_type3_w, 183, 0, 7, 3, 5,-1,-1
	};
	namcos1_driver_init(&blastoff_specific);
}

/*******************************************************************************
*   World Stadium '89 specific (CUS184)                                        *
*******************************************************************************/
DRIVER_INIT( ws89 )
{
	const struct namcos1_specific ws89_specific=
	{
		key_type3_r,key_type3_w, 184, 2,-1,-1,-1,-1,-1
	};
	namcos1_driver_init(&ws89_specific);
}

/*******************************************************************************
*   Tank Force specific (CUS185)                                               *
*******************************************************************************/
DRIVER_INIT( tankfrce )
{
	const struct namcos1_specific tankfrce_specific=
	{
		key_type3_r,key_type3_w, 185, 5,-1, 1,-1, 2,-1
	};
	namcos1_driver_init(&tankfrce_specific);
}

/*******************************************************************************
*   Dangerous Seed specific (CUS308)                                           *
*******************************************************************************/
DRIVER_INIT( dangseed )
{
	const struct namcos1_specific dangseed_specific=
	{
		key_type3_r,key_type3_w, 308, 6,-1, 5,-1, 0, 4
	};
	namcos1_driver_init(&dangseed_specific);
}

/*******************************************************************************
*   Pistol Daimyo no Bouken specific (CUS309)                                  *
*******************************************************************************/
DRIVER_INIT( pistoldm )
{
	const struct namcos1_specific pistoldm_specific=
	{
		key_type3_r,key_type3_w, 309, 1, 2, 0,-1, 4,-1
	};
	namcos1_driver_init(&pistoldm_specific);
}

/*******************************************************************************
*   World Stadium '90 specific (CUS310)                                        *
*******************************************************************************/
DRIVER_INIT( ws90 )
{
	const struct namcos1_specific ws90_specific=
	{
		key_type3_r,key_type3_w, 310, 4,-1, 7,-1, 3,-1
	};
	namcos1_driver_init(&ws90_specific);
}

/*******************************************************************************
*   Souko Ban DX specific (CUS311)                                             *
*******************************************************************************/
DRIVER_INIT( soukobdx )
{
	const struct namcos1_specific soukobdx_specific=
	{
		key_type3_r,key_type3_w, 311, 2, 3/*?*/, 0,-1, 4,-1
	};
	namcos1_driver_init(&soukobdx_specific);
}



/*******************************************************************************
*   Quester specific                                                           *
*******************************************************************************/
static READ8_HANDLER( quester_paddle_r )
{
	static int qnum=0, qstrobe=0;

	if (offset == 0)
	{
		int ret;

		if (!qnum)
			ret = (readinputportbytag("CONTROL0")&0x90) | qstrobe | (readinputportbytag("PADDLE0")&0x0f);
		else
			ret = (readinputportbytag("CONTROL0")&0x90) | qstrobe | (readinputportbytag("PADDLE1")&0x0f);

		qstrobe ^= 0x40;

		return ret;
	}
	else
	{
		int ret;

		if (!qnum)
			ret = (readinputportbytag("CONTROL1")&0x90) | qnum | (readinputportbytag("PADDLE0")>>4);
		else
			ret = (readinputportbytag("CONTROL1")&0x90) | qnum | (readinputportbytag("PADDLE1")>>4);

		if (!qstrobe) qnum ^= 0x20;

		return ret;
	}
}

DRIVER_INIT( quester )
{
	namcos1_driver_init(NULL);
	memory_install_read8_handler(3, ADDRESS_SPACE_PROGRAM, 0x1400, 0x1401, 0, 0, quester_paddle_r);
}



/*******************************************************************************
*   Beraboh Man specific                                                       *
*******************************************************************************/

static READ8_HANDLER( berabohm_buttons_r )
{
	int res;
	static int input_count, strobe, strobe_count;


	if (offset == 0)
	{
		int inp = input_count;

		if (inp == 4) res = readinputportbytag("CONTROL0");
		else
		{
			char portname[4];

#ifdef PRESSURE_SENSITIVE
			static int counter[4];

			sprintf(portname,"IN%d",inp);	/* IN0-IN3 */
			res = readinputportbytag(portname);
			if (res & 0x80)
			{
				if (counter[inp] >= 0)
					res = 0x40 | counter[inp];
				else
				{
					if (res & 0x40) res = 0x40;
					else res = 0x00;
				}
			}
			else if (res & 0x40)
			{
				if (counter[inp] < 0x3f)
				{
					counter[inp] += 4;
					res = 0x00;
				}
				else res = 0x7f;
			}
			else
				counter[inp] = -1;
#else
			sprintf(portname,"IN%d",inp);	/* IN0-IN3 */
			res = readinputportbytag(portname);
			if (res & 1) res = 0x7f;		/* weak */
			else if (res & 2) res = 0x48;	/* medium */
			else if (res & 4) res = 0x40;	/* strong */
#endif
		}

		return res;
	}
	else
	{
		res = readinputportbytag("CONTROL1") & 0x8f;

		/* the strobe cannot happen too often, otherwise the MCU will waste too
           much time reading the inputs and won't have enough cycles to play two
           digital sounds at once. This value is enough to read all inputs at least
           once per frame */
		if (++strobe_count > 4)
		{
			strobe_count = 0;
			strobe ^= 0x40;
			if (strobe == 0)
			{
				input_count = (input_count + 1) % 5;
				if (input_count == 3) res |= 0x10;
			}
		}

		res |= strobe;

		return res;
	}
}

DRIVER_INIT( berabohm )
{
	namcos1_driver_init(NULL);
	memory_install_read8_handler(3, ADDRESS_SPACE_PROGRAM, 0x1400, 0x1401, 0, 0, berabohm_buttons_r);
}



/*******************************************************************************
*   Face Off specific                                                          *
*******************************************************************************/

static READ8_HANDLER( faceoff_inputs_r )
{
	int res;
	static int input_count, strobe_count, stored_input[2];

	if (offset == 0)
	{
		res = (readinputportbytag("CONTROL0") & 0x80) | stored_input[0];

		return res;
	}
	else
	{
		res = readinputportbytag("CONTROL1") & 0x80;

		/* the strobe cannot happen too often, otherwise the MCU will waste too
           much time reading the inputs and won't have enough cycles to play two
           digital sounds at once. This value is enough to read all inputs at least
           once per frame */
		if (++strobe_count > 8)
		{
			strobe_count = 0;

			res |= input_count;

			switch (input_count)
			{
				case 0:
					stored_input[0] = readinputportbytag("IN0") & 0x1f;
					stored_input[1] = (readinputportbytag("IN3") & 0x07) << 3;

				case 3:
					stored_input[0] = readinputportbytag("IN2") & 0x1f;

				case 4:
					stored_input[0] = readinputportbytag("IN1") & 0x1f;
					stored_input[1] = readinputportbytag("IN3") & 0x18;
			}

			input_count = (input_count + 1) & 7;
		}
		else
		{
			res |= 0x40 | stored_input[1];
		}

		return res;
	}
}

DRIVER_INIT( faceoff )
{
	namcos1_driver_init(NULL);
	memory_install_read8_handler(3, ADDRESS_SPACE_PROGRAM, 0x1400, 0x1401, 0, 0, faceoff_inputs_r);
}
