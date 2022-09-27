/***************************************************************************

    Bally/Midway Jr. Pac-Man

    Games supported:
        * Jr. Pac-Man

    Known issues:
        * none

****************************************************************************

    Jr. Pac Man memory map (preliminary)

    0000-3fff ROM
    4000-47ff Video RAM (also color RAM)
    4800-4fff RAM
    8000-dfff ROM

    memory mapped ports:

    read:
    5000      IN0
    5040      IN1
    5080      DSW1

    *
     * IN0 (all bits are inverted)
     * bit 7 : CREDIT
     * bit 6 : COIN 2
     * bit 5 : COIN 1
     * bit 4 : RACK TEST
     * bit 3 : DOWN player 1
     * bit 2 : RIGHT player 1
     * bit 1 : LEFT player 1
     * bit 0 : UP player 1
     *
    *
     * IN1 (all bits are inverted)
     * bit 7 : TABLE or UPRIGHT cabinet select (1 = UPRIGHT)
     * bit 6 : START 2
     * bit 5 : START 1
     * bit 4 : TEST SWITCH
     * bit 3 : DOWN player 2 (TABLE only)
     * bit 2 : RIGHT player 2 (TABLE only)
     * bit 1 : LEFT player 2 (TABLE only)
     * bit 0 : UP player 2 (TABLE only)
     *
    *
     * DSW1 (all bits are inverted)
     * bit 7 :  ?
     * bit 6 :  difficulty level
     *                       1 = Normal  0 = Harder
     * bit 5 :\ bonus pac at xx000 pts
     * bit 4 :/ 00 = 10000  01 = 15000  10 = 20000  11 = 30000
     * bit 3 :\ nr of lives
     * bit 2 :/ 00 = 1  01 = 2  10 = 3  11 = 5
     * bit 1 :\ play mode
     * bit 0 :/ 00 = free play   01 = 1 coin 1 credit
     *          10 = 1 coin 2 credits   11 = 2 coins 1 credit
     *

    write:
    4ff2-4ffd 6 pairs of two bytes:
              the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
              X flip (bit 1); the second byte the color
    5000      interrupt enable
    5001      sound enable
    5002      unused
    5003      flip screen
    5004      unused
    5005      unused
    5006      unused
    5007      coin counter
    5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
    5045      sound voice 1 waveform (nibble)
    5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
    504a      sound voice 2 waveform (nibble)
    504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
    504f      sound voice 3 waveform (nibble)
    5050-5054 sound voice 1 frequency (nibbles)
    5055      sound voice 1 volume (nibble)
    5056-5059 sound voice 2 frequency (nibbles)
    505a      sound voice 2 volume (nibble)
    505b-505e sound voice 3 frequency (nibbles)
    505f      sound voice 3 volume (nibble)
    5062-506d Sprite coordinates, x/y pairs for 6 sprites
    5070      palette bank
    5071      colortable bank
    5073      background priority over sprites
    5074      char gfx bank
    5075      sprite gfx bank
    5080      scroll
    50c0      Watchdog reset

    I/O ports:
    OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "driver.h"
#include "pacman.h"
#include "sound/namco.h"


static WRITE8_HANDLER( jrpacman_interrupt_vector_w )
{
	cpunum_set_input_line_vector(0, 0, data);
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_READWRITE(MRA8_RAM, jrpacman_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4800, 0x4fef) AM_RAM
	AM_RANGE(0x4ff0, 0x4fff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(pacman_sound_enable_w)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pacman_flipscreen_w)
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5040, 0x505f) AM_WRITE(pacman_sound_w) AM_BASE(&pacman_soundregs)
	AM_RANGE(0x5060, 0x506f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0x5070, 0x5070) AM_WRITE(pengo_palettebank_w)
	AM_RANGE(0x5071, 0x5071) AM_WRITE(pengo_colortablebank_w)
	AM_RANGE(0x5073, 0x5073) AM_WRITE(jrpacman_bgpriority_w)
	AM_RANGE(0x5074, 0x5074) AM_WRITE(jrpacman_charbank_w)
	AM_RANGE(0x5075, 0x5075) AM_WRITE(jrpacman_spritebank_w)
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x5080, 0x5080) AM_WRITE(jrpacman_scroll_w)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x8000, 0xdfff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( port_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0, 0) AM_WRITE(jrpacman_interrupt_vector_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( jrpacman )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test (Cheat)") PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout tilelayout =
{
	8,8,	/* 8*8 characters */
    RGN_FRAC(1,2),    /* 256 characters */
    2,  /* 2 bits per pixel */
    { 0, 4 },   /* the two bitplanes for 4 pixels are packed into one byte */
    { 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 }, /* bits are packed in groups of four */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 bytes */
};


