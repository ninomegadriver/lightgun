/***************************************************************************

Super Real Mahjong P2
-------------------------------------
driver by Yochizo and Takahiro Nogi

  Yochizo took charge of video and I/O part.
  Takahiro Nogi took charge of sound, I/O and NVRAM part.

  ... and this is based on "seta.c" driver written by Luca Elia.

  Thanks for your reference, Takahiro Nogi and Luca Elia.


Supported games :
==================
 Super Real Mahjong Part2     (C) 1987 Seta
 Super Real Mahjong Part3     (C) 1988 Seta
 Mahjong Yuugi (set 1)        (C) 1990 Visco
 Mahjong Yuugi (set 2)        (C) 1990 Visco
 Mahjong Pon Chin Kan (set 1) (C) 1991 Visco
 Mahjong Pon Chin Kan (set 2) (C) 1991 Visco


Not supported game :
=====================
 Super Real Mahjong Part1 (not dumped)


System specs :
===============
   CPU       : 68000 (8MHz)
   Sound     : AY8910 + MSM5205
   Chips     : X1-001, X1-002A, X1-003, X1-004x2, X0-005 x2
           X1-001, X1-002A  : Sprites
           X1-003           : Video output
           X1-004           : ???
           X1-005           : ???


Known issues :
===============
 - I/O port isn't fully analized. Currently avoid I/O error message with hack.
 - AY-3-8910 sound may be wrong.
 - CPU clock of srmp3 does not match the real machine.
 - MSM5205 clock frequency in srmp3 is wrong.


Note:
======
 - In mjyuugi and mjyuugia, DSW3 (Debug switch) is available if you
   turn on the cheat switch.


****************************************************************************/


#include "driver.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"


/***************************************************************************

  Variables

***************************************************************************/

PALETTE_INIT( srmp2 );
VIDEO_UPDATE( srmp2 );
PALETTE_INIT( srmp3 );
VIDEO_UPDATE( srmp3 );
VIDEO_UPDATE( mjyuugi );


extern int srmp2_color_bank;
extern int srmp3_gfx_bank;
extern int mjyuugi_gfx_bank;

static int srmp2_adpcm_bank;
static int srmp2_adpcm_data;
static unsigned long srmp2_adpcm_sptr;
static unsigned long srmp2_adpcm_eptr;

static int srmp2_port_select;


/***************************************************************************

  Interrupt(s)

***************************************************************************/

static INTERRUPT_GEN( srmp2_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:		cpunum_set_input_line(0, 4, HOLD_LINE);	break;	/* vblank */
		default:	cpunum_set_input_line(0, 2, HOLD_LINE);	break;	/* sound */
	}
}


static DRIVER_INIT( srmp2 )
{
	UINT16 *RAM = (UINT16 *) memory_region(REGION_CPU1);

	/* Fix "ERROR BACK UP" and "ERROR IOX" */
	RAM[0x20c80 / 2] = 0x4e75;								// RTS
}

static DRIVER_INIT( srmp3 )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* BANK ROM (0x08000 - 0x1ffff) Check skip [MAIN ROM side] */
	RAM[0x00000 + 0x7b69] = 0x00;							// NOP
	RAM[0x00000 + 0x7b6a] = 0x00;							// NOP

	/* MAIN ROM (0x00000 - 0x07fff) Check skip [BANK ROM side] */
	RAM[0x08000 + 0xc10b] = 0x00;							// NOP
	RAM[0x08000 + 0xc10c] = 0x00;							// NOP
	RAM[0x08000 + 0xc10d] = 0x00;							// NOP
	RAM[0x08000 + 0xc10e] = 0x00;							// NOP
	RAM[0x08000 + 0xc10f] = 0x00;							// NOP
	RAM[0x08000 + 0xc110] = 0x00;							// NOP
	RAM[0x08000 + 0xc111] = 0x00;							// NOP

	/* "ERR IOX" Check skip [MAIN ROM side] */
	RAM[0x00000 + 0x784e] = 0x00;							// NOP
	RAM[0x00000 + 0x784f] = 0x00;							// NOP
	RAM[0x00000 + 0x7850] = 0x00;							// NOP
}

static MACHINE_RESET( srmp2 )
{
	srmp2_port_select = 0;
}

static MACHINE_RESET( srmp3 )
{
	srmp2_port_select = 0;
}


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static WRITE16_HANDLER( srmp2_flags_w )
{
/*
    ---- ---x : Coin Counter
    ---x ---- : Coin Lock Out
    --x- ---- : ADPCM Bank
    x--- ---- : Palette Bank
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
	srmp2_adpcm_bank = ( (data & 0x20) >> 5 );
	srmp2_color_bank = ( (data & 0x80) >> 7 );
}


static WRITE16_HANDLER( mjyuugi_flags_w )
{
/*
    ---- ---x : Coin Counter
    ---x ---- : Coin Lock Out
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
}


static WRITE16_HANDLER( mjyuugi_adpcm_bank_w )
{
/*
    ---- xxxx : ADPCM Bank
    --xx ---- : GFX Bank
*/
	srmp2_adpcm_bank = (data & 0x0f);
	mjyuugi_gfx_bank = ((data >> 4) & 0x03);
}


