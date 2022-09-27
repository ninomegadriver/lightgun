/***************************************************************************

"Combat School" (also known as "Boot Camp") - (Konami GX611)

TODO:
- in combasc (and more generally the 007121) the number of sprites can be
  increased from 0x40 to 0x80. There is a hack in konamiic.c to handle that,
  but it is wrong. If you don't pass the Iron Man stage, a few sprites are
  left dangling on the screen.(*not a bug, 64 sprites are the maximum)
- it seems that to get correct target colors in firing range III we have to
  use the WRONG lookup table (the one for tiles instead of the one for
  sprites).
- in combascb, wrong sprite/char priority (see cpu head at beginning of arm
  wrestling, and heads in intermission after firing range III)
- hook up sound in bootleg (the current sound is a hack, making use of the
  Konami ROMset)
- understand how the trackball really works
- YM2203 pitch is wrong. Fixing it screws up the tempo.

  Update: 3MHz(24MHz/8) is the more appropriate clock speed for the 2203.
  It gives the correct pitch(ear subjective) compared to the official
  soundtrack albeit the music plays slow by about 10%.

  Execution timing of the Z80 is important because it maintains music tempo
  by polling the 2203's second timer. Even when working alone with no
  context-switch the chip shouldn't be running at 1.5MHz otherwise it won't
  keep the right pace. Similar Konmai games from the same period(mainevt,
  battlnts, flkatck...etc.) all have a 3.579545MHz Z80 for sound.

  In spite of adjusting clock speed polling is deemed inaccurate when
  interleaving is taken into account. A high resolution timer around the
  poll loop is probably the best bet. The driver sets its timer manually
  because strange enough, interleaving doesn't occur immediately when
  cpu_boost_interleave() is called. Speculations are TIME_NOWs could have
  been used as the timer durations to force instant triggering.


Credits:

    Hardware Info:
        Jose Tejada Gomez
        Manuel Abadia
        Cesareo Gutierrez

    MAME Driver:
        Phil Stroffolino
        Manuel Abadia

Memory Maps (preliminary):

***************************
* Combat School (bootleg) *
***************************

MAIN CPU:
---------
00c0-00c3   Objects control
0500        bankswitch control
0600-06ff   palette
0800-1fff   RAM
2000-2fff   Video RAM (banked)
3000-3fff   Object RAM (banked)
4000-7fff   Banked Area + IO + Video Registers
8000-ffff   ROM

SOUND CPU:
----------
0000-8000   ROM
8000-87ef   RAM
87f0-87ff   ???
9000-9001   YM2203
9008        ???
9800        OKIM5205?
a000        soundlatch?
a800        OKIM5205?
fffc-ffff   ???


        Notes about the sound systsem of the bootleg:
        ---------------------------------------------
        The positions 0x87f0-0x87ff are very important, it
        does work similar to a semaphore (same as a lot of
        vblank bits). For example in the init code, it writes
        zero to 0x87fa, then it waits to it 'll be different
        to zero, but it isn't written by this cpu. (shareram?)
        I have tried put here a K007232 chip, but it didn't
        work.

        Sound chips: OKI M5205 & YM2203

        We are using the other sound hardware for now.

****************************
* Combat School (Original) *
****************************

0000-005f   Video Registers (banked)
0400-0407   input ports
0408        coin counters
0410        bankswitch control
0600-06ff   palette
0800-1fff   RAM
2000-2fff   Video RAM (banked)
3000-3fff   Object RAM (banked)
4000-7fff   Banked Area + IO + Video Registers
8000-ffff   ROM

SOUND CPU:
----------
0000-8000   ROM
8000-87ff   RAM
9000        uPD7759
b000        uPD7759
c000        uPD7759
d000        soundlatch_r
e000-e001   YM2203

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "sound/upd7759.h"

/* from vidhrdw/combasc.c */
PALETTE_INIT( combasc );
PALETTE_INIT( combascb );
READ8_HANDLER( combasc_video_r );
WRITE8_HANDLER( combasc_video_w );
VIDEO_START( combasc );
VIDEO_START( combascb );

WRITE8_HANDLER( combascb_bankselect_w );
WRITE8_HANDLER( combasc_bankselect_w );
MACHINE_RESET( combasc );
MACHINE_RESET( combascb );
WRITE8_HANDLER( combasc_pf_control_w );
READ8_HANDLER( combasc_scrollram_r );
WRITE8_HANDLER( combasc_scrollram_w );

VIDEO_UPDATE( combascb );
VIDEO_UPDATE( combasc );
WRITE8_HANDLER( combasc_io_w );
WRITE8_HANDLER( combasc_vreg_w );




static WRITE8_HANDLER( combasc_coin_counter_w )
{
	/* b7-b3: unused? */
	/* b1: coin counter 2 */
	/* b0: coin counter 1 */

	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
}

static READ8_HANDLER( trackball_r )
{
	static UINT8 pos[4],sign[4];

	if (offset == 0)
	{
		int i,dir[4];

		for (i = 0;i < 4;i++)
		{
			UINT8 curr;

			curr = readinputport(4 + i);

			dir[i] = curr - pos[i];
			sign[i] = dir[i] & 0x80;
			pos[i] = curr;
		}

		/* fix sign for orthogonal movements */
		if (dir[0] || dir[1])
		{
			if (!dir[0]) sign[0] = sign[1] ^ 0x80;
			if (!dir[1]) sign[1] = sign[0];
		}
		if (dir[2] || dir[3])
		{
			if (!dir[2]) sign[2] = sign[3] ^ 0x80;
			if (!dir[3]) sign[3] = sign[2];
		}
	}

	return sign[offset] | (pos[offset] & 0x7f);
}


/* the protection is a simple multiply */
static int prot[2];

static WRITE8_HANDLER( protection_w )
{
	prot[offset] = data;
}
static READ8_HANDLER( protection_r )
{
	return ((prot[0] * prot[1]) >> (offset * 8)) & 0xff;
}
static WRITE8_HANDLER( protection_clock_w )
{
	/* 0x3f is written here every time before accessing the other registers */
}


/****************************************************************************/

static WRITE8_HANDLER( combasc_sh_irqtrigger_w )
{
	cpunum_set_input_line_and_vector(1,0,HOLD_LINE,0xff);
}

static WRITE8_HANDLER( combasc_play_w )
{
	upd7759_start_w(0, data & 2);
}

static WRITE8_HANDLER( combasc_voice_reset_w )
{
    upd7759_reset_w(0,data & 1);
}

static WRITE8_HANDLER( combasc_portA_w )
{
	/* unknown. always write 0 */
}

static mame_timer *combasc_interleave_timer;

static READ8_HANDLER ( combasc_YM2203_status_port_0_r )
{
	static int boost = 1;
	int status = YM2203_status_port_0_r(0);

	if (activecpu_get_pc() == 0x334)
	{
		if (boost)
		{
			boost = 0;
			timer_adjust(combasc_interleave_timer, TIME_NOW, 0, TIME_IN_CYCLES(80,1));
		}
		else if (status & 2)
		{
			boost = 1;
			timer_adjust(combasc_interleave_timer, TIME_NOW, 0, TIME_NEVER);
		}
	}

	return(status);
}

/****************************************************************************/

static ADDRESS_MAP_START( combasc_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0020, 0x005f) AM_READ(combasc_scrollram_r)
	AM_RANGE(0x0200, 0x0201) AM_READ(protection_r)
	AM_RANGE(0x0400, 0x0400) AM_READ(input_port_0_r)
	AM_RANGE(0x0401, 0x0401) AM_READ(input_port_1_r)			/* DSW #3 */
	AM_RANGE(0x0402, 0x0402) AM_READ(input_port_2_r)			/* DSW #1 */
	AM_RANGE(0x0403, 0x0403) AM_READ(input_port_3_r)			/* DSW #2 */
	AM_RANGE(0x0404, 0x0407) AM_READ(trackball_r)			/* 1P & 2P controls / trackball */
	AM_RANGE(0x0600, 0x06ff) AM_READ(MRA8_RAM)				/* palette */
	AM_RANGE(0x0800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(combasc_video_r)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)				/* banked ROM area */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)				/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( combasc_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0007) AM_WRITE(combasc_pf_control_w)
	AM_RANGE(0x0020, 0x005f) AM_WRITE(combasc_scrollram_w)
//  AM_RANGE(0x0060, 0x00ff) AM_WRITE(MWA8_RAM)                 /* RAM */
	AM_RANGE(0x0200, 0x0201) AM_WRITE(protection_w)
	AM_RANGE(0x0206, 0x0206) AM_WRITE(protection_clock_w)
	AM_RANGE(0x0408, 0x0408) AM_WRITE(combasc_coin_counter_w)	/* coin counters */
	AM_RANGE(0x040c, 0x040c) AM_WRITE(combasc_vreg_w)
	AM_RANGE(0x0410, 0x0410) AM_WRITE(combasc_bankselect_w)
	AM_RANGE(0x0414, 0x0414) AM_WRITE(soundlatch_w)
	AM_RANGE(0x0418, 0x0418) AM_WRITE(combasc_sh_irqtrigger_w)
	AM_RANGE(0x041c, 0x041c) AM_WRITE(watchdog_reset_w)			/* watchdog reset? */
	AM_RANGE(0x0600, 0x06ff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)
	AM_RANGE(0x0800, 0x1fff) AM_WRITE(MWA8_RAM)					/* RAM */
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(combasc_video_w)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_ROM)					/* banked ROM area */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)					/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( combascb_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x04ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0600, 0x06ff) AM_READ(MRA8_RAM)	/* palette */
	AM_RANGE(0x0800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(combasc_video_r)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)				/* banked ROM/RAM area */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)				/* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( combascb_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x04ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0500, 0x0500) AM_WRITE(combascb_bankselect_w)
	AM_RANGE(0x0600, 0x06ff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_le_w) AM_BASE(&paletteram)
	AM_RANGE(0x0800, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(combasc_video_w)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(MWA8_BANK1) /* banked ROM/RAM area */
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)				/* ROM */
ADDRESS_MAP_END

#if 0
static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)					/* ROM */
	AM_RANGE(0x8000, 0x87ef) AM_READ(MRA8_RAM)					/* RAM */
	AM_RANGE(0x87f0, 0x87ff) AM_READ(MRA8_RAM)					/* ??? */
	AM_RANGE(0x9000, 0x9000) AM_READ(YM2203_status_port_0_r)		/* YM 2203 */
	AM_RANGE(0x9008, 0x9008) AM_READ(YM2203_status_port_0_r)		/* ??? */
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)				/* soundlatch_r? */
	AM_RANGE(0x8800, 0xfffb) AM_READ(MRA8_ROM)					/* ROM? */
	AM_RANGE(0xfffc, 0xffff) AM_READ(MRA8_RAM)					/* ??? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)				/* ROM */
	AM_RANGE(0x8000, 0x87ef) AM_WRITE(MWA8_RAM)				/* RAM */
	AM_RANGE(0x87f0, 0x87ff) AM_WRITE(MWA8_RAM)				/* ??? */
 	AM_RANGE(0x9000, 0x9000) AM_WRITE(YM2203_control_port_0_w)/* YM 2203 */
	AM_RANGE(0x9001, 0x9001) AM_WRITE(YM2203_write_port_0_w)	/* YM 2203 */
	//AM_RANGE(0x9800, 0x9800) AM_WRITE(combasc_unknown_w_1)    /* OKIM5205? */
	//AM_RANGE(0xa800, 0xa800) AM_WRITE(combasc_unknown_w_2)    /* OKIM5205? */
	AM_RANGE(0x8800, 0xfffb) AM_WRITE(MWA8_ROM)				/* ROM */
	AM_RANGE(0xfffc, 0xffff) AM_WRITE(MWA8_RAM)				/* ??? */
ADDRESS_MAP_END
#endif

static ADDRESS_MAP_START( combasc_readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)					/* ROM */
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)					/* RAM */
	AM_RANGE(0xb000, 0xb000) AM_READ(upd7759_0_busy_r)			/* upd7759 busy? */
	AM_RANGE(0xd000, 0xd000) AM_READ(soundlatch_r)				/* soundlatch_r? */
	AM_RANGE(0xe000, 0xe000) AM_READ(combasc_YM2203_status_port_0_r)	/* YM 2203 intercepted */
ADDRESS_MAP_END

static ADDRESS_MAP_START( combasc_writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)				/* ROM */
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)				/* RAM */
	AM_RANGE(0x9000, 0x9000) AM_WRITE(combasc_play_w)			/* upd7759 play voice */
	AM_RANGE(0xa000, 0xa000) AM_WRITE(upd7759_0_port_w)		/* upd7759 voice select */
	AM_RANGE(0xc000, 0xc000) AM_WRITE(combasc_voice_reset_w)	/* upd7759 reset? */
 	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2203_control_port_0_w)/* YM 2203 */
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2203_write_port_0_w)	/* YM 2203 */
ADDRESS_MAP_END


#define COINAGE \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x00, "coin 2 invalidity" )

INPUT_PORTS_START( combasc )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW #3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On )  )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW # 1 */
	COINAGE

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING( 0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING( 0x20, "Difficult" )
	PORT_DIPSETTING( 0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)

	PORT_START	/* only used in trackball version */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* only used in trackball version */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* only used in trackball version */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( combasct )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW #3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On )  )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW # 1 */
	COINAGE

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING( 0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING( 0x20, "Difficult" )
	PORT_DIPSETTING( 0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	/* trackball 1P */
	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_PLAYER(1)

	/* trackball 2P (not implemented yet) */
	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( combascb )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING( 0x10, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING( 0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING( 0x20, "Difficult" )
	PORT_DIPSETTING( 0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END



static const gfx_layout gfxlayout =
{
	8,8,
	0x4000,
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tile_layout =
{
	8,8,
	0x2000, /* number of tiles */
	4,		/* bitplanes */
	{ 0*0x10000*8, 1*0x10000*8, 2*0x10000*8, 3*0x10000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_layout sprite_layout =
{
	16,16,
	0x800,	/* number of sprites */
	4,		/* bitplanes */
	{ 3*0x10000*8, 2*0x10000*8, 1*0x10000*8, 0*0x10000*8 }, /* plane offsets */
	{
		0,1,2,3,4,5,6,7,
		16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7
	},
	{
		0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8
	},
	8*8*4
};

static const gfx_decode combasc_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &gfxlayout, 0, 8*16 },
	{ REGION_GFX2, 0x00000, &gfxlayout, 0, 8*16 },
	{ -1 }
};

static const gfx_decode combascb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tile_layout,   0, 8*16 },
	{ REGION_GFX1, 0x40000, &tile_layout,   0, 8*16 },
	{ REGION_GFX2, 0x00000, &sprite_layout, 0, 8*16 },
	{ REGION_GFX2, 0x40000, &sprite_layout, 0, 8*16 },
	{ -1 }
};

static struct YM2203interface ym2203_interface =
{
	0,
	0,
	combasc_portA_w,
	0
};

static struct upd7759_interface upd7759_interface =
{
	REGION_SOUND1,			/* memory region */
	0
};



/* combat school (original) */
static MACHINE_DRIVER_START( combasc )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)	/* 3 MHz? */
	MDRV_CPU_PROGRAM_MAP(combasc_readmem,combasc_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,3579545)	/* 3.579545 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(combasc_readmem_sound,combasc_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(20)

	MDRV_MACHINE_RESET(combasc)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(combasc_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)
	MDRV_COLORTABLE_LENGTH(8*16*16)

	MDRV_PALETTE_INIT(combasc)
	MDRV_VIDEO_START(combasc)
	MDRV_VIDEO_UPDATE(combasc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd7759_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)
MACHINE_DRIVER_END

/* combat school (bootleg on different hardware) */
static MACHINE_DRIVER_START( combascb )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)	/* 3 MHz? */
	MDRV_CPU_PROGRAM_MAP(combascb_readmem,combascb_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,3579545)	/* 3.579545 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(combasc_readmem_sound,combasc_writemem_sound) /* FAKE */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(20)

	MDRV_MACHINE_RESET(combasc)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(combascb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)
	MDRV_COLORTABLE_LENGTH(8*16*16)

	MDRV_PALETTE_INIT(combascb)
	MDRV_VIDEO_START(combascb)
	MDRV_VIDEO_UPDATE(combascb)

	/* We are using the original sound subsystem */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd7759_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)
MACHINE_DRIVER_END



ROM_START( combasc )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 6309 code */
	ROM_LOAD( "611g01.rom", 0x30000, 0x08000, CRC(857ffffe) SHA1(de7566d58314df4b7fdc07eb31a3f9bdd12d1a73) )
	ROM_CONTINUE(           0x08000, 0x08000 )
	ROM_LOAD( "611g02.rom", 0x10000, 0x20000, CRC(9ba05327) SHA1(ea03845fb49d18ac4fca97cfffce81db66b9967b) )
	/* extra 0x8000 for banked RAM */

	ROM_REGION( 0x10000 , REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "611g03.rom", 0x00000, 0x08000, CRC(2a544db5) SHA1(94a97c3c54bf13ccc665aa5057ac6b1d700fae2d) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g07.rom",    0x00000, 0x40000, CRC(73b38720) SHA1(e109eb78aea464127d813284ca040e8d719599e3) )
	ROM_LOAD16_BYTE( "611g08.rom",    0x00001, 0x40000, CRC(46e7d28c) SHA1(1ece7fac954204ac35d00f3d573964fcf82dcf77) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g11.rom",    0x00000, 0x40000, CRC(69687538) SHA1(4349a1c052a759acdf7259f8bf8c5c9489b788f2) )
	ROM_LOAD16_BYTE( "611g12.rom",    0x00001, 0x40000, CRC(9c6bf898) SHA1(eafc227b4e7df0c652ec7d78784c039c35965fdc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "611g06.h14",  0x0000, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g05.h15",  0x0100, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */
	ROM_LOAD( "611g10.h6",   0x0200, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g09.h7",   0x0300, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* uPD7759 data */
	ROM_LOAD( "611g04.rom",  0x00000, 0x20000, CRC(2987e158) SHA1(87c5129161d3be29a339083349807e60b625c3f7) )
ROM_END

ROM_START( combasct )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 6309 code */
	ROM_LOAD( "g01.rom",     0x30000, 0x08000, CRC(489c132f) SHA1(c717195f89add4be4a21ecc1ddd58361b0ab4a74) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "611g02.rom",  0x10000, 0x20000, CRC(9ba05327) SHA1(ea03845fb49d18ac4fca97cfffce81db66b9967b) )
	/* extra 0x8000 for banked RAM */

	ROM_REGION( 0x10000 , REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "611g03.rom", 0x00000, 0x08000, CRC(2a544db5) SHA1(94a97c3c54bf13ccc665aa5057ac6b1d700fae2d) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g07.rom",    0x00000, 0x40000, CRC(73b38720) SHA1(e109eb78aea464127d813284ca040e8d719599e3) )
	ROM_LOAD16_BYTE( "611g08.rom",    0x00001, 0x40000, CRC(46e7d28c) SHA1(1ece7fac954204ac35d00f3d573964fcf82dcf77) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g11.rom",    0x00000, 0x40000, CRC(69687538) SHA1(4349a1c052a759acdf7259f8bf8c5c9489b788f2) )
	ROM_LOAD16_BYTE( "611g12.rom",    0x00001, 0x40000, CRC(9c6bf898) SHA1(eafc227b4e7df0c652ec7d78784c039c35965fdc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "611g06.h14",  0x0000, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g05.h15",  0x0100, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */
	ROM_LOAD( "611g10.h6",   0x0200, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g09.h7",   0x0300, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* uPD7759 data */
	ROM_LOAD( "611g04.rom",  0x00000, 0x20000, CRC(2987e158) SHA1(87c5129161d3be29a339083349807e60b625c3f7) )
ROM_END

ROM_START( combascj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 6309 code */
	ROM_LOAD( "611p01.a14",  0x30000, 0x08000, CRC(d748268e) SHA1(91588b6a0d3af47065204b980a56544a9f29b6d9) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "611g02.rom",  0x10000, 0x20000, CRC(9ba05327) SHA1(ea03845fb49d18ac4fca97cfffce81db66b9967b) )
	/* extra 0x8000 for banked RAM */

	ROM_REGION( 0x10000 , REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "611g03.rom", 0x00000, 0x08000, CRC(2a544db5) SHA1(94a97c3c54bf13ccc665aa5057ac6b1d700fae2d) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g07.rom",    0x00000, 0x40000, CRC(73b38720) SHA1(e109eb78aea464127d813284ca040e8d719599e3) )
	ROM_LOAD16_BYTE( "611g08.rom",    0x00001, 0x40000, CRC(46e7d28c) SHA1(1ece7fac954204ac35d00f3d573964fcf82dcf77) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g11.rom",    0x00000, 0x40000, CRC(69687538) SHA1(4349a1c052a759acdf7259f8bf8c5c9489b788f2) )
	ROM_LOAD16_BYTE( "611g12.rom",    0x00001, 0x40000, CRC(9c6bf898) SHA1(eafc227b4e7df0c652ec7d78784c039c35965fdc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "611g06.h14",  0x0000, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g05.h15",  0x0100, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */
	ROM_LOAD( "611g10.h6",   0x0200, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g09.h7",   0x0300, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* uPD7759 data */
	ROM_LOAD( "611g04.rom",  0x00000, 0x20000, CRC(2987e158) SHA1(87c5129161d3be29a339083349807e60b625c3f7) )
ROM_END

ROM_START( bootcamp )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 6309 code */
	ROM_LOAD( "xxx-v01.12a", 0x30000, 0x08000, CRC(c10dca64) SHA1(f34de26e998b1501e430d46e96cdc58ebc68481e) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "611g02.rom",  0x10000, 0x20000, CRC(9ba05327) SHA1(ea03845fb49d18ac4fca97cfffce81db66b9967b) )
	/* extra 0x8000 for banked RAM */

	ROM_REGION( 0x10000 , REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "611g03.rom", 0x00000, 0x08000, CRC(2a544db5) SHA1(94a97c3c54bf13ccc665aa5057ac6b1d700fae2d) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g07.rom",    0x00000, 0x40000, CRC(73b38720) SHA1(e109eb78aea464127d813284ca040e8d719599e3) )
	ROM_LOAD16_BYTE( "611g08.rom",    0x00001, 0x40000, CRC(46e7d28c) SHA1(1ece7fac954204ac35d00f3d573964fcf82dcf77) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "611g11.rom",    0x00000, 0x40000, CRC(69687538) SHA1(4349a1c052a759acdf7259f8bf8c5c9489b788f2) )
	ROM_LOAD16_BYTE( "611g12.rom",    0x00001, 0x40000, CRC(9c6bf898) SHA1(eafc227b4e7df0c652ec7d78784c039c35965fdc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "611g06.h14",  0x0000, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g05.h15",  0x0100, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */
	ROM_LOAD( "611g10.h6",   0x0200, 0x0100, CRC(f916129a) SHA1(d5e4a8a3baab8fcdac86ef5182858cede1abf040) ) /* sprites lookup table */
	ROM_LOAD( "611g09.h7",   0x0300, 0x0100, CRC(207a7b07) SHA1(f4e638e7f182e5228a062b243406d0ceaaa5bfdc) ) /* chars lookup table */

    ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* uPD7759 data */
	ROM_LOAD( "611g04.rom",  0x00000, 0x20000, CRC(2987e158) SHA1(87c5129161d3be29a339083349807e60b625c3f7) )
ROM_END

ROM_START( combascb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "combat.002",	 0x30000, 0x08000, CRC(0996755d) SHA1(bb6bbbf7ab3b5fab5e1c6cebc7b3f0d720493c3b) )
	ROM_CONTINUE(            0x08000, 0x08000 )
	ROM_LOAD( "combat.003",	 0x10000, 0x10000, CRC(229c93b2) SHA1(ac3fd3df1bb5f6a461d0d1423c50568348ef69df) )
	ROM_LOAD( "combat.004",	 0x20000, 0x10000, CRC(a069cb84) SHA1(f49f70afb17df46b16f5801ef42edb0706730723) )
	/* extra 0x8000 for banked RAM */

	ROM_REGION( 0x10000 , REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "combat.001",  0x00000, 0x10000, CRC(61456b3b) SHA1(320db628283dd1bec465e95020d1a1158e6d6ae4) )
	ROM_LOAD( "611g03.rom",  0x00000, 0x08000, CRC(2a544db5) SHA1(94a97c3c54bf13ccc665aa5057ac6b1d700fae2d) ) /* FAKE - from Konami set! */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "combat.006",  0x00000, 0x10000, CRC(8dc29a1f) SHA1(564dd7c6acff34db93b8e300dda563f5f38ba159) ) /* tiles, bank 0 */
	ROM_LOAD( "combat.008",  0x10000, 0x10000, CRC(61599f46) SHA1(cfd79a88bb496773daf207552c67f595ee696bc4) )
	ROM_LOAD( "combat.010",  0x20000, 0x10000, CRC(d5cda7cd) SHA1(140db6270c3f358aa27013db3bb819a48ceb5142) )
	ROM_LOAD( "combat.012",  0x30000, 0x10000, CRC(ca0a9f57) SHA1(d6b3daf7c34345bb2f64068d480bd51d7bb36e4d) )
	ROM_LOAD( "combat.005",  0x40000, 0x10000, CRC(0803a223) SHA1(67d4162385dd56d5396e181070bfa6760521eb45) ) /* tiles, bank 1 */
	ROM_LOAD( "combat.007",  0x50000, 0x10000, CRC(23caad0c) SHA1(0544cde479c6d4192da5bb4b6f0e2e75d09663c3) )
	ROM_LOAD( "combat.009",  0x60000, 0x10000, CRC(5ac80383) SHA1(1e89c371a92afc000d593daebda4156952a15244) )
	ROM_LOAD( "combat.011",  0x70000, 0x10000, CRC(cda83114) SHA1(12d2a9f694287edb3bb0ee7a8ba0e0724dad8e1f) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "combat.013",  0x00000, 0x10000, CRC(4bed2293) SHA1(3369de47d4ba041d9f17a18dcca2af7ac9f8bc0c) ) /* sprites, bank 0 */
	ROM_LOAD( "combat.015",  0x10000, 0x10000, CRC(26c41f31) SHA1(f8eb7d0729a21a0dd92ce99c9cda0cde9526b861) )
	ROM_LOAD( "combat.017",  0x20000, 0x10000, CRC(6071e6da) SHA1(ba5f8e83b07faaffc564d3568630e17efdb5a09f) )
	ROM_LOAD( "combat.019",  0x30000, 0x10000, CRC(3b1cf1b8) SHA1(ff4de37c051bcb374c44d1b99006ff6ff5e1f927) )
	ROM_LOAD( "combat.014",  0x40000, 0x10000, CRC(82ea9555) SHA1(59bf7836938ce9e3242d1cca754de8dbe85bbfb7) ) /* sprites, bank 1 */
	ROM_LOAD( "combat.016",  0x50000, 0x10000, CRC(2e39bb70) SHA1(a6c4acd93cc803e987de6e18fbdc5ce4634b14a8) )
	ROM_LOAD( "combat.018",  0x60000, 0x10000, CRC(575db729) SHA1(6b1676da4f24fc90c77262789b6cc116184ab912) )
	ROM_LOAD( "combat.020",  0x70000, 0x10000, CRC(8d748a1a) SHA1(4386e14e19b91e053033dde2a13019bc6d8e1d5a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom.d10",    0x0000, 0x0100, CRC(265f4c97) SHA1(76f1b75a593d3d77ef6173a1948f842d5b27d418) ) /* sprites lookup table */
	ROM_LOAD( "prom.c11",    0x0100, 0x0100, CRC(a7a5c0b4) SHA1(48bfc3af40b869599a988ebb3ed758141bcfd4fc) ) /* priority? */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* uPD7759 data */
	ROM_LOAD( "611g04.rom",  0x00000, 0x20000, CRC(2987e158) SHA1(87c5129161d3be29a339083349807e60b625c3f7) )	/* FAKE - from Konami set! */
ROM_END



static void combasc_init_common(void)
{
	combasc_interleave_timer = timer_alloc(NULL);
}

static DRIVER_INIT( combasct )
{
	combasc_init_common();
}

static DRIVER_INIT( combasc )
{
	/* joystick instead of trackball */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0404, 0x0404, 0, 0, input_port_4_r);

	combasc_init_common();
}

static DRIVER_INIT( combascb )
{
	combasc_init_common();
}



GAME( 1988, combasc,  0,       combasc,  combasc,  combasc,  ROT0, "Konami", "Combat School (joystick)", 0 )
GAME( 1987, combasct, combasc, combasc,  combasct, combasct, ROT0, "Konami", "Combat School (trackball)", 0 )
GAME( 1987, combascj, combasc, combasc,  combasct, combasct, ROT0, "Konami", "Combat School (Japan trackball)", 0 )
GAME( 1987, bootcamp, combasc, combasc,  combasct, combasct, ROT0, "Konami", "Boot Camp", 0 )
GAME( 1988, combascb, combasc, combascb, combascb, combascb, ROT0, "bootleg", "Combat School (bootleg)", GAME_IMPERFECT_COLORS )
