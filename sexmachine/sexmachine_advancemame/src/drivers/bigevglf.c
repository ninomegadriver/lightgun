/***************************************************************************
Big Event Golf (c) Taito 1986

driver by Jaroslaw Burczynski
          Tomasz Slanina


****************************************************************************


Taito 1986  M4300056B

K1100156A Sound
J1100068A
                                YM2149     2016
                                MSM5232    A67-16
                                Z80        A67-17
                                           A67-18
                                8MHz
-----------------------------------------------------
K1100215A CPU
J1100066A

2064
A67-21
A67-20      A67-03
Z80         A67-04
            A67-05
            A67-06                          2016
            A67-07
            A67-08
            A67-09
            A67-10-2          2016
                              A67-11
            10MHz             Z80
                                   A67-19-1 (68705P5)
----------------------------------------------------
K1100215A VIDEO
J1100072A

                                 41464            2148
  93422                          41464            2148
  93422                          41464            2148
  93422                          41464
  93422                          41464
                                 41464
                                 41464
                                 41464


       A67-12-1                2016
       A67-13-1
       A67-14-1     2016
       A67-15-1                      18.432MHz

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/msm5232.h"

VIDEO_START( bigevglf );
VIDEO_UPDATE( bigevglf );

READ8_HANDLER( bigevglf_vidram_r );
WRITE8_HANDLER( bigevglf_vidram_w );
WRITE8_HANDLER( bigevglf_vidram_addr_w );

READ8_HANDLER( bigevglf_68705_portA_r );
WRITE8_HANDLER( bigevglf_68705_portA_w );
READ8_HANDLER( bigevglf_68705_portB_r );
WRITE8_HANDLER( bigevglf_68705_portB_w );
READ8_HANDLER( bigevglf_68705_portC_r );
WRITE8_HANDLER( bigevglf_68705_portC_w );
WRITE8_HANDLER( bigevglf_68705_ddrA_w );
WRITE8_HANDLER( bigevglf_68705_ddrB_w );
WRITE8_HANDLER( bigevglf_68705_ddrC_w );

WRITE8_HANDLER( bigevglf_mcu_w );
WRITE8_HANDLER( bigevglf_mcu_set_w );
READ8_HANDLER( bigevglf_mcu_r );
READ8_HANDLER( bigevglf_mcu_status_r );

WRITE8_HANDLER( beg_gfxcontrol_w );
WRITE8_HANDLER( beg_palette_w );

extern UINT8 *beg_spriteram1;
extern UINT8 *beg_spriteram2;

static UINT32 beg_bank=0;
UINT8 *beg_sharedram;

static int sound_nmi_enable=0,pending_nmi=0;
static UINT8 for_sound = 0;
static UINT8 from_sound = 0;
static UINT8 sound_state = 0;

static WRITE8_HANDLER( beg_banking_w )
{
	beg_bank = data;

/* d0-d3 connect to A11-A14 of the ROMs (via ls273 latch)
   d4-d7 select one of ROMs (via ls273(above) and then ls154)
*/
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + 0x800*(beg_bank&0xff)); /* empty sockets for IC37-IC44 ROMS */
}

static void from_sound_latch_callback(int param)
{
	from_sound = param&0xff;
	sound_state |= 2;
}
static WRITE8_HANDLER(beg_fromsound_w)	/* write to D800 sets bit 1 in status */
{
	timer_set(TIME_NOW, (activecpu_get_pc()<<16)|data, from_sound_latch_callback);
}

static READ8_HANDLER(beg_fromsound_r)
{
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);
	return from_sound;
}

static READ8_HANDLER(beg_soundstate_r)
{
	UINT8 ret = sound_state;
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);
	sound_state &= ~2; /* read from port 21 clears bit 1 in status */
	return ret;
}

static READ8_HANDLER(soundstate_r)
{
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);
	return sound_state;
}

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpunum_set_input_line(2,INPUT_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
	sound_state &= ~1;
}
static WRITE8_HANDLER( sound_command_w )	/* write to port 20 clears bit 0 in status */
{
	for_sound = data;
	timer_set(TIME_NOW,data,nmi_callback);
}

static READ8_HANDLER( sound_command_r )	/* read from D800 sets bit 0 in status */
{
	sound_state |= 1;
	return for_sound;
}

static WRITE8_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE8_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpunum_set_input_line(2,INPUT_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

static UINT8 beg13_ls74[2];

static void deferred_ls74_w( int param )
{
	int offs = (param>>8) & 255;
	int data = param & 255;
	beg13_ls74[offs] = data;
}

/* do this on a timer to let the CPUs synchronize */
static WRITE8_HANDLER (beg13A_clr_w)
{
	timer_set(TIME_NOW, (0<<8) | 0, deferred_ls74_w);
}
static WRITE8_HANDLER (beg13B_clr_w)
{
	timer_set(TIME_NOW, (1<<8) | 0, deferred_ls74_w);
}
static WRITE8_HANDLER (beg13A_set_w)
{
	timer_set(TIME_NOW, (0<<8) | 1, deferred_ls74_w);
}
static WRITE8_HANDLER (beg13B_set_w)
{
	timer_set(TIME_NOW, (1<<8) | 1, deferred_ls74_w);
}

static READ8_HANDLER( beg_status_r )
{
/* d0 = Q of 74ls74 IC13(partA)
   d1 = Q of 74ls74 IC13(partB)
   d2 =
   d3 =
   d4 =
   d5 =
   d6 = d7 = 10MHz/2

*/
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);
	return (beg13_ls74[0]<<0) | (beg13_ls74[1]<<1);
}


static READ8_HANDLER(  beg_sharedram_r )
{
	return beg_sharedram[offset];
}
static WRITE8_HANDLER( beg_sharedram_w )
{
	beg_sharedram[offset] = data;
}

INPUT_PORTS_START( bigevglf )

	PORT_START	/* port 00 on sub cpu */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START	/* port 04 on sub cpu - bit 0 and bit 1 are coin inputs */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* port 05 on sub cpu */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
 	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
 	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_2C ) )

	PORT_START	/* port 06 on sub cpu */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x0c, 0x0c, "Holes" )
	PORT_DIPSETTING(    0x0c, "3" )
 	PORT_DIPSETTING(    0x08, "2" )
 	PORT_DIPSETTING(    0x04, "1" )
 	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, "Title" )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
 	PORT_DIPSETTING(    0x10, DEF_STR( Japanese ) )
 	PORT_DIPNAME( 0xe0, 0xa0, "Full game price (credits)" )
	PORT_DIPSETTING(    0xe0, "3" )
	PORT_DIPSETTING(    0xc0, "4" )
	PORT_DIPSETTING(    0xa0, "5" )
	PORT_DIPSETTING(    0x80, "6" )
	PORT_DIPSETTING(    0x60, "7" )
	PORT_DIPSETTING(    0x40, "8" )
	PORT_DIPSETTING(    0x20, "9" )
	PORT_DIPSETTING(    0x00, "10" )

	PORT_START  /* TRACKBALL X - port 02 on sub cpu */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X  ) PORT_SENSITIVITY(30) PORT_KEYDELTA(10)

	PORT_START  /* TRACKBALL Y - port 03 on sub cpu */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_REVERSE
INPUT_PORTS_END


/*****************************************************************************/
/* Main CPU */
static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xcfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xd800, 0xdbff) AM_READ(beg_sharedram_r) /* only half of the RAM is accessible, line a10 of IC73 (6116) is GNDed */
	AM_RANGE(0xf000, 0xf0ff) AM_READ(bigevglf_vidram_r) /* 41464 (64kB * 8 chips), addressed using ports 1 and 5 */
	AM_RANGE(0xf840, 0xf8ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xcfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xd800, 0xdbff) AM_WRITE(beg_sharedram_w) AM_BASE(&beg_sharedram)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(beg_palette_w) AM_BASE(&paletteram)
	AM_RANGE(0xe800, 0xefff) AM_WRITE(MWA8_RAM) AM_BASE(&beg_spriteram1) /* sprite 'templates' */
	AM_RANGE(0xf000, 0xf0ff) AM_WRITE(bigevglf_vidram_w)
	AM_RANGE(0xf840, 0xf8ff) AM_WRITE(MWA8_RAM) AM_BASE(&beg_spriteram2)  /* spriteram (x,y,offset in spriteram1,palette) */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bigevglf_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(MWA8_NOP) 	/* video ram enable ???*/
	AM_RANGE(0x01, 0x01) AM_WRITE(beg_gfxcontrol_w)  /* plane select */
	AM_RANGE(0x02, 0x02) AM_WRITE(beg_banking_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(beg13A_set_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(beg13B_clr_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(bigevglf_vidram_addr_w)	/* video banking (256 banks) for f000-f0ff area */
ADDRESS_MAP_END

static ADDRESS_MAP_START( bigevglf_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x06, 0x06) AM_READ(beg_status_r)
ADDRESS_MAP_END


/*********************************************************************************/
/* Sub CPU */

static ADDRESS_MAP_START( readmem_sub, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(beg_sharedram_r) /* shared with main CPU */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sub, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(beg_sharedram_w) /* shared with main CPU */
ADDRESS_MAP_END


static ADDRESS_MAP_START( bigevglf_sub_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x08, 0x08) AM_WRITE(MWA8_NOP) /*coinlockout_w ???? watchdog ???? */
	AM_RANGE(0x0c, 0x0c) AM_WRITE(bigevglf_mcu_w)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(MWA8_NOP) /* 0-enable MCU, 1-keep reset line ASSERTED; D0 goes to the input of ls74 and the /Q of this ls74 goes to reset line on 68705 */
	AM_RANGE(0x10, 0x17) AM_WRITE(beg13A_clr_w)
	AM_RANGE(0x18, 0x1f) AM_WRITE(beg13B_set_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(sound_command_w)
ADDRESS_MAP_END



static READ8_HANDLER( sub_cpu_mcu_coin_port_r )
{
	static int bit5=0;
	/*
            bit 0 and bit 1 = coin inputs
            bit 3 and bit 4 = MCU status
            bit 5           = must toggle, vblank ?

    */
	bit5 ^= 0x20;
	return bigevglf_mcu_status_r(0) | (readinputport(1) & 3) | bit5; /* bit 0 and bit 1 - coin inputs */
}

static ADDRESS_MAP_START( bigevglf_sub_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(MRA8_NOP)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_4_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_5_r)
	AM_RANGE(0x04, 0x04) AM_READ(sub_cpu_mcu_coin_port_r)
	AM_RANGE(0x05, 0x05) AM_READ(input_port_2_r)
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)
	AM_RANGE(0x07, 0x07) AM_READ(MRA8_NOP)
	AM_RANGE(0x0b, 0x0b) AM_READ(bigevglf_mcu_r)
	AM_RANGE(0x20, 0x20) AM_READ(beg_fromsound_r)
	AM_RANGE(0x21, 0x21) AM_READ(beg_soundstate_r)
ADDRESS_MAP_END


/*********************************************************************************/
/* Sound CPU */

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xda00, 0xda00) AM_READ(soundstate_r)
	AM_RANGE(0xd800, 0xd800) AM_READ(sound_command_r)	/* read from D800 sets bit 0 in status */
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_NOP)	/* space for diagnostics ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc800, 0xc800) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc801, 0xc801) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xca00, 0xca0d) AM_WRITE(MSM5232_0_w)
	AM_RANGE(0xcc00, 0xcc00) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xce00, 0xce00) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xd800, 0xd800) AM_WRITE(beg_fromsound_w)	/* write to D800 sets bit 1 in status */
	AM_RANGE(0xda00, 0xda00) AM_WRITE(nmi_enable_w)
	AM_RANGE(0xdc00, 0xdc00) AM_WRITE(nmi_disable_w)
	AM_RANGE(0xde00, 0xde00) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END


/*********************************************************************************/
/* MCU */

static ADDRESS_MAP_START( m68705_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(bigevglf_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(bigevglf_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(bigevglf_68705_portC_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( m68705_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(bigevglf_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(bigevglf_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(bigevglf_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(bigevglf_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(bigevglf_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(bigevglf_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static const gfx_layout gfxlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0, RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4)},
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout,   0x20*16, 16 },
	{ -1 }	/* end of array */
};

MACHINE_RESET( bigevglf )
{
	beg13_ls74[0] = 0;
	beg13_ls74[1] = 0;
}


static struct MSM5232interface msm5232_interface =
{
	{ 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6 }	/* 0.65 (???) uF capacitors */
};

static MACHINE_DRIVER_START( bigevglf )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/2)		/* 5 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(bigevglf_readport,bigevglf_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* vblank */

	MDRV_CPU_ADD(Z80,10000000/2)		/* 5 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem_sub,writemem_sub)
	MDRV_CPU_IO_MAP(bigevglf_sub_readport,bigevglf_sub_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* vblank */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */		/* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ???;
        2 irqs/frame give good music tempo but also SOUND ERROR in test mode,
        4 irqs/frame give SOUND OK in test mode but music seems to be running too fast */

	MDRV_CPU_ADD(M68705,2000000)	/* ??? */
	MDRV_CPU_PROGRAM_MAP(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - interleaving is forced on the fly */

	MDRV_MACHINE_RESET(bigevglf)
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_VIDEO_START(bigevglf)
	MDRV_VIDEO_UPDATE(bigevglf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 8000000/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15) /* YM2149 really */

	MDRV_SOUND_ADD(MSM5232, 8000000/4)
	MDRV_SOUND_CONFIG(msm5232_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bigevglf )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )
	ROM_LOAD( "a67-21",   0x00000, 0x8000, CRC(2a62923d) SHA1(7b025180e203f268ae4d11baa18096e0a5704f77))
	ROM_LOAD( "a67-20",   0x08000, 0x4000, CRC(841561b1) SHA1(5d91449e135ef22508194a9543343c29e1c496cf))
	ROM_LOAD( "a67-03",   0x10000, 0x8000, CRC(695b3396) SHA1(2a3738e6bc492a4c68d2e15a2e474f7654aeb94d))
	ROM_LOAD( "a67-04",   0x18000, 0x8000, CRC(b8941902) SHA1(a03e432cbd8ea1df7223ea99ff1db220a57fc698))
	ROM_LOAD( "a67-05",   0x20000, 0x8000, CRC(681f5f4f) SHA1(2a5d8eeaf6ac697d5d4ee15164b6c4b1b81d7a29))
	ROM_LOAD( "a67-06",   0x28000, 0x8000, CRC(026f6fe5) SHA1(923b7d8363e587ef20b6518bee968d378166c76b))
	ROM_LOAD( "a67-07",   0x30000, 0x8000, CRC(27706bed) SHA1(da702c5a098eb106332996ec5d0e2c014782031e))
	ROM_LOAD( "a67-08",   0x38000, 0x8000, CRC(e922023a) SHA1(ea4c4b5e2f82ab20afb15f115e8cbc66d8471927))
	ROM_LOAD( "a67-09",   0x40000, 0x8000, CRC(a9d4263e) SHA1(8c3f2d541583e8e4b22e0beabcd04c5765508535))
	ROM_LOAD( "a67-10",   0x48000, 0x8000, CRC(7c35f4a3) SHA1(de60dc991c67fb7d48314bc8b2c1a27ad040bf1e))

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "a67-11",   0x00000, 0x4000, CRC(a2660d20) SHA1(3d8b670c071862d677d4e30655dcb6b8d830648a))

 	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a67-16",   0x0000, 0x4000, CRC(5fb6d22e) SHA1(1701aa94b7f524187fd7213a94535bed3e8b6cc9))
	ROM_LOAD( "a67-17",   0x4000, 0x4000, CRC(9f57deae) SHA1(dbdb3d77c3de0113ef6671aec854e4e44ee162ef))
	ROM_LOAD( "a67-18",   0x8000, 0x4000, CRC(40d54fed) SHA1(bfa0922809bffafec15d3ef59ac8b8ad653860a1))

	ROM_REGION( 0x0800, REGION_CPU4, 0 )
	ROM_LOAD( "a67_19-1", 0x0000, 0x0800, CRC(25691658) SHA1(aabf47abac43abe2ffed18ead1cb94e587149e6e))

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a67-12",   0x00000, 0x8000, CRC(980cc3c5) SHA1(c35c20cfff0b5cfd5b95742333ae20a2c93371b5))
	ROM_LOAD( "a67-13",   0x08000, 0x8000, CRC(ad6e04af) SHA1(4680d789cf53c4808105ad4f3c70aedb6d8bcf36))
	ROM_LOAD( "a67-14",   0x10000, 0x8000, CRC(d6708cce) SHA1(5b48f9dff2a3e28242dc2004469dc2ac2b5d0321))
	ROM_LOAD( "a67-15",   0x18000, 0x8000, CRC(1d261428) SHA1(0f3e6d83a8a462436fa414de4e1e4306db869d3e))
ROM_END

GAME( 1986, bigevglf,  0,        bigevglf,  bigevglf,  0, ROT270, "Taito America Corporation", "Big Event Golf", 0)
