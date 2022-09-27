/*

Trivia Quest
Sunn/Techstar 1984

CPU: 6809
Three SY6522 - for flashing lighted buttons?

Sound: Two AY-3-8910

Forty eight! MK4027 4K Rams

Six 6116 Rams with battery backup

Two Crystals:
11.6688 Mhz
6 Mhz

Two 8 position DIPS

rom3 thru rom7 - Main pcb PRG.

roma thru romi - Sub pcb Questions.

The main pcb had empty sockets for
rom0, rom1 and rom2.
This pcb has been tested and works
as is.


 driver by Pierpaolo Prazzoli

TODO:
- Where are inputs ?!


Graphics hardware is the same used in gameplan.c

*/

#include "driver.h"
#include "machine/6522via.h"
#include "sound/ay8910.h"

static int cmd,portA;
static UINT8 xpos,ypos;

static int trvquest_question = 0;

static WRITE8_HANDLER( trvquest_video_portA_w )
{
	portA = data;
}

static WRITE8_HANDLER( trvquest_video_portB_w )
{
	cmd = data & 7;
}

static WRITE8_HANDLER( trvquest_video_CA2_w )
{
	if (data != 0) return;

	switch (cmd)
	{
		case 0:	// draw pixel
			if (portA & 0x10)	// auto increment X
			{
				if (portA & 0x40)
					xpos--;
				else
					xpos++;
			}
			if (portA & 0x20)	// auto increment Y
			{
				if (portA & 0x80)
					ypos--;
				else
					ypos++;
			}

			plot_pixel(tmpbitmap, xpos, ypos, Machine->pens[portA & 7]);
			break;

		case 1:	// load X register
			xpos = portA;
			break;

		case 2:	// load Y register
			ypos = portA;
			break;

		case 3:	// clear screen
			fillbitmap(tmpbitmap, Machine->pens[portA & 7], NULL);
			break;
	}
}

static WRITE8_HANDLER( trvquest_question_w )
{
	trvquest_question = data;
}

static READ8_HANDLER( trvquest_question_r )
{
	return memory_region(REGION_USER1)[trvquest_question * 0x2000 + offset];
}

static READ8_HANDLER( trvquest_vblank_r )
{
	int pc = activecpu_get_pc();

	if(pc != 0xE52C)
		printf("read @ %X\n",pc);

	return 0x20;
}

static READ8_HANDLER( read_rand )
{
	return rand();
}


static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x1000, 0x10ff) AM_WRITENOP
	AM_RANGE(0x2700, 0x27ff) AM_RAM

	AM_RANGE(0x3800, 0x380f) AM_READWRITE(via_0_r, via_0_w)
	AM_RANGE(0x3810, 0x381f) AM_READWRITE(via_1_r, via_1_w)
	AM_RANGE(0x3820, 0x382f) AM_READWRITE(via_2_r, via_2_w)

	AM_RANGE(0x3830, 0x3830) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x3831, 0x3831) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x3840, 0x3840) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x3841, 0x3841) AM_WRITE(AY8910_control_port_1_w)

	AM_RANGE(0x3850, 0x3850) AM_READ(read_rand)

	AM_RANGE(0x8000, 0x9fff) AM_READ(trvquest_question_r)

	AM_RANGE(0xa000, 0xa000) AM_WRITE(trvquest_question_w)
	AM_RANGE(0xa000, 0xa000) AM_READNOP	// read because the game reads 2 bytes at time for questions roms tag

	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( trvquest )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static void via2_irq(int state)
{
	cpunum_set_input_line(0, 0, state);
}

static struct via6522_interface via_0_interface =
{
	/*inputs : A/B         */ NULL, NULL,
	/*inputs : CA/B1,CA/B2 */ NULL, NULL, NULL, NULL,
	/*outputs: A/B         */ NULL, NULL,
	/*outputs: CA/B1,CA/B2 */ NULL, NULL, NULL, NULL,
	/*irq                  */ NULL
};

static struct via6522_interface via_1_interface =
{
	/*inputs : A/B         */ NULL, NULL,
	/*inputs : CA/B1,CA/B2 */ NULL, NULL, NULL, NULL,
	/*outputs: A/B         */ NULL, NULL,
	/*outputs: CA/B1,CA/B2 */ NULL, NULL, NULL, NULL,
	/*irq                  */ NULL
};

static struct via6522_interface via_2_interface =
{
	/*inputs : A/B         */ NULL, trvquest_vblank_r,
	/*inputs : CA/B1,CA/B2 */ NULL, NULL, NULL, NULL,
	/*outputs: A/B         */ trvquest_video_portA_w, trvquest_video_portB_w,
	/*outputs: CA/B1,CA/B2 */ NULL, NULL, trvquest_video_CA2_w, NULL,
	/*irq                  */ via2_irq
};

static PALETTE_INIT( trvquest )
{
	palette_set_color(0,0x00,0x00,0x00); /* 0 BLACK   */
	palette_set_color(1,0xff,0x00,0x00); /* 1 RED     */
	palette_set_color(2,0x00,0xff,0x00); /* 2 GREEN   */
	palette_set_color(3,0xff,0xff,0x00); /* 3 YELLOW  */
	palette_set_color(4,0x00,0x00,0xff); /* 4 BLUE    */
	palette_set_color(5,0xff,0x00,0xff); /* 5 MAGENTA */
	palette_set_color(6,0x00,0xff,0xff); /* 6 CYAN    */
	palette_set_color(7,0xff,0xff,0xff); /* 7 WHITE   */
}

static MACHINE_RESET( trvquest )
{
	via_config(0, &via_0_interface);
	via_config(1, &via_1_interface);
	via_config(2, &via_2_interface);
	via_reset();
}

static INTERRUPT_GEN( trvquest_interrupt )
{
	via_2_ca1_w(0,1);
	via_2_ca1_w(0,0);
}

static MACHINE_DRIVER_START( trvquest )
	MDRV_CPU_ADD(M6809,6000000)
	MDRV_CPU_PROGRAM_MAP(cpu_map,0)
	MDRV_CPU_VBLANK_INT(trvquest_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_1fill)
	MDRV_MACHINE_RESET(trvquest)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(trvquest)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( trvquest )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom3", 0xb000, 0x1000, CRC(2ff7f370) SHA1(66f40426ed02ee44235e17a49d9054ede42b83b9) )
	ROM_LOAD( "rom4", 0xc000, 0x1000, CRC(b1adebcb) SHA1(661cabc92b1defce5c2edb8e873a80d5032084d0) )
	ROM_LOAD( "rom5", 0xd000, 0x1000, CRC(2fc10a15) SHA1(8ecce32a5a167056c8fb48554a8907ae6299921e) )
	ROM_LOAD( "rom6", 0xe000, 0x1000, CRC(fabf4846) SHA1(862cac32de78f2ff4afef398b864d5533d302a4f) )
	ROM_LOAD( "rom7", 0xf000, 0x1000, CRC(a9f56551) SHA1(fb6fc3b17a6e66571a5ba837befbfac1ac26cc39) )

	ROM_REGION( 0x18000, REGION_USER1, ROMREGION_ERASEFF ) /* Question roms */
	/* 0x00000 - 0x07fff empty */
	ROM_LOAD( "romi", 0x06000, 0x2000, CRC(c8368f69) SHA1(c1dfb701482c5ae922df0a93665a519995a2f4f1) )
	ROM_LOAD( "romh", 0x08000, 0x2000, CRC(f3aa8a08) SHA1(2bf8f878cc1df84806a6fb8e7be2656c422d61b9) )
	ROM_LOAD( "romg", 0x0a000, 0x2000, CRC(f85f8e48) SHA1(38c9142181a8ee5c0bc80cf2a06d4137fcb2a8b9) )
	ROM_LOAD( "romf", 0x0c000, 0x2000, CRC(2bffdcab) SHA1(96bd9aede5a76f9ddcf29e8df2c632075d21b8f6) )
	ROM_LOAD( "rome", 0x0e000, 0x2000, CRC(3ff66402) SHA1(da13fe6b99d7517ad2ecd0e42d0c306d4e49563a) )
	ROM_LOAD( "romd", 0x10000, 0x2000, CRC(4e21653f) SHA1(719a8dda9b81963a6b6d7d3e4966ecde676b9ecf) )
	ROM_LOAD( "romc", 0x12000, 0x2000, CRC(081a5322) SHA1(09e7ea5f1ee1dc35ec00bcea1550c6fe0bbdf60d) )
	ROM_LOAD( "romb", 0x14000, 0x2000, CRC(819ab451) SHA1(78c181eae63d55d1d0643bb7be07ca3cdbe14285) )
	ROM_LOAD( "roma", 0x16000, 0x2000, CRC(b4bcaf33) SHA1(c6b08fb8d55b2834d0c6c5baff9f544c795e4c15) )
ROM_END


GAME( 1984, trvquest, 0, trvquest, trvquest, 0, ROT90, "Sunn / Techstar", "Trivia Quest", GAME_NOT_WORKING )
