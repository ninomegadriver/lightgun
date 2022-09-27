/***************************************************************************

Rastan

driver by Jarek Burczynski


custom ICs
----------
PC040DA x3  video DAC
PC050       coin I/O
PC060HS     main/sub CPU communication
PC080       tilemap generator
PC090       sprite generator


memory map
----------
68000:

The address decoding is done by two PALs (IC11 and IC12) which haven't been
read, so the memory map is inferred by program behaviour

Address                  Dir Data             Name      Description
------------------------ --- ---------------- --------- -----------------------
0000000xxxxxxxxxxxxxxxx- R   xxxxxxxxxxxxxxxx ROM0      program ROM
0000001xxxxxxxxxxxxxxxx- R   xxxxxxxxxxxxxxxx ROM1      program ROM
0000010xxxxxxxxxxxxxxxx- R   xxxxxxxxxxxxxxxx ROM2      program ROM
000100--11xxxxxxxxxxxxxx R/W xxxxxxxxxxxxxxxx WLRAM/WURAM work RAM
001000------xxxxxxxxxxx- R/W xxxxxxxxxxxxxxxx CLCS      palette RAM
0011100-------------000- R   --------xxxxxxxx IN PORT   player 1 inputs
0011100-------------001- R   --------xxxxxxxx IN PORT   player 2 inputs
0011100-------------010- R   --------xxxxxxxx IN PORT   extra inputs
0011100-------------011- R   --------xxxxxxxx IN PORT   coin inputs
0011100-------------100- R   --------xxxxxxxx IN PORT   dip SW1
0011100-------------101- R   --------xxxxxxxx IN PORT   dip SW2
0011100-------------110- R   ---------------- n.c.
0011100-------------111- R   ---------------- n.c.
0011100-----------------   W --------xxx----- OUT8-10   sprite palette bank
0011100-----------------   W -----------x---- n.c.
0011100-----------------   W ------------xxxx           PC050 (coin counters, coin lockout)
0011101----------------- R   ---------------- n.c.
0011101-----------------   W --------------x- M INT     [1]
0011101-----------------   W ---------------x SUB RESET [1]
0011110----------------- R   ---------------- n.c.
0011110-----------------   W ----------------           watchdog reset
0011111---------------x- R/W ------------xxxx SNRD/SNWR PC060HS
0011                                          EXT       [1]
11000xxxxxxxxxxxxxxxxxxx R/W xxxxxxxxxxxxxxxx SCN       PC080 PGA
11010000---xxxxxxxxxxxxx R/W xxxxxxxxxxxxxxxx OBJ       PC090 PGA

[1] Not used, goes to external connector, maybe provision for an MCU?


Z80:

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
00xxxxxxxxxxxxxx R   xxxxxxxx           program ROM
01xxxxxxxxxxxxxx R   xxxxxxxx           program ROM (banked)
1000xxxxxxxxxxxx R/W xxxxxxxx SRAM      work RAM
1001-----------x R/W xxxxxxxx YM2151    YM2151 [1]
1010-----------x R/W ----xxxx PC6       PC060HS
1011------------   W xxxxxxxx V-ST-ADRS MSM5205 start address (bits 15-8)
1100------------   W -------- START-VCE MSM5205 start
1101------------   W -------- STOP-VCE  MSM5205 stop
1110------------              n.c.
1111------------              n.c.

[1] Schematics also show a YM3526 that can replace the YM2151


Notes:
- For sound communication, we are using code in sndhrdw/taitosnd.c, which
  claims to be for the TC0140SYT chip. That chip is not present in Rastan,
  the communication is handled by PC060HS, which I guess is compatible.

TODO:
- Unknown writes to 0x350008.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"


WRITE16_HANDLER( rastan_spritectrl_w );

VIDEO_START( rastan );
VIDEO_UPDATE( rastan );


static WRITE8_HANDLER( rastan_bankswitch_w )
{
	int offs;

	data &= 3;
	if (data == 0) offs = 0x0000;
	else offs = (data-1) * 0x4000 + 0x10000;

	memory_set_bankptr( 1, memory_region(REGION_CPU2) + offs );
}


static int adpcm_pos;

static void rastan_msm5205_vck(int chip)
{
	static int adpcm_data = -1;

	if (adpcm_data != -1)
	{
		MSM5205_data_w(0, adpcm_data & 0x0f);
		adpcm_data = -1;
	}
	else
	{
		adpcm_data = memory_region(REGION_SOUND1)[adpcm_pos];
		adpcm_pos = (adpcm_pos + 1) & 0xffff;
		MSM5205_data_w(0, adpcm_data >> 4);
	}
}

static WRITE8_HANDLER( rastan_msm5205_address_w )
{
	adpcm_pos = (adpcm_pos & 0x00ff) | (data << 8);
}

static WRITE8_HANDLER( rastan_msm5205_start_w )
{
	MSM5205_reset_w(0, 0);
}

static WRITE8_HANDLER( rastan_msm5205_stop_w )
{
	MSM5205_reset_w(0, 1);
	adpcm_pos &= 0xff00;
}



static ADDRESS_MAP_START( rastan_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_ROM
	AM_RANGE(0x10c000, 0x10ffff) AM_RAM
	AM_RANGE(0x200000, 0x200fff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x350008, 0x350009) AM_WRITE(MWA16_NOP)	/* 0 only (often) ? */
	AM_RANGE(0x380000, 0x380001) AM_WRITE(rastan_spritectrl_w)	/* sprite palette bank, coin counters & lockout */
	AM_RANGE(0x390000, 0x390001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x390002, 0x390003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x390004, 0x390005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x390006, 0x390007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x390008, 0x390009) AM_READ(input_port_4_word_r)
	AM_RANGE(0x39000a, 0x39000b) AM_READ(input_port_5_word_r)
	AM_RANGE(0x3c0000, 0x3c0001) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x3e0000, 0x3e0001) AM_READWRITE(MRA16_NOP, taitosound_port16_lsb_w)
	AM_RANGE(0x3e0002, 0x3e0003) AM_READWRITE(taitosound_comm16_lsb_r, taitosound_comm16_lsb_w)
	AM_RANGE(0xc00000, 0xc0ffff) AM_READWRITE(PC080SN_word_0_r, PC080SN_word_0_w)
	AM_RANGE(0xc20000, 0xc20003) AM_WRITE(PC080SN_yscroll_word_0_w)
	AM_RANGE(0xc40000, 0xc40003) AM_WRITE(PC080SN_xscroll_word_0_w)
	AM_RANGE(0xc50000, 0xc50003) AM_WRITE(PC080SN_ctrl_word_0_w)
	AM_RANGE(0xd00000, 0xd03fff) AM_READWRITE(PC090OJ_word_0_r, PC090OJ_word_0_w)	/* sprite ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( rastan_s_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x9001, 0x9001) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xa001, 0xa001) AM_READWRITE(taitosound_slave_comm_r, taitosound_slave_comm_w)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(rastan_msm5205_address_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(rastan_msm5205_start_w)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(rastan_msm5205_stop_w)
ADDRESS_MAP_END



INPUT_PORTS_START( rastan )
	PORT_START_TAG( "IN0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	// button 3
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG( "IN1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	// button 3
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG( "IN2" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )	// P1 button 4
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )	// P1 button 5
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )	// P2 button 4
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )	// P2 button 5
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )	// from PC050 (coin A gets locked if 0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	// from PC050 (coin B gets locked if 0)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	// from PC050 (above 2 bits not checked when 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG( "IN3" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG( "DSW1" )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START_TAG( "DSW2" )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPSETTING(    0x00, "250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( rastsaga )		/* same as rastan, coinage is different */
	PORT_INCLUDE( rastan )

	PORT_MODIFY( "DSW1" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
INPUT_PORTS_END



static const gfx_layout tilelayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, RGN_FRAC(1,2)+0 ,RGN_FRAC(1,2)+4, 8+0, 8+4, RGN_FRAC(1,2)+8+0, RGN_FRAC(1,2)+8+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, RGN_FRAC(1,2)+0 ,RGN_FRAC(1,2)+4, 8+0, 8+4, RGN_FRAC(1,2)+8+0, RGN_FRAC(1,2)+8+4,
			16+0, 16+4, RGN_FRAC(1,2)+16+0, RGN_FRAC(1,2)+16+4, 24+0, 24+4, RGN_FRAC(1,2)+24+0, RGN_FRAC(1,2)+24+4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 0x80 },
	{ REGION_GFX2, 0, &spritelayout, 0, 0x80 },
	{ -1 } /* end of array */
};



/* handler called by the YM2151 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	irqhandler,
	rastan_bankswitch_w
};

static struct MSM5205interface msm5205_interface =
{
	rastan_msm5205_vck,	/* VCK function */
	MSM5205_S48_4B		/* 8 kHz */
};



static MACHINE_DRIVER_START( rastan )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(rastan_map,0)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(rastan_s_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(rastan)
	MDRV_VIDEO_UPDATE(rastan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rastan )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "b04-35.19", 0x00000, 0x10000, CRC(1c91dbb1) SHA1(17fc55e8546cc0b847aebd67fb4570a1e9f128f3) )
	ROM_LOAD16_BYTE( "b04-37.07", 0x00001, 0x10000, CRC(ecf20bdd) SHA1(92e46b1edef40a19be17091c09daba598d77bca8) )
	ROM_LOAD16_BYTE( "b04-40.20", 0x20000, 0x10000, CRC(0930d4b3) SHA1(c269b3856040ed9409de99cca48f22a2f355fc4c) )
	ROM_LOAD16_BYTE( "b04-39.08", 0x20001, 0x10000, CRC(d95ade5e) SHA1(f47557dcfa9d3137e2a3838e45858fc21471cc91) )
	ROM_LOAD16_BYTE( "b04-42.21", 0x40000, 0x10000, CRC(1857a7cb) SHA1(7d967d04ade648c6ddb19aad9e184b6e272856da) )
	ROM_LOAD16_BYTE( "b04-43.09", 0x40001, 0x10000, CRC(c34b9152) SHA1(6ed9247ad455bc3b71d78b541591b269969830cb) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "b04-19.49", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(          0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-01.40",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )
	ROM_LOAD( "b04-03.39",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )
	ROM_LOAD( "b04-02.67",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )
	ROM_LOAD( "b04-04.66",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-05.15",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )
	ROM_LOAD( "b04-07.14",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )
	ROM_LOAD( "b04-06.28",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )
	ROM_LOAD( "b04-08.27",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the samples */
	ROM_LOAD( "b04-20.76", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) ) /* samples are 4bit ADPCM */
ROM_END

ROM_START( rastanu )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "b04-35.19", 0x00000, 0x10000, CRC(1c91dbb1) SHA1(17fc55e8546cc0b847aebd67fb4570a1e9f128f3) )
	ROM_LOAD16_BYTE( "b04-37.07", 0x00001, 0x10000, CRC(ecf20bdd) SHA1(92e46b1edef40a19be17091c09daba598d77bca8) )
	ROM_LOAD16_BYTE( "b04-45.20", 0x20000, 0x10000, CRC(362812dd) SHA1(f7df037ef423d931ca780b34813d4e9e4db67054) )
	ROM_LOAD16_BYTE( "b04-44.08", 0x20001, 0x10000, CRC(51cc5508) SHA1(2bd266351a4d1b94c8c3a489e4d267695d93db5e) )
	ROM_LOAD16_BYTE( "b04-42.21", 0x40000, 0x10000, CRC(1857a7cb) SHA1(7d967d04ade648c6ddb19aad9e184b6e272856da) )
	ROM_LOAD16_BYTE( "b04-41-1.09", 0x40001, 0x10000, CRC(bd403269) SHA1(14aee828d5efb65370a5e453c8fd1c7b3e718074) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "b04-19.49", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-01.40",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )
	ROM_LOAD( "b04-03.39",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )
	ROM_LOAD( "b04-02.67",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )
	ROM_LOAD( "b04-04.66",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-05.15",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )
	ROM_LOAD( "b04-07.14",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )
	ROM_LOAD( "b04-06.28",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )
	ROM_LOAD( "b04-08.27",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* MSM5205 samples */
	ROM_LOAD( "b04-20.76", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) )
ROM_END

ROM_START( rastanu2 )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "rs19_38.bin", 0x00000, 0x10000, CRC(a38ac909) SHA1(66d792fee03c6bd87d15060b9d5cae74137c5ebd) )
	ROM_LOAD16_BYTE( "b04-21.7",    0x00001, 0x10000, CRC(7c8dde9a) SHA1(0cfc3b4f3bc7b940a6c07267ac95e4aae25801ea) )
	ROM_LOAD16_BYTE( "b04-23.20",   0x20000, 0x10000, CRC(254b3dce) SHA1(5126cd5268abaa78dfdcd2ca70621c093c79be67) )
	ROM_LOAD16_BYTE( "b04-22.8",    0x20001, 0x10000, CRC(98e8edcf) SHA1(cc89ef36da6d21192efc19c3bbb215b1635b7ef3) )
	ROM_LOAD16_BYTE( "b04-25.21",   0x40000, 0x10000, CRC(d1e5adee) SHA1(eafc275a0023aecb2efaff14cd890915fa162624) )
	ROM_LOAD16_BYTE( "b04-24.9",    0x40001, 0x10000, CRC(a3dcc106) SHA1(3a8854530b08864a1f7f46c427e49ceec8297806) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "b04-19.49", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-01.40",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )
	ROM_LOAD( "b04-03.39",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )
	ROM_LOAD( "b04-02.67",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )
	ROM_LOAD( "b04-04.66",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-05.15",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )
	ROM_LOAD( "b04-07.14",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )
	ROM_LOAD( "b04-06.28",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )
	ROM_LOAD( "b04-08.27",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* MSM5205 samples */
	ROM_LOAD( "b04-20.76", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) )
ROM_END

ROM_START( rastsaga )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "rs19_38.bin", 0x00000, 0x10000, CRC(a38ac909) SHA1(66d792fee03c6bd87d15060b9d5cae74137c5ebd) )
	ROM_LOAD16_BYTE( "rs07_37.bin", 0x00001, 0x10000, CRC(bad60872) SHA1(e020f79b3ac3d2abccfcd5d135d2dc49e1335c7d) )
	ROM_LOAD16_BYTE( "rs20_40.bin", 0x20000, 0x10000, CRC(6bcf70dc) SHA1(3e369548ac01981c503150b44c2747e6c2cec12a) )
	ROM_LOAD16_BYTE( "rs08_39.bin", 0x20001, 0x10000, CRC(8838ecc5) SHA1(42b43ab77969bbacdf178fbe73a0a27652ccb297) )
	ROM_LOAD16_BYTE( "rs21_42.bin", 0x40000, 0x10000, CRC(b626c439) SHA1(976e820edc4ba107c5b579edaaee1e354e85fb67) )
	ROM_LOAD16_BYTE( "rs09_43.bin", 0x40001, 0x10000, CRC(c928a516) SHA1(fe87fdf2d1b7ba93e1986460eb6af648b58f42e4) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "b04-19.49", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-01.40",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )
	ROM_LOAD( "b04-03.39",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )
	ROM_LOAD( "b04-02.67",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )
	ROM_LOAD( "b04-04.66",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b04-05.15",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )
	ROM_LOAD( "b04-07.14",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )
	ROM_LOAD( "b04-06.28",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )
	ROM_LOAD( "b04-08.27",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* MSM5205 samples */
	ROM_LOAD( "b04-20.76", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) )
ROM_END



GAME( 1987, rastan,   0,      rastan, rastan,   0, ROT0, "Taito Corporation Japan", "Rastan (World)", 0)
/* IDENTICAL to rastan, only difference is copyright notice and Coin B coinage */
GAME( 1987, rastanu,  rastan, rastan, rastsaga, 0, ROT0, "Taito America Corporation", "Rastan (US set 1)", 0)
GAME( 1987, rastanu2, rastan, rastan, rastsaga, 0, ROT0, "Taito America Corporation", "Rastan (US set 2)", 0)
GAME( 1987, rastsaga, rastan, rastan, rastsaga, 0, ROT0, "Taito Corporation", "Rastan Saga (Japan)", 0)