static WRITE16_HANDLER( srmp2_adpcm_code_w )
{
/*
    - Received data may be playing ADPCM number.
    - 0x000000 - 0x0000ff and 0x010000 - 0x0100ff are offset table.
    - When the hardware receives the ADPCM number, it refers the offset
      table and plays the ADPCM for itself.
*/

	unsigned char *ROM = memory_region(REGION_SOUND1);

	srmp2_adpcm_sptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 0)] << 8);
	srmp2_adpcm_eptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 1)] << 8);
	srmp2_adpcm_eptr  = (srmp2_adpcm_eptr - 1) & 0x0ffff;

	srmp2_adpcm_sptr += (srmp2_adpcm_bank * 0x10000);
	srmp2_adpcm_eptr += (srmp2_adpcm_bank * 0x10000);

	MSM5205_reset_w(0, 0);
	srmp2_adpcm_data = -1;
}


static WRITE8_HANDLER( srmp3_adpcm_code_w )
{
/*
    - Received data may be playing ADPCM number.
    - 0x000000 - 0x0000ff and 0x010000 - 0x0100ff are offset table.
    - When the hardware receives the ADPCM number, it refers the offset
      table and plays the ADPCM for itself.
*/

	unsigned char *ROM = memory_region(REGION_SOUND1);

	srmp2_adpcm_sptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 0)] << 8);
	srmp2_adpcm_eptr = (ROM[((srmp2_adpcm_bank * 0x10000) + (data << 2) + 1)] << 8);
	srmp2_adpcm_eptr  = (srmp2_adpcm_eptr - 1) & 0x0ffff;

	srmp2_adpcm_sptr += (srmp2_adpcm_bank * 0x10000);
	srmp2_adpcm_eptr += (srmp2_adpcm_bank * 0x10000);

	MSM5205_reset_w(0, 0);
	srmp2_adpcm_data = -1;
}


static void srmp2_adpcm_int(int num)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	if (srmp2_adpcm_sptr)
	{
		if (srmp2_adpcm_data == -1)
		{
			srmp2_adpcm_data = ROM[srmp2_adpcm_sptr];

			if (srmp2_adpcm_sptr >= srmp2_adpcm_eptr)
			{
				MSM5205_reset_w(0, 1);
				srmp2_adpcm_data = 0;
				srmp2_adpcm_sptr = 0;
			}
			else
			{
				MSM5205_data_w(0, ((srmp2_adpcm_data >> 4) & 0x0f));
			}
		}
		else
		{
			MSM5205_data_w(0, ((srmp2_adpcm_data >> 0) & 0x0f));
			srmp2_adpcm_sptr++;
			srmp2_adpcm_data = -1;
		}
	}
	else
	{
		MSM5205_reset_w(0, 1);
	}
}


static READ16_HANDLER( srmp2_cchip_status_0_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_cchip_status_1_r )
{
	return 0x01;
}


static READ16_HANDLER( srmp2_input_1_r )
{
/*
    ---x xxxx : Key code
    --x- ---- : Player 1 and 2 side flag
*/

	if (!ACCESSING_LSB)
	{
		return 0xffff;
	}

	if (srmp2_port_select != 2)			/* Panel keys */
	{
		int i, j, t;

		for (i = 0x00 ; i < 0x20 ; i += 8)
		{
			j = (i / 0x08) + 3;

			for (t = 0 ; t < 8 ; t ++)
			{
				if (!(readinputport(j) & ( 1 << t )))
				{
					return (i + t);
				}
			}
		}
	}
	else								/* Analizer and memory reset keys */
	{
		return readinputportbytag("IN7");
	}

	return 0xffff;
}


static READ16_HANDLER( srmp2_input_2_r )
{
	if (!ACCESSING_LSB)
	{
		return 0x0001;
	}

	/* Always return 1, otherwise freeze. Maybe read I/O status */
	return 0x0001;
}


static WRITE16_HANDLER( srmp2_input_1_w )
{
	if (data != 0x0000)
	{
		srmp2_port_select = 1;
	}
	else
	{
		srmp2_port_select = 0;
	}
}


static WRITE16_HANDLER( srmp2_input_2_w )
{
	if (data == 0x0000)
	{
		srmp2_port_select = 2;
	}
	else
	{
		srmp2_port_select = 0;
	}
}


static WRITE8_HANDLER( srmp3_rombank_w )
{
/*
    ---x xxxx : MAIN ROM bank
    xxx- ---- : ADPCM ROM bank
*/

	unsigned char *ROM = memory_region(REGION_CPU1);
	int addr;

	srmp2_adpcm_bank = ((data & 0xe0) >> 5);

	if (data & 0x1f) addr = ((0x10000 + (0x2000 * (data & 0x0f))) - 0x8000);
	else addr = 0x10000;

	memory_set_bankptr(1, &ROM[addr]);
}

/**************************************************************************

  Memory Map(s)

**************************************************************************/


