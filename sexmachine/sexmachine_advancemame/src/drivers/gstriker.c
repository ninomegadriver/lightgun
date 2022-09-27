/*** DRIVER INFO **************************************************************

Grand Striker, V Goal Soccer, World Cup '94
driver by Farfetch and David Haywood

Grand Striker (c)199?  Human
V Goal Soccer (c)199?  Tecmo (2 sets)
Tecmo World Cup '94 (c) 1994 Tecmo

******************************************************************************

    Hardware notes

Both games seem to be similar hardware, V Goal Soccer doesn't work.
the hardware is also quite similar to several other Video System games.

In particular, the sound hardware is identical to aerofgt (including the
memory mapping of the Z80, it's really just a romswap), and the sprite chip
(Fujitsu CG10103) is the same used in several Video System games (see the notes
in the vidhrdw).

Grand Striker has an IRQ2 which is probably network related.

DSWs need correctly mapping, they're just commented for the moment.

TODO:
Finish hooking up the inputs
Tilemap scrolling/rotation/zooming or whatever effect it needs
Priorities are wrong. I suspect they need sprite orthogonality
Missing mixer registers (mainly layer enable/disable)
Merge with other Video System games ?

******************************************************************************/

#include "driver.h"
#include "gstriker.h"
#include "sound/2610intf.h"

/*** README INFO **************************************************************

*** ROMSET: gstriker

Grand Striker
Human 1993

This game runs on Video Systems h/w.

PCB Nos: TW-107 94V-0
         LD01-A
CPU    : MC68000P10
SND    : Zilog Z0840006PSC (Z80), YM2610, YM3016-D
OSC    : 14.31818 MHz, 20.000MHz
XTAL   : 8.000MHz
DIPs   : 8 position (x2)
RAM    : 6264 (x12), 62256 (x4), CY7C195 (x1), 6116 (x3)
PALs   : 16L8 labelled S204A (near Z80)
         16L8 labelled S205A (near VS920A)
         16L8 labelled S201A \
                       S202A  |
                       S203A /  (Near 68000)


Other  :

MC68B50P (located next to 68000)
Fujitsu MB3773 (8 pin DIP)
Fujitsu MB605E53U (160 pin PQFP, located near U2 & U4) (screen tilemap)
Fujitsu CG10103 145 (160 pin PQFP, located near U25) (sprites)
VS9209 (located near DIPs)
VS920A (located near U79) (score tilemap)

ROMs:
human-1.u58 27C240   - Main Program
human-2.u79 27C1024  - ? (near VS920A)
human-3.u87 27C010   - Sound Program
human-4.u6      27C240   - ?, maybe region specific gfx
scrgs101.u25    23C16000 - GFX
scrgs102.u24    23C16000 - GFX
scrgs103.u23    23C16000 - GFX
scrgs104.u22    23C16000 - GFX
scrgs105.u2     23C16000 - GFX   \
scrgs105.u4     23C16000 - GFX   / note, contents of these are identical.
scrgs106.u93    232001   - Sounds
scrgs107.u99    23c8000  - Sounds

*** ROMSET: vgoalsoc

V Goal Soccer
Tecmo 199x?

This game runs on Video Systems h/w.

PCB No: VSIS-20V3, Tecmo No. VG63
CPU: MC68HC000P16
SND: Zilog Z0840006PSC (Z80), YM2610, YM3016-D
OSC: 14.31818 MHz (Near Z80), 32.000MHz (Near 68000), 20.000MHz (Near MCU)
DIPs: 8 position (x2)
RAM: LH5168 (x12), KM62256 (x4), CY7C195 (x1), LH5116 (x3)
PALs: 16L8 labelled S2032A (near Z80)
      16L8 labelled S2036A (near U104)
 4 x  16L8 labelled S2031A \
                    S2033A  |
                    S2034A  |  (Near 68000)
                    S2035A /


Other:

Hitachi H8/325  HD6473258P10 (Micro-controller, located next to 68000)
Fujitsu MB3773 (8 pin DIP)
Fujitsu MB605E53U (160 pin PQFP, located near U17 & U20)
Fujitsu CG10103 145 (160 pin PQFP, located next to VS9210)
VS9210 (located near U11 & U12)
VS9209 (located near DIPs)
VS920A (located near U48) (score tilemap)

ROMs:
c16_u37.u37 27C4002  - Main Program
c16_u48.u48 27C1024  - ?
c16_u65.u65 27C2001  - Sound Program
c13_u86.u86 HN62302  - Sounds
c13_u104.104    HN624116 - Sounds
c13_u20.u20     HN62418  - GFX   \
c13_u17.u17     HN62418  - GFX   / note, contents of these are identical.
c13_u11.u11     HN624116 - GFX
c13_u12.u12     HN624116 - GFX

              Screenshots and board pics are available here...
              http://unemulated.emuunlim.com/shopraid/index.html

*** ROMSET: vgoalsca

Tecmo V Goal Soccer (c)1994? Tecmo

CPU: 68000, Z80
Sound: YM2610
Other: VS9209, VS920A, VS9210, VS920B, HD6473258P10, CG10103, CY7C195,

X1: 20
X2: 32
X3: 14.31818

Note: Same hardware as Tecmo World Cup '94, minus one VS9209 chip.

*** ROMSET: twrldc94

Tecmo World Cup 94
Tecmo 1994

VSIS-20V3

   6264
   6264            H8/320         SW    SW
   6264            20MHz  13  6264
   6264   ?        68000-16   6264
   6264
   6264    ?
   6264
   6264
   6264
   6264

   U11         6264
   U12         6264
   U13
   U14         11


   U17-20                U104
           6264 6264
                      U86
   U17-20    ?                 YM2610
                   12   Z80


******************************************************************************/


