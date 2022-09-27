/***************************************************************************

Tube Panic
(c)1984 Nichibutsu

Driver by Jarek Burczynski.

It wouldn't be possible without help from following people:
Al Kossow helped with finding TTL chips' numbers and made PCB scans.
Tim made some nice screenshots.
Dox lent the Tube Panic PCB to me - I have drawn the schematics using the PCB,
this allowed me to emulate the background-drawing circuit.

----
Tube Panic
Nichibutsu 1984

CPU
84P0100B

tp-b 6->1          19.968MHz

                  tp-2 tp-1  2147 2147 2147 2147 2147 2147 2147 2147

               +------ daughter board ------+
               tp-p 5->8 6116  6116 tp-p 4->1
               +----------------------------+

   z80a              z80a                     z80a

                8910 8910 8910     6116  - tp-s 2->1

       16MHz

 VID
 84P101B

   6MHz                                        +------+ daughter board
                                  6116          tp-c 1
   MS2010-A                                     tp-c 2
                              tp-g 3            tp-c 3
   tp-g 6                                       tp-c 4
                              tp-g 4
   tp-g 5                                       tp-c 8
                                                tp-c 7
   6116                                         tp-c 6
                                       tp-g 1   tp-c 5
                                               +------+
     tp-g 7
                                       tp-g 2
     tp-g 8
                                             4164 4164 4164 4164
                                        4164 4164 4164 4164
  2114

----

Roller Jammer
Nichibutsu 1985

84P0501A

               SW1      SW2                      16A

Z80   6116                        TP-B.5         16B     6116
TP-S.1 TP-S.2 TP-S.3 TP-B.1  8212 TP-B.2 TP-B.3          TP-B.4


 TP-P.1 TP-P.2 TP-P.3 TP-P.4 6116 6116 TP-P.5 TP-P.6 TP-P.7 TP-P.8    6116


       8910 8910 8910         Z80A      Z80A

                               16MHz                       19.968MHz



                      --------------------------------

  6MHz
                                     6116
                                                     TP-C.8
  MS2010-A                     TP-G.4                TP-C.7
                                                     TP-C.6
  TP-G.8                        TP-G.3               TP-C.5

  TP-G.7                                 TP-G.2
                                                     TP-C.4
  6116                                   TP-G.1      TP-C.3
                                                     TP-C.2
                                                     TP-C.1
   TP-G.6

   TP-G.5                                         4164 4164 4164 4164
                                             4164 4164 4164 4164
 2114

----




***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"

#include "tubep.h"


static int sound_latch;


/*************************** Main CPU on main PCB **************************/


static WRITE8_HANDLER( tubep_LS259_w )
{
	switch(offset)
	{
		case 0:
		case 1:
				/*
                    port b0: bit0 - coin 1 counter
                    port b1  bit0 - coin 2 counter
                */
				coin_counter_w(offset,data&1);
				break;
		case 2:
				//something...
				break;
		case 5:
				//screen_flip_w(offset,data&1); /* bit 0 = screen flip, active high */
				break;
		case 6:
				tubep_background_romselect_w(offset,data);	/* bit0 = 0->select roms: B1,B3,B5; bit0 = 1->select roms: B2,B4,B6 */
				break;
		case 7:
				tubep_colorproms_A4_line_w(offset,data);	/* bit0 = line A4 (color proms address) state */
				break;
		default:
				break;
	}
}


static ADDRESS_MAP_START( tubep_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_RAM
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(tubep_textram_w) AM_BASE(&tubep_textram)	/* RAM on GFX PCB @B13 */
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(MWA8_RAM) AM_SHARE(1)
	AM_RANGE(0xe800, 0xebff) AM_WRITE(MWA8_RAM) AM_SHARE(4)				/* row of 8 x 2147 RAMs on main PCB */
ADDRESS_MAP_END


static WRITE8_HANDLER( main_cpu_irq_line_clear_w )
{
//  cpunum_set_input_line(0,CLEAR_LINE);
//not used - handled by MAME anyway (because it is usual Vblank int)
	return;
}

static WRITE8_HANDLER( tubep_soundlatch_w )
{
	sound_latch = (data&0x7f) | 0x80;
}

static ADDRESS_MAP_START( tubep_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x80, 0x80) AM_READ(input_port_3_r)
	AM_RANGE(0x90, 0x90) AM_READ(input_port_4_r)
	AM_RANGE(0xa0, 0xa0) AM_READ(input_port_5_r)

	AM_RANGE(0xb0, 0xb0) AM_READ(input_port_2_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(input_port_1_r)
	AM_RANGE(0xd0, 0xd0) AM_READ(input_port_0_r)

	AM_RANGE(0x80, 0x80) AM_WRITE(main_cpu_irq_line_clear_w)
	AM_RANGE(0xb0, 0xb7) AM_WRITE(tubep_LS259_w)
	AM_RANGE(0xd0, 0xd0) AM_WRITE(tubep_soundlatch_w)
ADDRESS_MAP_END




/************************** Slave CPU on main PCB ****************************/

static ADDRESS_MAP_START( tubep_g_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xa000, 0xa000) AM_WRITE(tubep_background_a000_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(tubep_background_c000_w)
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_SHARE(1) 								/* 6116 #1 */
	AM_RANGE(0xe800, 0xebff) AM_WRITE(MWA8_RAM) AM_SHARE(4) AM_BASE(&tubep_backgroundram)	/* row of 8 x 2147 RAMs on main PCB */
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(MWA8_RAM) AM_SHARE(3)						/* sprites color lookup table */
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_SHARE(2)									/* program copies here part of shared ram ?? */
ADDRESS_MAP_END

static READ8_HANDLER( tubep_soundlatch_r )
{
 	int res;

	res = sound_latch;
	sound_latch = 0; /* "=0" ????  or "&= 0x7f" ?????  works either way */

	/*logerror("SOUND COMM READ %2x\n",res);*/

	return res;
}

static READ8_HANDLER( tubep_sound_irq_ack )
{
	cpunum_set_input_line(2, 0, CLEAR_LINE);
	return 0;
}

static WRITE8_HANDLER( tubep_sound_unknown )
{
	/*logerror("Sound CPU writes to port 0x07 - unknown function\n");*/
	return;
}


static ADDRESS_MAP_START( tubep_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0xd000, 0xd000) AM_READ(tubep_sound_irq_ack)
	AM_RANGE(0xe000, 0xe7ff) AM_RAM		/* 6116 #3 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tubep_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(AY8910_control_port_2_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(AY8910_write_port_2_w)
	AM_RANGE(0x06, 0x06) AM_READ(tubep_soundlatch_r)
	AM_RANGE(0x07, 0x07) AM_WRITE(tubep_sound_unknown)
ADDRESS_MAP_END

static void scanline_callback(int scanline)
{
	/* interrupt is generated whenever line V6 from video part goes lo->hi */
	/* that is when scanline is 64 and 192 accordingly */

	cpunum_set_input_line(2,0,ASSERT_LINE);	/* sound cpu interrupt (music tempo) */

	scanline += 128;
	scanline &= 255;

	timer_set( cpu_getscanlinetime( scanline ), scanline, scanline_callback );
}

static MACHINE_RESET( tubep )
{
	timer_set(cpu_getscanlinetime( 64 ), 64, scanline_callback );
}











/****************************************************************/

static WRITE8_HANDLER( rjammer_LS259_w )
{
	switch(offset)
	{
		case 0:
		case 1:
				coin_counter_w(offset,data&1);	/* bit 0 = coin counter */
				break;
		case 5:
				//screen_flip_w(offset,data&1); /* bit 0 = screen flip, active high */
				break;
		default:
				break;
	}
}

static WRITE8_HANDLER( rjammer_soundlatch_w )
{
	sound_latch = data;
	cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
}

static ADDRESS_MAP_START( rjammer_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_2_r)	/* a bug in game code (during attract mode) */
	AM_RANGE(0x80, 0x80) AM_READ(input_port_2_r)
	AM_RANGE(0x90, 0x90) AM_READ(input_port_3_r)
	AM_RANGE(0xa0, 0xa0) AM_READ(input_port_4_r)
	AM_RANGE(0xb0, 0xb0) AM_READ(input_port_0_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(input_port_1_r)

	AM_RANGE(0xd0, 0xd7) AM_WRITE(rjammer_LS259_w)
	AM_RANGE(0xe0, 0xe0) AM_WRITE(main_cpu_irq_line_clear_w)	/* clear IRQ interrupt */
	AM_RANGE(0xf0, 0xf0) AM_WRITE(rjammer_soundlatch_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( rjammer_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_RAM									/* MB8416 SRAM on daughterboard on main PCB (there are two SRAMs, this is the one on the left) */
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(tubep_textram_w) AM_BASE(&tubep_textram)/* RAM on GFX PCB @B13 */
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_SHARE(1)						/* MB8416 SRAM on daughterboard (the one on the right) */
ADDRESS_MAP_END



static ADDRESS_MAP_START( rjammer_slave_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xb0, 0xb0) AM_WRITE(rjammer_background_page_w)
	AM_RANGE(0xd0, 0xd0) AM_WRITE(rjammer_background_LS377_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rjammer_slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_RAM							/* M5M5117P @21G */
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_SHARE(1)				/* MB8416 on daughterboard (the one on the right) */
	AM_RANGE(0xe800, 0xefff) AM_RAM AM_BASE(&rjammer_backgroundram)/* M5M5117P @19B (background) */
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_SHARE(2)
ADDRESS_MAP_END


/* MS2010-A CPU (equivalent to NSC8105 with one new opcode: 0xec) on graphics PCB */
static ADDRESS_MAP_START( nsc_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_SHARE(3) AM_BASE(&tubep_sprite_colorsharedram)
	AM_RANGE(0x0800, 0x0fff) AM_RAM AM_SHARE(2)
	AM_RANGE(0x2000, 0x2009) AM_WRITE(tubep_sprite_control_w)
	AM_RANGE(0x200a, 0x200b) AM_WRITE(MWA8_NOP) /* not used by the games - perhaps designed for debugging */
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END





/****************************** Sound CPU *******************************/




static READ8_HANDLER( rjammer_soundlatch_r )
{
 	int res = sound_latch;
	return res;
}

static WRITE8_HANDLER( rjammer_voice_startstop_w )
{
	/* bit 0 of data selects voice start/stop (reset pin on MSM5205)*/
	// 0 -stop; 1-start
	MSM5205_reset_w (0, (data&1)^1 );

	return;
}
static WRITE8_HANDLER( rjammer_voice_frequency_select_w )
{
	/* bit 0 of data selects voice frequency on MSM5205 */
	// 0 -4 KHz; 1- 8KHz
	if (data&1)
		MSM5205_playmode_w(0,MSM5205_S48_4B);	/* 8 KHz */
	else
		MSM5205_playmode_w(0,MSM5205_S96_4B);	/* 4 KHz */

	return;
}

static int ls74 = 0;
static int ls377 = 0;

static void rjammer_adpcm_vck (int data)
{
	ls74 = (ls74+1) & 1;

	if (ls74==1)
	{
		MSM5205_data_w(0, (ls377>>0) & 15 );
		cpunum_set_input_line(2, 0, ASSERT_LINE );
	}
	else
	{
		MSM5205_data_w(0, (ls377>>4) & 15 );
	}

}

static WRITE8_HANDLER( rjammer_voice_input_w )
{
	/* 8 bits of adpcm data for MSM5205 */
	/* need to buffer the data, and switch two nibbles on two following interrupts*/

	ls377 = data;


	/* NOTE: game resets interrupt line on ANY access to ANY I/O port.
            I do it here because this port (0x80) is first one accessed
            in the interrupt routine.
    */
	cpunum_set_input_line(2, 0, CLEAR_LINE );
	return;
}

static WRITE8_HANDLER( rjammer_voice_intensity_control_w )
{
	/* 4 LSB bits select the intensity (analog circuit that alters the output from MSM5205) */
	// need to buffer the data
	return;
}

static ADDRESS_MAP_START( rjammer_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM		/* M5M5117P (M58125P @2C on schematics) */
ADDRESS_MAP_END

static ADDRESS_MAP_START( rjammer_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(rjammer_soundlatch_r)
	AM_RANGE(0x10, 0x10) AM_WRITE(rjammer_voice_startstop_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(rjammer_voice_frequency_select_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(rjammer_voice_input_w)
	AM_RANGE(0x90, 0x90) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x91, 0x91) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x92, 0x92) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x93, 0x93) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x94, 0x94) AM_WRITE(AY8910_control_port_2_w)
	AM_RANGE(0x95, 0x95) AM_WRITE(AY8910_write_port_2_w)
	AM_RANGE(0x96, 0x96) AM_WRITE(rjammer_voice_intensity_control_w)
ADDRESS_MAP_END


static WRITE8_HANDLER( ay8910_portA_0_w )
{
		//analog sound control
}
static WRITE8_HANDLER( ay8910_portB_0_w )
{
		//analog sound control
}
static WRITE8_HANDLER( ay8910_portA_1_w )
{
		//analog sound control
}
static WRITE8_HANDLER( ay8910_portB_1_w )
{
		//analog sound control
}
static WRITE8_HANDLER( ay8910_portA_2_w )
{
		//analog sound control
}
static WRITE8_HANDLER( ay8910_portB_2_w )
{
		//analog sound control
}



INPUT_PORTS_START( tubep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Coin, Start */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "40000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x04, "60000" )
	PORT_DIPSETTING(    0x00, "80000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "In Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END




INPUT_PORTS_START( rjammer )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bonus Time" )
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x00, "200" )
	PORT_DIPNAME( 0x0c, 0x0c, "Clear Men" )
	PORT_DIPSETTING(    0x0c, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x20, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8, 8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};
static const gfx_decode tubep_gfxdecodeinfo[] =
{
	{ REGION_GFX1,      0, &charlayout,       0, 32 },	/* 32 color codes */
	{ -1 }
};

static const gfx_decode rjammer_gfxdecodeinfo[] =
{
	{ REGION_GFX1,      0, &charlayout,       0, 16 },	/* 16 color codes */
	{ -1 }
};

static struct AY8910interface ay8910_interface_1 =
{
	0,
	0,
	ay8910_portA_0_w, /* write port A */
	ay8910_portB_0_w  /* write port B */
};

static struct AY8910interface ay8910_interface_2 =
{
	0,
	0,
	ay8910_portA_1_w, /* write port A */
	ay8910_portB_1_w  /* write port B */
};

static struct AY8910interface ay8910_interface_3 =
{
	0,
	0,
	ay8910_portA_2_w, /* write port A */
	ay8910_portB_2_w  /* write port B */
};

static struct MSM5205interface msm5205_interface =
{
	rjammer_adpcm_vck,			/* VCK function */
	MSM5205_S48_4B				/* 8 KHz (changes at run time) */
};



static MACHINE_DRIVER_START( tubep )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(tubep_map,0)
	MDRV_CPU_IO_MAP(tubep_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(tubep_g_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,19968000 / 8)	/* X2 19968000 Hz divided by LS669 (on Qc output) (signal RH0) */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(tubep_sound_map,0)
	MDRV_CPU_IO_MAP(tubep_sound_portmap,0)

	MDRV_CPU_ADD(NSC8105,6000000/4)	/* 6 MHz Xtal - divided internally ??? */
	MDRV_CPU_PROGRAM_MAP(nsc_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_RESET(tubep)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tubep_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32 + 256*64)
	MDRV_COLORTABLE_LENGTH(32*2)

	MDRV_PALETTE_INIT(tubep)
	MDRV_VIDEO_START(tubep)
	MDRV_VIDEO_EOF(tubep_eof)
	MDRV_VIDEO_UPDATE(tubep)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rjammer )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(rjammer_map,0)
	MDRV_CPU_IO_MAP(rjammer_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,16000000 / 4)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(rjammer_slave_map,0)
	MDRV_CPU_IO_MAP(rjammer_slave_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,19968000 / 8)	/* Xtal3 divided by LS669 (on Qc output) (signal RH0) */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(rjammer_sound_map,0)
	MDRV_CPU_IO_MAP(rjammer_sound_portmap,0)

	MDRV_CPU_ADD(NSC8105,6000000/4)	/* 6 MHz Xtal - divided internally ??? */
	MDRV_CPU_PROGRAM_MAP(nsc_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(rjammer_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(2*16 + 16*2)

	MDRV_PALETTE_INIT(rjammer)
	MDRV_VIDEO_START(tubep)
	MDRV_VIDEO_EOF(tubep_eof)
	MDRV_VIDEO_UPDATE(rjammer)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(AY8910, 19968000 / 8 / 2)
	MDRV_SOUND_CONFIG(ay8910_interface_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END




ROM_START( tubep )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (master) cpu code */
	ROM_LOAD( "tp-p.5", 0x0000, 0x2000, CRC(d5e0cc2f) SHA1(db9b062b14af52bb5458fe71996da295a69148ac) )
	ROM_LOAD( "tp-p.6", 0x2000, 0x2000, CRC(97b791a0) SHA1(20ef87b3d3bdfc8b983bcb8231252f81d98ad452) )
	ROM_LOAD( "tp-p.7", 0x4000, 0x2000, CRC(add9983e) SHA1(70a517451553a8c0e74a1995d9afddb779efc92c) )
	ROM_LOAD( "tp-p.8", 0x6000, 0x2000, CRC(b3793cb5) SHA1(0ed622b6bb97b9877acb6dc174edcd9977fa784e) )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (slave) cpu code */
	ROM_LOAD( "tp-p.1", 0x0000, 0x2000, CRC(b4020fcc) SHA1(437a037adedd596d295a0b6e400d64dee6c4488e) )
	ROM_LOAD( "tp-p.2", 0x2000, 0x2000, CRC(a69862d6) SHA1(7180cc26cd11d2daf453fcda8e6cc90851068bc4) )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, CRC(f1d86e00) SHA1(5c26f20f49e09a736cede4f276f5bdf76f932400) )
	ROM_LOAD( "tp-p.4", 0x6000, 0x2000, CRC(0a1027bc) SHA1(2ebb53a1da53a9c3f0b99da084030c4d2b62a7b3) )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-s.1", 0x0000, 0x2000, CRC(78964fcc) SHA1(a2c6119275d6291d82ac11dcffdaf2e8726e935a) )
	ROM_LOAD( "tp-s.2", 0x2000, 0x2000, CRC(61232e29) SHA1(a9ef0fefb7250392ef51173b69a69c903ff91ee8) )

	ROM_REGION( 0x10000,REGION_CPU4, 0 ) /* 64k for the custom CPU */
	ROM_LOAD( "tp-g5.e1", 0xc000, 0x2000, CRC(9f375b27) SHA1(9666d1b20169d899176fbdf5954df41df06b4b82) )
	ROM_LOAD( "tp-g6.d1", 0xe000, 0x2000, CRC(3ea127b8) SHA1(a5f83ee0eb871da81eeaf839499baf14b986c69e) )

	ROM_REGION( 0xc000, REGION_USER1, 0 ) /* background data */
	ROM_LOAD( "tp-b.1", 0x0000, 0x2000, CRC(fda355e0) SHA1(3270c65a4ee5d01388727f38691f7fe38f541031) )
	ROM_LOAD( "tp-b.2", 0x2000, 0x2000, CRC(cbe30149) SHA1(e66057286bea3026743f6de27a7e8dc8a709f8f7) )
	ROM_LOAD( "tp-b.3", 0x4000, 0x2000, CRC(f5d118e7) SHA1(a899bef3accef8995c457e8142a0001eed033fae) )
	ROM_LOAD( "tp-b.4", 0x6000, 0x2000, CRC(01952144) SHA1(d1074c79b51d3e2c152c9f3df6892027fe3a0e00) )
	ROM_LOAD( "tp-b.5", 0x8000, 0x2000, CRC(4dabea43) SHA1(72b9df9a3665baf34fb1f7301c5b9dd2619ed206) )
	ROM_LOAD( "tp-b.6", 0xa000, 0x2000, CRC(01952144) SHA1(d1074c79b51d3e2c152c9f3df6892027fe3a0e00) )

	ROM_REGION( 0x18000,REGION_USER2, 0 )
	ROM_LOAD( "tp-c.1", 0x0000, 0x2000, CRC(ec002af2) SHA1(f9643c2ff01412d3da42b050bce4cf7f7d2e6f6a) )
	ROM_LOAD( "tp-c.2", 0x2000, 0x2000, CRC(c44f7128) SHA1(e05c00a7094b3fbf7ac6ed6ed38e1b227d462b27) )
	ROM_LOAD( "tp-c.3", 0x4000, 0x2000, CRC(4146b0c9) SHA1(cd3d620531660834530c64cdf1ef0659f9f6f437) )
	ROM_LOAD( "tp-c.4", 0x6000, 0x2000, CRC(552b58cf) SHA1(4ffd50bd55a9f88275c96a180dafe5e04b7ffb40) )
	ROM_LOAD( "tp-c.5", 0x8000, 0x2000, CRC(2bb481d7) SHA1(c07a11b938952be36c27fbfaefd0707a704acdf6) )
	ROM_LOAD( "tp-c.6", 0xa000, 0x2000, CRC(c07a4338) SHA1(3a40bacc2a98dc54612352a80f9b9ebf769de339) )
	ROM_LOAD( "tp-c.7", 0xc000, 0x2000, CRC(87b8700a) SHA1(4ddb032de9d6e124fb2661da77e6ba078360ec75) )
	ROM_LOAD( "tp-c.8", 0xe000, 0x2000, CRC(a6497a03) SHA1(68b42a5fd55b7c08f140dc1e3bb2eaa563545ef6) )
	ROM_LOAD( "tp-g4.d10", 0x10000, 0x1000, CRC(40a1fe00) SHA1(2e1e12efe8083bf96233016a7712e6e486d968e4) ) /* 2732 eprom is used, but the PCB is prepared for 2764 eproms */
	ROM_RELOAD(            0x11000, 0x1000 )
	ROM_LOAD( "tp-g1.e13", 0x12000, 0x1000, CRC(4a7407a2) SHA1(7ca4e03c637a6f1c338ca438a7ab9e4ba537fee0) )
	ROM_LOAD( "tp-g2.f13", 0x13000, 0x1000, CRC(f0b26c2e) SHA1(54057c619675bb384035547becd2019974bf23fa) )

	ROM_LOAD( "tp-g7.h2",  0x14000, 0x2000, CRC(105cb9e4) SHA1(b9d8ffe35c1f66aa401e5d8e415bf7c016ff53bb) )
	ROM_LOAD( "tp-g8.i2",  0x16000, 0x2000, CRC(27e5e6c1) SHA1(f3896d0006351d165e36bafa4340175077b3d6ba) )

	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "tp-g3.c10", 0x0000, 0x1000, CRC(657a465d) SHA1(848217c3b736550586e8e9ba7a6e99e884094066) )	/* text characters */

	ROM_REGION( 0x40,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "tp-2.c12", 0x0000, 0x0020, CRC(ac7e582f) SHA1(9d8f9eda7130b49b91d9c63bafa119b2a91eeda0) ) /* text and sprites palette */
	ROM_LOAD( "tp-1.c13", 0x0020, 0x0020, CRC(cd0910d6) SHA1(1e6dae16115d5a03bbaf76c695327a06eb6da602) ) /* color control prom */
ROM_END


ROM_START( rjammer )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (master) cpu code */
	ROM_LOAD( "tp-p.1", 0x0000, 0x2000, CRC(93eeed67) SHA1(9ccfc49f42c6b451ff1c541d6487276f4bf9338e) )
	ROM_LOAD( "tp-p.2", 0x2000, 0x2000, CRC(ed2830c4) SHA1(078046e88604617342d29f0f4a0473fe6d484b19) )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, CRC(e29f25e3) SHA1(21abf0e7c315fac15dd39355c16f9401c2cf4593) )
	ROM_LOAD( "tp-p.4", 0x8000, 0x2000, CRC(6ed71fbc) SHA1(821506943b980077a9b4f309db095be7e952b13d) )
	ROM_CONTINUE(       0x6000, 0x2000 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (slave) cpu code */
	ROM_LOAD( "tp-p.8", 0x0000, 0x2000, CRC(388b9c66) SHA1(6d3e614736a7f06c26191699e8a8a13b239b259f) )
	ROM_LOAD( "tp-p.7", 0x2000, 0x2000, CRC(595030bb) SHA1(00dd0b3af965a2768c71297ba2a358050bdb8ef7) )
	ROM_LOAD( "tp-p.6", 0x4000, 0x2000, CRC(b5aa0f89) SHA1(d7e8b7e76fe6e5ef1d9bcad8469d56b81c9509ac) )
	ROM_LOAD( "tp-p.5", 0x6000, 0x2000, CRC(56eae9ac) SHA1(e5cd75df0c38021b81de2abf049b12c10db4f3cb) )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-b1.6d", 0x0000, 0x2000, CRC(b1c2525c) SHA1(7a184142e83982e33bc41cabae6fe804cec78748) )
	ROM_LOAD( "tp-s3.4d", 0x2000, 0x2000, CRC(90c9d0b9) SHA1(8657ee93d7b67ba89848bf94e03b5c3bcace92c4) )
	ROM_LOAD( "tp-s2.2d", 0x4000, 0x2000, CRC(444b6a1d) SHA1(1252b14d473d764a5326401aac782a1fa3419784) )
	ROM_LOAD( "tp-s1.1d", 0x6000, 0x2000, CRC(391097cd) SHA1(d4b48a3f26044b131e65f74479bf1671ad677eb4) )

	ROM_REGION( 0x10000,REGION_CPU4, 0 ) /* 64k for the custom CPU */
	ROM_LOAD( "tp-g7.e1",  0xc000, 0x2000, CRC(9f375b27) SHA1(9666d1b20169d899176fbdf5954df41df06b4b82) )
	ROM_LOAD( "tp-g8.d1",  0xe000, 0x2000, CRC(2e619fec) SHA1(d3d5fa708ca0097abf12d59ae41cb852278fe45d) )

	ROM_REGION( 0x7000, REGION_USER1, 0 ) /* background data */
	ROM_LOAD( "tp-b3.13d", 0x0000, 0x1000, CRC(b80ef399) SHA1(75fa17e1bb39363e194737a32db2d92e0cae5e79) )
	ROM_LOAD( "tp-b5.11b", 0x1000, 0x2000, CRC(0f260bfe) SHA1(975b7837f6c3c9d743903910fbdc3111c18a5955) )
	ROM_LOAD( "tp-b2.11d", 0x3000, 0x2000, CRC(8cd2c917) SHA1(472aceaf4a1050b2513d56b2703e556ac1e2a61a) )
	ROM_LOAD( "tp-b4.19c", 0x5000, 0x2000, CRC(6600f306) SHA1(2e25790839a465f5f8729964cfe27a587eb663f5) )

	ROM_REGION( 0x18000,REGION_USER2, 0 )
	ROM_LOAD( "tp-c.8", 0x0000, 0x2000, CRC(9f31ecb5) SHA1(c4b979c7da096648d0c58b2c8a205e1622ee28e9) )
	ROM_LOAD( "tp-c.7", 0x2000, 0x2000, CRC(cbf093f1) SHA1(128e01249165a87304eaf8003a9adf6f38d35d5e) )
	ROM_LOAD( "tp-c.6", 0x4000, 0x2000, CRC(11f9752b) SHA1(11dcbbfe4e673e379afd67874b64b48cdafa00f5) )
	ROM_LOAD( "tp-c.5", 0x6000, 0x2000, CRC(513f8777) SHA1(ebdbf164c20bbb8a52e32beb148917023e30c72b) )
	ROM_LOAD( "tp-c.1", 0x8000, 0x2000, CRC(ef573117) SHA1(e2cf1e7b7c4f64bf3f9723eca2061a6cf8d2eddb) )
	ROM_LOAD( "tp-c.2", 0xa000, 0x2000, CRC(1d29f1e6) SHA1(278556f89c8aed9b16bdbef7ba2847736473e63d) )
	ROM_LOAD( "tp-c.3", 0xc000, 0x2000, CRC(086511a7) SHA1(92691aec024312e7c8593a35303df15cb6e9c9f2) )
	ROM_LOAD( "tp-c.4", 0xe000, 0x2000, CRC(49f372ea) SHA1(16b500157b95437ea27a097010e798f3e82b2b6a) )
	ROM_LOAD( "tp-g3.d10", 0x10000, 0x1000, CRC(1f2abec5) SHA1(3e7d2849d517cc4941ac86df507743782ed9c694) )	/* 2732 eprom is used, but the PCB is prepared for 2764 eproms */
	ROM_RELOAD(            0x11000, 0x1000 )
	ROM_LOAD( "tp-g2.e13", 0x12000, 0x1000, CRC(4a7407a2) SHA1(7ca4e03c637a6f1c338ca438a7ab9e4ba537fee0) )
	ROM_LOAD( "tp-g1.f13", 0x13000, 0x1000, CRC(f0b26c2e) SHA1(54057c619675bb384035547becd2019974bf23fa) )
	ROM_LOAD( "tp-g6.h2",  0x14000, 0x2000, CRC(105cb9e4) SHA1(b9d8ffe35c1f66aa401e5d8e415bf7c016ff53bb) )
	ROM_LOAD( "tp-g5.i2",  0x16000, 0x2000, CRC(27e5e6c1) SHA1(f3896d0006351d165e36bafa4340175077b3d6ba) )

	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "tp-g4.c10", 0x0000, 0x1000, CRC(99e72549) SHA1(2509265c2d84ac6144aecd77f1b3f0d16bdcb572) )	/* text characters */

	ROM_REGION( 0x40,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "16b", 0x0000, 0x0020, CRC(9a12873a) SHA1(70f088b6eb5431e2ac6afcf15531eeb02a169442) ) /* text palette, sprites palette */
	ROM_LOAD( "16a", 0x0020, 0x0020, CRC(90222a71) SHA1(c3fd49c8075b0af451f6d2a142a4c4a2e397ac08) ) /* background palette */
ROM_END

/*     year  rom      parent  machine  inp   init */
GAME( 1984, tubep,   0,      tubep,   tubep,   0, ROT0, "Nichibutsu + Fujitek", "Tube Panic", 0 )
GAME( 1984, rjammer, 0,      rjammer, rjammer, 0, ROT0, "Nichibutsu + Alice", "Roller Jammer", 0 )