static ADDRESS_MAP_START( srmp2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x143fff) AM_READ(MRA16_RAM)				/* Sprites Code + X + Attr */
	AM_RANGE(0x180000, 0x180607) AM_READ(MRA16_RAM)				/* Sprites Y */
	AM_RANGE(0x900000, 0x900001) AM_READ(input_port_0_word_r)	/* Coinage */
	AM_RANGE(0xa00000, 0xa00001) AM_READ(srmp2_input_1_r)		/* I/O port 1 */
	AM_RANGE(0xa00002, 0xa00003) AM_READ(srmp2_input_2_r)		/* I/O port 2 */
	AM_RANGE(0xb00000, 0xb00001) AM_READ(srmp2_cchip_status_0_r)	/* Custom chip status ??? */
	AM_RANGE(0xb00002, 0xb00003) AM_READ(srmp2_cchip_status_1_r)	/* Custom chip status ??? */
	AM_RANGE(0xf00000, 0xf00001) AM_READ(AY8910_read_port_0_lsb_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srmp2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x140000, 0x143fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2)	/* Sprites Code + X + Attr */
	AM_RANGE(0x180000, 0x180609) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)	/* Sprites Y */
	AM_RANGE(0x1c0000, 0x1c0001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0x800000, 0x800001) AM_WRITE(srmp2_flags_w)				/* ADPCM bank, Color bank, etc. */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(srmp2_input_1_w)			/* I/O ??? */
	AM_RANGE(0xa00002, 0xa00003) AM_WRITE(srmp2_input_2_w)			/* I/O ??? */
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(srmp2_adpcm_code_w)			/* ADPCM number */
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0xd00000, 0xd00001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0xe00000, 0xe00001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0xf00000, 0xf00001) AM_WRITE(AY8910_control_port_0_lsb_w)
	AM_RANGE(0xf00002, 0xf00003) AM_WRITE(AY8910_write_port_0_lsb_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mjyuugi_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_READ(input_port_0_word_r)	/* Coinage */
	AM_RANGE(0x100010, 0x100011) AM_READ(MRA16_NOP)				/* ??? */
	AM_RANGE(0x200000, 0x200001) AM_READ(MRA16_NOP)				/* ??? */
	AM_RANGE(0x300000, 0x300001) AM_READ(MRA16_NOP)				/* ??? */
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_8_word_r)	/* DSW 3-1 */
	AM_RANGE(0x500010, 0x500011) AM_READ(input_port_9_word_r)	/* DSW 3-2 */
	AM_RANGE(0x700000, 0x7003ff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x800000, 0x800001) AM_READ(MRA16_NOP)				/* ??? */
	AM_RANGE(0x900000, 0x900001) AM_READ(srmp2_input_1_r)		/* I/O port 1 */
	AM_RANGE(0x900002, 0x900003) AM_READ(srmp2_input_2_r)		/* I/O port 2 */
	AM_RANGE(0xa00000, 0xa00001) AM_READ(srmp2_cchip_status_0_r)	/* custom chip status ??? */
	AM_RANGE(0xa00002, 0xa00003) AM_READ(srmp2_cchip_status_1_r)	/* custom chip status ??? */
	AM_RANGE(0xb00000, 0xb00001) AM_READ(AY8910_read_port_0_lsb_r)
	AM_RANGE(0xd00000, 0xd00609) AM_READ(MRA16_RAM)				/* Sprites Y */
	AM_RANGE(0xd02000, 0xd023ff) AM_READ(MRA16_RAM)				/* ??? */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)				/* Sprites Code + X + Attr */
	AM_RANGE(0xffc000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mjyuugi_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_WRITE(mjyuugi_flags_w)			/* Coin Counter */
	AM_RANGE(0x100010, 0x100011) AM_WRITE(mjyuugi_adpcm_bank_w)		/* ADPCM bank, GFX bank */
	AM_RANGE(0x700000, 0x7003ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x900000, 0x900001) AM_WRITE(srmp2_input_1_w)			/* I/O ??? */
	AM_RANGE(0x900002, 0x900003) AM_WRITE(srmp2_input_2_w)			/* I/O ??? */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(srmp2_adpcm_code_w)			/* ADPCM number */
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(AY8910_control_port_0_lsb_w)
	AM_RANGE(0xb00002, 0xb00003) AM_WRITE(AY8910_write_port_0_lsb_w)
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(MWA16_NOP)					/* ??? */
	AM_RANGE(0xd00000, 0xd00609) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)	/* Sprites Y */
	AM_RANGE(0xd02000, 0xd023ff) AM_WRITE(MWA16_RAM)					/* ??? only writes $00fa */
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2)	/* Sprites Code + X + Attr */
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END


static READ8_HANDLER( srmp3_cchip_status_0_r )
{
	return 0x01;
}

static READ8_HANDLER( srmp3_cchip_status_1_r )
{
	return 0x01;
}

static WRITE8_HANDLER( srmp3_input_1_w )
{
/*
    ---- --x- : Player 1 side flag ?
    ---- -x-- : Player 2 side flag ?
*/

	logerror("PC:%04X DATA:%02X  srmp3_input_1_w\n", activecpu_get_pc(), data);

	srmp2_port_select = 0;

	{
		static int qqq01 = 0;
		static int qqq02 = 0;
		static int qqq49 = 0;
		static int qqqzz = 0;

		if (data == 0x01) qqq01++;
		else if (data == 0x02) qqq02++;
		else if (data == 0x49) qqq49++;
		else qqqzz++;

//      ui_popup("%04X %04X %04X %04X", qqq01, qqq02, qqq49, qqqzz);
	}
}

static WRITE8_HANDLER( srmp3_input_2_w )
{

	/* Key matrix reading related ? */

	logerror("PC:%04X DATA:%02X  srmp3_input_2_w\n", activecpu_get_pc(), data);

	srmp2_port_select = 1;

}

static READ8_HANDLER( srmp3_input_r )
{
/*
    ---x xxxx : Key code
    --x- ---- : Player 1 and 2 side flag
*/

	/* Currently I/O port of srmp3 is fully understood. */

	int keydata = 0xff;

	logerror("PC:%04X          srmp3_input_r\n", activecpu_get_pc());

	// PC:0x8903    ROM:0xC903
	// PC:0x7805    ROM:0x7805

	if ((activecpu_get_pc() == 0x8903) || (activecpu_get_pc() == 0x7805))	/* Panel keys */
	{
		int i, j, t;

		for (i = 0x00 ; i < 0x20 ; i += 8)
		{
			j = (i / 0x08) + 3;

			for (t = 0 ; t < 8 ; t ++)
			{
				if (!(readinputport(j) & ( 1 << t )))
				{
					keydata = (i + t);
				}
			}
		}
	}

	// PC:0x8926    ROM:0xC926
	// PC:0x7822    ROM:0x7822

	if ((activecpu_get_pc() == 0x8926) || (activecpu_get_pc() == 0x7822))	/* Analizer and memory reset keys */
	{
		keydata = readinputport(7);
	}

	return keydata;
}

static WRITE8_HANDLER( srmp3_flags_w )
{
/*
    ---- ---x : Coin Counter
    ---x ---- : Coin Lock Out
    xx-- ---- : GFX Bank
*/

	coin_counter_w( 0, ((data & 0x01) >> 0) );
	coin_lockout_w( 0, (((~data) & 0x10) >> 4) );
	srmp3_gfx_bank = (data >> 6) & 0x03;
}


static ADDRESS_MAP_START( srmp3_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)						/* rom bank */
	AM_RANGE(0xa000, 0xa7ff) AM_READ(MRA8_RAM)						/* work ram */
	AM_RANGE(0xb000, 0xb303) AM_READ(MRA8_RAM)						/* Sprites Y */
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)						/* Sprites Code + X + Attr */
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srmp3_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_ROM)						/* rom bank */
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)	/* work ram */
	AM_RANGE(0xa800, 0xa800) AM_WRITE(MWA8_NOP)						/* flag ? */
	AM_RANGE(0xb000, 0xb303) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)			/* Sprites Y */
	AM_RANGE(0xb800, 0xb800) AM_WRITE(MWA8_NOP)						/* flag ? */
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)			/* Sprites Code + X + Attr */
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srmp3_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_READ(input_port_0_r)						/* coin, service */
	AM_RANGE(0xa1, 0xa1) AM_READ(srmp3_cchip_status_0_r)				/* custom chip status ??? */
	AM_RANGE(0xc0, 0xc0) AM_READ(srmp3_input_r)						/* key matrix */
	AM_RANGE(0xc1, 0xc1) AM_READ(srmp3_cchip_status_1_r)				/* custom chip status ??? */
	AM_RANGE(0xe2, 0xe2) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( srmp3_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x20, 0x20) AM_WRITE(MWA8_NOP)							/* elapsed interrupt signal */
	AM_RANGE(0x40, 0x40) AM_WRITE(srmp3_flags_w)						/* GFX bank, counter, lockout */
	AM_RANGE(0x60, 0x60) AM_WRITE(srmp3_rombank_w)					/* ROM bank select */
	AM_RANGE(0xa0, 0xa0) AM_WRITE(srmp3_adpcm_code_w)					/* ADPCM number */
	AM_RANGE(0xc0, 0xc0) AM_WRITE(srmp3_input_1_w)					/* I/O ??? */
	AM_RANGE(0xc1, 0xc1) AM_WRITE(srmp3_input_2_w)					/* I/O ??? */
	AM_RANGE(0xe0, 0xe0) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xe1, 0xe1) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

#define SETAMJCTRL_PORT3 \
	PORT_START_TAG("KMI3")	/* KEY MATRIX INPUT (3) */ \
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL ) \
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP ) \
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_BIG ) \
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE ) \
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) \
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT4 \
	PORT_START_TAG("KMI4")	/* KEY MATRIX INPUT (4) */ \
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_K ) \
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) \
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_G ) \
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) \
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_C ) \
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_L ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT5 \
	PORT_START_TAG("KMI5")	/* KEY MATRIX INPUT (5) */ \
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_H ) \
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) \
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_D ) \
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_I ) \
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) \
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_E ) \
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_MAHJONG_M ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SETAMJCTRL_PORT6 \
	PORT_START_TAG("KMI6")	/* KEY MATRIX INPUT (6) */ \
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_A ) \
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_BET ) \
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_J ) \
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) \
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_F ) \
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_N ) \
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_B ) \
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( srmp2 )
	PORT_START_TAG("IN0")			/* Coinage (0) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (Easy)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START_TAG("IN7")
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( srmp3 )
	PORT_START_TAG("IN0")			/* Coinage (0) */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Debug Mode (Cheat)")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Open Reach of CPU" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xe0, "1 (Easy)" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START_TAG("IN7")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( mjyuugi )
	PORT_START_TAG("IN0")			/* Coinage (0) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easy)" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Gal Score" )
	PORT_DIPSETTING(    0x10, "+0" )
	PORT_DIPSETTING(    0x00, "+1000" )
	PORT_DIPNAME( 0x20, 0x20, "Player Score" )
	PORT_DIPSETTING(    0x20, "+0" )
	PORT_DIPSETTING(    0x00, "+1000" )
	PORT_DIPNAME( 0x40, 0x40, "Item price initialize ?" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START_TAG("IN7")
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_DIPNAME( 0x0004, 0x0004, "Debug Mode (Cheat)")
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW3-1")			/* [Debug switch] */
	PORT_DIPNAME( 0x0001, 0x0001, "Debug  0 (Cheat)")
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Debug  1 (Cheat)")
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Debug  2 (Cheat)")
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Debug  3 (Cheat)")
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW3-2")			/* [Debug switch] */
	PORT_DIPNAME( 0x0001, 0x0001, "Debug  4 (Cheat)")
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Debug  5 (Cheat)")
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Debug  6 (Cheat)")
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Debug  7 (Cheat)")
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ponchin )
	PORT_START_TAG("IN0")			/* Coinage (0) */
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "1 (Easy)" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x07, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x30, "Player Score" )
	PORT_DIPSETTING(    0x30, "1000" )
	PORT_DIPSETTING(    0x20, "2000" )
//  PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPNAME( 0x40, 0x40, "Debug Mode (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Auto Tsumo" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SETAMJCTRL_PORT3	/* INPUT1 (3) */
	SETAMJCTRL_PORT4	/* INPUT1 (4) */
	SETAMJCTRL_PORT5	/* INPUT1 (5) */
	SETAMJCTRL_PORT6	/* INPUT1 (6) */

	PORT_START_TAG("IN7")
	PORT_BIT ( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW3-1")			/* [Debug switch] */
	PORT_DIPNAME( 0x0001, 0x0001, "Debug  0 (Cheat)")
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Debug  1 (Cheat)")
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Debug  2 (Cheat)")
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Debug  3 (Cheat)")
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW3-2")			/* [Debug switch] */
	PORT_DIPNAME( 0x0001, 0x0001, "Debug  4 (Cheat)")
	PORT_DIPSETTING(   0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Debug  5 (Cheat)")
	PORT_DIPSETTING(   0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Debug  6 (Cheat)")
	PORT_DIPSETTING(   0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Debug  7 (Cheat)")
	PORT_DIPSETTING(   0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0000, DEF_STR( On ) )
	PORT_BIT ( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct AY8910interface srmp2_ay8910_interface =
{
	input_port_2_r,				/* Input A: DSW 2 */
	input_port_1_r,				/* Input B: DSW 1 */
};


static struct MSM5205interface msm5205_interface =
{
	srmp2_adpcm_int,			/* IRQ handler */
	MSM5205_S48_4B				/* 8 KHz, 4 Bits  */
};


static const gfx_layout charlayout =
{
	16, 16,
	RGN_FRAC(1, 2),
	4,
	{ RGN_FRAC(1, 2) + 8, RGN_FRAC(1, 2) + 0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	16*16*2
};

static const gfx_decode srmp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 64 },
	{ -1 }
};

static const gfx_decode srmp3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 32 },
	{ -1 }
};


static MACHINE_DRIVER_START( srmp2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,16000000/2)				/* 8.00 MHz */
	MDRV_CPU_PROGRAM_MAP(srmp2_readmem,srmp2_writemem)
	MDRV_CPU_VBLANK_INT(srmp2_interrupt,16)		/* Interrupt times is not understood */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(srmp2)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(464, 256-16)
	MDRV_VISIBLE_AREA(16, 464-1, 8, 256-1-24)
	MDRV_GFXDECODE(srmp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_COLORTABLE_LENGTH(1024)	/* sprites only */

	MDRV_PALETTE_INIT(srmp2)
	MDRV_VIDEO_UPDATE(srmp2)		/* just draw the sprites */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 20000000/16)
	MDRV_SOUND_CONFIG(srmp2_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( srmp3 )

	/* basic machine hardware */

	MDRV_CPU_ADD(Z80, 3500000)		/* 3.50 MHz ? */
	//      4000000,                /* 4.00 MHz ? */
	MDRV_CPU_PROGRAM_MAP(srmp3_readmem,srmp3_writemem)
	MDRV_CPU_IO_MAP(srmp3_readport,srmp3_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(srmp3)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256-16)
	MDRV_VISIBLE_AREA(16, 400-1, 8, 256-1-24)
	MDRV_GFXDECODE(srmp3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)	/* sprites only */

	MDRV_PALETTE_INIT(srmp3)
	MDRV_VIDEO_UPDATE(srmp3)	/* just draw the sprites */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 16000000/16)
	MDRV_SOUND_CONFIG(srmp2_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.45)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mjyuugi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,16000000/2)				/* 8.00 MHz */
	MDRV_CPU_PROGRAM_MAP(mjyuugi_readmem,mjyuugi_writemem)
	MDRV_CPU_VBLANK_INT(srmp2_interrupt,16)		/* Interrupt times is not understood */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(srmp2)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256-16)
	MDRV_VISIBLE_AREA(16, 400-1, 0, 256-1-16)
	MDRV_GFXDECODE(srmp3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)			/* sprites only */

	MDRV_VIDEO_UPDATE(mjyuugi)			/* just draw the sprites */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 16000000/16)
	MDRV_SOUND_CONFIG(srmp2_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.45)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( srmp2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "uco-2.17", 0x000000, 0x020000, CRC(0d6c131f) SHA1(be85f2578b0ae2a072565605b7dbeb970e5e3851) )
	ROM_LOAD16_BYTE( "uco-3.18", 0x000001, 0x020000, CRC(e9fdf5f8) SHA1(aa1f8cc3f1d0ed942403c0473605775bc1537cbf) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD       ( "ubo-4.60",  0x000000, 0x040000, CRC(cb6f7cce) SHA1(27d7c2f4f998023081fac1bbacfd4b0b56edaee2) )
	ROM_LOAD       ( "ubo-5.61",  0x040000, 0x040000, CRC(7b48c540) SHA1(06229caec6846581f95409204ea22f0f76684b08) )
	ROM_LOAD16_BYTE( "uco-8.64",  0x080000, 0x040000, CRC(1ca1c7c9) SHA1(05bcca1f88d976d836944a7f5cc74a857fdf6cb9) )
	ROM_LOAD16_BYTE( "uco-9.65",  0x080001, 0x040000, CRC(ef75471b) SHA1(b9843559d36e071bc7d8d81eef44424c4566a10e) )
	ROM_LOAD       ( "ubo-6.62",  0x100000, 0x040000, CRC(6c891ac5) SHA1(eab595bce16e4cdc465a5e2e029c3949a0f28629) )
	ROM_LOAD       ( "ubo-7.63",  0x140000, 0x040000, CRC(60a45755) SHA1(22bbf024bbe2186b621389a23697e55d512b501a) )
	ROM_LOAD16_BYTE( "uco-10.66", 0x180000, 0x040000, CRC(cb6bd857) SHA1(1bd673e10416bc3ca14859cc15cd05caa7d7a625) )
	ROM_LOAD16_BYTE( "uco-11.67", 0x180001, 0x040000, CRC(199f79c0) SHA1(46f437e90ee25c242bf418c0fa1af77d6e4cafc6) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "uco-1.19", 0x000000, 0x020000, CRC(f284af8e) SHA1(f0b5ef8ae98101bf8c8885e469a5a36dd5e29129) )

	ROM_REGION( 0x000800, REGION_PROMS, 0 )					/* Color PROMs */
	ROM_LOAD( "uc-1o.12", 0x000000, 0x000400, CRC(fa59b5cb) SHA1(171c4c36bd1c8e6548b34a9f6e2ff755ecf09b47) )
	ROM_LOAD( "uc-2o.13", 0x000400, 0x000400, CRC(50a33b96) SHA1(cfb6d3cb6b73d1bf484014fb340c28bc1774137d) )
ROM_END

ROM_START( srmp3 )
	ROM_REGION( 0x028000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD( "za0-10.bin", 0x000000, 0x008000, CRC(939d126f) SHA1(7a5c7f7fbee8de11a08194d3c8f10a20f8dc2f0a) )
	ROM_CONTINUE(           0x010000, 0x018000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "za0-02.bin", 0x000000, 0x080000, CRC(85691946) SHA1(8b91210b1b6671ba2c9ec6722e5dc40bdf44e4b5) )
	ROM_LOAD16_BYTE( "za0-04.bin", 0x000001, 0x080000, CRC(c06e7a96) SHA1(a2dfb81004ea72bfa21724374eb8533af606a5df) )
	ROM_LOAD16_BYTE( "za0-01.bin", 0x100000, 0x080000, CRC(95e0d87c) SHA1(34e6c0a95e63cf092092e27c7ba2f649ebf56507) )
	ROM_LOAD16_BYTE( "za0-03.bin", 0x100001, 0x080000, CRC(7c98570e) SHA1(26e28e67bca9954d62d72260370ea872c6058a10) )
	ROM_LOAD16_BYTE( "za0-06.bin", 0x200000, 0x080000, CRC(8b874b0a) SHA1(27fe1ccc2938e1703e484e2925a2f073064cf019) )
	ROM_LOAD16_BYTE( "za0-08.bin", 0x200001, 0x080000, CRC(3de89d88) SHA1(1e6dabe6aeee6a2613feab26b871c235bf491bfa) )
	ROM_LOAD16_BYTE( "za0-05.bin", 0x300000, 0x080000, CRC(80d3b4e6) SHA1(d31d3f904ee8463c1efbb1d106eeb3dc0dc42ab8) )
	ROM_LOAD16_BYTE( "za0-07.bin", 0x300001, 0x080000, CRC(39d15129) SHA1(62b71a82cfc39e6dab3175e03eca5ff92e854f13) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "za0-11.bin", 0x000000, 0x080000, CRC(2248c23f) SHA1(35591b51bb23dfd7fa81a05026e9ec0789bb0dde) )

	ROM_REGION( 0x000400, REGION_PROMS, 0 )					/* Color PROMs */
	ROM_LOAD( "za0-12.prm", 0x000000, 0x000200, CRC(1ac5387c) SHA1(022f204dbe2374478279b586451673a08ee489c8) )
	ROM_LOAD( "za0-13.prm", 0x000200, 0x000200, CRC(4ea3d2fe) SHA1(c7d18b9c1331e08faadf33e52033c658bf2b16fc) )
ROM_END

ROM_START( mjyuugi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "um001.001", 0x000000, 0x020000, CRC(28d5340f) SHA1(683d89987b8b794695fdb6104d8e6ff5204afafb) )
	ROM_LOAD16_BYTE( "um001.003", 0x000001, 0x020000, CRC(275197de) SHA1(2f8efa112f23f172eaef9bb732b2a253307dd896) )
	ROM_LOAD16_BYTE( "um001.002", 0x040000, 0x020000, CRC(d5dd4710) SHA1(b70c280f828af507c73ebec3209043eb7ce0ce95) )
	ROM_LOAD16_BYTE( "um001.004", 0x040001, 0x020000, CRC(c5ddb567) SHA1(1a35228439108f3d866547d94d4bafca54a710ec) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "maj-001.10",  0x000000, 0x080000, CRC(3c08942a) SHA1(59165052d7c760ac82157844d54c8dced4125259) )
	ROM_LOAD16_BYTE( "maj-001.08",  0x000001, 0x080000, CRC(e2444311) SHA1(88673d57d54ef0674c8c23a95da5d03cb9c894aa) )
	ROM_LOAD16_BYTE( "maj-001.09",  0x100000, 0x080000, CRC(a1974860) SHA1(f944026cf1aadb9c24ac689cc67d374eea17cb85) )
	ROM_LOAD16_BYTE( "maj-001.07",  0x100001, 0x080000, CRC(b1f1d118) SHA1(37d64ba662b431cf0fdee12983c95b9989eb00af) )
	ROM_LOAD16_BYTE( "maj-001.06",  0x200000, 0x080000, CRC(4c60acdd) SHA1(0ab69cc3ea4bebd9e7b139c89b5ac42a621493e2) )
	ROM_LOAD16_BYTE( "maj-001.04",  0x200001, 0x080000, CRC(0a4b2de1) SHA1(f9cddeffcdceb06053216502eb03d52abf527eb2) )
	ROM_LOAD16_BYTE( "maj-001.05",  0x300000, 0x080000, CRC(6be7047a) SHA1(22ce8c6fead9e16550047dea341983f59c3a6c28) )
	ROM_LOAD16_BYTE( "maj-001.03",  0x300001, 0x080000, CRC(c4fb6ea0) SHA1(b5cd3cf71831fecd096cd7bae6fb813504d1e0d5) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "maj-001.01", 0x000000, 0x080000, CRC(029a0b60) SHA1(d02788b8673ae73aca81f1570ff335982ac9ab40) )
	ROM_LOAD( "maj-001.02", 0x080000, 0x080000, CRC(eb28e641) SHA1(67e1d89c9b40e4a83a3783d4343d7a8121668091) )
ROM_END

ROM_START( mjyuugia )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "um_001.001", 0x000000, 0x020000, CRC(76dc0594) SHA1(4bd81616769cdc59eaf6f7921e404e166500f67f) )
	ROM_LOAD16_BYTE( "um001.003",  0x000001, 0x020000, CRC(275197de) SHA1(2f8efa112f23f172eaef9bb732b2a253307dd896) )
	ROM_LOAD16_BYTE( "um001.002",  0x040000, 0x020000, CRC(d5dd4710) SHA1(b70c280f828af507c73ebec3209043eb7ce0ce95) )
	ROM_LOAD16_BYTE( "um001.004",  0x040001, 0x020000, CRC(c5ddb567) SHA1(1a35228439108f3d866547d94d4bafca54a710ec) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "maj-001.10", 0x000000, 0x080000, CRC(3c08942a) SHA1(59165052d7c760ac82157844d54c8dced4125259) )
	ROM_LOAD16_BYTE( "maj-001.08", 0x000001, 0x080000, CRC(e2444311) SHA1(88673d57d54ef0674c8c23a95da5d03cb9c894aa) )
	ROM_LOAD16_BYTE( "maj-001.09", 0x100000, 0x080000, CRC(a1974860) SHA1(f944026cf1aadb9c24ac689cc67d374eea17cb85) )
	ROM_LOAD16_BYTE( "maj-001.07", 0x100001, 0x080000, CRC(b1f1d118) SHA1(37d64ba662b431cf0fdee12983c95b9989eb00af) )
	ROM_LOAD16_BYTE( "maj-001.06", 0x200000, 0x080000, CRC(4c60acdd) SHA1(0ab69cc3ea4bebd9e7b139c89b5ac42a621493e2) )
	ROM_LOAD16_BYTE( "maj-001.04", 0x200001, 0x080000, CRC(0a4b2de1) SHA1(f9cddeffcdceb06053216502eb03d52abf527eb2) )
	ROM_LOAD16_BYTE( "maj-001.05", 0x300000, 0x080000, CRC(6be7047a) SHA1(22ce8c6fead9e16550047dea341983f59c3a6c28) )
	ROM_LOAD16_BYTE( "maj-001.03", 0x300001, 0x080000, CRC(c4fb6ea0) SHA1(b5cd3cf71831fecd096cd7bae6fb813504d1e0d5) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "maj-001.01", 0x000000, 0x080000, CRC(029a0b60) SHA1(d02788b8673ae73aca81f1570ff335982ac9ab40) )
	ROM_LOAD( "maj-001.02", 0x080000, 0x080000, CRC(eb28e641) SHA1(67e1d89c9b40e4a83a3783d4343d7a8121668091) )
ROM_END

ROM_START( ponchin )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "um2_1_1.u22", 0x000000, 0x020000, CRC(cf88efbb) SHA1(7bd2304d365524fc5bcf3fb30752f5efec73a9f5) )
	ROM_LOAD16_BYTE( "um2_1_3.u42", 0x000001, 0x020000, CRC(e053458f) SHA1(db4a34589a08d0252d700144a6260a0f6c4e8e30) )
	ROM_LOAD16_BYTE( "um2_1_2.u29", 0x040000, 0x020000, CRC(5c2f9bcf) SHA1(e2880123373653c7e5d85fb957474e1c5774640d) )
	ROM_LOAD16_BYTE( "um2_1_4.u44", 0x040001, 0x020000, CRC(2ad4e0c7) SHA1(ca97b825af41f86ebbfc2cf88faafb240c4058d1) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "um2_1_8.u55", 0x000000, 0x080000, CRC(f74a8cb3) SHA1(d1bf712f7ef97a96fc251c7729b39e9f10aab45d) )
	ROM_LOAD16_BYTE( "um2_1_7.u43", 0x000001, 0x080000, CRC(1e87ca84) SHA1(5ddbfd92d6ed1947a3c35f3e93cbcca5059fa1f9) )
	ROM_LOAD16_BYTE( "um2_1_6.u28", 0x200000, 0x080000, CRC(b11e85a7) SHA1(02971b45791d06f88efbae8e0713d28105faf341) )
	ROM_LOAD16_BYTE( "um2_1_5.u20", 0x200001, 0x080000, CRC(a5469d11) SHA1(7e96af23c8434c32f87be1482309999d6a7b33bb) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "um2_1_9.u56",  0x000000, 0x080000, CRC(9165c79a) SHA1(854e30fc6121f7b3e5e1e5b6772757a92b63aef8) )
	ROM_LOAD( "um2_1_10.u63", 0x080000, 0x080000, CRC(53e643e9) SHA1(3b221217e8f846ae96a9a47149037cea19d97549) )
ROM_END

ROM_START( ponchina )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )					/* 68000 Code */
	ROM_LOAD16_BYTE( "u22.bin",     0x000000, 0x020000, CRC(9181de20) SHA1(03fdb289d862ff2d87249d35991bd60784e172d9) )
	ROM_LOAD16_BYTE( "um2_1_3.u42", 0x000001, 0x020000, CRC(e053458f) SHA1(db4a34589a08d0252d700144a6260a0f6c4e8e30) )
	ROM_LOAD16_BYTE( "um2_1_2.u29", 0x040000, 0x020000, CRC(5c2f9bcf) SHA1(e2880123373653c7e5d85fb957474e1c5774640d) )
	ROM_LOAD16_BYTE( "um2_1_4.u44", 0x040001, 0x020000, CRC(2ad4e0c7) SHA1(ca97b825af41f86ebbfc2cf88faafb240c4058d1) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "um2_1_8.u55", 0x000000, 0x080000, CRC(f74a8cb3) SHA1(d1bf712f7ef97a96fc251c7729b39e9f10aab45d) )
	ROM_LOAD16_BYTE( "um2_1_7.u43", 0x000001, 0x080000, CRC(1e87ca84) SHA1(5ddbfd92d6ed1947a3c35f3e93cbcca5059fa1f9) )
	ROM_LOAD16_BYTE( "um2_1_6.u28", 0x200000, 0x080000, CRC(b11e85a7) SHA1(02971b45791d06f88efbae8e0713d28105faf341) )
	ROM_LOAD16_BYTE( "um2_1_5.u20", 0x200001, 0x080000, CRC(a5469d11) SHA1(7e96af23c8434c32f87be1482309999d6a7b33bb) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )				/* Samples */
	ROM_LOAD( "um2_1_9.u56",  0x000000, 0x080000, CRC(9165c79a) SHA1(854e30fc6121f7b3e5e1e5b6772757a92b63aef8) )
	ROM_LOAD( "um2_1_10.u63", 0x080000, 0x080000, CRC(53e643e9) SHA1(3b221217e8f846ae96a9a47149037cea19d97549) )
ROM_END


GAME( 1987, srmp2,     0,        srmp2,    srmp2,    srmp2,   ROT0, "Seta", "Super Real Mahjong Part 2 (Japan)", 0 )
GAME( 1988, srmp3,     0,        srmp3,    srmp3,    srmp3,   ROT0, "Seta", "Super Real Mahjong Part 3 (Japan)", 0 )
GAME( 1990, mjyuugi,   0,        mjyuugi,  mjyuugi,  0,       ROT0, "Visco", "Mahjong Yuugi (Japan set 1)", 0 )
GAME( 1990, mjyuugia,  mjyuugi,  mjyuugi,  mjyuugi,  0,       ROT0, "Visco", "Mahjong Yuugi (Japan set 2)", 0 )
GAME( 1991, ponchin,   0,        mjyuugi,  ponchin,  0,       ROT0, "Visco", "Mahjong Pon Chin Kan (Japan set 1)", 0 )
GAME( 1991, ponchina,  ponchin,  mjyuugi,  ponchin,  0,       ROT0, "Visco", "Mahjong Pon Chin Kan (Japan set 2)", 0 )
