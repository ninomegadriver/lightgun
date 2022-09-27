/***************************************************************************

                            -= Gals Panic II =-

                    driver by   Luca Elia (l.elia@tin.it)

CPU     :   2 x 68000  +  MCU
SOUND   :   2 x OKIM6295
OTHER   :   EEPROM
CUSTOM  :   ?

To Do:

- Simulation of the MCU: it sits between the 2 68000's and passes
  messages along. It is currently incomplete, thus no backgrounds
  and the game is unplayable

- The layers are offset

- Sound banking's not correct

***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "kaneko16.h"
#include "sound/okim6295.h"

/***************************************************************************


                                    EEPROM


***************************************************************************/

static UINT16 eeprom_word;
READ16_HANDLER(galpani2_eeprom_r)
{
	return (eeprom_word & ~1) | (EEPROM_read_bit() & 1);
}

WRITE16_HANDLER(galpani2_eeprom_w)
{
	COMBINE_DATA( &eeprom_word );
	if ( ACCESSING_LSB )
	{
		// latch the bit
		EEPROM_write_bit(data & 0x02);

		// reset line asserted: reset.
		EEPROM_set_cs_line((data & 0x08) ? CLEAR_LINE : ASSERT_LINE );

		// clock line asserted: write latch or select next bit to read
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE );
	}
}


/***************************************************************************


                                MCU Simulation

100010.w    software watchdog?
100020.b    number of tasks for the mcu

***************************************************************************/

static UINT16 *galpani2_ram, *galpani2_ram2;

static MACHINE_RESET( galpani2 )
{
	machine_reset_kaneko16();

	kaneko16_sprite_type = 1;

	kaneko16_sprite_xoffs = 0x10000 - 0x16c0 + 0xc00;
	kaneko16_sprite_yoffs = 0x000;
}

static void galpani2_write_kaneko(void)
{
	cpunum_write_byte(0,0x100000,0x4b);
	cpunum_write_byte(0,0x100001,0x41);
	cpunum_write_byte(0,0x100002,0x4e);
	cpunum_write_byte(0,0x100003,0x45);
	cpunum_write_byte(0,0x100004,0x4b);
	cpunum_write_byte(0,0x100005,0x4f);
}

void galpani2_mcu_run(void)
{
	int i,x;

	/* Write "KANEKO" to 100000-100005, but do not clash with ram test */

	x  = 0;

	for (i = 0x100000; i < 0x100007; i++)
		x |= cpunum_read_byte(0,i);

	if	( x == 0 )
	{
		galpani2_write_kaneko();
		cpunum_write_byte(1,0x100006,1);
		logerror("MCU executes CHECK0\n");
	}
}

static void galpani2_mcu_nmi(void)
{
	UINT32 mcu_list, mcu_command, mcu_address, mcu_src, mcu_dst, mcu_size;

	/* "Last Check" */
	galpani2_write_kaneko();

	for ( mcu_list = 0x100020; mcu_list < (0x100020 + 0x40); mcu_list += 4 )
	{
		mcu_command		=	cpunum_read_byte(0, mcu_list + 1 );

		mcu_address		=	0x100000 +
							(cpunum_read_byte(0, mcu_list + 2)<<8) +
							(cpunum_read_byte(0, mcu_list + 3)<<0) ;

		switch (mcu_command)
		{
		case 0x00:
			break;

		case 0x0a:	// Copy N bytes from RAM1 to RAM2
			mcu_src		=	(cpunum_read_byte(0, mcu_address + 2)<<8) +
							(cpunum_read_byte(0, mcu_address + 3)<<0) ;

			mcu_dst		=	(cpunum_read_byte(0, mcu_address + 6)<<8) +
							(cpunum_read_byte(0, mcu_address + 7)<<0) ;

			mcu_size	=	(cpunum_read_byte(0, mcu_address + 8)<<8) +
							(cpunum_read_byte(0, mcu_address + 9)<<0) ;

			logerror("CPU #0 PC %06X : MCU executes command $A, %04X %02X-> %04x\n",activecpu_get_pc(),mcu_src,mcu_size,mcu_dst);

			for( ; mcu_size > 0 ; mcu_size-- )
			{
				mcu_src &= 0xffff;	mcu_dst &= 0xffff;
				cpunum_write_byte(1,0x100000 + mcu_dst,cpunum_read_byte(0,0x100000 + mcu_src));
				mcu_src ++;			mcu_dst ++;
			}

			/* Raise a "job done" flag */
			cpunum_write_byte(0,mcu_address+0,0xff);
			cpunum_write_byte(0,mcu_address+1,0xff);

			break;

		default:
			/* Raise a "job done" flag */
			cpunum_write_byte(0,mcu_address+0,0xff);
			cpunum_write_byte(0,mcu_address+1,0xff);

			logerror("CPU #0 PC %06X : MCU ERROR, unknown command %02X\n",activecpu_get_pc(),mcu_command);
		}

		/* Erase command? */
		cpunum_write_byte(0,mcu_list + 1,0x00);
	}
}

static WRITE16_HANDLER( galpani2_mcu_nmi_w )
{
	static UINT16 old = 0;
	if ( (data & 1) && !(old & 1) )	galpani2_mcu_nmi();
	old = data;
}



/***************************************************************************


                            CPU#1 - Main + Sound


***************************************************************************/

WRITE16_HANDLER( galpani2_coin_lockout_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0, data & 0x0100);
		coin_counter_w(1, data & 0x0200);
		coin_lockout_w(0,~data & 0x0400);
		coin_lockout_w(1,~data & 0x0800);
		// & 0x1000     CARD in lockout?
		// & 0x2000     CARD in lockout?
		// & 0x4000     CARD out
	}
}

WRITE16_HANDLER( galpani2_oki_0_bank_w )
{
	if (ACCESSING_LSB)
	{
		UINT8 *ROM = memory_region(REGION_SOUND1);
		logerror("CPU #0 PC %06X : OKI 0 bank %08X\n",activecpu_get_pc(),data);
		memcpy(ROM + 0x30000, ROM + 0x40000 + 0x10000 * (~data & 0xf), 0x10000);
	}
}

WRITE16_HANDLER( galpani2_oki_1_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(1, 0x40000 * (data & 0xf) );
		logerror("CPU #0 PC %06X : OKI 1 bank %08X\n",activecpu_get_pc(),data);
	}
}


static ADDRESS_MAP_START( galpani2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM					)	// Work RAM
	AM_RANGE(0x300000, 0x301fff) AM_READ(MRA16_RAM					)	// ?
	AM_RANGE(0x302000, 0x303fff) AM_READ(MRA16_RAM					)	// Sprites
	AM_RANGE(0x304000, 0x30401f) AM_READ(MRA16_RAM					)	// Sprites Regs
	AM_RANGE(0x310000, 0x3101ff) AM_READ(MRA16_RAM					)	// Background Palette
	AM_RANGE(0x318000, 0x318001) AM_READ(galpani2_eeprom_r			)	// EEPROM
	AM_RANGE(0x380000, 0x38ffff) AM_READ(MRA16_RAM					)	// ? + Sprites Palette
	AM_RANGE(0x400000, 0x43ffff) AM_READ(MRA16_RAM					)	// Background 0
	AM_RANGE(0x440000, 0x440001) AM_READ(MRA16_RAM					)	// Background 0 Scroll X
	AM_RANGE(0x480000, 0x480001) AM_READ(MRA16_RAM					)	// Background 0 Scroll Y
	AM_RANGE(0x500000, 0x53ffff) AM_READ(MRA16_RAM					)	// Background 1
	AM_RANGE(0x540000, 0x540001) AM_READ(MRA16_RAM					)	// Background 1 Scroll X
	AM_RANGE(0x580000, 0x580001) AM_READ(MRA16_RAM					)	// Background 1 Scroll Y
	AM_RANGE(0x780000, 0x780001) AM_READ(input_port_0_word_r		)	// Input Ports
	AM_RANGE(0x780002, 0x780003) AM_READ(input_port_1_word_r		)	//
	AM_RANGE(0x780004, 0x780005) AM_READ(input_port_2_word_r		)	//
	AM_RANGE(0x780006, 0x780007) AM_READ(input_port_3_word_r		)	//
	AM_RANGE(0xc00000, 0xc00001) AM_READ(OKIM6295_status_0_lsb_r	)	// 2 x OKIM6295
	AM_RANGE(0xc40000, 0xc40001) AM_READ(OKIM6295_status_1_lsb_r	)	//
ADDRESS_MAP_END

static ADDRESS_MAP_START( galpani2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM								)	// ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_ram				)	// Work RAM
	AM_RANGE(0x300000, 0x301fff) AM_WRITE(MWA16_RAM								)	// ?
	AM_RANGE(0x302000, 0x303fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size			)	// Sprites
	AM_RANGE(0x304000, 0x30401f) AM_WRITE(kaneko16_sprites_regs_w) AM_BASE(&kaneko16_sprites_regs	)	// Sprites Regs
	AM_RANGE(0x30c000, 0x30c001) AM_WRITE(MWA16_NOP								)	// ? hblank effect ?
	AM_RANGE(0x310000, 0x3101ff) AM_WRITE(galpani2_palette_0_w) AM_BASE(&galpani2_palette_0		)	// ?
	AM_RANGE(0x314000, 0x314001) AM_WRITE(MWA16_NOP								)	// ? flip backgrounds ?
	AM_RANGE(0x318000, 0x318001) AM_WRITE(galpani2_eeprom_w						)	// EEPROM
	AM_RANGE(0x380000, 0x387fff) AM_WRITE(MWA16_RAM								)	// Palette?
	AM_RANGE(0x388000, 0x38ffff) AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16	)	// Palette
	AM_RANGE(0x400000, 0x43ffff) AM_WRITE(galpani2_bg8_0_w) AM_BASE(&galpani2_bg8_0		)	// Background 0
	AM_RANGE(0x440000, 0x440001) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_bg8_0_scrollx	)	// Background 0 Scroll X
	AM_RANGE(0x480000, 0x480001) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_bg8_0_scrolly	)	// Background 0 Scroll Y
	AM_RANGE(0x4c0000, 0x4c0001) AM_WRITE(MWA16_NOP								)	// ? 0 at startup only
	AM_RANGE(0x500000, 0x53ffff) AM_WRITE(galpani2_bg8_1_w) AM_BASE(&galpani2_bg8_1		)	// Background 1
	AM_RANGE(0x540000, 0x540001) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_bg8_1_scrollx	)	// Background 1 Scroll X
	AM_RANGE(0x580000, 0x580001) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_bg8_1_scrolly	)	// Background 1 Scroll Y
	AM_RANGE(0x5c0000, 0x5c0001) AM_WRITE(MWA16_NOP								)	// ? 0 at startup only
	AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP								)	// Watchdog
//  AM_RANGE(0x640000, 0x640001) AM_WRITE(MWA16_NOP                             )   // ? 0 before resetting and at startup
//  AM_RANGE(0x680000, 0x680001) AM_WRITE(MWA16_NOP                             )   // ? 0 -> 1 -> 0 (lev 5) / 0 -> $10 -> 0
AM_RANGE(0x680000, 0x680001) AM_WRITE(galpani2_mcu_nmi_w	)	// ? 0 -> 1 -> 0 (lev 5) / 0 -> $10 -> 0
	AM_RANGE(0x6c0000, 0x6c0001) AM_WRITE(galpani2_coin_lockout_w				)	// Coin + Card Lockout
	AM_RANGE(0xc00000, 0xc00001) AM_WRITE(OKIM6295_data_0_lsb_w					)	// 2 x OKIM6295
	AM_RANGE(0xc40000, 0xc40001) AM_WRITE(OKIM6295_data_1_lsb_w					)	//
	AM_RANGE(0xc80000, 0xc80001) AM_WRITE(galpani2_oki_0_bank_w					)	//
	AM_RANGE(0xcc0000, 0xcc0001) AM_WRITE(galpani2_oki_1_bank_w					)	//
ADDRESS_MAP_END


/***************************************************************************


                            CPU#2 - Backgrounds


***************************************************************************/

static UINT16 *galpani2_rombank;

READ16_HANDLER( galpani2_bankedrom_r )
{
	UINT16 *ROM = (UINT16 *) memory_region( REGION_USER1 );
	size_t    len = memory_region_length( REGION_USER1 ) / 2;

	offset += (0x800000/2) * (*galpani2_rombank & 0x0003);

	if ( offset < len )	return ROM[offset];
	else				return 0xffff;
}


static ADDRESS_MAP_START( galpani2_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM					)	// ROM
	AM_RANGE(0x100000, 0x13ffff) AM_READ(MRA16_RAM					)	// Work RAM
	AM_RANGE(0x400000, 0x4fffff) AM_READ(MRA16_RAM					)	// bg15
	AM_RANGE(0x500000, 0x5fffff) AM_READ(MRA16_RAM					)	// bg15
	AM_RANGE(0x800000, 0xffffff) AM_READ(galpani2_bankedrom_r		)	// Banked ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( galpani2_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM						)	// ROM
	AM_RANGE(0x100000, 0x13ffff) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_ram2		)	// Work RAM
	AM_RANGE(0x400000, 0x4fffff) AM_WRITE(galpani2_bg15_w) AM_BASE(&galpani2_bg15	)	// bg15
	AM_RANGE(0x500000, 0x5fffff) AM_WRITE(MWA16_RAM							)	// bg15
	AM_RANGE(0x600000, 0x600001) AM_WRITE(MWA16_NOP						)	// ? 0 at startup only
	AM_RANGE(0x640000, 0x640001) AM_WRITE(MWA16_NOP						)	// ? 0 at startup only
	AM_RANGE(0x680000, 0x680001) AM_WRITE(MWA16_NOP						)	// ? 0 at startup only
	AM_RANGE(0x6c0000, 0x6c0001) AM_WRITE(MWA16_NOP						)	// ? 0 at startup only
	AM_RANGE(0x700000, 0x700001) AM_WRITE(MWA16_NOP						)	// Watchdog
	AM_RANGE(0x780000, 0x780001) AM_WRITE(MWA16_NOP						)	// ? 0 -> 1 -> 0 (lev 5)
	AM_RANGE(0x7c0000, 0x7c0001) AM_WRITE(MWA16_RAM) AM_BASE(&galpani2_rombank	)	// Rom Bank
ADDRESS_MAP_END


/***************************************************************************


                                Input Ports


***************************************************************************/

INPUT_PORTS_START( galpani2 )
	PORT_START_TAG("IN0")	// IN0 - DSW + Player - 780000.w
	PORT_DIPNAME( 0x0007, 0x0007, "Unknown 2-0&1&2*" )
	PORT_DIPSETTING(      0x007, "7" )
	PORT_DIPSETTING(      0x006, "6" )
	PORT_DIPSETTING(      0x005, "5" )
	PORT_DIPSETTING(      0x004, "4" )
	PORT_DIPSETTING(      0x003, "3" )
	PORT_DIPSETTING(      0x002, "2" )
	PORT_DIPSETTING(      0x001, "1" )
	PORT_DIPSETTING(      0x000, "0" )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3*" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, "Unknown 2-4&5" )	// from?
	PORT_DIPSETTING(      0x0030, "3" )	// 2
	PORT_DIPSETTING(      0x0020, "2" )	// 0
	PORT_DIPSETTING(      0x0010, "1" )	// 3
	PORT_DIPSETTING(      0x0000, "0" )	// 4
	PORT_DIPNAME( 0x00c0, 0x00c0, "Unknown 2-6&7" )	// to?
	PORT_DIPSETTING(      0x00c0, "3" )	// 9
	PORT_DIPSETTING(      0x0080, "2" )	// 1
	PORT_DIPSETTING(      0x0040, "1" )	// 4
	PORT_DIPSETTING(      0x0000, "0" )	// 6

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN1")	// IN1 - DSW + Player - 780002.w
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x000f, "1 Coin/1 Credit  1/1" )
	PORT_DIPSETTING(      0x000e, "2 Coin/1 Credit  2/1" )
	PORT_DIPSETTING(      0x000d, "3 Coin/1 Credit  3/1" )
	PORT_DIPSETTING(      0x000c, "4 Coin/1 Credit  4/1" )
	PORT_DIPSETTING(      0x000b, "5 Coin/1 Credit  5/1" )
	PORT_DIPSETTING(      0x000a, "2 Coin/1 Credit  1/1" )
	PORT_DIPSETTING(      0x0009, "3 Coin/1 Credit  1/1" )
	PORT_DIPSETTING(      0x0008, "4 Coin/1 Credit  1/1" )
	PORT_DIPSETTING(      0x0007, "5 Coin/1 Credit  1/1" )
	PORT_DIPSETTING(      0x0006, "2 Coin/1 Credit  2/1" )
	PORT_DIPSETTING(      0x0005, "3 Coin/1 Credit  2/1" )
	PORT_DIPSETTING(      0x0004, "4 Coin/1 Credit  2/1" )
	PORT_DIPSETTING(      0x0003, "5 Coin/1 Credit  2/1" )
	PORT_DIPSETTING(      0x0002, "1 Coin/2 Credit  1/2" )
	PORT_DIPSETTING(      0x0001, "1 Coin/3 Credit  1/3" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4*" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN2")	// IN2 - Coins - 780004.w
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_SPECIAL )	// CARD full
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	// CARD full
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	// CARD empty

	PORT_START_TAG("IN3")	// IN3 - ? - 780006.w
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_SERVICE_NO_TOGGLE( 0x2000, IP_ACTIVE_LOW)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END



/***************************************************************************


                                Graphics Layouts


***************************************************************************/

/*
    16x16x8 made of four 8x8x8 tiles arrenged like: 01
                                                    23
*/
static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8),   STEP8(8*8*8*1,8)   },
	{ STEP8(0,8*8), STEP8(8*8*8*2,8*8) },
	16*16*8
};

static const gfx_decode galpani2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,	0,	0x40	}, // [0] Sprites
	{ -1 }
};

/***************************************************************************


                                Machine Drivers


***************************************************************************/


/* CPU#1 Interrups */
#define GALPANI2_INTERRUPTS_NUM	4
INTERRUPT_GEN( galpani2_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 3:  cpunum_set_input_line(0, 3, HOLD_LINE); break;
		case 2:  cpunum_set_input_line(0, 4, HOLD_LINE); break;
		case 1:  cpunum_set_input_line(0, 5, HOLD_LINE); break;	// vblank?
		case 0:  cpunum_set_input_line(0, 6, HOLD_LINE); break;	// hblank?
	}
}

/* CPU#2 Interrups */
/* lev 3,4 & 5 are tested on power up. The rest is rte, but lev 7 */
#define GALPANI2_INTERRUPTS_NUM2	3
INTERRUPT_GEN( galpani2_interrupt2 )
{
	switch ( cpu_getiloops() )
	{
		case 2:  cpunum_set_input_line(1, 3, HOLD_LINE); break;
		case 1:  cpunum_set_input_line(1, 4, HOLD_LINE); break;
		case 0:  cpunum_set_input_line(1, 5, HOLD_LINE); break;
	}
}

static MACHINE_DRIVER_START( galpani2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(galpani2_readmem,galpani2_writemem)
	MDRV_CPU_VBLANK_INT(galpani2_interrupt,GALPANI2_INTERRUPTS_NUM)

	MDRV_CPU_ADD(M68000, 16000000)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(galpani2_readmem2,galpani2_writemem2)
	MDRV_CPU_VBLANK_INT(galpani2_interrupt2,GALPANI2_INTERRUPTS_NUM2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(galpani2)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 256-1-16)
	MDRV_GFXDECODE(galpani2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000 + 0x200 + 0x8000)	// sprites, bg8, bg15
	MDRV_COLORTABLE_LENGTH(0x4000)

	MDRV_PALETTE_INIT(galpani2)
	MDRV_VIDEO_START(galpani2)
	MDRV_VIDEO_UPDATE(galpani2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 12000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************


                                Roms Loading


***************************************************************************/

/***************************************************************************

                                Gals Panic II


Location      Device         File ID       Checksum
---------------------------------------------------
CPU U134      27C4001      G001T1-U134-0     21A3   [ CPU 2 PROG ]
CPU U125       27C010      G002T1-U125-0     8072   [ CPU 1 PROG ]
CPU U126       27C010      G003T1-U126-0     C9C4   [ CPU 1 PROG ]
CPU U133      27C4001      G204T1-U133-0     9EF7   [ CPU 2 PROG ]
ROM U27        27C020      G204T1-U27-00     CA1D
ROM U33        27C020      G204T1-U33-00     AF5D
ROM U3       LH538500      GP2-100-0043      C90A   [ SOUND DATA ]
ROM U7       LH538500      GP2-101-0044      CFEF   [ SOUND DATA ]
ROM U10     KM2316000      GP2-102-0045      1558   [ SOUND DATA ]
ROM U21      LM538500      GP2-200-0046      2E2E
ROM U20      LH538500      GP2-201-0047      DB6E
ROM U19      LH538500      GP2-202-0048      E181
ROM U18      LH538500      GP2-203-0049      E520
ROM U51      LH538500      GP2-300A-0052     50E9
ROM U52      LH538500      GP2-300B-0053     DC51
ROM U60     KM2316000      GP2-301-0035      35F6
ROM U59     KM2316000      GP2-302-0036      BFF5
ROM U58     KM2316000      GP2-303-0037      B860
ROM U57     KM2316000      GP2-304-0038      BD55
ROM U56     KM2316000      GP2-305-0039      D0F4
ROM U55     KM2316000      GP2-306-0040      1311
ROM U54     KM2316000      GP2-307-0041      1874
ROM U53     KM2316000      GP2-308-0042      375F
ROM U46      LH538500      GP2-309A-0050     97ED
ROM U47      LH538500      GP2-309B-0051     2C13
ROM U48      LH538500      GP2-309A-0055     2059
ROM U75      GAL16V8A      S075.JED          08F9
ROM U76      GAL16V8A      S076.JED          0878
ROM U1      PEEL18CV8      S001.JED          03CA
ROM U14     PEEL18CV8      S014.JED          039A


Notes:  CPU - Main PCB   Z04G2-003
        ROM - ROM PCB    Z04G2-SUB3

        Checksums for the PLDs are the JEDEC checksums, not the file checksums

Brief Hardware Overview
-----------------------

CPU1         - 68HC000-16
CPU2         - 68HC000-16
Sound     2x - M6295

Custom ICs   - 10x PQFPs

***************************************************************************/

ROM_START( galpani2 )
 	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* CPU#1 Code */
	ROM_LOAD16_BYTE( "g000t1.133", 0x000000, 0x080000, CRC(332048e7) SHA1(1a353d4b29f7a08158fc454309dc496df6b5b108) )
	ROM_LOAD16_BYTE( "g001t1.134", 0x000001, 0x080000, CRC(c92937c3) SHA1(0c9e894c0e23e319bd2d01ec573f02ed510e3ed6) )

 	ROM_REGION( 0x1000000, REGION_CPU2, 0 )			/* CPU#2 Code */
	ROM_LOAD16_BYTE( "g002t1.125", 0x000000, 0x020000, CRC(a3034e1c) SHA1(493e4be36f2aea0083d5d37e16486ed66dab952e) )
	ROM_LOAD16_BYTE( "g003t1.126", 0x000001, 0x020000, CRC(20d3a2ad) SHA1(93450e5a23456c242ebf1a3560013a17c6b05354) )

	ROM_REGION16_BE( 0x1500000, REGION_USER1, 0 )	/* Backgrounds (CPU2) */
	ROM_LOAD16_BYTE( "gp2-300b.053", 0x0000000, 0x100000, CRC(d7d12920) SHA1(4b6e01cc0ac5192758f4b3d26f102905b2b5e8ac) )
	ROM_LOAD16_BYTE( "gp2-300a.052", 0x0000001, 0x100000, CRC(09ebedba) SHA1(3c06614633f0da03facb5199deac492b8ce07257) )
	ROM_LOAD( "gp2-301.035", 0x0200000, 0x200000, CRC(e71e749d) SHA1(420c4c085e89d9641a84e34fa870df2bc02165b6) )
	ROM_LOAD( "gp2-302.036", 0x0400000, 0x200000, CRC(832ebbb0) SHA1(a753285d874fcab979e70d6a289cf9fcd48affc6) )
	ROM_LOAD( "gp2-303.037", 0x0600000, 0x200000, CRC(36c872d0) SHA1(e0aa3089dfa1765ba70ce60e8696b1ba87c95703) )
	ROM_LOAD( "gp2-304.038", 0x0800000, 0x200000, CRC(7200f918) SHA1(6d23bd371b32319fdd08923deb81278b36b9cd79) )
	ROM_LOAD( "gp2-305.039", 0x0a00000, 0x200000, CRC(a308dc4b) SHA1(db40329c383c765471941ab89fded6b8789d29c7) )
	ROM_LOAD( "gp2-306.040", 0x0c00000, 0x200000, CRC(cd294225) SHA1(c51c95d5edd5e5d7191ccbfa1ba2e92199bb04b9) )
	ROM_LOAD( "gp2-307.041", 0x0e00000, 0x200000, CRC(0fda01af) SHA1(ca30d995ff8d83b46c05898a2ecde3f08a95c788) )
	ROM_LOAD( "gp2-308.042", 0x1000000, 0x200000, CRC(3c806376) SHA1(5c440a0cfd5d5c07ff074bc0c2563956d256a80e) )
	ROM_LOAD16_BYTE( "gp2-309a.050", 0x1200000, 0x100000, CRC(2c025ec3) SHA1(bc25ad92415e662d6b0f845aa4621a733fbf5a48) )
	ROM_LOAD16_BYTE( "gp2-309b.051", 0x1200001, 0x100000, CRC(e8bf1730) SHA1(0d9a446aecc19a43368550348745c9b167ec4941) )
	ROM_LOAD( "gp2-310a.055", 0x1400000, 0x100000, CRC(01eca246) SHA1(19cb35d7873b84486f9105127a1e3cf3235d3109) )

	ROM_REGION( 0x480000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "gp2-200.046", 0x080000, 0x080000, CRC(11b49470) SHA1(d11c2374a7c9b9b0d1f27c29759b16630700561d) )
	ROM_CONTINUE(            0x000000, 0x080000             )
	ROM_LOAD( "gp2-201.047", 0x180000, 0x080000, CRC(2f6392b4) SHA1(67446974c00481a7a806f4bc5b10eb6e442a1186) )
	ROM_CONTINUE(            0x100000, 0x080000             )
	ROM_LOAD( "gp2-202.048", 0x280000, 0x080000, CRC(c8177181) SHA1(30d0a49334e370eb1b45d2eb6501df3f857a95d5) )
	ROM_CONTINUE(            0x200000, 0x080000             )
	ROM_LOAD( "gp2-203.049", 0x380000, 0x080000, CRC(14e0cb38) SHA1(d9a778ebf0c6b67bee5f6f7016cb9ead96c6a992) )
	ROM_CONTINUE(            0x300000, 0x080000             )
	ROM_LOAD16_BYTE( "g204t1.33", 0x400000, 0x040000, CRC(65a1f838) SHA1(ccc3bb4a4f4ea1677caa1a3a51bc0a13b4b619c7) )
	ROM_LOAD16_BYTE( "g204t1.27", 0x400001, 0x040000, CRC(39059f66) SHA1(6bf41738033a13b63d96babf827c73c914323425) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gp2-100.043", 0x040000, 0x100000, CRC(4235ac5b) SHA1(7e35831523fbb2d0587b9ab93c13b2b43dc481a8) )	// $10 x $10000
	ROM_COPY( REGION_SOUND1, 0x0c0000, 0, 0x40000 )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gp2-102.045", 0x000000, 0x100000, CRC(b4bee779) SHA1(a41098e4b8e48577719dc4bd7f09f5e893e8b388) )	//  $8 x $40000
	ROM_CONTINUE(            0x200000, 0x100000 )
	ROM_LOAD( "gp2-101.044", 0x100000, 0x100000, CRC(f75ba6a0) SHA1(91cc0c019a7ebfa2562bbe570af029f00b5e0699) )	//  $4 x $40000
	ROM_RELOAD(              0x300000, 0x100000 )
ROM_END
/*
CPU #0 PC 02F8F4 : OKI 1 bank 00000007
CPU #0 PC 02F918 : OKI 1 (030)000000-000000
043(16 banks):  0x10000 *: 2,8,
044(4 banks):   0x40000 *: 1,3,
045(8 banks):   0x40000 *: 1,
CPU #0 PC 02F8D6 : OKI 0 bank 0000000A
CPU #0 PC 02F918 : OKI 0 (380)000000-000000
043(16 banks):  0x10000 *: 5,8,
044(4 banks):   0x40000 *: -
045(8 banks):   0x40000 *: -
*/

/*

Gals Panic II - Quiz Version (c) 1993 Kaneko

CPUs: 68HC000-16 (x2)
Sound: OKI6295 (x2)
Customs: KC-BYO KA05-1068 (x2), KC-TAS KA07-1209, KC-YUU KA06-0041, KC-SHU KA03-1849 (x2), KC-001 (x2), PISCES, KC-002
RAM: 6116 (x6), 52B256 (x4), 42101 (x2), 42426 (x2), 6264 (x2), 431000 (x8)
X1: 27 MHz
X2: 16 MHz
X3: 33.333 MHz
X4: 20 MHz

*/

ROM_START( gpan2qiz )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* CPU#1 Code */
	ROM_LOAD16_BYTE( "g000j2.165", 0x000000, 0x080000, CRC(e0c5a03d) SHA1(e12457400ca8cd78674b44d7f4d664cfc0afc8c9) )
	ROM_LOAD16_BYTE( "g001j2.164", 0x000001, 0x080000, CRC(c8e12223) SHA1(0e0160565e95cb33dc6ad796225e995ed3baf8eb) )

	ROM_REGION( 0x1000000, REGION_CPU2, 0 )			/* CPU#2 Code */
	ROM_LOAD16_BYTE( "g002j1.64",  0x000000, 0x020000, CRC(5e523829) SHA1(dad11e4a3348c988ff658609cf78a3fbee58064e) )
	ROM_LOAD16_BYTE( "g003j1.63",  0x000001, 0x020000, CRC(2a0d5f89) SHA1(0a7031c4b8b7bc757da25250dbb5fa1004205aeb) )

	ROM_REGION16_BE( 0x800000, REGION_USER1, 0 )	/* Backgrounds (CPU2) */
	ROM_LOAD( "gp2-300j.175", 0x000000, 0x200000, CRC(3a0afc1d) SHA1(91fba9074cc3c28e919053f0ea07b28d88b2ce5f) )
	ROM_LOAD( "gp2-301j.176", 0x200000, 0x200000, CRC(5b6d1709) SHA1(a7d35247fe71895f2b6169409aa0bdaef446804c) )
	ROM_LOAD16_BYTE( "gp2-302a.177", 0x400000, 0x100000, CRC(311fa273) SHA1(c2adeac45be701f6f474841755fac4347d44f844) )
	ROM_LOAD16_BYTE( "gp2-302b.178", 0x400001, 0x100000, CRC(80cb211b) SHA1(7567c9d1309edddb9c1fa68346506de48e91ca6a) )
	ROM_LOAD16_BYTE( "gp2-303a.179", 0x600000, 0x100000, CRC(162d83b7) SHA1(16daf2ba09e63eaca5e50c944472773b1774c946) )
	ROM_LOAD16_BYTE( "gp2-303b.180", 0x600001, 0x100000, CRC(458a1fbc) SHA1(971548ec8cce592773e762a0c972264013b7cb8d) )

	ROM_REGION( 0x480000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "gp2-200j.189", 0x000000, 0x200000, CRC(2f81e519) SHA1(c07f4dad15b6f7f1fb867f773c0ada309d172326) )
	ROM_LOAD( "gp2-201j.171", 0x200000, 0x200000, CRC(bbe404e0) SHA1(198db9a6c6ec97ed8fd32d946051ba4d6e4bd354) )
	ROM_LOAD16_BYTE( "g204j0.169", 0x400000, 0x040000, CRC(212d8aab) SHA1(459f556978ef9a103279cf633fcc1cacb367ea61) )
	ROM_LOAD16_BYTE( "g204j1.170", 0x400001, 0x040000, CRC(bfd89343) SHA1(884d17b3302643d86f84a4a4917de850c5bf8924) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "gp2-100j.61", 0x040000, 0x100000, CRC(60382cbf) SHA1(766c50a3302bc11d54de49a2850522d93fc36ba2) )
	ROM_COPY( REGION_SOUND1, 0x0c0000, 0, 0x40000 )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 )	/* Samples */
	ROM_LOAD( "gp2-102.59",  0x280000, 0x080000, CRC(1bed6ecd) SHA1(3208aacac64ac95fcb6eeef59986c3154c1c415b) )
	ROM_CONTINUE(            0x200000, 0x080000 )
	ROM_CONTINUE(            0x080000, 0x080000 )
	ROM_CONTINUE(            0x000000, 0x080000 )
	ROM_LOAD( "gp2-101.60",  0x180000, 0x080000, CRC(3c45134f) SHA1(a5362bfcc6beb6e776c1bce4544475f8947fccea) )
	ROM_CONTINUE(            0x100000, 0x080000 )
	ROM_COPY( REGION_SOUND2, 0x100000, 0x300000, 0x100000 )
ROM_END

GAME( 1993, galpani2, 0, galpani2, galpani2, 0, ROT90, "Kaneko", "Gals Panic II (Japan?)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION )
GAME( 1993, gpan2qiz, 0, galpani2, galpani2, 0, ROT90, "Kaneko", "Gals Panic II - Quiz Version (Japan)", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION )
