/****************************************************************************

Operation Wolf  (c) Taito 1987
==============

David Graves, Jarek Burczynski
C-Chip emulation by Bryan McPhail

Sources:        MAME Rastan driver
            MAME Taito F2 driver
            Raine source - many thanks to Richard Bush
              and the Raine Team.

Main CPU: MC68000 uses irq 5.
Sound   : Z80 & YM2151 & MSM5205


Operation Wolf uses similar hardware to Rainbow Islands and Rastan.
The screen layout and registers and sprites appear to be identical.


Gun Travel
----------

Horizontal gun travel maybe could be widened to include more
of the status bar (you can shoot enemies underneath it).

To keep the input span 0-255 a multiplier (300/256 ?)
would be used.


TODO
====

Need to verify Opwolf against original board: various reports
claim there are discrepancies (perhaps limitations of the fake
Z80 c-chip substitute to blame?).

There are a few unmapped writes for the sound Z80 in the log.

Unknown writes to the MSM5205 control addresses

Raine source has standard Asuka/Mofflot sprite/tile priority:
0x2000 in sprite_ctrl puts all sprites under top bg layer. But
Raine simply kludges in this value, failing to read it from a
register. So what is controlling priority.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"

static UINT8 *cchip_ram;
static UINT8 adpcm_b[0x08];
static UINT8 adpcm_c[0x08];
static int opwolf_gun_xoffs, opwolf_gun_yoffs;

void opwolf_cchip_init(void);

READ16_HANDLER( opwolf_cchip_status_r );
READ16_HANDLER( opwolf_cchip_data_r );
WRITE16_HANDLER( opwolf_cchip_status_w );
WRITE16_HANDLER( opwolf_cchip_data_w );
WRITE16_HANDLER( opwolf_cchip_bank_w );
WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rastan_spriteflip_w );
VIDEO_START( opwolf );
VIDEO_UPDATE( opwolf );

static READ16_HANDLER( cchip_r )
{
	return cchip_ram[offset];
}

static WRITE16_HANDLER( cchip_w )
{
	cchip_ram[offset] = data &0xff;
}

/**********************************************************
                GAME INPUTS
**********************************************************/

static READ16_HANDLER( opwolf_lightgun_r )
{
	int scaled;

	switch (offset)
	{
		case 0x00:	/* P1X - Have to remap 8 bit input value, into 0-319 visible range */
			scaled=(input_port_4_word_r(0,mem_mask) * 320 ) / 256;
			return (scaled + 0x15 + opwolf_gun_xoffs);
		case 0x01:	/* P1Y */
			return (input_port_5_word_r(0,mem_mask) - 0x24 + opwolf_gun_yoffs);
	}

	return 0xff;
}

static READ8_HANDLER( z80_input1_r )
{
	return input_port_0_word_r(0,0);	/* irrelevant mirror ? */
}

static READ8_HANDLER( z80_input2_r )
{
	return input_port_0_word_r(0,0);	/* needed for coins */
}


/******************************************************
                SOUND
******************************************************/

static WRITE8_HANDLER( sound_bankswitch_w )
{
	memory_set_bank(10, (data-1) & 0x03);
}

/***********************************************************
             MEMORY STRUCTURES
***********************************************************/


