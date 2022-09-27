/***************************************************************************

Ping Pong King  (c) Taito 1985
Gladiator       (c) Taito 1986

Credits:
- Victor Trucco: original emulation and MAME driver
- Steve Ellenoff: YM2203 Sound, ADPCM Sound, dip switch fixes, high score save,
          input port patches, panning fix, sprite banking,
          Golden Castle Rom Set Support
- Phil Stroffolino: palette, sprites, misc video driver fixes
- Tatsuyuki Satoh: YM2203 sound improvements, NEC 8741 simulation,ADPCM with MC6809
- Tomasz Slanina   preliminary Ping POng King driver
- Nicola Salmoria  clean up

special thanks to:
- Camilty for precious hardware information and screenshots
- Jason Richmond for hardware information and misc. notes
- Joe Rounceville for schematics
- and everyone else who'se offered support along the way!


***************************************************************************

PING PONG KING
TAITO 1985
M6100094A

-------------
P0-003


X Q0_10 Q0_9 Q0_8 Q0_7 Q0_6 6116 Q0_5 Q0_4 Q0_3 Q1_2 Q1_1
                                                 Q1
                                   Z80B
                                       8251
 2009 2009 2009
               AQ-001
          Q2
              2116
              2116
                                 AQ-003
-------------
P1-004


 QO_15 TMM2009 Q0_14 Q0_13 Q0_12     2128 2128


                                                           SW1
                                                    AQ-004

                                              SW2          SW3
   6809 X Q0_19 Q0_18
                           Z80  Q0_17 Q0_16 2009 AQ-003 YM2203


***************************************************************************

Gladiator Memory Map
--------------------

Main CPU (Z80)
--------------
The address decoding is done by two PROMs, Q3 @ 2B and Q4 @ 5S.

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
00xxxxxxxxxxxxxx R   xxxxxxxx ROM 1F    program ROM
010xxxxxxxxxxxxx R   xxxxxxxx ROM 1D    program ROM
011xxxxxxxxxxxxx R   xxxxxxxx ROM 1A    program ROM (paged)
10xxxxxxxxxxxxxx R   xxxxxxxx ROM 1C    program ROM (paged)
110000xxxxxxxxxx R/W xxxxxxxx RAM 4S    sprite RAM [1]
110001xxxxxxxxxx R/W xxxxxxxx RAM 4U    sprite RAM
110010xxxxxxxxxx R/W xxxxxxxx RAM 4T    sprite RAM
110011000-------   W xxxxxxxx VCSA2     fg relative scroll Y?
110011001-------   W ------xx           fg tile bank select
110011001-------   W -----x--           bg scroll X msb
110011001-------   W ----x---           fg scroll X msb
110011001-------   W ---x----           bg tile bank select
110011001-------   W --x-----           blank screen?
11001101--------   W xxxxxxxx VCSA1     fg scroll X
11001110--------   W xxxxxxxx           fg+bg scroll Y?
11001111--------   W xxxxxxxx VCSA3     bg scroll X
11010xxxxxxxxxxx R/W xxxxxxxx CCS       palette RAM
11011xxxxxxxxxxx R/W xxxxxxxx VCS1      bg tilemap RAM
11100xxxxxxxxxxx R/W xxxxxxxx VCS2      bg tilemap RAM
11101xxxxxxxxxxx R/W xxxxxxxx VCS3      fg tilemap RAM
11110xxxxxxxxxxx R/W xxxxxxxx RAM 1H    work RAM (battery backed)
11111-----------              n.c.

[1] only the first 256 bytes of each RAM actually contain sprite data (two buffers
    of 128 bytes).

I/O ports:

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
--------000--000   W -------x OBJACS ?  select active sprite buffer
--------000--001   W -------x OBJCGBK   sprite bank select
--------000--010   W -------x PR.BK     ROM bank select
--------000--011   W -------x NMIFG     NMI enable/acknowledge (NMI not used by Gladiator)
--------000--100   W -------x SRST      reset sound CPU
--------000--101   W -------x CBK0      unknown
--------000--110   W -------x LOBJ      unknown (related to sprites)
--------000--111   W -------x REVERS    flip screen
--------001-----              n.c.
--------010-----              n.c.
--------011-----              n.c.
--------100----x R/W xxxxxxxx CIOMCS    8741 #0 (communication with 2nd Z80, DIPSW1)
--------101----- R/W          ARST      watchdog reset
--------110----x R/W xxxxxxxx SI/.CS    8251 (serial communication for debug purposes)
--------111-----              n.c.


Sound CPU (Z80)
---------------

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
00xxxxxxxxxxxxxx R   xxxxxxxx ROM 6F    program ROM
01xxxxxxxxxxxxxx R   xxxxxxxx ROM 6E    program ROM
10----xxxxxxxxxx R/W xxxxxxxx RAM 6D    work RAM
11--------------              n.c.

I/O ports:

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
--------000----x R/W xxxxxxxx OPNCS     YM2203 (portA: SSRST, unknown; portB: DIPSW3)
--------001----x R/W xxxxxxxx CIOSCS    8741 #1 (communication with main Z80, DIPSW2)
--------010----- R/W -------- INTCS     irq enable/acknowledge
--------011----x R/W xxxxxxxx AX1       8741 #2 (digital inputs, coins)
--------100----x R/W xxxxxxxx AX2       8741 #3 (digital inputs)
--------101--xxx   W -------x FCS       control filters in sound output section
--------110-----              n.c.
--------111-----   W xxxxxxxx ADCS      command to 6809


Third CPU (6809)
----------------

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0000------------              n.c.
0001------------   W ----xxxx           MSM5205 data
0001------------   W ---x----           MSM5205 clock
0001------------   W --x-----           MSM5205 reset
0001------------   W -x------ ADBR      ROM bank select
0010------------ R   xxxxxxxx           command from Z80
0011------------              n.c.
01xxxxxxxxxxxxxx R   xxxxxxxx ROM 5P    program ROM (banked)
10xxxxxxxxxxxxxx R   xxxxxxxx ROM 5N    program ROM (banked)
11xxxxxxxxxxxxxx R   xxxxxxxx ROM 5M    program ROM (banked)


***************************************************************************

Notes:
------
- The fg tilemap is a 1bpp layer which selects the second palette bank when
  active, so it could be used for some cool effects. Gladiator just sets the
  whole palette to white so we can just treat it as a monochromatic layer.
- tilemap Y scroll is not implemented because the game doesn't use it so I can't
  verify it's right.

TODO:
-----
- gladiatr_irq_patch_w, which triggers irq on the second CPU, is a kludge. It
  shouldn't work that way, that address should actually reset the second CPU
  (but the main CPU never asserts the line). The schematics are too fuzzy to
  understand what should trigger irq on the second CPU. Just using a vblank
  interrupt doesn't work, probably because the CPU expects interrupts to only
  begin happening when the main CPU has finished the self test.
- YM2203 mixing problems (loss of bass notes)
- YM2203 some sound effects just don't sound correct
- Audio Filter Switch not hooked up (might solve YM2203 mixing issue)
- Ports 60,61,80,81 not fully understood yet...
- The four 8741 ROMs are available but not used.


***************************************************************************/

#include "driver.h"
#include "machine/tait8741.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"


/*Video functions*/
extern UINT8 *gladiatr_videoram, *gladiatr_colorram,*gladiatr_textram;
WRITE8_HANDLER( gladiatr_videoram_w );
WRITE8_HANDLER( gladiatr_colorram_w );
WRITE8_HANDLER( gladiatr_textram_w );
WRITE8_HANDLER( gladiatr_paletteram_w );
WRITE8_HANDLER( ppking_video_registers_w );
WRITE8_HANDLER( gladiatr_video_registers_w );
WRITE8_HANDLER( gladiatr_spritebuffer_w );
WRITE8_HANDLER( gladiatr_spritebank_w );
VIDEO_START( ppking );
VIDEO_UPDATE( ppking );
VIDEO_START( gladiatr );
VIDEO_UPDATE( gladiatr );


/*Rom bankswitching*/
WRITE8_HANDLER( gladiatr_bankswitch_w )
{
	UINT8 *rom = memory_region(REGION_CPU1) + 0x10000;

	memory_set_bankptr(1, rom + 0x6000 * (data & 0x01));
}


static READ8_HANDLER( gladiator_dsw1_r )
{
	int orig = readinputportbytag("DSW1")^0xff;

	return BITSWAP8(orig, 0,1,2,3,4,5,6,7);
}

static READ8_HANDLER( gladiator_dsw2_r )
{
	int orig = readinputportbytag("DSW2")^0xff;

	return BITSWAP8(orig, 2,3,4,5,6,7,1,0);
}

static READ8_HANDLER( gladiator_controll_r )
{
	int coins = 0;

	if( readinputportbytag("COINS") & 0xc0 ) coins = 0x80;
	switch(offset)
	{
	case 0x01: /* start button , coins */
		return readinputportbytag("IN0") | coins;
	case 0x02: /* Player 1 Controller , coins */
		return readinputportbytag("IN1") | coins;
	case 0x04: /* Player 2 Controller , coins */
		return readinputportbytag("IN2") | coins;
	}
	/* unknown */
	return 0;
}

static READ8_HANDLER( gladiator_button3_r )
{
	switch(offset)
	{
	case 0x01: /* button 3 */
		return readinputportbytag("IN3");
	}
	/* unknown */
	return 0;
}

static struct TAITO8741interface gsword_8741interface=
{
	4,         /* 4 chips */
	{TAITO8741_MASTER,TAITO8741_SLAVE,TAITO8741_PORT,TAITO8741_PORT},/* program mode */
	{1,0,0,0},	/* serial port connection */
	{gladiator_dsw1_r,gladiator_dsw2_r,gladiator_button3_r,gladiator_controll_r}	/* port handler */
};

static MACHINE_RESET( gladiator )
{
	TAITO8741_start(&gsword_8741interface);
	/* 6809 bank memory set */
	{
		UINT8 *rom = memory_region(REGION_CPU3) + 0x10000;
		memory_set_bankptr(2,rom);
	}
}

/* YM2203 port A handler (input) */
static READ8_HANDLER( gladiator_dsw3_r )
{
	return readinputportbytag("DSW3");
}
/* YM2203 port B handler (output) */
static WRITE8_HANDLER( gladiator_int_control_w )
{
	/* bit 7   : SSRST = sound reset ? */
	/* bit 6-1 : N.C.                  */
	/* bit 0   : ??                    */
}
/* YM2203 IRQ */
static void gladiator_ym_irq(int irq)
{
	/* NMI IRQ is not used by gladiator sound program */
	cpunum_set_input_line(1, INPUT_LINE_NMI, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Sound Functions*/
static WRITE8_HANDLER( glad_adpcm_w )
{
	UINT8 *rom = memory_region(REGION_CPU3) + 0x10000;

	/* bit6 = bank offset */
	memory_set_bankptr(2,rom + ((data & 0x40) ? 0xc000 : 0));

	MSM5205_data_w(0,data);         /* bit0..3  */
	MSM5205_reset_w(0,(data>>5)&1); /* bit 5    */
	MSM5205_vclk_w (0,(data>>4)&1); /* bit4     */
}

static WRITE8_HANDLER( glad_cpu_sound_command_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(2, INPUT_LINE_NMI, ASSERT_LINE);
}

static READ8_HANDLER( glad_cpu_sound_command_r )
{
	cpunum_set_input_line(2, INPUT_LINE_NMI, CLEAR_LINE);
	return soundlatch_r(0);
}

static WRITE8_HANDLER( gladiatr_flipscreen_w )
{
	flip_screen_set(data & 1);
}


#if 1
/* !!!!! patch to IRQ timming for 2nd CPU !!!!! */
WRITE8_HANDLER( gladiatr_irq_patch_w )
{
	cpunum_set_input_line(1,0,HOLD_LINE);
}
#endif






static int data1,data2,flag1=1,flag2=1;

static WRITE8_HANDLER(qx0_w)
{
	if(!offset)
	{
		data2=data;
		flag2=1;
	}
}

static WRITE8_HANDLER(qx1_w)
{
	if(!offset)
	{
		data1=data;
		flag1=1;
	}
}

static WRITE8_HANDLER(qx2_w){ }

static WRITE8_HANDLER(qx3_w){ }

static READ8_HANDLER(qx2_r){ return rand(); }

static READ8_HANDLER(qx3_r){ return rand()&0xf; }

static READ8_HANDLER(qx0_r)
{
	if(!offset)
		 return data1;
	else
		return flag2;
}

static READ8_HANDLER(qx1_r)
{
	if(!offset)
		return data2;
	else
		return flag1;
}

static ADDRESS_MAP_START( ppking_cpu1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcbff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xcc00, 0xcfff) AM_WRITE(ppking_video_registers_w)
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE(MRA8_RAM, gladiatr_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xd800, 0xdfff) AM_READWRITE(MRA8_RAM, gladiatr_videoram_w) AM_BASE(&gladiatr_videoram)
	AM_RANGE(0xe000, 0xe7ff) AM_READWRITE(MRA8_RAM, gladiatr_colorram_w) AM_BASE(&gladiatr_colorram)
	AM_RANGE(0xe800, 0xefff) AM_READWRITE(MRA8_RAM, gladiatr_textram_w) AM_BASE(&gladiatr_textram)
	AM_RANGE(0xf000, 0xf7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) /* battery backed RAM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( ppking_cpu3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x2000, 0x2fff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppking_cpu1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(gladiatr_spritebuffer_w)
	AM_RANGE(0x04, 0x04) AM_NOP	// WRITE(ppking_irq_patch_w)
	AM_RANGE(0x9e, 0x9f) AM_READ(qx0_r) AM_WRITE(qx0_w)
	AM_RANGE(0xbf, 0xbf) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppking_cpu2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READ(YM2203_read_port_0_r) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x20, 0x21) AM_READ(qx1_r) AM_WRITE(qx1_w)
	AM_RANGE(0x40, 0x40) AM_READ(MRA8_NOP)
	AM_RANGE(0x60, 0x61) AM_READ(qx2_r) AM_WRITE(qx2_w)
	AM_RANGE(0x80, 0x81) AM_READ(qx3_r) AM_WRITE(qx3_w)
ADDRESS_MAP_END




static ADDRESS_MAP_START( gladiatr_cpu1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xcbff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xcc00, 0xcfff) AM_WRITE(gladiatr_video_registers_w)
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE(MRA8_RAM, gladiatr_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xd800, 0xdfff) AM_READWRITE(MRA8_RAM, gladiatr_videoram_w) AM_BASE(&gladiatr_videoram)
	AM_RANGE(0xe000, 0xe7ff) AM_READWRITE(MRA8_RAM, gladiatr_colorram_w) AM_BASE(&gladiatr_colorram)
	AM_RANGE(0xe800, 0xefff) AM_READWRITE(MRA8_RAM, gladiatr_textram_w) AM_BASE(&gladiatr_textram)
	AM_RANGE(0xf000, 0xf7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) /* battery backed RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( gladiatr_cpu3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1000, 0x1fff) AM_WRITE(glad_adpcm_w)
	AM_RANGE(0x2000, 0x2fff) AM_READ(glad_cpu_sound_command_r)
	AM_RANGE(0x4000, 0xffff) AM_ROMBANK(2)
ADDRESS_MAP_END


static ADDRESS_MAP_START( gladiatr_cpu1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(gladiatr_spritebuffer_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(gladiatr_spritebank_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(gladiatr_bankswitch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(gladiatr_irq_patch_w) /* !!! patch to 2nd CPU IRQ !!! */
	AM_RANGE(0x07, 0x07) AM_WRITE(gladiatr_flipscreen_w)
	AM_RANGE(0x9e, 0x9f) AM_READWRITE(TAITO8741_0_r, TAITO8741_0_w)
	AM_RANGE(0xbf, 0xbf) AM_NOP	// watchdog_reset_w doesn't work
ADDRESS_MAP_END

static ADDRESS_MAP_START( gladiatr_cpu2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x20, 0x21) AM_READWRITE(TAITO8741_1_r, TAITO8741_1_w)
	AM_RANGE(0x40, 0x40) AM_NOP	// WRITE(sub_irq_ack_w)
	AM_RANGE(0x60, 0x61) AM_READWRITE(TAITO8741_2_r, TAITO8741_2_w)
	AM_RANGE(0x80, 0x81) AM_READWRITE(TAITO8741_3_r, TAITO8741_3_w)
	AM_RANGE(0xa0, 0xa7) AM_NOP	// filters on sound output
	AM_RANGE(0xe0, 0xe0) AM_WRITE(glad_cpu_sound_command_w)
ADDRESS_MAP_END



INPUT_PORTS_START( ppking )

INPUT_PORTS_END


INPUT_PORTS_START( gladiatr )
	PORT_START_TAG("DSW1")		/* (8741-0 parallel port)*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, "After 4 Stages" )
	PORT_DIPSETTING(    0x00, DEF_STR( Continues ) )
	PORT_DIPSETTING(    0x04, "Ends" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )   /*NOTE: Actual manual has these settings reversed(typo?)! */
	PORT_DIPSETTING(    0x08, "Only at 100000" )
	PORT_DIPSETTING(    0x00, "Every 100000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")      /* (8741-1 parallel port) - Dips 6 Unused */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")      /* (YM2203 port B) - Dips 5,6,7 Unused */
	PORT_DIPNAME( 0x01, 0x01, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Memory Backup" )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Clear" )
	PORT_DIPNAME( 0x0c, 0x0c, "Starting Stage" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN0")	/*(8741-3 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START_TAG("COINS")	/*(8741-3 parallel port bit7) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN1")	/* (8741-3 parallel port 2) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START_TAG("IN2")	/* (8741-3 parallel port 4) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START_TAG("IN3")	/* (8741-2 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************************/

static const gfx_layout charlayout  =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tilelayout  =
{
	8,8,
	RGN_FRAC(1,2),
	3,
	{ 4, RGN_FRAC(1,2), RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout spritelayout  =
{
	16,16,
	RGN_FRAC(1,2),
	3,
	{ 4, RGN_FRAC(1,2), RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_decode ppking_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 1 },
	{ REGION_GFX2, 0, &tilelayout, 0, 32 },
	{ REGION_GFX3, 0, &spritelayout, 0x100, 32 },
	{ -1 }
};

static const gfx_decode gladiatr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x200, 1 },
	{ REGION_GFX2, 0, &tilelayout,   0x000, 32 },
	{ REGION_GFX3, 0, &spritelayout, 0x100, 32 },
	{ -1 } /* end of array */
};



static READ8_HANDLER(f1_r)
{
	return rand();
}

static struct YM2203interface ppking_ym2203_interface =
{
	f1_r,
	f1_r
};

static struct YM2203interface gladiatr_ym2203_interface =
{
	0,
	gladiator_dsw3_r,         /* port B read */
	gladiator_int_control_w, /* port A write */
	0,
	gladiator_ym_irq          /* NMI request for 2nd cpu */
};

static struct MSM5205interface msm5205_interface =
{
	0,				/* interrupt function */
	MSM5205_SEX_4B	/* vclk input mode    */
};



static MACHINE_DRIVER_START( ppking )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 12000000/2) /* 6 MHz */
	MDRV_CPU_PROGRAM_MAP(ppking_cpu1_map,0)
	MDRV_CPU_IO_MAP(ppking_cpu1_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 12000000/4) /* 3 MHz */
	MDRV_CPU_PROGRAM_MAP(cpu2_map,0)
	MDRV_CPU_IO_MAP(ppking_cpu2_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6809, 12000000/16) /* 750 kHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(ppking_cpu3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(ppking_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(ppking)
	MDRV_VIDEO_UPDATE(ppking)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 12000000/8)
	MDRV_SOUND_CONFIG(ppking_ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)
	MDRV_SOUND_ROUTE(1, "mono", 0.60)
	MDRV_SOUND_ROUTE(2, "mono", 0.60)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 12000000/32)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gladiatr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 12000000/2) /* 6 MHz */
	MDRV_CPU_PROGRAM_MAP(gladiatr_cpu1_map,0)
	MDRV_CPU_IO_MAP(gladiatr_cpu1_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 12000000/4) /* 3 MHz */
	MDRV_CPU_PROGRAM_MAP(cpu2_map,0)
	MDRV_CPU_IO_MAP(gladiatr_cpu2_io,0)

	MDRV_CPU_ADD(M6809, 12000000/16) /* 750 kHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(gladiatr_cpu3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_RESET(gladiator)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gladiatr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(gladiatr)
	MDRV_VIDEO_UPDATE(gladiatr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 12000000/8)
	MDRV_SOUND_CONFIG(gladiatr_ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.60)
	MDRV_SOUND_ROUTE(1, "mono", 0.60)
	MDRV_SOUND_ROUTE(2, "mono", 0.60)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 12000000/32)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ppking )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "q1_1.a1",        0x00000, 0x2000, CRC(b74b2718) SHA1(29833439211076873324ccfa5897eb1e6aa9d134) )
	ROM_LOAD( "q1_2.b1",        0x02000, 0x2000, CRC(1b1e4cd4) SHA1(34c6cf5e0775c0c834dda34a3a2a4685465daa8e) )
	ROM_LOAD( "q0_3.c1",        0x04000, 0x2000, CRC(6a7acf8e) SHA1(06d37e813605f507ea1c720764fc554e58defdf8) )
	ROM_LOAD( "q0_4.d1",        0x06000, 0x2000, CRC(b83eb6d5) SHA1(f112d3c0d701977dcc5c312ad74d78b44882201b) )
	ROM_LOAD( "q0_5.e1",        0x08000, 0x4000, CRC(4d2007e2) SHA1(973ef0e6ff6065b753402489a3d10a9b68164969) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "q0_17.6f",       0x0000, 0x2000, CRC(f7fe0d24) SHA1(6dcb23aa7fc08fc892a8b3843ccb982997c20571) )
	ROM_LOAD( "q0_16.6e",       0x4000, 0x2000, CRC(b1e32588) SHA1(13c74479238a34a08e249f9120b42a52d80f8274) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "q0_19.5n",       0x0c000, 0x2000, CRC(4bcf896d) SHA1(f587a66fcc63e989742ce2d5f4cf2bb464987038) )
	ROM_LOAD( "q0_18.5m",       0x0e000, 0x2000, CRC(89ba64f8) SHA1(fa01316ea744b4277ee64d5f14cb6d7e3a949f2b) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_15.1r",       0x00000, 0x2000, CRC(fbd33219) SHA1(78b9bb327ededaa818d26c41c5e8fd1c041ef142) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_12.1j",       0x00000, 0x2000, CRC(b1a44482) SHA1(84cc40976aa9b015a9f970a878bbde753651b3ba) )	/* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "q0_13.1k",       0x04000, 0x2000, CRC(468f35e6) SHA1(8e28481910663fe525cefd4ad406468b7736900e) ) /* planes 1,2 */
	ROM_LOAD( "q0_14.1m",       0x06000, 0x2000, CRC(eed04a7f) SHA1(d139920889653c33ded38a85510789380dd0aa9e) ) /* planes 1,2 */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "q0_6.k1",        0x00000, 0x2000, CRC(bb3d666c) SHA1(a689c7a1e75b916d69f396db7c4688ac355c2aff) )	/* plane 3 */
	ROM_LOAD( "q0_7.l1",        0x02000, 0x2000, CRC(16a2550e) SHA1(adb54b70a6db660b5f29ad66da02afd8e99884bb) )	/* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "q0_8.m1",        0x08000, 0x2000, CRC(41235b22) SHA1(4d9702efe0ea320dab7c0d889f4d03f196b32661) ) /* planes 1,2 */
	ROM_LOAD( "q0_9.p1",        0x0a000, 0x2000, CRC(74cc94b2) SHA1(2cb981ecb2487dfa5c0974e036106fc06c2c1880) ) /* planes 1,2 */
	ROM_LOAD( "q0_10.r1",       0x0c000, 0x2000, CRC(af438cc6) SHA1(cf79c8d3f2a0c489a756b341f8df747f6654764b) ) /* planes 1,2 */
	/* empty! */

	ROM_REGION( 0x040, REGION_PROMS, 0 )
	ROM_LOAD( "q1",             0x0000, 0x0020, CRC(cca9ae7b) SHA1(e18416fbe2a5b09db749cc9a14fa89186ed8f4ba) )
	ROM_LOAD( "q2",             0x0020, 0x0020, CRC(da952b5e) SHA1(0863ad8fdcf69435a7455cd345d3d0af0b0c0a0a) )
ROM_END


ROM_START( gladiatr )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "qb0-5",          0x00000, 0x4000, CRC(25b19efb) SHA1(c41344278f6c7f3d6527aced3e459ed1ba86dea5) )
	ROM_LOAD( "qb0-4",          0x04000, 0x2000, CRC(347ec794) SHA1(51100f9fef2e96f00e94fce709eed6583b01a2eb) )
	ROM_LOAD( "qb0-1",          0x10000, 0x2000, CRC(040c9839) SHA1(8c0d9a246847461a59eb5e6a53a94218e701d6c3) )
	ROM_CONTINUE(               0x16000, 0x2000 )
	ROM_LOAD( "qc0-3",          0x12000, 0x4000, CRC(8d182326) SHA1(f0af3757c2cf9e1e8035272567adee6efc733319) )
	ROM_CONTINUE(               0x18000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, CRC(e78be010) SHA1(157231d858d13a006b57a4ab419368168e64edb7) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )  /* 6809 Code & ADPCM data */
	ROM_LOAD( "qb0-20",         0x10000, 0x4000, CRC(15916eda) SHA1(6558bd2ae6f14d630ae93e66ce7d09be33870cce) )
	ROM_CONTINUE(               0x1c000, 0x4000 )
	ROM_LOAD( "qb0-19",         0x14000, 0x4000, CRC(79caa7ed) SHA1(57adc8429ad016c4da41deda6b7b6fe36de5a225) )
	ROM_CONTINUE(               0x20000, 0x4000 )
	ROM_LOAD( "qb0-18",         0x18000, 0x4000, CRC(e9591260) SHA1(e427aa10c683fbeb98171f6d1820781d21075a24) )
	ROM_CONTINUE(               0x24000, 0x4000 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qc0-15",       	0x00000, 0x2000, CRC(a7efa340) SHA1(f87e061b8e4d8cd0834fab301779a8493549419b) ) /* (monochrome) */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "qb0-12",       	0x00000, 0x8000, CRC(0585d9ac) SHA1(e3cb07e9dc5ec2fcfa0c90294d32f0b751f67752) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qb0-13",       	0x10000, 0x8000, CRC(a6bb797b) SHA1(852e9993270e5557c1a0350007d0beaec5ca6286) ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x18000, 0x8000, CRC(85b71211) SHA1(81545cd168da4a707e263fdf0ee9902e3a13ba93) ) /* planes 1,2 */

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "qc1-6",        	0x00000, 0x4000, CRC(651e6e44) SHA1(78ce576e6c29e43d590c42f0d4926cff82fd0268) ) /* plane 3 */
	ROM_LOAD( "qc2-7",        	0x04000, 0x8000, CRC(c992c4f7) SHA1(3263973474af07c8b93c4ec97924568848cb7201) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qc0-8",        	0x18000, 0x4000, CRC(1c7ffdad) SHA1(b224fd4cce078186f22e6393a38c7a2d84dc0066) ) /* planes 1,2 */
	ROM_LOAD( "qc1-9",        	0x1c000, 0x4000, CRC(01043e03) SHA1(6a6dddc0a036873135dceaa989e757bdd2455ae7) ) /* planes 1,2 */
	ROM_LOAD( "qc1-10",       	0x20000, 0x8000, CRC(364cdb58) SHA1(4d8548f9dfa9d105dd277c61cf3d56583a5ebbcb) ) /* planes 1,2 */
	ROM_LOAD( "qc2-11",       	0x28000, 0x8000, CRC(c9fecfff) SHA1(7c13ace4293fbfab7fe924b7b24c498d8cefc7ac) ) /* planes 1,2 */

	ROM_REGION( 0x00040, REGION_PROMS, 0 )	/* unused */
	ROM_LOAD( "q3.2b",          0x00000, 0x0020, CRC(6a7c3c60) SHA1(5125bfeb03752c8d76b140a4e74d5cac29dcdaa6) )	/* address decoding */
	ROM_LOAD( "q4.5s",          0x00020, 0x0020, CRC(e325808e) SHA1(5fd92ad4eff24f6ccf2df19d268a6cafba72202e) )
ROM_END

ROM_START( ogonsiro )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "qb0-5",          0x00000, 0x4000, CRC(25b19efb) SHA1(c41344278f6c7f3d6527aced3e459ed1ba86dea5) )
	ROM_LOAD( "qb0-4",          0x04000, 0x2000, CRC(347ec794) SHA1(51100f9fef2e96f00e94fce709eed6583b01a2eb) )
	ROM_LOAD( "qb0-1",          0x10000, 0x2000, CRC(040c9839) SHA1(8c0d9a246847461a59eb5e6a53a94218e701d6c3) )
	ROM_CONTINUE(               0x16000, 0x2000 )
	ROM_LOAD( "qb0_3",          0x12000, 0x4000, CRC(d6a342e7) SHA1(96274ae3bda4679108a25fcc514b625552abda30) )
	ROM_CONTINUE(               0x18000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, CRC(e78be010) SHA1(157231d858d13a006b57a4ab419368168e64edb7) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )  /* 6809 Code & ADPCM data */
	ROM_LOAD( "qb0-20",         0x10000, 0x4000, CRC(15916eda) SHA1(6558bd2ae6f14d630ae93e66ce7d09be33870cce) )
	ROM_CONTINUE(               0x1c000, 0x4000 )
	ROM_LOAD( "qb0-19",         0x14000, 0x4000, CRC(79caa7ed) SHA1(57adc8429ad016c4da41deda6b7b6fe36de5a225) )
	ROM_CONTINUE(               0x20000, 0x4000 )
	ROM_LOAD( "qb0-18",         0x18000, 0x4000, CRC(e9591260) SHA1(e427aa10c683fbeb98171f6d1820781d21075a24) )
	ROM_CONTINUE(               0x24000, 0x4000 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qb0_15",       	0x00000, 0x2000, CRC(5e1332b8) SHA1(fab6e2c7ea9bc94c1245bf759b4004a70c57d666) ) /* (monochrome) */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "qb0-12",       	0x00000, 0x8000, CRC(0585d9ac) SHA1(e3cb07e9dc5ec2fcfa0c90294d32f0b751f67752) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qb0-13",       	0x10000, 0x8000, CRC(a6bb797b) SHA1(852e9993270e5557c1a0350007d0beaec5ca6286) ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x18000, 0x8000, CRC(85b71211) SHA1(81545cd168da4a707e263fdf0ee9902e3a13ba93) ) /* planes 1,2 */

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "qb0_6",        	0x00000, 0x4000, CRC(1a2bc769) SHA1(498861f4d0cffeaff90609c8000c921a114756b6) ) /* plane 3 */
	ROM_LOAD( "qb0_7",        	0x04000, 0x8000, CRC(4b677bd9) SHA1(3314ef58ff5307faf0ecd8f99950d43d571c91a6) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qc0-8",        	0x18000, 0x4000, CRC(1c7ffdad) SHA1(b224fd4cce078186f22e6393a38c7a2d84dc0066) ) /* planes 1,2 */
	ROM_LOAD( "qb0_9",        	0x1c000, 0x4000, CRC(38f5152d) SHA1(fbb7b13a625999807d180a3212e6e12870629438) ) /* planes 1,2 */
	ROM_LOAD( "qb0_10",       	0x20000, 0x8000, CRC(87ab6cc4) SHA1(50bc1108ff5609c0e7dad615e92e16eb72b7bc03) ) /* planes 1,2 */
	ROM_LOAD( "qb0_11",       	0x28000, 0x8000, CRC(25eaa4ff) SHA1(3547fc600a617ba7fe5240a7830edb90230b6c51) ) /* planes 1,2 */

	ROM_REGION( 0x00040, REGION_PROMS, 0 ) /* unused */
	ROM_LOAD( "q3.2b",          0x00000, 0x0020, CRC(6a7c3c60) SHA1(5125bfeb03752c8d76b140a4e74d5cac29dcdaa6) )	/* address decoding */
	ROM_LOAD( "q4.5s",          0x00020, 0x0020, CRC(e325808e) SHA1(5fd92ad4eff24f6ccf2df19d268a6cafba72202e) )
ROM_END

ROM_START( greatgur )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "qb0-5",          0x00000, 0x4000, CRC(25b19efb) SHA1(c41344278f6c7f3d6527aced3e459ed1ba86dea5) )
	ROM_LOAD( "qb0-4",          0x04000, 0x2000, CRC(347ec794) SHA1(51100f9fef2e96f00e94fce709eed6583b01a2eb) )
	ROM_LOAD( "qb0-1",          0x10000, 0x2000, CRC(040c9839) SHA1(8c0d9a246847461a59eb5e6a53a94218e701d6c3) )
	ROM_CONTINUE(               0x16000, 0x2000 )
	ROM_LOAD( "qb0_3",          0x12000, 0x4000, CRC(d6a342e7) SHA1(96274ae3bda4679108a25fcc514b625552abda30) )
	ROM_CONTINUE(               0x18000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",         0x0000, 0x4000, CRC(e78be010) SHA1(157231d858d13a006b57a4ab419368168e64edb7) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )  /* 6809 Code & ADPCM data */
	ROM_LOAD( "qb0-20",         0x10000, 0x4000, CRC(15916eda) SHA1(6558bd2ae6f14d630ae93e66ce7d09be33870cce) )
	ROM_CONTINUE(               0x1c000, 0x4000 )
	ROM_LOAD( "qb0-19",         0x14000, 0x4000, CRC(79caa7ed) SHA1(57adc8429ad016c4da41deda6b7b6fe36de5a225) )
	ROM_CONTINUE(               0x20000, 0x4000 )
	ROM_LOAD( "qb0-18",         0x18000, 0x4000, CRC(e9591260) SHA1(e427aa10c683fbeb98171f6d1820781d21075a24) )
	ROM_CONTINUE(               0x24000, 0x4000 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qb0_15",         0x00000, 0x2000, CRC(5e1332b8) SHA1(fab6e2c7ea9bc94c1245bf759b4004a70c57d666) ) /* (monochrome) */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "qb0-12",       	0x00000, 0x8000, CRC(0585d9ac) SHA1(e3cb07e9dc5ec2fcfa0c90294d32f0b751f67752) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qb0-13",       	0x10000, 0x8000, CRC(a6bb797b) SHA1(852e9993270e5557c1a0350007d0beaec5ca6286) ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x18000, 0x8000, CRC(85b71211) SHA1(81545cd168da4a707e263fdf0ee9902e3a13ba93) ) /* planes 1,2 */

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "qc0-06.bin",     0x00000, 0x4000, CRC(96b20201) SHA1(212270d3ba72974f22e96744c752860cc5ffba5b) ) /* plane 3 */
	ROM_LOAD( "qc0-07.bin",     0x04000, 0x8000, CRC(9e89fa8f) SHA1(b133ae2ac62f43a7a51fa0d1a023a4f95fef2996) ) /* plane 3 */
	/* space to unpack plane 3 */
	ROM_LOAD( "qc0-8",        	0x18000, 0x4000, CRC(1c7ffdad) SHA1(b224fd4cce078186f22e6393a38c7a2d84dc0066) ) /* planes 1,2 */
	ROM_LOAD( "qc0-09.bin",     0x1c000, 0x4000, CRC(204cd385) SHA1(e7a8720feeac8ced581d72190345daed5750379f) ) /* planes 1,2 */
	ROM_LOAD( "qc1-10",         0x20000, 0x8000, CRC(364cdb58) SHA1(4d8548f9dfa9d105dd277c61cf3d56583a5ebbcb) ) /* planes 1,2 */
	ROM_LOAD( "qc1-11.bin",     0x28000, 0x8000, CRC(b2aabbf5) SHA1(9eb4d80f38a30f6e45231a9bfd1aff7a124c6ee9) ) /* planes 1,2 */

	ROM_REGION( 0x00040, REGION_PROMS, 0 ) /* unused */
	ROM_LOAD( "q3.2b",          0x00000, 0x0020, CRC(6a7c3c60) SHA1(5125bfeb03752c8d76b140a4e74d5cac29dcdaa6) )	/* address decoding */
	ROM_LOAD( "q4.5s",          0x00020, 0x0020, CRC(e325808e) SHA1(5fd92ad4eff24f6ccf2df19d268a6cafba72202e) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* ROMs for the four 8741 (not emulated yet) */
	ROM_LOAD( "gladcctl.1",     0x00000, 0x0400, CRC(b30d225f) SHA1(f383286530975c440589c276aa8c46fdfe5292b6) )
	ROM_LOAD( "gladccpu.2",     0x00000, 0x0400, CRC(1d02cd5f) SHA1(f7242039788c66a1d91b01852d7d447330b847c4) )
	ROM_LOAD( "gladucpu.17",    0x00000, 0x0400, CRC(3c5ca4c6) SHA1(0d8c2e1c2142ada11e30cfb9a48663386fee9cb8) )
	ROM_LOAD( "gladcsnd.18",    0x00000, 0x0400, CRC(3c5ca4c6) SHA1(0d8c2e1c2142ada11e30cfb9a48663386fee9cb8) )
ROM_END


static void swap_block(UINT8 *src1,UINT8 *src2,int len)
{
	int i,t;

	for (i = 0;i < len;i++)
	{
		t = src1[i];
		src1[i] = src2[i];
		src2[i] = t;
	}
}

static DRIVER_INIT( gladiatr )
{
	UINT8 *rom;
	int i,j;

	rom = memory_region(REGION_GFX2);
	// unpack 3bpp graphics
	for (j = 3; j >= 0; j--)
	{
		for (i = 0; i < 0x2000; i++)
		{
			rom[i+(2*j+1)*0x2000] = rom[i+j*0x2000] >> 4;
			rom[i+2*j*0x2000] = rom[i+j*0x2000];
		}
	}
	// sort data
	swap_block(rom + 0x14000, rom + 0x18000, 0x4000);


	rom = memory_region(REGION_GFX3);
	// unpack 3bpp graphics
	for (j = 5; j >= 0; j--)
	{
		for (i = 0; i < 0x2000; i++)
		{
			rom[i+(2*j+1)*0x2000] = rom[i+j*0x2000] >> 4;
			rom[i+2*j*0x2000] = rom[i+j*0x2000];
		}
	}
	// sort data
	swap_block(rom + 0x1a000, rom + 0x1c000, 0x2000);
	swap_block(rom + 0x22000, rom + 0x28000, 0x2000);
	swap_block(rom + 0x26000, rom + 0x2c000, 0x2000);
	swap_block(rom + 0x24000, rom + 0x28000, 0x4000);
}


static READ8_HANDLER(f6a3_r)
{
	if(activecpu_get_previouspc()==0x8e)
		generic_nvram[0x6a3]=1;

	return generic_nvram[0x6a3];
}

static DRIVER_INIT(ppking)
{
	UINT8 *rom;
	int i,j;

	rom = memory_region(REGION_GFX2);
	// unpack 3bpp graphics
	for (i = 0; i < 0x2000; i++)
	{
		rom[i+0x2000] = rom[i] >> 4;
	}

	rom = memory_region(REGION_GFX3);
	// unpack 3bpp graphics
	for (j = 1; j >= 0; j--)
	{
		for (i = 0; i < 0x2000; i++)
		{
			rom[i+(2*j+1)*0x2000] = rom[i+j*0x2000] >> 4;
			rom[i+2*j*0x2000] = rom[i+j*0x2000];
		}
	}

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf6a3,0xf6a3,0,0, f6a3_r );
}



GAME( 1985, ppking,   0,        ppking,   ppking,   ppking,   ROT90, "Taito America Corporation", "Ping-Pong King", GAME_NOT_WORKING)
GAME( 1986, gladiatr, 0,        gladiatr, gladiatr, gladiatr, ROT0,  "Taito America Corporation", "Gladiator (US)", 0 )
GAME( 1986, ogonsiro, gladiatr, gladiatr, gladiatr, gladiatr, ROT0,  "Taito Corporation", "Ohgon no Siro (Japan)", 0 )
GAME( 1986, greatgur, gladiatr, gladiatr, gladiatr, gladiatr, ROT0,  "Taito Corporation", "Great Gurianos (Japan?)", 0 )
