/*************************************************************************

    X the Ball

    driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "vidhrdw/tlc34076.h"
#include "machine/ticket.h"
#include "sound/dac.h"



/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT16 *vram_bg, *vram_fg;
static int display_start;
static UINT8 bitvals[32];



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( xtheball )
{
	tlc34076_reset(6);
	ticket_dispenser_init(100, 1, 1);
}



/*************************************
 *
 *  Video update
 *
 *************************************/

static VIDEO_UPDATE( xtheball )
{
	UINT8 *src0 = (UINT8 *)vram_bg;
	UINT8 *src1 = (UINT8 *)vram_fg;
	int x, y;

	/* check for disabled video */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dst = (UINT16 *)bitmap->line[y];

		/* bit value 0x13 controls which foreground mode to use */
		if (!bitvals[0x13])
		{
			/* mode 0: foreground is the same as background */
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int pix = src1[BYTE_XOR_LE((y * 0x200 + x) & 0x1ffff)];
				if (!pix)
					pix = src0[BYTE_XOR_LE(y * 0x200 + x)];
				dst[x] = pix;
			}
		}
		else
		{
			/* mode 1: foreground is half background resolution in */
			/* X and supports two pages */
			int srcbase = display_start / 16;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int pix = src1[BYTE_XOR_LE((srcbase + y * 0x100 + x/2) & 0x1ffff)];
				if (!pix)
					pix = src0[BYTE_XOR_LE(y * 0x200 + x)];
				dst[x] = pix;
			}
		}
	}
}



/*************************************
 *
 *  Callback for display address change
 *
 *************************************/

static void xtheball_display_addr_changed(UINT32 offs, int rowbytes, int scanline)
{
	logerror("display_start = %X\n", offs);
	force_partial_update(scanline - 1);
	display_start = offs;
}



/*************************************
 *
 *  Shift register transfers
 *
 *************************************/

static void xtheball_to_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	if (address >= 0x01000000 && address <= 0x010fffff)
		memcpy(shiftreg, &vram_bg[TOWORD(address & 0xff000)], TOBYTE(0x1000));
	else if (address >= 0x02000000 && address <= 0x020fffff)
		memcpy(shiftreg, &vram_fg[TOWORD(address & 0xff000)], TOBYTE(0x1000));
	else
		logerror("%08X:xtheball_to_shiftreg(%08X)\n", activecpu_get_pc(), address);
}


static void xtheball_from_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	if (address >= 0x01000000 && address <= 0x010fffff)
		memcpy(&vram_bg[TOWORD(address & 0xff000)], shiftreg, TOBYTE(0x1000));
	else if (address >= 0x02000000 && address <= 0x020fffff)
		memcpy(&vram_fg[TOWORD(address & 0xff000)], shiftreg, TOBYTE(0x1000));
	else
		logerror("%08X:xtheball_from_shiftreg(%08X)\n", activecpu_get_pc(), address);
}



/*************************************
 *
 *  Sound data access
 *
 *************************************/

