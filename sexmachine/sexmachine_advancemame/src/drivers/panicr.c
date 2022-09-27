/*
Panic Road
----------

TODO:
- there are 3 more bitplanes of tile graphics, but colors are correct as they are...
  what are they, priority info? Should they be mapped at 8000-bfff in memory?
- problems with bg tilemaps reading (USER3 region)
- sound

--

Panic Road (JPN Ver.)
(c)1986 Taito / Seibu
SEI-8611M (M6100219A)

CPU  : Sony CXQ70116D-8 (8086)
Sound: YM2151,
OSC  : 14.31818MHz,12.0000MHz,16.0000MHz
Other:
    Toshiba T5182
    SEI0010BU(TC17G005AN-0025) x3
    SEI0021BU(TC17G008AN-0022)
    Toshiba(TC17G008AN-0024)
    SEI0030BU(TC17G005AN-0026)
    SEI0050BU(MA640 00)
    SEI0040BU(TC15G008AN-0048)

13F.BIN      [4e6b3c04]
15F.BIN      [d735b572]

22D.BIN      [eb1a46e1]

5D.BIN       [f3466906]
7D.BIN       [8032c1e9]

2A.BIN       [3ac0e5b4]
2B.BIN       [567d327b]
2C.BIN       [cd77ec79]
2D.BIN       [218d2c3e]

2J.BIN       [80f05923]
2K.BIN       [35f07bca]

1.19N        [674131b9]
2.19M        [3d48b0b5]

A.15C        [c75772bc] 82s129
B.14C        [145d1e0d]  |
C.13C        [11c11bbd]  |
D.9B         [f99cac4b] /

8A.BPR       [908684a6] 63s281
10J.BPR      [1dd80ee1]  |
10L.BPR      [f3f29695]  |
12D.BPR      [0df8aa3c] /

*/

#include "driver.h"

static tilemap *bgtilemap, *txttilemap;
static unsigned char *scrollram;
static unsigned char *mainram;

PALETTE_INIT( panicr )
{
	int i;


	palette_init_RRRR_GGGG_BBBB(colortable, color_prom);
	color_prom += 256*3;

	// txt lookup table
	for (i = 0;i < 256;i++)
	{
		if (*color_prom & 0x40)
			*(colortable++) = 0;
		else
			*(colortable++) = (*color_prom & 0x3f) + 0x80;
		color_prom++;
	}

	// tile lookup table
	for (i = 0;i < 256;i++)
	{
		*(colortable++) = (*color_prom & 0x3f) + 0x00;
		color_prom++;
	}

	// sprite lookup table
	for (i = 0;i < 256;i++)
	{
		if (*color_prom & 0x40)
			*(colortable++) = 0;
		else
			*(colortable++) = (*color_prom & 0x3f) + 0x40;
		color_prom++;
	}
}

static void get_bgtile_info(int tile_index)
{
	int code,attr;

	code=memory_region(REGION_USER1)[tile_index];
	attr=memory_region(REGION_USER2)[tile_index];
	code+=((attr&7)<<8);
	SET_TILE_INFO(
		1,
        code,
		(attr & 0xf0) >> 4,
        0)
}

static void get_txttile_info(int tile_index)
{
	int code,attr;

	code=videoram[tile_index*4];
	attr=videoram[tile_index*4+2];
	SET_TILE_INFO(
		0,
		code + ((attr & 8) << 5),
		attr&7,
		0)
}

static ADDRESS_MAP_START( panicr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x01fff) AM_RAM AM_BASE(&mainram)
	AM_RANGE(0x02000, 0x02fff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0x03000, 0x03fff) AM_RAM
	AM_RANGE(0x08000, 0x0bfff) AM_RAM AM_REGION(REGION_USER3, 0) //attribue map ?
	AM_RANGE(0x0c000, 0x0cfff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x0d000, 0x0d008) AM_RAM
	AM_RANGE(0x0d200, 0x0d2ff) AM_RAM // t5182 (seibu sound system)
	AM_RANGE(0x0d800, 0x0d81f) AM_RAM AM_BASE (&scrollram)
	AM_RANGE(0x0d400, 0x0d400) AM_READ(input_port_0_r)
	AM_RANGE(0x0d402, 0x0d402) AM_READ(input_port_1_r)
	AM_RANGE(0x0d404, 0x0d404) AM_READ(input_port_2_r)
	AM_RANGE(0x0d406, 0x0d406) AM_READ(input_port_3_r)
	AM_RANGE(0x0d407, 0x0d407) AM_READ(input_port_4_r)
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

