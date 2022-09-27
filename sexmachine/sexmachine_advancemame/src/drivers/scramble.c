/***************************************************************************

 Scramble hardware


Interesting tidbit:

There is a bug in Amidars and Triple Punch. Look at the loop at 0x2715.
It expects DE to be saved during the call to 0x2726, but it can be destroyed,
causing the loop to read all kinds of bogus memory locations.


To Do:

- Mariner has discrete sound circuits connected to the 8910's output ports


Notes:

- While Atlantis has a cabinet switch, it doesn't use the 2nd player controls
  in cocktail mode.

***************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "sound/5110intf.h"
#include "sound/tms5110.h"
#include "galaxian.h"


/***************************************************************************
    AD2083 TMS5110 implementation (still wrong!)
***************************************************************************/

static INT32 speech_rom_address = 0;
static INT32 speech_rom_address_hi = 0;
static INT32 speech_rom_bit = 0;

static void start_talking (void)
{
	tms5110_CTL_w(0,TMS5110_CMD_SPEAK);
	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);
}

static void reset_talking (void)
{
	tms5110_CTL_w(0,TMS5110_CMD_RESET);
	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	tms5110_PDC_w(0,0);
	tms5110_PDC_w(0,1);
	tms5110_PDC_w(0,0);

	speech_rom_address    = 0;
	speech_rom_address_hi = 0;
    speech_rom_bit        = 0;
}

int ad2083_speech_rom_read_bit(void)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);
	int bit;

	speech_rom_address %= memory_region_length(REGION_SOUND1);

	bit = (ROM[speech_rom_address] >> speech_rom_bit) & 1;
//  bit = (ROM[speech_rom_address] >> (speech_rom_bit ^ 7)) & 1;

	speech_rom_bit++;
	if(speech_rom_bit == 8)
	{
		speech_rom_address++;
		speech_rom_bit = 0;
	}

	return bit;
}


static WRITE8_HANDLER( ad2083_soundlatch_w )
{
	soundlatch_w(0,data);

	if(data & 0x80)
	{
		reset_talking();
	}
	else if(data & 0x30)
	{
		start_talking();

		if((data & 0x30) == 0x30)
			speech_rom_address_hi = 0x1000;
		else
			speech_rom_address_hi = 0;

	}
}

static WRITE8_HANDLER( ad2083_tms5110_ctrl_w )
{
	speech_rom_address = speech_rom_address_hi | (data * 0x40);
}

static MACHINE_START( ad2083 )
{
	state_save_register_global(speech_rom_address);
	state_save_register_global(speech_rom_address_hi);
	state_save_register_global(speech_rom_bit);

	return 0;
}


static ADDRESS_MAP_START( scramble_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)	/* mirror */
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x7800, 0x7800) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8100, 0x8103) AM_READ(ppi8255_0_r)
	AM_RANGE(0x8110, 0x8113) AM_READ(ppi8255_0_r)  /* mirror for Frog */
	AM_RANGE(0x8200, 0x8203) AM_READ(ppi8255_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( scramble_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(galaxian_videoram_w)	/* mirror address */
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_coin_counter_w)
	AM_RANGE(0x6804, 0x6804) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6806, 0x6806) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8100, 0x8103) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x8200, 0x8203) AM_WRITE(ppi8255_1_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( explorer_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)	/* mirror */
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5100, 0x51ff) AM_READ(MRA8_NOP)	/* test mode mirror? */
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8000, 0x8000) AM_READ(input_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(input_port_1_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(input_port_2_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( explorer_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(galaxian_videoram_w)	/* mirror address */
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5100, 0x51ff) AM_WRITE(MWA8_NOP)	/* test mode mirror? */
	AM_RANGE(0x6800, 0x6800) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_coin_counter_w)
	AM_RANGE(0x6803, 0x6803) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x6804, 0x6804) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6806, 0x6806) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(soundlatch_w)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(hotshock_sh_irqtrigger_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( ckongs_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x6bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7003) AM_READ(ppi8255_0_r)
	AM_RANGE(0x7800, 0x7803) AM_READ(ppi8255_1_r)
	AM_RANGE(0x9000, 0x93ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9800, 0x98ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb000, 0xb000) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ckongs_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x6bff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7000, 0x7003) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x7800, 0x7803) AM_WRITE(ppi8255_1_w)
	AM_RANGE(0x9000, 0x93ff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x9800, 0x983f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x9840, 0x985f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x9860, 0x987f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x9880, 0x98ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa801, 0xa801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0xa802, 0xa802) AM_WRITE(galaxian_coin_counter_w)
	AM_RANGE(0xa806, 0xa806) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xa807, 0xa807) AM_WRITE(galaxian_flip_screen_y_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mars_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x7000, 0x7000) AM_READ(MRA8_NOP)
	AM_RANGE(0x8100, 0x810f) AM_READ(mars_ppi8255_0_r)
	AM_RANGE(0x8200, 0x820f) AM_READ(mars_ppi8255_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mars_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6808, 0x6808) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0x6809, 0x6809) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x680b, 0x680b) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8100, 0x810f) AM_WRITE(mars_ppi8255_0_w)
	AM_RANGE(0x8200, 0x820f) AM_WRITE(mars_ppi8255_1_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( newsin7_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8200, 0x820f) AM_READ(mars_ppi8255_1_r)
	AM_RANGE(0xa000, 0xafff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc100, 0xc10f) AM_READ(mars_ppi8255_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( newsin7_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6808, 0x6808) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0x6809, 0x6809) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x680b, 0x680b) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8200, 0x820f) AM_WRITE(mars_ppi8255_1_w)
	AM_RANGE(0xa000, 0xafff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc100, 0xc10f) AM_WRITE(mars_ppi8255_0_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mrkougar_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(galaxian_videoram_w)	/* mirror address */
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6808, 0x6808) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0x6809, 0x6809) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x680b, 0x680b) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8100, 0x810f) AM_WRITE(mars_ppi8255_0_w)
	AM_RANGE(0x8200, 0x820f) AM_WRITE(mars_ppi8255_1_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hotshock_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0x8000) AM_READ(input_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(input_port_1_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(input_port_2_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotshock_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(galaxian_coin_counter_2_w)
	AM_RANGE(0x6002, 0x6002) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0x6004, 0x6004) AM_WRITE(hotshock_flip_screen_w)
	AM_RANGE(0x6005, 0x6005) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0x6006, 0x6006) AM_WRITE(galaxian_gfxbank_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(soundlatch_w)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(hotshock_sh_irqtrigger_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hunchbks_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1210, 0x1213) AM_READ(ppi8255_1_r)
	AM_RANGE(0x1400, 0x14ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1500, 0x1503) AM_READ(ppi8255_0_r)
	AM_RANGE(0x1680, 0x1680) AM_READ(watchdog_reset_r)
	AM_RANGE(0x1780, 0x1780) AM_READ(watchdog_reset_r)
	AM_RANGE(0x1800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_READ(hunchbks_mirror_r)
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_READ(hunchbks_mirror_r)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(hunchbks_mirror_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hunchbks_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1210, 0x1213) AM_WRITE(ppi8255_1_w)
	AM_RANGE(0x1400, 0x143f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x1440, 0x145f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x1460, 0x147f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x1480, 0x14ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1500, 0x1503) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x1606, 0x1606) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x1607, 0x1607) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x1800, 0x1bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x1c00, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(hunchbks_mirror_w)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(hunchbks_mirror_w)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(hunchbks_mirror_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sfx_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(galaxian_videoram_r)	/* mirror */
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8100, 0x8103) AM_READ(ppi8255_0_r)
	AM_RANGE(0x8200, 0x8203) AM_READ(ppi8255_1_r)
	AM_RANGE(0xc000, 0xefff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(galaxian_videoram_w)	/* mirror address */
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(scramble_background_red_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_coin_counter_w)
	AM_RANGE(0x6803, 0x6803) AM_WRITE(scramble_background_blue_w)
	AM_RANGE(0x6804, 0x6804) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6805, 0x6805) AM_WRITE(scramble_background_green_w)
	AM_RANGE(0x6806, 0x6806) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8100, 0x8103) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x8200, 0x8203) AM_WRITE(ppi8255_1_w)
	AM_RANGE(0xc000, 0xefff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_sample_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_sample_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_sample_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x07) AM_READ(ppi8255_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_sample_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x07) AM_WRITE(ppi8255_2_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(DAC_0_signed_data_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mimonscr_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(galaxian_videoram_r)	/* mirror address?, probably not */
	AM_RANGE(0x4400, 0x4bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x50ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8100, 0x8103) AM_READ(ppi8255_0_r)
	AM_RANGE(0x8200, 0x8203) AM_READ(ppi8255_1_r)
	AM_RANGE(0xc000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mimonscr_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(galaxian_videoram_w)	/* mirror address?, probably not */
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5000, 0x503f) AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_WRITE(MWA8_RAM) AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x5080, 0x50ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6800, 0x6802) AM_WRITE(galaxian_gfxbank_w)
	AM_RANGE(0x6806, 0x6806) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8100, 0x8103) AM_WRITE(ppi8255_0_w)
	AM_RANGE(0x8200, 0x8203) AM_WRITE(ppi8255_1_w)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( frogf_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8bff) AM_RAM AM_WRITE(galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x9000, 0x903f) AM_RAM AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x9040, 0x905f) AM_RAM AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x9060, 0x90ff) AM_RAM
	AM_RANGE(0xa802, 0xa802) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xa804, 0xa804) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0xa806, 0xa806) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xa808, 0xa808) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0xa80e, 0xa80e) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0xb800, 0xb800) AM_READ(watchdog_reset_r)
	AM_RANGE(0xd000, 0xd018) AM_READWRITE(hustler_ppi8255_0_r, hustler_ppi8255_0_w)
	AM_RANGE(0xe000, 0xe018) AM_READWRITE(hustler_ppi8255_1_r, hustler_ppi8255_1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ad2083_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x4800, 0x4bff) AM_READWRITE(galaxian_videoram_r, galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0x5000, 0x503f) AM_RAM AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0x5040, 0x505f) AM_RAM AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0x5060, 0x507f) AM_RAM AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0x6004, 0x6004) AM_WRITE(hotshock_flip_screen_w)
	AM_RANGE(0x6800, 0x6800) AM_WRITE(galaxian_coin_counter_2_w)
	AM_RANGE(0x6801, 0x6801) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_WRITE(galaxian_coin_counter_0_w)
	AM_RANGE(0x6803, 0x6803) AM_WRITE(scramble_background_blue_w)
	AM_RANGE(0x6805, 0x6805) AM_WRITE(galaxian_coin_counter_1_w)
	AM_RANGE(0x6806, 0x6806) AM_WRITE(scramble_background_red_w)
	AM_RANGE(0x6807, 0x6807) AM_WRITE(scramble_background_green_w)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(ad2083_soundlatch_w)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(hotshock_sh_irqtrigger_w)
	AM_RANGE(0x7000, 0x7000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8000, 0x8000) AM_READ(input_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(input_port_1_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(input_port_2_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(input_port_3_r)
	AM_RANGE(0xa000, 0xdfff) AM_ROM
	AM_RANGE(0xe800, 0xebff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ad2083_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ad2083_sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_WRITE(ad2083_tms5110_ctrl_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x20, 0x20) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(AY8910_control_port_1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( triplep_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(triplep_pip_r)
	AM_RANGE(0x03, 0x03) AM_READ(triplep_pap_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( triplep_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hotshock_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x20, 0x20) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x40, 0x40) AM_READ(AY8910_read_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotshock_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(AY8910_control_port_1_w)
ADDRESS_MAP_END

static WRITE8_HANDLER( scorpion_extra_sound_w )
{
	if(data != 0xff)
		ui_popup("Played sample/speech %02X",data);
}

static WRITE8_HANDLER( scorpion_sound_cmd_w )
{
	// data == 0xfc -> don't play anything
	if(data == 0xf8)
	{
		/* play the sample/speech */
	}
}

static ADDRESS_MAP_START( scorpion_sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x04) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x08, 0x08) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x20, 0x20) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_control_port_2_w)
	AM_RANGE(0x80, 0x80) AM_READWRITE(AY8910_read_port_2_r, AY8910_write_port_2_w)
ADDRESS_MAP_END

static READ8_HANDLER( hncholms_prot_r )
{
	if(activecpu_get_pc() == 0x2b || activecpu_get_pc() == 0xa27)
		return 1;
	else
		return 0;
}

static ADDRESS_MAP_START( hunchbks_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(hncholms_prot_r)
    AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_3_r)
ADDRESS_MAP_END

INPUT_PORTS_START( scramble )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "255 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1  B 2/1  C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2  B 1/1  C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3  B 3/1  C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4  B 4/1  C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
INPUT_PORTS_END

INPUT_PORTS_START( explorer )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pressing this disables the coins */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START_TAG("IN3")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x0c, "15000" )
	PORT_DIPSETTING(    0x14, "20000" )
	PORT_DIPSETTING(    0x1c, "25000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x10, "70000" )
	PORT_DIPSETTING(    0x18, "90000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END

INPUT_PORTS_START( strfbomb )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "255 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/2  B 4/1  C 1/2" )
	PORT_DIPSETTING(    0x02, "A 1/3  B 2/1  C 1/3" )
	PORT_DIPSETTING(    0x04, "A 1/4  B 3/1  C 1/4" )
	PORT_DIPSETTING(    0x06, "A 1/5  B 1/1  C 1/5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
INPUT_PORTS_END

INPUT_PORTS_START( atlantis )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/3  B 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/6  B 1/1" )
	PORT_DIPSETTING(    0x04, "A 1/99 B 1/99")
	/* all the other combos give 99 credits */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( theend )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "256 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )		/* output bits */
INPUT_PORTS_END

INPUT_PORTS_START( froggers )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* read - function unknown */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "256 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(    0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( amidars )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "256 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( triplep )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "256 (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(    0x80, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test (Cheat)") PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ckongs )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
INPUT_PORTS_END

INPUT_PORTS_START( mars )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_8WAY PORT_PLAYER(2)  /* this also control cocktail mode */
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "255 (Cheat)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_8WAY

	PORT_START_TAG("IN3")
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_8WAY PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( devilfsh )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "15000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( newsin7 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, " A 1C/1C  B 2C/1C" )
	PORT_DIPSETTING(    0x01, " A 1C/3C  B 3C/1C" )
	PORT_DIPSETTING(    0x02, " A 1C/2C  B 1C/1C" )
	PORT_DIPSETTING(    0x00, " A 1C/4C  B 4C/1C" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )  /* difficulty? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mrkougar )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hotshock )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pressing this disables the coins */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START_TAG("IN3")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x04, DEF_STR( English ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Italian ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "75000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x10, "200000" )
	PORT_DIPSETTING(    0x18, DEF_STR( None ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( hunchbks )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "80000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */

    PORT_START_TAG("SENSE")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( hncholms )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x02, "A 1/1 B 1/5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "80000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */

    PORT_START_TAG("SENSE")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( cavelon )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* force UR controls in CK mode? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
INPUT_PORTS_END

INPUT_PORTS_START( sfx )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	// "Fire" left
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	// "Fire" right
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x03, "Invulnerability (Cheat)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	// "Fire" left
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	// "Fire" right
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
INPUT_PORTS_END

/* Same as 'mimonkey' (scobra.c driver) */
INPUT_PORTS_START( mimonscr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_DIPNAME( 0x20, 0x00, "Infinite Lives (Cheat)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )   /* used, something to do with the bullets */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( scorpion )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3")
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "255" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, "A 1/1  B 1/1" )
	PORT_DIPSETTING(    0x00, "A 1/1  B 1/3" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_DIPNAME( 0xa0, 0xa0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
INPUT_PORTS_END

INPUT_PORTS_START( ad2083 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // if ON it doesn't accept any COIN
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( None ) )
	PORT_DIPSETTING(    0x04, "150000" )
	PORT_DIPSETTING(    0x08, "100000" )
	PORT_DIPSETTING(    0x00, "200000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout devilfsh_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout devilfsh_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static const gfx_layout newsin7_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 2*2*256*8*8, 0, 2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout newsin7_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 2*2*64*16*16, 0, 2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout mrkougar_charlayout =
{
	8,8,
	256,
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};
static const gfx_layout mrkougar_spritelayout =
{
	16,16,
	64,
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
	  24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_layout sfx_charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,4),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout sfx_spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,4),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout ad2083_charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,2),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout ad2083_spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,2),
	2,	/* 2 bits per pixel */
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static const gfx_decode devilfsh_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &devilfsh_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0800, &devilfsh_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

static const gfx_decode newsin7_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &newsin7_charlayout,   0, 4 },
	{ REGION_GFX1, 0x0800, &newsin7_spritelayout, 0, 4 },
	{ -1 } /* end of array */
};

static const gfx_decode mrkougar_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &mrkougar_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0000, &mrkougar_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

gfx_decode sfx_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0800, &sfx_charlayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &sfx_spritelayout,  0, 8 },
	{ -1 } /* end of array */
};

gfx_decode ad2083_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &ad2083_charlayout,    0, 8 },
	{ REGION_GFX1, 0x0000, &ad2083_spritelayout,  0, 8 },
	{ -1 } /* end of array */
};

static struct AY8910interface sfx_ay8910_interface_1 =
{
	0,
	0,
	soundlatch2_w,
	sfx_sh_irqtrigger_w
};

struct AY8910interface explorer_ay8910_interface_1 =
{
	scramble_portB_r
};

struct AY8910interface explorer_ay8910_interface_2 =
{
	hotshock_soundlatch_r
};

struct AY8910interface hotshock_ay8910_interface_2 =
{
	hotshock_soundlatch_r,
	scramble_portB_r
};

struct AY8910interface scorpion_ay8910_interface_1 =
{
	0,
	0,
	scorpion_extra_sound_w,
	scorpion_sound_cmd_w,
};

struct AY8910interface triplep_ay8910_interface =
{
	0
};

static struct TMS5110interface tms5110_interface =
{
	0,							/* irq callback function */
	ad2083_speech_rom_read_bit	/* M0 callback function. Called whenever chip requests a single bit of data */
};

static MACHINE_DRIVER_START( scramble )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(scramble_readmem,scramble_writemem)

	MDRV_CPU_ADD_TAG("audio", Z80, 14318000/8)	/* 1.78975 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(scobra_sound_readmem,scobra_sound_writemem)
	MDRV_CPU_IO_MAP(scobra_sound_readport,scobra_sound_writeport)

	MDRV_MACHINE_RESET(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+1)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */

	MDRV_PALETTE_INIT(scramble)
	MDRV_VIDEO_START(scramble)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910.1", AY8910, 14318000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)

	MDRV_SOUND_ADD_TAG("8910.2", AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(scobra_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fscramble )

	/* scramble with filters */
	MDRV_IMPORT_FROM(scramble)
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_ROUTE(0, "filter1A", 1.0)
	MDRV_SOUND_ROUTE(1, "filter1B", 1.0)
	MDRV_SOUND_ROUTE(2, "filter1C", 1.0)

	MDRV_SOUND_ADD_TAG("filter1A", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.3)
	MDRV_SOUND_ADD_TAG("filter1B", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.3)
	MDRV_SOUND_ADD_TAG("filter1C", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15) // scramblec057gre

	MDRV_SOUND_MODIFY("8910.2")
	MDRV_SOUND_ROUTE(0, "filter2A", 1.0)
	MDRV_SOUND_ROUTE(1, "filter2B", 1.0)
	MDRV_SOUND_ROUTE(2, "filter2C", 1.0)

	MDRV_SOUND_ADD_TAG("filter2A", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.3)
	MDRV_SOUND_ADD_TAG("filter2B", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.3)
	MDRV_SOUND_ADD_TAG("filter2C", FILTER_RC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15) // scramblec057gre
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( explorer )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(explorer_readmem,explorer_writemem)

	MDRV_MACHINE_RESET(explorer)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_MODIFY("8910.2")
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( theend )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */

	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(theend)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( froggers )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_PROGRAM_MAP(frogger_sound_readmem,frogger_sound_writemem)
	MDRV_CPU_IO_MAP(frogger_sound_readport,frogger_sound_writeport)

	/* video hardware */
	MDRV_PALETTE_INIT(frogger)
	MDRV_VIDEO_START(froggers)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(frogger_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_REMOVE("8910.2")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( frogf )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(frogf_map,0)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_PROGRAM_MAP(frogger_sound_readmem,frogger_sound_writemem)
	MDRV_CPU_IO_MAP(frogger_sound_readport,frogger_sound_writeport)

	/* video hardware */
	MDRV_PALETTE_INIT(frogger)
	MDRV_VIDEO_START(froggers)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(frogger_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_REMOVE("8910.2")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mars )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mars_readmem,mars_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( devilfsh )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mars_readmem,mars_writemem)

	/* video hardware */
	MDRV_GFXDECODE(devilfsh_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( newsin7 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(newsin7_readmem,newsin7_writemem)

	/* video hardware */
	MDRV_GFXDECODE(newsin7_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(newsin7)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mrkougar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mars_readmem,mrkougar_writemem)

	/* video hardware */
	MDRV_GFXDECODE(mrkougar_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mrkougb )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mars_readmem,mrkougar_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ckongs )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(ckongs_readmem,ckongs_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(ckongs)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hotshock )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(hotshock_readmem,hotshock_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_IO_MAP(hotshock_sound_readport,hotshock_sound_writeport)

	MDRV_MACHINE_RESET(galaxian)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(pisces)

	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_MODIFY("8910.2")
	MDRV_SOUND_CONFIG(hotshock_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cavelon )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets, 0/1 for background */
	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(ckongs)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sfx )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sfx_readmem,sfx_writemem)

	MDRV_CPU_ADD(Z80, 14318000/8)	/* 1.78975 MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sfx_sample_readmem,sfx_sample_writemem)
	MDRV_CPU_IO_MAP(sfx_sample_readport,sfx_sample_writeport)

	MDRV_MACHINE_RESET(sfx)

	/* video hardware */
	MDRV_VISIBLE_AREA(2*8, 30*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(32+64+2+8)	/* 32 for characters, 64 for stars, 2 for bullets, 8 for background */
	MDRV_GFXDECODE(sfx_gfxdecodeinfo)
	MDRV_PALETTE_INIT(turtles)
	MDRV_VIDEO_START(sfx)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(sfx_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)

	MDRV_SOUND_MODIFY("8910.2")
	MDRV_SOUND_CONFIG(scobra_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mimonscr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mimonscr_readmem,mimonscr_writemem)

	/* video hardware */
	MDRV_VIDEO_START(mimonkey)
MACHINE_DRIVER_END

/* Triple Punch and Mariner are different - only one CPU, one 8910 */
static MACHINE_DRIVER_START( triplep )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(triplep_readport,triplep_writeport)

	MDRV_CPU_REMOVE("audio")

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets */

	MDRV_PALETTE_INIT(galaxian)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(triplep_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_REMOVE("8910.2")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mariner )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(triplep)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+16)	/* 32 for characters, 64 for stars, 2 for bullets, 16 for background */

	MDRV_PALETTE_INIT(mariner)
	MDRV_VIDEO_START(mariner)
MACHINE_DRIVER_END

/* Hunchback replaces the Z80 with a S2650 CPU */
static MACHINE_DRIVER_START( hunchbks )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)
	MDRV_CPU_REPLACE("main", S2650, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(hunchbks_readmem,hunchbks_writemem)
	MDRV_CPU_IO_MAP(hunchbks_readport,0)
	MDRV_CPU_VBLANK_INT(hunchbks_vh_interrupt,1)

	MDRV_VBLANK_DURATION(2500)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32+64+2+0)	/* 32 for characters, 64 for stars, 2 for bullets */

	MDRV_PALETTE_INIT(galaxian)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hncholms )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hunchbks)
	MDRV_CPU_REPLACE("main", S2650, 18432000/6/2/2)

	MDRV_VIDEO_START(scorpion)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( scorpion )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(scramble)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_IO_MAP(scorpion_sound_io,0)

	MDRV_VIDEO_START(scorpion)

	/* sound hardware */
	MDRV_SOUND_MODIFY("8910.1")
	MDRV_SOUND_CONFIG(scorpion_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)

	MDRV_SOUND_MODIFY("8910.2")
	MDRV_SOUND_CONFIG(triplep_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)

	MDRV_SOUND_ADD(AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(scobra_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.16)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ad2083 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(ad2083_map,0)

	MDRV_CPU_ADD(Z80, 14318000/8)	/* 1.78975 MHz */
	MDRV_CPU_PROGRAM_MAP(ad2083_sound_map,0)
	MDRV_CPU_IO_MAP(ad2083_sound_io_map,0)

	MDRV_MACHINE_START(ad2083)
	MDRV_MACHINE_RESET(galaxian)

	MDRV_FRAMES_PER_SECOND(16000.0/132/2)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(ad2083_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+8)	/* 32 for characters, 64 for stars, 2 for bullets, 8 for background */

	MDRV_PALETTE_INIT(turtles)
	MDRV_VIDEO_START(ad2083)
	MDRV_VIDEO_UPDATE(galaxian)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_ADD(AY8910, 14318000/8)
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	MDRV_SOUND_ADD(TMS5110, 640000)
	MDRV_SOUND_CONFIG(tms5110_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s1.2d",         0x0000, 0x0800, CRC(ea35ccaa) SHA1(1dcb375987fe21e0483c27d485c405de53848d61) )
	ROM_LOAD( "s2.2e",         0x0800, 0x0800, CRC(e7bba1b3) SHA1(240877576045fddcc9ff01d97dc78139454ac4f1) )
	ROM_LOAD( "s3.2f",         0x1000, 0x0800, CRC(12d7fc3e) SHA1(a84d191c7be8700f630a83ddad798be9e83b5d55) )
	ROM_LOAD( "s4.2h",         0x1800, 0x0800, CRC(b59360eb) SHA1(5d155808c19dcf2e14aa8e29c0ee41a6d3d3c43a) )
	ROM_LOAD( "s5.2j",         0x2000, 0x0800, CRC(4919a91c) SHA1(9cb5861c61e4783e5fbaa3869d51195f127b1129) )
	ROM_LOAD( "s6.2l",         0x2800, 0x0800, CRC(26a4547b) SHA1(67c0fa81729370631647b5d78bb5a61433facd7f) )
	ROM_LOAD( "s7.2m",         0x3000, 0x0800, CRC(0bb49470) SHA1(05a6fe3010c2136284ca76352dac147797c79778) )
	ROM_LOAD( "s8.2p",         0x3800, 0x0800, CRC(6a5740e5) SHA1(e3b09141cee26857d626412e9d1a0e759469b97a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ot1.5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "ot2.5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "ot3.5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c2.5f",         0x0000, 0x0800, CRC(4708845b) SHA1(a8b1ad19a95a9d35050a2ab7194cc96fc5afcdc9) )
	ROM_LOAD( "c1.5h",         0x0800, 0x0800, CRC(11fd2887) SHA1(69844e48bb4d372cac7ae83c953df573c7ecbb7f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( scrambls )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d",           0x0000, 0x0800, CRC(b89207a1) SHA1(5422df979e82bcc73df49f50515fe76c126c037b) )
	ROM_LOAD( "2e",           0x0800, 0x0800, CRC(e9b4b9eb) SHA1(a8ee9ddfadf5e9accedfaf81da757a88a2e55a0a) )
	ROM_LOAD( "2f",           0x1000, 0x0800, CRC(a1f14f4c) SHA1(3eae2b3e4596505a8afb5c5cfb108e823c2c4319) )
	ROM_LOAD( "2h",           0x1800, 0x0800, CRC(591bc0d9) SHA1(170f9e92f0a3bee04407be27210b4fa825367688) )
	ROM_LOAD( "2j",           0x2000, 0x0800, CRC(22f11b6b) SHA1(e426ef6a7444a39a34d59799973b07d11b89f372) )
	ROM_LOAD( "2l",           0x2800, 0x0800, CRC(705ffe49) SHA1(174df3f281068c767344f751daace646360e26d6) )
	ROM_LOAD( "2m",           0x3000, 0x0800, CRC(ea26c35c) SHA1(a2f3380982d93a022f46756f974fd16c4cd617de) )
	ROM_LOAD( "2p",           0x3800, 0x0800, CRC(94d8f5e3) SHA1(f3a9c4d1d91836476fcad87ea0d243dde7171e0a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ot1.5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "ot2.5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "ot3.5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, CRC(5f30311a) SHA1(d64134089bebd995b3a1a089411e180c8c29f32d) )
	ROM_LOAD( "5h",           0x0800, 0x0800, CRC(516e029e) SHA1(81b44eb1ce43cebde87f0a41ade2e7eb291af78d) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( explorer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "10l.bin",      0x0000, 0x1000, CRC(d5adf626) SHA1(f362322f780c13cee73697f9158a8ca8aa943a2e) )
	ROM_LOAD( "9l.bin",       0x1000, 0x1000, CRC(48e32788) SHA1(7a98848d2ed8ba5b2da28c014226109af7cc9287) )
	ROM_LOAD( "8l.bin",       0x2000, 0x1000, CRC(c0dbdbde) SHA1(eac7444246bdf80f97962031bf900ce09b28c8b5) )
	ROM_LOAD( "7l.bin",       0x3000, 0x1000, CRC(9b30d227) SHA1(22764e0a2a5ce7abe862e42c84abaaf25949575f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "3f.bin",		  0x0000, 0x1000, CRC(9faf18cf) SHA1(1b6c65472d639753cc39031750f85efe1d31ae5e) )
	ROM_LOAD( "4b.bin",       0x1000, 0x0800, CRC(e910b5c3) SHA1(228e8d36dd1ac8a00a396df74b80aa6616997028) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c2.5f",         0x0000, 0x0800, CRC(4708845b) SHA1(a8b1ad19a95a9d35050a2ab7194cc96fc5afcdc9) )
	ROM_LOAD( "c1.5h",         0x0800, 0x0800, CRC(11fd2887) SHA1(69844e48bb4d372cac7ae83c953df573c7ecbb7f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( strfbomb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.2c",         0x0000, 0x0800, CRC(b102aaa0) SHA1(00560da7a2ded6afcdc1d46e12cc3c795654639a) )
	ROM_LOAD( "2.2e",         0x0800, 0x0800, CRC(d4155703) SHA1(defd37df55536890456c29812340e0d6b4292b78) )
	ROM_LOAD( "3.2f",         0x1000, 0x0800, CRC(a9568c89) SHA1(0d8e6b3af92e4933814700d54acfd43407f3ede1) )
	ROM_LOAD( "4.2h",         0x1800, 0x0800, CRC(663b6c35) SHA1(354fb2e92f4376b20aee412ed361d59b8a2c01e1) )
	ROM_LOAD( "5.2j",         0x2000, 0x0800, CRC(4919a91c) SHA1(9cb5861c61e4783e5fbaa3869d51195f127b1129) )
	ROM_LOAD( "6.2l",         0x2800, 0x0800, CRC(4ec66ae3) SHA1(a74827e161212e9b2eddd980321507a377f1e30b) )
	ROM_LOAD( "7.2m",         0x3000, 0x0800, CRC(0feb0192) SHA1(45a44bde3bf1483abf95fe1d1d5066bfcb1736df) )
	ROM_LOAD( "8.2p",         0x3800, 0x0800, CRC(280a6142) SHA1(f17625b91eaaffa36a433be32e4e80651d94b3b9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ot1.5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "ot2.5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "ot3.5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9.5f",         0x0000, 0x0800, CRC(3abeff25) SHA1(ff6de0596c849ec877fb759c1ab9c7a8ffe2edac) )
	ROM_LOAD( "10.5h",        0x0800, 0x0800, CRC(79ecacbe) SHA1(285cb3ee0ff8d596877bb571ea8479566ab36eb9) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( atlantis )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x0800, CRC(0e485b9a) SHA1(976f1d6f4552fbee134359a776b5688588824cbb) )
	ROM_LOAD( "2e",           0x0800, 0x0800, CRC(c1640513) SHA1(a0dfb34f401330b16e9e4d66ec4b49d120499606) )
	ROM_LOAD( "2f",           0x1000, 0x0800, CRC(eec265ee) SHA1(29b6cf6b93220414eb58cddeba591dc8813c4935) )
	ROM_LOAD( "2h",           0x1800, 0x0800, CRC(a5d2e442) SHA1(e535d1a501ebd861ad62da70b87215fb7c23de1d) )
	ROM_LOAD( "2j",           0x2000, 0x0800, CRC(45f7cf34) SHA1(d1e0e0be6dec377b684625bdfdc5a3a8af847492) )
	ROM_LOAD( "2l",           0x2800, 0x0800, CRC(f335b96b) SHA1(17daa6d9bc916081f3c6cbdfe5b4960177dc7c9b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ot1.5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "ot2.5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "ot3.5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, CRC(57f9c6b9) SHA1(ad0d09a6611998d093d676a9c9fe9e32b10f643e) )
	ROM_LOAD( "5h",           0x0800, 0x0800, CRC(e989f325) SHA1(947aee915779687deae040aeef9e9aee680aaebf) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( atlants2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom1",         0x0000, 0x0800, CRC(ad348089) SHA1(3548b94192c451c0126e7aaecefa7137ae074cd3) )
	ROM_LOAD( "rom2",         0x0800, 0x0800, CRC(caa705d1) SHA1(b4aefbea21fa9608e1dae2a09ae0d31270eb8c78) )
	ROM_LOAD( "rom3",         0x1000, 0x0800, CRC(e420641d) SHA1(103e7590f5acbac6991d665495f933c3a68da1c8) )
	ROM_LOAD( "rom4",         0x1800, 0x0800, CRC(04792d90) SHA1(cb477e4b8e4538def01c10b0348f8f8e3a2a9500) )
	ROM_LOAD( "2j",           0x2000, 0x0800, CRC(45f7cf34) SHA1(d1e0e0be6dec377b684625bdfdc5a3a8af847492) )
	ROM_LOAD( "rom6",         0x2800, 0x0800, CRC(b297bd4b) SHA1(0c48da41d9cf2a3456df5b1e8bf27fa641bc643b) )
	ROM_LOAD( "rom7",         0x3000, 0x0800, CRC(a50bf8d5) SHA1(5bca98e1c0838d27ec66bf4b906877977b212b6d) )
	ROM_LOAD( "rom8",         0x3800, 0x0800, CRC(d2c5c984) SHA1(a9432f9aff8a2f5ca1d347443efc008a177d8ae0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ot1.5c",           0x0000, 0x0800, CRC(bcd297f0) SHA1(8ed78487d76fd0a917ab7b258937a46e2cd9800c) )
	ROM_LOAD( "ot2.5d",           0x0800, 0x0800, CRC(de7912da) SHA1(8558b4eff5d7e63029b325edef9914feda5834c3) )
	ROM_LOAD( "ot3.5e",           0x1000, 0x0800, CRC(ba2fa933) SHA1(1f976d8595706730e29f93027e7ab4620075c078) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom9",         0x0000, 0x0800, CRC(55cd5acd) SHA1(b3e2ce71d4e48255d44cd451ee015a7234a108c8) )
	ROM_LOAD( "rom10",        0x0800, 0x0800, CRC(72e773b8) SHA1(6ce178df3bd6a4177c68761572a13a56d222c48f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( theend )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13_1t.bin",  0x0000, 0x0800, CRC(93e555ba) SHA1(f684927cecabfbd7544f7549a6152c0a6a436019) )
	ROM_LOAD( "ic14_2t.bin",  0x0800, 0x0800, CRC(2de7ad27) SHA1(caf369fde632652a0a5fb11d3605f0d2386d297a) )
	ROM_LOAD( "ic15_3t.bin",  0x1000, 0x0800, CRC(035f750b) SHA1(5f70518e5dbfca0ba12ba4dc4f357ce8e6b27bc8) )
	ROM_LOAD( "ic16_4t.bin",  0x1800, 0x0800, CRC(61286b5c) SHA1(14464aa5284aecc9c6046e464ab3d13da89d8dda) )
	ROM_LOAD( "ic17_5t.bin",  0x2000, 0x0800, CRC(434a8f68) SHA1(3c8c099c7865997d475c096f1b1c93d88ab21543) )
	ROM_LOAD( "ic18_6t.bin",  0x2800, 0x0800, CRC(dc4cc786) SHA1(3311361a1eb29715aa41d61fbb3563014bd9eeb1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56_1.bin",   0x0000, 0x0800, CRC(7a141f29) SHA1(ca483943971c8fc7f5775a8a7cc6ddd331d48170) )
	ROM_LOAD( "ic55_2.bin",   0x0800, 0x0800, CRC(218497c1) SHA1(3e080621f2e83909a6f304a2d960a080bccbbdc2) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30_2c.bin",  0x0000, 0x0800, CRC(68ccf7bf) SHA1(a8ea784a2660f855757ae0b30cb2a33ab6f2cd59) )
	ROM_LOAD( "ic31_1c.bin",  0x0800, 0x0800, CRC(4a48c999) SHA1(f1abcbfc3146a18dc3ff865e3ba278377a42a875) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d) )
ROM_END

ROM_START( theends )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13",         0x0000, 0x0800, CRC(90e5ab14) SHA1(b926801ab1cc1e2787a76ced6c7cffd6fce753d4) )
	ROM_LOAD( "ic14",         0x0800, 0x0800, CRC(950f0a07) SHA1(bde9f3c6cf060dc6f5b7652287b94e04bed7bcf7) )
	ROM_LOAD( "ic15",         0x1000, 0x0800, CRC(6786bcf5) SHA1(7556d3dc51d6a112b6357b8a36df05fd1a4d1cc9) )
	ROM_LOAD( "ic16",         0x1800, 0x0800, CRC(380a0017) SHA1(3354eb328a32537f722fe8a0949ddcab6cf21eb8) )
	ROM_LOAD( "ic17",         0x2000, 0x0800, CRC(af067b7f) SHA1(855c6ddf29fbfea004c7143fe29064abf53801ad) )
	ROM_LOAD( "ic18",         0x2800, 0x0800, CRC(a0411b93) SHA1(d644968758a1b73d13e09b24d24bfec82276e8f4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56",         0x0000, 0x0800, CRC(3b2c2f70) SHA1(bcccdacacfc9a3b5f1412dfba6bb0046d283bccc) )
	ROM_LOAD( "ic55",         0x0800, 0x0800, CRC(e0429e50) SHA1(27678fc3172cbca3ae1eae96e9d8a62561d5ce40) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30",         0x0000, 0x0800, CRC(527fd384) SHA1(92a384899d5acd2c689f637da16a0e2d11a9d9c6) )
	ROM_LOAD( "ic31",         0x0800, 0x0800, CRC(af6d09b6) SHA1(f3ad51dc88aa58fd39195ead978b039e0b0b585c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, CRC(24652bc4) SHA1(d89575f3749c75dc963317fe451ffeffd9856e4d) )
ROM_END

ROM_START( froggers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_d2.bin",   0x0000, 0x0800, CRC(c103066e) SHA1(8c2d4c825e9c4180fe70b0db18a547dc3ddc3c2c) )
	ROM_LOAD( "vid_e2.bin",   0x0800, 0x0800, CRC(f08bc094) SHA1(23ad1e57f244d6b63fd9640249dcb1eeafb8206e) )
	ROM_LOAD( "vid_f2.bin",   0x1000, 0x0800, CRC(637a2ff8) SHA1(e9b9fc692ca5d8deb9cd30d9d73ad25c8d8bafe1) )
	ROM_LOAD( "vid_h2.bin",   0x1800, 0x0800, CRC(04c027a5) SHA1(193550731513c02cad464661a1ceb230819ca70f) )
	ROM_LOAD( "vid_j2.bin",   0x2000, 0x0800, CRC(fbdfbe74) SHA1(48d5d1247d09eaea2a9a29f4ed6543d0411597aa) )
	ROM_LOAD( "vid_l2.bin",   0x2800, 0x0800, CRC(8a4389e1) SHA1(b2c74afb93927dac0d8bb24e02e0b2a069f2d3c8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )
	ROM_LOAD( "epr-1036.1k",  0x0800, 0x0800, CRC(658745f8) SHA1(e4e5c3e011c8a7233a36d29e10e08905873500aa) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, CRC(413703bf) SHA1(66648b2b28d3dcbda5bdb2605d1977428939dd3c) )
ROM_END

ROM_START( frogf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6.bin",        0x0000, 0x1000, CRC(8ff0a973) SHA1(adb1c28617d915fbcfa9190bd8589a56a8858e25) )
	ROM_LOAD( "7.bin",        0x1000, 0x1000, CRC(3087bb4b) SHA1(3fe1f68a2ad12b1cadba89d99afe574cf5342d81) )
	ROM_LOAD( "8.bin",        0x2000, 0x1000, CRC(c3869d12) SHA1(7bd95c12fc1fe1a3cfc0140b64cf76fa57aa3fb4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, CRC(e8ab0256) SHA1(f090afcfacf5f13cdfa0dfda8e3feb868c6ce8bc) )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, CRC(7380a48f) SHA1(75582a94b696062cbdb66a4c5cf0bc0bb94f81ee) )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, CRC(31d7eb27) SHA1(2e1d34ae4da385fd7cac94707d25eeddf4604e1a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, CRC(05f7d883) SHA1(78831fd287da18928651a8adb7e578d291493eff) )
	ROM_LOAD( "epr-1036.1k",  0x0800, 0x0800, CRC(658745f8) SHA1(e4e5c3e011c8a7233a36d29e10e08905873500aa) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, CRC(413703bf) SHA1(66648b2b28d3dcbda5bdb2605d1977428939dd3c) )
ROM_END

ROM_START( amidars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "am2d",         0x0000, 0x0800, CRC(24b79547) SHA1(eca735c6a35561a9a6ba8a20dca1e1c78ed073fc) )
	ROM_LOAD( "am2e",         0x0800, 0x0800, CRC(4c64161e) SHA1(5b2e49ff915295617671b13f15b566046a5dbc15) )
	ROM_LOAD( "am2f",         0x1000, 0x0800, CRC(b3987a72) SHA1(1d72e9ae3005029628c6f9beb6ca65afcb1f7893) )
	ROM_LOAD( "am2h",         0x1800, 0x0800, CRC(29873461) SHA1(7d0ee9a82f02163b4cc6a7097e88ae34e96ebf58) )
	ROM_LOAD( "am2j",         0x2000, 0x0800, CRC(0fdd54d8) SHA1(c32fdc8e292d91159e6c80c7033abea6404a4f2c) )
	ROM_LOAD( "am2l",         0x2800, 0x0800, CRC(5382f7ed) SHA1(425ec2c2caf404fc8ab13ee38d6567413022e1a1) )
	ROM_LOAD( "am2m",         0x3000, 0x0800, CRC(1d7109e9) SHA1(e0d24475547bbe5a94b45be6abefb84ad84d2534) )
	ROM_LOAD( "am2p",         0x3800, 0x0800, CRC(c9163ac6) SHA1(46d757180426b71c827d14a35824a248f2c787b6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c",  0x0000, 0x1000, CRC(8ca7b750) SHA1(4f4c2915503b85abe141d717fd254ee10c9da99e) )
	ROM_LOAD( "amidarus.5d",  0x1000, 0x1000, CRC(9b5bdc0a) SHA1(84d953618c8bf510d23b42232a856ac55f1baff5) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2716.a6",      0x0000, 0x0800, CRC(2082ad0a) SHA1(c6014d9575e92adf09b0961c2158a779ebe940c4) )   /* Same graphics ROMs as Amigo */
	ROM_LOAD( "2716.a5",      0x0800, 0x0800, CRC(3029f94f) SHA1(3b432b42e79f8b0a7d65e197f373a04e3c92ff20) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "amidar.clr",   0x0000, 0x0020, CRC(f940dcc3) SHA1(1015e56f37c244a850a8f4bf0e36668f047fd46d) )
ROM_END

ROM_START( triplep )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "triplep.2g",   0x0000, 0x1000, CRC(c583a93d) SHA1(2bd4a02f945d64ef3ff814d0b8cbf32380d3f790) )
	ROM_LOAD( "triplep.2h",   0x1000, 0x1000, CRC(c03ddc49) SHA1(5a2fba848c4ddf2ef0bb0f00e93dbd88e939441a) )
	ROM_LOAD( "triplep.2k",   0x2000, 0x1000, CRC(e83ca6b5) SHA1(b16690cfe6fb45cf7b9a9cfa22822ba947c0e432) )
	ROM_LOAD( "triplep.2l",   0x3000, 0x1000, CRC(982cc3b9) SHA1(28bb08679126e5aa8bd0b8f387b881fe799fb009) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, CRC(d51cbd6f) SHA1(c3766a69a4599e54b8d7fb893e45802ec8bf6713) )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, CRC(f21c0059) SHA1(b1ba87f13908e3e662de8bf444f59bd5c2009720) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, CRC(624f75df) SHA1(0e9a7c48dd976af1dca1d5351236d4d5bf7a9dc8) )
ROM_END

ROM_START( knockout )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "knockout.2h",  0x0000, 0x1000, CRC(eaaa848e) SHA1(661026567db87206200ee610c3d5f5eb725aeec9) )
	ROM_LOAD( "knockout.2k",  0x1000, 0x1000, CRC(bc26d2c0) SHA1(b9934ddb2918f6c4123dafd07cc39ae31d7e28e9) )
	ROM_LOAD( "knockout.2l",  0x2000, 0x1000, CRC(02025c10) SHA1(16ffc7681d949172034b8c85dc72c1a528309abf) )
	ROM_LOAD( "knockout.2m",  0x3000, 0x1000, CRC(e9abc42b) SHA1(93b9c55a76e273b4709ee65870c0848a0d3db7cc) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, CRC(d51cbd6f) SHA1(c3766a69a4599e54b8d7fb893e45802ec8bf6713) )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, CRC(f21c0059) SHA1(b1ba87f13908e3e662de8bf444f59bd5c2009720) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, CRC(624f75df) SHA1(0e9a7c48dd976af1dca1d5351236d4d5bf7a9dc8) )
ROM_END

ROM_START( mariner )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tp1.2h",       0x0000, 0x1000, CRC(dac1dfd0) SHA1(57b9106bb7452640544ba0ab2d2ba290cccb45f0) )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, CRC(efe7ca28) SHA1(496f8eb2ebc9edeed5b19d87f437f23bbeb2a007) )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, CRC(027881a6) SHA1(47953aa5140a157ade484341609d477510e8342b) )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, CRC(a0fde7dc) SHA1(ea6700520b1bd31e6c6bfac6f067bbf652676eef) )
	ROM_LOAD( "tm5.2p",       0x6000, 0x0800, CRC(d7ebcb8e) SHA1(bddefdc5f04c2f940e08a6968fbd6f930d16b8e4) )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, CRC(70ae611f) SHA1(2686dc6d3910bd58b290d6296f30c552686709f5) )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, CRC(8e4e999e) SHA1(195e6896ca2f3175137d8c92777ba32c41e835d3) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, CRC(ca42b6dd) SHA1(d1e224e788e3dcf57249e72f03f9fe3fd71e6c12) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, CRC(ad208ccc) SHA1(66a4122e46467344a7f3ddcc953a5f7f451411fa) )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, CRC(1bd88cff) SHA1(8d1620386ef654d99c51e489c822eeb2e8a4fe76) )	/* char banking and star placement */
ROM_END

ROM_START( 800fath )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tu1.2h",       0x0000, 0x1000, CRC(5dd3d42f) SHA1(887403c385897044e1cb9709ab2b6ff5abdf9eb4) )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, CRC(efe7ca28) SHA1(496f8eb2ebc9edeed5b19d87f437f23bbeb2a007) )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, CRC(027881a6) SHA1(47953aa5140a157ade484341609d477510e8342b) )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, CRC(a0fde7dc) SHA1(ea6700520b1bd31e6c6bfac6f067bbf652676eef) )
	ROM_LOAD( "tu5.2p",       0x6000, 0x0800, CRC(f864a8a6) SHA1(bd0c84284d13d099da4e139db7c9948a074d6774) )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, CRC(70ae611f) SHA1(2686dc6d3910bd58b290d6296f30c552686709f5) )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, CRC(8e4e999e) SHA1(195e6896ca2f3175137d8c92777ba32c41e835d3) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, CRC(ca42b6dd) SHA1(d1e224e788e3dcf57249e72f03f9fe3fd71e6c12) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, CRC(ad208ccc) SHA1(66a4122e46467344a7f3ddcc953a5f7f451411fa) )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, CRC(1bd88cff) SHA1(8d1620386ef654d99c51e489c822eeb2e8a4fe76) )	/* char banking and star placement */
ROM_END

ROM_START( ckongs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x1000, CRC(49a8c234) SHA1(91d8da03a76094b6fed4bf1d9a3943dee72bf039) )
	ROM_LOAD( "vid_2e.bin",   0x1000, 0x1000, CRC(f1b667f1) SHA1(c09e0f3b70afd5a4b6ec47ac9237f278dff75783) )
	ROM_LOAD( "vid_2f.bin",   0x2000, 0x1000, CRC(b194b75d) SHA1(514b195dd02a7324e439dd63ae654af117e0c70d) )
	ROM_LOAD( "vid_2h.bin",   0x3000, 0x1000, CRC(2052ba8a) SHA1(e4200219d1a142a4aba8ef21ae1dd806400f4422) )
	ROM_LOAD( "vid_2j.bin",   0x4000, 0x1000, CRC(b377afd0) SHA1(8e42e7623a1749cea1c9861cd7dfa9b97571dc8b) )
	ROM_LOAD( "vid_2l.bin",   0x5000, 0x1000, CRC(fe65e691) SHA1(736fe70c9adc6d2c142fa876f1a1e3c6879eccd8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c",  0x0000, 0x1000, CRC(f0c30f9a) SHA1(5621f336e9be8acf986a34bbb8855ed5d45c28ef) )
	ROM_LOAD( "snd_5d.bin",   0x1000, 0x1000, CRC(892c9547) SHA1(c3ec98049b560eb0ddefdb1e1b2d551b418b9a1c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_5f.bin",   0x0000, 0x1000, CRC(7866d2cb) SHA1(62dd8b80bc0459c7337d8a8cb83e53b999e7f4a9) )
	ROM_LOAD( "vid_5h.bin",   0x1000, 0x1000, CRC(7311a101) SHA1(49d54c8b94cae4ba81d7a7684eaa4e87815bb4da) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "vid_6e.bin",   0x0000, 0x0020, CRC(5039af97) SHA1(b1a5b32b8c944bf19d9d97aaf678726df003c194) )
ROM_END

ROM_START( mars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.3",        0x0000, 0x0800, CRC(2f88892c) SHA1(580c7b502321868f63d9e67286e63b5c5268827c) )
	ROM_LOAD( "u56.4",        0x0800, 0x0800, CRC(9e6bcbf7) SHA1(c3acdba073a1f3703776a7905867d81acf328f37) )
	ROM_LOAD( "u69.5",        0x1000, 0x0800, CRC(df496e6e) SHA1(b597e996ac797a239e4bc8f242f59c98a850723c) )
	ROM_LOAD( "u98.6",        0x1800, 0x0800, CRC(75f274bb) SHA1(2eb83ccc8404c69ab262bf680dce892c23c94f39) )
	ROM_LOAD( "u114.7",       0x2000, 0x0800, CRC(497fd8d0) SHA1(545aaf1d68ff727df356bbcf8ddd23df75b5ce97) )
	ROM_LOAD( "u133.8",       0x2800, 0x0800, CRC(3d4cd59f) SHA1(da96d96a40a896e1272700c50cc34f91ac9f7a23) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, CRC(bb5968b9) SHA1(8bc57fd80da7aff294e12e991e9acf60c1ab2893) )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, CRC(75fd7720) SHA1(3ee4ca0d85ffacf0388cada17581da0cdaaf83ef) )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, CRC(72a492da) SHA1(a272f72378850f7ecf52498746c2015f6dad3ab9) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.1",        0x0000, 0x0800, CRC(279789d0) SHA1(3ccf39da252df8b3605efc26299831279d697dd8) )
	ROM_LOAD( "u101.2",       0x0800, 0x0800, CRC(c5dc627f) SHA1(529307238707d0676d1cae508f4eb66bbdd623d7) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( devilfsh )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.1",        0x0000, 0x0800, CRC(ec047d71) SHA1(c35555010fe239213e92946b65a54612d5a23399) )
	ROM_LOAD( "u56.2",        0x0800, 0x0800, CRC(0138ade9) SHA1(8c75572fa0a5d0665cc46b1c0080192bb0148df9) )
	ROM_LOAD( "u69.3",        0x1000, 0x0800, CRC(5dd0b3fc) SHA1(cf19193986920853d93e4ff38b70dcd46b42276b) )
	ROM_LOAD( "u98.4",        0x1800, 0x0800, CRC(ded0b745) SHA1(2a3c741f11d211b4ec8dfa2dd2b3ae0c0a2d9590) )
	ROM_LOAD( "u114.5",       0x2000, 0x0800, CRC(5fd40176) SHA1(536dd870057c48591f0bee468325c8780afb7026) )
	ROM_LOAD( "u133.6",       0x2800, 0x0800, CRC(03538336) SHA1(66cffcbc53e42c626880151a62721ef3e6bf90bc) )
	ROM_LOAD( "u143.7",       0x3000, 0x0800, CRC(64676081) SHA1(6ae628ad582680d6a825238b60d1195990dbb56f) )
	ROM_LOAD( "u163.8",       0x3800, 0x0800, CRC(bc3d6770) SHA1(ac8803520a668b1759acba23f06ba7a6c5792cbd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, CRC(09987e2e) SHA1(6b0d8413a137726a67ceedeacfc0c48b058698f1) )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, CRC(1e2b1471) SHA1(3ba33b26a5bff21b9ddc34e0a468db7b89361314) )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, CRC(45279aaa) SHA1(20aca9bcfa1010311290cb3d8e4fb548182dcd4b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.12",       0x0000, 0x1000, CRC(5406508e) SHA1(1d01a796c3ac22e3fddd1ec2103ef5bd8c409278) )
	ROM_LOAD( "u101.13",      0x1000, 0x1000, CRC(8c4018b6) SHA1(83c4f73a3e40fa6ecece38404fa5f64ab59d7b4e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( newsin7 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "newsin.1",     0x0000, 0x1000, CRC(e6c23fe0) SHA1(b15c56ca7868be0f038c246f29ba54f41b5cb755) )
	ROM_LOAD( "newsin.2",     0x1000, 0x1000, CRC(3d477b5f) SHA1(9e22f7262077ce6b00b2efbba0e13dcec143d122) )
	ROM_LOAD( "newsin.3",     0x2000, 0x1000, CRC(7dfa9af0) SHA1(8cbbfff22a3c6429f7ab22d86c8d760b08871bac) )
	ROM_LOAD( "newsin.4",     0x3000, 0x1000, CRC(d1b0ba19) SHA1(66c128bc9b306aa6470de5d413f782ae17e78b14) )
	ROM_LOAD( "newsin.5",     0xa000, 0x1000, CRC(06275d59) SHA1(a5a5436c0b014af06181eeb044d8c4e3188414ea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "newsin.13",    0x0000, 0x0800, CRC(d88489a2) SHA1(ab10fd4862129824301a0a847de298f1826aa03e) )
	ROM_LOAD( "newsin.12",    0x0800, 0x0800, CRC(b154a7af) SHA1(91ca28b2530f22786ff581e5710b40f0cf99f516) )
	ROM_LOAD( "newsin.11",    0x1000, 0x0800, CRC(7ade709b) SHA1(bda1401172139cd6e3e03424c56e4f59e5afebd5) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "newsin.7",     0x2000, 0x1000, CRC(6bc5d64f) SHA1(4f52224a4d5294a7487a1fc55eba13cf0b5fb6af) )
	ROM_LOAD( "newsin.8",     0x1000, 0x1000, CRC(0c5b895a) SHA1(994ad7f051b30a3045ffc08ac8d9d7092fbadef3) )
	ROM_LOAD( "newsin.9",     0x0000, 0x1000, CRC(6b87adff) SHA1(c0943832e498ab04978b11b163ba951d4a7e2e60) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "newsin.6",     0x0000, 0x0020, CRC(5cf2cd8d) SHA1(0c85737add75545ab11aaf64fe37c7bd078308c9) )
ROM_END

ROM_START( mrkougar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2732-7.bin",   0x0000, 0x1000, CRC(fd060ffb) SHA1(b3bee6fe879f13f3178bef3b2dff3041e698f061) )
	ROM_LOAD( "2732-6.bin",   0x1000, 0x1000, CRC(9e05d868) SHA1(802514f5de347913f0315b42b3689baa37030141) )
	ROM_LOAD( "2732-5.bin",   0x2000, 0x1000, CRC(cbc7c536) SHA1(b959c29bb7ab81ee123ba2f397eef1e32656b441) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2732-1.bin",   0x0000, 0x1000, CRC(60ef1d43) SHA1(ab42fa98350051526fcc4bfe35ebed3d6daf424f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( mrkougr2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "atw-7l-7.bin", 0x0000, 0x1000, CRC(7b34b198) SHA1(c7793c49c5bd1360ef2d419bc4710b35f0a02760) )
	ROM_LOAD( "atw-7k-6.bin", 0x1000, 0x1000, CRC(fbca23c7) SHA1(da24a01d83174bad36072d4bf6764c5a3e242561) )
	ROM_LOAD( "atw-7h-5.bin", 0x2000, 0x1000, CRC(05b257a2) SHA1(728df1f1cb726d333818db8fedb27bf537be8a36) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "atw-1h-1.bin", 0x0000, 0x1000, CRC(38fdfb63) SHA1(9fc4eafd6d106ffe35c179e59a234c706c489f8c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "atw-prom.bin", 0x0000, 0x0020, CRC(c65db188) SHA1(90f0a5f22bb761693ab5895da08b20821e79ba65) )
ROM_END

ROM_START( mrkougb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p01.bin",	  0x0000, 0x0800, CRC(dea0cde1) SHA1(aaf9c622b86d475a90f91628d033989e72dda361) )
	ROM_LOAD( "p02.bin",	  0x0800, 0x0800, CRC(c8017751) SHA1(021bd6a6efb90119767162a5847b4bbbc47f321e) )
	ROM_LOAD( "p03.bin",	  0x1000, 0x0800, CRC(b8921984) SHA1(1adccd2bad8f748995f844183cac487ad00dd71e) )
	ROM_LOAD( "p04.bin",	  0x1800, 0x0800, CRC(b3c9754c) SHA1(16a162a19079125fa01f49d90dbf8cd61b9b4833) )
	ROM_LOAD( "p05.bin",	  0x2000, 0x0800, CRC(8d94adbc) SHA1(ac5932c84864e08c6b7937ef20d5bdceb48e2d24) )
	ROM_LOAD( "p06.bin",	  0x2800, 0x0800, CRC(acc921ff) SHA1(f75158c62c6b9871ef05a6a97542469698100eb0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g07.bin",      0x0000, 0x0800, CRC(0ecfd116) SHA1(0ea173c4f7f2613ef71ee5dcd52c4c6f640020b7) )
	ROM_LOAD( "g08.bin",      0x0800, 0x0800, CRC(00bfa3c6) SHA1(57a7fc48ec740b72baece96d50380dbbc826af77) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "atw-prom.bin", 0x0000, 0x0020, CRC(c65db188) SHA1(90f0a5f22bb761693ab5895da08b20821e79ba65) )
ROM_END

ROM_START( mrkougb2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mrk1.bin",	  0x0000, 0x0800, CRC(fc93acb9) SHA1(e53373d47959a99f0b6574654d198f43b493c20f) )
	ROM_LOAD( "p02.bin",	  0x0800, 0x0800, CRC(c8017751) SHA1(021bd6a6efb90119767162a5847b4bbbc47f321e) )
	ROM_LOAD( "p03.bin",	  0x1000, 0x0800, CRC(b8921984) SHA1(1adccd2bad8f748995f844183cac487ad00dd71e) )
	ROM_LOAD( "p04.bin",	  0x1800, 0x0800, CRC(b3c9754c) SHA1(16a162a19079125fa01f49d90dbf8cd61b9b4833) )
	ROM_LOAD( "p05.bin",	  0x2000, 0x0800, CRC(8d94adbc) SHA1(ac5932c84864e08c6b7937ef20d5bdceb48e2d24) )
	ROM_LOAD( "p06.bin",	  0x2800, 0x0800, CRC(acc921ff) SHA1(f75158c62c6b9871ef05a6a97542469698100eb0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "atw-6w-2.bin", 0x0000, 0x1000, CRC(af42a371) SHA1(edacbb29df34fdf400a5c726d851af1479a34c70) )
	ROM_LOAD( "atw-6y-3.bin", 0x1000, 0x1000, CRC(862b8902) SHA1(91dcbc634f7c7ed78dfbd0be5cf1e0631429cfbf) )
	ROM_LOAD( "atw-6z-4.bin", 0x2000, 0x1000, CRC(a0396cc8) SHA1(c8266b58b144a4bc564f3a2503d5b953c0ba6ca7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g07.bin",      0x0000, 0x0800, CRC(0ecfd116) SHA1(0ea173c4f7f2613ef71ee5dcd52c4c6f640020b7) )
	ROM_LOAD( "g08.bin",      0x0800, 0x0800, CRC(00bfa3c6) SHA1(57a7fc48ec740b72baece96d50380dbbc826af77) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "atw-prom.bin", 0x0000, 0x0020, CRC(c65db188) SHA1(90f0a5f22bb761693ab5895da08b20821e79ba65) )
ROM_END

ROM_START( hotshock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hotshock.l10", 0x0000, 0x1000, CRC(401078f7) SHA1(d4415a41eba1d3a2dcbb119f3136c177b02d1fb6) )
	ROM_LOAD( "hotshock.l9",  0x1000, 0x1000, CRC(af76c237) SHA1(bb54e1652a2d2e56731434ed85b40dab4aad91c9) )
	ROM_LOAD( "hotshock.l8",  0x2000, 0x1000, CRC(30486031) SHA1(bed06cb62afee6b000a0e21927559ac5d3538b38) )
	ROM_LOAD( "hotshock.l7",  0x3000, 0x1000, CRC(5bde9312) SHA1(d3ba06790c8210f41902bb8ad27a1e5abafccb33) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hotshock.b3",  0x0000, 0x1000, CRC(0092f0e2) SHA1(d85b05370fa0ce4ba27fd331bb6a7fae067ce83b) )
	ROM_LOAD( "hotshock.b4",  0x1000, 0x1000, CRC(c2135a44) SHA1(809cf305b1f43f99f2248020c369fb5f1d7c5c44) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hotshock.h4",  0x0000, 0x1000, CRC(60bdaea9) SHA1(fd10109803661dc1ce72e1291e3721bdb2bb159f) )
	ROM_LOAD( "hotshock.h5",  0x1000, 0x1000, CRC(4ef17453) SHA1(7dc58456b2f25775c85b3ae92f605d69bb68d590) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

/*

Conqueror (c) ???? ????

CPU: Z80 (x2)
Sound: AY-3-8910 (x2)
RAM: 74S201 (x5), 2114 (x8), Mostek MN4801AN-3IRL
X1: ??

Notes: Has a very crude, homemade looking potted module which crumbled apart when handled.
Contained two 20-pin DIP chips with no markings. Could be PROMs, PLDs or TTL

*/

ROM_START( conquer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "conquer3.l10",       0x0000, 0x1000, BAD_DUMP CRC(a33a824f) SHA1(787ac1f1942ba97c64317b9455d6788281c02f60) )
	ROM_LOAD( "conquer2.l9",        0x1000, 0x1000, CRC(3ffa8285) SHA1(a110e52fe5f637606c1be3a9e290fc6625b9aa48) )
	ROM_LOAD( "conquer1.l8",        0x2000, 0x1000, CRC(9ded2dff) SHA1(9364195d3f86e55df5ecf90d53041517c3658388) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound code */
	ROM_LOAD( "conquer6.b3",       0x0000, 0x1000, CRC(d363b2ea) SHA1(ca4887d51eee4053cd981b4a97fb8a29eecd14e9) )
	ROM_LOAD( "conquer7.b4",       0x1000, 0x1000, CRC(e6a63d71) SHA1(84e199cd214046829f038bc9f151373e9ced575c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "conquer4.h3",       0x0000, 0x1000, CRC(ac533893) SHA1(bb1fee3ec1b856423aa032a905c90a62f405bba8) )
	ROM_LOAD( "conquer5.h5",       0x1000, 0x1000, CRC(d884fd49) SHA1(108ed4a1aebd20b2c44e0bf07c2144b9b58dbda1) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( hunchbks )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "2c_hb01.bin",  0x0000, 0x0800, CRC(8bebd834) SHA1(08f2ce732d2d8754bf559260e1f656a33e2a06a5) )
	ROM_LOAD( "2e_hb02.bin",  0x0800, 0x0800, CRC(07de4229) SHA1(9f333509ae3d6c579f6d96caa172a0abe9eefb30) )
	ROM_LOAD( "2f_hb03.bin",  0x2000, 0x0800, CRC(b75a0dfc) SHA1(c60c833f28c6de027d46f5a2a54ad5646ec58453) )
	ROM_LOAD( "2h_hb04.bin",  0x2800, 0x0800, CRC(f3206264) SHA1(36a614db3fda4f97cc085d84bd13ea44969de95b) )
	ROM_LOAD( "2j_hb05.bin",  0x4000, 0x0800, CRC(1bb78728) SHA1(aebfca355d937825217d069689f9b4d7a113b10a) )
	ROM_LOAD( "2l_hb06.bin",  0x4800, 0x0800, CRC(f25ed680) SHA1(7854e4975a4f75916f60749ac24147c335927394) )
	ROM_LOAD( "2m_hb07.bin",  0x6000, 0x0800, CRC(c72e0e17) SHA1(90da1e375733873bc592e11980bdaf8168bd5aea) )
	ROM_LOAD( "2p_hb08.bin",  0x6800, 0x0800, CRC(412087b0) SHA1(4d6f343577ae73031f32cda8903c74e5a840e71d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "11d_snd.bin",  0x0000, 0x0800, CRC(88226086) SHA1(fe2da172313063e5b056fc8c8d8b2a5c64db5179) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f_hb09.bin",  0x0000, 0x0800, CRC(db489c3d) SHA1(df08607ad07222c1c1c4b3589b50b785bdeefbf2) )
	ROM_LOAD( "5h_hb10.bin",  0x0800, 0x0800, CRC(3977650e) SHA1(1de05d6ceed3f2ed0925caa8235b63a93f03f61e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6e_prom.bin",  0x0000, 0x0020, CRC(01004d3f) SHA1(e53cbc54ea96e846481a67bbcccf6b1726e70f9c) )
ROM_END

ROM_START( hncholms )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "hncholym.2d",  0x0000, 0x0800, CRC(fb453f9c) SHA1(e4c059b10af1aa8405958c0fd139fb84d08ec9f3) )
	ROM_LOAD( "hncholym.2e",  0x0800, 0x0800, CRC(b1429420) SHA1(9e393750e5651c8b14acc11e3591db0a0a599a4d) )
	ROM_LOAD( "hncholym.2f",  0x2000, 0x0800, CRC(afc98e28) SHA1(efc7918a95d9011cbc0c5fbaee3793d95ecbcf89) )
	ROM_LOAD( "hncholym.2h",  0x2800, 0x0800, CRC(6785bf17) SHA1(e0dadda7d55d2046312a87ed700654952662a6b3) )
	ROM_LOAD( "hncholym.2j",  0x4000, 0x0800, CRC(0e1e4133) SHA1(84c3b8e3e81f6ef3311f1272ee6633cec10b796e) )
	ROM_LOAD( "hncholym.2l",  0x4800, 0x0800, CRC(6e982609) SHA1(2d2aa16ad27f6de486eebfd82b23f7ac706faee5) )
	ROM_LOAD( "hncholym.2m",  0x6000, 0x0800, CRC(b9141914) SHA1(955f7b909b3ec27a07817d031fcbb4029e1cff81) )
	ROM_LOAD( "hncholym.2p",  0x6800, 0x0800, CRC(ca37b55b) SHA1(cb423c1aac91654657e72d0a0cbc311cffc9df0c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hncholym.5c",  0x0000, 0x0800, CRC(e7758775) SHA1(3ca843e7519d7f38812e6e2e7b5bb78ac3c02676) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hncholym.5f",  0x0000, 0x1000, CRC(75ad3542) SHA1(1094a30861c68c1f4fc85fbfd5606c5feec3843b) )
	ROM_LOAD( "hncholym.5h",  0x1000, 0x1000, CRC(6fec9dd3) SHA1(2366b10e8f9ba58a565ef2e1a6eddf7c4b51fe79) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "prom.6e",      0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )

	ROM_REGION( 0x0200, REGION_USER1, 0 ) /* unknown - from the custom module */
	ROM_LOAD( "82s147.1a",    0x0000, 0x0200, CRC(d461a48b) SHA1(832fc1de4875d5f19e53d72ccf5dcdb5bcbee1af) )
ROM_END

ROM_START( cavelon )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k + 16K banked for code */
	ROM_LOAD( "2.bin",		 0x00000, 0x2000, CRC(a3b353ac) SHA1(1d5cc402f83c410f2ccd186dafb8bf16a7778fb0) )
	ROM_LOAD( "1.bin",		 0x02000, 0x2000, CRC(3f62efd6) SHA1(b03a46f8478f499812c5d9c11816ee28d67fb77b) )
	ROM_RELOAD(				 0x12000, 0x2000)
	ROM_LOAD( "3.bin",		 0x10000, 0x2000, CRC(39d74e4e) SHA1(4789eab2741555f59a97ef5a10b0500f6b64a6ce) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1c_snd.bin",	  0x0000, 0x0800, CRC(f58dcf55) SHA1(517dab8684109188d7d78c8a2cf94a4fac17d40c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "h.bin",		  0x0000, 0x1000, CRC(d44fcd6f) SHA1(c275741bb1d876e7308e131cac2f1fee249613c7) )
	ROM_LOAD( "k.bin",		  0x1000, 0x1000, CRC(59bc7f9e) SHA1(4374955d0fdfbba57ba432da22b0d94b66832fb8) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "cavelon.clr",  0x0000, 0x0020, CRC(d133356b) SHA1(58db4013a9ad77107f0d462c96363d7c38d86fa2) )
ROM_END

ROM_START( sfx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sfx_b-0.1j",   0x0000, 0x1000, CRC(e5bc6952) SHA1(7bfb772418d738d3c49fd59c0bfc04590945977a) )
	ROM_CONTINUE(             0xe000, 0x1000             )
	ROM_LOAD( "1.1c",         0x1000, 0x1000, CRC(1b3c48e7) SHA1(2f245aaf9b4bb5d949aae18ee89a0be639e7b2df) )
	ROM_LOAD( "22.1d",        0x2000, 0x1000, CRC(ed44950d) SHA1(f8c54ff89ac461171df951d703d5571be1b8da38) )
	ROM_LOAD( "23.1e",        0x3000, 0x1000, CRC(f44a3ca0) SHA1(3917ea960329a06d3d0c447cb6a4ba710fb7ca92) )
	ROM_LOAD( "27.1a",        0x7000, 0x1000, CRC(ed86839f) SHA1(a0d8c941a6e01058eab66d5da9b49b6b5695b981) )
	ROM_LOAD( "24.1g",        0xc000, 0x1000, CRC(e6d7dc74) SHA1(c1e6d9598fb837775ee6550fea3cd4910572615e) )
	ROM_LOAD( "5.1h",         0xd000, 0x1000, CRC(d1e8d390) SHA1(f8fe9f69e6500fbcf25f8151c1070d9a1a20a38c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5.5j",         0x0000, 0x1000, CRC(59028fb6) SHA1(94105b5b03c81a948a409f7ea20312bb9c79c150) )
	ROM_LOAD( "6.6j",         0x1000, 0x1000, CRC(5427670f) SHA1(ffc3f7186d0319f0fd7ed25eb97bb0db7bc107c6) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the sample CPU */
	ROM_LOAD( "1.1j",         0x0000, 0x1000, CRC(2f172c58) SHA1(4706d55fcfad4d5a87d96a0a0187f59997ef9720) )
	ROM_LOAD( "2.2j",         0x1000, 0x1000, CRC(a6ad2f6b) SHA1(14d1a93e507c349b14a1b26408cce23f089fa33c) )
	ROM_LOAD( "3.3j",         0x2000, 0x1000, CRC(fa1274fa) SHA1(e98cb602b265b209eaa4a9b3972e47c869ff863b) )
	ROM_LOAD( "4.4j",         0x3000, 0x1000, CRC(1cd33f3a) SHA1(cf9248fd6cb56ec81d354afe032a2dea810e834b) )
	ROM_LOAD( "10.3h",        0x4000, 0x1000, CRC(b833a15b) SHA1(0d21aaa0ca5ccba89118b205a6b3b36b15663c47) )
	ROM_LOAD( "11.4h",        0x5000, 0x1000, CRC(cbd76ec2) SHA1(9434350ee93ca71efe78018b69913386353306ff) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "28.5a",        0x0000, 0x1000, CRC(d73a8252) SHA1(59d14f41f1a806f98ee33596b84fe5aefe606944) )
	ROM_LOAD( "29.5c",        0x1000, 0x1000, CRC(1401ccf2) SHA1(5762eafd9f402330e1d4ac677f46595087716c47) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331.9g",      0x0000, 0x0020, CRC(ca1d9ccd) SHA1(27124759a06497c1bc1a64b6d3faa6ba924a8447) )
ROM_END

ROM_START( skelagon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	/* first half of 36.bin is missing */
	ROM_LOAD( "31.bin",       0x1000, 0x1000, CRC(ae6f8647) SHA1(801e88b91c204f2797e5ce45390ea6eec27a3f54) )
	ROM_LOAD( "32.bin",       0x2000, 0x1000, CRC(a28c5838) SHA1(0a37de7986c494d1522ce76635dd1fa6d03f05c7) )
	ROM_LOAD( "33.bin",       0x3000, 0x1000, CRC(32f7e99c) SHA1(2718063a77eeeb8067a9cad7ff3d9e0266b61566) )
	ROM_LOAD( "37.bin",       0x7000, 0x1000, CRC(47f68a31) SHA1(6e15024f67c88a733ede8702d2a80ddb1892b27e) )
	ROM_LOAD( "24.bin",       0xc000, 0x1000, CRC(e6d7dc74) SHA1(c1e6d9598fb837775ee6550fea3cd4910572615e) )
	ROM_LOAD( "35.bin",       0xd000, 0x1000, CRC(5b2a0158) SHA1(66d2fb05a8daaa86bb547b4860d5bf27b4359326) )
	ROM_LOAD( "36.bin",       0xe000, 0x1000, BAD_DUMP CRC(f53ead29) SHA1(f8957b0c0558acc005f418adbfeb66d1d562c9ac) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5.5j",         0x0000, 0x1000, CRC(59028fb6) SHA1(94105b5b03c81a948a409f7ea20312bb9c79c150) )
	ROM_LOAD( "6.6j",         0x1000, 0x1000, CRC(5427670f) SHA1(ffc3f7186d0319f0fd7ed25eb97bb0db7bc107c6) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the sample CPU */
	ROM_LOAD( "1.1j",         0x0000, 0x1000, CRC(2f172c58) SHA1(4706d55fcfad4d5a87d96a0a0187f59997ef9720) )
	ROM_LOAD( "2.2j",         0x1000, 0x1000, CRC(a6ad2f6b) SHA1(14d1a93e507c349b14a1b26408cce23f089fa33c) )
	ROM_LOAD( "3.3j",         0x2000, 0x1000, CRC(fa1274fa) SHA1(e98cb602b265b209eaa4a9b3972e47c869ff863b) )
	ROM_LOAD( "4.4j",         0x3000, 0x1000, CRC(1cd33f3a) SHA1(cf9248fd6cb56ec81d354afe032a2dea810e834b) )
	ROM_LOAD( "10.bin",       0x4000, 0x1000, CRC(2c719de2) SHA1(0953e96f8be1cbab3f4a8e166457c74e986a87b1) )
	ROM_LOAD( "8.bin",        0x5000, 0x1000, CRC(350379dd) SHA1(e979251b11d6702170dd60ffd28fc15ea737588b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "38.bin",       0x0000, 0x1000, CRC(2fffa8b1) SHA1(6a6032f55b9fe1da209e4ed4423042efec773d4d) )
	ROM_LOAD( "39.bin",       0x1000, 0x1000, CRC(a854b5de) SHA1(dd038f20ee366d439f09f0c82fd6432101b3781a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331.9g",      0x0000, 0x0020, CRC(ca1d9ccd) SHA1(27124759a06497c1bc1a64b6d3faa6ba924a8447) )
ROM_END

ROM_START( mimonscr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mm1",          0x0000, 0x1000, CRC(0399a0c4) SHA1(8314124f9b535ce531663625d19cd3a76782ed3b) )
	ROM_LOAD( "mm2",          0x1000, 0x1000, CRC(2c5e971e) SHA1(39f979b99566e30a19c63115c936bb11fae4c609) )
	ROM_LOAD( "mm3",          0x2000, 0x1000, CRC(24ce1ce3) SHA1(ae5ba6913cabab2152bf48c0c0d5983ecbe5c700) )
	ROM_LOAD( "mm4",          0x3000, 0x1000, CRC(c83fb639) SHA1(38ddd80b25cc0707b9e53396c322fe731ea8bc3e) )
	ROM_LOAD( "mm5",          0xc000, 0x1000, CRC(a9f12dfc) SHA1(c279e3ac84194cc83642a2c330fd869eaae8f063) )
	ROM_LOAD( "mm6",          0xd000, 0x1000, CRC(e492a40c) SHA1(d01d6f9c18821fd8c7ed11d65d13bd0c9595881f) )
	ROM_LOAD( "mm7",          0xe000, 0x1000, CRC(5339928d) SHA1(7c28516fb7d762e2f77d0ed3dc56a57d0213dbf9) )
	ROM_LOAD( "mm8",          0xf000, 0x1000, CRC(eee7a12e) SHA1(bde6bfe98b15215c48c85a22615b0242ea4f0224) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mmsound1",	  0x0000, 0x1000, CRC(2d14c527) SHA1(062414ce0415b6c471149319ecae22f465df3a4f) )
	ROM_LOAD( "mmsnd2a",	  0x1000, 0x1000, CRC(35ed0f96) SHA1(5aaacae5c2acf97540b72491f71ea823f5eeae1a) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mmgfx1",		  0x0000, 0x2000, CRC(4af47337) SHA1(225f7bcfbb61e3a163ecaed675d4c81b3609562f) )
	ROM_LOAD( "mmgfx2",		  0x2000, 0x2000, CRC(def47da8) SHA1(8e62e5dc5c810efaa204d0fcb3d02bc84f61ba35) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "c01s.6e",    0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( scorpion )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.2d",         0x0000, 0x1000, CRC(ba1219b4) SHA1(33c7843dba44152a8bc3223ea0c30b13609b80ba) )
	ROM_LOAD( "2.2f",         0x1000, 0x1000, CRC(c3909ab6) SHA1(0bec902ae4291fa0530f4c89ad45cc7aab888b7a) )
	ROM_LOAD( "3.2g",         0x2000, 0x1000, CRC(43261352) SHA1(49468cbed7e0286b260eef297bd5fad0ab9fd45b) )
	ROM_LOAD( "4.2h",         0x3000, 0x1000, CRC(aba2276a) SHA1(42b0378f06d2bdb4faaaa95274a6c0f965716877) )
	ROM_LOAD( "5.2k",         0x6000, 0x0800, CRC(952f78f2) SHA1(9562037b104fc1852c2d2650209a77ffce2cb90e) )
	ROM_CONTINUE(             0x5800, 0x0800 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "32_a4.7c",     0x0000, 0x1000, CRC(361b8a36) SHA1(550ac5f721aaa9fea5f6d63ba590d6b367525c23) )
	ROM_LOAD( "32_a5.7d",     0x1000, 0x1000, CRC(addecdd4) SHA1(ba28f1d9c7c6b5e8ecef56a4b3f64be13fc10d43) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "32_f5.5f",     0x0000, 0x1000, CRC(1e5da9d6) SHA1(ca8b27e6dd40e4ca13e7e6b5f813bafca78b62f4) )
	ROM_LOAD( "32_h5.5h",     0x1000, 0x1000, CRC(a57adb0a) SHA1(d97c7dc4a6c5efb59cc0148e2498156c682c6714) )

	ROM_REGION( 0x3000, REGION_SOUND1, 0 ) /* Samples? / Speech? */
	ROM_LOAD( "32_a3.6e",     0x0000, 0x1000, CRC(279ae6f9) SHA1(a93b1d68c9f4b6ad62fdb8816285e61bd3b4b884) )
	ROM_LOAD( "32_a2.6d",     0x1000, 0x1000, CRC(90352dd4) SHA1(62c261a2f2fbd8eff31d5c72cf532d5e43d86dd3) )
	ROM_LOAD( "32_a1.6c",     0x2000, 0x1000, CRC(3bf2452d) SHA1(7a163e0ef108dd40d3beab5e9805886e45be744b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "prom.6e",      0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( scrpiona )
	/* this dump is bad (at least one rom) */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "scor_d2.bin",  0x0000, 0x1000, BAD_DUMP CRC(c5b9daeb) SHA1(faf7a22013dd5f063eb8f506f3722cfd5522539a) )
	ROM_LOAD( "scor_e2.bin",  0x1000, 0x1000, BAD_DUMP CRC(82308d05) SHA1(26bc7c8b3ea0020fd1b93f6aaa29d82d04ae64b2) )
	ROM_LOAD( "scor_g2.bin",  0x2000, 0x1000, BAD_DUMP CRC(756b09cd) SHA1(9aec34e063fe8c0d1392db09daea2875d06eec46) )
	ROM_LOAD( "scor_h2.bin",  0x3000, 0x1000, BAD_DUMP CRC(a0457b93) SHA1(5ed32e117a97660dae001bd97fcb3f31e0debb24) )
	ROM_LOAD( "scor_k2.bin",  0x5800, 0x0800, BAD_DUMP CRC(42ec34d8) SHA1(b358d10a96490f325420b992e8e03bb3884e415a) )
	ROM_LOAD( "scor_l2.bin",  0x6000, 0x0800, BAD_DUMP CRC(6623da33) SHA1(99110005d00c80d674bde5d21608f50b85ee488c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "32_a4.7c",     0x0000, 0x1000, CRC(361b8a36) SHA1(550ac5f721aaa9fea5f6d63ba590d6b367525c23) )
	ROM_LOAD( "32_a5.7d",     0x1000, 0x1000, CRC(addecdd4) SHA1(ba28f1d9c7c6b5e8ecef56a4b3f64be13fc10d43) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "scor_f5.bin",  0x0000, 0x1000, CRC(60180a38) SHA1(518c1267523139aa4e27860012a722b67fe25b6d) )
	ROM_LOAD( "32_h5.5h",     0x1000, 0x1000, CRC(a57adb0a) SHA1(d97c7dc4a6c5efb59cc0148e2498156c682c6714) )

	ROM_REGION( 0x3000, REGION_SOUND1, 0 ) /* Samples? / Speech? */
	ROM_LOAD( "scor_a3.bin",  0x0000, 0x1000, CRC(04abf178) SHA1(2e7f231413d9ec461ca21840f31d1d6b8b17c4d5) )
	ROM_LOAD( "scor_a2.bin",  0x1000, 0x1000, CRC(452d6354) SHA1(3d5397fddcc17b4d03b9cdc53a6439f159d1bfcc) )
	ROM_LOAD( "32_a1.6c",     0x2000, 0x1000, CRC(3bf2452d) SHA1(7a163e0ef108dd40d3beab5e9805886e45be744b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "prom.6e",      0x0000, 0x0020, CRC(4e3caeab) SHA1(a25083c3e36d28afdefe4af6e6d4f3155e303625) )
ROM_END

ROM_START( ad2083 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "ad0.10o",      0x0000, 0x2000, CRC(4d34325a) SHA1(4a0eb1cd94382c44ab2642d734d3da9025872eba) )
	ROM_LOAD( "ad1.9o",       0x2000, 0x2000, CRC(0f37134b) SHA1(a935ae013e9fb26b5ef44f7ebd2a043763b146db) )
	ROM_LOAD( "ad2.8o",       0xa000, 0x2000, CRC(bcfa655f) SHA1(6a552c67f48e9ece6c6d38b4151ff6f3dbfd8dcb) )
	ROM_LOAD( "ad3.7o",       0xc000, 0x2000, CRC(60655225) SHA1(628796b23ad66f8f7b2c160d923ecbea10fd7553) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU */
	ROM_LOAD( "ad1s.3d",      0x0000, 0x2000, CRC(80f39b0f) SHA1(35671eaf6fc7643ad691414349f1b2772d020e9a) )
	ROM_LOAD( "ad2s.4d",      0x2000, 0x1000, CRC(5177fe2b) SHA1(9aee953ae43131c4db9db71ca69a8ce9ad62ff05) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ad4.5k",       0x0000, 0x2000, CRC(388cdd21) SHA1(52f97d8e4f7c7f45a2875f03eadc622b540693e7) )
	ROM_LOAD( "ad5.3k",       0x2000, 0x2000, CRC(f53f3449) SHA1(0711f2e47504f256d46eea1e225e35f9bde8b9fb) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 ) /* data for the TMS5110 speech chip */
	ROM_LOAD( "ad1v.9a",      0x0000, 0x1000, CRC(4cb93fff) SHA1(2cc686a9a58a85f2bb04fb6ced4626e9952635bb) )
	ROM_LOAD( "ad2v.10a",     0x1000, 0x1000, CRC(4b530ea7) SHA1(8793b3497b598f33b34bf9524e360c6c62e8001d) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "prom-am27s19dc.1m", 0x0000, 0x0020, CRC(2759aebd) SHA1(644fd2c95ca49cbbc0ee1b88ca2563451ddd4fe0) )

	ROM_REGION( 0x0020, REGION_USER1, 0 ) /* sample related? near TMS5110 and sample roms */
	ROM_LOAD( "prom-sn74s188.8a",  0x0000, 0x0020, CRC(5e395112) SHA1(427d6a5b5d0837db4bf804f392d77ba5a86ffd72) )
ROM_END


GAME( 1981, scramble, 0,        fscramble,scramble, scramble,     ROT90, "Konami", "Scramble", GAME_SUPPORTS_SAVE )
GAME( 1981, scrambls, scramble, fscramble,scramble, scrambls,     ROT90, "[Konami] (Stern license)", "Scramble (Stern)", GAME_SUPPORTS_SAVE )
GAME( 1981, explorer, scramble, explorer, explorer, 0,		      ROT90, "bootleg", "Explorer", GAME_SUPPORTS_SAVE )
GAME( 1981, strfbomb, scramble, scramble, strfbomb, scramble,     ROT90, "Omni", "Strafe Bomb", GAME_SUPPORTS_SAVE )
GAME( 1981, atlantis, 0,        scramble, atlantis, atlantis,     ROT90, "Comsoft", "Battle of Atlantis (set 1)", GAME_SUPPORTS_SAVE )
GAME( 1981, atlants2, atlantis, scramble, atlantis, atlantis,     ROT90, "Comsoft", "Battle of Atlantis (set 2)", GAME_SUPPORTS_SAVE )
GAME( 1980, theend,   0,        theend,   theend,   theend,       ROT90, "Konami", "The End", GAME_SUPPORTS_SAVE )
GAME( 1980, theends,  theend,   theend,   theend,   theend,       ROT90, "[Konami] (Stern license)", "The End (Stern)", GAME_SUPPORTS_SAVE )
GAME( 1981, froggers, frogger,  froggers, froggers, froggers,     ROT90, "bootleg", "Frog", GAME_SUPPORTS_SAVE )
GAME( 1981, frogf,    frogger,  frogf,    froggers, froggers,     ROT90, "Falcon", "Frogger (Falcon bootleg)", GAME_SUPPORTS_SAVE )
GAME( 1982, amidars,  amidar,   scramble, amidars,  atlantis,     ROT90, "Konami", "Amidar (Scramble hardware)", GAME_SUPPORTS_SAVE )
GAME( 1982, triplep,  0,        triplep,  triplep,  scramble_ppi, ROT90, "KKI", "Triple Punch", GAME_SUPPORTS_SAVE )
GAME( 1982, knockout, triplep,  triplep,  triplep,  scramble_ppi, ROT90, "KKK", "Knock Out!!", GAME_SUPPORTS_SAVE )
GAME( 1981, mariner,  0,        mariner,  scramble, mariner,      ROT90, "Amenip", "Mariner", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE)
GAME( 1981, 800fath,  mariner,  mariner,  scramble, mariner,      ROT90, "Amenip (US Billiards Inc. license)", "800 Fathoms", GAME_SUPPORTS_SAVE )
GAME( 1981, ckongs,   ckong,    ckongs,   ckongs,   ckongs,       ROT90, "bootleg", "Crazy Kong (Scramble hardware)", GAME_SUPPORTS_SAVE )
GAME( 1981, mars,     0,        mars,     mars,     mars,         ROT90, "Artic", "Mars", GAME_SUPPORTS_SAVE )
GAME( 1982, devilfsh, 0,        devilfsh, devilfsh, devilfsh,     ROT90, "Artic", "Devil Fish", GAME_SUPPORTS_SAVE )
GAME( 1983, newsin7,  0,        newsin7,  newsin7,  mars,         ROT90, "ATW USA, Inc.", "New Sinbad 7", GAME_SUPPORTS_SAVE )
GAME( 1984, mrkougar, 0,        mrkougar, mrkougar, mrkougar,     ROT90, "ATW", "Mr. Kougar", GAME_SUPPORTS_SAVE )
GAME( 1983, mrkougr2, mrkougar, mrkougar, mrkougar, mrkougar,     ROT90, "ATW", "Mr. Kougar (earlier)", GAME_SUPPORTS_SAVE )
GAME( 1983, mrkougb,  mrkougar, mrkougb,  mrkougar, mrkougb,      ROT90, "bootleg", "Mr. Kougar (bootleg)", GAME_SUPPORTS_SAVE )
GAME( 1983, mrkougb2, mrkougar, mrkougb,  mrkougar, mrkougb,      ROT90, "bootleg", "Mr. Kougar (bootleg Set 2)", GAME_SUPPORTS_SAVE )
GAME( 1982, hotshock, 0,        hotshock, hotshock, hotshock,     ROT90, "E.G. Felaco", "Hot Shocker", GAME_SUPPORTS_SAVE )
GAME( 1982, conquer,  0,        hotshock, hotshock, 0,            ROT90, "<unknown>", "Conquer", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE)
GAME( 1983, hunchbks, hunchbak, hunchbks, hunchbks, scramble_ppi, ROT90, "Century Electronics", "Hunchback (Scramble hardware)", GAME_SUPPORTS_SAVE )
GAME( 1984, hncholms, huncholy, hncholms, hncholms, scramble_ppi, ROT90, "Century Electronics", "Hunchback Olympic (Scramble hardware)", GAME_SUPPORTS_SAVE )
GAME( 1983, cavelon,  0,        cavelon,  cavelon,  cavelon,      ROT90, "Jetsoft", "Cavelon", GAME_SUPPORTS_SAVE )
GAME( 1983, sfx,      0,        sfx,      sfx,      sfx,          ORIENTATION_FLIP_X, "Nichibutsu", "SF-X", GAME_SUPPORTS_SAVE )
GAME( 1983, skelagon, sfx,      sfx,      sfx,      sfx,          ORIENTATION_FLIP_X, "Nichibutsu USA", "Skelagon", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE)
GAME( 198?, mimonscr, mimonkey, mimonscr, mimonscr, mimonscr,     ROT90, "bootleg", "Mighty Monkey (bootleg on Scramble hardware)", GAME_SUPPORTS_SAVE )
GAME( 1982, scorpion, 0,		scorpion, scorpion, scorpion,	  ROT90, "Zaccaria", "Scorpion (set 1)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE)
GAME( 1982, scrpiona, scorpion, scorpion, scorpion, scorpion,	  ROT90, "Zaccaria", "Scorpion (set 2)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE)
GAME( 1983, ad2083,   0,        ad2083,   ad2083,   ad2083,       ROT90, "Midcoin", "A. D. 2083", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE)
