/***************************************************************************

    Art & Magic hardware

    driver by Aaron Giles and Nicola Salmoria

    Games supported:
        * Cheese Chase
        * Ultimate Tennis
        * Stone Ball

    Known bugs:
        * measured against a real PCB, the games run slightly too fast
          in spite of accurately measured VBLANK timings

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "vidhrdw/tlc34076.h"
#include "artmagic.h"
#include "sound/okim6295.h"


static UINT16 *control;

static UINT8 tms_irq, hack_irq;

static UINT8 prot_input[16];
static UINT8 prot_input_index;
static UINT8 prot_output[16];
static UINT8 prot_output_index;
static UINT8 prot_output_bit;
static UINT8 prot_bit_index;
static UINT16 prot_save;

static void (*protection_handler)(void);



/*************************************
 *
 *  Interrupts
 *
 *************************************/

static void update_irq_state(void)
{
	int irq_lines = 0;

	if (tms_irq)
		irq_lines |= 4;
	if (hack_irq)
		irq_lines |= 5;

	if (irq_lines)
		cpunum_set_input_line(0, irq_lines, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 7, CLEAR_LINE);
}


static void m68k_gen_int(int state)
{
	tms_irq = state;
	update_irq_state();
}



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( artmagic )
{
	tms_irq = hack_irq = 0;
	update_irq_state();
	tlc34076_reset(6);
}



/*************************************
 *
 *  TMS34010 interface
 *
 *************************************/

static READ16_HANDLER( tms_host_r )
{
	return tms34010_host_r(1, offset);
}


static WRITE16_HANDLER( tms_host_w )
{
	tms34010_host_w(1, offset, data);
}



/*************************************
 *
 *  Misc control memory accesses
 *
 *************************************/

static WRITE16_HANDLER( control_w )
{
	COMBINE_DATA(&control[offset]);

	/* OKI banking here */
	if (offset == 0)
		OKIM6295_set_bank_base(0, (((data >> 4) & 1) * 0x40000) % memory_region_length(REGION_SOUND1));

	logerror("%06X:control_w(%d) = %04X\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *  Ultimate Tennis protection workarounds
 *
 *************************************/

static READ16_HANDLER( ultennis_hack_r )
{
	/* IRQ5 points to: jsr (a5); rte */
	if (activecpu_get_pc() == 0x18c2)
	{
		hack_irq = 1;
		update_irq_state();
		hack_irq = 0;
		update_irq_state();
	}
	return readinputport(0);
}



/*************************************
 *
 *  Game-specific protection
 *
 *************************************/

static void ultennis_protection(void)
{
	/* check the command byte */
	switch (prot_input[0])
	{
		case 0x00:	/* reset */
			prot_input_index = prot_output_index = 0;
			prot_output[0] = mame_rand();
			break;

		case 0x01:	/* 01 aaaa bbbb cccc dddd (xxxx) */
			if (prot_input_index == 9)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT16 b = prot_input[3] | (prot_input[4] << 8);
				UINT16 c = prot_input[5] | (prot_input[6] << 8);
				UINT16 d = prot_input[7] | (prot_input[8] << 8);
				UINT16 x = a - b;
				if ((INT16)x >= 0)
					x = (x * c) >> 16;
				else
					x = -(((UINT16)-x * c) >> 16);
				x += d;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 11)
				prot_input_index = 0;
			break;

		case 0x02:	/* 02 aaaa bbbb cccc (xxxxxxxx) */
			/*
                Ultimate Tennis -- actual values from a board:

                    hex                             decimal
                    0041 0084 00c8 -> 00044142       65 132 200 -> 278850 = 65*65*66
                    001e 0084 00fc -> 0000e808       30 132 252 ->  59400 = 30*30*66
                    0030 007c 005f -> 00022e00       48 124  95 -> 142848 = 48*48*62
                    0024 00dd 0061 -> 00022ce0       36 221  97 -> 142560 = 36*36*110
                    0025 0096 005b -> 00019113       37 150  91 -> 102675 = 37*37*75
                    0044 00c9 004c -> 00070e40       68 201  76 -> 462400 = 68*68*100

                question is: what is the 3rd value doing there?
            */
			if (prot_input_index == 7)
			{
				UINT16 a = (INT16)(prot_input[1] | (prot_input[2] << 8));
				UINT16 b = (INT16)(prot_input[3] | (prot_input[4] << 8));
				/*UINT16 c = (INT16)(prot_input[5] | (prot_input[6] << 8));*/
				UINT32 x = a * a * (b/2);
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output[2] = x >> 16;
				prot_output[3] = x >> 24;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 11)
				prot_input_index = 0;
			break;

		case 0x03:	/* 03 (xxxx) */
			if (prot_input_index == 1)
			{
				UINT16 x = prot_save;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 3)
				prot_input_index = 0;
			break;

		case 0x04:	/* 04 aaaa */
			if (prot_input_index == 3)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				prot_save = a;
				prot_input_index = prot_output_index = 0;
			}
			break;

		default:
			logerror("protection command %02X: unknown\n", prot_input[0]);
			prot_input_index = prot_output_index = 0;
			break;
	}
}


