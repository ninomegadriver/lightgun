/*********************************************************************
 Erotictac/Tactic [Sisteme 1992]
 (title depends on "Sexy Views" DIP)

 driver by
  Tomasz Slanina
  Steve Ellenoff
    Nicola Salmoria

TODO:
 - sound
    my guess:
     - data (samples?) in mem range 0000-$0680 ($1f00000 - $1f00680)
     - 4 chn?? ( masks $000000ff, $0000ff00, $00ff0000, $ff000000)
     - INT7 (bit 1 in IRQRQB) = sound hw 'ready' flag (checked at $56c)
     - writes to 3600000 - 3ffffff related to snd hardware

 - game doesn't work with 'demo sound' disabled
 - get rid of dirty hack in DRIVER_INIT (bug in the ARM core?)
 - is screen double buffered ?
 - flashing text in test mode

  Poizone PCB appears to be a Jamam Acorn Archimedes based system,
   Poizone was also released on the AA computer.

**********************************************************************/
#include "driver.h"
#include "cpu/arm/arm.h"

static UINT32 *ertictac_mainram;
static UINT32 *ram;
static UINT32 IRQSTA, IRQMSKA, IRQMSKB, FIQMSK, T1low, T1high;
static UINT32 vidFIFO[256];

static READ32_HANDLER(ram_r)
{
	return ram[offset];
}

static WRITE32_HANDLER(ram_w)
{
	COMBINE_DATA(&ram[offset]);
	if(offset>=vidFIFO[0x88]/4 && offset<( (vidFIFO[0x88]/4) + 0x28000/4))
	{
		int tmp=offset-vidFIFO[0x88]/4;
		int x=(tmp%80)<<2;
		int y=(tmp/80)&0xff;

		plot_pixel(tmpbitmap, x++, y, (ram[offset]&0xff));
		plot_pixel(tmpbitmap, x++, y, ((ram[offset]>>8)&0xff));
		plot_pixel(tmpbitmap, x++, y, ((ram[offset]>>16)&0xff));
		plot_pixel(tmpbitmap, x, y, ((ram[offset]>>24)&0xff));
	}
}

static WRITE32_HANDLER(video_fifo_w)
{
	vidFIFO[data>>24]=data&0xffffff;
}

static READ32_HANDLER(IOCR_r)
{
	return (input_port_5_r(0)&0x80)|0x34;
}

static WRITE32_HANDLER(IOCR_w)
{
	//ignored
}


static READ32_HANDLER(IRQSTA_r)
{
	return (IRQSTA&(~2))|0x80;
}

static READ32_HANDLER(IRQRQA_r)
{
	return (IRQSTA&IRQMSKA)|0x80;
}

static WRITE32_HANDLER(IRQRQA_w)
{
	if(ACCESSING_LSB32)
		IRQSTA&=~data;
}

static READ32_HANDLER(IRQMSKA_r)
{
	return IRQMSKA;
}

static WRITE32_HANDLER(IRQMSKA_w)
{
	if(ACCESSING_LSB32)
		IRQMSKA=(data&(~2))|0x80;
}

static READ32_HANDLER(IRQRQB_r)
{
	return rand()&IRQMSKB; /* hack  0x20 - controls,  0x02 - ?sound? */
}

static READ32_HANDLER(IRQMSKB_r)
{
	return IRQMSKB;
}

static WRITE32_HANDLER(IRQMSKB_w)
{
	if(ACCESSING_LSB32)
		IRQMSKB=data;
}

static READ32_HANDLER(FIQMSK_r)
{
	return FIQMSK;
}

static WRITE32_HANDLER(FIQMSK_w)
{
	if(ACCESSING_LSB32)
		FIQMSK=(data&(~0x2c))|0x80;
}

static READ32_HANDLER(T1low_r)
{
	return T1low;
}

static WRITE32_HANDLER(T1low_w)
{
	if(ACCESSING_LSB32)
		T1low=data;
}

static READ32_HANDLER(T1high_r)
{
	return T1high;
}

static WRITE32_HANDLER(T1high_w)
{
	if(ACCESSING_LSB32)
		T1high=data;
}

static void startTimer(void);

static void ertictacTimer(int val)
{
	IRQSTA|=0x40;
	if(IRQMSKA&0x40)
	{
		cpunum_set_input_line(0, ARM_IRQ_LINE, HOLD_LINE);
	}
	startTimer();
}

static void startTimer(void)
{
	timer_set(TIME_IN_USEC( ((T1low&0xff)|((T1high&0xff)<<8))>>4), 0, ertictacTimer);
}

static WRITE32_HANDLER(T1GO_w)
{
	startTimer();
}

static ADDRESS_MAP_START( ertictac_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE (&ertictac_mainram)
	AM_RANGE(0x01f00000, 0x01ffffff) AM_READWRITE(ram_r, ram_w) AM_BASE (&ram)
	AM_RANGE(0x03200000, 0x03200003) AM_READWRITE(IOCR_r, IOCR_w)
	AM_RANGE(0x03200010, 0x03200013) AM_READ(IRQSTA_r)
	AM_RANGE(0x03200014, 0x03200017) AM_READWRITE(IRQRQA_r, IRQRQA_w)
	AM_RANGE(0x03200018, 0x0320001b) AM_READWRITE(IRQMSKA_r, IRQMSKA_w)
	AM_RANGE(0x03200024, 0x03200027) AM_READ(IRQRQB_r)
	AM_RANGE(0x03200028, 0x0320002b) AM_READWRITE(IRQMSKB_r, IRQMSKB_w)
	AM_RANGE(0x03200038, 0x0320003b) AM_READWRITE(FIQMSK_r, FIQMSK_w)

	AM_RANGE(0x03200050, 0x03200053) AM_READWRITE(T1low_r, T1low_w)
	AM_RANGE(0x03200054, 0x03200057) AM_READWRITE(T1high_r, T1high_w)
	AM_RANGE(0x03200058, 0x0320005b) AM_WRITE( T1GO_w )

	AM_RANGE(0x03340000, 0x03340003) AM_NOP
	AM_RANGE(0x03340010, 0x03340013) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x03340014, 0x03340017) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x03340018, 0x0334001b) AM_READ(input_port_1_dword_r)

	AM_RANGE(0x033c0004, 0x033c0007) AM_READ(input_port_3_dword_r)
	AM_RANGE(0x033c0008, 0x033c000b) AM_READ(input_port_4_dword_r)
	AM_RANGE(0x033c0010, 0x033c0013) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x033c0014, 0x033c0017) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x033c0018, 0x033c001b) AM_READ(input_port_1_dword_r)

	AM_RANGE(0x03400000, 0x03400003) AM_WRITE(video_fifo_w)
	AM_RANGE(0x03800000, 0x03ffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

INPUT_PORTS_START( ertictac )
	PORT_START_TAG("IN 0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN 1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)

	PORT_START_TAG("IN 2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sound" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "Test Mode" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x30, "Game Timing" )
	PORT_DIPSETTING(    0x30, "Normal Game" )
	PORT_DIPSETTING(    0x20, "3.0 Minutes" )
	PORT_DIPSETTING(    0x10, "2.5 Minutes" )
	PORT_DIPSETTING(    0x00, "2.0 Minutes" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x05, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_DIPNAME( 0x0a, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_4C ) )

	PORT_DIPNAME( 0x10, 0x00, "Sexy Views" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START_TAG("dummy")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )


INPUT_PORTS_END

static MACHINE_RESET( ertictac )
{
	ertictac_mainram[0]=0xeae00007; //reset vector
}

static INTERRUPT_GEN( ertictac_interrupt )
{
	IRQSTA|=0x08;
	if(IRQMSKA&0x08)
	{
		cpunum_set_input_line(0, ARM_IRQ_LINE, HOLD_LINE);
	}
}

PALETTE_INIT(ertictac)
{
	int c;

	for (c = 0; c < 256; c++)
	{
 		int r,g,b,i;

 		i = c & 0x03;
 		r = ((c & 0x04) >> 0) | ((c & 0x10) >> 1) | i;
 		g = ((c & 0x20) >> 3) | ((c & 0x40) >> 3) | i;
 		b = ((c & 0x08) >> 1) | ((c & 0x80) >> 4) | i;

 		palette_set_color(c, r * 0x11, g * 0x11, b * 0x11);
	}
}

static MACHINE_DRIVER_START( ertictac )

	MDRV_CPU_ADD(ARM, 16000000) /* guess */
	MDRV_CPU_PROGRAM_MAP(ertictac_map,0)
	MDRV_CPU_VBLANK_INT(ertictac_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(ertictac)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(ertictac)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

ROM_START( ertictac )
	ROM_REGION(0x800000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "01", 0x00000, 0x10000, CRC(8dce677c) SHA1(9f12b1febe796038caa1ecad1d17864dc546cfd8) )
	ROM_LOAD32_BYTE( "02", 0x00001, 0x10000, CRC(7c5c916c) SHA1(d4ed5fc3a7b27253551e7d9176ed9e6513092e60) )
	ROM_LOAD32_BYTE( "03", 0x00002, 0x10000, CRC(edca5ac6) SHA1(f6c4b8030f3c1c93922c5f7232f2159e0471b93a) )
	ROM_LOAD32_BYTE( "04", 0x00003, 0x10000, CRC(959be81c) SHA1(3e8eaacf8809863fd712ad605d23fdda4e904aee) )
	ROM_LOAD32_BYTE( "05", 0x40000, 0x10000, CRC(d08a6c89) SHA1(17b0f5fb2094126146b09d69c91bf413737b0c9e) )
	ROM_LOAD32_BYTE( "06", 0x40001, 0x10000, CRC(d727bcd8) SHA1(4627f8d4d6e6f323445b3ffcfc0d9c699a9a4f89) )
	ROM_LOAD32_BYTE( "07", 0x40002, 0x10000, CRC(23b75de2) SHA1(e18f5339ca2dd362298784ff3e5479d780d823f8) )
	ROM_LOAD32_BYTE( "08", 0x40003, 0x10000, CRC(9448ccdf) SHA1(75fe3c31625f8ba1eedd806b52993e92b1f585b6) )
	ROM_LOAD32_BYTE( "09", 0x80000, 0x10000, CRC(2bfb312e) SHA1(af7a4a1926c9c3d0b3ad41a4729a17383581c070) )
	ROM_LOAD32_BYTE( "10", 0x80001, 0x10000, CRC(e7a05477) SHA1(0ec6f94a35b87e1e4529dea194fce1fde9a5b0ad) )
	ROM_LOAD32_BYTE( "11", 0x80002, 0x10000, CRC(3722591c) SHA1(e0c4075bc4b1c90a6decba3005a1f8298bd61bd1) )
	ROM_LOAD32_BYTE( "12", 0x80003, 0x10000, CRC(fe022b7e) SHA1(056f7283bc54eff555fd1db91410c020fd905063) )
	ROM_LOAD32_BYTE( "13", 0xc0000, 0x10000, CRC(83550842) SHA1(0fee78dbf13ba970e0544c48f8939550f9347822) )
	ROM_LOAD32_BYTE( "14", 0xc0001, 0x10000, CRC(3029567c) SHA1(6d49bea3a3f6f11f4182a602d37b53f1f896c154) )
	ROM_LOAD32_BYTE( "15", 0xc0002, 0x10000, CRC(500997ab) SHA1(028c7b3ca03141e5b596ab1e2ab98d0ccd9bf93a) )
	ROM_LOAD32_BYTE( "16", 0xc0003, 0x10000, CRC(70a8d136) SHA1(50b11f5701ed5b79a5d59c9a3c7d5b7528e66a4d) )
ROM_END

ROM_START( poizone )
	ROM_REGION(0x800000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "p_son01.bin", 0x00000, 0x10000, CRC(28793c9f) SHA1(2d9f7d667203e745b47cd2cc97501ae961ae1a66) )
	ROM_LOAD32_BYTE( "p_son02.bin", 0x00001, 0x10000, CRC(2d4b6f4b) SHA1(8df2680d6e5dc41787b3a72e594f01f5e732d0ec) )
	ROM_LOAD32_BYTE( "p_son03.bin", 0x00002, 0x10000, CRC(0834d46e) SHA1(bf1cc9b47759ef39ed8fd8f334ed8f2902be3bf8) )
	ROM_LOAD32_BYTE( "p_son04.bin", 0x00003, 0x10000, CRC(9e9b1f6e) SHA1(d95067f3ecca3c079a67bd0b80e3b45c5b42151e) )
	ROM_LOAD32_BYTE( "p_son05.bin", 0x40000, 0x10000, CRC(be62ad42) SHA1(5eb51ad277ec7b7f1b5995bcdea35114f805baae) )
	ROM_LOAD32_BYTE( "p_son06.bin", 0x40001, 0x10000, CRC(c2f9141c) SHA1(e910fefcd6f0b99ab299b3a5f099b9ef84e1cc23) )
	ROM_LOAD32_BYTE( "p_son07.bin", 0x40002, 0x10000, CRC(8929c748) SHA1(35c108170590fbe97fdd4a1db7d660b4ee0adac8) )
	ROM_LOAD32_BYTE( "p_son08.bin", 0x40003, 0x10000, CRC(0ef5b14f) SHA1(425f130b2a94a4152fab763e0734e71f2913b25f) )
	ROM_LOAD32_BYTE( "p_son09.bin", 0x80000, 0x10000, CRC(e8cd75a6) SHA1(386a4ff576574e49711e72640dd3f33c8b7e04b3) )
	ROM_LOAD32_BYTE( "p_son10.bin", 0x80001, 0x10000, CRC(1dc01da7) SHA1(d37456e3407cab5eff5bbd9735c3a54e73b27545) )
	ROM_LOAD32_BYTE( "p_son11.bin", 0x80002, 0x10000, CRC(85e973ad) SHA1(850cd0dbda42eab78625038c6ea1f5b31674018a) )
	ROM_LOAD32_BYTE( "p_son12.bin", 0x80003, 0x10000, CRC(b89376d1) SHA1(cff29c2a8db88d4d104bae19a90de034158fe9e7) )

	ROM_LOAD32_BYTE( "p_son21.bin", 0x140000, 0x10000, CRC(a0c06c1e) SHA1(8d065117788e96ecd147d3d7ffdd273d4b69bb7a) )
	ROM_LOAD32_BYTE( "p_son22.bin", 0x140001, 0x10000, CRC(16f0bb52) SHA1(893ab1e72b84de7a38f88f9d713769968ebd4553) )
	ROM_LOAD32_BYTE( "p_son23.bin", 0x140002, 0x10000, CRC(e9c118b2) SHA1(110d9a204e701b9b54d89f027f8892c3f3a819c7) )
	ROM_LOAD32_BYTE( "p_son24.bin", 0x140003, 0x10000, CRC(a09d7f55) SHA1(e0d562c655c16034b40db93de801b98b7948beb2) )
ROM_END

static DRIVER_INIT( ertictac )
{
	((UINT32 *)memory_region(REGION_USER1))[0x55]=0;// patched TSTS r11,r15,lsl #32  @ $3800154
}


static DRIVER_INIT( poizone )
{
	((UINT32 *)memory_region(REGION_USER1))[0x21C/4]=0;// patched TSTS r11,r15,lsl #32  @ $380021C
}


GAME( 1990, ertictac, 0, ertictac, ertictac, ertictac, ROT0, "Sisteme", "Erotictac/Tactic" ,GAME_NO_SOUND)
GAME( 1991, poizone,  0, ertictac, ertictac, poizone,  ROT0, "Eterna" ,"Poizone" ,GAME_NO_SOUND|GAME_NOT_WORKING)

