/*
 Meijinsen (snk/alpha)
 ---------------------

 driver by Tomasz Slanina

It's something between typical alpha 68k hardware
(alpha mcu, sound hw (same as in jongbou)) and
old alpha shougi hardware (framebuffer).

There's probably no upright cabinet, only
cocktail table (controls in 2p mode are inverted).

Buttons:
 1st = 'decision'
 2nd = 'promotion'

 Service switch  = memory clear

--------------------------------------------------



                        p8      p7
   16mhz                p6      p5
                5816    p4      p3
                5816    p2      p1
                 ?
                          68000-8

        4416 4416 4416 4416
             clr             8910
     z80 p9 p10 2016



5816 = Sony CXK5816-10L (Ram for video)
2016 = Toshiba TMM2016AP-10 (SRAM for sound)
4416 = TI TMS4416-15NL (DRAM for MC68000)
clr  = TI TBP18S030N (32*8 Bipolar PROM)
Z80  = Sharp LH0080A Z80A-CPU-D
8910 = GI AY-3-8910A (Sound chip)
?    = Chip with Surface Scratched ....

"0" "1" MC68000 Program ROMs:
p1  p2
p3  p4
p5  p6
p7  p8

P9  = Z80 Program
P10 = AY-3-8910A Sounds

Text inside P9:

ALPHA DENSHI CO.,LTD  JUNE / 24 / 1986  FOR
* SHOUGI * GAME USED  SOUND BOARD CONTROL
SOFT  PSG & VOICE  BY M.C & S.H

*/
#include "driver.h"
#include "vidhrdw/res_net.h"
#include "sound/ay8910.h"

static UINT16 *shared_ram;
static unsigned deposits1=0, deposits2=0, credits=0;

static WRITE16_HANDLER( sound_w )
{
	if(ACCESSING_LSB)
		soundlatch_w(0, data&0xff);
}

static READ16_HANDLER( alpha_mcu_r )
{
	static unsigned coinvalue=0;
	static UINT8 coinage1[2][2]={{1,1},{1,2}};
	static UINT8 coinage2[2][2]={{1,5},{2,1}};

	static int latch;
	int source=shared_ram[offset];

	switch (offset)
	{
		case 0: /* Dipswitch 2 */
			shared_ram[0] = (source&0xff00)|readinputportbytag("IN2");
			return 0;

		case 0x22: /* Coin value */
			shared_ram[0x22] = (source&0xff00)|(credits&0x00ff);
			return 0;

		case 0x29: /* Query microcontroller for coin insert */

			credits=0;

			if ((readinputportbytag("IN3")&0x3)==3) latch=0;

			if ((readinputportbytag("IN3")&0x1)==0 && !latch)
			{
				shared_ram[0x29] = (source&0xff00)|(0x22);	// coinA
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;

				coinvalue = (~readinputportbytag("IN2")>>3) & 1;

				deposits1++;
				if (deposits1 == coinage1[coinvalue][0])
				{
					credits = coinage1[coinvalue][1];
					deposits1 = 0;
				}
				else
					credits = 0;
			}
			else if ((readinputportbytag("IN3")&0x2)==0 && !latch)
			{
				shared_ram[0x29] = (source&0xff00)|(0x22);	// coinA
				shared_ram[0x22] = (source&0xff00)|0x0;
				latch=1;

				coinvalue = (~readinputportbytag("IN2")>>3) & 1;

				deposits2++;
				if (deposits2 == coinage2[coinvalue][0])
				{
					credits = coinage2[coinvalue][1];
					deposits2 = 0;
				}
				else
					credits = 0;
			}
			else
			{
				shared_ram[0x29] = (source&0xff00)|0x22;
			}
			return 0;
	}
	return 0;
}