static WRITE16_HANDLER( dac_w )
{
	if (ACCESSING_MSB)
		DAC_data_w(0, data >> 8);
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE16_HANDLER( bit_controls_w )
{
	if (ACCESSING_LSB)
	{
		if (bitvals[offset] != (data & 1))
		{
			logerror("%08x:bit_controls_w(%x,%d)\n", activecpu_get_pc(), offset, data & 1);

			switch (offset)
			{
				case 7:
					ticket_dispenser_w(0, data << 7);
					break;

				case 8:
					set_led_status(0, data & 1);
					break;

				case 0x13:
					force_partial_update(cpu_getscanline());
					break;
			}
		}
		bitvals[offset] = data & 1;
	}
//  ui_popup("%d%d%d%d-%d%d%d%d--%d%d%d%d-%d%d%d%d",
/*
        bitvals[0],
        bitvals[1],
        bitvals[2],
        bitvals[3],
        bitvals[4],     // meter
        bitvals[5],
        bitvals[6],     // tickets
        bitvals[7],     // tickets
        bitvals[8],     // start lamp
        bitvals[9],     // lamp
        bitvals[10],    // lamp
        bitvals[11],    // lamp
        bitvals[12],    // lamp
        bitvals[13],    // lamp
        bitvals[14],    // lamp
        bitvals[15]     // lamp
*/
/*      bitvals[16],
        bitvals[17],
        bitvals[18],
        bitvals[19],    // video foreground control
        bitvals[20],
        bitvals[21],
        bitvals[22],
        bitvals[23],
        bitvals[24],
        bitvals[25],
        bitvals[26],
        bitvals[27],
        bitvals[28],
        bitvals[29],
        bitvals[30],
        bitvals[31]
*/
//  );
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static READ16_HANDLER( port0_r )
{
	int result = readinputport(0);
	result ^= ticket_dispenser_r(0) >> 3;
	return result;
}


static READ16_HANDLER( analogx_r )
{
	return (readinputportbytag("ANALOGX") << 8) | 0x00ff;
}


static READ16_HANDLER( analogy_watchdog_r )
{
	/* doubles as a watchdog address */
	watchdog_reset_w(0,0);
	return (readinputportbytag("ANALOGY") << 8) | 0x00ff;
}



/*************************************
 *
 *  Main CPU memory map
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x0001ffff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x01000000, 0x010fffff) AM_RAM AM_BASE(&vram_bg)
	AM_RANGE(0x02000000, 0x020fffff) AM_RAM AM_BASE(&vram_fg)
	AM_RANGE(0x03000000, 0x030000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0x03040000, 0x030401ff) AM_WRITE(bit_controls_w)
	AM_RANGE(0x03040080, 0x0304008f) AM_READ(port0_r)
	AM_RANGE(0x03040100, 0x0304010f) AM_READ(analogx_r)
	AM_RANGE(0x03040110, 0x0304011f) AM_READ(input_port_1_word_r)
	AM_RANGE(0x03040130, 0x0304013f) AM_READ(input_port_2_word_r)
	AM_RANGE(0x03040140, 0x0304014f) AM_READ(input_port_3_word_r)
	AM_RANGE(0x03040150, 0x0304015f) AM_READ(input_port_4_word_r)
	AM_RANGE(0x03040160, 0x0304016f) AM_READ(input_port_5_word_r)
	AM_RANGE(0x03040170, 0x0304017f) AM_READ(input_port_6_word_r)
	AM_RANGE(0x03040180, 0x0304018f) AM_READ(analogy_watchdog_r)
	AM_RANGE(0x03060000, 0x0306000f) AM_WRITE(dac_w)
	AM_RANGE(0x04000000, 0x057fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( xtheball )
	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0700, 0x0000, "Target Tickets")
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPSETTING(      0x0300, "6" )
	PORT_DIPSETTING(      0x0400, "7" )
	PORT_DIPSETTING(      0x0500, "8" )
	PORT_DIPSETTING(      0x0600, "9" )
	PORT_DIPSETTING(      0x0700, "10" )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x1000, 0x1000, "Automatic 1 Ticket" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x1000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x2000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Game Tune" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x4000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x8000, DEF_STR( On ))

	/* service mode is funky:
        hit F2 to enter bookkeeping mode; hit service1 (9) to exit
        hold service 1 (9) and hit F2 to enter test mode; hit service2 (0) to exit
    */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_SERVICE_NO_TOGGLE( 0x0001, IP_ACTIVE_HIGH )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("ANALOGX")
	PORT_BIT( 0x00ff, 0x0080, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START_TAG("ANALOGY")
	PORT_BIT( 0x00ff, 0x0080, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)
INPUT_PORTS_END



/*************************************
 *
 *  34010 configuration
 *
 *************************************/

static struct tms34010_config tms_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	xtheball_to_shiftreg,			/* write to shiftreg function */
	xtheball_from_shiftreg,			/* read from shiftreg function */
	xtheball_display_addr_changed,	/* display address changed */
	0								/* display interrupt callback */
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( xtheball )

	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_PERIODIC_INT(irq1_line_pulse,TIME_IN_HZ(15000))

	MDRV_MACHINE_RESET(xtheball)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (256 - 224)) / (60 * 256))
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512,256)
	MDRV_VISIBLE_AREA(0,511, 0,221)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(xtheball)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( xtheball )
	ROM_REGION16_LE( 0x10000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "aces18-lo.ic13",  0x000000, 0x8000, CRC(c0e80004) SHA1(d79c2e7301857148674fffad349c7a2a98fa1ee2) )
	ROM_LOAD16_BYTE( "aces18-hi.ic19",  0x000001, 0x8000, CRC(5a682f92) SHA1(ed77c02cbdcff9eac32760cee67e3a784efacac7) )

	ROM_REGION16_LE( 0x300000, REGION_USER2, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "xtb-ic6.bin", 0x000000, 0x40000, CRC(a3cc01b8) SHA1(49d42bb17c314609f371df7d7ace57e54fdf6335) )
	ROM_LOAD16_BYTE( "xtb-ic7.bin", 0x000001, 0x40000, CRC(8dfa6c1b) SHA1(a32940b3f9501a44e1d1ef1628f8a64b32aa2183) )
	ROM_LOAD16_BYTE( "xtb-1l.ic8",  0x100000, 0x80000, CRC(df52c00f) SHA1(9a89d780ad394b55ce9540a5743bbe571543288f) )
	ROM_LOAD16_BYTE( "xtb-1h.ic9",  0x100001, 0x80000, CRC(9ce7785b) SHA1(9a002ba492cdc35391df2b55dfe90c55b7d48ad1) )
	ROM_LOAD16_BYTE( "xtb-2l.ic10", 0x200000, 0x80000, CRC(e2545a19) SHA1(c2fe5adf7a174cec6aad63baa1b92761ef21d5a4) )
	ROM_LOAD16_BYTE( "xtb-2h.ic11", 0x200001, 0x80000, CRC(50c27558) SHA1(ecfb7d918868d35a8cde45f7d04fdfc3ffc06328) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1991, xtheball, 0, xtheball, xtheball, 0,  ROT0, "Rare", "X the Ball", 0 )
