/***************************************************************************

    Sega Hang On hardware

****************************************************************************

    Known bugs:
        * none at this time

    To do for each game:
        * verify memory test
        * verify inputs
        * verify DIP switches
        * verify protection
        * check playability

***************************************************************************/

#include "driver.h"
#include "system16.h"
#include "machine/segaic16.h"
#include "machine/8255ppi.h"
#include "cpu/m68000/m68000.h"
#include "sound/2203intf.h"
#include "sound/2151intf.h"
#include "sound/segapcm.h"


/*************************************
 *
 *  Statics
 *
 *************************************/

static UINT16 *workram;

static UINT8 adc_select;
static UINT8 sound_control;
static UINT8 ppi_write_offset;

static void (*i8751_vblank_hook)(void);



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static WRITE8_HANDLER( sound_command_w );
static WRITE8_HANDLER( video_lamps_w );
static WRITE8_HANDLER( tilemap_sound_w );
static READ8_HANDLER( tilemap_sound_r );
static WRITE8_HANDLER( sub_control_adc_w );

static READ8_HANDLER( adc_status_r );



/*************************************
 *
 *  PPI interfaces
 *
 *************************************/

static ppi8255_interface hangon_ppi_intf =
{
	2,
	{ NULL, NULL },
	{ NULL, NULL },
	{ tilemap_sound_r, adc_status_r },
	{ sound_command_w, sub_control_adc_w },
	{ video_lamps_w,   NULL },
	{ tilemap_sound_w, NULL }
};



/*************************************
 *
 *  Configuration
 *
 *************************************/

static void hangon_generic_init(void)
{
	/* configure the 8255 interface */
	ppi8255_init(&hangon_ppi_intf);

	/* reset the custom handlers and other pointers */
	i8751_vblank_hook = NULL;
}



/*************************************
 *
 *  Initialization & interrupts
 *
 *************************************/

static MACHINE_RESET( hangon )
{
	/* reset misc components */
	segaic16_tilemap_reset(0);

	/* if we have a fake i8751 handler, disable the actual 8751 */
	if (i8751_vblank_hook != NULL)
		cpunum_suspend(mame_find_cpu_index("mcu"), SUSPEND_REASON_DISABLE, 1);

	/* reset global state */
	adc_select = 0;
	sound_control = 0;
	ppi_write_offset = 0;
}

#if 0
static INTERRUPT_GEN( hangon_irq )
{
	/* according to the schematics, IRQ2 is generated every 16 scanlines */
	if (cpu_getiloops() != 0)
		cpunum_set_input_line(0, 2, HOLD_LINE);
	else
		cpunum_set_input_line(0, 4, HOLD_LINE);
}
#endif


/*************************************
 *
 *  I/O space
 *
 *************************************/

static READ16_HANDLER( hangon_io_r )
{
	switch (offset & 0x3020/2)
	{
		case 0x0000/2: /* PPI @ 4B */
			return ppi8255_0_r(offset & 3);

		case 0x1000/2: /* Input ports and DIP switches */
			return readinputport(offset & 3);

		case 0x3000/2: /* PPI @ 4C */
			return ppi8255_1_r(offset & 3);

		case 0x3020/2: /* ADC0804 data output */
			return readinputport(4 + adc_select);
	}

	logerror("%06X:hangon_io_r - unknown read access to address %04X\n", activecpu_get_pc(), offset * 2);
	return segaic16_open_bus_r(0,0);
}


static WRITE16_HANDLER( hangon_io_w )
{
	if (ACCESSING_LSB)
		switch (offset & 0x3020/2)
		{
			case 0x0000/2: /* PPI @ 4B */
				ppi8255_0_w(ppi_write_offset = offset & 3, data & 0xff);
				return;

			case 0x3000/2: /* PPI @ 4C */
				ppi8255_1_w(ppi_write_offset = offset & 3, data & 0xff);
				return;

			case 0x3020/2: /* ADC0804 */
				return;
		}

	logerror("%06X:hangon_io_w - unknown write access to address %04X = %04X & %04X\n", activecpu_get_pc(), offset * 2, data, mem_mask ^ 0xffff);
}


static READ16_HANDLER( sharrier_io_r )
{
	switch (offset & 0x0030/2)
	{
		case 0x0000/2:
			return ppi8255_0_r(offset & 3);

		case 0x0010/2: /* Input ports and DIP switches */
			return readinputport(offset & 3);

		case 0x0020/2: /* PPI @ 4C */
			if (offset == 2) return 0;
			return ppi8255_1_r(offset & 3);

		case 0x0030/2: /* ADC0804 data output */
			return readinputport(4 + adc_select);
	}

	logerror("%06X:sharrier_io_r - unknown read access to address %04X\n", activecpu_get_pc(), offset * 2);
	return segaic16_open_bus_r(0,0);
}


static WRITE16_HANDLER( sharrier_io_w )
{
	if (ACCESSING_LSB)
		switch (offset & 0x0030/2)
		{
			case 0x0000/2:
				ppi8255_0_w(ppi_write_offset = offset & 3, data & 0xff);
				return;

			case 0x0020/2: /* PPI @ 4C */
				ppi8255_1_w(ppi_write_offset = offset & 3, data & 0xff);
				return;

			case 0x0030/2: /* ADC0804 */
				return;
		}

	logerror("%06X:sharrier_io_w - unknown write access to address %04X = %04X & %04X\n", activecpu_get_pc(), offset * 2, data, mem_mask ^ 0xffff);
}



/*************************************
 *
 *  PPI I/O handlers
 *
 *************************************/

static WRITE8_HANDLER( sound_command_w )
{
	/* Port A : Z80 sound command */
	/* gross hack: because our 8255 implementation writes here on a control register */
	/* change, we have to be careful that we don't generate an NMI in that case. */
	/* The sound handing in Hang On is particularly fragile in this way, and won't */
	/* work with the extraneous writes */
	if (ppi_write_offset != 3)
	{
		soundlatch_w(0, data & 0xff);
		cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
		sound_control |= 0x80;
		cpu_boost_interleave(0, TIME_IN_USEC(50));
	}
}


static WRITE8_HANDLER( video_lamps_w )
{
	/* Port B : Miscellaneous outputs */
	/* D7 : FLIPC (1= flip screen, 0= normal orientation) */
	/* D6 : SHADE0 (1= highlight, 0= shadow) */
	/* D4 : /KILL (1= screen on, 0= screen off) */
	/* D3 : LAMP2 */
	/* D2 : LAMP1 */
	/* D1 : COIN2 */
	/* D0 : COIN1 */
	segaic16_tilemap_set_flip(0, data & 0x80);
	segaic16_sprites_set_flip(0, data & 0x80);
	segaic16_sprites_set_shadow(0, ~data & 0x40);
	segaic16_set_display_enable(data & 0x10);
	set_led_status(1, data & 0x08);
	set_led_status(0, data & 0x04);
	coin_counter_w(1, data & 0x02);
	coin_counter_w(0, data & 0x01);
}


static WRITE8_HANDLER( tilemap_sound_w )
{
	/* Port C : Tilemap origin and audio mute */
	/* D3 : INTR */
	/* D2 : SCONT1 - Tilemap origin bit 1 */
	/* D1 : SCONT0 - Tilemap origin bit 0 */
	/* D0 : MUTE (1= audio on, 0= audio off) */
	segaic16_tilemap_set_colscroll(0, ~data & 0x04);
	segaic16_tilemap_set_rowscroll(0, ~data & 0x02);
}


static READ8_HANDLER( tilemap_sound_r )
{
	/* Port C : Tilemap origin and audio mute */
	/* D7 : OBF - sound output buffer full */
	/* D6 : /ACK - ack from sound chip */
	/* D5 : IBF - sound input buffer full */
	/* D4 : /STB - ??? */
	return sound_control;
}


static WRITE8_HANDLER( sub_control_adc_w )
{
	/* Port A : S.CPU control and ADC channel select */
	/* D6 : INTR line on second CPU */
	/* D5 : RESET line on second CPU */
	/* D3-D2 : ADC_SELECT */
	cpunum_set_input_line(1, 4, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
	adc_select = (data >> 2) & 3;
}


static READ8_HANDLER( adc_status_r )
{
	/* D7 = 0 (left open) */
	/* D6 = /INTR of ADC0804 */
	/* D5 = 0 (left open) */
	/* D4 = 0 (left open) */
	return 0x00;
}



/*************************************
 *
 *  I8751 interrupt generation
 *
 *************************************/

static INTERRUPT_GEN( i8751_main_cpu_vblank )
{
	/* if we have a fake 8751 handler, call it on VBLANK */
	if (i8751_vblank_hook != NULL)
		(*i8751_vblank_hook)();
	irq4_line_hold();
}



/*************************************
 *
 *  Per-game I8751 workarounds
 *
 *************************************/

static void sharrier_i8751_sim(void)
{
	workram[0x492/2] = (readinputport(4) << 8) | readinputport(5);
}



/*************************************
 *
 *  Sound communications?
 *
 *************************************/

static void sound_irq(int irq)
{
	cpunum_set_input_line(2, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}


static READ8_HANDLER( sound_data_r )
{
	UINT8 result = soundlatch_r(offset);
	sound_control &= ~0x80;
	return result;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( hangon_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x20c000, 0x20ffff) AM_RAM
	AM_RANGE(0x400000, 0x403fff) AM_READWRITE(MRA16_RAM, segaic16_tileram_0_w) AM_BASE(&segaic16_tileram_0)
	AM_RANGE(0x410000, 0x410fff) AM_READWRITE(MRA16_RAM, segaic16_textram_0_w) AM_BASE(&segaic16_textram_0)
	AM_RANGE(0x600000, 0x6007ff) AM_RAM AM_BASE(&segaic16_spriteram_0)
	AM_RANGE(0xa00000, 0xa00fff) AM_READWRITE(MRA16_RAM, segaic16_paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0xc00000, 0xc3ffff) AM_ROM AM_REGION(REGION_CPU2, 0)
	AM_RANGE(0xc68000, 0xc68fff) AM_RAM AM_SHARE(1) AM_BASE(&segaic16_roadram_0)
	AM_RANGE(0xc7c000, 0xc7ffff) AM_RAM AM_SHARE(2)
	AM_RANGE(0xe00000, 0xffffff) AM_READWRITE(hangon_io_r, hangon_io_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sharrier_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x040000, 0x043fff) AM_RAM AM_BASE(&workram)
	AM_RANGE(0x100000, 0x107fff) AM_READWRITE(MRA16_RAM, segaic16_tileram_0_w) AM_BASE(&segaic16_tileram_0)
	AM_RANGE(0x108000, 0x108fff) AM_READWRITE(MRA16_RAM, segaic16_textram_0_w) AM_BASE(&segaic16_textram_0)
	AM_RANGE(0x110000, 0x110fff) AM_READWRITE(MRA16_RAM, segaic16_paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x124000, 0x127fff) AM_RAM AM_SHARE(2)
	AM_RANGE(0x130000, 0x130fff) AM_RAM AM_BASE(&segaic16_spriteram_0)
	AM_RANGE(0x140000, 0x14ffff) AM_READWRITE(sharrier_io_r, sharrier_io_w)
	AM_RANGE(0xc68000, 0xc68fff) AM_RAM AM_SHARE(1) AM_BASE(&segaic16_roadram_0)
ADDRESS_MAP_END



/*************************************
 *
 *  Second CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sub_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) | AMEF_ABITS(19) )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x068000, 0x068fff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x07c000, 0x07ffff) AM_RAM AM_SHARE(2)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_map_2203, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_MIRROR(0x0800) AM_RAM
	AM_RANGE(0xd000, 0xd000) AM_MIRROR(0x0ffe) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0xd001, 0xd001) AM_MIRROR(0x0ffe) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0xe000, 0xe0ff) AM_MIRROR(0x0f00) AM_READWRITE(SegaPCM_r, SegaPCM_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_portmap_2203, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) | AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_READ(sound_data_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_map_2151, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xf000, 0xf0ff) AM_MIRROR(0x700) AM_READWRITE(SegaPCM_r, SegaPCM_w)
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_portmap_2151, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) | AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x3e) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x3e) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_READ(sound_data_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_portmap_2203x2, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) | AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x3e) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x3e) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_READ(sound_data_r)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x3e) AM_READWRITE(YM2203_status_port_1_r, YM2203_control_port_1_w)
	AM_RANGE(0xc1, 0xc1) AM_MIRROR(0x3e) AM_WRITE(YM2203_write_port_1_w)
ADDRESS_MAP_END



/*************************************
 *
 *  i8751 MCU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_data_map, ADDRESS_SPACE_DATA, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
ADDRESS_MAP_END



/*************************************
 *
 *  Generic port definitions
 *
 *************************************/

static INPUT_PORTS_START( sharrier_generic )
	PORT_START_TAG("SERVICE")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("UNKNOWN")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COINAGE")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1" )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Game-specific port definitions
 *
 *************************************/

static INPUT_PORTS_START( hangon )
	PORT_INCLUDE( sharrier_generic )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1" )

	PORT_MODIFY("COINAGE")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, "Play Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("ADC0")	/* steering */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START_TAG("ADC1")	/* gas pedal */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20)

	PORT_START_TAG("ADC2")	/* brake */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(40)
INPUT_PORTS_END


static INPUT_PORTS_START( shangupb )
	PORT_INCLUDE( hangon )

	PORT_MODIFY("SERVICE")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_MODIFY("COINAGE")
	PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static INPUT_PORTS_START( sharrier )
	PORT_INCLUDE( sharrier_generic )

	PORT_MODIFY("SERVICE")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, "Moving" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, "Add Player Score" )
	PORT_DIPSETTING(    0x10, "5000000" )
	PORT_DIPSETTING(    0x00, "7000000" )
	PORT_DIPNAME( 0x20, 0x20, "Trial Time" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START_TAG("ADC0")	/* X axis */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0x20,0xdf) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START_TAG("ADC1")	/* Y axis */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_Y ) PORT_MINMAX(0x60,0x9f) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE
INPUT_PORTS_END


static INPUT_PORTS_START( enduror )
	PORT_INCLUDE( sharrier_generic )

	PORT_MODIFY("SERVICE")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, "Moving" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, "Time Adjust" )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x60, 0x60, "Time Control" )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("ADC0")	/* gas pedal */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20)

	PORT_START_TAG("ADC1")	/* brake */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(40)

	PORT_START_TAG("ADC2")	/* bank up/down */
	PORT_BIT( 0xff, 0x20, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)

	PORT_START_TAG("ADC3")	/* steering */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,sound_irq
};


static struct YM2151interface ym2151_interface =
{
	sound_irq
};


static struct SEGAPCMinterface segapcm_interface =
{
	BANK_512,
	REGION_SOUND1
};



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	0, 1024 },
	{ -1 }
};



/*************************************
 *
 *  Generic machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( hangon_base )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 6000000)
	MDRV_CPU_PROGRAM_MAP(hangon_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD_TAG("sub", M68000, 6000000)
	MDRV_CPU_PROGRAM_MAP(sub_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1000000 * (262 - 224) / (262 * 60))

	MDRV_MACHINE_RESET(hangon)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(1*8, 39*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*3)

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sharrier_base )
	MDRV_IMPORT_FROM(hangon_base)

	/* basic machine hardware */
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(sharrier_map,0)
	MDRV_CPU_VBLANK_INT(i8751_main_cpu_vblank,1)

	MDRV_CPU_REPLACE("sub", M68000, 10000000)

	/* video hardware */
	MDRV_VIDEO_START(sharrier)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sound_board_2203 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map_2203,0)
	MDRV_CPU_IO_MAP(sound_portmap_2203,0)

	/* soud hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.13)
	MDRV_SOUND_ROUTE(0, "right", 0.13)
	MDRV_SOUND_ROUTE(1, "left",  0.13)
	MDRV_SOUND_ROUTE(1, "right", 0.13)
	MDRV_SOUND_ROUTE(2, "left",  0.13)
	MDRV_SOUND_ROUTE(2, "right", 0.13)
	MDRV_SOUND_ROUTE(3, "left",  0.37)
	MDRV_SOUND_ROUTE(3, "right", 0.37)

	MDRV_SOUND_ADD_TAG("pcm", SEGAPCM, 8000000)
	MDRV_SOUND_CONFIG(segapcm_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sound_board_2203x2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map_2151,0)
	MDRV_CPU_IO_MAP(sound_portmap_2203x2,0)

	/* soud hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.13)
	MDRV_SOUND_ROUTE(0, "right", 0.13)
	MDRV_SOUND_ROUTE(1, "left",  0.13)
	MDRV_SOUND_ROUTE(1, "right", 0.13)
	MDRV_SOUND_ROUTE(2, "left",  0.13)
	MDRV_SOUND_ROUTE(2, "right", 0.13)
	MDRV_SOUND_ROUTE(3, "left",  0.37)
	MDRV_SOUND_ROUTE(3, "right", 0.37)

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_ROUTE(0, "left",  0.13)
	MDRV_SOUND_ROUTE(0, "right", 0.13)
	MDRV_SOUND_ROUTE(1, "left",  0.13)
	MDRV_SOUND_ROUTE(1, "right", 0.13)
	MDRV_SOUND_ROUTE(2, "left",  0.13)
	MDRV_SOUND_ROUTE(2, "right", 0.13)
	MDRV_SOUND_ROUTE(3, "left",  0.37)
	MDRV_SOUND_ROUTE(3, "right", 0.37)

	MDRV_SOUND_ADD_TAG("pcm", SEGAPCM, 4000000)
	MDRV_SOUND_CONFIG(segapcm_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sound_board_2151 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map_2151,0)
	MDRV_CPU_IO_MAP(sound_portmap_2151,0)

	/* soud hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.43)
	MDRV_SOUND_ROUTE(1, "right", 0.43)

	MDRV_SOUND_ADD_TAG("pcm", SEGAPCM, 4000000)
	MDRV_SOUND_CONFIG(segapcm_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  Specific machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( hangon )
	MDRV_IMPORT_FROM(hangon_base)
	MDRV_IMPORT_FROM(sound_board_2203)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( shangupb )
	MDRV_IMPORT_FROM(hangon_base)
	MDRV_IMPORT_FROM(sound_board_2151)

	/* not sure about these speeds, but at 6MHz, the road is not updated fast enough */
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_REPLACE("sub", M68000, 10000000)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sharrier )
	MDRV_IMPORT_FROM(sharrier_base)
	MDRV_IMPORT_FROM(sound_board_2203)

	MDRV_CPU_ADD_TAG("mcu", I8751, 8000000)
	MDRV_CPU_PROGRAM_MAP(mcu_map,0)
	MDRV_CPU_DATA_MAP(mcu_data_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( enduror )
	MDRV_IMPORT_FROM(sharrier_base)
	MDRV_IMPORT_FROM(sound_board_2151)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( enduror1 )
	MDRV_IMPORT_FROM(sharrier_base)
	MDRV_IMPORT_FROM(sound_board_2203)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( endurob2 )
	MDRV_IMPORT_FROM(sharrier_base)
	MDRV_IMPORT_FROM(sound_board_2203x2)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
    Hang On
    CPU: 68000 (317-????)
*/
ROM_START( hangon )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "6918.rom", 0x000000, 0x8000, CRC(20b1c2b0) SHA1(01b4f5105e2bbeb6ec6dbd18bfb728e3a973e0ca) )
	ROM_LOAD16_BYTE( "6916.rom", 0x000001, 0x8000, CRC(7d9db1bf) SHA1(952ee3e7a0d57ec1bb3385e0e6675890b8378d31) )
	ROM_LOAD16_BYTE( "6917.rom", 0x010000, 0x8000, CRC(fea12367) SHA1(9a1ce5863c562160b657ad948812b43f42d7d0cc) )
	ROM_LOAD16_BYTE( "6915.rom", 0x010001, 0x8000, CRC(ac883240) SHA1(f943341ae13e062f3d12c6221180086ce8bdb8c4) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "6920.rom", 0x0000, 0x8000, CRC(1c95013e) SHA1(8344ac953477279c2c701f984d98292a21dd2f7d) )
	ROM_LOAD16_BYTE( "6919.rom", 0x0001, 0x8000, CRC(6ca30d69) SHA1(ed933351883ebf6d9ef9428a81d09749b609cd60) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "6841.rom", 0x00000, 0x08000, CRC(54d295dc) SHA1(ad8cdb281032a2f931c2abbeb966998944683dc3) )
	ROM_LOAD( "6842.rom", 0x08000, 0x08000, CRC(f677b568) SHA1(636ca60bd4be9b5c2be09de8ae49db1063aa6c79) )
	ROM_LOAD( "6843.rom", 0x10000, 0x08000, CRC(a257f0da) SHA1(9828f8ce4ef245ffb8dbad347f9ca74ed81aa998) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "6819.rom", 0x000001, 0x8000, CRC(469dad07) SHA1(6d01c0b3506e28832928ad74d518577ff5be323b) )
	ROM_LOAD16_BYTE( "6820.rom", 0x000000, 0x8000, CRC(87cbc6de) SHA1(b64652e062e1b88c6f6ae8dd2ffe4533bb27ba45) )
	ROM_LOAD16_BYTE( "6821.rom", 0x010001, 0x8000, CRC(15792969) SHA1(b061dbf24e8b511116446794753c8b0cc49e2149) )
	ROM_LOAD16_BYTE( "6822.rom", 0x010000, 0x8000, CRC(e9718de5) SHA1(30e3a7d5b33504da03c5780b4a946b977e46098a) )
	ROM_LOAD16_BYTE( "6823.rom", 0x020001, 0x8000, CRC(49422691) SHA1(caee2a4a3f4587ae27dec330214edaa1229012af) )
	ROM_LOAD16_BYTE( "6824.rom", 0x020000, 0x8000, CRC(701deaa4) SHA1(053032ef886b85a4cb4753d17b3c27d228695157) )
	ROM_LOAD16_BYTE( "6825.rom", 0x030001, 0x8000, CRC(6e23c8b4) SHA1(b17fd7d590ed4e6616b7b4d91a47a2820248d8c7) )
	ROM_LOAD16_BYTE( "6826.rom", 0x030000, 0x8000, CRC(77d0de2c) SHA1(83b126ed1d463504b2702391816e6e20dcd04ffc) )
	ROM_LOAD16_BYTE( "6827.rom", 0x040001, 0x8000, CRC(7fa1bfb6) SHA1(a27b54c93613372f59050f0b2182d2984a8d2efe) )
	ROM_LOAD16_BYTE( "6828.rom", 0x040000, 0x8000, CRC(8e880c93) SHA1(8c55deec065daf09a5d1c1c1f3f3f7bc1aeaf563) )
	ROM_LOAD16_BYTE( "6829.rom", 0x050001, 0x8000, CRC(7ca0952d) SHA1(617d73591158ed3fea5174f7dabf0413d28de9b3) )
	ROM_LOAD16_BYTE( "6830.rom", 0x050000, 0x8000, CRC(b1a63aef) SHA1(5db0a1cc2d13c6cfc77044f5d7f6f99d198531ed) )
	ROM_LOAD16_BYTE( "6845.rom", 0x060001, 0x8000, CRC(ba08c9b8) SHA1(65ceaefa18999c468b38576c29101674d1f63e5f) )
	ROM_LOAD16_BYTE( "6846.rom", 0x060000, 0x8000, CRC(f21e57a3) SHA1(92ce0723e722f446c0cef9e23080a008aa9752e7) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "6840.rom", 0x0000, 0x8000, CRC(581230e3) SHA1(954eab35059322a12a197bba04bf85f816132f20) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "6833.rom", 0x00000, 0x4000, CRC(3b942f5f) SHA1(4384b5c090954e69de561dde0ef32104aa11399a) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "6831.rom", 0x00000, 0x8000, CRC(cfef5481) SHA1(c04b302fee58f0e59a097b2be2b61e5d03df7c91) )
	ROM_LOAD( "6832.rom", 0x08000, 0x8000, CRC(4165aea5) SHA1(be05c6d295807af2f396a1ff72d5a3d2a1e6054d) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
    Super Hang On (bootleg upgrade)
    CPU: 68000 (317-????)
*/
ROM_START( shangupb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "s-hangon.30", 0x000000, 0x10000, CRC(d95e82fc) SHA1(bc6cd0b0ac98a9c53f2e22ac086521704ab59e4d) )
	ROM_LOAD16_BYTE( "s-hangon.32", 0x000001, 0x10000, CRC(2ee4b4fb) SHA1(ba4042ab6e533c16c3cde848248d75e484be113f) )
	ROM_LOAD16_BYTE( "s-hangon.29", 0x020000, 0x08000, CRC(12ee8716) SHA1(8e798d23d22f85cd046641184d104c17b27995b2) )
	ROM_LOAD16_BYTE( "s-hangon.31", 0x020001, 0x08000, CRC(155e0cfd) SHA1(e51734351c887fe3920c881f57abdfbb7d075f57) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "s-hangon.09", 0x00000, 0x10000, CRC(070c8059) SHA1(a18c5e9473b6634f6e7165300e39029335b41ba3) )
	ROM_LOAD16_BYTE( "s-hangon.05", 0x00001, 0x10000, CRC(9916c54b) SHA1(41a7c5a9bdb1e3feae8fadf1ac5f51fab6376157) )
	ROM_LOAD16_BYTE( "s-hangon.08", 0x20000, 0x10000, CRC(000ad595) SHA1(eb80e798159c09bc5142a7ea8b9b0f895976b0d4) )
	ROM_LOAD16_BYTE( "s-hangon.04", 0x20001, 0x10000, CRC(8f8f4af0) SHA1(1dac21b7df6ec6874d36a07e30de7129b7f7f33a) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "epr10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "epr10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION16_BE( 0x00e0000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_LOAD16_BYTE( "epr10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_LOAD16_BYTE( "s-hangon.20", 0x020001, 0x010000, CRC(eef23b3d) SHA1(2416fa9991afbdddf25d469082e53858289550db) )
	ROM_LOAD16_BYTE( "s-hangon.14", 0x020000, 0x010000, CRC(0f26d131) SHA1(0d8b6eb8b8aae0aa8f0fa0c31dc91ad0e610be3e) )
	ROM_LOAD16_BYTE( "epr10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "epr10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "epr10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "epr10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "epr10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "epr10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "epr10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "epr10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "epr10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "epr10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "s-hangon.26", 0x0000, 0x8000, CRC(1bbe4fc8) SHA1(30f7f301e4d10d3b254d12bf3d32e5371661a566) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "s-hangon.03", 0x0000, 0x08000, CRC(83347dc0) SHA1(079bb750edd6372750a207764e8c84bb6abf2f79) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "s-hangon.02", 0x00000, 0x10000, CRC(da08ca2b) SHA1(2c94c127efd66f6cf86b25e2653637818a99aed1) )
	ROM_LOAD( "s-hangon.01", 0x10000, 0x10000, CRC(8b10e601) SHA1(75e9bcdd3f096be9bed672d61064b9240690deec) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, BAD_DUMP CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
    Space Harrier
    CPU: 68000 + i8751 (315-5163A)
*/
ROM_START( sharrier )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7188a.97",  0x000000, 0x8000, CRC(45e173c3) SHA1(cbab555c5053f3e4a3f75ff78c41528e2d9d34c7) )
	ROM_LOAD16_BYTE( "epr7184a.84",  0x000001, 0x8000, CRC(e1934a51) SHA1(67817a360b3f1f6c2440986272975bd696a38e70) )
	ROM_LOAD16_BYTE( "ic98.bin",     0x010000, 0x8000, CRC(40b1309f) SHA1(9b050983f043a88f414745d02c912b59bbf1b121) )
	ROM_LOAD16_BYTE( "ic85.bin",     0x010001, 0x8000, CRC(ce78045c) SHA1(ce640f05bed64ff5b47f1064b5fc13e58bc422a4) )
	ROM_LOAD16_BYTE( "ic99.bin",     0x020000, 0x8000, CRC(f6391091) SHA1(3160b342b6447cccf67c932c7c1a42354cdfb058) )
	ROM_LOAD16_BYTE( "ic86.bin",     0x020001, 0x8000, CRC(79b367d7) SHA1(e901036b1b9fac460415d513837c8f852f7750b0) )
	ROM_LOAD16_BYTE( "ic100.bin",    0x030000, 0x8000, CRC(6171e9d3) SHA1(72f8736f421dc93139859fd47f0c8c3c32b6ff0b) )
	ROM_LOAD16_BYTE( "ic87.bin",     0x030001, 0x8000, CRC(70cb72ef) SHA1(d1d89bd133b6905f81c25513d852b7e3a05a7312) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "ic54.bin", 0x0000, 0x8000, CRC(d7c535b6) SHA1(c0659a678c0c3776387a4a675016e9a2e9c67ee3) )
	ROM_LOAD16_BYTE( "ic67.bin", 0x0001, 0x8000, CRC(a6153af8) SHA1(b56ba472e4afb474c7a3f7dc11d7428ebbe1a9c7) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sic31.bin", 0x00000, 0x08000, CRC(347fa325) SHA1(9076b16de9598b52a75e5084651ee5a220b0e88b) )
	ROM_LOAD( "sic46.bin", 0x08000, 0x08000, CRC(39d98bd1) SHA1(5aab91bdd08b0f1ea537cd43ccc2e82fd01dd031) )
	ROM_LOAD( "sic60.bin", 0x10000, 0x08000, CRC(3da3ea6b) SHA1(9a6ce304a14e6ef0be41d867284a63b941f960fb) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "ic36.bin", 0x00000, 0x8000, CRC(93e2d264) SHA1(ca56de13756ab77408506d88f291da1da8134435) )
	ROM_LOAD32_BYTE( "ic28.bin", 0x00001, 0x8000, CRC(edbf5fc3) SHA1(a93f8c431075741c181eb422b24c9303487ca16c) )
	ROM_LOAD32_BYTE( "ic118.bin",0x00002, 0x8000, CRC(e8c537d8) SHA1(c9b3c0f33272c47d32e6aa349d72f7e355468e0e) )
	ROM_LOAD32_BYTE( "ic8.bin",  0x00003, 0x8000, CRC(22844fa4) SHA1(6d0f177082084c8c92085cd53e1d7ddc62d09a4c) )
	ROM_LOAD32_BYTE( "ic35.bin", 0x20000, 0x8000, CRC(cd6e7500) SHA1(6f0e42eb28ad3f5df93d8bb39dc41aba66eee144) )
	ROM_LOAD32_BYTE( "ic27.bin", 0x20001, 0x8000, CRC(41f25a9c) SHA1(bebbd4c4600028205aeff54190b32b664e4710d0) )
	ROM_LOAD32_BYTE( "ic17.bin", 0x20002, 0x8000, CRC(5bb09a67) SHA1(8bedefd2fa29a1a5e36f1d81c4e9067e3c7f28e9) )
	ROM_LOAD32_BYTE( "ic7.bin",  0x20003, 0x8000, CRC(dcaa2ebf) SHA1(9cf77bb966859febc5e5f1447cb719db64aa4db4) )
	ROM_LOAD32_BYTE( "ic34.bin", 0x40000, 0x8000, CRC(d5e15e66) SHA1(2bd81c5c736d725577adf85532de7802bef057f2) )
	ROM_LOAD32_BYTE( "ic26.bin", 0x40001, 0x8000, CRC(ac62ae2e) SHA1(d472dcc1d9b7889d04870920d5c6e5578597b8dc) )
	ROM_LOAD32_BYTE( "ic16.bin", 0x40002, 0x8000, CRC(9c782295) SHA1(c1627f3d849d2fdb02a590502bafbe133212b943) )
	ROM_LOAD32_BYTE( "ic6.bin",  0x40003, 0x8000, CRC(3711105c) SHA1(075197f614786f89bee28ed944371223bc75c9be) )
	ROM_LOAD32_BYTE( "ic33.bin", 0x60000, 0x8000, CRC(60d7c1bb) SHA1(19ddc1d8f269b50343266d508ad04d4c0fff69d1) )
	ROM_LOAD32_BYTE( "ic25.bin", 0x60001, 0x8000, CRC(f6330038) SHA1(805d4ed664972c0773c837d62f094858c1804148) )
	ROM_LOAD32_BYTE( "ic15.bin", 0x60002, 0x8000, CRC(60737b98) SHA1(5e91498bc473f099a1b2887baf980486e922af97) )
	ROM_LOAD32_BYTE( "ic5.bin",  0x60003, 0x8000, CRC(70fb5ebb) SHA1(38755a9be92865d2c5da8112d8d9c0fe8e778cff) )
	ROM_LOAD32_BYTE( "ic32.bin", 0x80000, 0x8000, CRC(6d7b5c97) SHA1(94c27e4ef1e197ee136f9399b07520cae00a366f) )
	ROM_LOAD32_BYTE( "ic24.bin", 0x80001, 0x8000, CRC(cebf797c) SHA1(d3d5aeba1a0e70a322ec86b930814fa8bc744829) )
	ROM_LOAD32_BYTE( "ic14.bin", 0x80002, 0x8000, CRC(24596a8b) SHA1(f580022c4f7dcaefb7209058c310a329479b5fcd) )
	ROM_LOAD32_BYTE( "ic4.bin",  0x80003, 0x8000, CRC(b537d082) SHA1(f87a644d9af8477c9eb94e5d3aeb5f6376264418) )
	ROM_LOAD32_BYTE( "ic31.bin", 0xa0000, 0x8000, CRC(5e784271) SHA1(063bd83b7f42dec556a7bdf736cb51456ba7184b) )
	ROM_LOAD32_BYTE( "ic23.bin", 0xa0001, 0x8000, CRC(510e5e10) SHA1(47b9f1bc8df0690c37d1d045bd361f8599e8a903) )
	ROM_LOAD32_BYTE( "ic13.bin", 0xa0002, 0x8000, CRC(7a2dad15) SHA1(227865447027f8669aa0d06d059f3d61da6d59dd) )
	ROM_LOAD32_BYTE( "ic3.bin",  0xa0003, 0x8000, CRC(f5ba4e08) SHA1(443b07c996cbb213fe57dfdd3879c1d4da27c001) )
	ROM_LOAD32_BYTE( "ic30.bin", 0xc0000, 0x8000, CRC(ec42c9ef) SHA1(313d908f3a964529b18e09825552879817a2e159) )
	ROM_LOAD32_BYTE( "ic22.bin", 0xc0001, 0x8000, CRC(6d4a7d7a) SHA1(997ac38e47d84f0166ca3ece50ac5f2d3435d8d3) )
	ROM_LOAD32_BYTE( "ic12.bin", 0xc0002, 0x8000, CRC(0f732717) SHA1(6a19c88d5d52f4ec4adb32c511fed4caae81c65f) )
	ROM_LOAD32_BYTE( "ic2.bin",  0xc0003, 0x8000, CRC(fc3bf8f3) SHA1(d9168b9bce110bfd531410179a107895c641e105) )
	ROM_LOAD32_BYTE( "ic29.bin", 0xe0000, 0x8000, CRC(ed51fdc4) SHA1(a2696b15a0911ac3b6b330b7d8df58ca51d629de) )
	ROM_LOAD32_BYTE( "ic21.bin", 0xe0001, 0x8000, CRC(dfe75f3d) SHA1(ff908419066494bc28cbd6afe72bd30350b20c4b) )
	ROM_LOAD32_BYTE( "ic11.bin", 0xe0002, 0x8000, CRC(a2c07741) SHA1(747c029ab399c4110dbe360b8913f5c2e57c87cc) )
	ROM_LOAD32_BYTE( "ic1.bin",  0xe0003, 0x8000, CRC(b191e22f) SHA1(406c7f4eed0b8fe93fa0bef370e496894f4d46a4) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "pic2.bin", 0x0000, 0x8000, CRC(b4740419) SHA1(8ece2dc85692e32d0ba0b427c260c3d10ac0b7cc) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "ic73.bin", 0x00000, 0x004000, CRC(d6397933) SHA1(b85bb47efb6c113b3676b10ab86f1798a89d45b4) )
	ROM_LOAD( "ic72.bin", 0x04000, 0x004000, CRC(504e76d9) SHA1(302af9101da01c97ca4be6acd21fb5b8e8f0b7ef) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "snd7231.256", 0x00000, 0x8000, CRC(871c6b14) SHA1(6d04ddc32fdf1db409cb519890821bd10fc9e58b) )
	ROM_LOAD( "snd7232.256", 0x08000, 0x8000, CRC(4b59340c) SHA1(a01ba8580b65dd17bfd92560265e502d95d3ff16) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* protection MCU */
	ROM_LOAD( "mcu.bin", 0x00000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "sic123.bin", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END
/**************************************************************************************************************************
    Space Harrier
    CPU: 68000 + i8751 (315-5163)
*/
ROM_START( sharrirb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7188.97",  0x000000, 0x8000, CRC(7c30a036) SHA1(d3902342be714b4e181c87ad2bad7102e3eeec20) )
	ROM_LOAD16_BYTE( "epr7184.84",  0x000001, 0x8000, CRC(16deaeb1) SHA1(bdf85b924a914865bf876eda7fc2b20131a4cf2d) )
	ROM_LOAD16_BYTE( "ic98.bin",    0x010000, 0x8000, CRC(40b1309f) SHA1(9b050983f043a88f414745d02c912b59bbf1b121) )
	ROM_LOAD16_BYTE( "ic85.bin",    0x010001, 0x8000, CRC(ce78045c) SHA1(ce640f05bed64ff5b47f1064b5fc13e58bc422a4) )
	ROM_LOAD16_BYTE( "ic99.bin",    0x020000, 0x8000, CRC(f6391091) SHA1(3160b342b6447cccf67c932c7c1a42354cdfb058) )
	ROM_LOAD16_BYTE( "ic86.bin",    0x020001, 0x8000, CRC(79b367d7) SHA1(e901036b1b9fac460415d513837c8f852f7750b0) )
	ROM_LOAD16_BYTE( "ic100.bin",   0x030000, 0x8000, CRC(6171e9d3) SHA1(72f8736f421dc93139859fd47f0c8c3c32b6ff0b) )
	ROM_LOAD16_BYTE( "ic87.bin",    0x030001, 0x8000, CRC(70cb72ef) SHA1(d1d89bd133b6905f81c25513d852b7e3a05a7312) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "ic54.bin", 0x0000, 0x8000, CRC(d7c535b6) SHA1(c0659a678c0c3776387a4a675016e9a2e9c67ee3) )
	ROM_LOAD16_BYTE( "ic67.bin", 0x0001, 0x8000, CRC(a6153af8) SHA1(b56ba472e4afb474c7a3f7dc11d7428ebbe1a9c7) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sic31.bin", 0x00000, 0x08000, CRC(347fa325) SHA1(9076b16de9598b52a75e5084651ee5a220b0e88b) )
	ROM_LOAD( "sic46.bin", 0x08000, 0x08000, CRC(39d98bd1) SHA1(5aab91bdd08b0f1ea537cd43ccc2e82fd01dd031) )
	ROM_LOAD( "sic60.bin", 0x10000, 0x08000, CRC(3da3ea6b) SHA1(9a6ce304a14e6ef0be41d867284a63b941f960fb) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "ic36.bin", 0x00000, 0x8000, CRC(93e2d264) SHA1(ca56de13756ab77408506d88f291da1da8134435) )
	ROM_LOAD32_BYTE( "ic28.bin", 0x00001, 0x8000, CRC(edbf5fc3) SHA1(a93f8c431075741c181eb422b24c9303487ca16c) )
	ROM_LOAD32_BYTE( "ic118.bin",0x00002, 0x8000, CRC(e8c537d8) SHA1(c9b3c0f33272c47d32e6aa349d72f7e355468e0e) )
	ROM_LOAD32_BYTE( "ic8.bin",  0x00003, 0x8000, CRC(22844fa4) SHA1(6d0f177082084c8c92085cd53e1d7ddc62d09a4c) )
	ROM_LOAD32_BYTE( "ic35.bin", 0x20000, 0x8000, CRC(cd6e7500) SHA1(6f0e42eb28ad3f5df93d8bb39dc41aba66eee144) )
	ROM_LOAD32_BYTE( "ic27.bin", 0x20001, 0x8000, CRC(41f25a9c) SHA1(bebbd4c4600028205aeff54190b32b664e4710d0) )
	ROM_LOAD32_BYTE( "ic17.bin", 0x20002, 0x8000, CRC(5bb09a67) SHA1(8bedefd2fa29a1a5e36f1d81c4e9067e3c7f28e9) )
	ROM_LOAD32_BYTE( "ic7.bin",  0x20003, 0x8000, CRC(dcaa2ebf) SHA1(9cf77bb966859febc5e5f1447cb719db64aa4db4) )
	ROM_LOAD32_BYTE( "ic34.bin", 0x40000, 0x8000, CRC(d5e15e66) SHA1(2bd81c5c736d725577adf85532de7802bef057f2) )
	ROM_LOAD32_BYTE( "ic26.bin", 0x40001, 0x8000, CRC(ac62ae2e) SHA1(d472dcc1d9b7889d04870920d5c6e5578597b8dc) )
	ROM_LOAD32_BYTE( "ic16.bin", 0x40002, 0x8000, CRC(9c782295) SHA1(c1627f3d849d2fdb02a590502bafbe133212b943) )
	ROM_LOAD32_BYTE( "ic6.bin",  0x40003, 0x8000, CRC(3711105c) SHA1(075197f614786f89bee28ed944371223bc75c9be) )
	ROM_LOAD32_BYTE( "ic33.bin", 0x60000, 0x8000, CRC(60d7c1bb) SHA1(19ddc1d8f269b50343266d508ad04d4c0fff69d1) )
	ROM_LOAD32_BYTE( "ic25.bin", 0x60001, 0x8000, CRC(f6330038) SHA1(805d4ed664972c0773c837d62f094858c1804148) )
	ROM_LOAD32_BYTE( "ic15.bin", 0x60002, 0x8000, CRC(60737b98) SHA1(5e91498bc473f099a1b2887baf980486e922af97) )
	ROM_LOAD32_BYTE( "ic5.bin",  0x60003, 0x8000, CRC(70fb5ebb) SHA1(38755a9be92865d2c5da8112d8d9c0fe8e778cff) )
	ROM_LOAD32_BYTE( "ic32.bin", 0x80000, 0x8000, CRC(6d7b5c97) SHA1(94c27e4ef1e197ee136f9399b07520cae00a366f) )
	ROM_LOAD32_BYTE( "ic24.bin", 0x80001, 0x8000, CRC(cebf797c) SHA1(d3d5aeba1a0e70a322ec86b930814fa8bc744829) )
	ROM_LOAD32_BYTE( "ic14.bin", 0x80002, 0x8000, CRC(24596a8b) SHA1(f580022c4f7dcaefb7209058c310a329479b5fcd) )
	ROM_LOAD32_BYTE( "ic4.bin",  0x80003, 0x8000, CRC(b537d082) SHA1(f87a644d9af8477c9eb94e5d3aeb5f6376264418) )
	ROM_LOAD32_BYTE( "ic31.bin", 0xa0000, 0x8000, CRC(5e784271) SHA1(063bd83b7f42dec556a7bdf736cb51456ba7184b) )
	ROM_LOAD32_BYTE( "ic23.bin", 0xa0001, 0x8000, CRC(510e5e10) SHA1(47b9f1bc8df0690c37d1d045bd361f8599e8a903) )
	ROM_LOAD32_BYTE( "ic13.bin", 0xa0002, 0x8000, CRC(7a2dad15) SHA1(227865447027f8669aa0d06d059f3d61da6d59dd) )
	ROM_LOAD32_BYTE( "ic3.bin",  0xa0003, 0x8000, CRC(f5ba4e08) SHA1(443b07c996cbb213fe57dfdd3879c1d4da27c001) )
	ROM_LOAD32_BYTE( "ic30.bin", 0xc0000, 0x8000, CRC(ec42c9ef) SHA1(313d908f3a964529b18e09825552879817a2e159) )
	ROM_LOAD32_BYTE( "ic22.bin", 0xc0001, 0x8000, CRC(6d4a7d7a) SHA1(997ac38e47d84f0166ca3ece50ac5f2d3435d8d3) )
	ROM_LOAD32_BYTE( "ic12.bin", 0xc0002, 0x8000, CRC(0f732717) SHA1(6a19c88d5d52f4ec4adb32c511fed4caae81c65f) )
	ROM_LOAD32_BYTE( "ic2.bin",  0xc0003, 0x8000, CRC(fc3bf8f3) SHA1(d9168b9bce110bfd531410179a107895c641e105) )
	ROM_LOAD32_BYTE( "ic29.bin", 0xe0000, 0x8000, CRC(ed51fdc4) SHA1(a2696b15a0911ac3b6b330b7d8df58ca51d629de) )
	ROM_LOAD32_BYTE( "ic21.bin", 0xe0001, 0x8000, CRC(dfe75f3d) SHA1(ff908419066494bc28cbd6afe72bd30350b20c4b) )
	ROM_LOAD32_BYTE( "ic11.bin", 0xe0002, 0x8000, CRC(a2c07741) SHA1(747c029ab399c4110dbe360b8913f5c2e57c87cc) )
	ROM_LOAD32_BYTE( "ic1.bin",  0xe0003, 0x8000, CRC(b191e22f) SHA1(406c7f4eed0b8fe93fa0bef370e496894f4d46a4) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "pic2.bin", 0x0000, 0x8000, CRC(b4740419) SHA1(8ece2dc85692e32d0ba0b427c260c3d10ac0b7cc) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "ic73.bin", 0x00000, 0x004000, CRC(d6397933) SHA1(b85bb47efb6c113b3676b10ab86f1798a89d45b4) )
	ROM_LOAD( "ic72.bin", 0x04000, 0x004000, CRC(504e76d9) SHA1(302af9101da01c97ca4be6acd21fb5b8e8f0b7ef) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "snd7231.256", 0x00000, 0x8000, CRC(871c6b14) SHA1(6d04ddc32fdf1db409cb519890821bd10fc9e58b) )
	ROM_LOAD( "snd7232.256", 0x08000, 0x8000, CRC(4b59340c) SHA1(a01ba8580b65dd17bfd92560265e502d95d3ff16) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* protection MCU */
	ROM_LOAD( "mcu.bin", 0x00000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "sic123.bin", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END


/**************************************************************************************************************************
 **************************************************************************************************************************
 **************************************************************************************************************************
    Enduro Racer with YM2151 sound board
    CPU: FD1089A (317-0013A)
*/
ROM_START( enduror )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7640a.rom",0x00000, 0x8000, CRC(1d1dc5d4) SHA1(8e7ae5abd23e949de5d5e1772f90e53d05c866ec) )
	ROM_LOAD16_BYTE( "7636a.rom",0x00001, 0x8000, CRC(84131639) SHA1(04981464577d2604eec36c14c5de9c91604ae501) )
	ROM_LOAD16_BYTE( "7641.rom", 0x10000, 0x8000, CRC(2503ae7c) SHA1(27009d5b47dc207145048edfcc1ac8ffda5f0b78) )
	ROM_LOAD16_BYTE( "7637.rom", 0x10001, 0x8000, CRC(82a27a8c) SHA1(4b182d8c23454aed7d786c9824932957319b6eff) )
	ROM_LOAD16_BYTE( "7642.rom", 0x20000, 0x8000, CRC(1c453bea) SHA1(c6e606cdcb1690de05ef5283b48a8a61b2e0ad51) )
	ROM_LOAD16_BYTE( "7638.rom", 0x20001, 0x8000, CRC(70544779) SHA1(e6403edd7fc0ad5d447c25be5d7f10889aa109ff) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634a.bin", 0x0000, 0x8000, CRC(aec83731) SHA1(3fe2d0f1a8806b850836741d664c07754a701459) )
	ROM_LOAD16_BYTE("7635a.bin", 0x0001, 0x8000, CRC(b2fce96f) SHA1(9d6c1a7c2bdbf86430b849a5f6c6fdb5595dc91c) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD32_BYTE( "7670.rom", 0x00001, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD32_BYTE( "7662.rom", 0x00002, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD32_BYTE( "7654.rom", 0x00003, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD32_BYTE( "7677.rom", 0x20000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD32_BYTE( "7669.rom", 0x20001, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD32_BYTE( "7661.rom", 0x20002, 0x8000, CRC(fe93a79b) SHA1(591025a371a451c9cddc8c7480c9841a18bb9a7f) )
	ROM_LOAD32_BYTE( "7653.rom", 0x20003, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD32_BYTE( "7676.rom", 0x40000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD32_BYTE( "7668.rom", 0x40001, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD32_BYTE( "7660.rom", 0x40002, 0x8000, CRC(86dfbb68) SHA1(a05ac16fbe3aaf34dd46229d4b71fc1f72a3a556) )
	ROM_LOAD32_BYTE( "7652.rom", 0x40003, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD32_BYTE( "7675.rom", 0x60000, 0x8000, CRC(05cd2d61) SHA1(51688a5a9bc4da3f88ce162ff30affe8c6d3d0c8) )
	ROM_LOAD32_BYTE( "7667.rom", 0x60001, 0x8000, CRC(923bde9d) SHA1(7722a7fdbf45f862f1011d1afae8dedd5885bf52) )
	ROM_LOAD32_BYTE( "7659.rom", 0x60002, 0x8000, CRC(629dc8ce) SHA1(4af2a53678890b02922dee54f7cd3c5550001572) )
	ROM_LOAD32_BYTE( "7651.rom", 0x60003, 0x8000, CRC(d7902bad) SHA1(f4872d1a42dcf7d5dbdbc1233606a706b39478d7) )
	ROM_LOAD32_BYTE( "7674.rom", 0x80000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD32_BYTE( "7666.rom", 0x80001, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD32_BYTE( "7658.rom", 0x80002, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD32_BYTE( "7650.rom", 0x80003, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD32_BYTE( "7673.rom", 0xa0000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD32_BYTE( "7665.rom", 0xa0001, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD32_BYTE( "7657.rom", 0xa0002, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD32_BYTE( "7649.rom", 0xa0003, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD32_BYTE( "7672.rom", 0xc0000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD32_BYTE( "7664.rom", 0xc0001, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD32_BYTE( "7656.rom", 0xc0002, 0x8000, CRC(6c741272) SHA1(ccaedda1436ddc339377e610d51e13726bb6c7eb) )
	ROM_LOAD32_BYTE( "7648.rom", 0xc0003, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD32_BYTE( "7671.rom", 0xe0000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD32_BYTE( "7663.rom", 0xe0001, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD32_BYTE( "7655.rom", 0xe0002, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD32_BYTE( "7647.rom", 0xe0003, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "7682.rom", 0x00000, 0x8000, CRC(c4efbf48) SHA1(2bcbc4757d98f291fcaec467abc36158b3f59be3) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x10000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END

/**************************************************************************************************************************
    Enduro Racer with YM2203 sound board
    CPU: FD1089A (317-0013A)
*/
ROM_START( enduror1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "eprxxxx.ic97", 0x00000, 0x8000, CRC(a1bdadab) SHA1(f52d747a6947ad2dbc12765133adfb41eb5a5f2f) )
	ROM_LOAD16_BYTE( "epr7629.ic84", 0x00001, 0x8000, CRC(f50f4169) SHA1(b4eebb5131bb472db03f0e340743437753a9efe3) )
	ROM_LOAD16_BYTE( "7641.rom",     0x10000, 0x8000, CRC(2503ae7c) SHA1(27009d5b47dc207145048edfcc1ac8ffda5f0b78) )
	ROM_LOAD16_BYTE( "7637.rom",     0x10001, 0x8000, CRC(82a27a8c) SHA1(4b182d8c23454aed7d786c9824932957319b6eff) )
	ROM_LOAD16_BYTE( "7642.rom",     0x20000, 0x8000, CRC(1c453bea) SHA1(c6e606cdcb1690de05ef5283b48a8a61b2e0ad51) )
	ROM_LOAD16_BYTE( "7638.rom",     0x20001, 0x8000, CRC(70544779) SHA1(e6403edd7fc0ad5d447c25be5d7f10889aa109ff) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD32_BYTE( "7670.rom", 0x00001, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD32_BYTE( "7662.rom", 0x00002, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD32_BYTE( "7654.rom", 0x00003, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD32_BYTE( "7677.rom", 0x20000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD32_BYTE( "7669.rom", 0x20001, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD32_BYTE( "7661.rom", 0x20002, 0x8000, CRC(fe93a79b) SHA1(591025a371a451c9cddc8c7480c9841a18bb9a7f) )
	ROM_LOAD32_BYTE( "7653.rom", 0x20003, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD32_BYTE( "7676.rom", 0x40000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD32_BYTE( "7668.rom", 0x40001, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD32_BYTE( "7660.rom", 0x40002, 0x8000, CRC(86dfbb68) SHA1(a05ac16fbe3aaf34dd46229d4b71fc1f72a3a556) )
	ROM_LOAD32_BYTE( "7652.rom", 0x40003, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD32_BYTE( "7675.rom", 0x60000, 0x8000, CRC(05cd2d61) SHA1(51688a5a9bc4da3f88ce162ff30affe8c6d3d0c8) )
	ROM_LOAD32_BYTE( "7667.rom", 0x60001, 0x8000, CRC(923bde9d) SHA1(7722a7fdbf45f862f1011d1afae8dedd5885bf52) )
	ROM_LOAD32_BYTE( "7659.rom", 0x60002, 0x8000, CRC(629dc8ce) SHA1(4af2a53678890b02922dee54f7cd3c5550001572) )
	ROM_LOAD32_BYTE( "7651.rom", 0x60003, 0x8000, CRC(d7902bad) SHA1(f4872d1a42dcf7d5dbdbc1233606a706b39478d7) )
	ROM_LOAD32_BYTE( "7674.rom", 0x80000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD32_BYTE( "7666.rom", 0x80001, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD32_BYTE( "7658.rom", 0x80002, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD32_BYTE( "7650.rom", 0x80003, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD32_BYTE( "7673.rom", 0xa0000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD32_BYTE( "7665.rom", 0xa0001, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD32_BYTE( "7657.rom", 0xa0002, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD32_BYTE( "7649.rom", 0xa0003, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD32_BYTE( "7672.rom", 0xc0000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD32_BYTE( "7664.rom", 0xc0001, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD32_BYTE( "7656.rom", 0xc0002, 0x8000, CRC(6c741272) SHA1(ccaedda1436ddc339377e610d51e13726bb6c7eb) )
	ROM_LOAD32_BYTE( "7648.rom", 0xc0003, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD32_BYTE( "7671.rom", 0xe0000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD32_BYTE( "7663.rom", 0xe0001, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD32_BYTE( "7655.rom", 0xe0002, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD32_BYTE( "7647.rom", 0xe0003, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU -- taken from endurobl below */
	ROM_LOAD( "13.16d", 0x00000, 0x004000, BAD_DUMP CRC(81c82fc9) SHA1(99eae7edc62d719993c46a703f9daaf332e236e9) )
	ROM_LOAD( "12.16e", 0x04000, 0x004000, BAD_DUMP CRC(755bfdad) SHA1(2942f3da5a45a3ac7bba6a73142663fd975f4379) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data -- taken from endurobl below */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, BAD_DUMP CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, BAD_DUMP CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END

/**************************************************************************************************************************
    Enduro Racer (bootleg) with YM2203 sound board
    CPU: 68000
*/
ROM_START( endurobl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7.13j", 0x030000, 0x08000, CRC(f1d6b4b7) SHA1(32bd966191cbb36d1e60ed1a06d4caa023dd6b88) )
	ROM_CONTINUE(             0x000000, 0x08000 )
	ROM_LOAD16_BYTE( "4.13h", 0x030001, 0x08000, CRC(43bff873) SHA1(04e906c1965a6211fb8e13987db52f1f99cc0203) )				// rom de-coded
	ROM_CONTINUE(             0x000001, 0x08000 )		// data de-coded
	ROM_LOAD16_BYTE( "8.14j", 0x010000, 0x08000, CRC(2153154a) SHA1(145d8ed59812d26ca412a01ae77cd7872adaba5a) )
	ROM_LOAD16_BYTE( "5.14h", 0x010001, 0x08000, CRC(0a97992c) SHA1(7a6fc8c575637107ed07a30f6f0f8cb8877cbb43) )
	ROM_LOAD16_BYTE( "9.15j", 0x020000, 0x08000, CRC(db3bff1c) SHA1(343ed27a690800683cdd5128dcdb28c7b45288a3) )	// one byte difference from
	ROM_LOAD16_BYTE( "6.15h", 0x020001, 0x08000, CRC(54b1885a) SHA1(f53d906390e5414e73c4cdcbc102d3cb3e719e67) )	// 7638.rom / 7642.rom

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD32_BYTE( "7670.rom", 0x00001, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD32_BYTE( "7662.rom", 0x00002, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD32_BYTE( "7654.rom", 0x00003, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD32_BYTE( "7677.rom", 0x20000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD32_BYTE( "7669.rom", 0x20001, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD32_BYTE( "7661.rom", 0x20002, 0x8000, CRC(fe93a79b) SHA1(591025a371a451c9cddc8c7480c9841a18bb9a7f) )
	ROM_LOAD32_BYTE( "7653.rom", 0x20003, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD32_BYTE( "7676.rom", 0x40000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD32_BYTE( "7668.rom", 0x40001, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD32_BYTE( "7660.rom", 0x40002, 0x8000, CRC(86dfbb68) SHA1(a05ac16fbe3aaf34dd46229d4b71fc1f72a3a556) )
	ROM_LOAD32_BYTE( "7652.rom", 0x40003, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD32_BYTE( "7675.rom", 0x60000, 0x8000, CRC(05cd2d61) SHA1(51688a5a9bc4da3f88ce162ff30affe8c6d3d0c8) )
	ROM_LOAD32_BYTE( "7667.rom", 0x60001, 0x8000, CRC(923bde9d) SHA1(7722a7fdbf45f862f1011d1afae8dedd5885bf52) )
	ROM_LOAD32_BYTE( "7659.rom", 0x60002, 0x8000, CRC(629dc8ce) SHA1(4af2a53678890b02922dee54f7cd3c5550001572) )
	ROM_LOAD32_BYTE( "7651.rom", 0x60003, 0x8000, CRC(d7902bad) SHA1(f4872d1a42dcf7d5dbdbc1233606a706b39478d7) )
	ROM_LOAD32_BYTE( "7674.rom", 0x80000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD32_BYTE( "7666.rom", 0x80001, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD32_BYTE( "7658.rom", 0x80002, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD32_BYTE( "7650.rom", 0x80003, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD32_BYTE( "7673.rom", 0xa0000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD32_BYTE( "7665.rom", 0xa0001, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD32_BYTE( "7657.rom", 0xa0002, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD32_BYTE( "7649.rom", 0xa0003, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD32_BYTE( "7672.rom", 0xc0000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD32_BYTE( "7664.rom", 0xc0001, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD32_BYTE( "7656.rom", 0xc0002, 0x8000, CRC(6c741272) SHA1(ccaedda1436ddc339377e610d51e13726bb6c7eb) )
	ROM_LOAD32_BYTE( "7648.rom", 0xc0003, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD32_BYTE( "7671.rom", 0xe0000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD32_BYTE( "7663.rom", 0xe0001, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD32_BYTE( "7655.rom", 0xe0002, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD32_BYTE( "7647.rom", 0xe0003, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "13.16d", 0x00000, 0x004000, CRC(81c82fc9) SHA1(99eae7edc62d719993c46a703f9daaf332e236e9) )
	ROM_LOAD( "12.16e", 0x04000, 0x004000, CRC(755bfdad) SHA1(2942f3da5a45a3ac7bba6a73142663fd975f4379) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END

/**************************************************************************************************************************
    Enduro Racer (bootleg) with 2xYM2203 sound board
    CPU: 68000
*/
ROM_START( endurob2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	/* the program roms should be twice the size */
	ROM_LOAD16_BYTE( "enduro.a07", 0x000000, 0x08000, BAD_DUMP CRC(259069bc) SHA1(42fa47ce4a29294f9eff3eddbba6c305d750aaa5) )
//  ROM_CONTINUE(                  0x030000, 0x08000 )
	ROM_LOAD16_BYTE( "enduro.a04", 0x000001, 0x08000, BAD_DUMP CRC(f584fbd9) SHA1(6c9ddcd1d9cf95c6250b705b27865644da45d197) )
//  ROM_CONTINUE(                  0x030000, 0x08000 )
	ROM_LOAD16_BYTE( "enduro.a08", 0x010000, 0x08000, CRC(d234918c) SHA1(ce2493a4ceff48331551e915fdbe19107865436e) )
	ROM_LOAD16_BYTE( "enduro.a05", 0x010001, 0x08000, CRC(a525dd57) SHA1(587f449ea317dc9eae06e755e7c63a652effbe15) )
	ROM_LOAD16_BYTE( "enduro.a09", 0x020000, 0x08000, CRC(f6391091) SHA1(3160b342b6447cccf67c932c7c1a42354cdfb058) )
	ROM_LOAD16_BYTE( "enduro.a06", 0x020001, 0x08000, CRC(79b367d7) SHA1(e901036b1b9fac460415d513837c8f852f7750b0) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION32_LE( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD32_BYTE( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD32_BYTE( "7670.rom", 0x00001, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD32_BYTE( "7662.rom", 0x00002, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD32_BYTE( "7654.rom", 0x00003, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD32_BYTE( "7677.rom", 0x20000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD32_BYTE( "7669.rom", 0x20001, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD32_BYTE( "enduro.a34", 0x20002, 0x8000, CRC(296454d8) SHA1(17e829a08606837d36006849edffe54c76c384d5) )
	ROM_LOAD32_BYTE( "7653.rom", 0x20003, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD32_BYTE( "7676.rom", 0x40000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD32_BYTE( "7668.rom", 0x40001, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD32_BYTE( "enduro.a35", 0x40002, 0x8000, CRC(1ebe76df) SHA1(c68257d92b79cd346ca9f5639e6b3dffc6e21a5d) )
	ROM_LOAD32_BYTE( "7652.rom", 0x40003, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD32_BYTE( "enduro.a20", 0x60000, 0x8000, CRC(7c280bc8) SHA1(ad8bb0204a53ea1415f088819748d40c47d96509) )
	ROM_LOAD32_BYTE( "enduro.a28", 0x60001, 0x8000, CRC(321f034b) SHA1(e30f541d0f17a75ac02a49bd5d621c75fdd89298) )
	ROM_LOAD32_BYTE( "enduro.a36", 0x60002, 0x8000, CRC(243e34e5) SHA1(4117435e97841ac2e0233089343f14b4a27dcaed) )
	ROM_LOAD32_BYTE( "enduro.a44", 0x60003, 0x8000, CRC(84bb12a1) SHA1(340de454cee9d78f8b64e12b74450b7a152b8726) )
	ROM_LOAD32_BYTE( "7674.rom", 0x80000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD32_BYTE( "7666.rom", 0x80001, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD32_BYTE( "7658.rom", 0x80002, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD32_BYTE( "7650.rom", 0x80003, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD32_BYTE( "7673.rom", 0xa0000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD32_BYTE( "7665.rom", 0xa0001, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD32_BYTE( "7657.rom", 0xa0002, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD32_BYTE( "7649.rom", 0xa0003, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD32_BYTE( "7672.rom", 0xc0000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD32_BYTE( "7664.rom", 0xc0001, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD32_BYTE( "enduro.a39", 0xc0002, 0x8000, CRC(1ff3a5e2) SHA1(b4672ed06f6f1ed28538e6dc63efa6eed5c34587) )
	ROM_LOAD32_BYTE( "7648.rom", 0xc0003, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD32_BYTE( "7671.rom", 0xe0000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD32_BYTE( "7663.rom", 0xe0001, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD32_BYTE( "7655.rom", 0xe0002, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD32_BYTE( "7647.rom", 0xe0003, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x8000, REGION_GFX3, 0 ) /* road gfx */
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* sound CPU */
	ROM_LOAD( "enduro.a16", 0x00000, 0x8000, CRC(d2cb6eb5) SHA1(80c5fab16ec4ddfa67fae94808026b2e6285b7f1) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x2000, REGION_PROMS, 0 ) /* zoom table */
	ROM_LOAD( "6844.rom", 0x0000, 0x2000, CRC(e3ec7bd6) SHA1(feec0fe664e16fac0fde61cf64b401b9b0575323) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( hangon )
{
	hangon_generic_init();
}


static DRIVER_INIT( sharrier )
{
	hangon_generic_init();
	i8751_vblank_hook = sharrier_i8751_sim;
}


static DRIVER_INIT( enduror )
{
	void fd1089_decrypt_0013A(void);
	hangon_generic_init();
	fd1089_decrypt_0013A();
}


static DRIVER_INIT( endurobl )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	UINT16 *decrypt = (UINT16 *)auto_malloc(0x40000);

	hangon_generic_init();
	memory_set_decrypted_region(0, 0x000000, 0x03ffff, decrypt);

	memcpy(decrypt + 0x00000/2, rom + 0x30000/2, 0x10000);
	memcpy(decrypt + 0x10000/2, rom + 0x10000/2, 0x20000);
}


static DRIVER_INIT( endurob2 )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	UINT16 *decrypt = (UINT16 *)auto_malloc(0x40000);

	hangon_generic_init();
	memory_set_decrypted_region(0, 0x000000, 0x03ffff, decrypt);

	memcpy(decrypt, rom, 0x30000);
	/* missing data ROM */
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1985, hangon,   0,        hangon,   hangon,   hangon,   ROT0, "Sega",    "Hang-On", 0 )
GAME( 1992, shangupb, shangon,  shangupb, shangupb, hangon,   ROT0, "bootleg", "Super Hang-On (Hang-On upgrade, bootleg)", 0 )
GAME( 1985, sharrier, 0,        sharrier, sharrier, sharrier, ROT0, "Sega",    "Space Harrier (Rev A, 8751 315-5163A)", 0 )
GAME( 1985, sharrirb, sharrier, sharrier, sharrier, sharrier, ROT0, "Sega",    "Space Harrier (8751 315-5163)", 0 )
GAME( 1986, enduror,  0,        enduror,  enduror,  enduror,  ROT0, "Sega",    "Enduro Racer (YM2151, FD1089B 317-0013A)", 0 )
GAME( 1986, enduror1, enduror,  enduror1, enduror,  enduror,  ROT0, "Sega",    "Enduro Racer (YM2203, FD1089B 317-0013A)", 0 )
GAME( 1986, endurobl, enduror,  enduror1, enduror,  endurobl, ROT0, "bootleg", "Enduro Racer (bootleg set 1)", 0 )
GAME( 1986, endurob2, enduror,  endurob2, enduror,  endurob2, ROT0, "bootleg", "Enduro Racer (bootleg set 2)", GAME_NOT_WORKING )