static ADDRESS_MAP_START( opwolf_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0f0000, 0x0f07ff) AM_MIRROR(0xf000) AM_READ(opwolf_cchip_data_r)
	AM_RANGE(0x0f0802, 0x0f0803) AM_MIRROR(0xf000) AM_READ(opwolf_cchip_status_r)
	AM_RANGE(0x100000, 0x107fff) AM_READ(MRA16_RAM)	/* RAM */
	AM_RANGE(0x200000, 0x200fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x380000, 0x380001) AM_READ(input_port_2_word_r)	/* DSW A */
	AM_RANGE(0x380002, 0x380003) AM_READ(input_port_3_word_r)	/* DSW B */
	AM_RANGE(0x3a0000, 0x3a0003) AM_READ(opwolf_lightgun_r)	/* lightgun, read at $11e0/6 */
	AM_RANGE(0x3e0000, 0x3e0001) AM_READ(MRA16_NOP)
	AM_RANGE(0x3e0002, 0x3e0003) AM_READ(taitosound_comm16_msb_r)
	AM_RANGE(0xc00000, 0xc0ffff) AM_READ(PC080SN_word_0_r)
	AM_RANGE(0xd00000, 0xd03fff) AM_READ(PC090OJ_word_0_r)	/* sprite ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( opwolf_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0ff000, 0x0ff7ff) AM_WRITE(opwolf_cchip_data_w)
	AM_RANGE(0x0ff802, 0x0ff803) AM_WRITE(opwolf_cchip_status_w)
	AM_RANGE(0x0ffc00, 0x0ffc01) AM_WRITE(opwolf_cchip_bank_w)
	AM_RANGE(0x100000, 0x107fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x380000, 0x380003) AM_WRITE(rainbow_spritectrl_w)	// usually 0x4, changes when you fire
	AM_RANGE(0x3c0000, 0x3c0001) AM_WRITE(MWA16_NOP)	/* watchdog ?? */
	AM_RANGE(0x3e0000, 0x3e0001) AM_WRITE(taitosound_port16_msb_w)
	AM_RANGE(0x3e0002, 0x3e0003) AM_WRITE(taitosound_comm16_msb_w)
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(PC080SN_word_0_w)
	AM_RANGE(0xc10000, 0xc1ffff) AM_WRITE(MWA16_RAM)	/* error in init code (?) */
	AM_RANGE(0xc20000, 0xc20003) AM_WRITE(PC080SN_yscroll_word_0_w)
	AM_RANGE(0xc40000, 0xc40003) AM_WRITE(PC080SN_xscroll_word_0_w)
	AM_RANGE(0xc50000, 0xc50003) AM_WRITE(PC080SN_ctrl_word_0_w)
	AM_RANGE(0xd00000, 0xd03fff) AM_WRITE(PC090OJ_word_0_w)	/* sprite ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( opwolfb_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0f0008, 0x0f0009) AM_READ(input_port_0_word_r)	/* IN0 */
	AM_RANGE(0x0f000a, 0x0f000b) AM_READ(input_port_1_word_r)	/* IN1 */
	AM_RANGE(0x0ff000, 0x0fffff) AM_READ(cchip_r)
	AM_RANGE(0x100000, 0x107fff) AM_READ(MRA16_RAM)	/* RAM */
	AM_RANGE(0x200000, 0x200fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x380000, 0x380001) AM_READ(input_port_2_word_r)	/* DSW A */
	AM_RANGE(0x380002, 0x380003) AM_READ(input_port_3_word_r)	/* DSW B */
	AM_RANGE(0x3a0000, 0x3a0003) AM_READ(opwolf_lightgun_r)	/* lightgun, read at $11e0/6 */
	AM_RANGE(0x3e0000, 0x3e0001) AM_READ(MRA16_NOP)
	AM_RANGE(0x3e0002, 0x3e0003) AM_READ(taitosound_comm16_msb_r)
	AM_RANGE(0xc00000, 0xc0ffff) AM_READ(PC080SN_word_0_r)
	AM_RANGE(0xd00000, 0xd03fff) AM_READ(PC090OJ_word_0_r)	/* sprite ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( opwolfb_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0ff000, 0x0fffff) AM_WRITE(cchip_w)
	AM_RANGE(0x100000, 0x107fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x200fff) AM_WRITE(paletteram16_xxxxRRRRGGGGBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x380000, 0x380003) AM_WRITE(rainbow_spritectrl_w)	// usually 0x4, changes when you fire
	AM_RANGE(0x3c0000, 0x3c0001) AM_WRITE(MWA16_NOP)	/* watchdog ?? */
	AM_RANGE(0x3e0000, 0x3e0001) AM_WRITE(taitosound_port16_msb_w)
	AM_RANGE(0x3e0002, 0x3e0003) AM_WRITE(taitosound_comm16_msb_w)
	AM_RANGE(0xc00000, 0xc0ffff) AM_WRITE(PC080SN_word_0_w)
	AM_RANGE(0xc10000, 0xc1ffff) AM_WRITE(MWA16_RAM)	/* error in init code (?) */
	AM_RANGE(0xc20000, 0xc20003) AM_WRITE(PC080SN_yscroll_word_0_w)
	AM_RANGE(0xc40000, 0xc40003) AM_WRITE(PC080SN_xscroll_word_0_w)
	AM_RANGE(0xc50000, 0xc50003) AM_WRITE(PC080SN_ctrl_word_0_w)
	AM_RANGE(0xd00000, 0xd03fff) AM_WRITE(PC090OJ_word_0_w)	/* sprite ram */
