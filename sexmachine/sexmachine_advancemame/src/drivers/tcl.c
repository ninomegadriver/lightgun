/*
Taiwan Chess Legend
Uniwang, 1995

Preliminary driver by Tomasz Slanina

PCB Layout
----------


|-----------------------------------------------|
|  AY8930   DSW5  TCL.1E                        |
|           DSW4  TCL.3E   TCL.3F  TCL.3H       |
|           DSW3  TCL.4E   TCL.4F  TCL.4H       |
|           DSW2  6116                          |
|           DSW1  6116          6116            |
|                               6116            |
|  8255                               12MHz     |
|                                               |
|                                               |
|                 TCL_PR3.9E                    |
|                                               |
|  8255                     PAL                 |
|                                               |
|                           PAL                 |
|  BATTERY                    LATTICE           |
|                       PAL   PLSI 1016 Z80 PAL |
|           TCL_PR1.15C                     PAL |
|  SW1      TCL_PR2.16C 6116      TCL.16F       |
|-----------------------------------------------|

Notes:
      Z80 Clock: 3.000MHz
          VSync: 60Hz
          HSync: 15.15kHz

*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"


VIDEO_START( tcl )
{
	return 0;
}
VIDEO_UPDATE( tcl )
{
}

static ADDRESS_MAP_START( tcl_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM  /* bfff ? */
ADDRESS_MAP_END


INPUT_PORTS_START( tcl )
	PORT_START
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ 0, RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charlayout2 =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0, RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4)},
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX2, 0, &charlayout2,     0, 16 }, /* wrong */
	{ -1 }	/* end of array */
};

static ppi8255_interface ppi8255_intf =
{
	2,
	{ NULL, NULL },
	{ NULL, NULL },
	{ NULL, NULL },
	{ NULL, NULL },
	{ NULL, NULL },
	{ NULL, NULL },
};

MACHINE_RESET( tcl )
{
	ppi8255_init(&ppi8255_intf);
}


static MACHINE_DRIVER_START( tcl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/4)
	MDRV_CPU_PROGRAM_MAP(tcl_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*16)

	MDRV_VIDEO_START(tcl)
	MDRV_VIDEO_UPDATE(tcl)

	MDRV_MACHINE_RESET(tcl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 12000000/6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tcl )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "tcl.16f",   0x00000, 0x20000, CRC(8e694a58) SHA1(7a3c20a7c740065b71fe66ec581edce0dd32f145) )

	ROM_REGION( 0x8000*3, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tcl.1e",   0x00000, 0x8000, CRC(37edf9b8) SHA1(9225728116d6edfe8476e565a12e1f1e59766d26) )
	ROM_LOAD( "tcl.3e",   0x08000, 0x8000, CRC(396298cf) SHA1(0ee306179a9d3dd84f7e5799527e6825d2979ddb) )
	ROM_LOAD( "tcl.4e",   0x10000, 0x8000, CRC(f880101c) SHA1(8417410a7dcb304a88e98f9199f44a4df1ee3fb7) )

	ROM_REGION( 0x2000*4, REGION_GFX2, ROMREGION_DISPOSE ) /* ??? */
	ROM_LOAD( "tcl.3f",   0x0000, 0x2000, CRC(c290c1eb) SHA1(00eb5ff46affe01f240081211b7f9a40e9f76bd8) )
	ROM_LOAD( "tcl.4f",   0x2000, 0x2000, CRC(225e0148) SHA1(26d8db263b1957fc6b2204765c8aa1f10f44b591) )
	ROM_LOAD( "tcl.3h",   0x4000, 0x2000, CRC(ee63d380) SHA1(c1d9ca4584bb2ef0fa85e2afb0876040b473a924) )
	ROM_LOAD( "tcl.4h",   0x6000, 0x2000, CRC(6afa36a1) SHA1(a87423f01dbf9b1e69feb049d6ae3fd63321ee1a) )

	ROM_REGION( 0x100*3, REGION_PROMS, 0 )
	ROM_LOAD( "tcl_pr1.15c",   0x000, 0x100, CRC(21eb5b19) SHA1(9b8425bdb97f11f4855c998c7792c3291fd07470) )
	ROM_LOAD( "tcl_pr2.16c",   0x100, 0x100, CRC(0489b760) SHA1(78f8632b17a76335183c5c204cdec856988368b0) )
	ROM_LOAD( "tcl_pr3.9e",    0x200, 0x100, CRC(50ec383b) SHA1(ae95b92bd3946b40134bcdc22708d5c6b0f4c23e) )
ROM_END

#define ROL(x,n) (BITSWAP8((x),(7+8-n)&7,(6+8-n)&7,(5+8-n)&7,(4+8-n)&7,(3+8-n)&7,(2+8-n)&7,(1+8-n)&7,(0+8-n)&7))

#define WRITEDEST( n ) \
		dest[idx]=n;	\
		dest[idx+0x10000]=(n)^0xff;	\
		idx++;

DRIVER_INIT(tcl)
{
	/* only the first part is decrypted (and verified)*/

	UINT8 *dest = memory_region(REGION_CPU1);
	UINT8 *src = malloc_or_die(memory_region_length(REGION_CPU1));

	int i,idx=0;
	memcpy(src, dest, memory_region_length(REGION_CPU1));
	for(i=0;i<64*1024;i+=4)
	{
		if(i&0x8000)
		{
			WRITEDEST(ROL(src[idx]^0x44,4)); // abcdefgh -> aFghaBcd
		 	WRITEDEST(ROL(src[idx]^0x44,7)); // abcdefgh -> haBcdeFg
		 	WRITEDEST(ROL(src[idx]^0x44,2)); // abcdefgh -> cdeFghaB
		 	WRITEDEST((src[idx]^0x44)^0xf0); // abcdefgh -> AbCEeFgh
		}
		else
		{
		 	WRITEDEST(ROL(src[idx]^0x11,4)); // abcdefgh -> efgHabcD
			WRITEDEST(ROL(src[idx]^0x11,7)); // abcdefgh -> HabcDefg
			WRITEDEST(ROL(src[idx]^0x11,2)); // abcdefgh -> cDefgHab
			WRITEDEST((src[idx]^0x11)^0xf0); // abcdefgh -> ABCdefgH
		}
	}
	free(src);

	memory_set_decrypted_region(0, 0x0000, 0x7fff, dest+0x10000);
}

GAME( 1995, tcl,  0,       tcl,  tcl,  tcl, ROT0, "Uniwang", "Taiwan Chess Legend", GAME_NOT_WORKING )
