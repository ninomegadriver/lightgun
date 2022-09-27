/***************************************************************************

Various Video System Co. games using the C7-01 GGA, VS8803, VS8904, VS8905
video chips.
C7-01 GGA is used in a lot of games, some of them without sprites. So it
either controls tilemaps, or the video signal, or both.
I think 8904/8905 handle sprites, don't know about 8803.
tail2nos doesn't have the 8904/8905, and indeed it has a different sprite
system.

Driver by Nicola Salmoria


Notes:
- Sprite zoom is probably not 100% accurate.
  In pspikes, the zooming text during attract mode is horrible.

pspikes/turbofrc/aerofgtb write to two addresses which look like control
registers for a video generator. Maybe they control the display size/position.
aerofgt is different, it writes to consecutive memory addresses and the values
it writes don't seem to be related to these ones.

                  00 01 02 03 04 05  08 09 0a 0b 0c 0d
                  ------------------------------------
pspikes  352x240? 57 63 69 71 1f 00  77 79 7b 7f 1f 00
karatblz 352x240  57 63 69 71 1f 00  77 79 7b 7f 1f 00
turbofrc 352x240  57 63 69 71 1f 00  77 79 7b 7f 1f 00
spinlbrk 352x240  57 68 6f 75 ff 01  77 78 7b 7f ff 00
aerofgtb 320x224  4f 5d 63 71 1f 00  6f 70 72 7c 1f 02
tail2nos 320x240  4f 5e 64 71 1f 09  7a 7c 7e 7f 1f 02
f1gp     320x240  4f 5e 64 71 1f 09  7a 7c 7e 7f 1f 02
welltris 352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00

games with 8x4 tiles:

pipedrm  352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
hatris   352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
idolmj   352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00
mjnatsu  352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
mfunclub 352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
daiyogen 352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
nmsengen 352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff
fromance 352x240  57 63 69 71 1f 00  7a 7b 7e 7f 1f 00 * register 0b also briefly toggled to ff

register 00 could be screen width / 4 (hblank start?)
register 08 could be screen height / 2 (vblank start?)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2610intf.h"
#include "sound/3812intf.h"
#include "sound/okim6295.h"


extern UINT16 *aerofgt_rasterram;
extern UINT16 *aerofgt_bg1videoram,*aerofgt_bg2videoram;
extern UINT16 *aerofgt_spriteram1,*aerofgt_spriteram2,*aerofgt_spriteram3;
extern UINT16 *wbbc97_bitmapram;
extern size_t aerofgt_spriteram1_size,aerofgt_spriteram2_size,aerofgt_spriteram3_size;

WRITE16_HANDLER( aerofgt_bg1videoram_w );
WRITE16_HANDLER( aerofgt_bg2videoram_w );
WRITE16_HANDLER( pspikes_gfxbank_w );
WRITE16_HANDLER( pspikesb_gfxbank_w );
WRITE16_HANDLER( karatblz_gfxbank_w );
WRITE16_HANDLER( spinlbrk_gfxbank_w );
WRITE16_HANDLER( turbofrc_gfxbank_w );
WRITE16_HANDLER( aerofgt_gfxbank_w );
WRITE16_HANDLER( aerofgt_bg1scrollx_w );
WRITE16_HANDLER( aerofgt_bg1scrolly_w );
WRITE16_HANDLER( aerofgt_bg2scrollx_w );
WRITE16_HANDLER( aerofgt_bg2scrolly_w );
WRITE16_HANDLER( pspikes_palette_bank_w );
WRITE16_HANDLER( wbbc97_bitmap_enable_w );
VIDEO_START( pspikes );
VIDEO_START( karatblz );
VIDEO_START( spinlbrk );
VIDEO_START( turbofrc );
VIDEO_UPDATE( pspikes );
VIDEO_UPDATE( pspikesb );
VIDEO_UPDATE( karatblz );
VIDEO_UPDATE( spinlbrk );
VIDEO_UPDATE( turbofrc );
VIDEO_UPDATE( aerofgt );
VIDEO_UPDATE( aerfboot );
VIDEO_START( wbbc97 );
VIDEO_UPDATE( wbbc97 );


static int pending_command;

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
	{
		pending_command = 1;
		soundlatch_w(offset,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static WRITE16_HANDLER( turbofrc_sound_command_w )
{
	if (ACCESSING_MSB)
	{
		pending_command = 1;
		soundlatch_w(offset,(data >> 8) & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static WRITE16_HANDLER( aerfboot_soundlatch_w )
{
	if(data & 0x8000)
	{
		soundlatch_w(0,data & 0xff);
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static READ16_HANDLER( pending_command_r )
{
	return pending_command;
}

static WRITE8_HANDLER( pending_command_clear_w )
{
	pending_command = 0;
}

static WRITE8_HANDLER( aerofgt_sh_bankswitch_w )
{
	UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;

	memory_set_bankptr(1,rom + (data & 0x03) * 0x8000);
}

static MACHINE_RESET( aerofgt )
{
	aerofgt_sh_bankswitch_w(0,0);	/* needed by spinlbrk */
}


static WRITE16_HANDLER( pspikesb_oki_banking_w )
{
	OKIM6295_set_bank_base(0, 0x40000 * (data & 3));
}


static ADDRESS_MAP_START( pspikes_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x203fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xff8fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffd000, 0xffdfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffe000, 0xffefff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfff002, 0xfff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfff006, 0xfff007) AM_READ(pending_command_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pspikes_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0xff8000, 0xff8fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0xffc000, 0xffc3ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0xffd000, 0xffdfff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* bg1 scroll registers */
	AM_RANGE(0xffe000, 0xffefff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xfff000, 0xfff001) AM_WRITE(pspikes_palette_bank_w)
	AM_RANGE(0xfff002, 0xfff003) AM_WRITE(pspikes_gfxbank_w)
	AM_RANGE(0xfff004, 0xfff005) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0xfff006, 0xfff007) AM_WRITE(sound_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pspikesb_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM	/* work RAM */
	AM_RANGE(0x200000, 0x203fff) AM_RAM AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0xff8000, 0xff8fff) AM_RAM AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0xffc000, 0xffcbff) AM_RAM AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0xffd200, 0xffd201) AM_WRITE(pspikesb_gfxbank_w)
	AM_RANGE(0xffd000, 0xffdfff) AM_RAM AM_BASE(&aerofgt_rasterram)	/* bg1 scroll registers */
	AM_RANGE(0xffe000, 0xffefff) AM_RAM AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfff002, 0xfff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfff004, 0xfff005) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0xfff008, 0xfff009) AM_WRITE(pspikesb_oki_banking_w)
	AM_RANGE(0xc04000, 0xc04001) AM_WRITENOP
	AM_RANGE(0xfff006, 0xfff007) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( karatblz_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x081fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x082000, 0x083fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0a0000, 0x0affff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0b0000, 0x0bffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0c0000, 0x0cffff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0f8000, 0x0fbfff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0fe000, 0x0fe7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0ff000, 0x0ff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0ff002, 0x0ff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0ff004, 0x0ff005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0ff006, 0x0ff007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0ff008, 0x0ff009) AM_READ(input_port_4_word_r)
	AM_RANGE(0x0ff00a, 0x0ff00b) AM_READ(pending_command_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( karatblz_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x081fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x082000, 0x083fff) AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0x0a0000, 0x0affff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0x0b0000, 0x0bffff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram2) AM_SIZE(&aerofgt_spriteram2_size)
	AM_RANGE(0x0c0000, 0x0cffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0f8000, 0x0fbfff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0x0fe000, 0x0fe7ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0ff002, 0x0ff003) AM_WRITE(karatblz_gfxbank_w)
	AM_RANGE(0x0ff006, 0x0ff007) AM_WRITE(sound_command_w)
	AM_RANGE(0x0ff008, 0x0ff009) AM_WRITE(aerofgt_bg1scrollx_w)
	AM_RANGE(0x0ff00a, 0x0ff00b) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0x0ff00c, 0x0ff00d) AM_WRITE(aerofgt_bg2scrollx_w)
	AM_RANGE(0x0ff00e, 0x0ff00f) AM_WRITE(aerofgt_bg2scrolly_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( spinlbrk_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x080fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x082000, 0x082fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xffbfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffc7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffd000, 0xffd1ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffe000, 0xffe7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfff002, 0xfff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_2_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( spinlbrk_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x080fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x082000, 0x082fff) AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0xff8000, 0xffbfff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0xffc000, 0xffc7ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0xffd000, 0xffd1ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* bg1 scroll registers */
	AM_RANGE(0xffe000, 0xffe7ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xfff000, 0xfff001) AM_WRITE(spinlbrk_gfxbank_w)
	AM_RANGE(0xfff002, 0xfff003) AM_WRITE(aerofgt_bg2scrollx_w)
	AM_RANGE(0xfff006, 0xfff007) AM_WRITE(sound_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( turbofrc_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x000000, 0x0bffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0c0000, 0x0cffff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0d0000, 0x0d1fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0d2000, 0x0d3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0e0000, 0x0e3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0e4000, 0x0e7fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0f8000, 0x0fbfff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0fd000, 0x0fdfff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0fe000, 0x0fe7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0ff000, 0x0ff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0ff002, 0x0ff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0ff004, 0x0ff005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0ff006, 0x0ff007) AM_READ(pending_command_r)
	AM_RANGE(0x0ff008, 0x0ff009) AM_READ(input_port_3_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( turbofrc_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x000000, 0x0bffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0c0000, 0x0cffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0d0000, 0x0d1fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x0d2000, 0x0d3fff) AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0x0e0000, 0x0e3fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0x0e4000, 0x0e7fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram2) AM_SIZE(&aerofgt_spriteram2_size)
	AM_RANGE(0x0f8000, 0x0fbfff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0x0fd000, 0x0fdfff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* bg1 scroll registers */
	AM_RANGE(0x0fe000, 0x0fe7ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0ff002, 0x0ff003) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0x0ff004, 0x0ff005) AM_WRITE(aerofgt_bg2scrollx_w)
	AM_RANGE(0x0ff006, 0x0ff007) AM_WRITE(aerofgt_bg2scrolly_w)
	AM_RANGE(0x0ff008, 0x0ff00b) AM_WRITE(turbofrc_gfxbank_w)
	AM_RANGE(0x0ff00c, 0x0ff00d) AM_WRITE(MWA16_NOP)	/* related to bg2 (written together with the scroll registers) */
	AM_RANGE(0x0ff00e, 0x0ff00f) AM_WRITE(turbofrc_sound_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgtb_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x0c0000, 0x0cffff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0d0000, 0x0d1fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0d2000, 0x0d3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0e0000, 0x0e3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0e4000, 0x0e7fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0f8000, 0x0fbfff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0fd000, 0x0fd7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x0fe000, 0x0fe001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0fe002, 0x0fe003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0fe004, 0x0fe005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0fe006, 0x0fe007) AM_READ(pending_command_r)
	AM_RANGE(0x0fe008, 0x0fe009) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0ff000, 0x0fffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgtb_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x0c0000, 0x0cffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0d0000, 0x0d1fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x0d2000, 0x0d3fff) AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0x0e0000, 0x0e3fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0x0e4000, 0x0e7fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram2) AM_SIZE(&aerofgt_spriteram2_size)
	AM_RANGE(0x0f8000, 0x0fbfff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0x0fd000, 0x0fd7ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0fe002, 0x0fe003) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0x0fe004, 0x0fe005) AM_WRITE(aerofgt_bg2scrollx_w)
	AM_RANGE(0x0fe006, 0x0fe007) AM_WRITE(aerofgt_bg2scrolly_w)
	AM_RANGE(0x0fe008, 0x0fe00b) AM_WRITE(turbofrc_gfxbank_w)
	AM_RANGE(0x0fe00e, 0x0fe00f) AM_WRITE(turbofrc_sound_command_w)
	AM_RANGE(0x0ff000, 0x0fffff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* used only for the scroll registers */
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgt_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x1a0000, 0x1a07ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1b0000, 0x1b07ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1b0800, 0x1b0801) AM_READ(MRA16_NOP)	/* ??? */
	AM_RANGE(0x1b0ff0, 0x1b0fff) AM_READ(MRA16_RAM)	/* stack area during boot */
	AM_RANGE(0x1b2000, 0x1b3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1b4000, 0x1b5fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1c0000, 0x1c3fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1c4000, 0x1c7fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1d0000, 0x1d1fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfef000, 0xffefff) AM_READ(MRA16_RAM)	/* work RAM */
	AM_RANGE(0xffffa0, 0xffffa1) AM_READ(input_port_0_word_r)
	AM_RANGE(0xffffa2, 0xffffa3) AM_READ(input_port_1_word_r)
	AM_RANGE(0xffffa4, 0xffffa5) AM_READ(input_port_2_word_r)
	AM_RANGE(0xffffa6, 0xffffa7) AM_READ(input_port_3_word_r)
	AM_RANGE(0xffffa8, 0xffffa9) AM_READ(input_port_4_word_r)
	AM_RANGE(0xffffac, 0xffffad) AM_READ(pending_command_r)
	AM_RANGE(0xffffae, 0xffffaf) AM_READ(input_port_5_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgt_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x1a0000, 0x1a07ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x1b0000, 0x1b07ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* used only for the scroll registers */
	AM_RANGE(0x1b0800, 0x1b0801) AM_WRITE(MWA16_NOP)	/* ??? */
	AM_RANGE(0x1b0ff0, 0x1b0fff) AM_WRITE(MWA16_RAM)	/* stack area during boot */
	AM_RANGE(0x1b2000, 0x1b3fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x1b4000, 0x1b5fff) AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0x1c0000, 0x1c3fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0x1c4000, 0x1c7fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram2) AM_SIZE(&aerofgt_spriteram2_size)
	AM_RANGE(0x1d0000, 0x1d1fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0xfef000, 0xffefff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0xffff80, 0xffff87) AM_WRITE(aerofgt_gfxbank_w)
	AM_RANGE(0xffff88, 0xffff89) AM_WRITE(aerofgt_bg1scrolly_w)	/* + something else in the top byte */
	AM_RANGE(0xffff90, 0xffff91) AM_WRITE(aerofgt_bg2scrolly_w)	/* + something else in the top byte */
	AM_RANGE(0xffffac, 0xffffad) AM_WRITE(MWA16_NOP)	/* ??? */
	AM_RANGE(0xffffc0, 0xffffc1) AM_WRITE(sound_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerfboot_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x0c0000, 0x0cffff) AM_RAM	/* work RAM */
	AM_RANGE(0x0d0000, 0x0d1fff) AM_RAM AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0x0d2000, 0x0d3fff) AM_RAM AM_WRITE(aerofgt_bg2videoram_w) AM_BASE(&aerofgt_bg2videoram)
	AM_RANGE(0x0e0000, 0x0e3fff) AM_RAM AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0x0e4000, 0x0e7fff) AM_RAM AM_BASE(&aerofgt_spriteram2) AM_SIZE(&aerofgt_spriteram2_size)
	AM_RANGE(0x0f8000, 0x0fbfff) AM_RAM	/* work RAM */
	AM_RANGE(0x0fc000, 0x0fc7ff) AM_RAM //AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0x0fd000, 0x0fd7ff) AM_RAM AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0fe000, 0x0fe001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0fe002, 0x0fe003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0fe004, 0x0fe005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0fe008, 0x0fe009) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0fe002, 0x0fe003) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0x0fe004, 0x0fe005) AM_WRITE(aerofgt_bg2scrollx_w)
	AM_RANGE(0x0fe006, 0x0fe007) AM_WRITE(aerofgt_bg2scrolly_w)
	AM_RANGE(0x0fe008, 0x0fe00b) AM_WRITE(turbofrc_gfxbank_w)
	AM_RANGE(0x0fe010, 0x0fe011) AM_WRITENOP
	AM_RANGE(0x0fe012, 0x0fe013) AM_WRITE(aerfboot_soundlatch_w)
	AM_RANGE(0x0fe400, 0x0fe401) AM_WRITENOP
	AM_RANGE(0x0fe402, 0x0fe403) AM_WRITENOP
	AM_RANGE(0x0ff000, 0x0fffff) AM_RAM AM_BASE(&aerofgt_rasterram)	/* used only for the scroll registers */
	AM_RANGE(0x10b800, 0x10bfff) AM_RAM AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wbbc97_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x600000, 0x605fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xa00000, 0xa3ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xff8fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffd000, 0xffdfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffe000, 0xffefff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
	AM_RANGE(0xfff002, 0xfff003) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfff006, 0xfff007) AM_READNOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( wbbc97_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM)	/* work RAM */
	AM_RANGE(0x600000, 0x605fff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram1) AM_SIZE(&aerofgt_spriteram1_size)
	AM_RANGE(0xa00000, 0xa3ffff) AM_WRITE(MWA16_RAM) AM_BASE(&wbbc97_bitmapram)
	AM_RANGE(0xff8000, 0xff8fff) AM_WRITE(aerofgt_bg1videoram_w) AM_BASE(&aerofgt_bg1videoram)
	AM_RANGE(0xffc000, 0xffc3ff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_spriteram3) AM_SIZE(&aerofgt_spriteram3_size)
	AM_RANGE(0xffd000, 0xffdfff) AM_WRITE(MWA16_RAM) AM_BASE(&aerofgt_rasterram)	/* bg1 scroll registers */
	AM_RANGE(0xffe000, 0xffefff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xfff000, 0xfff001) AM_WRITE(pspikes_palette_bank_w)
	AM_RANGE(0xfff002, 0xfff003) AM_WRITE(pspikes_gfxbank_w)
	AM_RANGE(0xfff004, 0xfff005) AM_WRITE(aerofgt_bg1scrolly_w)
	AM_RANGE(0xfff006, 0xfff007) AM_WRITE(sound_command_w)
	AM_RANGE(0xfff00e, 0xfff00f) AM_WRITE(wbbc97_bitmap_enable_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( turbofrc_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x14, 0x14) AM_READ(soundlatch_r)
	AM_RANGE(0x18, 0x18) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0x1a, 0x1a) AM_READ(YM2610_status_port_0_B_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( turbofrc_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(aerofgt_sh_bankswitch_w)
	AM_RANGE(0x14, 0x14) AM_WRITE(pending_command_clear_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(YM2610_data_port_0_B_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgt_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0x02, 0x02) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0x0c, 0x0c) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerofgt_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(aerofgt_sh_bankswitch_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(pending_command_clear_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aerfboot_snd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( wbbc97_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf800) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xf810, 0xf810) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xf811, 0xf811) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0xfc00, 0xfc00) AM_NOP
	AM_RANGE(0xfc20, 0xfc20) AM_READ(soundlatch_r)
ADDRESS_MAP_END

INPUT_PORTS_START( pspikes )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	/* the following two select country in the Chinese version (ROMs not available) */
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0600, 0x0600, "1 Player Starting Score" )
	PORT_DIPSETTING(      0x0600, "12-12" )
	PORT_DIPSETTING(      0x0400, "11-11" )
	PORT_DIPSETTING(      0x0200, "11-12" )
	PORT_DIPSETTING(      0x0000, "10-12" )
	PORT_DIPNAME( 0x1800, 0x1800, "2 Players Starting Score" )
	PORT_DIPSETTING(      0x1800, "9-9" )
	PORT_DIPSETTING(      0x1000, "7-7" )
	PORT_DIPSETTING(      0x0800, "5-5" )
	PORT_DIPSETTING(      0x0000, "0-0" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x4000, 0x4000, "2 Players Time per Credit" )
	PORT_DIPSETTING(      0x4000, "3 min" )
	PORT_DIPSETTING(      0x0000, "2 min" )
	PORT_DIPNAME( 0x8000, 0x8000, "Debug" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( pspikesb )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_2C ) )
	/* the following two select country in the Chinese version (ROMs not available) */
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0600, 0x0600, "1 Player Starting Score" )
	PORT_DIPSETTING(      0x0600, "12-12" )
	PORT_DIPSETTING(      0x0400, "11-11" )
	PORT_DIPSETTING(      0x0200, "11-12" )
	PORT_DIPSETTING(      0x0000, "10-12" )
	PORT_DIPNAME( 0x1800, 0x1800, "2 Players Starting Score" )
	PORT_DIPSETTING(      0x1800, "9-9" )
	PORT_DIPSETTING(      0x1000, "7-7" )
	PORT_DIPSETTING(      0x0800, "5-5" )
	PORT_DIPSETTING(      0x0000, "0-0" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x4000, 0x4000, "2 Players Time per Credit" )
	PORT_DIPSETTING(      0x4000, "3 min" )
	PORT_DIPSETTING(      0x0000, "2 min" )
	PORT_DIPNAME( 0x8000, 0x8000, "Debug" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( karatblz )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) ) /* Coins 1, 2, 3 & 4 */
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0008, 0x0008, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Cabinet ) )	/* Game Type */
	PORT_DIPSETTING(      0x0060, "2 Players" )		/* 1 Unit / 2 Players */
	PORT_DIPSETTING(      0x0040, "3 Players" )		/* 1 Unit / 3 Players */
	PORT_DIPSETTING(      0x0020, "4 Players" )		/* 1 Unit / 4 Players */
	PORT_DIPSETTING(      0x0000, "4 Players (Team)" )	/* 2 Units / 4 Players */
	/*  With 4 player (Team) selected and Same Coin Slot:
        Coin A & B credit together for use by _only_ player 1 or player 2
        Coin C & D credit together for use by _only_ player 3 or player 4
        Otherwise with Individual selected, everyone is seperate  */
	PORT_DIPNAME( 0x0080, 0x0080, "Coin Slot" )
	PORT_DIPSETTING(      0x0080, "Same" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0600, 0x0200, "Number of Enemies" ) /* Default is DEF_STR( Hard ) */
	PORT_DIPSETTING(      0x0400, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x1800, 0x0800, "Strength of Enemies" ) /* Default is DEF_STR( Hard ) */
	PORT_DIPSETTING(      0x1000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Freeze" )	/* Listed in manual as N.C. (aka No Connection) */
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( spinlbrk )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x000f, "1 Credit 1 Health Pack" )	/* I chose "Health Packs" as the actual value can change */
	PORT_DIPSETTING(      0x000e, "1 Credit 2 Health Packs" )	/*  via dipswitch 2-7 (0x4000) see below */
	PORT_DIPSETTING(      0x000d, "1 Credit 3 Health Packs" )
	PORT_DIPSETTING(      0x000c, "1 Credit 4 Health Packs" )
	PORT_DIPSETTING(      0x000b, "1 Credit 5 Health Packs" )
	PORT_DIPSETTING(      0x000a, "1 Credit 6 Health Packs" )
	PORT_DIPSETTING(      0x0009, "2 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0008, "3 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0007, "4 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0006, "5 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0005, "2 Credits 2 Health Packs" )
	PORT_DIPSETTING(      0x0004, "2-1-1C  1-1-1 HPs" )
	PORT_DIPSETTING(      0x0003, "2-2C 1-2 HPs" )
	PORT_DIPSETTING(      0x0002, "1-1-1-1-1C 1-1-1-1-2 HPs" )
	PORT_DIPSETTING(      0x0001, "1-1-1-1C 1-1-1-2 HPs" )
	PORT_DIPSETTING(      0x0000, "1-1C 1-2 HPs" )
/* The last 5 Coin/Credit selections are cycles:
    Example: 0x0004 = 2-1-1C 1-1-1 HPs:
    2 Credits for the 1st Health Pack, 1 Credit for the 2nd Health Pack, 1 Credit
    for the 3rd Health Pack... Then back to 2 Credits agian for 1 HP, then 1 credit
    and 1 credit.... on and on.  With all Coin/Credit dips set to on, it's 1 Health
    Pack for odd credits, 2 Health Packs for even credits :p
    */
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00f0, "1 Credit 1 Health Pack" )
	PORT_DIPSETTING(      0x00e0, "1 Credit 2 Health Packs" )
	PORT_DIPSETTING(      0x00d0, "1 Credit 3 Health Packs" )
	PORT_DIPSETTING(      0x00c0, "1 Credit 4 Health Packs" )
	PORT_DIPSETTING(      0x00b0, "1 Credit 5 Health Packs" )
	PORT_DIPSETTING(      0x00a0, "1 Credit 6 Health Packs" )
	PORT_DIPSETTING(      0x0090, "2 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0080, "3 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0070, "4 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0060, "5 Credits 1 Health Pack" )
	PORT_DIPSETTING(      0x0050, "2 Credits 2 Health Packs" )
	PORT_DIPSETTING(      0x0040, "2-1-1C  1-1-1 HPs" )
	PORT_DIPSETTING(      0x0030, "2-2C 1-2 HPs" )
	PORT_DIPSETTING(      0x0020, "1-1-1-1-1C 1-1-1-1-2 HPs" )
	PORT_DIPSETTING(      0x0010, "1-1-1-1C 1-1-1-2 HPs" )
	PORT_DIPSETTING(      0x0000, "1-1C 1-2 HPs" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Coin Slot" )
	PORT_DIPSETTING(      0x0000, "Same" )
	PORT_DIPSETTING(      0x0400, "Individual" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Lever Type" )
	PORT_DIPSETTING(      0x1000, "Digital" )
	PORT_DIPSETTING(      0x0000, "Analog" )		/* This setting causes lever error??? */
	PORT_SERVICE( 0x2000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x4000, 0x4000, "Health Pack" )
	PORT_DIPSETTING(      0x4000, "32 Hitpoints" )
	PORT_DIPSETTING(      0x0000, "40 Hitpoints" )
	PORT_DIPNAME( 0x8000, 0x8000, "Life Restoration" )
	PORT_DIPSETTING(      0x8000, "10 Points" )
	PORT_DIPSETTING(      0x0000, "5 Points" )
INPUT_PORTS_END

INPUT_PORTS_START( spinlbrj )
	PORT_INCLUDE(spinlbrk)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0400, "Unlimited" )
	PORT_DIPSETTING(      0x0000, "6 Times" )
INPUT_PORTS_END

INPUT_PORTS_START( spinlbru )
	PORT_INCLUDE(spinlbrk)

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x4000, 0x4000, "Health Pack" )
	PORT_DIPSETTING(      0x4000, "20 Hitpoints" )
	PORT_DIPSETTING(      0x0000, "32 Hitpoints" )
INPUT_PORTS_END

INPUT_PORTS_START( turbofrc )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE )	/* "TEST" */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )//START1 )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) ) /* Coins 1, 2 & 3 */
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0008, 0x0008, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Coin Slot" )
	PORT_DIPSETTING(      0x0010, "Same" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x0020, 0x0000, "Play Mode" )
	PORT_DIPSETTING(      0x0020, "2 Players" )
	PORT_DIPSETTING(      0x0000, "3 Players" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e00, 0x0800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0e00, "1 (Easiest)")
	PORT_DIPSETTING(      0x0c00, "2" )
	PORT_DIPSETTING(      0x0a00, "3" )
	PORT_DIPSETTING(      0x0800, "4 (Normal)" )
	PORT_DIPSETTING(      0x0600, "5" )
	PORT_DIPSETTING(      0x0400, "6" )
	PORT_DIPSETTING(      0x0200, "7" )
	PORT_DIPSETTING(      0x0000, "8 (Hardest)" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x1000, "3" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x2000, "200000" )
	PORT_DIPSETTING(      0x0000, "300000" )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( aerofgtb )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Slot" )
	PORT_DIPSETTING(      0x0001, "Same" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0070, 0x0070, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x2000, "1" )
	PORT_DIPSETTING(      0x1000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x4000, "200000" )
	PORT_DIPSETTING(      0x0000, "300000" )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0001, 0x0000, "Country" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(      0x0001, "Taiwan" )
	/* TODO: there are others in the table at 11910 */
	/* this port is checked at 1b080 */
INPUT_PORTS_END

INPUT_PORTS_START( aerofgt )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Slot" )
	PORT_DIPSETTING(      0x0001, "Same" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0070, 0x0070, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0040, "200000" )
	PORT_DIPSETTING(      0x0000, "300000" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x000f, 0x0000, "Country" )
	PORT_DIPSETTING(      0x0000, "Any" )
	PORT_DIPSETTING(      0x000f, DEF_STR( USA ) )
	PORT_DIPSETTING(      0x000e, "Korea" )
	PORT_DIPSETTING(      0x000d, "Hong Kong" )
	PORT_DIPSETTING(      0x000b, "Taiwan" )
INPUT_PORTS_END

INPUT_PORTS_START( wbbc97 )
	PORT_INCLUDE(pspikes)
	PORT_MODIFY("IN0")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END

static const gfx_layout pspikes_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout aerofgt_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout pspikesb_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout aerfboot_charlayout =
{
	8,8,
	RGN_FRAC(1,8),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout pspikes_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, RGN_FRAC(1,2)+1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+3*4, RGN_FRAC(1,2)+2*4,
			5*4, 4*4, 7*4, 6*4, RGN_FRAC(1,2)+5*4, RGN_FRAC(1,2)+4*4, RGN_FRAC(1,2)+7*4, RGN_FRAC(1,2)+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static const gfx_layout pspikesb_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_layout aerofgtb_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 3*4, 2*4, 1*4, 0*4, RGN_FRAC(1,2)+3*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+1*4, RGN_FRAC(1,2)+0*4,
			7*4, 6*4, 5*4, 4*4, RGN_FRAC(1,2)+7*4, RGN_FRAC(1,2)+6*4, RGN_FRAC(1,2)+5*4, RGN_FRAC(1,2)+4*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static const gfx_layout aerofgt_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
			10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_layout aerfboot_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+3*4, 0*4, 1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4,
			6*4, 7*4, RGN_FRAC(1,2)+6*4, RGN_FRAC(1,2)+7*4, 4*4, 5*4, RGN_FRAC(1,2)+4*4, RGN_FRAC(1,2)+5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static const gfx_layout wbbc97_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{
		RGN_FRAC(3,4)+4,  RGN_FRAC(3,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+4,  RGN_FRAC(1,4)+0, RGN_FRAC(0,4)+4, RGN_FRAC(0,4)+0,
		RGN_FRAC(3,4)+12, RGN_FRAC(3,4)+8, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8,
		RGN_FRAC(1,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(0,4)+12, RGN_FRAC(0,4)+8
	},
	{ 0*8,2*8,4*8,6*8,8*8,10*8,12*8,14*8,16*8,18*8,20*8,22*8,24*8,26*8,28*8,30*8 },
	8*32
};

static const gfx_decode pspikes_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pspikes_charlayout,      0, 64 },	/* colors    0-1023 in 8 banks */
	{ REGION_GFX2, 0, &pspikes_spritelayout, 1024, 64 },	/* colors 1024-2047 in 4 banks */
	{ -1 } /* end of array */
};

static const gfx_decode pspikesb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pspikesb_charlayout,      0, 64 },	/* colors    0-1023 in 8 banks */
	{ REGION_GFX2, 0, &pspikesb_spritelayout, 1024, 64 },	/* colors 1024-2047 in 4 banks */
	{ -1 } /* end of array */
};

static const gfx_decode turbofrc_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pspikes_charlayout,     0, 16 },
	{ REGION_GFX2, 0, &pspikes_charlayout,   256, 16 },
	{ REGION_GFX3, 0, &pspikes_spritelayout, 512, 16 },
	{ REGION_GFX4, 0, &pspikes_spritelayout, 768, 16 },
	{ -1 } /* end of array */
};

static const gfx_decode aerofgtb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pspikes_charlayout,      0, 16 },
	{ REGION_GFX2, 0, &pspikes_charlayout,    256, 16 },
	{ REGION_GFX3, 0, &aerofgtb_spritelayout, 512, 16 },
	{ REGION_GFX4, 0, &aerofgtb_spritelayout, 768, 16 },
	{ -1 } /* end of array */
};

static const gfx_decode aerofgt_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &aerofgt_charlayout,     0, 16 },
	{ REGION_GFX1, 0, &aerofgt_charlayout,   256, 16 },
	{ REGION_GFX2, 0, &aerofgt_spritelayout, 512, 16 },
	{ REGION_GFX3, 0, &aerofgt_spritelayout, 768, 16 },
	{ -1 } /* end of array */
};

static const gfx_decode aerfboot_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0,       &aerfboot_charlayout,     0, 16 },
	{ REGION_GFX1, 0x20000, &aerfboot_charlayout,   256, 16 },
	{ REGION_GFX2, 0,       &aerfboot_spritelayout, 512, 16 },
	{ REGION_GFX3, 0,       &aerfboot_spritelayout, 768, 16 },
	{ -1 } /* end of array */
};

static const gfx_decode wbbc97_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pspikes_charlayout,      0, 64 },	/* colors    0-1023 in 8 banks */
	{ REGION_GFX2, 0, &wbbc97_spritelayout, 1024, 64 },	/* colors 1024-2047 in 4 banks */
	{ -1 } /* end of array */
};

static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	irqhandler,
	REGION_SOUND1,
	REGION_SOUND2
};

static struct YM3812interface ym3812_interface =
{
	irqhandler	/* IRQ Line */
};


static MACHINE_DRIVER_START( pspikes )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(pspikes_readmem,pspikes_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(turbofrc_sound_readport,turbofrc_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8+4, 44*8+4-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(pspikes_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(pspikes)
	MDRV_VIDEO_UPDATE(pspikes)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pspikesb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(pspikesb_map,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8+4, 44*8+4-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(pspikesb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(pspikes)
	MDRV_VIDEO_UPDATE(pspikesb)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( karatblz )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(karatblz_readmem,karatblz_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(turbofrc_sound_readport,turbofrc_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 45*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(turbofrc_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(karatblz)
	MDRV_VIDEO_UPDATE(karatblz)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( spinlbrk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(spinlbrk_readmem,spinlbrk_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* there are vectors for 3 and 4 too */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(turbofrc_sound_readport,turbofrc_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 45*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(turbofrc_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(spinlbrk)
	MDRV_VIDEO_UPDATE(spinlbrk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( turbofrc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(turbofrc_readmem,turbofrc_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(turbofrc_sound_readport,turbofrc_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 44*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(turbofrc_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(turbofrc)
	MDRV_VIDEO_UPDATE(turbofrc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( aerofgtb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(aerofgtb_readmem,aerofgtb_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(aerofgt_sound_readport,aerofgt_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(500)
				/* wrong but improves sprite-background synchronization */

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8+12, 40*8-1+12, 0*8, 28*8-1)
	MDRV_GFXDECODE(aerofgtb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(turbofrc)
	MDRV_VIDEO_UPDATE(turbofrc)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( aerofgt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(aerofgt_readmem,aerofgt_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(aerofgt_sound_readport,aerofgt_sound_writeport)
								/* IRQs are triggered by the YM2610 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(400)
				/* wrong but improves sprite-background synchronization */

	MDRV_MACHINE_RESET(aerofgt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(aerofgt_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(turbofrc)
	MDRV_VIDEO_UPDATE(aerofgt)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( aerfboot )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(aerfboot_map,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2) /* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(aerfboot_snd_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(500)
				/* wrong but improves sprite-background synchronization */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8+12, 40*8-1+12, 0*8, 28*8-1)
	MDRV_GFXDECODE(aerfboot_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(turbofrc)
	MDRV_VIDEO_UPDATE(aerfboot)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wbbc97 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)	/* 10 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(wbbc97_readmem,wbbc97_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* all irq vectors are the same */

	MDRV_CPU_ADD(Z80,8000000/2) /* 4 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(wbbc97_sound_map,0)
								/* IRQs are triggered by the YM3812 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8+14, 44*8-1+4, 0*8, 30*8-1)

	MDRV_GFXDECODE(wbbc97_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(wbbc97)
	MDRV_VIDEO_UPDATE(wbbc97)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 3579545)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pspikes )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pspikes2.bin", 0x00000, 0x40000, CRC(ec0c070e) SHA1(4ddcc184e835a2f9d15f01aaa03734fd75fe797e) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "19",           0x00000, 0x20000, CRC(7e8ed6e5) SHA1(eeb1a1e1989fad8fc1e741928422efaec0598868) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g7h",          0x000000, 0x80000, CRC(74c23c3d) SHA1(c0ac57d1f05c42556f97154ce1a08f465948546b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g7j",          0x000000, 0x80000, CRC(0b9e4739) SHA1(64b440a5026735aafe1a7cc2806fe0d78f4a6fba) )
	ROM_LOAD( "g7l",          0x080000, 0x80000, CRC(943139ff) SHA1(59065f9c3b3a47159c5968df199bdcb1b4f51f29) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "a47",          0x00000, 0x40000, CRC(c6779dfa) SHA1(ea7adefdb0da02755428aac9a6f86c908fc11253) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "o5b",          0x000000, 0x100000, CRC(07d6cbac) SHA1(d3d5778dbaca7b6cdceae959d0847d56df7b5cc1) )
ROM_END

ROM_START( pspikesk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "20",           0x00000, 0x40000, CRC(75cdcee2) SHA1(272a08c46c1d0989f9fbb156e28e6a7ffa9c0a53) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "19",           0x00000, 0x20000, CRC(7e8ed6e5) SHA1(eeb1a1e1989fad8fc1e741928422efaec0598868) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g7h",          0x000000, 0x80000, CRC(74c23c3d) SHA1(c0ac57d1f05c42556f97154ce1a08f465948546b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g7j",          0x000000, 0x80000, CRC(0b9e4739) SHA1(64b440a5026735aafe1a7cc2806fe0d78f4a6fba) )
	ROM_LOAD( "g7l",          0x080000, 0x80000, CRC(943139ff) SHA1(59065f9c3b3a47159c5968df199bdcb1b4f51f29) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "a47",          0x00000, 0x40000, CRC(c6779dfa) SHA1(ea7adefdb0da02755428aac9a6f86c908fc11253) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "o5b",          0x000000, 0x100000, CRC(07d6cbac) SHA1(d3d5778dbaca7b6cdceae959d0847d56df7b5cc1) )
ROM_END

ROM_START( svolly91 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "u11.jpn",      0x00000, 0x40000, CRC(ea2e4c82) SHA1(f9cf9122499d9b1e54221fb8b6ef9c12004ca85e) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "19",           0x00000, 0x20000, CRC(7e8ed6e5) SHA1(eeb1a1e1989fad8fc1e741928422efaec0598868) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g7h",          0x000000, 0x80000, CRC(74c23c3d) SHA1(c0ac57d1f05c42556f97154ce1a08f465948546b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g7j",          0x000000, 0x80000, CRC(0b9e4739) SHA1(64b440a5026735aafe1a7cc2806fe0d78f4a6fba) )
	ROM_LOAD( "g7l",          0x080000, 0x80000, CRC(943139ff) SHA1(59065f9c3b3a47159c5968df199bdcb1b4f51f29) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "a47",          0x00000, 0x40000, CRC(c6779dfa) SHA1(ea7adefdb0da02755428aac9a6f86c908fc11253) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "o5b",          0x000000, 0x100000, CRC(07d6cbac) SHA1(d3d5778dbaca7b6cdceae959d0847d56df7b5cc1) )
ROM_END

ROM_START( pspikesb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2ic63.bin",    0x00000, 0x20000, CRC(d25e184c) SHA1(89ad275b03d909a7d16d2927df3ddf12301e4c60) )
	ROM_LOAD16_BYTE( "3ic62.bin",    0x00001, 0x20000, CRC(5add1a34) SHA1(e166d5c76f2f087254f2af442f49251a9885f5bc) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "4ic122.bin",   0x00000, 0x20000, CRC(ea1c05a7) SHA1(adfdfeac80df287ffa6f469dc38ea94698817cf4) )
	ROM_LOAD( "5ic120.bin",   0x20000, 0x20000, CRC(bfdc60f4) SHA1(2b1893fac2651ac82f5a05b8f891b20c928ced7e) )
	ROM_LOAD( "6ic118.bin",   0x40000, 0x20000, CRC(96a5c235) SHA1(dad4ef9069d3130f719a402737909bb48225b73c) )
	ROM_LOAD( "7ic116.bin",   0x60000, 0x20000, CRC(a7e00b36) SHA1(2b5e85ec02e8893d7d730aad4d690883b1d236cc) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "8ic121.bin",   0x00000, 0x40000, CRC(fc096cfc) SHA1(75af810c97361b6f08767949b90c394a7a03f60b) )
	ROM_LOAD( "9ic119.bin",   0x40000, 0x40000, CRC(a45ec985) SHA1(16357f5df7841e11889ac6fced1e2a9288585a29) )
	ROM_LOAD( "10ic117.bin",  0x80000, 0x40000, CRC(3976b372) SHA1(72feec5a6fe7995f39d4b431dbbf25435359b04d) )
	ROM_LOAD( "11ic115.bin",  0xc0000, 0x40000, CRC(f9249937) SHA1(5993e5ab7295ca2fa5c8f4c05ce23731741f4e97) )

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* Samples */
	ROM_LOAD( "1ic21.bin",    0x000000, 0x80000, CRC(1b78ed0b) SHA1(886bfd78709c295839dd51c7f5a13f5c452c0ab3) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is what gets switched */
	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x020000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x020000)
	ROM_COPY( REGION_USER1, 0x020000, 0x060000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x020000)
	ROM_COPY( REGION_USER1, 0x040000, 0x0a0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x000000, 0x0c0000, 0x020000)
	ROM_COPY( REGION_USER1, 0x060000, 0x0e0000, 0x020000)
ROM_END

ROM_START( spinlbrk )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ic98",    0x00000, 0x10000, CRC(36c2bf70) SHA1(f627d0e7dad1760bcc95af4a6346050a1a277048) )
	ROM_LOAD16_BYTE( "ic104",   0x00001, 0x10000, CRC(34a7e158) SHA1(5884570c1be38bfedffca3fd38089d0ae3391d4f) )
	ROM_LOAD16_BYTE( "ic93",    0x20000, 0x10000, CRC(726f4683) SHA1(65aff0548333571d47a96d4bf5a7857f12399cc7) )
	ROM_LOAD16_BYTE( "ic94",    0x20001, 0x10000, CRC(c4385e03) SHA1(6683eed812fa8a5430125b14e8647f8e9024bbdd) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "ic117",        0x00000, 0x08000, CRC(625ada41) SHA1(2dd0674c68ea382431115c155afbf880f5b9deb2) )
	ROM_LOAD( "ic118",        0x10000, 0x10000, CRC(1025f024) SHA1(3e497c74c950d2cd2a0931cf2ae9b0124d11ca6a) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15",         0x000000, 0x80000, CRC(e318cf3a) SHA1(d634001a0029566ce7b8fa30075970919eb5f44e) )
	ROM_LOAD( "ic9",          0x080000, 0x80000, CRC(e071f674) SHA1(b6d98d7fcc28516d937d8c655d07305515be8a20) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic17",         0x000000, 0x80000, CRC(a63d5a55) SHA1(a942651a206a2abe4f60f0717e1d5d8e89b993d4) )
	ROM_LOAD( "ic11",         0x080000, 0x80000, CRC(7dcc913d) SHA1(527bae5020581d1ac322ea25c8e0994d54bbc051) )
	ROM_LOAD( "ic16",         0x100000, 0x80000, CRC(0d84af7f) SHA1(07356ee61c84c4c4ccb49c8dfe8c468990580041) )	//FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic12",         0x000000, 0x80000, CRC(d63fac4e) SHA1(bb96d2e41334d136b9208dbe7e88a45e3bbc6542) )
	ROM_LOAD( "ic18",         0x080000, 0x80000, CRC(5a60444b) SHA1(62c418aedd1087dac82dcb44830cce00278103dd) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic14",         0x000000, 0x80000, CRC(1befd0f3) SHA1(7ab6fb5bf814ef3ae9a306a0d32d1078ee594461) )
	ROM_LOAD( "ic20",         0x080000, 0x80000, CRC(c2f84a61) SHA1(1dce538ced54a61c43ed25e1d71b5ac1c8935cc5) )
	ROM_LOAD( "ic35",         0x100000, 0x80000, CRC(eba8e1a3) SHA1(976ef30437df9aba6fa6d5cd11728476f34eb05b) )
	ROM_LOAD( "ic40",         0x180000, 0x80000, CRC(5ef5aa7e) SHA1(8d4b0f2348c536c6781c8ba25722301673aca289) )

	ROM_REGION16_BE( 0x24000, REGION_GFX5, 0 )	/* hardcoded sprite maps */
	ROM_LOAD16_BYTE( "ic19",    0x00000, 0x10000, CRC(db24eeaa) SHA1(300dd1ce81dd258b265bc3a64b8542ed152ed2cf) )
	ROM_LOAD16_BYTE( "ic13",    0x00001, 0x10000, CRC(97025bf4) SHA1(0519f0c94f3d417bf8ff0124a3a137035a4013dc) )
	/* 20000-23fff empty space, filled in vh_startup */

	/* no REGION_SOUND1 */

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "ic166",        0x000000, 0x80000, CRC(6e0d063a) SHA1(313983e69f9625814de033fef7f6e9564694117a) )
	ROM_LOAD( "ic163",        0x080000, 0x80000, CRC(e6621dfb) SHA1(85ee77c4720b7eb20ecf293c16b3105c8dcb1114) )	//FIRST AND SECOND HALF IDENTICAL
ROM_END

ROM_START( spinlbru )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ic98.u5", 0x00000, 0x10000, CRC(3a0f7667) SHA1(55d5fa1a325c17532ed83d231032bdbe9fb84d85) )
	ROM_LOAD16_BYTE( "ic104.u6",0x00001, 0x10000, CRC(a0e0af31) SHA1(21f6c3246bb7be2fd926324fd6d041e319a4e214) )
	ROM_LOAD16_BYTE( "ic93.u4", 0x20000, 0x10000, CRC(0cf73029) SHA1(e1346b759a41f9eec9536dc90671778582e595b4) )
	ROM_LOAD16_BYTE( "ic94.u3", 0x20001, 0x10000, CRC(5cf7c426) SHA1(b201da40c4511d2845004dff72d36adbb8a4fab9) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "ic117",        0x00000, 0x08000, CRC(625ada41) SHA1(2dd0674c68ea382431115c155afbf880f5b9deb2) )
	ROM_LOAD( "ic118",        0x10000, 0x10000, CRC(1025f024) SHA1(3e497c74c950d2cd2a0931cf2ae9b0124d11ca6a) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15",         0x000000, 0x80000, CRC(e318cf3a) SHA1(d634001a0029566ce7b8fa30075970919eb5f44e) )
	ROM_LOAD( "ic9",          0x080000, 0x80000, CRC(e071f674) SHA1(b6d98d7fcc28516d937d8c655d07305515be8a20) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic17",         0x000000, 0x80000, CRC(a63d5a55) SHA1(a942651a206a2abe4f60f0717e1d5d8e89b993d4) )
	ROM_LOAD( "ic11",         0x080000, 0x80000, CRC(7dcc913d) SHA1(527bae5020581d1ac322ea25c8e0994d54bbc051) )
	ROM_LOAD( "ic16",         0x100000, 0x80000, CRC(0d84af7f) SHA1(07356ee61c84c4c4ccb49c8dfe8c468990580041) )	//FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic12",         0x000000, 0x80000, CRC(d63fac4e) SHA1(bb96d2e41334d136b9208dbe7e88a45e3bbc6542) )
	ROM_LOAD( "ic18",         0x080000, 0x80000, CRC(5a60444b) SHA1(62c418aedd1087dac82dcb44830cce00278103dd) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic14",         0x000000, 0x80000, CRC(1befd0f3) SHA1(7ab6fb5bf814ef3ae9a306a0d32d1078ee594461) )
	ROM_LOAD( "ic20",         0x080000, 0x80000, CRC(c2f84a61) SHA1(1dce538ced54a61c43ed25e1d71b5ac1c8935cc5) )
	ROM_LOAD( "ic35",         0x100000, 0x80000, CRC(eba8e1a3) SHA1(976ef30437df9aba6fa6d5cd11728476f34eb05b) )
	ROM_LOAD( "ic40",         0x180000, 0x80000, CRC(5ef5aa7e) SHA1(8d4b0f2348c536c6781c8ba25722301673aca289) )

	ROM_REGION16_BE( 0x24000, REGION_GFX5, 0 )	/* hardcoded sprite maps */
	ROM_LOAD16_BYTE( "ic19",    0x00000, 0x10000, CRC(db24eeaa) SHA1(300dd1ce81dd258b265bc3a64b8542ed152ed2cf) )
	ROM_LOAD16_BYTE( "ic13",    0x00001, 0x10000, CRC(97025bf4) SHA1(0519f0c94f3d417bf8ff0124a3a137035a4013dc) )
	/* 20000-23fff empty space, filled in vh_startup */

	/* no REGION_SOUND1 */

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "ic166",        0x000000, 0x80000, CRC(6e0d063a) SHA1(313983e69f9625814de033fef7f6e9564694117a) )
	ROM_LOAD( "ic163",        0x080000, 0x80000, CRC(e6621dfb) SHA1(85ee77c4720b7eb20ecf293c16b3105c8dcb1114) )	//FIRST AND SECOND HALF IDENTICAL
ROM_END

ROM_START( spinlbrj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "j5",      0x00000, 0x10000, CRC(6a3d690e) SHA1(4ac1985ea0a73b8fc12105ff75121718595dd171) )
	ROM_LOAD16_BYTE( "j6",      0x00001, 0x10000, CRC(869593fa) SHA1(5821b011d42113f247bd100cecf140bbfc1e969c) )
	ROM_LOAD16_BYTE( "j4",      0x20000, 0x10000, CRC(33e33912) SHA1(d6d052cd8dbedfd254bdf5e82ad770e4bf241777) )
	ROM_LOAD16_BYTE( "j3",      0x20001, 0x10000, CRC(16ca61d0) SHA1(5d99a1261251412c3c758af751997fe31026c0d6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "ic117",        0x00000, 0x08000, CRC(625ada41) SHA1(2dd0674c68ea382431115c155afbf880f5b9deb2) )
	ROM_LOAD( "ic118",        0x10000, 0x10000, CRC(1025f024) SHA1(3e497c74c950d2cd2a0931cf2ae9b0124d11ca6a) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15",         0x000000, 0x80000, CRC(e318cf3a) SHA1(d634001a0029566ce7b8fa30075970919eb5f44e) )
	ROM_LOAD( "ic9",          0x080000, 0x80000, CRC(e071f674) SHA1(b6d98d7fcc28516d937d8c655d07305515be8a20) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic17",         0x000000, 0x80000, CRC(a63d5a55) SHA1(a942651a206a2abe4f60f0717e1d5d8e89b993d4) )
	ROM_LOAD( "ic11",         0x080000, 0x80000, CRC(7dcc913d) SHA1(527bae5020581d1ac322ea25c8e0994d54bbc051) )
	ROM_LOAD( "ic16",         0x100000, 0x80000, CRC(0d84af7f) SHA1(07356ee61c84c4c4ccb49c8dfe8c468990580041) )	//FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic12",         0x000000, 0x80000, CRC(d63fac4e) SHA1(bb96d2e41334d136b9208dbe7e88a45e3bbc6542) )
	ROM_LOAD( "ic18",         0x080000, 0x80000, CRC(5a60444b) SHA1(62c418aedd1087dac82dcb44830cce00278103dd) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic14",         0x000000, 0x80000, CRC(1befd0f3) SHA1(7ab6fb5bf814ef3ae9a306a0d32d1078ee594461) )
	ROM_LOAD( "ic20",         0x080000, 0x80000, CRC(c2f84a61) SHA1(1dce538ced54a61c43ed25e1d71b5ac1c8935cc5) )
	ROM_LOAD( "ic35",         0x100000, 0x80000, CRC(eba8e1a3) SHA1(976ef30437df9aba6fa6d5cd11728476f34eb05b) )
	ROM_LOAD( "ic40",         0x180000, 0x80000, CRC(5ef5aa7e) SHA1(8d4b0f2348c536c6781c8ba25722301673aca289) )

	ROM_REGION16_BE( 0x24000, REGION_GFX5, 0 )	/* hardcoded sprite maps */
	ROM_LOAD16_BYTE( "ic19",    0x00000, 0x10000, CRC(db24eeaa) SHA1(300dd1ce81dd258b265bc3a64b8542ed152ed2cf) )
	ROM_LOAD16_BYTE( "ic13",    0x00001, 0x10000, CRC(97025bf4) SHA1(0519f0c94f3d417bf8ff0124a3a137035a4013dc) )
	/* 20000-23fff empty space, filled in vh_startup */

	/* no REGION_SOUND1 */

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "ic166",        0x000000, 0x80000, CRC(6e0d063a) SHA1(313983e69f9625814de033fef7f6e9564694117a) )
	ROM_LOAD( "ic163",        0x080000, 0x80000, CRC(e6621dfb) SHA1(85ee77c4720b7eb20ecf293c16b3105c8dcb1114) )	//FIRST AND SECOND HALF IDENTICAL
ROM_END

ROM_START( karatblz )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rom2v3",  0x00000, 0x40000, CRC(01f772e1) SHA1(f87f19a82d75839b5671f23ce14218d7b910eabc) )
	ROM_LOAD16_WORD_SWAP( "1.u15",   0x40000, 0x40000, CRC(d16ee21b) SHA1(d454cdf22b72a537b9d7ae73deb8136a4f09da47) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "5.u92",        0x00000, 0x20000, CRC(97d67510) SHA1(1ffd419e3dec7de1099cd5819b0309f7dd0df80e) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gha.u55",      0x00000, 0x80000, CRC(3e0cea91) SHA1(bab41715f106d364013b64649441d280bc6893cf) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gh9.u61",      0x00000, 0x80000, CRC(5d1676bd) SHA1(6227d489c9c6259a0ac2bef62821fbf94efca8c6) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u42",          0x000000, 0x100000, CRC(65f0da84) SHA1(0bfbc6f4b87583703246704eb9fa13b1b3e6f90e) )
	ROM_LOAD( "3.u44",        0x100000, 0x020000, CRC(34bdead2) SHA1(99f9a8cac807fcd599db55d2dc624ed92a3862ef) )
	ROM_LOAD( "u43",          0x200000, 0x100000, CRC(7b349e5d) SHA1(8590a328f403e2c697a8d698c08d4adaf01fff62) )
	ROM_LOAD( "4.u45",        0x300000, 0x020000, CRC(be4d487d) SHA1(6d19c91d0498c43017219f0c10f4845a51ccfa7f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u59.ghb",      0x000000, 0x80000, CRC(158c9cde) SHA1(a2c1b404d40e6c2627691f5c7a3f63484bd5d2de) )
	ROM_LOAD( "ghd.u60",      0x080000, 0x80000, CRC(73180ae3) SHA1(e4eaf6693826d9e72032d0a0e25938a23ab7d792) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "u105.gh8",     0x000000, 0x080000, CRC(7a68cb1b) SHA1(1bdd0000c2d68019b9e5bf8f7ad84a6ae1af8443) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "u104",         0x000000, 0x100000, CRC(5795e884) SHA1(a4178497ad0a1e60ceb87612b218d77b36d2a11b) )
ROM_END

ROM_START( karatblu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "2.u14",   0x00000, 0x40000, CRC(202e6220) SHA1(2605511a0574cbc39fdf3d8ae27a0aa9b43345fb) )
	ROM_LOAD16_WORD_SWAP( "1.u15",   0x40000, 0x40000, CRC(d16ee21b) SHA1(d454cdf22b72a537b9d7ae73deb8136a4f09da47) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "5.u92",        0x00000, 0x20000, CRC(97d67510) SHA1(1ffd419e3dec7de1099cd5819b0309f7dd0df80e) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gha.u55",      0x00000, 0x80000, CRC(3e0cea91) SHA1(bab41715f106d364013b64649441d280bc6893cf) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gh9.u61",      0x00000, 0x80000, CRC(5d1676bd) SHA1(6227d489c9c6259a0ac2bef62821fbf94efca8c6) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u42",          0x000000, 0x100000, CRC(65f0da84) SHA1(0bfbc6f4b87583703246704eb9fa13b1b3e6f90e) )
	ROM_LOAD( "3.u44",        0x100000, 0x020000, CRC(34bdead2) SHA1(99f9a8cac807fcd599db55d2dc624ed92a3862ef) )
	ROM_LOAD( "u43",          0x200000, 0x100000, CRC(7b349e5d) SHA1(8590a328f403e2c697a8d698c08d4adaf01fff62) )
	ROM_LOAD( "4.u45",        0x300000, 0x020000, CRC(be4d487d) SHA1(6d19c91d0498c43017219f0c10f4845a51ccfa7f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u59.ghb",      0x000000, 0x80000, CRC(158c9cde) SHA1(a2c1b404d40e6c2627691f5c7a3f63484bd5d2de) )
	ROM_LOAD( "ghd.u60",      0x080000, 0x80000, CRC(73180ae3) SHA1(e4eaf6693826d9e72032d0a0e25938a23ab7d792) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "u105.gh8",     0x000000, 0x080000, CRC(7a68cb1b) SHA1(1bdd0000c2d68019b9e5bf8f7ad84a6ae1af8443) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "u104",         0x000000, 0x100000, CRC(5795e884) SHA1(a4178497ad0a1e60ceb87612b218d77b36d2a11b) )
ROM_END

ROM_START( karatblj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "2tecmo.u14",   0x00000, 0x40000, CRC(57e52654) SHA1(15939d8f7c693b9248f3dd2b2ad5fbae2c19621f) )
	ROM_LOAD16_WORD_SWAP( "1.u15",        0x40000, 0x40000, CRC(d16ee21b) SHA1(d454cdf22b72a537b9d7ae73deb8136a4f09da47) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "5.u92",        0x00000, 0x20000, CRC(97d67510) SHA1(1ffd419e3dec7de1099cd5819b0309f7dd0df80e) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gha.u55",      0x00000, 0x80000, CRC(3e0cea91) SHA1(bab41715f106d364013b64649441d280bc6893cf) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gh9.u61",      0x00000, 0x80000, CRC(5d1676bd) SHA1(6227d489c9c6259a0ac2bef62821fbf94efca8c6) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u42",          0x000000, 0x100000, CRC(65f0da84) SHA1(0bfbc6f4b87583703246704eb9fa13b1b3e6f90e) )
	ROM_LOAD( "3.u44",        0x100000, 0x020000, CRC(34bdead2) SHA1(99f9a8cac807fcd599db55d2dc624ed92a3862ef) )
	ROM_LOAD( "u43",          0x200000, 0x100000, CRC(7b349e5d) SHA1(8590a328f403e2c697a8d698c08d4adaf01fff62) )
	ROM_LOAD( "4.u45",        0x300000, 0x020000, CRC(be4d487d) SHA1(6d19c91d0498c43017219f0c10f4845a51ccfa7f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u59.ghb",      0x000000, 0x80000, CRC(158c9cde) SHA1(a2c1b404d40e6c2627691f5c7a3f63484bd5d2de) )
	ROM_LOAD( "ghd.u60",      0x080000, 0x80000, CRC(73180ae3) SHA1(e4eaf6693826d9e72032d0a0e25938a23ab7d792) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "u105.gh8",     0x000000, 0x080000, CRC(7a68cb1b) SHA1(1bdd0000c2d68019b9e5bf8f7ad84a6ae1af8443) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "u104",         0x000000, 0x100000, CRC(5795e884) SHA1(a4178497ad0a1e60ceb87612b218d77b36d2a11b) )
ROM_END

ROM_START( turbofrc )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tfrc2.bin",    0x00000, 0x40000, CRC(721300ee) SHA1(79ab32fdfd377592a0bdbd1c4794cfd529a3eb7b) )
	ROM_LOAD16_WORD_SWAP( "tfrc1.bin",    0x40000, 0x40000, CRC(6cd5312b) SHA1(57b109fe268fb963e981c91b6d288667a3c9a665) )
	ROM_LOAD16_WORD_SWAP( "tfrc3.bin",    0x80000, 0x40000, CRC(63f50557) SHA1(f8dba8c9ba412c9a67457ec31a804c57593ab20b) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "tfrcu166.bin", 0x00000, 0x20000, CRC(2ca14a65) SHA1(95f6e7b4fa7ca26872ff472d7e6fb75fd4f281d5) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tfrcu94.bin",  0x000000, 0x80000, CRC(baa53978) SHA1(7f103122dd0bf675226ccf309fba73f645e0c79b) )
	ROM_LOAD( "tfrcu95.bin",  0x080000, 0x20000, CRC(71a6c573) SHA1(f14ebca676d85fabcde27631145933abc376dd12) )

	ROM_REGION( 0x0a0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tfrcu105.bin", 0x000000, 0x80000, CRC(4de4e59e) SHA1(571396dadb8aac043319cabe24e629210e442d57) )
	ROM_LOAD( "tfrcu106.bin", 0x080000, 0x20000, CRC(c6479eb5) SHA1(47a58f082c73bc9dae3970e760ba46478ce6a190) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tfrcu116.bin", 0x000000, 0x80000, CRC(df210f3b) SHA1(990ac43e4a46fee6b929c5b27d317cdadf179b8b) )
	ROM_LOAD( "tfrcu118.bin", 0x080000, 0x40000, CRC(f61d1d79) SHA1(2b8e33912c05c26170afd2fced0ff06cb7a097fa) )
	ROM_LOAD( "tfrcu117.bin", 0x100000, 0x80000, CRC(f70812fd) SHA1(1964e1134940825211cd4825fdd3f13b8242192d) )
	ROM_LOAD( "tfrcu119.bin", 0x180000, 0x40000, CRC(474ea716) SHA1(67753e96fa4fc8cd689a8bddeb60dbde259cacaa) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tfrcu134.bin", 0x000000, 0x80000, CRC(487330a2) SHA1(0bd36c1f5776ba2773f621e9bcb22f56ed1d84ec) )
	ROM_LOAD( "tfrcu135.bin", 0x080000, 0x80000, CRC(3a7e5b6d) SHA1(0079ffaa1bf93a5087c75615c78ec596b28c9a32) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "tfrcu180.bin",   0x00000, 0x20000, CRC(39c7c7d5) SHA1(66ee9f7cbc18ffab2c70f77ab0edead6bb018ca9) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "tfrcu179.bin", 0x000000, 0x100000, CRC(60ca0333) SHA1(28b94edc98d360386759780ccd1122d43ffa5279) )
ROM_END

ROM_START( aerofgt )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "1.u4",         0x00000, 0x80000, CRC(6fdff0a2) SHA1(7cc9529b426091027aa3e23586cb7d162376c0ff) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "2.153",        0x00000, 0x20000, CRC(a1ef64ec) SHA1(fa3e434738bf4e742ad68882c1e914100ce0f761) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "538a54.124",   0x000000, 0x80000, CRC(4d2c4df2) SHA1(f51c2b3135f0a921ac1a79e63d6878c03cb6254b) )
	ROM_LOAD( "1538a54.124",  0x080000, 0x80000, CRC(286d109e) SHA1(3a5f3d2d89cf58f6ef15e4bd3f570b84e8e695b2) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "538a53.u9",    0x000000, 0x100000, CRC(630d8e0b) SHA1(5a0c252ccd53c5199a695909d25ecb4e53dc15b9) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "534g8f.u18",   0x000000, 0x80000, CRC(76ce0926) SHA1(5ef4cec215d4dd600d8fcd1bd9a4c09081d59e33) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "it-19-01",     0x00000, 0x40000, CRC(6d42723d) SHA1(57c59234e9925430a4c687733682efed06d7eed1) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "it-19-06",     0x000000, 0x100000, CRC(cdbbdb1d) SHA1(067c816545f246ff1fd4c821d70df1e7eb47938c) )
ROM_END

ROM_START( aerofgtb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "v2",                0x00000, 0x40000, CRC(5c9de9f0) SHA1(93b62c59f0bc052c6fdbd5aae292a7ab2122dfd1) )
	ROM_LOAD16_BYTE( "v1",                0x00001, 0x40000, CRC(89c1dcf4) SHA1(41401d63049c140e4254dc791022d85c44271390) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "v3",           0x00000, 0x20000, CRC(cbb18cf4) SHA1(7119a7536cf710660ff06d1e7d2879c79ef12b3d) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-03",     0x000000, 0x80000, CRC(85eba1a4) SHA1(5691a95d6359fdab29be0d615066370c2b856c0a) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-02",     0x000000, 0x80000, CRC(4f57f8ba) SHA1(aaad548e9a7490dfd48a975135716225f416b6f6) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-04",     0x000000, 0x80000, CRC(3b329c1f) SHA1(279cb32d69ce1e71f42cfad93d395794a3e92bc6) )
	ROM_LOAD( "it-19-05",     0x080000, 0x80000, CRC(02b525af) SHA1(07f23d15938dfbdc4f0977ba1463a06090569026) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "g27",          0x000000, 0x40000, CRC(4d89cbc8) SHA1(93f248f3dc1a15c32d14a147b37d5d660d0e4337) )
	ROM_LOAD( "g26",          0x040000, 0x40000, CRC(8072c1d2) SHA1(c14634f5f2686cf616f415d9ea4a0c6490054beb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "it-19-01",     0x00000, 0x40000, CRC(6d42723d) SHA1(57c59234e9925430a4c687733682efed06d7eed1) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "it-19-06",     0x000000, 0x100000, CRC(cdbbdb1d) SHA1(067c816545f246ff1fd4c821d70df1e7eb47938c) )
ROM_END

ROM_START( aerofgtc )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "v2.149",            0x00000, 0x40000, CRC(f187aec6) SHA1(8905af34f114ae22fbfbd3ae115f19280bdd4fb3) )
	ROM_LOAD16_BYTE( "v1.111",            0x00001, 0x40000, CRC(9e684b19) SHA1(b5e1e5b74ed9fd223c9315ee2d548e620224c102) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "2.153",        0x00000, 0x20000, CRC(a1ef64ec) SHA1(fa3e434738bf4e742ad68882c1e914100ce0f761) )
	ROM_RELOAD(               0x10000, 0x20000 )

	/* gfx ROMs were missing in this set, I'm using the aerofgtb ones */
	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-03",     0x000000, 0x80000, CRC(85eba1a4) SHA1(5691a95d6359fdab29be0d615066370c2b856c0a) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-02",     0x000000, 0x80000, CRC(4f57f8ba) SHA1(aaad548e9a7490dfd48a975135716225f416b6f6) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-04",     0x000000, 0x80000, CRC(3b329c1f) SHA1(279cb32d69ce1e71f42cfad93d395794a3e92bc6) )
	ROM_LOAD( "it-19-05",     0x080000, 0x80000, CRC(02b525af) SHA1(07f23d15938dfbdc4f0977ba1463a06090569026) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "g27",          0x000000, 0x40000, CRC(4d89cbc8) SHA1(93f248f3dc1a15c32d14a147b37d5d660d0e4337) )
	ROM_LOAD( "g26",          0x040000, 0x40000, CRC(8072c1d2) SHA1(c14634f5f2686cf616f415d9ea4a0c6490054beb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "it-19-01",     0x00000, 0x40000, CRC(6d42723d) SHA1(57c59234e9925430a4c687733682efed06d7eed1) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "it-19-06",     0x000000, 0x100000, CRC(cdbbdb1d) SHA1(067c816545f246ff1fd4c821d70df1e7eb47938c) )
ROM_END

ROM_START( sonicwi )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2.149",        0x00000, 0x40000, CRC(3d1b96ba) SHA1(941be323c0cb15e05c92b897984617b05c5cf676) )
	ROM_LOAD16_BYTE( "1.111",        0x00001, 0x40000, CRC(a3d09f94) SHA1(a1064d659488878f5303edc2b8636312ab632a83) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "2.153",        0x00000, 0x20000, CRC(a1ef64ec) SHA1(fa3e434738bf4e742ad68882c1e914100ce0f761) )	// 3.156
	ROM_RELOAD(               0x10000, 0x20000 )

	/* gfx ROMs were missing in this set, I'm using the aerofgtb ones */
	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-03",     0x000000, 0x80000, CRC(85eba1a4) SHA1(5691a95d6359fdab29be0d615066370c2b856c0a) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-02",     0x000000, 0x80000, CRC(4f57f8ba) SHA1(aaad548e9a7490dfd48a975135716225f416b6f6) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "it-19-04",     0x000000, 0x80000, CRC(3b329c1f) SHA1(279cb32d69ce1e71f42cfad93d395794a3e92bc6) )
	ROM_LOAD( "it-19-05",     0x080000, 0x80000, CRC(02b525af) SHA1(07f23d15938dfbdc4f0977ba1463a06090569026) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "g27",          0x000000, 0x40000, CRC(4d89cbc8) SHA1(93f248f3dc1a15c32d14a147b37d5d660d0e4337) )
	ROM_LOAD( "g26",          0x040000, 0x40000, CRC(8072c1d2) SHA1(c14634f5f2686cf616f415d9ea4a0c6490054beb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "it-19-01",     0x00000, 0x40000, CRC(6d42723d) SHA1(57c59234e9925430a4c687733682efed06d7eed1) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "it-19-06",     0x000000, 0x100000, CRC(cdbbdb1d) SHA1(067c816545f246ff1fd4c821d70df1e7eb47938c) )
ROM_END

ROM_START( aerfboot )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "afb_ep2.u3",  0x00000, 0x40000, CRC(2bb9edf7) SHA1(cf0a62070fc0803dd8c473c375f6a2d1884ba2bf) )
	ROM_LOAD16_BYTE( "afb_ep3.u2",  0x00001, 0x40000, CRC(475d3df3) SHA1(58bde24e9dea2fb0d7ae4f2a574b06bc1a33a13d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "afb_ep1.u17",  0x0000, 0x8000, CRC(d41b5ab2) SHA1(17d9b999c9af1f332d67e7ce1a2f71fd08178303) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "afb_ep9.hh",   0x000000, 0x40000, CRC(41233923) SHA1(20f2849407ac7bd851d2617ad72fd413775da410) )
	ROM_LOAD( "afb_ep8.hi",   0x040000, 0x40000, CRC(97607ad3) SHA1(fb72e7ef0c6f7a736e12a9ff71017460f866195e) )
	ROM_LOAD( "afb_ep7.hj",   0x080000, 0x40000, CRC(01dc793e) SHA1(dbd9d22d75f5bcef9102667722cebb75574badd3) )
	ROM_LOAD( "afb_ep6.hk",   0x0c0000, 0x40000, CRC(cad7862a) SHA1(bfd729b19ff740ad3dc3b645c4f07f71126c0f3e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "afb_ep12.tc",  0x000000, 0x80000, CRC(1e692065) SHA1(a67da59cd65ec492d6e6ab14b1800fd35480a52d) )
	ROM_LOAD( "afb_ep10.ta",  0x080000, 0x80000, CRC(e50db1a7) SHA1(952676879fb6a260c56a120b849abfae75f4cf2b) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "afb_ep13.td",  0x000000, 0x40000, CRC(1830f70c) SHA1(1759de9b56e4999defc08b2423eff38ec98c4f17) )
	ROM_LOAD( "afb_ep11.tb",  0x040000, 0x40000, CRC(6298c0eb) SHA1(ede63849973742c67637eac0ec9cda95ea2ecebc) )

	ROM_REGION( 0x140000, REGION_SOUND1, ROMREGION_ERASEFF ) /* sound samples */
	ROM_LOAD( "afb_ep5.u29",  0x000000, 0x20000, CRC(3559609a) SHA1(6f0b633bf74f41487fc98dcdc43a83eb67f3d14c) )
	ROM_LOAD( "afb_ep4.u30",  0x040000, 0x20000, CRC(f9652163) SHA1(d8c1fcf44b350cc65378869e4eb188ea232b4948) )
	ROM_CONTINUE(			  0x080000, 0x20000 )
	ROM_CONTINUE(			  0x0c0000, 0x20000 )
	ROM_CONTINUE(			  0x100000, 0x20000 )
ROM_END

ROM_START( wbbc97 )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "03.27c040.ur4.rom",  0x000001, 0x80000, CRC(fb4e48fc) SHA1(cffc75766a9b867ab73597156142aa7c70bf6f20) )
	ROM_LOAD16_BYTE( "07.27c040.uo4.rom",  0x000000, 0x80000, CRC(87605dcc) SHA1(c5d05e7c581e02f88fd42c65768f5c8632e571a1) )
	ROM_LOAD16_BYTE( "04.27c4000.ur4a.rom",0x100001, 0x80000, CRC(2dd6ff07) SHA1(54724f49d4ca1db16a799704a9e023f6ee407fee) )
	ROM_LOAD16_BYTE( "08.27c4000.uo4a.rom",0x100000, 0x80000, CRC(1b96ef5b) SHA1(10bfecfc18c65735ddecf830dd72dd855ecf5ee7) )
	ROM_LOAD16_BYTE( "05.27c4000.ur4b.rom",0x200001, 0x80000, CRC(84104886) SHA1(807d4441bde6535b780c0c680773804b1268a024) )
	ROM_LOAD16_BYTE( "09.27c4000.uo4b.rom",0x200000, 0x80000, CRC(0367043c) SHA1(a5b77730e17b6223a8b465fe36d9447b60eb51ab) )
	ROM_LOAD16_BYTE( "06.27c4000.ur4c.rom",0x300001, 0x80000, CRC(b22d11c4) SHA1(15d2ba97704bbcf9d851b650a9c56a6a668cfe63) )
	ROM_LOAD16_BYTE( "10.27c040.uo4c.rom", 0x300000, 0x80000, CRC(fe403e8b) SHA1(5f8202792d9ec3e0404637614277c0375c747f7e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* z80 code */
	ROM_LOAD( "02.27c512.su11.rom",  0x000000, 0x10000, CRC(f03178e9) SHA1(5b0abee03059109a3cdb08a9341091255d5cb6ae) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )	/* GFX */
	ROM_LOAD( "15.27c020.uu10.rom",  0x000000, 0x40000, CRC(965bc99e) SHA1(db72121cfbcd6916f46ac5bd3592681eafa4e5da) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* GFX */
	ROM_LOAD( "11.27c020.ue12.rom", 0x000000, 0x40000, CRC(a0b23c8a) SHA1(85ccc5dcc891a352b90f0f3d89f6115bc52face6) )
	ROM_LOAD( "12.27c020.ue11.rom", 0x040000, 0x40000, CRC(4e529623) SHA1(b3e1e1ba5e05f7e095c0409f199c89b81297cf40) )
	ROM_LOAD( "13.27c020.ue10.rom", 0x080000, 0x40000, CRC(3745f892) SHA1(085986dff9639dedaee3bcecca17a6ea7e4a45f4) )
	ROM_LOAD( "14.27c020.ue9.rom",  0x0c0000, 0x40000, CRC(2814f4d2) SHA1(bf459b9ff160d0f18d74224d5e0729b8120261e6) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "01.27c020.su10.rom",  0x000000, 0x40000, CRC(c024e48c) SHA1(d3caedd22044c1645d96301a93f794db3ff77047) )

	ROM_REGION( 0x200, REGION_USER1, 0 ) /* ??? */
	ROM_LOAD( "82s147a.rom",  0x000000, 0x200, CRC(72cec9d2) SHA1(1c6fe6b47fe24bdbebb51d6bef56bf71c9029e72) )
ROM_END


GAME( 1990, spinlbrk, 0,        spinlbrk, spinlbrk, 0, ROT0,   "V-System Co.", "Spinal Breakers (World)", GAME_NO_COCKTAIL )
GAME( 1990, spinlbru, spinlbrk, spinlbrk, spinlbru, 0, ROT0,   "V-System Co.", "Spinal Breakers (US)", GAME_NO_COCKTAIL )
GAME( 1990, spinlbrj, spinlbrk, spinlbrk, spinlbrj, 0, ROT0,   "V-System Co.", "Spinal Breakers (Japan)", GAME_NO_COCKTAIL )
GAME( 1991, pspikes,  0,        pspikes,  pspikes,  0, ROT0,   "Video System Co.", "Power Spikes (World)", GAME_NO_COCKTAIL )
GAME( 1991, pspikesk, pspikes,  pspikes,  pspikes,  0, ROT0,   "Video System Co.", "Power Spikes (Korea)", GAME_NO_COCKTAIL )
GAME( 1991, svolly91, pspikes,  pspikes,  pspikes,  0, ROT0,   "Video System Co.", "Super Volley '91 (Japan)", GAME_NO_COCKTAIL )
GAME( 1991, pspikesb, pspikes,  pspikesb, pspikesb, 0, ROT0,   "bootleg",          "Power Spikes (bootleg)", GAME_NO_COCKTAIL )
GAME( 1991, karatblz, 0,        karatblz, karatblz, 0, ROT0,   "Video System Co.", "Karate Blazers (World?)", GAME_NO_COCKTAIL )
GAME( 1991, karatblu, karatblz, karatblz, karatblz, 0, ROT0,   "Video System Co.", "Karate Blazers (US)", GAME_NO_COCKTAIL )
GAME( 1991, karatblj, karatblz, karatblz, karatblz, 0, ROT0,   "Video System Co.", "Karate Blazers (Japan)", GAME_NO_COCKTAIL )
GAME( 1991, turbofrc, 0,        turbofrc, turbofrc, 0, ROT270, "Video System Co.", "Turbo Force", GAME_NO_COCKTAIL )
GAME( 1992, aerofgt,  0,        aerofgt,  aerofgt,  0, ROT270, "Video System Co.", "Aero Fighters", GAME_NO_COCKTAIL )
GAME( 1992, aerofgtb, aerofgt,  aerofgtb, aerofgtb, 0, ROT270, "Video System Co.", "Aero Fighters (Turbo Force hardware set 1)", GAME_NO_COCKTAIL )
GAME( 1992, aerofgtc, aerofgt,  aerofgtb, aerofgtb, 0, ROT270, "Video System Co.", "Aero Fighters (Turbo Force hardware set 2)", GAME_NO_COCKTAIL )
GAME( 1992, sonicwi,  aerofgt,  aerofgtb, aerofgtb, 0, ROT270, "Video System Co.", "Sonic Wings (Japan)", GAME_NO_COCKTAIL )
GAME( 1992, aerfboot, aerofgt,  aerfboot, aerofgtb, 0, ROT270, "bootleg",          "Aero Fighters (bootleg)", GAME_NO_COCKTAIL | GAME_NO_SOUND | GAME_NOT_WORKING )
GAME( 1997, wbbc97,   0,        wbbc97,   wbbc97,   0, ROT0,   "Comad",            "Beach Festival World Championship 1997", GAME_NO_COCKTAIL )