UINT16 *gs_videoram3;
UINT16 *gs_mixer_regs;

/* in vidhrdw */
VIDEO_UPDATE( gstriker );
VIDEO_START( gstriker );
VIDEO_START( twrldc94 );


/*** MISC READ / WRITE HANDLERS **********************************************/

static READ16_HANDLER(dmmy_8f)
{
	static int ret = 0xFFFF;
	ret = ~ret;
	return ret;
}

/*** SOUND RELATED ***********************************************************/

static int pending_command;

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
	{
		pending_command = 1;
		soundlatch_w(offset,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

#if 0
static READ16_HANDLER( pending_command_r )
{
	return pending_command;
}
#endif

static WRITE8_HANDLER( gs_sh_pending_command_clear_w )
{
	pending_command = 0;
}

static WRITE8_HANDLER( gs_sh_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x03) * 0x8000;
	memory_set_bankptr(1,&RAM[bankaddress]);
}

/*** GFX DECODE **************************************************************/

static const gfx_layout gs_8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static const gfx_layout gs_16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
	32+0,32+4,32+8,32+12,32+16,32+20,32+24,32+28
	},

	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	 8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64
	},
	16*64
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gs_8x8x4_layout,     0, 256 },
	{ REGION_GFX2, 0, &gs_16x16x4_layout,   0, 256 },
	{ REGION_GFX3, 0, &gs_16x16x4_layout,   0, 256 },

	{ -1 },
};

/*** MORE SOUND RELATED ******************************************************/

