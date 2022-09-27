/***************************************************************************

Solar Warrior / Xain'd Sleena
Technos, 1986

PCB Layout
----------

Top Board

TA-0019-P1-03
|------------------------------------------------------------------------|
|M51516 YM3014 YM2203 YM2203 68A09 P2-0.49                               |
|       YM3014                                                           |
|                                  6116                                 |--|
|                                                                       |  |
|        2018                      *                                    |  |
|        6148                                                           |  |
|                           PT-0.59                                     |  |
|                                  P3-0.46                              |  |
|                                                                       |  |
|J                                                                      |  |
|A    DSW2                         P4-0.45                              |--|
|M                                                     2018              |
|M                                                                       |
|A                                 P5-0.44                               |
|     DSW1                                                               |
|                                                                       |--|
|                                  P6-0.43                              |  |
|            68B09                                                      |  |
|                                              68B09                    |  |
|                   P9-02.66       P7-0.42                              |  |
|                                                                       |  |
|                                                                       |  |
| 68705             PA-03.65       P8-0.41                              |  |
| (PZ-0.113)                                P1-02.29  P0-02.15   6264   |--|
|                                                                        |
|                                  *                                     |
|------------------------------------------------------------------------|
Notes:
        6809 - Hitachi HD68A09 / HD68B09 CPU, running at 1.500MHz [12/8] (x3, DIP40)
      YM2203 - Yamaha YM2203C sound chip, running at 3.000MHz [12/4] (x2, DIP40)
       68705 - Motorola MC68705P5S Microcontroller, running at 3.000MHz [12/4] (labelled 'PZ-0, DIP28)
           * - Empty socket
        6264 - Hitachi HM6264LP-15 8K x8 SRAM (DIP28)
        2018 - Toshiba TMM2018D-45 2K x8 SRAM (DIP24)
        6148 - Hitachi HM6148HP-45 1K x4 SRAM (DIP18)
        6116 - Hitachi HM6116LP-3 2K x8 SRAM (DIP24)

       VSync - 57Hz

        ROMs - Name         Device       Use
               P9-02.66     TMM24256   \ CPU1 program
               PA-03.65     MBM27256   /

               P0-02.15     TMM24256   \ CPU2 program
               P1-02.29     TMM24256   /

               P2-0.49      TMM24256     Sound CPU program

               P3-0.46      TMM24256   / GFX
               P4-0.45      TMM24256   |
               P5-0.44      TMM24256   |
               P6-0.43      TMM24256   |
               P7-0.42      TMM24256   |
               P8-0.41      TMM24256   /

Bottom Board

TA-0019-P2-03
|------------------------------------------------------------------------|
|    PK-0.136  PC-0.114                                                  |
|    PL-0.135  PD-0.113                                                  |
|                                                                       |--|
|    PM-0.134  PE-0.112                                                 |  |
|    PN-02.133 PF-02.111                                                |  |
|                                                                       |  |
|           2018                                                        |  |
|                                                                       |  |
|    PO-0.131  PG-0.109                                                 |  |
|                                                                       |  |
|    PP-0.130  PH-0.108                                                 |--|
|                                                                  12MHz |
|    PQ-0.129  PI-0.107                                PB-0.24           |
|                                                                        |
|    PR-0.128  PJ-0.106                                6116              |
|                                                                       |--|
|         2018                                                          |  |
|                                                                       |  |
|                                                                       |  |
|                                                                       |  |
|                                                                       |  |
|                                                                       |  |
|         2018                                                   2018   |  |
|                                                                       |--|
|             2018                                                       |
|             2018                                                       |
|------------------------------------------------------------------------|
Notes:
        2018 - Toshiba TMM2018D-45 2K x8 SRAM (DIP24)

        ROMs - Name         Device       Use
               PB-0.24      TMM24256   / GFX
               PC-0.114     TMM24256   |
               PD-0.113     TMM24256   |
               PE-0.112     TMM24256   |
               PF-02.111    MBM27256   |
               PG-0.109     TMM24256   |
               PH-0.108     TMM24256   |
               PI-0.107     TMM24256   |
               PJ-0.106     TMM24256   |
               PK-0.136     TMM24256   |
               PL-0.135     TMM24256   |
               PM-0.134     TMM24256   |
               PN-02.133    MBM27256   |
               PO-0.131     TMM24256   |
               PP-0.130     TMM24256   |
               PQ-0.129     TMM24256   |
               PR-0.128     TMM24256   /


Driver by Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino
Updates by Bryan McPhail, 12/12/2004:
    Fixed NMI & FIRQ handling according to schematics.
    Fixed clock speeds.
    Implemented GFX priority register/priority PROM

    Xain has a semi-bug that shows up in MAME - at 0xa26b there is a tight
    loop that checks for the VBLANK input bit going high.  However at the
    start of a game the VBLANK interrupt routine doesn't return before
    the VBLANK input bit goes low (VBLANK is held high for 8 scanlines only).
    This would cause the emulation to hang, but it would work on the real
    board because the instruction currently being decoded would finish
    before the NMI was taken, so the VBLANK bit and NMI are not actually
    exactly synchronised in practice.  This is currently hacked in MAME
    by raising the VBLANK bit a scanline early.


TODO:
    - 68705 microcontroller not dumped, patched out.

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2203intf.h"

static unsigned char *xain_sharedram;
static int vblank;

VIDEO_UPDATE( xain );
VIDEO_START( xain );
WRITE8_HANDLER( xain_scrollxP0_w );
WRITE8_HANDLER( xain_scrollyP0_w );
WRITE8_HANDLER( xain_scrollxP1_w );
WRITE8_HANDLER( xain_scrollyP1_w );
WRITE8_HANDLER( xain_charram_w );
WRITE8_HANDLER( xain_bgram0_w );
WRITE8_HANDLER( xain_bgram1_w );
WRITE8_HANDLER( xain_flipscreen_w );

extern unsigned char *xain_charram, *xain_bgram0, *xain_bgram1, xain_pri;


static READ8_HANDLER( xain_sharedram_r )
{
	return xain_sharedram[offset];
}

static WRITE8_HANDLER( xain_sharedram_w )
{
	/* locations 003d and 003e are used as a semaphores between CPU A and B, */
	/* so let's resync every time they are changed to avoid deadlocks */
	if ((offset == 0x003d || offset == 0x003e)
			&& xain_sharedram[offset] != data)
		cpu_boost_interleave(0, TIME_IN_USEC(20));
	xain_sharedram[offset] = data;
}

static WRITE8_HANDLER( xainCPUA_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	xain_pri=data&0x7;

	if (data & 0x08) {memory_set_bankptr(1,&RAM[0x10000]);}
	else {memory_set_bankptr(1,&RAM[0x4000]);}
}

static WRITE8_HANDLER( xainCPUB_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (data & 0x01) {memory_set_bankptr(2,&RAM[0x10000]);}
	else {memory_set_bankptr(2,&RAM[0x4000]);}
}

static WRITE8_HANDLER( xain_sound_command_w )
{
	soundlatch_w(offset,data);
	cpunum_set_input_line(2,M6809_IRQ_LINE,HOLD_LINE);
}

static WRITE8_HANDLER( xain_main_irq_w )
{
	switch (offset)
	{
	case 0: /* 0x3a09 - NMI clear */
		cpunum_set_input_line(0,INPUT_LINE_NMI,CLEAR_LINE);
		break;
	case 1: /* 0x3a0a - FIRQ clear */
		cpunum_set_input_line(0,M6809_FIRQ_LINE,CLEAR_LINE);
		break;
	case 2: /* 0x3a0b - IRQ clear */
		cpunum_set_input_line(0,M6809_IRQ_LINE,CLEAR_LINE);
		break;
	case 3: /* 0x3a0c - IRQB assert */
		cpunum_set_input_line(1,M6809_IRQ_LINE,ASSERT_LINE);
		break;
	}
}

static WRITE8_HANDLER( xain_irqA_assert_w )
{
	cpunum_set_input_line(0,M6809_IRQ_LINE,ASSERT_LINE);
}

static WRITE8_HANDLER( xain_irqB_clear_w )
{
	cpunum_set_input_line(1,M6809_IRQ_LINE,CLEAR_LINE);
}

static READ8_HANDLER( xain_68705_r )
{
//  logerror("read 68705\n");
	return 0x4d;	/* fake P5 checksum test pass */
}

static WRITE8_HANDLER( xain_68705_w )
{
//  logerror("write %02x to 68705\n",data);
}

static READ8_HANDLER( xain_input_port_4_r )
{
	return readinputport(4) | vblank;
}

static INTERRUPT_GEN( xain_interrupt )
{
	int scanline=255-cpu_getiloops();

	/* FIRQ (IMS) fires every on every 8th scanline (except 0) */
	if (scanline&0x08)
		cpunum_set_input_line(0, M6809_FIRQ_LINE, ASSERT_LINE);

	/* NMI fires on scanline 248 (VBL) and is latched */
	if (scanline==248)
		cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);

	/* VBLANK input bit is held high from scanlines 248-255 */
	if (scanline>=248-1) // -1 is a hack - see notes above
		vblank=0x20;
	else
		vblank=0;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(xain_sharedram_r)
	AM_RANGE(0x2000, 0x37ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x3a00, 0x3a00) AM_READ(input_port_0_r)
	AM_RANGE(0x3a01, 0x3a01) AM_READ(input_port_1_r)
	AM_RANGE(0x3a02, 0x3a02) AM_READ(input_port_2_r)
	AM_RANGE(0x3a03, 0x3a03) AM_READ(input_port_3_r)
	AM_RANGE(0x3a04, 0x3a04) AM_READ(xain_68705_r)	/* from the 68705 */
	AM_RANGE(0x3a05, 0x3a05) AM_READ(xain_input_port_4_r)
//  AM_RANGE(0x3a06, 0x3a06) AM_READ(MRA8_NOP)  /* ?? read (and discarded) on startup. Maybe reset the 68705 */
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(xain_sharedram_w) AM_BASE(&xain_sharedram)
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(xain_charram_w) AM_BASE(&xain_charram)
	AM_RANGE(0x2800, 0x2fff) AM_WRITE(xain_bgram1_w) AM_BASE(&xain_bgram1)
	AM_RANGE(0x3000, 0x37ff) AM_WRITE(xain_bgram0_w) AM_BASE(&xain_bgram0)
	AM_RANGE(0x3800, 0x397f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x3a00, 0x3a01) AM_WRITE(xain_scrollxP1_w)
	AM_RANGE(0x3a02, 0x3a03) AM_WRITE(xain_scrollyP1_w)
	AM_RANGE(0x3a04, 0x3a05) AM_WRITE(xain_scrollxP0_w)
	AM_RANGE(0x3a06, 0x3a07) AM_WRITE(xain_scrollyP0_w)
	AM_RANGE(0x3a08, 0x3a08) AM_WRITE(xain_sound_command_w)
	AM_RANGE(0x3a09, 0x3a0c) AM_WRITE(xain_main_irq_w)
	AM_RANGE(0x3a0d, 0x3a0d) AM_WRITE(xain_flipscreen_w)
	AM_RANGE(0x3a0e, 0x3a0e) AM_WRITE(xain_68705_w)	/* to 68705 */
	AM_RANGE(0x3a0f, 0x3a0f) AM_WRITE(xainCPUA_bankswitch_w)
	AM_RANGE(0x3c00, 0x3dff) AM_WRITE(paletteram_xxxxBBBBGGGGRRRR_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0x3e00, 0x3fff) AM_WRITE(paletteram_xxxxBBBBGGGGRRRR_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmemB, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(xain_sharedram_r)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writememB, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(xain_sharedram_w)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(xain_irqA_assert_w)
	AM_RANGE(0x2800, 0x2800) AM_WRITE(xain_irqB_clear_w)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(xainCPUB_bankswitch_w)
	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

#if 0
static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END
#endif

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x1000) AM_READ(soundlatch_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2800, 0x2800) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x2801, 0x2801) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(YM2203_write_port_1_w)
	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INPUT_PORTS_START( xsleena )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Game_Time ) )
	PORT_DIPSETTING(    0x0c, "Slow" )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "20k 70k and every 70k" )
	PORT_DIPSETTING(    0x20, "30k 80k and every 80k" )
	PORT_DIPSETTING(    0x10, "20k and 80k" )
	PORT_DIPSETTING(    0x00, "30k and 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "3")
	PORT_DIPSETTING(    0x80, "4")
	PORT_DIPSETTING(    0x40, "6")
	PORT_DIPSETTING(    0x00, "Infinite (Cheat")

	PORT_START_TAG("IN2")
	PORT_BIT( 0x03, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* when 0, 68705 is ready to send data */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )	/* when 1, 68705 is ready to receive data */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* VBLANK */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 8*8 chars */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 2, 4, 6 },	/* plane offset */
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* every char takes 32 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,	/* 8*8 chars */
	4*512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0x8000*4*8+0, 0x8000*4*8+4, 0, 4 },	/* plane offset */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8	/* every char takes 64 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 8 },	/* 8x8 text */
	{ REGION_GFX2, 0, &tilelayout, 256, 8 },	/* 16x16 Background */
	{ REGION_GFX3, 0, &tilelayout, 384, 8 },	/* 16x16 Background */
	{ REGION_GFX4, 0, &tilelayout, 128, 8 },	/* Sprites */
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpunum_set_input_line(2,M6809_FIRQ_LINE,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,irqhandler
};



static MACHINE_DRIVER_START( xsleena )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1500000)	/* Confirmed 1.5MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(xain_interrupt,256)

	MDRV_CPU_ADD(M6809, 1500000)	/* Confirmed 1.5MHz */
	MDRV_CPU_PROGRAM_MAP(readmemB,writememB)

	MDRV_CPU_ADD(M6809, 1500000)	/* Confirmed 1.5MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

//  MDRV_CPU_ADD(M68705, 300000)    /* Confirmed 3MHz */
//  MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(xain)
	MDRV_VIDEO_UPDATE(xain)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 0.40)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
	MDRV_SOUND_ROUTE(2, "mono", 0.50)
	MDRV_SOUND_ROUTE(3, "mono", 0.40)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( xsleena )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s-10.7d",      0x08000, 0x8000, CRC(370164be) SHA1(65c9951cac7dc3943fa4d5f9919ebb4c4f29b3ae) )
	ROM_LOAD( "s-11.7c",      0x04000, 0x4000, CRC(d22bf859) SHA1(9edb159bef2eba2c5d93c03c15fbcb87eea52236) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "s-2.3b",       0x08000, 0x8000, CRC(a1a860e2) SHA1(fb2b152bfafc44608039774436ddf3b17eed979c) )
	ROM_LOAD( "s-1.2b",       0x04000, 0x4000, CRC(948b9757) SHA1(3ea840cc47ae6a66f3e5f6a2f3e88475dcfe1840) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

//  ROM_REGION( 0x800, REGION_CPU4, 0 )
//  ROM_LOAD( "pz-0.113",       0x000, 0x800, CRC(0) SHA1(0) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "s-24.13i",     0x18000, 0x8000, CRC(d37939e0) SHA1(301d9f6720857c64a4e070444a07a38138ddd4ef) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "s-16.13g",     0x38000, 0x8000, CRC(e59a2f27) SHA1(4643cea85f8613c36b416f46f9d1753fa9839237) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) ) /* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* Priority */
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END

ROM_START( xsleenab )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.rom",        0x08000, 0x8000, CRC(79f515a7) SHA1(e61f18e3639dd9afe16c7bcb90fa7be31905e2c6) )
	ROM_LOAD( "s-11.7c",      0x04000, 0x4000, CRC(d22bf859) SHA1(9edb159bef2eba2c5d93c03c15fbcb87eea52236) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "s-2.3b",       0x08000, 0x8000, CRC(a1a860e2) SHA1(fb2b152bfafc44608039774436ddf3b17eed979c) )
	ROM_LOAD( "s-1.2b",       0x04000, 0x4000, CRC(948b9757) SHA1(3ea840cc47ae6a66f3e5f6a2f3e88475dcfe1840) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "s-24.13i",     0x18000, 0x8000, CRC(d37939e0) SHA1(301d9f6720857c64a4e070444a07a38138ddd4ef) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "s-16.13g",     0x38000, 0x8000, CRC(e59a2f27) SHA1(4643cea85f8613c36b416f46f9d1753fa9839237) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) ) /* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* Priority */
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END

ROM_START( solarwar )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p9-0.bin",     0x08000, 0x8000, CRC(8ff372a8) SHA1(0fc396e662419fb9cb5bea11748aa8e0e8d072e6) )
	ROM_LOAD( "pa-0.bin",     0x04000, 0x4000, CRC(154f946f) SHA1(25b776eb9c494e5302795ae79e494cbfc7c104b1) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "p1-0.bin",     0x08000, 0x8000, CRC(f5f235a3) SHA1(9f57dd7c5e514afa750edc6da6d263bf1e913c14) )
	ROM_LOAD( "p0-0.bin",     0x04000, 0x4000, CRC(51ae95ae) SHA1(e03f7ccb0b33b05547577c60a7f92dc75e24b4d6) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

//  ROM_REGION( 0x800, REGION_CPU4, 0 )
//  ROM_LOAD( "pz-0.113",       0x000, 0x800, CRC(0) SHA1(0) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "pn-0.bin",     0x18000, 0x8000, CRC(d2ed6f94) SHA1(155a0d1d978f07517400d0c602fc40657f8569dc) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "pf-0.bin",     0x38000, 0x8000, CRC(6e627a77) SHA1(1d16031acd53c9e691ae7eac8a6f1ae3954fac8c) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) )	/* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* Priority */
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END



DRIVER_INIT( xsleena )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* do the same patch as the bootleg xsleena */
	RAM[0xd488] = 0x12;
	RAM[0xd489] = 0x12;
	RAM[0xd48a] = 0x12;
	RAM[0xd48b] = 0x12;
	RAM[0xd48c] = 0x12;
	RAM[0xd48d] = 0x12;
}

DRIVER_INIT( solarwar )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* do the same patch as the bootleg xsleena */
	RAM[0xd47e] = 0x12;
	RAM[0xd47f] = 0x12;
	RAM[0xd480] = 0x12;
	RAM[0xd481] = 0x12;
	RAM[0xd482] = 0x12;
	RAM[0xd483] = 0x12;
}



GAME( 1986, xsleena,  0,       xsleena, xsleena, xsleena,  ROT0, "Technos", "Xain'd Sleena", 0 )
GAME( 1986, xsleenab, xsleena, xsleena, xsleena, 0,        ROT0, "bootleg", "Xain'd Sleena (bootleg)", 0 )
GAME( 1986, solarwar, xsleena, xsleena, xsleena, solarwar, ROT0, "[Technos] Taito (Memetron license)", "Solar-Warrior", 0 )
