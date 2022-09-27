/*
PCB Layout
----------

|----------------------------------------------------------|
|Z8430   ROM2.U84                          ROM3.U32        |
|                                                          |
|                                          ROM4.U31        |
|YM2203  Z80                                               |
|                                          ROM5.U30        |
|        6116                                              |
|                                          6116            |
| 8255                                                     |
|                                          6116   7.3728MHz|
|       Y3014                                              |
|                                                82S129.U10|
|J       82S123.U79                                        |
|A                                               82S129.U9 |
|M                                                         |
|M             6116                                        |
|A                                               ROM6.U8   |
|          ROM1.U77                                        |
| VOL                                            ROM7.U7   |
|               Z80                                        |
| UPC1241H                                       ROM8.U6   |
|              8255                                        |
|                                  6116          ROM9.U5   |
|                                                          |
|           68705R3                6116          ROM10.U4  |
|                                                          |
|DSW1                              6116          ROM11.U3  |
|                                                          |
|    8255                          6116          ROM12.U2  |
|                                                          |
|DSW2                              6116          ROM13.U1  |
|----------------------------------------------------------|
Notes:
      Z80 clocks (both) - 3.6864MHz [7.3728/2]
      Z8430             - zILOG Z8430 z80 CTC Counter Timer Circuit, clock 3.6864MHz [7.3728/2] (DIP28)
      68705R3           - Motorola MC68705R3 Microcontroller, clock 3.6864MHz [7.3728/2] (DIP40)
      YM2203 clock      - 1.8432MHz [7.3728/4]
      6116              - 2K x8 SRAM (DIP24)
      8255              - NEC D8255AC-2 Programmable Peripheral Interface (DIP40)
      VSync             - 60.0Hz
      HSync             - 15.40kHz


*/

#include "driver.h"
#include "machine/z80ctc.h"
#include "sound/2203intf.h"
#include "machine/8255ppi.h"
#include "cpu/z80/z80daisy.h"

static tilemap *tilemap1;
static tilemap *tilemap2;

static UINT8 *vram1;
static UINT8 *vram2;

static UINT8 vidctrl;
static UINT8 *palram;
static UINT8 toMCU, fromMCU, ddrA;

static void get_tile_info(int tile_index)
{
	int code = vram2[tile_index]+vram2[tile_index+0x800]*256;
	SET_TILE_INFO(
		0,
		code,
		0,
		0)
}

static void get_tile_info2(int tile_index)
{
	int code =vram1[tile_index]+((vram1[tile_index+0x800]>>4))*256;
	int color=((vram1[tile_index+0x800])&0xf);
	SET_TILE_INFO
	(
		1,
		code,
		color,
		0
	)
}

VIDEO_START ( pipeline )
{
	palram=auto_malloc(0x1000);
	tilemap1 = tilemap_create( get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32 );
	tilemap2 = tilemap_create( get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32 );
	tilemap_set_transparent_pen(tilemap2,0);
	return 0;
}

VIDEO_UPDATE ( pipeline)
{
	tilemap_draw(bitmap,cliprect,tilemap1, 0,0);
	tilemap_draw(bitmap,cliprect,tilemap2, 0,0);
}


INPUT_PORTS_START( pipeline )
	PORT_START
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 1-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 1-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIPSW 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIPSW 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIPSW 2-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIPSW 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIPSW 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIPSW 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

static WRITE8_HANDLER(vidctrl_w)
{
	vidctrl=data;
}

static READ8_HANDLER(vram2_r)
{
	return vram2[offset];
}

static WRITE8_HANDLER(vram2_w)
{

	if(!(vidctrl&1))
	{
		tilemap_mark_tile_dirty(tilemap1,offset&0x7ff);
		vram2[offset]=data;
	}
	else
	{
		 palram[offset]=data;
		 if(offset<0x300)
		 {
		 	offset&=0xff;
		 	palette_set_color(offset, palram[offset]<<2, palram[offset+0x100]<<2, palram[offset+0x200]<<2);
		 }
	}
}

static READ8_HANDLER(vram1_r)
{
	return vram1[offset];
}

static WRITE8_HANDLER(vram1_w)
{
		tilemap_mark_tile_dirty(tilemap2,offset&0x7ff);
		vram1[offset]=data;
}

static READ8_HANDLER(protection_r)
{
	return fromMCU;
}

static WRITE8_HANDLER(protection_w)
{
	toMCU=data;
	timer_set(TIME_NOW, 0, NULL);
}

static ADDRESS_MAP_START( cpu0_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x97ff) AM_READWRITE(vram1_r, vram1_w) AM_BASE(&vram1)
	AM_RANGE(0x9800, 0xa7ff) AM_READWRITE(vram2_r, vram2_w) AM_BASE(&vram2)
	AM_RANGE(0xb800, 0xb803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0xb810, 0xb813) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0xb830, 0xb830) AM_NOP
	AM_RANGE(0xb840, 0xb840) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xe000, 0xe003) AM_READWRITE(ppi8255_2_r, ppi8255_2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_port, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_READ(z80ctc_0_r) AM_WRITE(z80ctc_0_w)
	AM_RANGE(0x06, 0x07) AM_NOP
ADDRESS_MAP_END

static WRITE8_HANDLER(mcu_portA_w)
{
	fromMCU=data;
	timer_set(TIME_NOW, 0, NULL);
}

static READ8_HANDLER(mcu_portA_r)
{
	return (fromMCU&ddrA)|(toMCU& ~ddrA);
}

static WRITE8_HANDLER(mcu_ddrA_w)
{
	ddrA=data;
}

static ADDRESS_MAP_START( mcu_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0000) AM_READ(mcu_portA_r) AM_WRITE(mcu_portA_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(mcu_ddrA_w)
	AM_RANGE(0x0010, 0x007f) AM_RAM
	AM_RANGE(0x0080, 0x0fff) AM_ROM
ADDRESS_MAP_END

static const gfx_layout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,8),
	8,
	{RGN_FRAC(0,8),RGN_FRAC(1,8),RGN_FRAC(2,8),RGN_FRAC(3,8),RGN_FRAC(4,8),RGN_FRAC(5,8),RGN_FRAC(6,8),RGN_FRAC(7,8)},
	{ 0, 1, 2,  3,  4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout layout_8x8x3 =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{RGN_FRAC(0,3),RGN_FRAC(1,3),RGN_FRAC(2,3)},
	{ 0, 1, 2,  3,  4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8, 0x000, 1 }, // 8bpp tiles
	{ REGION_GFX2, 0, &layout_8x8x3, 0x100, 32 }, // 3bpp tiles
	{ -1 }
};

static void ctc0_interrupt(int state)
{
	cpunum_set_input_line(1, 0, state);
}

static z80ctc_interface ctc_intf =
{
	1,					// clock
	0,					// timer disables
	ctc0_interrupt,		// interrupt handler
	0,					// ZC/TO0 callback
	0,					// ZC/TO1 callback
	0,					// ZC/TO2 callback
};

static struct z80_irq_daisy_chain daisy_chain_sound[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },	/* device 0 = CTC_1 */
	{ 0, 0, 0, 0, -1 }		/* end mark */
};

static ppi8255_interface ppi8255_intf =
{
	3,
	{ input_port_0_r,     input_port_1_r,	NULL },	/* Port A read */
	{ NULL,             	input_port_2_r,	NULL },	/* Port B read */
	{ NULL,        				protection_r,		NULL },	/* Port C read */
	{ NULL, 							NULL,						NULL },	/* Port A write */
	{ NULL, 							NULL,						NULL },	/* Port B write */
	{ vidctrl_w,          protection_w,		NULL },	/* Port C write */
};

static struct YM2203interface ym2203_interface =
{
	0,
	0,
	0,
	0
};

static PALETTE_INIT(pipeline)
{
	int r,g,b,i,c;
	UINT8 *prom1 = &memory_region(REGION_PROMS)[0x000];
	UINT8 *prom2 = &memory_region(REGION_PROMS)[0x100];

	for(i=0;i<0x100;i++)
	{
		c=prom1[i]|(prom2[i]<<4);
		r=c&7;
		g=(c>>3)&7;
		b=(c>>6)&3;
		r*=36;
		g*=36;
		b*=85;
		palette_set_color(0x100+i, r, g, b);
	}
}

MACHINE_RESET( pipeline )
{
   ctc_intf.baseclock = Machine->drv->cpu[0].cpu_clock;
   z80ctc_init(0, &ctc_intf);
   ppi8255_init(&ppi8255_intf);
}

static MACHINE_DRIVER_START( pipeline )
	/* basic machine hardware */

	MDRV_CPU_ADD(Z80, 7372800/2)
	MDRV_CPU_PROGRAM_MAP(cpu0_mem, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 7372800/2)
	MDRV_CPU_CONFIG(daisy_chain_sound)
	MDRV_CPU_PROGRAM_MAP(cpu1_mem, 0)
	MDRV_CPU_IO_MAP(sound_port, 0)
	/* audio CPU */

	MDRV_CPU_ADD(M68705, 7372800/2)
	MDRV_CPU_PROGRAM_MAP(mcu_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 319, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_MACHINE_RESET(pipeline)

	MDRV_PALETTE_INIT(pipeline)
	MDRV_PALETTE_LENGTH(0x100+0x100)

	MDRV_VIDEO_START(pipeline)
	MDRV_VIDEO_UPDATE(pipeline)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 7372800/4)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)

MACHINE_DRIVER_END

ROM_START( pipeline )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1.u77", 0x00000, 0x08000, CRC(6e928290) SHA1(e2c8c35c04fd8ce3ddd6ecec04b0193a248e4362) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "rom2.u84", 0x00000, 0x08000, CRC(e77c43b7) SHA1(8b04005bc448083a429ace3319fc7e168a61f2f9) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )
	ROM_LOAD( "68705r3.u74", 0x00000, 0x01000, CRC(0550e87f) SHA1(bc051720ee2b1b871c883ab37140380a7b616445) )

	ROM_REGION( 0x18000, REGION_GFX2,ROMREGION_DISPOSE  )
	ROM_LOAD( "rom3.u32", 0x00000, 0x08000, CRC(d065ca46) SHA1(9fd8bb66735195d1cd20420096438abb5cb3fd54) )
	ROM_LOAD( "rom4.u31", 0x08000, 0x08000, CRC(6dc86355) SHA1(4b73e95726f7f244977634a5a152c90acb4ba89f) )
	ROM_LOAD( "rom5.u30", 0x10000, 0x08000, CRC(93f3f82a) SHA1(bc018370efc67a614ef6efa82526225f0db008ac) )

	ROM_REGION( 0x100000, REGION_GFX1,ROMREGION_DISPOSE )
	ROM_LOAD( "rom13.u1", 0x00000, 0x20000,CRC(611d7e01) SHA1(77e83c51b059f6d64009740d2e7e7ac1c8a6c7ec) )
	ROM_LOAD( "rom12.u2", 0x20000, 0x20000,CRC(8933c908) SHA1(9f04e454aacda479b6d6dd84dedbf56855f07fca) )
	ROM_LOAD( "rom11.u3", 0x40000, 0x20000,CRC(4e20e82d) SHA1(507b996ab5c8b8fb88f826d808f4b74dfc770db3) )
	ROM_LOAD( "rom10.u4", 0x60000, 0x20000,CRC(9892e465) SHA1(8789d169128bfc8de449bd617601a0f7fe1a19fb) )
	ROM_LOAD( "rom9.u5",  0x80000, 0x20000,CRC(07d16ca1) SHA1(caecf98284236af0e1f77566a9c6950491d0902a) )
	ROM_LOAD( "rom8.u6",  0xa0000, 0x20000,CRC(4e244c8a) SHA1(6808b2b195601e8041a12fff4b77e487efba015e) )
	ROM_LOAD( "rom7.u7",  0xc0000, 0x20000,CRC(23eb84dd) SHA1(ad1d359ba59087a5e786d262194cfe7db9bb0000) )
	ROM_LOAD( "rom6.u8",  0xe0000, 0x20000,CRC(c34bee64) SHA1(2da2dc6e6615ccc2e3e7ca0ceb735a347923a728) )

	ROM_REGION( 0x00220, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.u10", 0x00000, 0x00100,CRC(e91a1f9e) SHA1(a293023ebe96a5438e89457a98d94beb6dad5418) )
	ROM_LOAD( "82s129.u9",  0x00100, 0x00100,CRC(1cc09f6f) SHA1(35c857e0f3df0dcceec963459978e779e94f76f6) )
	ROM_LOAD( "82s123.u79", 0x00200, 0x00020,CRC(6df3f972) SHA1(0096a7f7452b70cac6c0752cb62e24b643015b5c) )
ROM_END

GAME( 1990, pipeline, 0, pipeline, pipeline, 0, ROT0, "Daehyun Electronics", "Pipeline",GAME_NO_SOUND )

