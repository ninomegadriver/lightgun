/***************************************************************************

X-Men

driver by Nicola Salmoria

notes:

the way the double screen works in xmen6p is not fully understood

the board only has one of each gfx chip, the only additional chip not found
on the 2/4p board is 053253.  This chip is also on Run n Gun which should
likewise be a 2 screen game

***************************************************************************/
#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "machine/eeprom.h"
#include "cpu/z80/z80.h"
#include "sound/2151intf.h"
#include "sound/k054539.h"

VIDEO_START( xmen );
VIDEO_START( xmen6p );
VIDEO_UPDATE( xmen );
VIDEO_UPDATE( xmen6p );
VIDEO_EOF( xmen6p );

UINT16 xmen_current_frame;

/***************************************************************************

  EEPROM

***************************************************************************/

static int init_eeprom_count;


static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER( xmen )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( eeprom_r )
{
	int res;

logerror("%06x eeprom_r\n",activecpu_get_pc());
	/* bit 6 is EEPROM data */
	/* bit 7 is EEPROM ready */
	/* bit 14 is service button */
	res = (EEPROM_read_bit() << 6) | input_port_2_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xbfff;
	}
	return res;
}

static READ16_HANDLER( xmen6p_eeprom_r )
{
	int res;

logerror("%06x xmen6p_eeprom_r\n",activecpu_get_pc());
	/* bit 6 is EEPROM data */
	/* bit 7 is EEPROM ready */
	/* bit 14 is service button */
	res = (EEPROM_read_bit() << 6) | input_port_2_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xbfff;
	}
	return (res&0x7fff)|xmen_current_frame;
}


static WRITE16_HANDLER( eeprom_w )
{
logerror("%06x: write %04x to 108000\n",activecpu_get_pc(),data);
	if (ACCESSING_LSB)
	{
		/* bit 0 = coin counter */
		coin_counter_w(0,data & 0x01);

		/* bit 2 is data */
		/* bit 3 is clock (active high) */
		/* bit 4 is cs (active low) */
		EEPROM_write_bit(data & 0x04);
		EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
	if (ACCESSING_MSB)
	{
		/* bit 8 = enable sprite ROM reading */
		K053246_set_OBJCHA_line((data & 0x0100) ? ASSERT_LINE : CLEAR_LINE);
		/* bit 9 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x0200) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static READ16_HANDLER( sound_status_r )
{
	return soundlatch2_r(0);
}

static WRITE16_HANDLER( sound_cmd_w )
{
	if (ACCESSING_LSB) {
		data &= 0xff;
		soundlatch_w(0, data);
	}
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

//int xmen_irqenabled;

static WRITE16_HANDLER( xmen_18fa00_w )
{
	if(ACCESSING_LSB) {
		/* bit 2 is interrupt enable */
		interrupt_enable_w(0,data & 0x04);
	//  xmen_irqenabled = data;
	}
}

static UINT8 sound_curbank;

static void sound_reset_bank(void)
{
	memory_set_bankptr(4, memory_region(REGION_CPU2) + 0x10000 + (sound_curbank & 0x07) * 0x4000);
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	sound_curbank = data;
	sound_reset_bank();
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_READ(K053247_word_r)
	AM_RANGE(0x101000, 0x101fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x104000, 0x104fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x108054, 0x108055) AM_READ(sound_status_r)
	AM_RANGE(0x10a000, 0x10a001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x10a002, 0x10a003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x10a004, 0x10a005) AM_READ(eeprom_r)
	AM_RANGE(0x10a00c, 0x10a00d) AM_READ(K053246_word_r)
	AM_RANGE(0x110000, 0x113fff) AM_READ(MRA16_RAM)	/* main RAM */
	AM_RANGE(0x18c000, 0x197fff) AM_READ(K052109_lsb_r)


ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_WRITE(K053247_word_w)
	AM_RANGE(0x101000, 0x101fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x104000, 0x104fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x108000, 0x108001) AM_WRITE(eeprom_w)
	AM_RANGE(0x108020, 0x108027) AM_WRITE(K053246_word_w)
	AM_RANGE(0x10804c, 0x10804d) AM_WRITE(sound_cmd_w)
	AM_RANGE(0x10804e, 0x10804f) AM_WRITE(sound_irq_w)
	AM_RANGE(0x108060, 0x10807f) AM_WRITE(K053251_lsb_w)
	AM_RANGE(0x10a000, 0x10a001) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x110000, 0x113fff) AM_WRITE(MWA16_RAM)	/* main RAM */
	AM_RANGE(0x18fa00, 0x18fa01) AM_WRITE(xmen_18fa00_w)
	AM_RANGE(0x18c000, 0x197fff) AM_WRITE(K052109_lsb_w)

ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK4)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_READ(K054539_0_r)
	AM_RANGE(0xec01, 0xec01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xf002, 0xf002) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_WRITE(K054539_0_w)
	AM_RANGE(0xe800, 0xe800) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xec01, 0xec01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(sound_bankswitch_w)
ADDRESS_MAP_END

/* 6p version */

static ADDRESS_MAP_START( readmem6p, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_READ(MRA16_RAM) /* 1st sprite list on 6p */
	AM_RANGE(0x101000, 0x101fff) AM_READ(MRA16_RAM)


	AM_RANGE(0x102000, 0x102fff) AM_READ(MRA16_RAM) /* 2nd sprite list on 6p */
	AM_RANGE(0x102000, 0x103fff) AM_READ(MRA16_RAM) /* 6p - a buffer? */


	AM_RANGE(0x104000, 0x104fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x108054, 0x108055) AM_READ(sound_status_r)
	AM_RANGE(0x10a000, 0x10a001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x10a002, 0x10a003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x10a004, 0x10a005) AM_READ(xmen6p_eeprom_r)
	AM_RANGE(0x10a006, 0x10a007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x10a00c, 0x10a00d) AM_READ(K053246_word_r) /* sprites */
	AM_RANGE(0x110000, 0x113fff) AM_READ(MRA16_RAM)	/* main RAM */
	AM_RANGE(0x18c000, 0x197fff) AM_READ(MRA16_RAM) /* left tilemap */

/*
    AM_RANGE(0x1ac000, 0x1af7ff) AM_READ(MRA16_RAM)
    AM_RANGE(0x1b0000, 0x1b37ff) AM_READ(MRA16_RAM)
    AM_RANGE(0x1b4000, 0x1b77ff) AM_READ(MRA16_RAM)
*/
	AM_RANGE(0x1ac000, 0x1b7fff) AM_READ(MRA16_RAM) /* right tilemap */


	/* what are the regions below buffers? (used by hw or software?) */
/*
    AM_RANGE(0x1cc000, 0x1cf7ff) AM_READ(MRA16_RAM)
    AM_RANGE(0x1d0000, 0x1d37ff) AM_READ(MRA16_RAM)
*/
	AM_RANGE(0x1cc000, 0x1d7fff) AM_READ(MRA16_RAM) /* tilemap ?*/

/*
    AM_RANGE(0x1ec000, 0x1ef7ff) AM_READ(MRA16_RAM)
    AM_RANGE(0x1f0000, 0x1f37ff) AM_READ(MRA16_RAM)
    AM_RANGE(0x1f4000, 0x1f77ff) AM_READ(MRA16_RAM)
*/
	AM_RANGE(0x1ec000, 0x1f7fff) AM_READ(MRA16_RAM) /* tilemap ?*/

ADDRESS_MAP_END

UINT16*xmen6p_spriteramleft;
UINT16*xmen6p_spriteramright;
UINT16*xmen6p_tilemapleft;
UINT16*xmen6p_tilemapright;

static ADDRESS_MAP_START( writemem6p, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_WRITE(MWA16_RAM) AM_BASE(&xmen6p_spriteramleft) /* sprites (screen 1) */
	AM_RANGE(0x101000, 0x101fff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x102000, 0x102fff) AM_WRITE(MWA16_RAM) AM_BASE(&xmen6p_spriteramright)/* sprites (screen 2) */
	AM_RANGE(0x103000, 0x103fff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x104000, 0x104fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x108000, 0x108001) AM_WRITE(eeprom_w)
	AM_RANGE(0x108020, 0x108027) AM_WRITE(K053246_word_w) /* sprites */
	AM_RANGE(0x10804c, 0x10804d) AM_WRITE(sound_cmd_w)
	AM_RANGE(0x10804e, 0x10804f) AM_WRITE(sound_irq_w)
	AM_RANGE(0x108060, 0x10807f) AM_WRITE(K053251_lsb_w)
	AM_RANGE(0x10a000, 0x10a001) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x110000, 0x113fff) AM_WRITE(MWA16_RAM)	/* main RAM */
	AM_RANGE(0x18fa00, 0x18fa01) AM_WRITE(xmen_18fa00_w)
	AM_RANGE(0x18c000, 0x197fff) AM_WRITE(MWA16_RAM/*K052109_lsb_w*/) AM_BASE(&xmen6p_tilemapleft) /* left tilemap (p1,p2,p3 counters) */

/*
    AM_RANGE(0x1ac000, 0x1af7ff) AM_WRITE(MWA16_RAM)
    AM_RANGE(0x1b0000, 0x1b37ff) AM_WRITE(MWA16_RAM)
    AM_RANGE(0x1b4000, 0x1b77ff) AM_WRITE(MWA16_RAM)
*/
	AM_RANGE(0x1ac000, 0x1b7fff) AM_WRITE(MWA16_RAM) AM_BASE(&xmen6p_tilemapright)/* tilemap right side (p4,p5,p6 counters) */

	/* whats the stuff below, buffers? */

/*
    AM_RANGE(0x1cc000, 0x1cf7ff) AM_WRITE(MWA16_RAM)
    AM_RANGE(0x1d0000, 0x1d37ff) AM_WRITE(MWA16_RAM)
*/
	AM_RANGE(0x1cc000, 0x1d7fff) AM_WRITE(MWA16_RAM) /* tilemap? */

/*
    AM_RANGE(0x1ec000, 0x1ef7ff) AM_WRITE(MWA16_RAM)
    AM_RANGE(0x1f0000, 0x1f37ff) AM_WRITE(MWA16_RAM)
    AM_RANGE(0x1f4000, 0x1f77ff) AM_WRITE(MWA16_RAM)
*/
	AM_RANGE(0x1ec000, 0x1f7fff) AM_WRITE(MWA16_RAM) /*  tilemap? */

ADDRESS_MAP_END



INPUT_PORTS_START( xmen )
	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* COIN  EEPROM and service */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status - always 1 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x3000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
INPUT_PORTS_END

INPUT_PORTS_START( xmen6p )
	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* COIN  EEPROM and service */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status - always 1 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START5 ) /* not verified */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START6 ) /* not verified */
	PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* screen indicator? */

	/* players 5+6 */
	PORT_START	/*  */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(5)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(5)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(5)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(5)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(5)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(5)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN5 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(6)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(6)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(6)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(6)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(6)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(6)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(6)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN6 )
INPUT_PORTS_END

INPUT_PORTS_START( xmen2p )
	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
/*
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )
*/

	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
/*
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )
*/

	PORT_START	/* COIN  EEPROM and service */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x003c, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM status - always 1 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x3000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
INPUT_PORTS_END



static struct K054539interface k054539_interface =
{
	REGION_SOUND1
};



static INTERRUPT_GEN( xmen_interrupt )
{
	if (cpu_getiloops() == 0) irq5_line_hold();
	else irq3_line_hold();
}

static MACHINE_START( xmen )
{
	state_save_register_global(sound_curbank);
	state_save_register_func_postload(sound_reset_bank);
	return 0;
}

static MACHINE_DRIVER_START( xmen )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(xmen_interrupt,2)

	MDRV_CPU_ADD(Z80,8000000)	/* verified with M1, guessed but accurate */
	/* audio CPU */	/* ????? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_MACHINE_START(xmen)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(xmen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(xmen)
	MDRV_VIDEO_UPDATE(xmen)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)
MACHINE_DRIVER_END


static MACHINE_RESET(xmen6p)
{
	xmen_current_frame = 0x0000;
}

static INTERRUPT_GEN( xmen6p_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		irq5_line_hold();


	}
	else
	{
//      if (xmen_irqenabled&0x04)
//      {
			irq3_line_hold();
//          xmen_current_frame = 0x0000;

//      }
	}
}

static MACHINE_DRIVER_START( xmen6p )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(readmem6p,writemem6p)
	MDRV_CPU_VBLANK_INT(xmen6p_interrupt,2)

	MDRV_CPU_ADD(Z80,8000000)	/* verified with M1, guessed but accurate */
	/* audio CPU */	/* ????? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(xmen)

	MDRV_MACHINE_RESET(xmen6p)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(128*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, 84*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)
	MDRV_ASPECT_RATIO(8,3)

	MDRV_VIDEO_START(xmen6p)
	MDRV_VIDEO_UPDATE(xmen6p)
	MDRV_VIDEO_EOF(xmen6p)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*

    KONAMI - X-MEN 2P/4P - GX065 - PWB353018A

    +--------------------------------------------------------------------------------------+
    |                                                                                      |
    |                                  [  065A06.1f  ]     [  065A12.1h  ][  065A11.1l  ]  |
    |                                                                                      |
    | sound                                                [  065A09.1h  ][  065A10.1l  ]  |
    |  out                                                                                 |
    |                                                                                      |
    |        [  054544  ] [  054539  ]                                                     |
    |                                                                                      |
    |                                                                                      |
    +-+     [    Z80    ] [ YM2151 ] [  065*01.6f  ]                                       |
      |                                                                                    |
    +-+                                                                     [  053246  ]   |
    |                                                                                      |
    |                   [  065A02.9d  ]  [  065A03.9f  ]                                   |
    +-+                                                                                    |
    +-+                 [  065*04.10d ]  [  065*05.10f ]                                   |
    |   J                                                                                  |
    |   A                                                                                  |
    |   M                                                                   [  053247  ]   |
    |   M                                                                                  |
    |   A              [     68000 - 16Mhz     ]  [qz 24Mhz]                               |
    |                                                                                      |
    |                                                                                      |
    |                                                                                      |
    |                    [qz 32Mhz/18.432Mhz]   [  052109  ] [  051962  ]   [  053251  ]   |
    |                                                                                      |
    +-+                                                                                    |
      |                                                                    [  065A08.15l ] |
    +-+                                                                                    |
    |                                                                      [  065A07.16l ] |
    |     [cn7 (4P)]                                                                       |
    |test                                                                                  |
    | sw  [cn6 (3P)]                                                                       |
    |                                                                                      |
    +--------------------------------------------------------------------------------------+

    054544
    054539
    053246
    053247
    052109
    051962
    053251
*/

ROM_START( xmen )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "065-ubb04.10d",  0x00000, 0x20000, CRC(f896c93b) SHA1(0bee89fe4d36a9b2ded864770198eb2df6903580) )
	ROM_LOAD16_BYTE( "065-ubb05.10f",  0x00001, 0x20000, CRC(e02e5d64) SHA1(9838c1cf9862db3ca70a23ef5f3c5883729c4e0c) )
	ROM_LOAD16_BYTE( "065-a02.9d",     0x80000, 0x40000, CRC(b31dc44c) SHA1(4bdac05826b4d6d4fe46686ede5190e2f73eefc5) )
	ROM_LOAD16_BYTE( "065-a03.9f",     0x80001, 0x40000, CRC(13842fe6) SHA1(b61f094eb94336edb8708d3437ead9b853b2d6e6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* 64k+128k for sound cpu */
	ROM_LOAD( "065-a01.6f",   0x00000, 0x20000, CRC(147d3a4d) SHA1(a14409fe991e803b9e7812303e3a9ebd857d8b01) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a08.15l",   0x000000, 0x100000, CRC(6b649aca) SHA1(2595f314517738e8614facf578cc951a6c36a180) )	/* tiles */
	ROM_LOAD( "065-a07.16l",   0x100000, 0x100000, CRC(c5dc8fc4) SHA1(9887cb002c8b72be7ce933cb397f00cdc5506c8c) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a09.2h",  0x000000, 0x100000, CRC(ea05d52f) SHA1(7f2c14f907355856fb94e3a67b73aa1919776835) )	/* sprites */
	ROM_LOAD( "065-a10.2l",  0x100000, 0x100000, CRC(96b91802) SHA1(641943557b59b91f0edd49ec8a73cef7d9268b32) )
	ROM_LOAD( "065-a12.1h",  0x200000, 0x100000, CRC(321ed07a) SHA1(5b00ed676daeea974bdce6701667cfe573099dad) )
	ROM_LOAD( "065-a11.1l",  0x300000, 0x100000, CRC(46da948e) SHA1(168ac9178ee5bad5931557fb549e1237971d7839) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "065-a06.1f",  0x000000, 0x200000, CRC(5adbcee0) SHA1(435feda697193bc51db80eba46be474cbbc1de4b) )
ROM_END

ROM_START( xmen2p )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "065-aaa04.10d",  0x00000, 0x20000, CRC(7f8b27c2) SHA1(052db1f47671564a440544a41fc397a19d1aff3a) )
	ROM_LOAD16_BYTE( "065-aaa04.10f",  0x00001, 0x20000, CRC(841ed636) SHA1(33f96022ce3dae9b49eb51fd4e8f7387a1777002) )
	ROM_LOAD16_BYTE( "065-a02.9d",     0x80000, 0x40000, CRC(b31dc44c) SHA1(4bdac05826b4d6d4fe46686ede5190e2f73eefc5) )
	ROM_LOAD16_BYTE( "065-a03.9f",     0x80001, 0x40000, CRC(13842fe6) SHA1(b61f094eb94336edb8708d3437ead9b853b2d6e6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* 64k+128k for sound cpu */
	ROM_LOAD( "065-a01.6f",   0x00000, 0x20000, CRC(147d3a4d) SHA1(a14409fe991e803b9e7812303e3a9ebd857d8b01) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a08.15l",   0x000000, 0x100000, CRC(6b649aca) SHA1(2595f314517738e8614facf578cc951a6c36a180) )	/* tiles */
	ROM_LOAD( "065-a07.16l",   0x100000, 0x100000, CRC(c5dc8fc4) SHA1(9887cb002c8b72be7ce933cb397f00cdc5506c8c) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a09.2h",  0x000000, 0x100000, CRC(ea05d52f) SHA1(7f2c14f907355856fb94e3a67b73aa1919776835) )	/* sprites */
	ROM_LOAD( "065-a10.2l",  0x100000, 0x100000, CRC(96b91802) SHA1(641943557b59b91f0edd49ec8a73cef7d9268b32) )
	ROM_LOAD( "065-a12.1h",  0x200000, 0x100000, CRC(321ed07a) SHA1(5b00ed676daeea974bdce6701667cfe573099dad) )
	ROM_LOAD( "065-a11.1l",  0x300000, 0x100000, CRC(46da948e) SHA1(168ac9178ee5bad5931557fb549e1237971d7839) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "065-a06.1f",  0x000000, 0x200000, CRC(5adbcee0) SHA1(435feda697193bc51db80eba46be474cbbc1de4b) )
ROM_END

ROM_START( xmen2pj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "065-jaa04.10d",  0x00000, 0x20000, CRC(66746339) SHA1(8cc5f5deb4178b0444ffc5974940a30cb003114e) )
	ROM_LOAD16_BYTE( "065-jaa05.10f",  0x00001, 0x20000, CRC(1215b706) SHA1(b746dedab9c509b5cd941f0f4ddd3709e8a58cce) )
	ROM_LOAD16_BYTE( "065-a02.9d",     0x80000, 0x40000, CRC(b31dc44c) SHA1(4bdac05826b4d6d4fe46686ede5190e2f73eefc5) )
	ROM_LOAD16_BYTE( "065-a03.9f",     0x80001, 0x40000, CRC(13842fe6) SHA1(b61f094eb94336edb8708d3437ead9b853b2d6e6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* 64k+128k for sound cpu */
	ROM_LOAD( "065-a01.6f",   0x00000, 0x20000, CRC(147d3a4d) SHA1(a14409fe991e803b9e7812303e3a9ebd857d8b01) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a08.15l",   0x000000, 0x100000, CRC(6b649aca) SHA1(2595f314517738e8614facf578cc951a6c36a180) )	/* tiles */
	ROM_LOAD( "065-a07.16l",   0x100000, 0x100000, CRC(c5dc8fc4) SHA1(9887cb002c8b72be7ce933cb397f00cdc5506c8c) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a09.2h",  0x000000, 0x100000, CRC(ea05d52f) SHA1(7f2c14f907355856fb94e3a67b73aa1919776835) )	/* sprites */
	ROM_LOAD( "065-a10.2l",  0x100000, 0x100000, CRC(96b91802) SHA1(641943557b59b91f0edd49ec8a73cef7d9268b32) )
	ROM_LOAD( "065-a12.1h",  0x200000, 0x100000, CRC(321ed07a) SHA1(5b00ed676daeea974bdce6701667cfe573099dad) )
	ROM_LOAD( "065-a11.1l",  0x300000, 0x100000, CRC(46da948e) SHA1(168ac9178ee5bad5931557fb549e1237971d7839) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "065-a06.1f",  0x000000, 0x200000, CRC(5adbcee0) SHA1(435feda697193bc51db80eba46be474cbbc1de4b) )
ROM_END

/*

 KONAMI - X-MEN 2P/4P/6P - GX065 - PWB352532B

    +--------------------------------------------------------------------------------------+
    |                                                                                      |
    |                      [  065A06.1d  ]                 [  065A07.1h  ][  065A08.1l  ]  |
    |                                                                                      |
    |       [  054544  ]                                                                   |
    |                      [  054539  ]     [  053251  ]   [  051962  ] [  052109  ]       |
    |                                                                                      |
    |         [ YM2151 ]                                                                   |
    | sound                     [qz 24Mhz]                                                 |
    |  out                                                                                 |
    +-+  [  065*01.7b  ]                                                                   |
      |                                                                                    |
    +-+                                                                                    |
    |      [    Z80    ]                                                                   |
    |                                                                                      |
    +-+                                                                                    |
    +-+                                                                                    |
    |   J                                                                                  |
    |   A                                                                           [0     |
    |   M                                                                            6     |
    |   M                                                                            5     |
    |   A                                                                            A     |
    |                                                                                0     |
    |                                      [  053253  ]   [  053247  ] [  053246  ]  9.12l]|
    |                                                                                      |
    |                                                                               [0     |
    |                                                                                6     |
    +-+                                                                              5     |
      |                                   [  065A02.17g ]  [  065A03.17j ]           A     |
    +-+                                                                              1     |
    | test                                [  065*04.18g ]  [  065*05.18j ]           0.17l]|
    |  sw                                                                                  |
    | [cn9 (6P)]                                 [     68000 - 16Mhz     ]                 |
    | [cn8 (5P)]                                                                           |
    | [cn7 (4P)]                      [qz 32Mhz/18.432Mhz] [  065A12.22h ][  065A11.22l ]  |
    | [cn6 (3P)]                                                                           |
    |    rgb out                                                                           |
    +--------------------------------------------------------------------------------------+

    054544 *
    054539 *
    053251 *
    051962 *
    052109 *
    053253 - not on other version?
    053247 *
    053246 *

*/

ROM_START( xmen6p )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "065-ecb04.18g",  0x00000, 0x20000, CRC(258eb21f) SHA1(f1a22a880245f28195e5b6519822c0aa3b166541) )
	ROM_LOAD16_BYTE( "065-ecb05.18j",  0x00001, 0x20000, CRC(25997bcd) SHA1(86fb1c64e133b7ca59ffb3910b62b61ee372c71a) )
	ROM_LOAD16_BYTE( "065-a02.17g",   0x80000, 0x40000, CRC(b31dc44c) SHA1(4bdac05826b4d6d4fe46686ede5190e2f73eefc5) )
	ROM_LOAD16_BYTE( "065-a03.17j",   0x80001, 0x40000, CRC(13842fe6) SHA1(b61f094eb94336edb8708d3437ead9b853b2d6e6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* 64k+128k for sound cpu */
	ROM_LOAD( "065-a01.7b",   0x00000, 0x20000, CRC(147d3a4d) SHA1(a14409fe991e803b9e7812303e3a9ebd857d8b01) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a08.1l",   0x000000, 0x100000, CRC(6b649aca) SHA1(2595f314517738e8614facf578cc951a6c36a180) )	/* tiles */
	ROM_LOAD( "065-a07.1h",   0x100000, 0x100000, CRC(c5dc8fc4) SHA1(9887cb002c8b72be7ce933cb397f00cdc5506c8c) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a09.12l",  0x000000, 0x100000, CRC(ea05d52f) SHA1(7f2c14f907355856fb94e3a67b73aa1919776835) )	/* sprites */
	ROM_LOAD( "065-a10.17l",  0x100000, 0x100000, CRC(96b91802) SHA1(641943557b59b91f0edd49ec8a73cef7d9268b32) )
	ROM_LOAD( "065-a12.22h",  0x200000, 0x100000, CRC(321ed07a) SHA1(5b00ed676daeea974bdce6701667cfe573099dad) )
	ROM_LOAD( "065-a11.22l",  0x300000, 0x100000, CRC(46da948e) SHA1(168ac9178ee5bad5931557fb549e1237971d7839) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "065-a06.1d",  0x000000, 0x200000, CRC(5adbcee0) SHA1(435feda697193bc51db80eba46be474cbbc1de4b) )
ROM_END


ROM_START( xmen6pu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "065-ucb04.18g",  0x00000, 0x20000, CRC(0f09b8e0) SHA1(79f4d86d8ec45b39e34ddf45860bea0c74dae183) )
	ROM_LOAD16_BYTE( "065-ucb05.18j",  0x00001, 0x20000, CRC(867becbf) SHA1(3f81f4dbd289f98b78d7821a8925598c771f01ef) )
	ROM_LOAD16_BYTE( "065-a02.17g",   0x80000, 0x40000, CRC(b31dc44c) SHA1(4bdac05826b4d6d4fe46686ede5190e2f73eefc5) )
	ROM_LOAD16_BYTE( "065-a03.17j",   0x80001, 0x40000, CRC(13842fe6) SHA1(b61f094eb94336edb8708d3437ead9b853b2d6e6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* 64k+128k for sound cpu */
	ROM_LOAD( "065-a01.7b",   0x00000, 0x20000, CRC(147d3a4d) SHA1(a14409fe991e803b9e7812303e3a9ebd857d8b01) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a08.1l",   0x000000, 0x100000, CRC(6b649aca) SHA1(2595f314517738e8614facf578cc951a6c36a180) )	/* tiles */
	ROM_LOAD( "065-a07.1h",   0x100000, 0x100000, CRC(c5dc8fc4) SHA1(9887cb002c8b72be7ce933cb397f00cdc5506c8c) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "065-a09.12l",  0x000000, 0x100000, CRC(ea05d52f) SHA1(7f2c14f907355856fb94e3a67b73aa1919776835) )	/* sprites */
	ROM_LOAD( "065-a10.17l",  0x100000, 0x100000, CRC(96b91802) SHA1(641943557b59b91f0edd49ec8a73cef7d9268b32) )
	ROM_LOAD( "065-a12.22h",  0x200000, 0x100000, CRC(321ed07a) SHA1(5b00ed676daeea974bdce6701667cfe573099dad) )
	ROM_LOAD( "065-a11.22l",  0x300000, 0x100000, CRC(46da948e) SHA1(168ac9178ee5bad5931557fb549e1237971d7839) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "065-a06.1d",  0x000000, 0x200000, CRC(5adbcee0) SHA1(435feda697193bc51db80eba46be474cbbc1de4b) )
ROM_END




static DRIVER_INIT( xmen )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}

GAME( 1992, xmen,    0,    xmen, xmen,   xmen,   ROT0, "Konami", "X-Men (4 Players ver UBB)", 0 )
GAME( 1992, xmen2p,  xmen, xmen, xmen2p, xmen,   ROT0, "Konami", "X-Men (2 Players ver AAA)", 0 )
GAME( 1992, xmen2pj, xmen, xmen, xmen2p, xmen,   ROT0, "Konami", "X-Men (2 Players ver JAA)", 0 )

GAME( 1992, xmen6p,  xmen, xmen6p,xmen6p,   xmen, ROT0, "Konami", "X-Men (6 Players ver ECB)", GAME_IMPERFECT_GRAPHICS )
GAME( 1992, xmen6pu, xmen, xmen6p,xmen6p,   xmen, ROT0, "Konami", "X-Men (6 Players ver UCB)", GAME_IMPERFECT_GRAPHICS )
