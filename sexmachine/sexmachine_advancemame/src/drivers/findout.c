/***************************************************************************

Find Out    (c) 1987
Trivia      (c) 1984 / 1986
Quiz        (c) 1991

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/dac.h"


VIDEO_UPDATE( findout )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}


static UINT8 drawctrl[3];

static WRITE8_HANDLER( findout_drawctrl_w )
{
	drawctrl[offset] = data;
}

static WRITE8_HANDLER( findout_bitmap_w )
{
	int sx,sy;
	int fg,bg,mask,bits;
	static int prevoffset, yadd;

	videoram[offset] = data;

	yadd = (offset==prevoffset) ? (yadd+1):0;
	prevoffset = offset;

	fg = drawctrl[0] & 7;
	bg = 2;
	mask = 0xff;//drawctrl[2];
	bits = drawctrl[1];

	sx = 8 * (offset % 64);
	sy = offset / 64;
	sy = (sy + yadd) & 0xff;

//if (mask != bits)
//  ui_popup("color %02x bits %02x mask %02x\n",fg,bits,mask);

	if (mask & 0x80) plot_pixel(tmpbitmap,sx+0,sy,(bits & 0x80) ? fg : bg);
	if (mask & 0x40) plot_pixel(tmpbitmap,sx+1,sy,(bits & 0x40) ? fg : bg);
	if (mask & 0x20) plot_pixel(tmpbitmap,sx+2,sy,(bits & 0x20) ? fg : bg);
	if (mask & 0x10) plot_pixel(tmpbitmap,sx+3,sy,(bits & 0x10) ? fg : bg);
	if (mask & 0x08) plot_pixel(tmpbitmap,sx+4,sy,(bits & 0x08) ? fg : bg);
	if (mask & 0x04) plot_pixel(tmpbitmap,sx+5,sy,(bits & 0x04) ? fg : bg);
	if (mask & 0x02) plot_pixel(tmpbitmap,sx+6,sy,(bits & 0x02) ? fg : bg);
	if (mask & 0x01) plot_pixel(tmpbitmap,sx+7,sy,(bits & 0x01) ? fg : bg);
}


static READ8_HANDLER( portC_r )
{
	return 4;
//  return (rand()&2);
}

static WRITE8_HANDLER( lamps_w )
{
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);
}

static WRITE8_HANDLER( sound_w )
{
	/* bit 3 used but unknown */

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);

//  logerror("%04x: sound_w %02x\n",activecpu_get_pc(),data);
//  ui_popup("%02x",data);
}

static ppi8255_interface ppi8255_intf =
{
	2, 									/* 2 chips */
	{ input_port_0_r, input_port_2_r },	/* Port A read */
	{ input_port_1_r, NULL },			/* Port B read */
	{ NULL,           portC_r },		/* Port C read */
	{ NULL,           NULL },			/* Port A write */
	{ NULL,           lamps_w },		/* Port B write */
	{ sound_w,        NULL },			/* Port C write */
};

MACHINE_RESET( findout )
{
	ppi8255_init(&ppi8255_intf);
}


static READ8_HANDLER( catchall )
{
	int pc = activecpu_get_pc();

	if (pc != 0x3c74 && pc != 0x0364 && pc != 0x036d)	/* weed out spurious blit reads */
		logerror("%04x: unmapped memory read from %04x\n",pc,offset);

	return 0xff;
}

static WRITE8_HANDLER( banksel_main_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x8000);
}
static WRITE8_HANDLER( banksel_1_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE8_HANDLER( banksel_2_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE8_HANDLER( banksel_3_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE8_HANDLER( banksel_4_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x28000);
}
static WRITE8_HANDLER( banksel_5_w )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x30000);
}


/* This signature is used to validate the question ROMs. Simple protection check? */
static int signature_answer,signature_pos;

static READ8_HANDLER( signature_r )
{
	return signature_answer;
}

