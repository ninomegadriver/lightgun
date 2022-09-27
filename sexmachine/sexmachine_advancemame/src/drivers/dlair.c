/* the way I hooked up the CTC is most likely completely wrong */

#include "driver.h"
#include "machine/z80ctc.h"
#include "cpu/z80/z80daisy.h"



/*
   Dragon's Lair has two 7 segment LEDs on the board, used to report error
   codes.
   The association between the bits of the port and the led segments is:

    ---0---
   |       |
   5       1
   |       |
    ---6---
   |       |
   4       2
   |       |
    ---3---

   bit 7 = enable (0 = display off)

   Error codes for led 0:
   1 bad CPU
   2 bad ROM
   3 bad RAM a000-a7ff
   4 bad RAM c000-c7ff
   5 bad I/O ports 0-3
   P ?
 */

static int led0,led1;

WRITE8_HANDLER( dlair_led0_w )
{
	led0 = data;
}
WRITE8_HANDLER( dlair_led1_w )
{
	led1 = data;
}

VIDEO_UPDATE( dlair )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs-=2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 32;
			sy = (offs/2) / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs+1],
					0,
					0,0,
					8*sx,16*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

/*
if (led0 & 128)
{
if ((led0 & 1) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    8,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 2) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    16,8,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 4) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    16,24,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 8) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    8,32,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 16) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    0,24,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 32) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    0,8,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led0 & 64) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    8,16,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
if (led1 & 128)
{
if ((led1 & 1) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+8,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 2) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+16,8,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 4) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+16,24,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 8) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+8,32,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 16) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+0,24,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 32) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+0,8,&Machine->visible_area,TRANSPARENCY_NONE,0);
if ((led1 & 64) == 0) drawgfx(bitmap,olduifont,'x',0,0,0,
    32+8,16,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
*/
}



/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpunum_set_input_line(0, 0, state);
}

static z80ctc_interface ctc_intf =
{
	0,              /* clock (filled in from the CPU 0 clock */
	0,              /* timer disables */
	ctc_interrupt,  /* interrupt handler */
	0,              /* ZC/TO0 callback */
	0,              /* ZC/TO1 callback */
	0               /* ZC/TO2 callback */
};


MACHINE_RESET( dlair )
{
   /* initialize the CTC */
   ctc_intf.baseclock = Machine->drv->cpu[0].cpu_clock;
   z80ctc_init(0, &ctc_intf);
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xa000, 0xa7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xc400, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(dlair_led0_w)
	AM_RANGE(0xe008, 0xe008) AM_WRITE(dlair_led1_w)
	AM_RANGE(0xe030, 0xe030) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END

static unsigned char pip[4];
static READ8_HANDLER( pip_r )
{
logerror("PC %04x: read I/O port %02x\n",activecpu_get_pc(),offset);
	return pip[offset];
}
static WRITE8_HANDLER( pip_w )
{
logerror("PC %04x: write %02x to I/O port %02x\n",activecpu_get_pc(),data,offset);
	pip[offset] = data;
z80ctc_0_w(offset,data);
}

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_READ(pip_r)
//  AM_RANGE(0x80, 0x83) AM_READ(z80ctc_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_WRITE(pip_w)
//  AM_RANGE(0x80, 0x83) AM_WRITE(z80ctc_0_w)
ADDRESS_MAP_END



INPUT_PORTS_START( dlair )
	PORT_START
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,16,
	512,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every char takes 8 consecutive bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,  0, 8 },
	{ -1 } /* end of array */
};



static struct z80_irq_daisy_chain daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 }, /* CTC number 0 */
	{ 0,0,0,0,-1} 		/* end mark */
};



static MACHINE_DRIVER_START( dlair )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ? */
	MDRV_CPU_CONFIG(daisy_chain)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(dlair)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(dlair)

	/* sound hardware */
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dlair )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u45",          0x0000, 0x2000, CRC(329b354a) SHA1(54bbc5aa647d3c20166a57f9d3aa5569e7289af8) )
	ROM_LOAD( "u46",          0x2000, 0x2000, CRC(8479612b) SHA1(b5543a06928274bde0e1bdda0747d936feaff177) )
	ROM_LOAD( "u47",          0x4000, 0x2000, CRC(6a66f6b4) SHA1(2bee981870e61977565439c34568952043656cfa) )
	ROM_LOAD( "u48",          0x6000, 0x2000, CRC(36575106) SHA1(178e26e7d5c7f879bc55c2fb170f3bb47a709610) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u33",          0x0000, 0x2000, CRC(e7506d96) SHA1(610ae25bd8db13b18b9e681e855ffa978043255b) )
ROM_END



GAME( 1983, dlair, 0, dlair, dlair, 0, ROT0, "Cinematronics", "Dragon's Lair", GAME_NOT_WORKING | GAME_NO_SOUND )