static void gs_ym2610_irq(int irq)
{
	if (irq)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
	else
		cpunum_set_input_line(1, 0, CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	gs_ym2610_irq,
	REGION_SOUND1,
	REGION_SOUND2
};

/*** MEMORY LAYOUTS **********************************************************/

static UINT16 *work_ram;
UINT16 *gstriker_lineram;


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x141fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x180000, 0x181fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1c0000, 0x1c0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffffff) AM_READ(MRA16_RAM)

  	AM_RANGE(0x200000, 0x20007f) AM_READ(MRA16_RAM)
	AM_RANGE(0x200080, 0x200081) AM_READ(input_port_1_word_r)
	AM_RANGE(0x200082, 0x200083) AM_READ(input_port_2_word_r)
	AM_RANGE(0x200084, 0x200085) AM_READ(input_port_0_word_r)
	AM_RANGE(0x200086, 0x200087) AM_READ(input_port_3_word_r)
	AM_RANGE(0x200088, 0x200089) AM_READ(input_port_4_word_r)
	AM_RANGE(0x20008e, 0x20008f) AM_READ(dmmy_8f)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MB60553_0_vram_w) AM_BASE(&MB60553_0_vram)
	AM_RANGE(0x140000, 0x141fff) AM_WRITE(MWA16_RAM) AM_BASE(&CG10103_0_vram)
	AM_RANGE(0x180000, 0x180fff) AM_WRITE(VS920A_0_vram_w) AM_BASE(&VS920A_0_vram)
	AM_RANGE(0x181000, 0x181fff) AM_WRITE(MWA16_RAM) AM_BASE(&gstriker_lineram)
	AM_RANGE(0x1c0000, 0x1c0fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x200000, 0x20000f) AM_WRITE(MB60553_0_regs_w)
	AM_RANGE(0x200040, 0x20005f) AM_WRITE(MWA16_RAM) AM_BASE(&gs_mixer_regs)
	AM_RANGE(0x2000a0, 0x2000a1) AM_WRITE(sound_command_w)
	AM_RANGE(0xffc000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&work_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0x02, 0x02) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0x0c, 0x0c) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(gs_sh_bankswitch_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(gs_sh_pending_command_clear_w)
ADDRESS_MAP_END

/*** INPUT PORTS *************************************************************/

INPUT_PORTS_START( gstriker )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )				// "Test"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN) // vbl?
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	// "Spare"
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)	// "Spare"
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0010, 0x0000, "2 Players VS CPU Game" )		// "Cooperation Coin"
	PORT_DIPSETTING(      0x0010, "1 Credit" )
	PORT_DIPSETTING(      0x0000, "2 Credits" )
	PORT_DIPNAME( 0x0020, 0x0000, "Player VS Player Game" )		// "Competitive Coin"
	PORT_DIPSETTING(      0x0020, "1 Credit" )
	PORT_DIPSETTING(      0x0000, "2 Credits" )
	PORT_DIPNAME( 0x0040, 0x0040, "New Challenger" )			/* unknown purpose */
	PORT_DIPSETTING(      0x0040, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Maximum Players" )			// "Cabinet Type"
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x0006, 0x0006, "Player(s) VS CPU Time" )		// "Tournament  Time"
	PORT_DIPSETTING(      0x0006, "1:30" )
	PORT_DIPSETTING(      0x0004, "2:00" )
	PORT_DIPSETTING(      0x0002, "3:00" )
	PORT_DIPSETTING(      0x0000, "4:00" )
	PORT_DIPNAME( 0x0018, 0x0018, "Player VS Player Time" )		// "Competitive Time"
	PORT_DIPSETTING(      0x0018, "2:00" )
	PORT_DIPSETTING(      0x0010, "3:00" )
	PORT_DIPSETTING(      0x0008, "4:00" )
	PORT_DIPSETTING(      0x0000, "5:00" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ) )		// "Demo Sound"
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Communication Mode" )			// "Master/Slave"
	PORT_DIPSETTING(      0x0040, "Master" )
	PORT_DIPSETTING(      0x0000, "Slave" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )					// "Self Test Mode"
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
INPUT_PORTS_END

/*** MACHINE DRIVER **********************************************************/

static MACHINE_DRIVER_START( gstriker )
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(5000) /* hand-tuned, it needs a bit */

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(gstriker)
	MDRV_VIDEO_UPDATE(gstriker)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( twrldc94 )
	MDRV_IMPORT_FROM( gstriker )
	MDRV_VIDEO_START( twrldc94 )
MACHINE_DRIVER_END

/*** ROM LOADING *************************************************************/