ADDRESS_MAP_END

/***************************************************************************
    This extra Z80 substitutes for the c-chip in the bootleg
 */

static ADDRESS_MAP_START( sub_z80_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8800, 0x8800) AM_READ(z80_input1_r)	/* read at PC=$637: poked to $c004 */
	AM_RANGE(0x9800, 0x9800) AM_READ(z80_input2_r)	/* read at PC=$631: poked to $c005 */
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_z80_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(MWA8_NOP)	/* unknown write, 0 then 1 each interrupt */
	AM_RANGE(0xa000, 0xa000) AM_WRITE(MWA8_NOP)	/* IRQ acknowledge (unimplemented) */
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM) AM_BASE(&cchip_ram)
ADDRESS_MAP_END

/***************************************************************************/

static ADDRESS_MAP_START( z80_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK10)
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9001, 0x9001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x9002, 0x9100) AM_READ(MRA8_RAM)
	AM_RANGE(0xa001, 0xa001) AM_READ(taitosound_slave_comm_r)
ADDRESS_MAP_END


static UINT32 adpcm_pos[2],adpcm_end[2];

//static unsigned char adpcm_d[0x08];
//0 - start ROM offset LSB
//1 - start ROM offset MSB
//2 - end ROM offset LSB
//3 - end ROM offset MSB
//start & end need to be multiplied by 16 to get a proper _byte_ address in adpcm ROM
//4 - always zero write (start trigger ?)
//5 - different values
//6 - different values

static MACHINE_START( opwolf )
{
	state_save_register_global_array(adpcm_b);
	state_save_register_global_array(adpcm_c);
	state_save_register_global_array(adpcm_pos);
	state_save_register_global_array(adpcm_end);

	return 0;
}

static MACHINE_RESET( opwolf )
{
	MSM5205_reset_w(0, 1);
	MSM5205_reset_w(1, 1);
}

static void opwolf_msm5205_vck(int chip)
{
	static int adpcm_data[2] = { -1, -1 };

	if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip, adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
		if (adpcm_pos[chip] == adpcm_end[chip])
			MSM5205_reset_w(chip, 1);
	}
	else
	{
		adpcm_data[chip] = memory_region(REGION_SOUND1)[adpcm_pos[chip]];
		adpcm_pos[chip] = (adpcm_pos[chip] + 1) & 0x7ffff;
		MSM5205_data_w(chip, adpcm_data[chip] >> 4);
	}
}

static WRITE8_HANDLER( opwolf_adpcm_b_w )
{
	int start;
	int end;

	adpcm_b[offset] = data;

	if (offset==0x04) //trigger ?
	{
		start = adpcm_b[0] + adpcm_b[1]*256;
		end   = adpcm_b[2] + adpcm_b[3]*256;
		start *=16;
		end   *=16;
		adpcm_pos[0] = start;
		adpcm_end[0] = end;
		MSM5205_reset_w(0, 0);
	}

//  logerror("CPU #1     b00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}


static WRITE8_HANDLER( opwolf_adpcm_c_w )
{
	int start;
	int end;

	adpcm_c[offset] = data;

	if (offset==0x04) //trigger ?
	{
		start = adpcm_c[0] + adpcm_c[1]*256;
		end   = adpcm_c[2] + adpcm_c[3]*256;
		start *=16;
		end   *=16;
		adpcm_pos[1] = start;
		adpcm_end[1] = end;
		MSM5205_reset_w(1, 0);
	}

//  logerror("CPU #1     c00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}


static WRITE8_HANDLER( opwolf_adpcm_d_w )
{
//   logerror("CPU #1         d00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}

static WRITE8_HANDLER( opwolf_adpcm_e_w )
{
//  logerror("CPU #1         e00%i-data=%2x   pc=%4x\n",offset,data,activecpu_get_pc() );
}


static ADDRESS_MAP_START( z80_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x9001, 0x9001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xb000, 0xb006) AM_WRITE(opwolf_adpcm_b_w)
	AM_RANGE(0xc000, 0xc006) AM_WRITE(opwolf_adpcm_c_w)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(opwolf_adpcm_d_w)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(opwolf_adpcm_e_w)
ADDRESS_MAP_END


/***********************************************************
             INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

INPUT_PORTS_START( opwolf )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "NY Conversion of Upright" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x02, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Ammo Magazines at Start" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )	// Manual says all 3 unused
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )

	PORT_START	/* P1X (span allows you to shoot enemies behind status bar) */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START	/* P1Y (span allows you to be slightly offscreen) */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)
INPUT_PORTS_END


/**************************************************************
                GFX DECODING
**************************************************************/

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4, 10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout_b =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout tilelayout_b =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_decode opwolf_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};

static const gfx_decode opwolfb_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout_b,  0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout_b,  0, 256 },	/* scr tiles */
	{ -1 } /* end of array */
};


/**************************************************************
                 YM2151 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irq_handler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface ym2151_interface =
{
	irq_handler,
	sound_bankswitch_w
};


static struct MSM5205interface msm5205_interface =
{
	opwolf_msm5205_vck,	/* VCK function */
	MSM5205_S48_4B		/* 8 kHz */
};



/***********************************************************
                 MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( opwolf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000 )	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(opwolf_readmem,opwolf_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000 )	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	MDRV_MACHINE_START(opwolf)
	MDRV_MACHINE_RESET(opwolf)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolf_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.60)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.60)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.60)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.60)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( opwolfb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(opwolfb_readmem,opwolfb_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(z80_readmem,z80_writemem)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sub_z80_readmem,sub_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	MDRV_MACHINE_START(opwolf)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(opwolfb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(opwolf)
	MDRV_VIDEO_UPDATE(opwolf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.60)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.60)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.60)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.60)
MACHINE_DRIVER_END


/***************************************************************************
                    DRIVERS
***************************************************************************/

ROM_START( opwolf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b20-05-02.40",  0x00000, 0x10000, CRC(3ffbfe3a) SHA1(e41257e6af18bab4e36267a0c25a6aaa742972d2) )
	ROM_LOAD16_BYTE( "b20-03-02.30",  0x00001, 0x10000, CRC(fdabd8a5) SHA1(866ec6168489024b8d157f2d5b1553d7f6e3d9b7) )
	ROM_LOAD16_BYTE( "b20-04.39",     0x20000, 0x10000, CRC(216b4838) SHA1(2851cae00bb3e32e20f35fdab8ed6f149e658363) )
	ROM_LOAD16_BYTE( "b20-20.29",     0x20001, 0x10000, CRC(d244431a) SHA1(cb6c1d330a526f05c205f68247328161b8d4a1ba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "b20-07.10",  0x00000, 0x04000, CRC(45c7ace3) SHA1(06f7393f6b973b7735c27e8380cb4148650cfc16) )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b20-13.13",  0x00000, 0x80000, CRC(f6acdab1) SHA1(716b94ab3fa330ecf22df576f6a9f47a49c7554a) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b20-14.72",  0x00000, 0x80000, CRC(89f889e5) SHA1(1592f6ce4fbb75e33d6ab957e5b90242a7a7a8c4) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b20-08.21",  0x00000, 0x80000, CRC(f3e19c64) SHA1(39d48645f776c9c2ade537d959ecc6f9dc6dfa1b) )
ROM_END

ROM_START( opwolfu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b20-05-02.40",  0x00000, 0x10000, CRC(3ffbfe3a) SHA1(e41257e6af18bab4e36267a0c25a6aaa742972d2) )
	ROM_LOAD16_BYTE( "b20-03-02.30",  0x00001, 0x10000, CRC(fdabd8a5) SHA1(866ec6168489024b8d157f2d5b1553d7f6e3d9b7) )
	ROM_LOAD16_BYTE( "b20-04.39",     0x20000, 0x10000, CRC(216b4838) SHA1(2851cae00bb3e32e20f35fdab8ed6f149e658363) )
	ROM_LOAD16_BYTE( "opwlf.29",      0x20001, 0x10000, CRC(b71bc44c) SHA1(5b404bd7630f01517ab98bda40ca43c11268035a) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "b20-07.10",  0x00000, 0x04000, CRC(45c7ace3) SHA1(06f7393f6b973b7735c27e8380cb4148650cfc16) )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b20-13.13",  0x00000, 0x80000, CRC(f6acdab1) SHA1(716b94ab3fa330ecf22df576f6a9f47a49c7554a) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b20-14.72",  0x00000, 0x80000, CRC(89f889e5) SHA1(1592f6ce4fbb75e33d6ab957e5b90242a7a7a8c4) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b20-08.21",  0x00000, 0x80000, CRC(f3e19c64) SHA1(39d48645f776c9c2ade537d959ecc6f9dc6dfa1b) )
ROM_END

ROM_START( opwolfb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "opwlfb.12",  0x00000, 0x10000, CRC(d87e4405) SHA1(de8a7763acd57293fbbff609e949ecd66c0f9234) )
	ROM_LOAD16_BYTE( "opwlfb.10",  0x00001, 0x10000, CRC(9ab6f75c) SHA1(85310258ca005ffb031e8d6b3f43c3d1fc29ef14) )
	ROM_LOAD16_BYTE( "opwlfb.13",  0x20000, 0x10000, CRC(61230c6e) SHA1(942764aec0c55ba00df8dbb54e127b73e24192ae) )
	ROM_LOAD16_BYTE( "opwlfb.11",  0x20001, 0x10000, CRC(342e318d) SHA1(a52918d16884ca42b2a3b910bc71bfd81b45f1ab) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "opwlfb.30",  0x00000, 0x04000, CRC(0669b94c) SHA1(f10894a6fad8ed144a528db696436b58f62ddee4) )
	ROM_CONTINUE(           0x10000, 0x04000 ) /* banked stuff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )      /* c-chip substitute Z80 */
	ROM_LOAD( "opwlfb.09",   0x00000, 0x08000, CRC(ab27a3dd) SHA1(cf589e7a9ccf3e86020b86f917fb91f3d8ba7512) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.08",   0x00000, 0x10000, CRC(134d294e) SHA1(bd05169dbd761c2944f0ac51c1ec114577777452) )	/* SCR tiles (8 x 8) */
	ROM_LOAD16_BYTE( "opwlfb.06",   0x20000, 0x10000, CRC(317d0e66) SHA1(70298c0ef5243f481b18f904be9404527d1d99d5) )
	ROM_LOAD16_BYTE( "opwlfb.07",   0x40000, 0x10000, CRC(e1c4095e) SHA1(d5f1d26d6612e78001002f92de670e68e00c6f9e) )
	ROM_LOAD16_BYTE( "opwlfb.05",   0x60000, 0x10000, CRC(fd9e72c8) SHA1(7a76f57641c3f0198565cd163188b581253173b2) )
	ROM_LOAD16_BYTE( "opwlfb.04",   0x00001, 0x10000, CRC(de0ca98d) SHA1(066e89ec0c64da14bdcd2b337f95c0de5de33c11) )
	ROM_LOAD16_BYTE( "opwlfb.02",   0x20001, 0x10000, CRC(6231fdd0) SHA1(1c830c106cf3c94a8d06ed2fff030a5d516ab6d6) )
	ROM_LOAD16_BYTE( "opwlfb.03",   0x40001, 0x10000, CRC(ccf8ba80) SHA1(8366f5ef0de885e5241567d1a083d98a8a2875d9) )
	ROM_LOAD16_BYTE( "opwlfb.01",   0x60001, 0x10000, CRC(0a65f256) SHA1(4dfcd3cb138a87d002eb65a02f94e33f4d07676d) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "opwlfb.14",   0x00000, 0x10000, CRC(663786eb) SHA1(a25710f6c16158e51d0934f184390a01ff0a614a) )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "opwlfb.15",   0x20000, 0x10000, CRC(315b8aa9) SHA1(4a904e5532421d933e4c401c03c958eb32b15e03) )
	ROM_LOAD16_BYTE( "opwlfb.16",   0x40000, 0x10000, CRC(e01099e3) SHA1(4c5391d71978f72c57c140e58a767e138acdce12) )
	ROM_LOAD16_BYTE( "opwlfb.17",   0x60000, 0x10000, CRC(56fbe61d) SHA1(0e4dce8ee981bdd851e500fa9dca5d40908e142f) )
	ROM_LOAD16_BYTE( "opwlfb.18",   0x00001, 0x10000, CRC(de9ab08e) SHA1(ef674c965f35efaf747f1ddbf9e9164fcceb0c1c) )
	ROM_LOAD16_BYTE( "opwlfb.19",   0x20001, 0x10000, CRC(645cf85e) SHA1(91c244c2e238b61c8b2f39e5fa01cc23ebbfe2ce) )
	ROM_LOAD16_BYTE( "opwlfb.20",   0x40001, 0x10000, CRC(d80b9cc6) SHA1(b189f35eb206da1ab313620e251e6bb10edeee04) )
	ROM_LOAD16_BYTE( "opwlfb.21",   0x60001, 0x10000, CRC(97d25157) SHA1(cfb3f76ed860d90235dc0e32919a5ec3d3e683dd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples (interleaved) */
	ROM_LOAD16_BYTE( "opwlfb.29",   0x00000, 0x10000, CRC(05a9eac0) SHA1(26eb1acc65aeb759920b35bcbcac6d6c2789584c) )
	ROM_LOAD16_BYTE( "opwlfb.28",   0x20000, 0x10000, CRC(281b2175) SHA1(3789e58da682041226f70eba87b31876cb206906) )
	ROM_LOAD16_BYTE( "opwlfb.27",   0x40000, 0x10000, CRC(441211a6) SHA1(82e84ae90765df5f7f6b6f32a2bb52ac40132f8d) )
	ROM_LOAD16_BYTE( "opwlfb.26",   0x60000, 0x10000, CRC(86d1d42d) SHA1(9d63e9e35fa51d8e6eac30556ba5a4dca7c14418) )
	ROM_LOAD16_BYTE( "opwlfb.25",   0x00001, 0x10000, CRC(85b87f58) SHA1(f26cf4ab8f9d30d1b1ac84be328ca821524b234e) )
	ROM_LOAD16_BYTE( "opwlfb.24",   0x20001, 0x10000, CRC(8efc5d4d) SHA1(21068d7fcfe293d99ad9f999d84483bf1a49ec6d) )
	ROM_LOAD16_BYTE( "opwlfb.23",   0x40001, 0x10000, CRC(a874c703) SHA1(c9d6074265f5d5028c69c81eaba29fa178943341) )
	ROM_LOAD16_BYTE( "opwlfb.22",   0x60001, 0x10000, CRC(9228481f) SHA1(8160f919f5e6a347c915a2bd7488b488fe2401bc) )
ROM_END


static DRIVER_INIT( opwolf )
{
	UINT16* rom=(UINT16*)memory_region(REGION_CPU1);

	opwolf_cchip_init();

	// World & US version have different gun offsets, presumably slightly different gun hardware
	opwolf_gun_xoffs = 0xec - (rom[0x1ffd8]&0xff);
	opwolf_gun_yoffs = 0x1c - (rom[0x1ffd7]&0xff);

	memory_configure_bank(10, 0, 4, memory_region(REGION_CPU2) + 0x10000, 0x4000);
}


static DRIVER_INIT( opwolfb )
{
	/* bootleg needs different range of raw gun coords */
	opwolf_gun_xoffs = -2;
	opwolf_gun_yoffs = 17;

	memory_configure_bank(10, 0, 4, memory_region(REGION_CPU2) + 0x10000, 0x4000);
}



/*    year  rom       parent    machine   inp       init */
GAME( 1987, opwolf,   0,        opwolf,   opwolf,   opwolf,   ROT0, "Taito Corporation Japan", "Operation Wolf (World)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1987, opwolfu,  opwolf,   opwolf,   opwolf,   opwolf,   ROT0, "Taito America Corporation", "Operation Wolf (US)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1987, opwolfb,  opwolf,   opwolfb,  opwolf,   opwolfb,  ROT0, "bootleg", "Operation Bear", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