static void cheesech_protection(void)
{
	/* check the command byte */
	switch (prot_input[0])
	{
		case 0x00:	/* reset */
			prot_input_index = prot_output_index = 0;
			prot_output[0] = mame_rand();
			break;

		case 0x01:	/* 01 aaaa bbbb (xxxx) */
			if (prot_input_index == 5)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT16 b = prot_input[3] | (prot_input[4] << 8);
				UINT16 c = 0x4000;		/* seems to be hard-coded */
				UINT16 d = 0x00a0;		/* seems to be hard-coded */
				UINT16 x = a - b;
				if ((INT16)x >= 0)
					x = (x * c) >> 16;
				else
					x = -(((UINT16)-x * c) >> 16);
				x += d;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 7)
				prot_input_index = 0;
			break;

		case 0x03:	/* 03 (xxxx) */
			if (prot_input_index == 1)
			{
				UINT16 x = prot_save;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 3)
				prot_input_index = 0;
			break;

		case 0x04:	/* 04 aaaa */
			if (prot_input_index == 3)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				prot_save = a;
				prot_input_index = prot_output_index = 0;
			}
			break;

		default:
			logerror("protection command %02X: unknown\n", prot_input[0]);
			prot_input_index = prot_output_index = 0;
			break;
	}
}


static void stonebal_protection(void)
{
	/* check the command byte */
	switch (prot_input[0])
	{
		case 0x01:	/* 01 aaaa bbbb cccc dddd (xxxx) */
			if (prot_input_index == 9)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				UINT16 b = prot_input[3] | (prot_input[4] << 8);
				UINT16 c = prot_input[5] | (prot_input[6] << 8);
				UINT16 d = prot_input[7] | (prot_input[8] << 8);
				UINT16 x = a - b;
				if ((INT16)x >= 0)
					x = (x * d) >> 16;
				else
					x = -(((UINT16)-x * d) >> 16);
				x += c;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 11)
				prot_input_index = 0;
			break;

		case 0x02:	/* 02 aaaa (xx) */
			if (prot_input_index == 3)
			{
				/*UINT16 a = prot_input[1] | (prot_input[2] << 8);*/
				UINT8 x = 0xa5;
				prot_output[0] = x;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 4)
				prot_input_index = 0;
			break;

		case 0x03:	/* 03 (xxxx) */
			if (prot_input_index == 1)
			{
				UINT16 x = prot_save;
				prot_output[0] = x;
				prot_output[1] = x >> 8;
				prot_output_index = 0;
			}
			else if (prot_input_index >= 3)
				prot_input_index = 0;
			break;

		case 0x04:	/* 04 aaaa */
			if (prot_input_index == 3)
			{
				UINT16 a = prot_input[1] | (prot_input[2] << 8);
				prot_save = a;
				prot_input_index = prot_output_index = 0;
			}
			break;

		default:
			logerror("protection command %02X: unknown\n", prot_input[0]);
			prot_input_index = prot_output_index = 0;
			break;
	}
}


static READ16_HANDLER( special_port5_r )
{
	return readinputport(5) | prot_output_bit;
}


static WRITE16_HANDLER( protection_bit_w )
{
	/* shift in the new bit based on the offset */
	prot_input[prot_input_index] <<= 1;
	prot_input[prot_input_index] |= offset;

	/* clock out the next bit based on the offset */
	prot_output_bit = prot_output[prot_output_index] & 0x01;
	prot_output[prot_output_index] >>= 1;

	/* are we done with a whole byte? */
	if (++prot_bit_index == 8)
	{
		/* add the data and process it */
		prot_input_index++;
		prot_output_index++;
		prot_bit_index = 0;

		/* update the protection state */
		(*protection_handler)();
	}
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x220000, 0x23ffff) AM_RAM
	AM_RANGE(0x240000, 0x240fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x300000, 0x300001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x300002, 0x300003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x300004, 0x300005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x300006, 0x300007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x300008, 0x300009) AM_READ(input_port_4_word_r)
	AM_RANGE(0x30000a, 0x30000b) AM_READ(special_port5_r)
	AM_RANGE(0x300000, 0x300003) AM_WRITE(control_w) AM_BASE(&control)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(protection_bit_w)
	AM_RANGE(0x360000, 0x360001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x380000, 0x380007) AM_READWRITE(tms_host_r, tms_host_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( stonebal_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x27ffff) AM_RAM
	AM_RANGE(0x280000, 0x280fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x300000, 0x300001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x300002, 0x300003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x300004, 0x300005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x300006, 0x300007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x300008, 0x300009) AM_READ(input_port_4_word_r)
	AM_RANGE(0x30000a, 0x30000b) AM_READ(special_port5_r)
	AM_RANGE(0x30000c, 0x30000d) AM_READ(input_port_6_word_r)
	AM_RANGE(0x30000e, 0x30000f) AM_READ(input_port_7_word_r)
	AM_RANGE(0x300000, 0x300003) AM_WRITE(control_w) AM_BASE(&control)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(protection_bit_w)
	AM_RANGE(0x340000, 0x340001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x380000, 0x380007) AM_READWRITE(tms_host_r, tms_host_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Slave CPU memory handlers
 *
 *************************************/

static struct tms34010_config tms_config =
{
	1,								/* halt on reset */
	m68k_gen_int,					/* generate interrupt */
	artmagic_to_shiftreg,			/* write to shiftreg function */
	artmagic_from_shiftreg,			/* read from shiftreg function */
	0								/* display offset update function */
};


static ADDRESS_MAP_START( tms_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&artmagic_vram0)
	AM_RANGE(0x00400000, 0x005fffff) AM_RAM AM_BASE(&artmagic_vram1)
	AM_RANGE(0x00800000, 0x0080007f) AM_READWRITE(artmagic_blitter_r, artmagic_blitter_w)
	AM_RANGE(0x00c00000, 0x00c000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xffe00000, 0xffffffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( stonebal_tms_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&artmagic_vram0)
	AM_RANGE(0x00400000, 0x005fffff) AM_RAM AM_BASE(&artmagic_vram1)
	AM_RANGE(0x00800000, 0x0080007f) AM_READWRITE(artmagic_blitter_r, artmagic_blitter_w)
	AM_RANGE(0x00c00000, 0x00c000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xffc00000, 0xffffffff) AM_RAM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( artmagic )
	PORT_START_TAG("300000")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("300002")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("300004")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("300006")
	PORT_DIPNAME( 0x0007, 0x0007, "Right Coinage" )
	PORT_DIPSETTING(      0x0002, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(      0x0006, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ))
	PORT_DIPNAME( 0x0038, 0x0038, "Left Coinage"  )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("300008")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("30000a")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SPECIAL )		/* protection data */
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_SPECIAL )		/* protection ready */
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( cheesech )
	PORT_INCLUDE(artmagic)

	PORT_MODIFY("300004")
	PORT_DIPNAME( 0x0006, 0x0004, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( French ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Italian ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( German ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Lives ))
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPSETTING(      0x0018, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPSETTING(      0x0010, "6" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x00c0, 0x0040, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x00c0, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
INPUT_PORTS_END


INPUT_PORTS_START( ultennis )
	PORT_INCLUDE(artmagic)

	PORT_MODIFY("300004")
	PORT_DIPNAME( 0x0001, 0x0000, "Button Layout" )
	PORT_DIPSETTING(      0x0001, "Triangular" )
	PORT_DIPSETTING(      0x0000, "Linear" )
	PORT_DIPNAME( 0x0002, 0x0002, "Start set at" )
	PORT_DIPSETTING(      0x0000, "0-0" )
	PORT_DIPSETTING(      0x0002, "4-4" )
	PORT_DIPNAME( 0x0004, 0x0004, "Sets per Match" )
	PORT_DIPSETTING(      0x0004, "1" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0018, 0x0008, "Game Duration" )
	PORT_DIPSETTING(      0x0018, "5 lost points" )
	PORT_DIPSETTING(      0x0008, "6 lost points" )
	PORT_DIPSETTING(      0x0010, "7 lost points" )
	PORT_DIPSETTING(      0x0000, "8 lost points" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x00c0, 0x0040, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x00c0, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
INPUT_PORTS_END


INPUT_PORTS_START( stonebal )
	PORT_INCLUDE(artmagic)

	PORT_MODIFY("300004")
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Free_Play ))
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x001c, 0x001c, "Left Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0x00e0, 0x00e0, "Right Coinage" )
	PORT_DIPSETTING(      0x0040, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(      0x0060, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x00a0, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ))

	PORT_MODIFY("300006")
	PORT_DIPNAME( 0x0003, 0x0002, DEF_STR( Difficulty ))
	PORT_DIPSETTING(      0x0003, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0038, 0x0038, "Match Time" )
	PORT_DIPSETTING(      0x0030, "60s" )
	PORT_DIPSETTING(      0x0028, "70s" )
	PORT_DIPSETTING(      0x0020, "80s" )
	PORT_DIPSETTING(      0x0018, "90s" )
	PORT_DIPSETTING(      0x0038, "100s" )
	PORT_DIPSETTING(      0x0010, "110s" )
	PORT_DIPSETTING(      0x0008, "120s" )
	PORT_DIPSETTING(      0x0000, "130s" )
	PORT_DIPNAME( 0x0040, 0x0040, "Free Match Time" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "Short" )
	PORT_DIPNAME( 0x0080, 0x0080, "Game Mode" )
	PORT_DIPSETTING(      0x0080, "4 Players" )
	PORT_DIPSETTING(      0x0000, "2 Players" )

	PORT_START_TAG("30000c")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("30000e")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

/*
    video timing:
        measured HSYNC frequency = 15.685kHz
        measured VSYNC frequency = 50-51Hz
        programmed total lines = 312
        derived frame rate = 15685/312 = 50.27Hz
*/

MACHINE_DRIVER_START( artmagic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 25000000/2)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_CPU_ADD_TAG("tms", TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(tms_map,0)

	MDRV_MACHINE_RESET(artmagic)
	MDRV_FRAMES_PER_SECOND(50.27)
	MDRV_VBLANK_DURATION((1000000 * (312 - 256)) / (50.27 * 312))
	MDRV_INTERLEAVE(100)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(artmagic)
	MDRV_VIDEO_UPDATE(artmagic)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 40000000/3/10/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( stonebal )
	MDRV_IMPORT_FROM(artmagic)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(stonebal_map,0)

	MDRV_CPU_MODIFY("tms")
	MDRV_CPU_PROGRAM_MAP(stonebal_tms_map,0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( cheesech )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102",     0x00000, 0x40000, CRC(1d6e07c5) SHA1(8650868cce47f685d22131aa28aad45033cb0a52) )
	ROM_LOAD16_BYTE( "u101",     0x00001, 0x40000, CRC(30ae9f95) SHA1(fede5d271aabb654c1efc077253d81ba23786f22) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "u134", 0x00000, 0x80000, CRC(354ba4a6) SHA1(68e7df750efb21c716ba8b8ed4ca15a8cdc9141b) )
	ROM_LOAD16_BYTE( "u135", 0x00001, 0x80000, CRC(97348681) SHA1(7e74685041cd5e8fbd45731284cf316dc3ffec60) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u151", 0x00000, 0x80000, CRC(65d5ebdb) SHA1(0d905b9a60b86e51de3bdcf6eeb059fe29606431) )
ROM_END


ROM_START( ultennis )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "utu102.bin", 0x00000, 0x40000, CRC(ec31385e) SHA1(244e78619c549712d5541fb252656afeba639bb7) )
	ROM_LOAD16_BYTE( "utu101.bin", 0x00001, 0x40000, CRC(08a7f655) SHA1(b8a4265472360b68bed71d6c175fc54dff088c1d) )

	ROM_REGION16_LE( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "utu133.bin", 0x000000, 0x200000, CRC(29d9204d) SHA1(0b2b77a55b8c2877c2e31b63156505584d4ee1f0) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "utu151.bin", 0x00000,  0x40000, CRC(4e19ca89) SHA1(ac7e17631ec653f83c4912df6f458b0e1df88096) )
ROM_END


ROM_START( stonebal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102",     0x00000, 0x40000, CRC(712feda1) SHA1(c5b385f425786566fa274fe166a7116615a8ce86) )
	ROM_LOAD16_BYTE( "u101",     0x00001, 0x40000, CRC(4f1656a9) SHA1(720717ae4166b3ec50bb572197a8c6c96b284648) )

	ROM_REGION16_LE( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD( "u1600.bin", 0x000000, 0x200000, CRC(d2ffe9ff) SHA1(1c5dcbd8208e45458da9db7621f6b8602bca0fae) )
	ROM_LOAD( "u1601.bin", 0x200000, 0x200000, CRC(dbe893f0) SHA1(71a8a022decc0ff7d4c65f7e6e0cbba9e0b5582c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u1801.bin", 0x00000, 0x80000, CRC(d98f7378) SHA1(700df7f29c039b96791c2704a67f01a722dc96dc) )
ROM_END


ROM_START( stoneba2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "u102.bin", 0x00000, 0x40000, CRC(b3c4f64f) SHA1(6327e9f3cd9deb871a6910cf1f006c8ee143e859) )
	ROM_LOAD16_BYTE( "u101.bin", 0x00001, 0x40000, CRC(fe373f74) SHA1(bafac4bbd1aae4ccc4ae16205309483f1bbdd464) )

	ROM_REGION16_LE( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD( "u1600.bin", 0x000000, 0x200000, CRC(d2ffe9ff) SHA1(1c5dcbd8208e45458da9db7621f6b8602bca0fae) )
	ROM_LOAD( "u1601.bin", 0x200000, 0x200000, CRC(dbe893f0) SHA1(71a8a022decc0ff7d4c65f7e6e0cbba9e0b5582c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "u1801.bin", 0x00000, 0x80000, CRC(d98f7378) SHA1(700df7f29c039b96791c2704a67f01a722dc96dc) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static void decrypt_ultennis(void)
{
	int i;

	/* set up the parameters for the blitter data decryption which will happen at runtime */
	for (i = 0;i < 16;i++)
	{
		artmagic_xor[i] = 0x0462;
		if (i & 1) artmagic_xor[i] ^= 0x0011;
		if (i & 2) artmagic_xor[i] ^= 0x2200;
		if (i & 4) artmagic_xor[i] ^= 0x4004;
		if (i & 8) artmagic_xor[i] ^= 0x0880;
	}
}


static void decrypt_cheesech(void)
{
	int i;

	/* set up the parameters for the blitter data decryption which will happen at runtime */
	for (i = 0;i < 16;i++)
	{
		artmagic_xor[i] = 0x0891;
		if (i & 1) artmagic_xor[i] ^= 0x1100;
		if (i & 2) artmagic_xor[i] ^= 0x0022;
		if (i & 4) artmagic_xor[i] ^= 0x0440;
		if (i & 8) artmagic_xor[i] ^= 0x8008;
	}
}


static DRIVER_INIT( ultennis )
{
	decrypt_ultennis();
	artmagic_is_stoneball = 0;
	protection_handler = ultennis_protection;

	/* additional (protection?) hack */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x300000, 0x300001, 0, 0, ultennis_hack_r);
}


static DRIVER_INIT( cheesech )
{
	decrypt_cheesech();
	artmagic_is_stoneball = 0;
	protection_handler = cheesech_protection;
}


static DRIVER_INIT( stonebal )
{
	decrypt_ultennis();
	artmagic_is_stoneball = 1;	/* blits 1 line high are NOT encrypted, also different first pixel decrypt */
	protection_handler = stonebal_protection;
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1993, ultennis, 0,        artmagic, ultennis, ultennis, ROT0, "Art & Magic", "Ultimate Tennis", 0 )
GAME( 1994, cheesech, 0,        artmagic, cheesech, cheesech, ROT0, "Art & Magic", "Cheese Chase", 0 )
GAME( 1994, stonebal, 0,        stonebal, stonebal, stonebal, ROT0, "Art & Magic", "Stone Ball (4 Players)", 0 )
GAME( 1994, stoneba2, stonebal, stonebal, stonebal, stonebal, ROT0, "Art & Magic", "Stone Ball (2 Players)", 0 )