ROM_START( gstriker )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "human-1.u58",  0x00000, 0x80000, CRC(45cf4857) SHA1(8133a9a7bdd547cc3d69140a68a1a5a7341e9f5b) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "human-3.u87",  0x00000, 0x20000, CRC(2f28c01e) SHA1(63829ad7969d197b2f2c87cb88bdb9e9880ed2d6) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) // score tilemap
	ROM_LOAD( "human-2.u79",  0x00000, 0x20000, CRC(a981993b) SHA1(ed92c7581d2b84a8628744dd5f8a2266c45dcd5b) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) // scroll tilemap
	ROM_LOAD( "scrgs105.u2",  0x00000, 0x200000, CRC(d584b568) SHA1(64c5e4fdbb859873e51f62d8f5314598108270ef) )
	ROM_LOAD( "scrgs105.u4",  0x00000, 0x200000, CRC(d584b568) SHA1(64c5e4fdbb859873e51f62d8f5314598108270ef) ) // same content, dif pos on board

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "scrgs101.u25", 0x000000, 0x200000, CRC(becaea24) SHA1(e96fca863f49f50992f56c7defa5a69599608785) )
	ROM_LOAD( "scrgs102.u24", 0x200000, 0x200000, CRC(0dae7aba) SHA1(304f336994be33fa8239c13e6fd9967c06f97d5c) )
	ROM_LOAD( "scrgs103.u23", 0x400000, 0x200000, CRC(3448fe92) SHA1(c4c2d2d5610795aff6633b0601ff484897598904) )
	ROM_LOAD( "scrgs104.u22", 0x600000, 0x200000, CRC(0ac33e5a) SHA1(9d7717d80f2c6817bac3fad50c39e04f0aa94255) )
	ROM_LOAD( "human-4.u6",   0xf80000, 0x080000, CRC(a990f9bb) SHA1(7ce31d4c650eb244e2ab285f253a98d6613b7dc8) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "scrgs106.u93", 0x00000, 0x040000, CRC(93c9868c) SHA1(dcecb34e46405155e35aaf134b8547430d23f5a7) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "scrgs107.u99", 0x00000, 0x100000, CRC(ecc0a01b) SHA1(239e832b7d22925460a8f44eb82e782cd13aba49) )
ROM_END

ROM_START( vgoalsoc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "c16_u37.u37",  0x00000, 0x80000, CRC(18c05440) SHA1(0fc78ee0ba6d7817d4a93a80f668f193c352c00d) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "c16_u65.u65",  0x000000, 0x040000, CRC(2f7bf23c) SHA1(1a1a06f57bbac59807679e3762cb2f23ab1ad35e) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) // score tilemap
	ROM_LOAD( "c16_u48.u48",  0x000000, 0x020000, CRC(ca059e7f) SHA1(2fa48b0fec1210575f3a1ecee7d2aec0af3fa9c4) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) // screen tilemap
	ROM_LOAD( "c13_u20.u20",  0x000000, 0x100000, CRC(bc6e07e8) SHA1(3f164165a2eed909aaf38d1ae23a622482d39f96) )
	ROM_LOAD( "c13_u17.u17",  0x000000, 0x100000, CRC(bc6e07e8) SHA1(3f164165a2eed909aaf38d1ae23a622482d39f96) ) // same content, dif pos on board

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD( "c13_u11.u11",  0x000000, 0x200000, CRC(76d09f27) SHA1(ffef83954426f9e56bbe2d98b32cea675c063fab) )
	ROM_LOAD( "c13_u12.u12",  0x200000, 0x200000, CRC(a3874419) SHA1(c9fa283106ada3419e311f400fcf4251b32318c4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "c13_u86.u86",  0x000000, 0x040000, CRC(4b76a162) SHA1(38dcb7536662f5f520e59f3ff746b42e9df789d2) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 )
	ROM_LOAD( "c13_u104.104", 0x000000, 0x200000, CRC(8437b6f8) SHA1(79f183dcbf3cde5c77e086e4fdd8341809396e37) )
ROM_END

ROM_START( vgoalsca )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "vgoalc16.u37", 0x00000, 0x80000, CRC(775ef300) SHA1(d0ab1c13a19ce646c6edfc25a0c0994989560cbc) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "c16_u65.u65",  0x000000, 0x040000, CRC(2f7bf23c) SHA1(1a1a06f57bbac59807679e3762cb2f23ab1ad35e) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) // fixed tile
	ROM_LOAD( "c16_u48.u48",  0x000000, 0x020000, CRC(ca059e7f) SHA1(2fa48b0fec1210575f3a1ecee7d2aec0af3fa9c4) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) // scroll tile
	ROM_LOAD( "vgoalc16.u20", 0x000000, 0x200000, CRC(2b211fb2) SHA1(4e04e099a1dae7abdb0732808a5d74fdc7afcf45) )
	ROM_LOAD( "vgoalc16.u17", 0x000000, 0x200000, CRC(2b211fb2) SHA1(4e04e099a1dae7abdb0732808a5d74fdc7afcf45) ) // same content, dif pos on board

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD( "vgoalc16.u11", 0x000000, 0x200000, CRC(5bc3146c) SHA1(ede4def1ddc4390fed8fd89643900967faff3640) )
	ROM_LOAD( "c13_u12.u12",  0x200000, 0x200000, CRC(a3874419) SHA1(c9fa283106ada3419e311f400fcf4251b32318c4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "c13_u86.u86",  0x000000, 0x040000, CRC(4b76a162) SHA1(38dcb7536662f5f520e59f3ff746b42e9df789d2) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "vgoalc16.104", 0x000000, 0x100000, CRC(6fb06e1b) SHA1(c4584b480fe1165f8e2f887acaa578690514d35d) )
ROM_END

ROM_START( twrldc94 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "13.u37",           0x00000, 0x80000, CRC(42adb463) SHA1(ec7bcb684489b56f81ab851a9d8f42d54679363b) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "12.u65",           0x000000, 0x040000, CRC(f316e7fc) SHA1(a2215605518e7293774735371c65abcead99bd88) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) // fixed tile
	ROM_LOAD( "11.u48",           0x000000, 0x020000, CRC(37d6dcb6) SHA1(679dd8b615497fff23c4638d413b5d4a724d3f2a) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) // scroll tile
	ROM_LOAD( "u17",          0x000000, 0x200000, CRC(a5e40a61) SHA1(a2cb452fb069862570870653b29b045d12caf062) )
	ROM_LOAD( "u20",          0x000000, 0x200000, CRC(a5e40a61) SHA1(a2cb452fb069862570870653b29b045d12caf062) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD( "u11",          0x000000, 0x200000, CRC(dd93fd45) SHA1(26491815b5443fe6d8b1ef4d795c5151fd75c101) )
	ROM_LOAD( "u12",          0x200000, 0x200000, CRC(8e3c9bd2) SHA1(bfd23157c836148a3860ccea5191f656fdd98ef4) )
	ROM_LOAD( "u13",          0x400000, 0x200000, CRC(8db6b3a9) SHA1(9422cd5d6fb57a7eaa7a13bdf4ccee1f8b57f773) )
	ROM_LOAD( "u14",          0x600000, 0x200000, CRC(89739c31) SHA1(29cd779bfe93448fb6cbfe6f8e3661dd659c0d21) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "u86",          0x000000, 0x040000, CRC(775f45dc) SHA1(1a740dd880d9f873e93dfc096fbcae1784b4f522) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "u104",         0x000000, 0x100000, CRC(df07d0af) SHA1(356560e164ff222bc9004fe202f829c93244a6c9) )
ROM_END

ROM_START( twrdc94a )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "twrdc94a_13.u37",           0x00000, 0x80000, CRC(08f314ee) SHA1(3fca5050f5bcd60533d3bd9dea81ba631a98bfd6) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "twrdc94a_12.u65",           0x000000, 0x040000, CRC(c131f5a4) SHA1(d8cc7c463ad628f6f052489a73b97f998532738d) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) // fixed tile
	ROM_LOAD( "twrdc94a_11.u48",           0x000000, 0x020000, CRC(37d6dcb6) SHA1(679dd8b615497fff23c4638d413b5d4a724d3f2a) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) // scroll tile
	ROM_LOAD( "u17",          0x000000, 0x200000, CRC(a5e40a61) SHA1(a2cb452fb069862570870653b29b045d12caf062) )
	ROM_LOAD( "u20",          0x000000, 0x200000, CRC(a5e40a61) SHA1(a2cb452fb069862570870653b29b045d12caf062) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD( "u11",          0x000000, 0x200000, CRC(dd93fd45) SHA1(26491815b5443fe6d8b1ef4d795c5151fd75c101) )
	ROM_LOAD( "u12",          0x200000, 0x200000, CRC(8e3c9bd2) SHA1(bfd23157c836148a3860ccea5191f656fdd98ef4) )
	ROM_LOAD( "u13",          0x400000, 0x200000, CRC(8db6b3a9) SHA1(9422cd5d6fb57a7eaa7a13bdf4ccee1f8b57f773) )
	ROM_LOAD( "u14",          0x600000, 0x200000, CRC(89739c31) SHA1(29cd779bfe93448fb6cbfe6f8e3661dd659c0d21) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "u86",          0x000000, 0x040000, CRC(775f45dc) SHA1(1a740dd880d9f873e93dfc096fbcae1784b4f522) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "u104",         0x000000, 0x100000, CRC(df07d0af) SHA1(356560e164ff222bc9004fe202f829c93244a6c9) )
ROM_END


/******************************************************************************************
Simple protection check concept.The M68k writes a command and the MCU
returns the PC at address 0xffc000.
The problem is that only the concept is easy,beating this protection requires a good
amount of time without a trojan...

Misc Notes:
-Protection routine is at 0x890
-An original feature of this game is that if you enter into service mode the game gives you
the possibility to test various stuff on a pre-registered play such as the speed or
the zooming.To use it,you should use Player 2 Start button to show the test screens
or to advance into the tests.
******************************************************************************************/
#define PC(_num_)\
work_ram[0x000/2] = (_num_ & 0xffff0000) >> 16;\
work_ram[0x002/2] = (_num_ & 0x0000ffff) >> 0;

static WRITE16_HANDLER( twrldc94_mcu_w )
{
	switch(data)
	{
		/*P.O.S.T. / Test Mode*/
		case 0x53: PC(0x00000a4c); break;
		/*Time Up*/
		case 0x68: break;
		/*Title Screen*/
		case 0x69: break;
		/*Gameplay / Attract mode*/
		case 0x6b: PC(0x00000746); break;
		/*(after the P.O.S.T.)*/
		case 0x6e: PC(0x00000746); break;/*WRONG?*/
		default: ui_popup("Unknown MCU CMD %04x",data);
	}
}

static DRIVER_INIT( twrldc94 )
{
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x20008a, 0x20008b, 0, 0, twrldc94_mcu_w);
}

/*** GAME DRIVERS ************************************************************/

GAME( 1993, gstriker, 0,        gstriker, gstriker, 0,        ROT0, "Human", "Grand Striker", GAME_IMPERFECT_GRAPHICS )

/* Similar, but not identical hardware, appear to be protected by an MCU :-( */
GAME( 199?, vgoalsoc, 0,        gstriker, gstriker, 0,        ROT0, "Tecmo", "V Goal Soccer", GAME_NOT_WORKING )
GAME( 199?, vgoalsca, vgoalsoc, gstriker, gstriker, 0,        ROT0, "Tecmo", "V Goal Soccer (alt)", GAME_NOT_WORKING )
GAME( 1994, twrldc94, 0,        twrldc94, gstriker, twrldc94,        ROT0, "Tecmo", "Tecmo World Cup '94", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS )
GAME( 1994, twrdc94a, twrldc94,        twrldc94, gstriker, twrldc94,        ROT0, "Tecmo", "Tecmo World Cup '94 (set 2)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS )


