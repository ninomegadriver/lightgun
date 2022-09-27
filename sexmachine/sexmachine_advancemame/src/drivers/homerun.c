/*
 Moero Pro Yakyuu Homerun - (c) 1988 Jaleco
 Driver by Tomasz Slanina

 *weird* hardware - based on NES version
 (gfx bank changed in the middle of screen,
  sprites in NES format etc)

Todo :
 - voice ( unemulated D7756C )
 - controls/dips
 - better emulation of gfx bank switching
 - is there 2 player mode ?

*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/2203intf.h"

extern int homerun_gc_up;
extern int homerun_gc_down;
extern int homerun_xpa,homerun_xpb,homerun_xpc;
extern UINT8 *homerun_videoram;

WRITE8_HANDLER( homerun_videoram_w );
WRITE8_HANDLER( homerun_color_w );
WRITE8_HANDLER( homerun_banking_w );
VIDEO_START(homerun);
VIDEO_UPDATE(homerun);

static WRITE8_HANDLER(pa_w){homerun_xpa=data;}
static WRITE8_HANDLER(pb_w){homerun_xpb=data;}
static WRITE8_HANDLER(pc_w){homerun_xpc=data;}

static ppi8255_interface ppi8255_intf =
{
	1,
	{ 0 },
	{ 0  },
	{ 0  },
	{ pa_w },
	{ pb_w },
	{ pc_w },
};


MACHINE_RESET( homerun )
{
	ppi8255_init(&ppi8255_intf);
}

static const gfx_layout gfxlayout =
{
   8,8,
   RGN_FRAC(1,1),
   2,
   { 8*8,0},
   { 0, 1, 2, 3, 4, 5, 6, 7},
   { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
   8*8*2
};



static const gfx_layout spritelayout =
{
   16,16,
   RGN_FRAC(1,1),
   2,
   { 8*8,0},
   { 0, 1, 2, 3, 4, 5, 6, 7,0+8*8*2,1+8*8*2,2+8*8*2,3+8*8*2,4+8*8*2,5+8*8*2,6+8*8*2,7+8*8*2},
   { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 0*8+2*8*8*2,1*8+2*8*8*2,2*8+2*8*8*2,3*8+2*8*8*2,4*8+2*8*8*2,5*8+2*8*8*2,6*8+2*8*8*2,7*8+2*8*8*2},
   8*8*2*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout,   0, 16 },
	{ REGION_GFX2, 0, &spritelayout,   0, 16 },
	{ -1 }
};

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)

ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(homerun_videoram_w) AM_BASE(&homerun_videoram)
	AM_RANGE(0xa000, 0xa0ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xb000, 0xb0ff) AM_WRITE(homerun_color_w)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)

ADDRESS_MAP_END


static READ8_HANDLER(homerun_40_r)
{
	if(cpu_getscanline()>116)
		return input_port_0_r(0)|0x40;
	else
		return input_port_0_r(0);
}


static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x30, 0x33) AM_READ(ppi8255_0_r)
	AM_RANGE(0x40, 0x40) AM_READ(homerun_40_r)
	AM_RANGE(0x50, 0x50) AM_READ(input_port_2_r)
	AM_RANGE(0x60, 0x60) AM_READ(input_port_1_r)
	AM_RANGE(0x70, 0x70) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x71, 0x71) AM_READ(YM2203_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(MWA8_NOP) /* ?? */
	AM_RANGE(0x20, 0x20) AM_WRITE(MWA8_NOP) /* ?? */
	AM_RANGE(0x30, 0x33) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x70, 0x70) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x71, 0x71) AM_WRITE(YM2203_write_port_0_w)
ADDRESS_MAP_END

static struct YM2203interface ym2203_interface =
{
	input_port_3_r,
	0,
	0,
	homerun_banking_w
};


INPUT_PORTS_START( homerun )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1   )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x00, DEF_STR( 1C_2C ) )

INPUT_PORTS_END

static MACHINE_DRIVER_START( homerun )
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_RESET(homerun)

	/* video hardware */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-25)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*4)

	MDRV_VIDEO_START(homerun)
	MDRV_VIDEO_UPDATE(homerun)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 6000000/2)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

MACHINE_DRIVER_END

ROM_START( homerun )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "homerun.43",        0x0000, 0x4000, CRC(e759e476) SHA1(ad4f356ff26209033320a3e6353e4d4d9beb59c1) )
	ROM_CONTINUE(        0x10000,0x1c000)

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "homerun.60",  0x00000, 0x10000, CRC(69a720d1) SHA1(0f0a4877578f358e9e829ece8c31e23f01adcf83) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "homerun.120",  0x00000, 0x20000, CRC(52f0709b) SHA1(19e675bcccadb774f60ec5929fc1fb5cf0d3f617) )

	ROM_REGION( 0x01000, REGION_SOUND1, 0 )
	ROM_LOAD( "homerun.snd",  0x00000, 0x1000, NO_DUMP ) /* D7756C internal rom */

ROM_END

GAME( 1988, homerun, 0, homerun, homerun, 0, ROT0, "Jaleco", "Moero Pro Yakyuu Homerun",GAME_IMPERFECT_GRAPHICS)