static WRITE8_HANDLER( signature_w )
{
	if (data == 0) signature_pos = 0;
	else
	{
		static UINT8 signature[8] = { 0xff, 0x01, 0xfd, 0x05, 0xf5, 0x15, 0xd5, 0x55 };

		signature_answer = signature[signature_pos++];

		signature_pos &= 7;	/* safety; shouldn't happen */
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4803) AM_READ(ppi8255_0_r)
	AM_RANGE(0x5000, 0x5003) AM_READ(ppi8255_1_r)
	AM_RANGE(0x6400, 0x6400) AM_READ(signature_r)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x0000, 0xffff) AM_READ(catchall)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x5000, 0x5003) AM_WRITE(ppi8255_1_w)
	/* banked ROMs are enabled by low 6 bits of the address */
	AM_RANGE(0x603e, 0x603e) AM_WRITE(banksel_1_w)
	AM_RANGE(0x603d, 0x603d) AM_WRITE(banksel_2_w)
	AM_RANGE(0x603b, 0x603b) AM_WRITE(banksel_3_w)
	AM_RANGE(0x6037, 0x6037) AM_WRITE(banksel_4_w)
	AM_RANGE(0x602f, 0x602f) AM_WRITE(banksel_5_w)
	AM_RANGE(0x601f, 0x601f) AM_WRITE(banksel_main_w)
	AM_RANGE(0x6200, 0x6200) AM_WRITE(signature_w)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_ROM)	/* space for diagnostic ROM? */
	AM_RANGE(0x8000, 0x8002) AM_WRITE(findout_drawctrl_w)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(findout_bitmap_w)  AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)	/* overlapped by the above */
ADDRESS_MAP_END



