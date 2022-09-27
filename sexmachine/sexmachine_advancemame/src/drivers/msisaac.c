/****************************************************************************

    Metal Soldier Isaac II  (c) Taito 1985

    driver by Jaroslaw Burczynski

****************************************************************************/

#include <math.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/msm5232.h"

/*
TO DO:
  - sprites are probably banked differently (no way to be sure until MCU dump is available)
  - TA7630 emulation needs filter support (characteristics depend on the frequency)
  - TA7630 volume table is hand tuned to match the sample, but still slighty off.
*/

/* in machine/buggychl.c */
READ8_HANDLER( buggychl_68705_portA_r );
WRITE8_HANDLER( buggychl_68705_portA_w );
WRITE8_HANDLER( buggychl_68705_ddrA_w );
READ8_HANDLER( buggychl_68705_portB_r );
WRITE8_HANDLER( buggychl_68705_portB_w );
WRITE8_HANDLER( buggychl_68705_ddrB_w );
READ8_HANDLER( buggychl_68705_portC_r );
WRITE8_HANDLER( buggychl_68705_portC_w );
WRITE8_HANDLER( buggychl_68705_ddrC_w );
WRITE8_HANDLER( buggychl_mcu_w );
READ8_HANDLER( buggychl_mcu_r );
READ8_HANDLER( buggychl_mcu_status_r );


//not used
//WRITE8_HANDLER( msisaac_textbank1_w );

//used
WRITE8_HANDLER( msisaac_fg_scrolly_w );
WRITE8_HANDLER( msisaac_fg_scrollx_w );
WRITE8_HANDLER( msisaac_bg_scrolly_w );
WRITE8_HANDLER( msisaac_bg_scrollx_w );
WRITE8_HANDLER( msisaac_bg2_scrolly_w );
WRITE8_HANDLER( msisaac_bg2_scrollx_w );

WRITE8_HANDLER( msisaac_bg2_textbank_w );

WRITE8_HANDLER( msisaac_bg_videoram_w );
WRITE8_HANDLER( msisaac_bg2_videoram_w );
WRITE8_HANDLER( msisaac_fg_videoram_w );

extern VIDEO_UPDATE( msisaac );
extern VIDEO_START( msisaac );
extern unsigned char *msisaac_videoram;
extern unsigned char *msisaac_videoram2;



static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE8_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	timer_set(TIME_NOW,data,nmi_callback);
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
		cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

#if 0
static WRITE8_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}

static WRITE8_HANDLER( msisaac_coin_counter_w )
{
	coin_counter_w(offset,data);
}
#endif
static WRITE8_HANDLER( ms_unknown_w )
{
	if (data!=0x08)
		ui_popup("CPU #0 write to 0xf0a3 data=%2x",data);
}





/* If good MCU dump will be available, it should be fully working game */
/* To test the game without the MCU simply comment out #define USE_MCU */

/* Disabled because the mcu dump is currently unavailable. -AS */
//#define USE_MCU

#ifndef USE_MCU
static UINT8 mcu_val = 0;
static UINT8 direction = 0;
#endif


static READ8_HANDLER( msisaac_mcu_r )
{
#ifdef USE_MCU
	return buggychl_mcu_r(offset);
#else
/*
MCU simulation TODO:
\-Find the command which allows player-2 to play.
\-Fix some graphics imperfections(*not* confirmed if they are caused by unhandled
  commands or imperfect video emulation).
*/

	switch(mcu_val)
	{
		/*Start-up check*/
		case 0x5f:  return (mcu_val+0x6b);
		/*These interferes with RAM operations(setting them to non-zero you  *
         * will have unexpected results,such as infinite lives or score not  *
         * incremented properly).*/
		case 0x40:
		case 0x41:
 		case 0x42:
 			return 0;
 		break;

 		/*With this command the MCU controls body direction  */
 		case 0x02:
 		{
 			//direction:
 			//0-left
 			//1-leftup
 			//2-up
 			//3-rigtup
 			//4-right
 			//5-rightdwn
 			//6-down
 			//7-leftdwn

 			UINT8 val= (readinputport(4)>>2) & 0x0f;
 			/* bit0 = left
               bit1 = right
               bit2 = down
               bit3 = up
            */
 			/* direction is encoded as:
                               4
                             3   5
                            2     6
                             1   7
                               0
            */
 			/*       0000   0001   0010   0011      0100   0101   0110   0111     1000   1001   1010   1011   1100   1101   1110   1111 */
 			/*      nochange left  right nochange   down downlft dwnrght down     up     upleft uprgt  up    nochnge left   right  nochange */

 			static const INT8 table[16] = { -1,    2,    6,     -1,       0,   1,      7,      0,       4,     3,     5,    4,     -1,     2,     6,    -1 };

 			if (table[val] >= 0 )
 				direction = table[val];

 			return direction;
 		}
 		break;

		/*This controls the arms when they return to the player.            */
 		case 0x07:
 			return 0x45;
 		break;

 		default:
 			logerror("CPU#0 read from MCU pc=%4x, mcu_val=%2x\n", activecpu_get_pc(), mcu_val );
 		   	return mcu_val;
 		break;
	}
#endif
}

static READ8_HANDLER( msisaac_mcu_status_r )
{
#ifdef USE_MCU
	return buggychl_mcu_status_r(offset);
#else
	return 3;	//mcu ready / cpu data ready
#endif
}

static WRITE8_HANDLER( msisaac_mcu_w )
{
#ifdef USE_MCU
	buggychl_mcu_w(offset,data);
#else
	//if(data != 0x0a && data != 0x42 && data != 0x02)
	//  ui_popup("PC = %04x %02x",activecpu_get_pc(),data);
	mcu_val = data;
#endif
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xe7ff) AM_READ(MRA8_RAM)

	AM_RANGE(0xf0e0, 0xf0e0) AM_READ(msisaac_mcu_r)
	AM_RANGE(0xf0e1, 0xf0e1) AM_READ(msisaac_mcu_status_r)

	AM_RANGE(0xf080, 0xf080) AM_READ(input_port_0_r)
	AM_RANGE(0xf081, 0xf081) AM_READ(input_port_1_r)
	AM_RANGE(0xf082, 0xf082) AM_READ(input_port_2_r)
	AM_RANGE(0xf083, 0xf083) AM_READ(input_port_3_r)
	AM_RANGE(0xf084, 0xf084) AM_READ(input_port_4_r)
//AM_RANGE(0xf086, 0xf086) AM_READ(input_port_5_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0xe800, 0xefff) AM_WRITE(paletteram_xxxxRRRRGGGGBBBB_le_w) AM_BASE(&paletteram)

//AM_RANGE(0xf400, 0xf43f) AM_WRITE(msisaac_fg_colorram_w) AM_BASE(&colorram)

	AM_RANGE(0xf0a3, 0xf0a3) AM_WRITE(ms_unknown_w)			//???? written in interrupt routine

	AM_RANGE(0xf060, 0xf060) AM_WRITE(sound_command_w)		//sound command
	AM_RANGE(0xf061, 0xf061) AM_WRITE(MWA8_NOP) /*sound_reset*/	//????

	AM_RANGE(0xf000, 0xf000) AM_WRITE(msisaac_bg2_textbank_w)
	AM_RANGE(0xf001, 0xf001) AM_WRITE(MWA8_RAM) 			//???
	AM_RANGE(0xf002, 0xf002) AM_WRITE(MWA8_RAM) 			//???

	AM_RANGE(0xf0c0, 0xf0c0) AM_WRITE(msisaac_fg_scrollx_w)
	AM_RANGE(0xf0c1, 0xf0c1) AM_WRITE(msisaac_fg_scrolly_w)
	AM_RANGE(0xf0c2, 0xf0c2) AM_WRITE(msisaac_bg2_scrollx_w)
	AM_RANGE(0xf0c3, 0xf0c3) AM_WRITE(msisaac_bg2_scrolly_w)
	AM_RANGE(0xf0c4, 0xf0c4) AM_WRITE(msisaac_bg_scrollx_w)
	AM_RANGE(0xf0c5, 0xf0c5) AM_WRITE(msisaac_bg_scrolly_w)

	AM_RANGE(0xf0e0, 0xf0e0) AM_WRITE(msisaac_mcu_w)

	AM_RANGE(0xf100, 0xf17f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)	//sprites
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(msisaac_fg_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xf800, 0xfbff) AM_WRITE(msisaac_bg2_videoram_w) AM_BASE(&msisaac_videoram2)
	AM_RANGE(0xfc00, 0xffff) AM_WRITE(msisaac_bg_videoram_w) AM_BASE(&msisaac_videoram)


//  AM_RANGE(0xf801, 0xf801) AM_WRITE(msisaac_bgcolor_w)
//  AM_RANGE(0xfc00, 0xfc00) AM_WRITE(flip_screen_w)
//  AM_RANGE(0xfc03, 0xfc04) AM_WRITE(msisaac_coin_counter_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc000, 0xc000) AM_READ(soundlatch_r)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_NOP) /*space for diagnostic ROM (not dumped, not reachable) */
ADDRESS_MAP_END


static int vol_ctrl[16];

static MACHINE_RESET( ta7630 )
{
	int i;

	double db			= 0.0;
	double db_step		= 0.50;	/* 0.50 dB step (at least, maybe more) */
	double db_step_inc	= 0.275;
	for (i=0; i<16; i++)
	{
		double max = 100.0 / pow(10.0, db/20.0 );
		vol_ctrl[ 15-i ] = max;
		/*logerror("vol_ctrl[%x] = %i (%f dB)\n",15-i,vol_ctrl[ 15-i ],db);*/
		db += db_step;
		db_step += db_step_inc;
	}

	/*for (i=0; i<8; i++)
        logerror("SOUND Chan#%i name=%s\n", i, mixer_get_name(i) );*/
/*
  channels 0-2 AY#0
  channels 3-5 AY#1
  channels 6,7 MSM5232 group1,group2
*/
}

static UINT8 snd_ctrl0=0;
static UINT8 snd_ctrl1=0;

static WRITE8_HANDLER( sound_control_0_w )
{
	snd_ctrl0 = data & 0xff;
	//ui_popup("SND0 0=%2x 1=%2x", snd_ctrl0, snd_ctrl1);

	sndti_set_output_gain(SOUND_MSM5232, 0, 0, vol_ctrl[  snd_ctrl0     & 15 ] / 100.0);	/* group1 from msm5232 */
	sndti_set_output_gain(SOUND_MSM5232, 0, 1, vol_ctrl[ (snd_ctrl0>>4) & 15 ] / 100.0);	/* group2 from msm5232 */

}
static WRITE8_HANDLER( sound_control_1_w )
{
	snd_ctrl1 = data & 0xff;
	//ui_popup("SND1 0=%2x 1=%2x", snd_ctrl0, snd_ctrl1);
}



static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)

	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8010, 0x801d) AM_WRITE(MSM5232_0_w)
	AM_RANGE(0x8020, 0x8020) AM_WRITE(sound_control_0_w)
	AM_RANGE(0x8030, 0x8030) AM_WRITE(sound_control_1_w)

	AM_RANGE(0xc001, 0xc001) AM_WRITE(nmi_enable_w)
	AM_RANGE(0xc002, 0xc002) AM_WRITE(nmi_disable_w)
	AM_RANGE(0xc003, 0xc003) AM_WRITE(MWA8_NOP) /*???*/ /* this is NOT mixer_enable */

ADDRESS_MAP_END

#ifdef USE_MCU

static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(buggychl_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(buggychl_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(buggychl_68705_portC_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(buggychl_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(buggychl_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(buggychl_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(buggychl_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(buggychl_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(buggychl_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

#endif

INPUT_PORTS_START( msisaac )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "DSW1 Unknown 0" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 Unknown 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPNAME( 0x20, 0x00, "DSW1 Unknown 5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DSW1 Unknown 6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DSW1 Unknown 7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW3 Unknown 1" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x02, "02" )
	PORT_DIPNAME( 0x04, 0x04, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DSW3 Unknown 3" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x08, "08" )
	PORT_DIPNAME( 0x30, 0x00, "Copyright Notice" )
	PORT_DIPSETTING(    0x00, "(C) 1985 Taito Corporation" )
	PORT_DIPSETTING(    0x10, "(C) Taito Corporation" )
	PORT_DIPSETTING(    0x20, "(C) Taito Corp. MCMLXXXV" )
	PORT_DIPSETTING(    0x30, "(C) Taito Corporation" )
	PORT_DIPNAME( 0x40, 0x00, "Coinage Display" )
	PORT_DIPSETTING(    0x40, "Insert Coin" )
	PORT_DIPSETTING(    0x00, "Coins/Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	//??
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	//??
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	//??

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


static const gfx_layout char_layout =
{
	8,8,
	0x400,
	4,
	{ 0*0x2000*8, 1*0x2000*8, 2*0x2000*8, 3*0x2000*8 },
	{ 7,6,5,4,3,2,1,0 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_layout tile_layout =
{
	16,16,
	0x100,
	4,
	{ 0*0x2000*8, 1*0x2000*8, 2*0x2000*8, 3*0x2000*8 },
	{ 7,6,5,4,3,2,1,0,  64+7,64+6,64+5,64+4,64+3,64+2,64+1,64+0 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout, 0, 64 },
	{ REGION_GFX2, 0, &char_layout, 0, 64 },
	{ REGION_GFX1, 0, &tile_layout, 0, 64 },
	{ REGION_GFX2, 0, &tile_layout, 0, 64 },
	{ -1 }
};

static struct MSM5232interface msm5232_interface =
{
	{ 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6 }	/* 0.65 (???) uF capacitors (match the sample, not verified) */
};


/*******************************************************************************/

static MACHINE_DRIVER_START( msisaac )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* source of IRQs is unknown */

#ifdef USE_MCU
	MDRV_CPU_ADD(M68705,8000000/2)  /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)
#endif

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(ta7630)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(msisaac)
	MDRV_VIDEO_UPDATE(msisaac)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD(MSM5232, 2000000)
	MDRV_SOUND_CONFIG(msm5232_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/*******************************************************************************/

ROM_START( msisaac )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 main CPU */
	ROM_LOAD( "a34_11.bin", 0x0000, 0x4000, CRC(40819334) SHA1(65352607165043909a09e96c07f7060f6ce087e6) )
	ROM_LOAD( "a34_12.bin", 0x4000, 0x4000, CRC(4c50b298) SHA1(5962882ad37ba6990ba2a6312b570f214cd4c103) )
	ROM_LOAD( "a34_13.bin", 0x8000, 0x4000, CRC(2e2b09b3) SHA1(daa715282ed9ef2e519e252a684ef28085becabd) )
	ROM_LOAD( "a34_10.bin", 0xc000, 0x2000, CRC(a2c53dc1) SHA1(14f23511f92bcfc94447dabe2826555d68bc1caa) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 sound CPU */
	ROM_LOAD( "a34_01.bin", 0x0000, 0x4000, CRC(545e45e7) SHA1(18ddb1ec8809bb62ae1c1068cd16cd3c933bf6ba) )

	ROM_REGION( 0x0800,  REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a34.mcu"       , 0x0000, 0x0800, NO_DUMP )

// I tried following MCUs; none of them work with this game:
//  ROM_LOAD( "a30-14"    , 0x0000, 0x0800, CRC(c4690279) ) //40love
//  ROM_LOAD( "a22-19.31",  0x0000, 0x0800, CRC(06a71df0) )     //buggy challenge
//  ROM_LOAD( "a45-19",     0x0000, 0x0800, CRC(5378253c) )     //flstory
//  ROM_LOAD( "a54-19",     0x0000, 0x0800, CRC(e08b8846) )     //lkage

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a34_02.bin", 0x0000, 0x2000, CRC(50da1a81) SHA1(8aa5a896f3e1173155d4574f5e1c2703e334cf44) )
	ROM_LOAD( "a34_03.bin", 0x2000, 0x2000, CRC(728a549e) SHA1(8969569d4b7a3ba7b740dbd236c047a46b723617) )
	ROM_LOAD( "a34_04.bin", 0x4000, 0x2000, CRC(e7d19f1c) SHA1(d55ee8085256c1f6a254d3249997326eebba7d88) )
	ROM_LOAD( "a34_05.bin", 0x6000, 0x2000, CRC(bed2107d) SHA1(83b16ca8a1b131aa6a2976cdbe907109750eaf71) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a34_06.bin", 0x0000, 0x2000, CRC(4ec71687) SHA1(e88f0c61a172fbca1784c95246776bf64c071bf7) )
	ROM_LOAD( "a34_07.bin", 0x2000, 0x2000, CRC(24922abf) SHA1(e42b4947b8c84bdf62990205308b8c187352d001) )
	ROM_LOAD( "a34_08.bin", 0x4000, 0x2000, CRC(3ddbf4c0) SHA1(7dd82aba661addd0a905bc185c1a6d7f2e21e0c6) )
	ROM_LOAD( "a34_09.bin", 0x6000, 0x2000, CRC(23eb089d) SHA1(fcf48862825bf09ba3718cbade0e163a660e1a68) )

ROM_END

GAME( 1985, msisaac, 0,        msisaac, msisaac, 0, ROT270, "Taito Corporation", "Metal Soldier Isaac II", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL)