static ADDRESS_MAP_START( meijinsn_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080e00, 0x080fff) AM_READ(alpha_mcu_r) AM_WRITENOP
	AM_RANGE(0x100000, 0x107fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&videoram16)
	AM_RANGE(0x180000, 0x180dff) AM_RAM
	AM_RANGE(0x180e00, 0x180fff) AM_RAM AM_BASE(&shared_ram)
	AM_RANGE(0x181000, 0x181fff) AM_RAM
	AM_RANGE(0x1c0000, 0x1c0001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x1a0000, 0x1a0001) AM_WRITE(sound_w) AM_READ(input_port_0_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( meijinsn_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( meijinsn_sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(soundlatch_clear_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

INPUT_PORTS_START( meijinsn )
PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1        ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2        ) PORT_PLAYER(1)
	PORT_BIT( 0x7cc0, IP_ACTIVE_HIGH, IPT_UNKNOWN        )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1        ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2        ) PORT_PLAYER(2)
	PORT_BIT( 0xc0ff, IP_ACTIVE_HIGH, IPT_UNKNOWN        )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x07, 0x00, "Game time (actual game)" )
	PORT_DIPSETTING(    0x07, "1:00" )
	PORT_DIPSETTING(    0x06, "2:00" )
	PORT_DIPSETTING(    0x05, "3:00" )
	PORT_DIPSETTING(    0x04, "4:00" )
	PORT_DIPSETTING(    0x03, "5:00" )
	PORT_DIPSETTING(    0x02, "10:00" )
	PORT_DIPSETTING(    0x01, "20:00" )
	PORT_DIPSETTING(    0x00, "0:30" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, "A 1C/1C B 1C/5C" )
	PORT_DIPSETTING(    0x00, "A 1C/2C B 2C/1C" )
	PORT_DIPNAME( 0x10, 0x00, "2 Player" )
	PORT_DIPSETTING(    0x00, "1C" )
	PORT_DIPSETTING(    0x10, "2C" )
	PORT_DIPNAME( 0x20, 0x00, "Game time (tsumeshougi)" )
	PORT_DIPSETTING(    0x20, "1:00" )
	PORT_DIPSETTING(    0x00, "2:00" )

	PORT_START_TAG("IN3")  /* Coin input to microcontroller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END

VIDEO_START(meijinsn)
{
	return 0;
}

PALETTE_INIT( meijinsn )
{
	int i;
	static const int resistances_b[2]  = { 470, 220 };
	static const int resistances_rg[3] = { 1000, 470, 220 };
	double weights_r[3], weights_g[3], weights_b[2];


	compute_resistor_weights(0,	255,	-1.0,
			3,	resistances_rg,	weights_r,	0,	1000+1000,
			3,	resistances_rg,	weights_g,	0,	1000+1000,
			2,	resistances_b,	weights_b,	0,	1000+1000);

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = combine_3_weights(weights_r, bit0, bit1, bit2);

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = combine_3_weights(weights_g, bit0, bit1, bit2);

		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = combine_2_weights(weights_b, bit0, bit1);

		palette_set_color(i,r,g,b);
	}
}


VIDEO_UPDATE(meijinsn)
{
	int offs;

	for (offs = 0;offs <0x4000; offs++)
	{
		int sx, sy, x, data1, data2, color, data;

		sx = offs >> 8;
		sy = offs & 0xff;

		data1 = videoram16[offs]>>8;
		data2 = videoram16[offs]&0xff;

		for (x=0; x<4; x++)
		{
			color= ((data1>>x) & 1) | (((data1>>(4+x)) & 1)<<1);
			data = ((data2>>x) & 1) | (((data2>>(4+x)) & 1)<<1);
			plot_pixel(bitmap, (sx*4 + (3-x)), sy, color*4 + data);
		}
	}
}


static INTERRUPT_GEN( meijinsn_interrupt )
{
	if (cpu_getiloops() == 0)
		cpunum_set_input_line(0, 1, HOLD_LINE);
	else
		cpunum_set_input_line(0, 2, HOLD_LINE);
}

static struct AY8910interface ay8910_interface =
{
	soundlatch_r
};

MACHINE_RESET( meijinsn )
{
	deposits1 = 0;
	deposits2 = 0;
	credits   = 0;
}


static MACHINE_DRIVER_START( meijinsn )
	MDRV_CPU_ADD_TAG("main", M68000, 9000000 )
	MDRV_CPU_PROGRAM_MAP(meijinsn_map, 0)
	MDRV_CPU_VBLANK_INT(meijinsn_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(meijinsn_sound_map,0)
	MDRV_CPU_IO_MAP(meijinsn_sound_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 160)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(meijinsn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(12, 243, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_PALETTE_INIT(meijinsn)

	MDRV_VIDEO_START(meijinsn)
	MDRV_VIDEO_UPDATE(meijinsn)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

MACHINE_DRIVER_END


ROM_START( meijinsn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p1", 0x00000, 0x04000, CRC(8c9697a3) SHA1(19258e20a6aaadd6ba3469079fef85bc6dba548c) )
	ROM_CONTINUE   (       0x20000,  0x4000 )
	ROM_LOAD16_BYTE( "p2", 0x00001, 0x04000, CRC(f7da3535) SHA1(fdbacd075d45abda782966b16b3ea1ad68d60f91) )
	ROM_CONTINUE   (       0x20001,  0x4000 )
	ROM_LOAD16_BYTE( "p3", 0x08000, 0x04000, CRC(0af0b266) SHA1(d68ed31bc932bc5e9c554b2c0df06a751dc8eb96) )
	ROM_CONTINUE   (       0x28000,  0x4000 )
	ROM_LOAD16_BYTE( "p4", 0x08001, 0x04000, CRC(aab159c5) SHA1(0c9cad8f9893f4080b498433068e9324c7f2e13c) )
	ROM_CONTINUE   (       0x28001,  0x4000 )
	ROM_LOAD16_BYTE( "p5", 0x10000, 0x04000, CRC(0ed10a47) SHA1(9e89ec69f1f4e1ffa712f2e0c590d067c8c63026) )
	ROM_CONTINUE   (       0x30000,  0x4000 )
	ROM_LOAD16_BYTE( "p6", 0x10001, 0x04000, CRC(60b58755) SHA1(1786fc1b4c6d1793fb8e9311356fa4119611cfae) )
	ROM_CONTINUE   (       0x30001,  0x4000 )
	ROM_LOAD16_BYTE( "p7", 0x18000, 0x04000, CRC(604c76f1) SHA1(37fdf904f5e4d69dc8cb711cf3dece8f3075254a) )
	ROM_CONTINUE   (       0x38000,  0x4000 )
	ROM_LOAD16_BYTE( "p8", 0x18001, 0x04000, CRC(e3eaef19) SHA1(b290922f252a790443109e5023c3c35b133275cc) )
	ROM_CONTINUE   (       0x38001,  0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "p9", 0x00000, 0x04000, CRC(aedfefdf) SHA1(f9d35737a0e942fe7d483f87c52efa92a1bbb3e5) )
	ROM_LOAD( "p10",0x04000, 0x04000, CRC(93b4d764) SHA1(4fedd3fd1f3ef6c5f60ca86219f877df68d3027d) )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* Colour PROM */
	ROM_LOAD( "clr", 0x00, 0x20, CRC(7b95b5a7) SHA1(c15be28bcd6f5ffdde659f2d352ae409f04b2557) )
ROM_END

GAME( 1986, meijinsn, 0, meijinsn, meijinsn, 0, ROT0, "SNK Electronics corp.", "Meijinsen", 0 )