VIDEO_START( panicr )
{
	bgtilemap = tilemap_create( get_bgtile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,1024,16 );
	txttilemap = tilemap_create( get_txttile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR,8,8,32,32 );
	tilemap_set_transparent_pen(txttilemap, 0);
	return 0;
}

static void draw_sprites( mame_bitmap *bitmap,const rectangle *cliprect )
{
	int offs,fx,fy,x,y,color,sprite;

	for (offs = 0; offs<0x1000; offs+=16)
	{

		fx = 0;
		fy = spriteram[offs+1] & 0x80;
		y = spriteram[offs+2];
		x = spriteram[offs+3];

		color = spriteram[offs+1] & 0x0f;

		sprite = spriteram[offs+0]+(scrollram[0x0c]<<8);

		drawgfx(bitmap,Machine->gfx[2],
				sprite,
				color,fx,fy,x,y,
				cliprect,TRANSPARENCY_COLOR,0);
	}
}

VIDEO_UPDATE( panicr)
{
	if(input_port_5_r(0)&1)
	{
		mainram[0x4a2]++;
	}

	fillbitmap(bitmap,get_black_pen(),cliprect);
	tilemap_mark_all_tiles_dirty( txttilemap );
	tilemap_set_scrollx( bgtilemap,0, ((scrollram[0x02]&0x0f)<<12)+((scrollram[0x02]&0xf0)<<4)+((scrollram[0x04]&0x7f)<<1)+((scrollram[0x04]&0x80)>>7) );
	tilemap_draw(bitmap,cliprect,bgtilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,txttilemap,0,0);

}

static INTERRUPT_GEN( panicr_interrupt )
{
	if (cpu_getiloops())
		cpunum_set_input_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc8/4);
	else
		cpunum_set_input_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc4/4);
}

INPUT_PORTS_START( panicr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) //left
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) //right
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) //shake 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) //shake 2
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) //left
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) //right
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) //shake 1
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) //shake 2
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ))

	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))

	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x20, 0x20, "Test Mode" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
 	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x0c, "Bonus" )
	PORT_DIPSETTING(    0x18, "50k & every 1OOk" )
	PORT_DIPSETTING(    0x10, "1Ok 20k" )
	PORT_DIPSETTING(    0x08, "20k 40k" )
	PORT_DIPSETTING(    0x00, "50k 100k" )
	PORT_DIPNAME( 0x08, 0x08, "DSW2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, "Balls" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) ) //?
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)


INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
//  8,
//  { 0, 4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4 },
	4,
	{ RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16+0, 16+1, 16+2, 16+3, 24+0, 24+1, 24+2, 24+3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*16
};