INPUT_PORTS_START( findout )
	PORT_START
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Game Repetition" )
	PORT_DIPSETTING( 0x08, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Orientation" )
	PORT_DIPSETTING( 0x10, "Horizontal" )
	PORT_DIPSETTING( 0x00, "Vertical" )
	PORT_DIPNAME( 0x20, 0x20, "Buy Letter" )
	PORT_DIPSETTING( 0x20, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Starting Letter" )
	PORT_DIPSETTING( 0x40, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Bonus Letter" )
	PORT_DIPSETTING( 0x80, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( quiz )
	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Questions" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x01, "5" )
//  PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Show Answer" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "No Coin Counter" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static MACHINE_DRIVER_START( findout )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(findout)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(findout)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( findout )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "12.bin",       0x00000, 0x4000, CRC(21132d4c) SHA1(e3562ee2f46b3f022a852a0e0b1c8fb8164f64a3) )
	ROM_LOAD( "11.bin",       0x08000, 0x2000, CRC(0014282c) SHA1(c6792f2ff712ba3759ff009950d78750df844d01) )	/* banked */
	ROM_LOAD( "13.bin",       0x10000, 0x8000, CRC(cea91a13) SHA1(ad3b395ab0362f3decf178824b1feb10b6335bb3) )	/* banked ROMs for solution data */
	ROM_LOAD( "14.bin",       0x18000, 0x8000, CRC(2a433a40) SHA1(4132d81256db940789a40aa1162bf1b3997cb23f) )
	ROM_LOAD( "15.bin",       0x20000, 0x8000, CRC(d817b31e) SHA1(11e6e1042ee548ce2080127611ce3516a0528ae0) )
	ROM_LOAD( "16.bin",       0x28000, 0x8000, CRC(143f9ac8) SHA1(4411e8ba853d7d5c032115ce23453362ab82e9bb) )
	ROM_LOAD( "17.bin",       0x30000, 0x8000, CRC(dd743bc7) SHA1(63f7e01ac5cda76a1d3390b6b83f4429b7d3b781) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s147.bin",   0x0000, 0x0200, CRC(f3b663bb) SHA1(5a683951c8d3a2baac4b49e379d6e10e35465c8a) )	/* unknown */
ROM_END

ROM_START( gt507uk )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "triv_3_2.bin",    0x00000, 0x4000, CRC(2d72a081) SHA1(8aa32acf335d027466799b097e0de66bcf13247f) )
	ROM_LOAD( "rom_ad.bin",      0x08000, 0x2000, CRC(c81cc847) SHA1(057b7b75a2fe1abf88b23e7b2de230d9f96139f5) )
	ROM_LOAD( "aerospace",       0x10000, 0x8000, CRC(cb555d46) SHA1(559ae05160d7893ff96311a2177eba039a4cf186) )
	ROM_LOAD( "english_sport_4", 0x18000, 0x8000, CRC(6ae8a63d) SHA1(c6018141d8bbe0ed7619980bf7da89dd91d7fcc2) )
	ROM_LOAD( "general_facts",   0x20000, 0x8000, CRC(f921f108) SHA1(fd72282df5cee0e6ab55268b40785b3dc8e3d65b) )
	ROM_LOAD( "horrors",         0x28000, 0x8000, CRC(5f7b262a) SHA1(047480d6bf5c6d0603d538b84c996bd226f07f77) )
	ROM_LOAD( "pop_music",       0x30000, 0x8000, CRC(884fec7c) SHA1(b389216c17f516df4e15eee46246719dd4acb587) )
ROM_END

ROM_START( gt103 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "trvmast3.bin",    0x00000, 0x4000, CRC(9ea277bc) SHA1(f813de572a3b5e2ad601dc6559dc93c30bb7a38c) )
	ROM_LOAD( "comics-cartoons", 0x10000, 0x8000, CRC(193c868e) SHA1(75f18eb6cc6467e927961271374bedaf7736e20b) )
	ROM_LOAD( "general_3",       0x18000, 0x8000, CRC(b0376464) SHA1(d1812d6dd42cd7f3753fcc0d2647b56a91bfcc9d) )
	ROM_LOAD( "science_2",       0x20000, 0x8000, CRC(3c8bc603) SHA1(5e8c1d27fc8ffb9a2a26b749328e09c548da4fca) )
	ROM_LOAD( "soccer",          0x28000, 0x8000, CRC(f821f860) SHA1(b0437ef5d31c507c6499c1fb732d2ba3b9beb151) )
	ROM_LOAD( "sports_alt",      0x30000, 0x8000, CRC(02712bc7) SHA1(a07a87c2c37f34a188d7c00acc0d977acc850f00) )
ROM_END

ROM_START( gt5 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "prog3_right",   0x00000, 0x2000, CRC(e2bd11f6) SHA1(217019bc9c2bb09279cfed4b82b1f5907aa7c1b1) )
	ROM_LOAD( "prog3_left",    0x02000, 0x2000, BAD_DUMP CRC(218bd5c5) SHA1(1321188ca3befb5a20434759718d4935fabcc383) )
	ROM_LOAD( "entertainment", 0x10000, 0x8000, CRC(07068c9f) SHA1(1aedc78d071281ec8b08488cd82655d41a77cf6b) )
	ROM_LOAD( "vices",         0x18000, 0x8000, CRC(e6069955) SHA1(68f7453f21a4ce1be912141bbe947fbd81d918a3) )
	ROM_LOAD( "war_and_peace", 0x20000, 0x8000, CRC(bc709383) SHA1(2fba4c80773abea7bbd826c39378b821cddaa255) )
ROM_END

ROM_START( gt103a )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin", 0x00000, 0x4000, CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "rich-famous",  0x10000, 0x8000, CRC(39e07e4a) SHA1(6e5a0bcefaa1169f313e8818cf50919108b3e121) )
	ROM_LOAD( "rock-n-roll",  0x18000, 0x8000, CRC(1be036b1) SHA1(0b262906044950319dd911b956ac2e0b433f6c7f) )
	ROM_LOAD( "adult_sex_3",  0x20000, 0x8000, CRC(2c46e355) SHA1(387ab389abaaea8e870b00039dd884237f7dd9c6) )
	ROM_LOAD( "sports",       0x28000, 0x8000, CRC(6bd1ba9a) SHA1(7caac1bd438a9b1d11fb33e11814b5d76951211a) )
	ROM_LOAD( "tv_comedies",  0x30000, 0x8000, CRC(992ae38e) SHA1(312780d651a85a1c433f587ff2ede579456d3fd9) )
ROM_END

ROM_START( gt103a1 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "prog1_versiona",  0x00000, 0x4000, CRC(537d6566) SHA1(282a33e4a9fc54d34094393c00026bf31ccd6ab5) )
	ROM_LOAD( "general_1",       0x10000, 0x8000, CRC(1efa01c3) SHA1(801ef5ab55184e488b08ef99ebd641ea4f7edb24) )
	ROM_LOAD( "new_science_2",   0x18000, 0x8000, CRC(3bd80fb8) SHA1(9a196595bc5dc6ed5ee5853786839ed4847fa436) )
	ROM_LOAD( "nfl_football",    0x20000, 0x8000, CRC(d676b7cd) SHA1(d652d2441adb500f7af526d110d0335ea453d75b) )
	ROM_LOAD( "potpourri",       0x28000, 0x8000, CRC(f2968a28) SHA1(87c08c59dfee71e7bf071f09c3017c750a1c5694) )
	ROM_LOAD( "rock_music",      0x30000, 0x8000, CRC(7f11733a) SHA1(d4d0dee75518edf986cb1241ade45ccb4840f088) )
ROM_END

ROM_START( gt103a2 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "prog1_versionc", 0x00000, 0x4000, CRC(340246a4) SHA1(d655e1cf2b1e87a05e87ff6af4b794e6d54a2a52) )
	ROM_LOAD( "cars-women",     0x10000, 0x8000, CRC(4c5dd1df) SHA1(f3e2146eeab07ec71617c7614c6e8f6bc844e6e3) )
	ROM_LOAD( "cops_&_robbers", 0x18000, 0x8000, CRC(8b367c33) SHA1(013468157bf469c9cf138809fdc45b3ba60a423b) )
	ROM_LOAD( "facts",          0x20000, 0x8000, CRC(21bd6181) SHA1(609ae1097a4011e90d03d4c4f03140fbe84c084a) )
	ROM_LOAD( "famous_couples", 0x28000, 0x8000, CRC(e0618218) SHA1(ff64fcd6dec83a2271b63c3ae64dc932a3954ec5) )
	ROM_LOAD( "famous_quotes",  0x30000, 0x8000, CRC(0a27d8ae) SHA1(427e6ae25e47da7f7f7c3e92a37e330d711da90c) )
ROM_END

ROM_START( gt103a3 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "prog1_versionc", 0x00000, 0x4000, CRC(340246a4) SHA1(d655e1cf2b1e87a05e87ff6af4b794e6d54a2a52) )
	ROM_LOAD( "new_sports",     0x10000, 0x8000, CRC(19eff1a3) SHA1(8e024ae6cc572176c90d819a438ace7b2512dbf2) )
	ROM_LOAD( "new_general",    0x18000, 0x8000, CRC(ba1f5b92) SHA1(7e94be0ef6904331d3a6b266e5887e9a15c5e7f9) )
	ROM_LOAD( "new_tv_mash",    0x20000, 0x8000, CRC(f73240c6) SHA1(78020644074da719414133a86a91c1328e5d8929) )
	ROM_LOAD( "new_entrtnmnt",  0x28000, 0x8000, CRC(0f54340c) SHA1(1ca4c23b542339791a2d8f4a9a857f755feca8a1) )
ROM_END

ROM_START( gt103aa )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin",    0x00000, 0x4000, CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "rock-n-roll_alt", 0x10000, 0x8000, CRC(8eb83052) SHA1(93e3c1ae6c2048fb44ecafe1013b6a96da38fa84) )
	ROM_LOAD( "science",         0x18000, 0x8000, CRC(9eaebd18) SHA1(3a4d787cb006dbb23ce346577cb1bb5e543ba52c) )
	ROM_LOAD( "the_sixties",     0x20000, 0x8000, CRC(8cfa854e) SHA1(81428c12f99841db1c61b471ac8d00f0c411883b) )
	ROM_LOAD( "television",      0x28000, 0x8000, CRC(731d4cc0) SHA1(184b6e48edda24f50e377a473a1a4709a218181b) )
	ROM_LOAD( "usa_trivia",      0x30000, 0x8000, CRC(829543b4) SHA1(deb0a4132852643ad884cf194b0a2e6671aa2b4e) )
ROM_END

ROM_START( gt103ab )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin",      0x00000, 0x4000, CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "entertainment_alt", 0x10000, 0x8000, CRC(9a6628b9) SHA1(c0cb7e974329d4d5b91f107296d21a674e35a51b) )
	ROM_LOAD( "general_alt",       0x18000, 0x8000, CRC(df34f7f9) SHA1(329d123eea711d5135dc02dd7b89b220ce8ddd28) )
	ROM_LOAD( "history-geog",      0x20000, 0x8000, CRC(c9a70fc3) SHA1(4021e5d702844416e8c798ed0a57c9ecd20b1d4b) )
	ROM_LOAD( "science_alt",       0x28000, 0x8000, CRC(ac93d348) SHA1(55550ba6b5daffdf9653854075ad4f8398a5e621) )
	ROM_LOAD( "sports_alt2",       0x30000, 0x8000, CRC(40207845) SHA1(2dddb9685dcefabfde07057a639aa9d08da2329e) )
ROM_END

ROM_START( gt103asx )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "t_3a-8_1.bin",    0x00000, 0x4000, CRC(02aef306) SHA1(1ffc10c79a55d41ea36bcaab13cb3f02cb3f9712) )
	ROM_LOAD( "adult_sex_2",     0x10000, 0x8000, CRC(0d683f21) SHA1(f47ce3c31c4c5ed02247fa280303e6ae760315df) )
	ROM_LOAD( "adult_sex_2_alt", 0x18000, 0x8000, CRC(8c0eacc8) SHA1(ddaa25548d161394b41c65a2db57a9fcf793062b) )
	ROM_LOAD( "adult_sex_3_alt", 0x20000, 0x8000, CRC(63cbd1d6) SHA1(8dcd5546dc8688d6b8404d5cf63d8a59acc9bf4c) )
	ROM_LOAD( "adult_sex_4",     0x28000, 0x8000, CRC(36a75071) SHA1(f08d31f241e1dc9b94b940cd2872a692f6f8475b) )
	ROM_LOAD( "adult_sex_5",     0x30000, 0x8000, CRC(fdbc3729) SHA1(7cb7cec4439ddc39de2f7f62c25623cfb869f493) )
ROM_END

ROM_START( quiz )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "1.bin",        0x00000, 0x4000, CRC(4e3204da) SHA1(291f1c9b8c4c07881621c3ecbba7af80f86b9520) )
	ROM_LOAD( "2.bin",        0x10000, 0x8000, CRC(b79f3ae1) SHA1(4b4aa50ec95138bc8ee4bc2a61bcbfa2515ac854) )
	ROM_LOAD( "3.bin",        0x18000, 0x8000, CRC(9c7e9608) SHA1(35ee9aa36d16bca64875640224c7fe9d327a95c3) )
	ROM_LOAD( "4.bin",        0x20000, 0x8000, CRC(30f6b4d0) SHA1(ab2624eb1a3fd9cd8d44433962d09496cd67d569) )
	ROM_LOAD( "5.bin",        0x28000, 0x8000, CRC(e9cdae21) SHA1(4de4a4edf9eccd8f9f7b935f47bee42c10ad606f) )
	ROM_LOAD( "6.bin",        0x30000, 0x8000, CRC(89e2b7e8) SHA1(e85c66f0cf37418f522c2d6384997d52f2f15117) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 ) /* unknown */
	ROM_LOAD( "prom_am27s29pc.bin", 0x0000, 0x0200, CRC(19e3f161) SHA1(52da3c1e50c2329454de14cb9c46149e573e562b) )
ROM_END

ROM_START( quiz211 )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "1a.bin",         0x000000, 0x4000, CRC(116de0ea) SHA1(9af97b100aa2c79a58de055abe726d6e2e00aab4) )
	ROM_CONTINUE(				0x000000, 0x4000 ) // halves identical
	ROM_LOAD( "hobby.bin",      0x10000, 0x8000, CRC(c86d0c2b) SHA1(987ef17c7b9cc119511a16cbd98ec44d24665af5) )
	ROM_LOAD( "musica.bin",     0x18000, 0x8000, CRC(6b08990f) SHA1(bbc633dc4e0c395269d3d3fbf1f7617ea7adabf1) )
	ROM_LOAD( "natura.bin",     0x20000, 0x8000, CRC(f17b0d59) SHA1(ebe3d5a0247f3065f0c5d4ee0b846a737700f379) )
	ROM_LOAD( "spettacolo.bin", 0x28000, 0x8000, CRC(38b8e37a) SHA1(e6df575f61ac61e825d98eaef99c128647806a75) )
	ROM_LOAD( "mondiali90.bin", 0x30000, 0x4000, CRC(35622870) SHA1(f2dab64106ca4ef07175a0ad9491470964d8a0d2) )

	ROM_REGION( 0x0e00, REGION_PROMS, 0 ) /* unknown */
	ROM_LOAD( "prom_27s13-1.bin", 0x0000, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_27s13-2.bin", 0x0200, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_27s13-3.bin", 0x0400, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_27s13-4.bin", 0x0600, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_27s13-5.bin", 0x0800, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_27s13-6.bin", 0x0a00, 0x0200, CRC(71df3345) SHA1(3d64c47b2ce093afad56d8963c151a7854451236) )
	ROM_LOAD( "prom_6349-1n.bin", 0x0c00, 0x0200, CRC(19e3f161) SHA1(52da3c1e50c2329454de14cb9c46149e573e562b) )
ROM_END

/*

When game is first run a RAM error will occur because the nvram needs initialising.

gt507uk - Press F2 to get into test mode, then press control/fire1 to continue
gt103 - When RAM Error appears press F3 to reset and the game will start
gt103a - When ERROR appears, press F2, then F3 to reset, then F2 again and the game will start

*/

GAME( 1986, gt507uk,  0,      findout, findout, 0, ROT0, "Grayhound Electronics", "Trivia (UK Version 5.07)",               GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1986, gt103,    0,      findout, findout, 0, ROT0, "Grayhound Electronics", "Trivia (Version 1.03)",                  GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt5,      0,      findout, findout, 0, ROT0, "Grayhound Electronics", "Trivia (Version 5)",                     GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

GAME( 1984, gt103a,   0,      findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a)",                 GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103a1,  gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a) (alt 1)",         GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103a2,  gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a) (alt 2)",         GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103a3,  gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a) (alt 3)",         GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103aa,  gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a Alt questions 1)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103ab,  gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a Alt questions 2)", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1984, gt103asx, gt103a, findout, findout, 0, ROT0, "Greyhound Electronics", "Trivia (Version 1.03a Sex questions)",   GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1986, quiz,     0,      findout, quiz,    0, ROT0, "Italian bootleg",       "Quiz (Revision 2)",                      GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1987, findout,  0,      findout, findout, 0, ROT0, "Elettronolo",           "Find Out (Version 4.04)",                GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

GAME( 1991, quiz211,  0,      findout, quiz,    0, ROT0, "Elettronolo",           "Quiz (Revision 2.1)",                    GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )

