/****************************************************************************

Tug Boat
6502 hooked up + preliminary video by MooglyGuy

TODO:
- verify connections of the two PIAs. I only hooked up a couple of ports but
  there are more.
- check how the score is displayed. I'm quite sure that tugboat_score_w is
  supposed to access videoram scanning it by columns (like btime_mirrorvideoram_w),
  but the current implementation is a big kludge, and it still looks wrong.
- colors might not be entirely accurate

the problem which caused the controls not to work
---
There's counter at $000b, counting up from $ff to 0 or from $fe to 0 (initial value depends
on game level). It's increased in main loop, and used for game flow control (scrolling speed , controls  etc).
Every interrupt, when (counter&3)!=0 , there's a check for left/right inputs .
But when init val was $ff (2nd level),  the condition 'counter&3!=0' was
always false - counter was reloaded and incremented before interrupt occurs

****************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "sound/ay8910.h"


UINT8 *tugboat_ram,*tugboat_score;


static UINT8 hd46505_0_reg[18],hd46505_1_reg[18];


/*  there isn't the usual resistor array anywhere near the color prom,
    just four 1k resistors. */
PALETTE_INIT( tugboat )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r,g,b,brt;


		brt = ((color_prom[i] >> 3) & 0x01) ? 0xff : 0x80;

		r = brt * ((color_prom[i] >> 0) & 0x01);
		g = brt * ((color_prom[i] >> 1) & 0x01);
		b = brt * ((color_prom[i] >> 2) & 0x01);

		palette_set_color(i,r,g,b);
	}
}



/* see crtc6845.c. That file is only a placeholder, I process the writes here
   because I need the start_addr register to handle scrolling */
static WRITE8_HANDLER( tugboat_hd46505_0_w )
{
	static int reg;
	if (offset == 0) reg = data & 0x0f;
	else if (reg < 18) hd46505_0_reg[reg] = data;
}
static WRITE8_HANDLER( tugboat_hd46505_1_w )
{
	static int reg;
	if (offset == 0) reg = data & 0x0f;
	else if (reg < 18) hd46505_1_reg[reg] = data;
}


static WRITE8_HANDLER( tugboat_score_w )
{
      if (offset>=0x8) tugboat_ram[0x291d + 32*offset + 32*(1-8)] = data ^ 0x0f;
      if (offset<0x8 ) tugboat_ram[0x291d + 32*offset + 32*9] = data ^ 0x0f;
}

static void draw_tilemap(mame_bitmap *bitmap,const rectangle *cliprect,
		int addr,int gfx0,int gfx1,int transparency)
{
	int x,y;

	for (y = 0;y < 32;y++)
	{
		for (x = 0;x < 32;x++)
		{
			int code = (tugboat_ram[addr + 0x400] << 8) | tugboat_ram[addr];
			int color = (code & 0x3c00) >> 10;
			int rgn;

			code &=0x3ff;
			rgn = gfx0;

			if (code > 0x1ff)
			{
				code &= 0x1ff;
				rgn = gfx1;
			}

			drawgfx(bitmap,Machine->gfx[rgn],
					code,
					color,
					0,0,
					8*x,8*y,
					cliprect,transparency,7);

			addr = (addr & 0xfc00) | ((addr + 1) & 0x03ff);
		}
	}
}

VIDEO_UPDATE( tugboat )
{
	int startaddr0 = hd46505_0_reg[0x0c]*256 + hd46505_0_reg[0x0d];
	int startaddr1 = hd46505_1_reg[0x0c]*256 + hd46505_1_reg[0x0d];


	draw_tilemap(bitmap,cliprect,startaddr0,0,1,TRANSPARENCY_NONE);
	draw_tilemap(bitmap,cliprect,startaddr1,2,3,TRANSPARENCY_PEN);
}


static int ctrl;

static READ8_HANDLER( tugboat_input_r )
{
	if (~ctrl & 0x80)
		return readinputport(0);
	else if (~ctrl & 0x40)
		return readinputport(1);
	else if (~ctrl & 0x20)
		return readinputport(2);
	else if (~ctrl & 0x10)
		return readinputport(3);
	else
		return readinputport(4);
}

static READ8_HANDLER( tugboat_ctrl_r )
{
	return ctrl;
}

static WRITE8_HANDLER( tugboat_ctrl_w )
{
	ctrl = data;
}

static const pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ tugboat_input_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

static const pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_5_r, tugboat_ctrl_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0,              tugboat_ctrl_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static void interrupt_gen(int scanline)
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
	timer_set(cpu_getscanlinetime(1), 0, interrupt_gen);
}

MACHINE_START( tugboat )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
	return 0;
}

MACHINE_RESET( tugboat )
{
	pia_reset();
	timer_set(cpu_getscanlinetime(1), 0, interrupt_gen);
}


static ADDRESS_MAP_START( tugboat_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x11e4, 0x11e7) AM_READ(pia_0_r)
	AM_RANGE(0x11e8, 0x11eb) AM_READ(pia_1_r)
	//AM_RANGE(0x1700, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xfff0, 0xffff) AM_READ(MRA8_ROM)	/* vectors */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tugboat_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_WRITE(MWA8_RAM) AM_BASE(&tugboat_ram)
	AM_RANGE(0x1060, 0x1060) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x1061, 0x1061) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x10a0, 0x10a1) AM_WRITE(tugboat_hd46505_0_w)	// scrolling is performed changing the start_addr register (0C/0D)
	AM_RANGE(0x10c0, 0x10c1) AM_WRITE(tugboat_hd46505_1_w)
	AM_RANGE(0x11e4, 0x11e7) AM_WRITE(pia_0_w)
	AM_RANGE(0x11e8, 0x11eb) AM_WRITE(pia_1_w)
	//AM_RANGE(0x1700, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x18e0, 0x18ef) AM_WRITE(tugboat_score_w)
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_RAM)	/* tilemap RAM */
    AM_RANGE(0x5000, 0x7fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


INPUT_PORTS_START( tugboat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( noahsark )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x80, 16 },
	{ REGION_GFX2, 0, &charlayout, 0x80, 16 },
	{ REGION_GFX3, 0, &charlayout, 0x00, 16 },
	{ REGION_GFX4, 0, &charlayout, 0x00, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( tugboat )
	MDRV_CPU_ADD_TAG("main", M6502, 2000000)	/* 2 MHz ???? */
	MDRV_CPU_PROGRAM_MAP(tugboat_readmem,tugboat_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(tugboat)
	MDRV_MACHINE_RESET(tugboat)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8,64*8)
	MDRV_VISIBLE_AREA(1*8,31*8-1,2*8,30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(tugboat)
	MDRV_VIDEO_UPDATE(tugboat)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.35)
MACHINE_DRIVER_END


ROM_START( tugboat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u7.bin", 0x5000, 0x1000, CRC(e81d7581) SHA1(c76327e3b027a5a2af69f8cfafa1f828ad0ebdb1) )
	ROM_LOAD( "u8.bin", 0x6000, 0x1000, CRC(7525de06) SHA1(0722c7a0b89c55162227173679ffbe398ca350a2) )
	ROM_LOAD( "u9.bin", 0x7000, 0x1000, CRC(aa4ae687) SHA1(a212eed5d04d6197aa3484ff36059fd7998604a6) )
	ROM_RELOAD(         0xf000, 0x1000 )	/* for the vectors */

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u67.bin",  0x0000, 0x0800, CRC(601c425b) SHA1(13ed54ba1307ba3f779293d88c19d0c0f2d91a96) )
	ROM_FILL(             0x0800, 0x0800, 0xff )
	ROM_FILL(             0x1000, 0x0800, 0xff )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u68.bin", 0x0000, 0x1000, CRC(d5835182) SHA1(f67c8f93e0d7dd1bf8e3a98756719d386c133d1c) )
	ROM_LOAD( "u69.bin", 0x1000, 0x1000, CRC(e6d25878) SHA1(de9096ef3108d031049be1e7f2c5e346d0bc0df1) )
	ROM_LOAD( "u70.bin", 0x2000, 0x1000, CRC(34ce2850) SHA1(8883126627ed8a1d2c3bed2a3d169ce35eafc8a3) )

	ROM_REGION( 0x1800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u168.bin", 0x0000, 0x0800, CRC(279042fd) SHA1(1361fff1bc532251bbd36b7b60776c2cc137cfba) )	/* labeled u-167 */
	ROM_RELOAD(         0x0800, 0x0800 )
	ROM_RELOAD(         0x1000, 0x0800 )

	ROM_REGION( 0x1800, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u170.bin", 0x0000, 0x0800, CRC(64d9f4d7) SHA1(3ff7fc099023512c33ec4583e91e6cbab903e7a8) )	/* labeled u-168 */
	ROM_LOAD( "u169.bin", 0x0800, 0x0800, CRC(1a636296) SHA1(bcb18d714328ba3db2d16d74c47a985c16a0bbe2) )	/* labeled u-169 */
	ROM_LOAD( "u167.bin", 0x1000, 0x0800, CRC(b9c9b4f7) SHA1(6685d580ae150d7c67bac2786ee4b7a2c28eddc3) )	/* labeled u-170 */

	ROM_REGION( 0x0100, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "nt2_u128.clr", 0x0000, 0x0100, CRC(236672bf) SHA1(57482d0a23223ef7b211045ad28d3e41e90f961e) )
ROM_END


ROM_START( noahsark )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u6.bin", 0x4000, 0x1000, CRC(3579eeac) SHA1(f54435ac6b31cf81342de83965cf8a8503b26eb8) )
	ROM_LOAD( "u7.bin", 0x5000, 0x1000, CRC(64b0afae) SHA1(1fcc17490d1290565be38a817f783604bcefb8be) )
	ROM_LOAD( "u8.bin", 0x6000, 0x1000, CRC(02d53f62) SHA1(e51a583a548b4bdaf43d376d5d276325ee448d49) )
	ROM_LOAD( "u9.bin", 0x7000, 0x1000, CRC(d425b61c) SHA1(a8d9562435cc910916df4cd7e958468d88ff92e7) )
	ROM_RELOAD(         0xf000, 0x1000 )	/* for the vectors */

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u67.bin",  0x0000, 0x0800, CRC(1a77605b) SHA1(8c25750f94895f5820ad4f1fa4ae1ea70ee0aee2) )
	ROM_FILL(             0x0800, 0x0800, 0xff )
	ROM_FILL(             0x1000, 0x0800, 0xff )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u68.bin", 0x0000, 0x1000, CRC(6a66eac8) SHA1(3a13c2f5ef45cdd8b8b5db07d8c1417a3304723a) )
	ROM_LOAD( "u69.bin", 0x1000, 0x1000, CRC(fa2c279c) SHA1(332fcfcfe605c4132114399c32932507b16752e5) )
	ROM_LOAD( "u70.bin", 0x2000, 0x1000, CRC(dcabc7c5) SHA1(68abfdedea518e3a5c90f9f72173e8c05e190535) )

	ROM_REGION( 0x1800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u168.bin", 0x0000, 0x0800, CRC(7fc7280f) SHA1(93bf46e421b580edf81177db85cb220073761c57) )	/* labeled u-167 */
	ROM_RELOAD(         0x0800, 0x0800 )
	ROM_RELOAD(         0x1000, 0x0800 )

	ROM_REGION( 0x3000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u170.bin", 0x0000, 0x1000, CRC(ba36641c) SHA1(df206dc4b6f2da7b60bdaa72c8175de928a630a4) )	/* labeled u-168 */
	ROM_LOAD( "u169.bin", 0x1000, 0x1000, CRC(68c58207) SHA1(e09f9f8b5f1071fbf8a4883f75f296ec4bc0eca1) )	/* labeled u-169 */
	ROM_LOAD( "u167.bin", 0x2000, 0x1000, CRC(76f16c5b) SHA1(a8a8f0ad7dcc57c2bf518fc5e2509ed8fb87f403) )	/* labeled u-170 */

	ROM_REGION( 0x0100, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "u128.bin", 0x0000, 0x0100, CRC(816784bd) SHA1(47181f4a6ab35c46796ca1d8c130b76f404c188d) )
ROM_END


GAME( 1982, tugboat,  0, tugboat, tugboat,  0, ROT90, "ETM", "Tugboat",    GAME_IMPERFECT_GRAPHICS )
GAME( 1983, noahsark, 0, tugboat, noahsark, 0, ROT90, "Enter-Tech", "Noah's Ark", GAME_IMPERFECT_GRAPHICS )