static const gfx_layout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,2),	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0x2000, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct namco_interface namco_interface =
{
	3,			/* number of voices */
	REGION_SOUND1	/* memory region */
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( jrpacman )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(port_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(128*4)

	MDRV_PALETTE_INIT(pacman)
	MDRV_VIDEO_START(jrpacman)
	MDRV_VIDEO_UPDATE(pacman)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(NAMCO, 3072000/32)
	MDRV_SOUND_CONFIG(namco_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( jrpacman )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",             0x0000, 0x2000, CRC(e3fa972e) SHA1(5ea34621213c649ca2848ab31aab2cbe751723d4) )
	ROM_LOAD( "jrp8e.bin",             0x2000, 0x2000, CRC(ec889e94) SHA1(8294e9e79f8fd19a419431fa690e6ac4a1302f58) )
	ROM_LOAD( "jrp8h.bin",             0x8000, 0x2000, CRC(35f1fc6e) SHA1(b84b34560b9aae18b24274712b052283faa01730) )
	ROM_LOAD( "jrp8j.bin",             0xa000, 0x2000, CRC(9737099e) SHA1(07d912a61824323c8fc1b8bd0da89172d4f70b91) )
	ROM_LOAD( "jrp8k.bin",             0xc000, 0x2000, CRC(5252dd97) SHA1(18bd4d5381656120e4242811006c20776774de4d) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",             0x0000, 0x2000, CRC(0527ff9b) SHA1(37fe3176b0d125b7d629e108e7ebdc1196e4a132) ) /* tiles (bank 1 & 2) */
	ROM_LOAD( "jrp2e.bin",             0x2000, 0x2000, CRC(73477193) SHA1(f00a488958ea0438642d345693787bdf771219ad) ) /* sprites (bank 1 & 2) */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD_NIB_LOW ( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* color palette (low bits) */
	ROM_LOAD_NIB_HIGH( "jrprom.9f",    0x0000, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* color palette (high bits) */
	ROM_LOAD( "jrprom.9p",             0x0020, 0x0100, CRC(9f6ea9d8) SHA1(62cf15513934d34641433c891a7f73bef82e2fb1) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "jrprom.7p",             0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) ) /* waveform */
	ROM_LOAD( "jrprom.5s",             0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( jrpacman )
{
	/* The encryption PALs garble bits 0, 2 and 7 of the ROMs. The encryption */
	/* scheme is complex (basically it's a state machine) and can only be */
	/* faithfully emulated at run time. To avoid the performance hit that would */
	/* cause, here we have a table of the values which must be XORed with */
	/* each memory region to obtain the decrypted bytes. */
	/* Decryption table provided by David Caldwell (david@indigita.com) */
	/* For an accurate reproduction of the encryption, see jrcrypt.c */
	static const struct {
	    int count;
	    int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
	    { 0,0 }
	};

	unsigned char *RAM = memory_region(REGION_CPU1);
	int i, j, A;

	for (i = A = 0; table[i].count; i++)
		for (j = 0; j < table[i].count; j++)
			RAM[A++] ^= table[i].value;
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, jrpacman, 0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man", GAME_SUPPORTS_SAVE )
