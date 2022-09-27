/***************************************************************************

    Meadows S2650 driver

    driver by J. Buchmueller, June '98

    Games supported:
        * Dead Eye
        * Gypsy Juggler
        * Inferno

    Known issues:
        * none at this time

****************************************************************************

    ***********************************************
    memory map CPU #0 (preliminary)
    ***********************************************

    0000..0bff  ROM part one

    0c00..0c03  H/W input ports
    -----------------------------------------------
            0 R control buttons
                D0      button 1
                D1      start 2 player game

            1 R analog control
                D0-D7   center is 0x7f

            2 R horizontal sync divider chain
                D7 9.765kHz ... D0 2.5MHz

            3 R dip switch settings
                D0-D2   select 2 to 9 coins
                D3-D4   Coins per play D3 D4
                        1 coin  1 play  0  0
                        2 coins 1 play  1  0
                        1 coin  2 plays 0  1
                        free play       1  1
                D5      Attact music 0:off, 1:on
                D6-D7   Extended play  D6 D7
                        none            0  0
                        5000 pts        1  0
                        15000 pts       0  1
                        35000 pts       1  1

    0d00-0d0f   H/W sprites
    -----------------------------------------------
            0 W D0-D7    sprite 0 horz
            1 W D0-D7    sprite 1 horz
            2 W D0-D7    sprite 2 horz
            3 W D0-D7    sprite 3 horz
            4 W D0-D7    sprite 0 vert
            5 W D0-D7    sprite 2 vert
            6 W D0-D7    sprite 3 vert
            7 W D0-D7    sprite 4 vert
            8 W D0-D7    sprite 0 select
                D0-D3    sprite #
                D4       prom (not sure)
                D5       flip x
            9 W          sprite 1 select
                D0-D3    sprite #
                D4       prom (not sure)
                D5       flip x
            a W          sprite 2 select
                D0-D3    sprite #
                D4       prom (not sure)
                D5       flip x
            b W          sprite 3 select
                D0-D3    sprite #
                D4       prom (not sure)
                D5       flip x

    0e00-0eff   RAM

    1000-1bff   ROM     part two

    1c00-1fff   RAM     video buffer

    ***********************************************
    memory map CPU #1 (preliminary)
    ***********************************************

    0000..0bff  ROM part one

    0c00..0c03  H/W input ports
    -----------------------------------------------
            0 R sound command from CPU #0
                D0-D7   8 different sounds ???

            1 R ???
            2 R ???
            3 R ???

            0 W D0-D7   DAC
            1 W D0-D3   preset for counter, clk is 5 MHz / 256
                D4-D7   volume bits 0 .. 3 (bit 4 is CPU #1 flag output)
            2 W D0-D7   preset for counter, clk is 5 MHz / 32
            3 W D0      divide c02 counter by 0: 2, 1: 4
                D1      sound enable for c02 tone generator
                D2      sound enable for DAC
                D3      sound enable for c01 tone generator

    0e00-0eff   RAM


    ********************************************
    Inferno memory map (very incomplete)
    ********************************************
    0000..0bff  ROM part one
    1c00..1eff  video buffer
    1f00..1f03  hardware?

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "cpu/s2650/s2650.h"
#include "meadows.h"
#include "sound/dac.h"
#include "sound/samples.h"



/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT32 cycles_at_vsync;
static UINT8 main_sense_state;
static UINT8 sound_sense_state;
static UINT8 coin1_state;
static UINT8 minferno_sense;



/*************************************
 *
 *  Special input ports
 *
 *************************************/

static READ8_HANDLER( hsync_chain_r )
{
	/* horizontal sync divider chain */
	UINT8 val = (activecpu_gettotalcycles() - cycles_at_vsync) & 0xff;
	return BITSWAP8(val,0,1,2,3,4,5,6,7);
}


static READ8_HANDLER( vsync_chain_hi_r )
{
	/* vertical sync divider chain */
	UINT8 val = cpu_getscanline();
	return ((val >> 1) & 0x08) | ((val >> 3) & 0x04) | ((val >> 5) & 0x02) | (val >> 7);
}


static READ8_HANDLER( vsync_chain_lo_r )
{
	/* vertical sync divider chain */
	UINT8 val = cpu_getscanline();
	return val & 0x0f;
}



/*************************************
 *
 *  Sound control writes
 *
 *************************************/

static WRITE8_HANDLER( meadows_sound_w )
{
	switch (offset)
	{
		case 0:
			if (meadows_0c00 == data)
				break;
			logerror("meadows_sound_w %d $%02x\n", offset, data);
			meadows_0c00 = data;
            break;

		case 1:
			logerror("meadows_sound_w %d $%02x\n", offset, data);
            break;

        case 2:
			logerror("meadows_sound_w %d $%02x\n", offset, data);
            break;

		case 3:
/*          S2650_Clear_Pending_Interrupts(); */
			break;
	}
}



/*************************************
 *
 *  Main CPU interrupt
 *
 *************************************/

static INTERRUPT_GEN( meadows_interrupt )
{
	/* preserve the actual cycle count */
    cycles_at_vsync = cpunum_gettotalcycles(0);

    /* fake something toggling the sense input line of the S2650 */
	main_sense_state ^= 1;
	cpunum_set_input_line(0, 1, main_sense_state ? ASSERT_LINE : CLEAR_LINE);

	/* check the fake coin input */
	if (readinputport(3) & 0x01)
	{
		if (!coin1_state)
		{
			coin1_state = 1;
			cpunum_set_input_line_and_vector(0, 0, PULSE_LINE, 0x82);
		}
	}
	else
		coin1_state = 0;
}



/*************************************
 *
 *  Main CPU interrupt (Inferno)
 *
 *************************************/

static INTERRUPT_GEN( minferno_interrupt )
{
	/* preserve the actual cycle count */
	cycles_at_vsync = cpunum_gettotalcycles(0);
	minferno_sense++;
	cpunum_set_input_line(0, 1, (minferno_sense & 0x40) ? ASSERT_LINE : CLEAR_LINE );
}



/*************************************
 *
 *  Sound hardware output control
 *
 *************************************/

static WRITE8_HANDLER( sound_hardware_w )
{
	switch (offset & 3)
	{
		case 0: /* DAC */
			meadows_sh_dac_w(data ^ 0xff);
            break;

		case 1: /* counter clk 5 MHz / 256 */
			if (data == meadows_0c01)
				break;
			logerror("sound_w ctr1 preset $%x amp %d\n", data & 15, data >> 4);
			meadows_0c01 = data;
			meadows_sh_update();
			break;

		case 2: /* counter clk 5 MHz / 32 (/ 2 or / 4) */
			if (data == meadows_0c02)
                break;
			logerror("sound_w ctr2 preset $%02x\n", data);
			meadows_0c02 = data;
			meadows_sh_update();
            break;

		case 3: /* sound enable */
			if (data == meadows_0c03)
                break;
			logerror("sound_w enable ctr2/2:%d ctr2:%d dac:%d ctr1:%d\n", data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1);
			meadows_0c03 = data;
			meadows_sh_update();
            break;
	}
}



/*************************************
 *
 *  Sound hardware read
 *
 *************************************/

static READ8_HANDLER( sound_hardware_r )
{
	int data = 0;

	switch (offset)
	{
		case 0:
			data = meadows_0c00;
            break;

		case 1: break;
		case 2: break;
		case 3: break;
	}
    return data;
}



/*************************************
 *
 *  Sound hardware interrupts
 *
 *************************************/

static INTERRUPT_GEN( sound_interrupt )
{
    /* fake something toggling the sense input line of the S2650 */
	sound_sense_state ^= 1;
	cpunum_set_input_line(1, 1, sound_sense_state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *  Overlays
 *
 *************************************/

/* some constants to make life easier */
#define SCR_HORZ        32
#define SCR_VERT        28

OVERLAY_START( deadeye_overlay )
	OVERLAY_RECT(            0,    0, SCR_HORZ*8,        2*8, MAKE_ARGB(0x04, 32,192, 64))
	OVERLAY_RECT(            0,  2*8, SCR_HORZ*8,        6*8, MAKE_ARGB(0x04, 64, 64,192))
	OVERLAY_RECT(            0,  6*8, SCR_HORZ*8,        9*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT(            0,  9*8,        1*8,       24*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT( SCR_HORZ*8-8,  9*8, SCR_HORZ*8,       24*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT(            0, 24*8, SCR_HORZ*8, SCR_VERT*8, MAKE_ARGB(0x04, 64, 64,192))
OVERLAY_END


OVERLAY_START( gypsyjug_overlay )
	OVERLAY_RECT(            0,    0, SCR_HORZ*8,        2*8, MAKE_ARGB(0x04, 32,192, 64))
//  OVERLAY_RECT(            0,  2*8, SCR_HORZ*8,        6*8, MAKE_ARGB(0x04, 64, 64,192))
	OVERLAY_RECT(            0,  2*8, SCR_HORZ*8,        3*8, MAKE_ARGB(0x04, 32,192, 64))
	OVERLAY_RECT(            0,  3*8, SCR_HORZ*8,        6*8, MAKE_ARGB(0x04, 64, 64,192))
	OVERLAY_RECT(            0,  6*8, SCR_HORZ*8,        9*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT(            0,  9*8,        1*8,       24*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT( SCR_HORZ*8-8,  9*8, SCR_HORZ*8,       24*8, MAKE_ARGB(0x04,192,160, 32))
	OVERLAY_RECT(            0, 24*8, SCR_HORZ*8, SCR_VERT*8, MAKE_ARGB(0x04,192,160, 32))
OVERLAY_END



/*************************************
 *
 *  Palette init
 *
 *************************************/

static PALETTE_INIT( meadows )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x0c00, 0x0c03) AM_WRITE(meadows_sound_w)
	AM_RANGE(0x0d00, 0x0d0f) AM_WRITE(meadows_spriteram_w) AM_BASE(&spriteram)
	AM_RANGE(0x0e00, 0x0eff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1000, 0x1bff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1c00, 0x1fff) AM_WRITE(meadows_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0c00, 0x0c00) AM_READ(input_port_0_r)
	AM_RANGE(0x0c01, 0x0c01) AM_READ(input_port_1_r)
	AM_RANGE(0x0c02, 0x0c02) AM_READ(hsync_chain_r)
	AM_RANGE(0x0c03, 0x0c03) AM_READ(input_port_2_r)
	AM_RANGE(0x0e00, 0x0eff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x1bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1c00, 0x1fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( minferno_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1c00, 0x1eff) AM_WRITE(meadows_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x1f00, 0x1f03) AM_WRITE(meadows_sound_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( minferno_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1c00, 0x1eff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1f00, 0x1f00) AM_READ(input_port_0_r)
	AM_RANGE(0x1f01, 0x1f01) AM_READ(input_port_1_r)
	AM_RANGE(0x1f02, 0x1f02) AM_READ(input_port_2_r)
	AM_RANGE(0x1f03, 0x1f03) AM_READ(input_port_3_r)
	AM_RANGE(0x1f04, 0x1f04) AM_READ(vsync_chain_hi_r)
	AM_RANGE(0x1f05, 0x1f05) AM_READ(vsync_chain_lo_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( minferno_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(S2650_DATA_PORT, S2650_DATA_PORT) AM_READ(input_port_4_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x0c00, 0x0c03) AM_WRITE(sound_hardware_w)
	AM_RANGE(0x0e00, 0x0eff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0c00, 0x0c03) AM_READ(sound_hardware_r)
	AM_RANGE(0x0e00, 0x0eff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( meadows )
	PORT_START		/* IN0 buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN1 control 1 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START		/* IN2 dip switch */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_DIPSETTING(    0x05, "7" )
	PORT_DIPSETTING(    0x06, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "5000")
	PORT_DIPSETTING(    0x80, "15000")
	PORT_DIPSETTING(    0xc0, "35000")
	PORT_DIPSETTING(    0x00, DEF_STR( None ))

	PORT_START		/* FAKE coinage */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x8e, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( minferno )
	PORT_START		/* IN0 left joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)

	PORT_START		/* IN1 right joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)

	PORT_START		/* IN2 buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN3 coinage */
	PORT_DIPNAME( 0x01, 0x01, "Coin Option" )
	PORT_DIPSETTING(    0x00, "1 Game/Coin" )
	PORT_DIPSETTING(    0x01, "1 Player/Coin" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START		/* IN4 dip switch */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Game_Time ) )
	PORT_DIPSETTING(    0x00, "60s" )
	PORT_DIPSETTING(    0x01, "90s" )
	PORT_DIPSETTING(    0x02, "120s" )
	PORT_DIPSETTING(    0x03, "180s" )
	PORT_DIPNAME( 0x0c, 0x04, "Extended Play Score" )
	PORT_DIPSETTING(    0x00, "3000/6000" )
	PORT_DIPSETTING(    0x04, "4000/7000" )
	PORT_DIPSETTING(    0x08, "5000/8000" )
	PORT_DIPSETTING(    0x0c, "6000/9000" )
	PORT_DIPNAME( 0x30, 0x10, "Extended Play Time" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x10, "20s" )
	PORT_DIPSETTING(    0x20, "40s" )
	PORT_DIPSETTING(    0x30, "60s" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,8,							/* 8*8 characters */
	128,							/* 128 characters ? */
	1,								/* 1 bit per pixel */
	{ 0 },							/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, 	/* pretty straight layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 							/* every char takes 8 bytes */
};


static const gfx_layout spritelayout =
{
	16,16,							/* 16*16 sprites ?  */
	32, 							/* 32 sprites  */
	1,								/* 1 bits per pixel */
	{ 0 },							/* 1 bitplane */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	8, 9,10,11,12,13,14,15 },	  /* pretty straight layout */
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	8*16, 9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*2*8							/* every sprite takes 32 bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	 0, 1 },		/* character generator */
	{ REGION_GFX2, 0, &spritelayout, 0, 1 },		/* sprite prom 1 */
	{ REGION_GFX3, 0, &spritelayout, 0, 1 },		/* sprite prom 2 */
	{ REGION_GFX4, 0, &spritelayout, 0, 1 },		/* sprite prom 3 (unused) */
	{ REGION_GFX5, 0, &spritelayout, 0, 1 },		/* sprite prom 4 (unused) */
	{ -1 } /* end of array */
};


static const gfx_decode minferno_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 4 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct Samplesinterface custom_interface =
{
	2,
	NULL,
	meadows_sh_start
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( meadows )

	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 5000000/8) 	/* 5MHz / 8 = 625 kHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(meadows_interrupt,1) 	/* one interrupt per frame!? */

	MDRV_CPU_ADD(S2650, 5000000/8)
	/* audio CPU */ 	/* 5MHz / 8 = 625 kHz */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(sound_interrupt,TIME_IN_HZ(38))	/* 5000000/131072 interrupts per frame */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(meadows)
	MDRV_VIDEO_START(meadows)
	MDRV_VIDEO_UPDATE(meadows)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( minferno )

	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 5000000/8/3) 	/* 5MHz / 8 = 625 kHz */
	MDRV_CPU_PROGRAM_MAP(minferno_readmem,minferno_writemem)
	MDRV_CPU_IO_MAP(minferno_readport,0)
	MDRV_CPU_VBLANK_INT(minferno_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)		/* VBLANK is defined by visible area */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 24*8-1)
	MDRV_GFXDECODE(minferno_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(meadows)
	MDRV_VIDEO_START(meadows)
	MDRV_VIDEO_UPDATE(meadows)

	/* sound hardware */
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( deadeye )
	ROM_REGION( 0x08000, REGION_CPU1, 0 ) 	/* 32K for code */
	ROM_LOAD( "de1.8h",       0x0000, 0x0400, CRC(bd09e4dc) SHA1(5428835f6bc3d162496fdce174fcaaaba98c09f9) )
	ROM_LOAD( "de2.9h",       0x0400, 0x0400, CRC(b89edec3) SHA1(5ce0058f23b7e5c832029ca97d9a40d1494bf972) )
	ROM_LOAD( "de3.10h",      0x0800, 0x0400, CRC(acf24438) SHA1(d7ea668ee19a167cb006c92e9606e20ef13d052e) )
	ROM_LOAD( "de4.11h",      0x1000, 0x0400, CRC(8b68f792) SHA1(e6c0b53726587768d39270f2f1e5b935035c20e5) )
	ROM_LOAD( "de5.12h",      0x1400, 0x0400, CRC(7bdb535c) SHA1(7bd2e261a22f5f3ffc60ea12ca5f38c445ec0030) )
	ROM_LOAD( "de6.13h",      0x1800, 0x0400, CRC(847f9467) SHA1(253d386b76be99a1deef9e6b4cd906efdd9cf6d9) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "de_char.15e",  0x0000, 0x0400, CRC(b032bd8d) SHA1(130614d951c440a31c1262517cca0a133ddd1545) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "de_mov1.5a",   0x0000, 0x0400, CRC(c046b4c6) SHA1(3baa47a6c8962f6f66c08847b4ee4aa91580ad1a) )

	ROM_REGION( 0x0400, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "de_mov2.13a",  0x0000, 0x0400, CRC(b89c5df9) SHA1(dd0eac9d646dd24575c7b61ce141fdc66994c188) )

	ROM_REGION( 0x0400, REGION_GFX4, ROMREGION_DISPOSE )
	/* empty */
	ROM_REGION( 0x0400, REGION_GFX5, ROMREGION_DISPOSE )
	/* empty */

	ROM_REGION( 0x08000, REGION_CPU2, 0 ) 	/* 32K for code for the sound cpu */
	ROM_LOAD( "de_snd",       0x0000, 0x0400, CRC(c10a1b1a) SHA1(779ea261d23360634081295a164cacbd819d8719) )
ROM_END


ROM_START( gypsyjug )
	ROM_REGION( 0x08000, REGION_CPU1, 0 ) 	/* 32K for code */
	ROM_LOAD( "gj.1b",        0x0000, 0x0400, CRC(f6a71d9f) SHA1(11a86ae781297e4077a69e6809487022fed9c444) )
	ROM_LOAD( "gj.2b",        0x0400, 0x0400, CRC(94c14455) SHA1(ed704680c2b83d1726d1a17d64f5d57925a495b2) )
	ROM_LOAD( "gj.3b",        0x0800, 0x0400, CRC(87ee0490) SHA1(7ecca4df9755b604d179d407e7c9c04d616b689b) )
	ROM_LOAD( "gj.4b",        0x1000, 0x0400, CRC(dca519c8) SHA1(7651aa8b2a8e53113eb08108a5b8fb20518ae185) )
	ROM_LOAD( "gj.5b",        0x1400, 0x0400, CRC(7d83f9d0) SHA1(9aa8b281b5de7d913cf364a1159f2762fc69022d) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gj.e15",       0x0000, 0x0400, CRC(adb25e13) SHA1(67b5a24a724310f3817a891a54d239d60fe80760) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gj.a",         0x0000, 0x0400, CRC(d3725193) SHA1(5ea28c410a7b9532276fb98c7003b4c8f64d24c9) )

	ROM_REGION( 0x0400, REGION_GFX3, ROMREGION_DISPOSE )
	/* empty (copied from 2) */

	ROM_REGION( 0x0400, REGION_GFX4, ROMREGION_DISPOSE )
	/* empty (filled with fake data) */

	ROM_REGION( 0x0400, REGION_GFX5, ROMREGION_DISPOSE )
	/* empty (filled with fake data) */

	ROM_REGION( 0x08000, REGION_CPU2, 0 ) 	/* 32K for code for the sound cpu */
	ROM_LOAD( "gj.a4s",       0x0000, 0x0400, CRC(17a116bc) SHA1(797ba0b292afa3ba7eec985b533014acc00ed47d) )
	ROM_LOAD( "gj.a5s",       0x0400, 0x0400, CRC(fc23ae09) SHA1(42be34a9ef8c4c8ef9f94c85ca031076f84faa96) )
	ROM_LOAD( "gj.a6s",       0x0800, 0x0400, CRC(9e7bd71e) SHA1(e00801820c1a39cbfed124a29470da03cf8b40b4) )
ROM_END


ROM_START( minferno )
	ROM_REGION( 0x08000, REGION_CPU1, ROMREGION_INVERT )	/* 32K for code */
	ROM_LOAD_NIB_LOW ( "inferno.f5",	0x0000, 0x0400, CRC(58472a73) SHA1(7f8b9502c3db11219d6b765dec7b6ff3f62d6c8b) )
	ROM_LOAD_NIB_HIGH( "inferno.e5",	0x0000, 0x0400, CRC(451942af) SHA1(0a03d74c1b98771d2170c76ca41e972300c34c3a) )
	ROM_LOAD_NIB_LOW ( "inferno.f6",	0x0400, 0x0400, CRC(d85a195b) SHA1(8250f8e80a9bf196d7bf122af9aad0ae00dedd26) )
	ROM_LOAD_NIB_HIGH( "inferno.e6",	0x0400, 0x0400, CRC(788ccfac) SHA1(dfa99745db1c3866bf568fad289485aa0850875a) )
	ROM_LOAD_NIB_LOW ( "inferno.f7",	0x0800, 0x0400, CRC(73b4e9a3) SHA1(d9de88748a3009f3fc1f90c96bfc9732dc6a4a22) )
	ROM_LOAD_NIB_HIGH( "inferno.e7",	0x0800, 0x0400, CRC(902d9b78) SHA1(3bebbba6c7d00bea2c687b965f59a9e55b430dfa) )

	ROM_REGION( 0x00400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "inferno.b8",		0x0200, 0x0200, CRC(1b06466b) SHA1(aef13ab84526ee7493837eef7f48d9ede65b8e62) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( deadeye )
{
	artwork_set_overlay(deadeye_overlay);
}


/* A fake for the missing ball sprites #3 and #4 */
static DRIVER_INIT( gypsyjug )
{
	static unsigned char ball[16*2] =
	{
		0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
		0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
		0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
		0x01,0x80, 0x03,0xc0, 0x03,0xc0, 0x01,0x80
	};
	int i;

	memcpy(memory_region(REGION_GFX3),memory_region(REGION_GFX2),memory_region_length(REGION_GFX3));

	for (i = 0; i < memory_region_length(REGION_GFX4); i += 16*2)
	{
		memcpy(memory_region(REGION_GFX4) + i, ball, sizeof(ball));
		memcpy(memory_region(REGION_GFX5) + i, ball, sizeof(ball));
	}

	artwork_set_overlay(gypsyjug_overlay);
}


/* A fake for inverting the data bus */
static DRIVER_INIT( minferno )
{
	int i, length;
	unsigned char *mem;

	/* Create an inverted copy of the graphics data */
	mem = memory_region(REGION_GFX1);
	length = memory_region_length(REGION_GFX1);
	for (i = 0; i < length/2; i++)
		mem[i] = ~mem[i + length/2];
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1978, deadeye,  0, meadows,  meadows,  deadeye,  ROT0, "Meadows", "Dead Eye", 0 )
GAME( 1978, gypsyjug, 0, meadows,  meadows,  gypsyjug, ROT0, "Meadows", "Gypsy Juggler", 0 )
GAME( 1978, minferno, 0, minferno, minferno, minferno, ROT0, "Meadows", "Inferno (S2650)", GAME_NO_SOUND )