static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 4*8+0, 4*8+1, 4*8+2,  4*8+3,
		8*8+0, 8*8+1, 8*8+2, 8*8+3, 12*8+0, 12*8+1, 12*8+2, 12*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 16*8, 17*8, 18*8, 19*8,
		32*8, 33*8, 34*8, 35*8, 48*8, 49*8, 50*8, 51*8 },
	32*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000,  8 },
	{ REGION_GFX2, 0, &tilelayout,   0x100, 16 },
	{ REGION_GFX3, 0, &spritelayout, 0x200, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( panicr )
	MDRV_CPU_ADD(V20,16000000/2) /* Sony 8623h9 CXQ70116D-8 (V20 compatible) */
	MDRV_CPU_PROGRAM_MAP(panicr_map,0)

	MDRV_CPU_VBLANK_INT(panicr_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256*3)
	MDRV_PALETTE_INIT(panicr)

	MDRV_VIDEO_START(panicr)
	MDRV_VIDEO_UPDATE(panicr)

MACHINE_DRIVER_END

ROM_START( panicr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v20 main cpu */
	ROM_LOAD16_BYTE("2.19m",   0x0f0000, 0x08000, CRC(3d48b0b5) SHA1(a6e8b38971a8964af463c16f32bb7dbd301dd314) )
	ROM_LOAD16_BYTE("1.19n",   0x0f0001, 0x08000, CRC(674131b9) SHA1(63499cd5ad39e79e70f3ba7060680f0aa133f095) )

	ROM_REGION( 0x18000, REGION_CPU3, 0 )	//seibu sound system
	ROM_LOAD( "t5182.bin", 0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "22d.bin",   0x010000, 0x08000, CRC(eb1a46e1) SHA1(278859ae4bca9f421247e646d789fa1206dcd8fc) ) //banked?

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "13f.bin", 0x000000, 0x2000, CRC(4e6b3c04) SHA1(f388969d5d822df0eaa4d8300cbf9cee47468360) )
	ROM_LOAD( "15f.bin", 0x002000, 0x2000, CRC(d735b572) SHA1(edcdb6daec97ac01a73c5010727b1694f512be71) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2a.bin", 0x000000, 0x20000, CRC(3ac0e5b4) SHA1(96b8bdf02002ec8ce87fd47fd21f7797a79d79c9) )
	ROM_LOAD( "2b.bin", 0x020000, 0x20000, CRC(567d327b) SHA1(762b18ef1627d71074ba02b0eb270bd9a01ac0d8) )
	ROM_LOAD( "2c.bin", 0x040000, 0x20000, CRC(cd77ec79) SHA1(94b61b7d77c016ae274eddbb1e66e755f312e11d) )
	ROM_LOAD( "2d.bin", 0x060000, 0x20000, CRC(218d2c3e) SHA1(9503b3b67e71dc63448aed7815845b844e240afe) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2j.bin", 0x000000, 0x20000, CRC(80f05923) SHA1(5c886446fd77d3c39cb4fa43ea4beb8c89d20636) )
	ROM_LOAD( "2k.bin", 0x020000, 0x20000, CRC(35f07bca) SHA1(54e6f82c2e6e1373c3ac1c6138ef738e5a0be6d0) )

	ROM_REGION( 0x04000, REGION_USER1, 0 )
	ROM_LOAD( "5d.bin", 0x00000, 0x4000, CRC(f3466906) SHA1(42b512ba93ba7ac958402d1871c5ae015def3501) ) //tilemaps
	ROM_REGION( 0x04000, REGION_USER2, 0 )
	ROM_LOAD( "7d.bin", 0x00000, 0x4000, CRC(8032c1e9) SHA1(fcc8579c0117ebe9271cff31e14a30f61a9cf031) ) //attribute maps

	ROM_REGION( 0x04000, REGION_USER3, 0 )
	ROM_COPY( REGION_USER2, 0x0000, 0x0000, 0x4000 )

	ROM_REGION( 0x0800,  REGION_PROMS, 0 )
	ROM_LOAD( "b.14c",   0x00000, 0x100, CRC(145d1e0d) SHA1(8073fd176a1805552a5ac00ca0d9189e6e8936b1) )	// red
	ROM_LOAD( "a.15c",   0x00100, 0x100, CRC(c75772bc) SHA1(ec84052aedc1d53f9caba3232ffff17de69561b2) )	// green
	ROM_LOAD( "c.13c",   0x00200, 0x100, CRC(11c11bbd) SHA1(73663b2cf7269a62011ee067a026269ce0c15a7c) )	// blue
	ROM_LOAD( "12d.bpr", 0x00300, 0x100, CRC(0df8aa3c) SHA1(5149265d788ea4885793b0786f765524b4745f04) )	// txt lookup table
	ROM_LOAD( "8a.bpr",  0x00400, 0x100, CRC(908684a6) SHA1(82d9cb8aed576d1132615b5341c36ef51856b3a6) )	// tile lookup table
	ROM_LOAD( "10j.bpr", 0x00500, 0x100, CRC(1dd80ee1) SHA1(2d634e75666b919446e76fd35a06af27a1a89707) )	// sprite lookup table
	ROM_LOAD( "d.9b",    0x00600, 0x100, CRC(f99cac4b) SHA1(b4e6d0e0186fe186e747a9f6857b97591948c682) )	// unknown
	ROM_LOAD( "10l.bpr", 0x00700, 0x100, CRC(f3f29695) SHA1(2607e96564a5e6e9a542377a01f399ea86a36c48) )	// unknown
ROM_END


static DRIVER_INIT( panicr )
{
	UINT8 *buf = malloc(0x80000);
	UINT8 *rom;
	int size;
	int i;

	if (!buf)
		return;

	rom = memory_region(REGION_GFX1);
	size = memory_region_length(REGION_GFX1);

	// text data lines
	for (i = 0;i < size/2;i++)
	{
		int w1;

		w1 = (rom[i + 0*size/2] << 8) + rom[i + 1*size/2];

		w1 = BITSWAP16(w1,  9,12,7,3,  8,13,6,2, 11,14,1,5, 10,15,4,0);

		buf[i + 0*size/2] = w1 >> 8;
		buf[i + 1*size/2] = w1 & 0xff;
	}

	// text address lines
	for (i = 0;i < size;i++)
	{
		rom[i] = buf[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6, 2,3,1,0,5,4)];
	}


	rom = memory_region(REGION_GFX2);
	size = memory_region_length(REGION_GFX2);

	// tiles data lines
	for (i = 0;i < size/4;i++)
	{
		int w1,w2;

		w1 = (rom[i + 0*size/4] << 8) + rom[i + 3*size/4];
		w2 = (rom[i + 1*size/4] << 8) + rom[i + 2*size/4];

		w1 = BITSWAP16(w1, 14,12,11,9,   3,2,1,0, 10,15,13,8,   7,6,5,4);
		w2 = BITSWAP16(w2,  3,13,15,4, 12,2,5,11, 14,6,1,10,    8,7,9,0);

		buf[i + 0*size/4] = w1 >> 8;
		buf[i + 1*size/4] = w1 & 0xff;
		buf[i + 2*size/4] = w2 >> 8;
		buf[i + 3*size/4] = w2 & 0xff;
	}

	// tiles address lines
	for (i = 0;i < size;i++)
	{
		rom[i] = buf[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12, 5,4,3,2, 11,10,9,8,7,6, 0,1)];
	}


	rom = memory_region(REGION_GFX3);
	size = memory_region_length(REGION_GFX3);

	// sprites data lines
	for (i = 0;i < size/2;i++)
	{
		int w1;

		w1 = (rom[i + 0*size/2] << 8) + rom[i + 1*size/2];


		w1 = BITSWAP16(w1, 11,5,7,12, 4,10,13,3, 6,14,9,2, 0,15,1,8);


		buf[i + 0*size/2] = w1 >> 8;
		buf[i + 1*size/2] = w1 & 0xff;
	}

	// sprites address lines
	for (i = 0;i < size;i++)
	{
		rom[i] = buf[i];
	}

	//rearrange  bg tilemaps a bit....

	rom = memory_region(REGION_USER1);
	size = memory_region_length(REGION_USER1);
	memcpy(buf,rom, size);

	{
		int j;
		for(j=0;j<16;j++)
			for (i = 0;i < size/16;i+=8)
			{
				memcpy(&rom[i+(size/16)*j],&buf[i*16+8*j],8);
			}
	}

	rom = memory_region(REGION_USER2);
	size = memory_region_length(REGION_USER2);

	memcpy(buf,rom, size);
	{
		int j;
		for(j=0;j<16;j++)
			for (i = 0;i < size/16;i+=8)
			{
				memcpy(&rom[i+(size/16)*j],&buf[i*16+8*j],8);
			}
	}

	free(buf);
}


GAME( 1986, panicr,  0,       panicr,  panicr,  panicr, ROT270, "Seibu", "Panic Road", GAME_NO_SOUND|GAME_NOT_WORKING )
