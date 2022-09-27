/***************************************************************************

    Lemmings                (c) 1991 Data East USA (DE-0357)

    Prototype!  Licensed from the home computer version this game never
    made it past the arcade field test stage.  Unlike most Data East games
    this hardware features a pixel layer and a VRAM layer, probably to
    make the transition from the pixel addressable computer code to the
    arcade hardware.

    As prototype software it seems to have a couple of non-critical bugs,
    the palette ram check and vram check both overrun their actual ramsize.

    Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "lemmings.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

/******************************************************************************/

static WRITE16_HANDLER( lemmings_control_w )
{
	/* Offset==0 Pixel layer X scroll */
	if (offset==4) return; /* Watchdog or IRQ ack */
	COMBINE_DATA(&lemmings_control_data[offset]);
}

static READ16_HANDLER( lemmings_trackball_r )
{
	switch (offset) {
	case 0: return readinputport(4); break;
	case 1: return readinputport(5); break;
	case 4: return readinputport(6); break;
	case 5: return readinputport(7); break;
	}
	return 0;
}

/* Same as Robocop 2 protection chip */
static READ16_HANDLER( lemmings_prot_r )
{
 	switch (offset<<1) {
		case 0x41a: /* Player input */
			return readinputport(0);

		case 0x320: /* Coins */
			return readinputport(1);

		case 0x4e6: /* Dips */
			return (readinputport(2) + (readinputport(3) << 8));
	}

	return 0;
}

static WRITE16_HANDLER( lemmings_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}

static WRITE16_HANDLER( lemmings_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpunum_set_input_line(1,1,HOLD_LINE);
}

static WRITE8_HANDLER( lemmings_sound_ack_w )
{
	cpunum_set_input_line(1,1,CLEAR_LINE);
}

/******************************************************************************/

static ADDRESS_MAP_START( lemmings_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x1207ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x1407ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x160000, 0x160fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1a0000, 0x1a07ff) AM_READ(lemmings_prot_r)
	AM_RANGE(0x190000, 0x19000f) AM_READ(lemmings_trackball_r)
	AM_RANGE(0x200000, 0x202fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x300000, 0x37ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x380000, 0x39ffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lemmings_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x140000, 0x1407ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2) AM_SIZE(&spriteram_2_size)
	AM_RANGE(0x160000, 0x160fff) AM_WRITE(lemmings_palette_24bit_w) AM_BASE(&paletteram16)
	AM_RANGE(0x170000, 0x17000f) AM_WRITE(lemmings_control_w) AM_BASE(&lemmings_control_data)
	AM_RANGE(0x1a0064, 0x1a0065) AM_WRITE(lemmings_sound_w)
	AM_RANGE(0x1c0000, 0x1c0001) AM_WRITE(buffer_spriteram16_w) /* 1 written once a frame */
	AM_RANGE(0x1e0000, 0x1e0001) AM_WRITE(buffer_spriteram16_2_w) /* 1 written once a frame */
	AM_RANGE(0x200000, 0x201fff) AM_WRITE(lemmings_vram_w) AM_BASE(&lemmings_vram_data)
	AM_RANGE(0x300000, 0x37ffff) AM_WRITE(lemmings_pixel_0_w) AM_BASE(&lemmings_pixel_0_data)
	AM_RANGE(0x380000, 0x39ffff) AM_WRITE(lemmings_pixel_1_w) AM_BASE(&lemmings_pixel_1_data)
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0801, 0x0801) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x1000, 0x1000) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0x1800, 0x1800) AM_READ(soundlatch_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0800, 0x0800) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x0801, 0x0801) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x1000, 0x1000) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(lemmings_sound_ack_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

/******************************************************************************/

INPUT_PORTS_START( lemmings )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Select")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Hurry")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Select")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Hurry")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")	/* Credits */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_SERVICE_NO_TOGGLE(0x0004, IP_ACTIVE_LOW)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, "Credits for 1 Player" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Credits for 2 Player" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Credits for Continue" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("AN0")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START_TAG("AN1")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("AN2")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(2)

	PORT_START_TAG("AN3")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END

/******************************************************************************/

static const gfx_layout charlayout =
{
	8,8,
	2048,
	4,
	{ 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*0, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};

static const gfx_layout sprite_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0x30000*8, 0x20000*8, 0x10000*8, 0x00000*8 },
	{
		7, 6, 5, 4, 3, 2, 1, 0, 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0
	},
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout,  32*16, 16 },	/* Sprites 16x16 */
	{ REGION_GFX2, 0, &sprite_layout,  48*16, 16 },	/* Sprites 16x16 */
	{ 0,           0, &charlayout,         0, 16 }, /* Dynamically modified */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(int state)
{
	cpunum_set_input_line(1,0,state);
}

static struct YM2151interface ym2151_interface =
{
	sound_irq
};

static MACHINE_DRIVER_START( lemmings )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_PROGRAM_MAP(lemmings_readmem,lemmings_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(M6809,32220000/8)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_EOF(lemmings)
	MDRV_VIDEO_START(lemmings)
	MDRV_VIDEO_UPDATE(lemmings)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 32220000/9)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 7757)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( lemmings )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "lemmings.5", 0x00000, 0x20000, CRC(e9a2b439) SHA1(873723a06d71bb41772951f451a75578b30267d5) )
	ROM_LOAD16_BYTE( "lemmings.1", 0x00001, 0x20000, CRC(bf52293b) SHA1(47a1ed64bf02776db086fdce80997b8a0c068791) )
	ROM_LOAD16_BYTE( "lemmings.6", 0x40000, 0x20000, CRC(0e3dc0ea) SHA1(533abf66ca4b578d03566d5de922dc5828c26eca) )
	ROM_LOAD16_BYTE( "lemmings.2", 0x40001, 0x20000, CRC(0cf3d7ce) SHA1(95dc43a8cded860fcf8743b62cbe4f2a97f43215) )
	ROM_LOAD16_BYTE( "lemmings.7", 0x80000, 0x20000, CRC(d020219c) SHA1(9678d8636798d1e528269fe2f9eb532e189c134e) )
	ROM_LOAD16_BYTE( "lemmings.3", 0x80001, 0x20000, CRC(c635494a) SHA1(e105dc79bd3c425d971629a3066c38dbf08b6428) )
	ROM_LOAD16_BYTE( "lemmings.8", 0xc0000, 0x20000, CRC(9166ce09) SHA1(7f0970cc07ebdbfc9a738342259d07d37b397161) )
	ROM_LOAD16_BYTE( "lemmings.4", 0xc0001, 0x20000, CRC(aa845488) SHA1(d17ec80f43d2a0123e93fad83d4e1319eb18d7c7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "lemmings.15",    0x00000, 0x10000, CRC(f0b24a35) SHA1(1aaeb1e6faee04d2e433161fd7aa965b58e3b8a7) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
  	ROM_LOAD( "lemmings.9",  0x000000, 0x10000, CRC(e06442f5) SHA1(d9c8b681cce1d0257a0446bc820c7d679e2a1168) )
	ROM_LOAD( "lemmings.10", 0x010000, 0x10000, CRC(36398848) SHA1(6c6956607f889c35367e6df4a32359042fad695e) )
  	ROM_LOAD( "lemmings.11", 0x020000, 0x10000, CRC(b46a54e5) SHA1(53b053346f80357aecff4ab888a8562f99cb318f) )
	ROM_FILL(                0x030000, 0x10000, 0 ) /* 3bpp data but sprite chip expects 4 */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
 	ROM_LOAD( "lemmings.12", 0x000000, 0x10000, CRC(dc9047ff) SHA1(1bbe573fa51127a9e8b970a353f3cceab00f486a) )
  	ROM_LOAD( "lemmings.13", 0x010000, 0x10000, CRC(7cc15491) SHA1(73c1c11b2738f6679c70cae8ac4c55cdc9b8fc27) )
	ROM_LOAD( "lemmings.14", 0x020000, 0x10000, CRC(c162788f) SHA1(e1f669efa59699cd1b7da71b112701ee79240c18) )
	ROM_FILL(                0x030000, 0x10000, 0 ) /* 3bpp data but sprite chip expects 4 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
  	ROM_LOAD( "lemmings.16",    0x00000, 0x20000, CRC(f747847c) SHA1(00880fa6dff979e5d15daea61938bd18c768c92f) )
ROM_END

/******************************************************************************/

GAME( 1991, lemmings, 0, lemmings, lemmings, 0, ROT0, "Data East USA", "Lemmings (US Prototype)", 0 )
