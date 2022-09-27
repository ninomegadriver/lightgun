/***************************************************************************

                              -= Tetris Plus 2 =-

                    driver by   Luca Elia (l.elia@tin.it)


Main  CPU    :  TMP68HC000P-12

Video Chips  :  SS91022-03 9428XX001
                GS91022-04 9721PD008
                SS91022-05 9347EX002
                GS91022-05 048 9726HX002

Sound Chips  :  Yamaha YMZ280B-F

Other        :  XILINX XC5210 PQ240C X68710M AKJ9544
                XC7336 PC44ACK9633 A63458A
                NVRAM


To Do:

-   There is a 3rd unimplemented layer capable of rotation (not used by
    the game, can be tested in service mode).
-   Priority RAM is not taken into account.
-   The video system on this hardware is almost the same as the MegaSystem 32 board

Notes:

-   The Japan set doesn't seem to have (or use) NVRAM. I can't enter
    a test mode or use the service coin either !?

***************************************************************************/

#include "driver.h"
#include "sound/ymz280b.h"

UINT16 tetrisp2_systemregs[0x10];
UINT16 rocknms_sub_systemregs[0x10];

UINT16 rockn_protectdata;
UINT16 rockn_adpcmbank;
UINT16 rockn_soundvolume;

static mame_timer *rockn_timer_l4;
static mame_timer *rockn_timer_sub_l4;

/* Variables defined in vidhrdw: */

extern UINT16 *tetrisp2_vram_bg, *tetrisp2_scroll_bg;
extern UINT16 *tetrisp2_vram_fg, *tetrisp2_scroll_fg;
extern UINT16 *tetrisp2_vram_rot, *tetrisp2_rotregs;

extern UINT16 *tetrisp2_priority;

extern UINT16 *rocknms_sub_vram_bg, *rocknms_sub_scroll_bg;
extern UINT16 *rocknms_sub_vram_fg, *rocknms_sub_scroll_fg;
extern UINT16 *rocknms_sub_vram_rot, *rocknms_sub_rotregs;

extern UINT16 *rocknms_sub_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( tetrisp2_palette_w );
WRITE16_HANDLER( rocknms_sub_palette_w );
READ16_HANDLER( tetrisp2_priority_r );
WRITE16_HANDLER( tetrisp2_priority_w );
WRITE16_HANDLER( rockn_priority_w );
READ16_HANDLER( rocknms_sub_priority_r );
WRITE16_HANDLER( rocknms_sub_priority_w );

WRITE16_HANDLER( tetrisp2_vram_bg_w );
WRITE16_HANDLER( tetrisp2_vram_fg_w );
WRITE16_HANDLER( tetrisp2_vram_rot_w );

WRITE16_HANDLER( rocknms_sub_vram_bg_w );
WRITE16_HANDLER( rocknms_sub_vram_fg_w );
WRITE16_HANDLER( rocknms_sub_vram_rot_w );

VIDEO_START( tetrisp2 );
VIDEO_UPDATE( tetrisp2 );
VIDEO_START( rockntread );
VIDEO_UPDATE( rockntread );
VIDEO_START( rocknms );
VIDEO_UPDATE( rocknms );

/***************************************************************************


                                    Sound


***************************************************************************/

static WRITE16_HANDLER( tetrisp2_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		tetrisp2_systemregs[offset] = data;
	}
}

#define ROCKN_TIMER_BASE 500000

static WRITE16_HANDLER( rockn_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		tetrisp2_systemregs[offset] = data;
		if (offset == 0x0c)
		{
			double timer = TIME_IN_NSEC(ROCKN_TIMER_BASE) * (4096 - data);
			timer_adjust(rockn_timer_l4, timer, 0, timer);
		}
	}
}


static WRITE16_HANDLER( rocknms_sub_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		rocknms_sub_systemregs[offset] = data;
		if (offset == 0x0c)
		{
			double timer = TIME_IN_NSEC(ROCKN_TIMER_BASE) * (4096 - data);
			timer_adjust(rockn_timer_sub_l4, timer, 0, timer);
		}
	}
}


/***************************************************************************


                                    Sound


***************************************************************************/

static READ16_HANDLER( tetrisp2_sound_r )
{
	return YMZ280B_status_0_r(offset);
}

static WRITE16_HANDLER( tetrisp2_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

static READ16_HANDLER( rockn_adpcmbank_r )
{
	return ((rockn_adpcmbank & 0xf0ff) | (rockn_protectdata << 8));
}

static WRITE16_HANDLER( rockn_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	rockn_adpcmbank = data;
	bank = ((data & 0x001f) >> 2);

	if (bank > 7)
	{
		ui_popup("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0c00000 * bank)], 0x0c00000);
}

static WRITE16_HANDLER( rockn2_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	char banktable[9][3]=
	{
		{  0,  1,  2 },		// bank $00
		{  3,  4,  5 },		// bank $04
		{  6,  7,  8 },		// bank $08
		{  9, 10, 11 },		// bank $0c
		{ 12, 13, 14 },		// bank $10
		{ 15, 16, 17 },		// bank $14
		{ 18, 19, 20 },		// bank $18
		{  0,  0,  0 },		// bank $1c
		{  0,  5, 14 },		// bank $20
	};

	rockn_adpcmbank = data;
	bank = ((data & 0x003f) >> 2);

	if (bank > 8)
	{
		ui_popup("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][0] )], 0x0400000);
	memcpy(&SNDROM[0x0800000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][1] )], 0x0400000);
	memcpy(&SNDROM[0x0c00000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][2] )], 0x0400000);
}


static READ16_HANDLER( rockn_soundvolume_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( rockn_soundvolume_w )
{
	rockn_soundvolume = data;
}

/***************************************************************************


                                Protection


***************************************************************************/

static READ16_HANDLER( tetrisp2_ip_1_word_r )
{
	return	( readinputportbytag("IN1") &  0xfcff ) |
			(           rand() & ~0xfcff ) |
			(      1 << (8 + (rand()&1)) );
}


/***************************************************************************


                                    NVRAM


***************************************************************************/

static UINT16 *tetrisp2_nvram;
static size_t tetrisp2_nvram_size;

NVRAM_HANDLER( tetrisp2 )
{
	if (read_or_write)
		mame_fwrite(file,tetrisp2_nvram,tetrisp2_nvram_size);
	else
	{
		if (file)
			mame_fread(file,tetrisp2_nvram,tetrisp2_nvram_size);
		else
		{
			/* fill in the default values */
			memset(tetrisp2_nvram,0,tetrisp2_nvram_size);
		}
	}
}


/* The game only ever writes even bytes and reads odd bytes */
READ16_HANDLER( tetrisp2_nvram_r )
{
	return	( (tetrisp2_nvram[offset] >> 8) & 0x00ff ) |
			( (tetrisp2_nvram[offset] << 8) & 0xff00 ) ;
}

WRITE16_HANDLER( tetrisp2_nvram_w )
{
	COMBINE_DATA(&tetrisp2_nvram[offset]);
}

READ16_HANDLER( rockn_nvram_r )
{
	return	tetrisp2_nvram[offset];
}


/***************************************************************************





***************************************************************************/

static UINT16 rocknms_main2sub;
static UINT16 rocknms_sub2main;

READ16_HANDLER( rocknms_main2sub_r )
{
	return rocknms_main2sub;
}

WRITE16_HANDLER( rocknms_main2sub_w )
{
	if (ACCESSING_LSB)
		rocknms_main2sub = (data ^ 0xffff);
}

READ16_HANDLER( rocknms_port_0_r )
{
	return ((readinputport(0) & 0xfffc ) | (rocknms_sub2main & 0x0003));
}

WRITE16_HANDLER( rocknms_sub2main_w )
{
	if (ACCESSING_LSB)
		rocknms_sub2main = (data ^ 0xffff);
}


WRITE16_HANDLER( tetrisp2_coincounter_w )
{
	coin_counter_w( 0, (data & 0x0001));
}


/***************************************************************************


                                Memory Map


***************************************************************************/

static ADDRESS_MAP_START( tetrisp2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM				)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_READ(MRA16_RAM				)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_READ(MRA16_RAM				)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_READ(tetrisp2_priority_r	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x400000, 0x403fff) AM_READ(MRA16_RAM				)	// Foreground
	AM_RANGE(0x404000, 0x407fff) AM_READ(MRA16_RAM				)	// Background
	AM_RANGE(0x408000, 0x409fff) AM_READ(MRA16_RAM				)	// ???
	AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM				)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_READ(MRA16_RAM				)	// Rotation
	AM_RANGE(0x650000, 0x651fff) AM_READ(MRA16_RAM				)	// Rotation (mirror)
	AM_RANGE(0x800002, 0x800003) AM_READ(tetrisp2_sound_r		)	// Sound
	AM_RANGE(0x900000, 0x903fff) AM_READ(tetrisp2_nvram_r	)	// NVRAM
	AM_RANGE(0x904000, 0x907fff) AM_READ(tetrisp2_nvram_r	)	// NVRAM (mirror)
	AM_RANGE(0xbe0000, 0xbe0001) AM_READ(MRA16_NOP				)	// INT-level1 dummy read
	AM_RANGE(0xbe0002, 0xbe0003) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xbe0004, 0xbe0005) AM_READ(tetrisp2_ip_1_word_r	)	// Inputs & protection
	AM_RANGE(0xbe0008, 0xbe0009) AM_READ(input_port_2_word_r	)	// Inputs
	AM_RANGE(0xbe000a, 0xbe000b) AM_READ(watchdog_reset16_r	)	// Watchdog
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetrisp2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM									)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_WRITE(MWA16_RAM									)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_WRITE(MWA16_RAM									)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(tetrisp2_priority_w) AM_BASE(&tetrisp2_priority	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(tetrisp2_palette_w) AM_BASE(&paletteram16			)	// Palette
	AM_RANGE(0x400000, 0x403fff) AM_WRITE(tetrisp2_vram_fg_w) AM_BASE(&tetrisp2_vram_fg		)	// Foreground
	AM_RANGE(0x404000, 0x407fff) AM_WRITE(tetrisp2_vram_bg_w) AM_BASE(&tetrisp2_vram_bg		)	// Background
	AM_RANGE(0x408000, 0x409fff) AM_WRITE(MWA16_RAM									)	// ???
	AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM									)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_WRITE(tetrisp2_vram_rot_w) AM_BASE(&tetrisp2_vram_rot	)	// Rotation
	AM_RANGE(0x650000, 0x651fff) AM_WRITE(tetrisp2_vram_rot_w						)	// Rotation (mirror)
	AM_RANGE(0x800000, 0x800003) AM_WRITE(tetrisp2_sound_w							)	// Sound
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(tetrisp2_nvram_w) AM_BASE(&tetrisp2_nvram) AM_SIZE(&tetrisp2_nvram_size	)	// NVRAM
	AM_RANGE(0x904000, 0x907fff) AM_WRITE(tetrisp2_nvram_w							)	// NVRAM (mirror)
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(tetrisp2_coincounter_w					)	// Coin Counter
	AM_RANGE(0xb20000, 0xb20001) AM_WRITE(MWA16_NOP									)	// ???
	AM_RANGE(0xb40000, 0xb4000b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_fg			)	// Foreground Scrolling
	AM_RANGE(0xb40010, 0xb4001b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_bg			)	// Background Scrolling
	AM_RANGE(0xb4003e, 0xb4003f) AM_WRITE(MWA16_NOP									)	// scr_size
	AM_RANGE(0xb60000, 0xb6002f) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_rotregs				)	// Rotation Registers
	AM_RANGE(0xba0000, 0xba001f) AM_WRITE(tetrisp2_systemregs_w						)	// system param
	AM_RANGE(0xba001a, 0xba001b) AM_WRITE(MWA16_NOP									)	// Lev 4 irq ack
	AM_RANGE(0xba001e, 0xba001f) AM_WRITE(MWA16_NOP									)	// Lev 2 irq ack
ADDRESS_MAP_END

static ADDRESS_MAP_START( rockn1_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM				)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_READ(MRA16_RAM				)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_READ(MRA16_RAM				)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_READ(tetrisp2_priority_r	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x400000, 0x403fff) AM_READ(MRA16_RAM				)	// Foreground
	AM_RANGE(0x404000, 0x407fff) AM_READ(MRA16_RAM				)	// Background
	AM_RANGE(0x408000, 0x409fff) AM_READ(MRA16_RAM				)	// ???
	AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM				)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_READ(MRA16_RAM				)	// Rotation
	AM_RANGE(0x900000, 0x903fff) AM_READ(rockn_nvram_r		)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_READ(rockn_soundvolume_r	)	// Sound Volume
	AM_RANGE(0xa40002, 0xa40003) AM_READ(tetrisp2_sound_r		)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_READ(rockn_adpcmbank_r		)	// Sound Bank
	AM_RANGE(0xbe0000, 0xbe0001) AM_READ(MRA16_NOP				)	// INT-level1 dummy read
	AM_RANGE(0xbe0002, 0xbe0003) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xbe0004, 0xbe0005) AM_READ(input_port_1_word_r	)	// Inputs
	AM_RANGE(0xbe0008, 0xbe0009) AM_READ(input_port_2_word_r	)	// Inputs
	AM_RANGE(0xbe000a, 0xbe000b) AM_READ(watchdog_reset16_r	)	// Watchdog
ADDRESS_MAP_END

static ADDRESS_MAP_START( rockn1_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM									)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_WRITE(MWA16_RAM									)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_WRITE(MWA16_RAM									)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(rockn_priority_w) AM_BASE(&tetrisp2_priority	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(tetrisp2_palette_w) AM_BASE(&paletteram16			)	// Palette
	AM_RANGE(0x400000, 0x403fff) AM_WRITE(tetrisp2_vram_fg_w) AM_BASE(&tetrisp2_vram_fg		)	// Foreground
	AM_RANGE(0x404000, 0x407fff) AM_WRITE(tetrisp2_vram_bg_w) AM_BASE(&tetrisp2_vram_bg		)	// Background
	AM_RANGE(0x408000, 0x409fff) AM_WRITE(MWA16_RAM									)	// ???
	AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM									)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_WRITE(tetrisp2_vram_rot_w) AM_BASE(&tetrisp2_vram_rot	)	// Rotation
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(tetrisp2_nvram_w) AM_BASE(&tetrisp2_nvram) AM_SIZE(&tetrisp2_nvram_size	)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_WRITE(rockn_soundvolume_w						)	// Sound Volume
	AM_RANGE(0xa40000, 0xa40003) AM_WRITE(tetrisp2_sound_w							)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_WRITE(rockn_adpcmbank_w							)	// Sound Bank
	AM_RANGE(0xa48000, 0xa48001) AM_WRITE(MWA16_NOP									)	// YMZ280 Reset
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(tetrisp2_coincounter_w					)	// Coin Counter
	AM_RANGE(0xb20000, 0xb20001) AM_WRITE(MWA16_NOP									)	// ???
	AM_RANGE(0xb40000, 0xb4000b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_fg			)	// Foreground Scrolling
	AM_RANGE(0xb40010, 0xb4001b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_bg			)	// Background Scrolling
	AM_RANGE(0xb4003e, 0xb4003f) AM_WRITE(MWA16_NOP									)	// scr_size
	AM_RANGE(0xb60000, 0xb6002f) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_rotregs				)	// Rotation Registers
	AM_RANGE(0xba0000, 0xba001f) AM_WRITE(rockn_systemregs_w						)	// system param
	AM_RANGE(0xba001a, 0xba001b) AM_WRITE(MWA16_NOP									)	// Lev 4 irq ack
	AM_RANGE(0xba001e, 0xba001f) AM_WRITE(MWA16_NOP									)	// Lev 2 irq ack
ADDRESS_MAP_END

static ADDRESS_MAP_START( rockn2_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM				)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_READ(MRA16_RAM				)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_READ(MRA16_RAM				)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_READ(tetrisp2_priority_r	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_READ(MRA16_RAM				)	// Palette
	AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM				)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_READ(MRA16_RAM				)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA16_RAM				)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_READ(MRA16_RAM				)	// Background
	AM_RANGE(0x808000, 0x809fff) AM_READ(MRA16_RAM				)	// ???
	AM_RANGE(0x900000, 0x903fff) AM_READ(rockn_nvram_r		)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_READ(rockn_soundvolume_r	)	// Sound Volume
	AM_RANGE(0xa40002, 0xa40003) AM_READ(tetrisp2_sound_r		)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_READ(rockn_adpcmbank_r		)	// Sound Bank
	AM_RANGE(0xbe0000, 0xbe0001) AM_READ(MRA16_NOP				)	// INT-level1 dummy read
	AM_RANGE(0xbe0002, 0xbe0003) AM_READ(input_port_0_word_r	)	// Inputs
	AM_RANGE(0xbe0004, 0xbe0005) AM_READ(input_port_1_word_r	)	// Inputs
	AM_RANGE(0xbe0008, 0xbe0009) AM_READ(input_port_2_word_r	)	// Inputs
	AM_RANGE(0xbe000a, 0xbe000b) AM_READ(watchdog_reset16_r	)	// Watchdog
ADDRESS_MAP_END

static ADDRESS_MAP_START( rockn2_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM									)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_WRITE(MWA16_RAM									)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_WRITE(MWA16_RAM									)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(rockn_priority_w) AM_BASE(&tetrisp2_priority	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(tetrisp2_palette_w) AM_BASE(&paletteram16			)	// Palette
	AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM									)	// Line
	AM_RANGE(0x600000, 0x60ffff) AM_WRITE(tetrisp2_vram_rot_w) AM_BASE(&tetrisp2_vram_rot	)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_WRITE(tetrisp2_vram_fg_w) AM_BASE(&tetrisp2_vram_fg		)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_WRITE(tetrisp2_vram_bg_w) AM_BASE(&tetrisp2_vram_bg		)	// Background
	AM_RANGE(0x808000, 0x809fff) AM_WRITE(MWA16_RAM									)	// ???
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(tetrisp2_nvram_w) AM_BASE(&tetrisp2_nvram) AM_SIZE(&tetrisp2_nvram_size	)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_WRITE(rockn_soundvolume_w						)	// Sound Volume
	AM_RANGE(0xa40000, 0xa40003) AM_WRITE(tetrisp2_sound_w							)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_WRITE(rockn2_adpcmbank_w						)	// Sound Bank
	AM_RANGE(0xa48000, 0xa48001) AM_WRITE(MWA16_NOP									)	// YMZ280 Reset
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(tetrisp2_coincounter_w					)	// Coin Counter
	AM_RANGE(0xb20000, 0xb20001) AM_WRITE(MWA16_NOP									)	// ???
	AM_RANGE(0xb40000, 0xb4000b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_fg			)	// Foreground Scrolling
	AM_RANGE(0xb40010, 0xb4001b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_bg			)	// Background Scrolling
	AM_RANGE(0xb4003e, 0xb4003f) AM_WRITE(MWA16_NOP									)	// scr_size
	AM_RANGE(0xb60000, 0xb6002f) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_rotregs				)	// Rotation Registers
	AM_RANGE(0xba0000, 0xba001f) AM_WRITE(rockn_systemregs_w						)	// system param
	AM_RANGE(0xba001a, 0xba001b) AM_WRITE(MWA16_NOP									)	// Lev 4 irq ack
	AM_RANGE(0xba001e, 0xba001f) AM_WRITE(MWA16_NOP									)	// Lev 2 irq ack
ADDRESS_MAP_END

static ADDRESS_MAP_START( rocknms_main_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM				)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_READ(MRA16_RAM				)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_READ(MRA16_RAM				)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_READ(tetrisp2_priority_r	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_READ(MRA16_RAM				)	// Palette
//  AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM              )   // Line
	AM_RANGE(0x600000, 0x60ffff) AM_READ(MRA16_RAM				)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA16_RAM				)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_READ(MRA16_RAM				)	// Background
//  AM_RANGE(0x808000, 0x809fff) AM_READ(MRA16_RAM              )   // ???
	AM_RANGE(0x900000, 0x903fff) AM_READ(rockn_nvram_r		)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_READ(rockn_soundvolume_r	)	// Sound Volume
	AM_RANGE(0xa40002, 0xa40003) AM_READ(tetrisp2_sound_r		)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_READ(rockn_adpcmbank_r		)	// Sound Bank
	AM_RANGE(0xbe0000, 0xbe0001) AM_READ(MRA16_NOP				)	// INT-level1 dummy read
	AM_RANGE(0xbe0002, 0xbe0003) AM_READ(rocknms_port_0_r		)	// Inputs
	AM_RANGE(0xbe0004, 0xbe0005) AM_READ(input_port_1_word_r	)	// Inputs
	AM_RANGE(0xbe0008, 0xbe0009) AM_READ(input_port_2_word_r	)	// Inputs
	AM_RANGE(0xbe000a, 0xbe000b) AM_READ(watchdog_reset16_r	)	// Watchdog
ADDRESS_MAP_END

static ADDRESS_MAP_START( rocknms_main_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM									)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size	)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_WRITE(MWA16_RAM									)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_WRITE(MWA16_RAM									)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(rockn_priority_w) AM_BASE(&tetrisp2_priority	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(tetrisp2_palette_w) AM_BASE(&paletteram16			)	// Palette
//  AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM                                 )   // Line
	AM_RANGE(0x600000, 0x60ffff) AM_WRITE(tetrisp2_vram_rot_w) AM_BASE(&tetrisp2_vram_rot	)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_WRITE(tetrisp2_vram_fg_w) AM_BASE(&tetrisp2_vram_fg		)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_WRITE(tetrisp2_vram_bg_w) AM_BASE(&tetrisp2_vram_bg		)	// Background
//  AM_RANGE(0x808000, 0x809fff) AM_WRITE(MWA16_RAM                                 )   // ???
	AM_RANGE(0x900000, 0x903fff) AM_WRITE(tetrisp2_nvram_w) AM_BASE(&tetrisp2_nvram) AM_SIZE(&tetrisp2_nvram_size	)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_WRITE(rockn_soundvolume_w						)	// Sound Volume
	AM_RANGE(0xa40000, 0xa40003) AM_WRITE(tetrisp2_sound_w							)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_WRITE(rockn_adpcmbank_w						)	// Sound Bank
	AM_RANGE(0xa48000, 0xa48001) AM_WRITE(MWA16_NOP									)	// YMZ280 Reset
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(rocknms_main2sub_w						)	// MAIN -> SUB Communication
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(tetrisp2_coincounter_w					)	// Coin Counter
	AM_RANGE(0xb20000, 0xb20001) AM_WRITE(MWA16_NOP									)	// ???
	AM_RANGE(0xb40000, 0xb4000b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_fg			)	// Foreground Scrolling
	AM_RANGE(0xb40010, 0xb4001b) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_scroll_bg			)	// Background Scrolling
	AM_RANGE(0xb4003e, 0xb4003f) AM_WRITE(MWA16_NOP									)	// scr_size
	AM_RANGE(0xb60000, 0xb6002f) AM_WRITE(MWA16_RAM) AM_BASE(&tetrisp2_rotregs				)	// Rotation Registers
	AM_RANGE(0xba0000, 0xba001f) AM_WRITE(rockn_systemregs_w						)	// system param
	AM_RANGE(0xba001a, 0xba001b) AM_WRITE(MWA16_NOP									)	// Lev 4 irq ack
	AM_RANGE(0xba001e, 0xba001f) AM_WRITE(MWA16_NOP									)	// Lev 2 irq ack
ADDRESS_MAP_END

static ADDRESS_MAP_START( rocknms_sub_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM				)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM				)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_READ(MRA16_RAM				)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_READ(MRA16_RAM				)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_READ(rocknms_sub_priority_r	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_READ(MRA16_RAM				)	// Palette
//  AM_RANGE(0x500000, 0x50ffff) AM_READ(MRA16_RAM              )   // Line
	AM_RANGE(0x600000, 0x60ffff) AM_READ(MRA16_RAM				)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA16_RAM				)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_READ(MRA16_RAM				)	// Background
//  AM_RANGE(0x808000, 0x809fff) AM_READ(MRA16_RAM              )   // ???
	AM_RANGE(0x900000, 0x907fff) AM_READ(MRA16_RAM				)	// NVRAM
//  AM_RANGE(0xbe0000, 0xbe0001) AM_READ(MRA16_NOP              )   // INT-level1 dummy read
	AM_RANGE(0xbe0002, 0xbe0003) AM_READ(rocknms_main2sub_r		)	// MAIN -> SUB Communication
	AM_RANGE(0xbe000a, 0xbe000b) AM_READ(watchdog_reset16_r	)	// Watchdog
ADDRESS_MAP_END


static ADDRESS_MAP_START( rocknms_sub_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM									)	// ROM
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16_2) AM_SIZE(&spriteram_2_size	)	// Object RAM
	AM_RANGE(0x104000, 0x107fff) AM_WRITE(MWA16_RAM									)	// Spare Object RAM
	AM_RANGE(0x108000, 0x10ffff) AM_WRITE(MWA16_RAM									)	// Work RAM
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(rocknms_sub_priority_w) AM_BASE(&rocknms_sub_priority	)	// Priority
	AM_RANGE(0x300000, 0x31ffff) AM_WRITE(rocknms_sub_palette_w) AM_BASE(&paletteram16_2			)	// Palette
//  AM_RANGE(0x500000, 0x50ffff) AM_WRITE(MWA16_RAM                                 )   // Line
	AM_RANGE(0x600000, 0x60ffff) AM_WRITE(rocknms_sub_vram_rot_w) AM_BASE(&rocknms_sub_vram_rot	)	// Rotation
	AM_RANGE(0x800000, 0x803fff) AM_WRITE(rocknms_sub_vram_fg_w) AM_BASE(&rocknms_sub_vram_fg		)	// Foreground
	AM_RANGE(0x804000, 0x807fff) AM_WRITE(rocknms_sub_vram_bg_w) AM_BASE(&rocknms_sub_vram_bg		)	// Background
//  AM_RANGE(0x808000, 0x809fff) AM_WRITE(MWA16_RAM                                 )   // ???
	AM_RANGE(0x900000, 0x907fff) AM_WRITE(MWA16_RAM	)	// NVRAM
	AM_RANGE(0xa30000, 0xa30001) AM_WRITE(rockn_soundvolume_w						)	// Sound Volume
	AM_RANGE(0xa40000, 0xa40003) AM_WRITE(tetrisp2_sound_w							)	// Sound
	AM_RANGE(0xa44000, 0xa44001) AM_WRITE(rockn_adpcmbank_w						)	// Sound Bank
	AM_RANGE(0xa48000, 0xa48001) AM_WRITE(MWA16_NOP									)	// YMZ280 Reset
	AM_RANGE(0xb00000, 0xb00001) AM_WRITE(rocknms_sub2main_w					)	// MAIN <- SUB Communication
	AM_RANGE(0xb20000, 0xb20001) AM_WRITE(MWA16_NOP									)	// ???
	AM_RANGE(0xb40000, 0xb4000b) AM_WRITE(MWA16_RAM) AM_BASE(&rocknms_sub_scroll_fg			)	// Foreground Scrolling
	AM_RANGE(0xb40010, 0xb4001b) AM_WRITE(MWA16_RAM) AM_BASE(&rocknms_sub_scroll_bg			)	// Background Scrolling
	AM_RANGE(0xb4003e, 0xb4003f) AM_WRITE(MWA16_NOP									)	// scr_size
	AM_RANGE(0xb60000, 0xb6002f) AM_WRITE(MWA16_RAM) AM_BASE(&rocknms_sub_rotregs				)	// Rotation Registers
	AM_RANGE(0xba0000, 0xba001f) AM_WRITE(rocknms_sub_systemregs_w						)	// system param
	AM_RANGE(0xba001a, 0xba001b) AM_WRITE(MWA16_NOP									)	// Lev 4 irq ack
	AM_RANGE(0xba001e, 0xba001f) AM_WRITE(MWA16_NOP									)	// Lev 2 irq ack
	AM_RANGE(0xbe0002, 0xbe0003) AM_WRITE(rocknms_sub2main_w						)		// MAIN <- SUB Communication (mirror)
ADDRESS_MAP_END


/***************************************************************************


                                Input Ports


***************************************************************************/

/***************************************************************************
                            Tetris Plus 2 (World)
***************************************************************************/

#define TETPLUS2_COMMON\
	PORT_START_TAG("IN0") /*$be0002.w*/ \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)\
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)\
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)\
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)\
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)\
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)\
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)\
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)\
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)\
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)\
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)\
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)\
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)\
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)\
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_START_TAG("IN1") /*$be0004.w*/\
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )\
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )\
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )\
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )\
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )\
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ?*/\
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ?*/\
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )\
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )


INPUT_PORTS_START( tetrisp2 )
TETPLUS2_COMMON

	PORT_START_TAG("IN2") //$be0008.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Language ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Japanese ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( English ) )
	PORT_DIPNAME( 0x1000, 0x1000, "F.B.I Logo" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
                            Tetris Plus 2 (Japan)
***************************************************************************/


INPUT_PORTS_START( teplus2j )
TETPLUS2_COMMON

/*
    The code for checking the "service mode" and "free play" DSWs
    is (deliberately?) bugged in this set
*/
	PORT_START_TAG("IN2")	// $be0008.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNUSED  ) // Language dip
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )	// F.B.I. Logo (in the USA set?)
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
                            Rock'n Tread (Japan)
***************************************************************************/


INPUT_PORTS_START( rockn )
	PORT_START_TAG("IN0")	//$be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	//$be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_LOW)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN2")	//$be0008.w
	PORT_DIPNAME( 0x0001, 0x0001, "DIPSW 1-1") // All these used to be marked 'Cheat', can't think why.
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIPSW 1-2")
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIPSW 1-3")
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIPSW 1-4")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 1-5")
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIPSW 1-6")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIPSW 1-7")
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIPSW 1-8")
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME(0x0100, 0x0100, "DIPSW 2-1")
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(0x0200, 0x0200, "DIPSW 2-2")
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x0400, 0x0400, "DIPSW 2-3")
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x0800, 0x0800, "DIPSW 2-4")
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x1000, 0x1000, "DIPSW 2-5")
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x2000, 0x2000, "DIPSW 2-6")
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x4000, 0x4000, "DIPSW 2-7")
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x8000, 0x8000, "DIPSW 2-8")
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( rocknms )
	PORT_START	// IN0 - $be0002.w

	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_LOW)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN2")	//$be0008.w
	PORT_DIPNAME( 0x0001, 0x0001, "DIPSW 1-1") // All these used to be marked 'Cheat', can't think why.
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIPSW 1-2")
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIPSW 1-3")
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIPSW 1-4")
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIPSW 1-5")
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIPSW 1-6")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIPSW 1-7")
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIPSW 1-8")
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME(0x0100, 0x0100, "DIPSW 2-1")
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(0x0200, 0x0200, "DIPSW 2-2")
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x0400, 0x0400, "DIPSW 2-3")
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x0800, 0x0800, "DIPSW 2-4")
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x1000, 0x1000, "DIPSW 2-5")
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x2000, 0x2000, "DIPSW 2-6")
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x4000, 0x4000, "DIPSW 2-7")
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME(    0x8000, 0x8000, "DIPSW 2-8")
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



/***************************************************************************


                            Graphics Layouts


***************************************************************************/


/* 8x8x8 tiles */
static const gfx_layout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

/* 16x16x8 tiles */
static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};

static const gfx_decode tetrisp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ -1 }
};

static const gfx_decode rocknms_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ REGION_GFX5, 0, &layout_8x8x8,   0x8000, 0x10 }, // [0] Sprites
	{ REGION_GFX6, 0, &layout_16x16x8, 0x9000, 0x10 }, // [1] Background
	{ REGION_GFX7, 0, &layout_16x16x8, 0xa000, 0x10 }, // [2] Rotation
	{ REGION_GFX8, 0, &layout_8x8x8,   0xe000, 0x10 }, // [3] Foreground
	{ -1 }
};


/***************************************************************************


                                Machine Drivers


***************************************************************************/

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0	// irq
};

void rockn_timer_level4_callback(int param)
{
	cpunum_set_input_line(0, 4, HOLD_LINE);
}

void rockn_timer_sub_level4_callback(int param)
{
	cpunum_set_input_line(1, 4, HOLD_LINE);
}


void rockn_timer_level1_callback(int param)
{
	cpunum_set_input_line(0, 1, HOLD_LINE);
}

void rockn_timer_sub_level1_callback(int param)
{
	cpunum_set_input_line(1, 1, HOLD_LINE);
}

DRIVER_INIT( rockn_timer )
{
	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_level1_callback);
	rockn_timer_l4 = timer_alloc(rockn_timer_level4_callback);

	state_save_register_global_array(tetrisp2_systemregs);
	state_save_register_global_array(rocknms_sub_systemregs);
	state_save_register_global(rockn_protectdata);
	state_save_register_global(rockn_adpcmbank);
	state_save_register_global(rockn_soundvolume);
}

DRIVER_INIT( rockn )
{
	init_rockn_timer();
	rockn_protectdata = 1;
}

DRIVER_INIT( rockn1 )
{
	init_rockn_timer();
	rockn_protectdata = 1;
}

DRIVER_INIT( rockn2 )
{
	init_rockn_timer();
	rockn_protectdata = 2;
}

DRIVER_INIT( rocknms )
{
	init_rockn_timer();

	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_sub_level1_callback);
	rockn_timer_sub_l4 = timer_alloc(rockn_timer_sub_level4_callback);

	rockn_protectdata = 3;

}

DRIVER_INIT( rockn3 )
{
	init_rockn_timer();
	rockn_protectdata = 4;
}


static MACHINE_DRIVER_START( tetrisp2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(tetrisp2_readmem,tetrisp2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(tetrisp2)
	MDRV_VIDEO_UPDATE(tetrisp2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rockn )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(rockn1_readmem,rockn1_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



static MACHINE_DRIVER_START( rockn2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(rockn2_readmem,rockn2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rocknms )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(rocknms_main_readmem,rocknms_main_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(rocknms_sub_readmem,rocknms_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_ASPECT_RATIO(4, 7)
	MDRV_SCREEN_SIZE(0x140, 0xe0+0x140)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0+0x140-1)
	MDRV_GFXDECODE(rocknms_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(rocknms)
	MDRV_VIDEO_UPDATE(rocknms)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************


                                ROMs Loading


***************************************************************************/


/***************************************************************************

                                Tetris Plus 2

(C) Jaleco 1996

TP-97222
96019 EB-00-20117-0
MDK332V-0

BRIEF HARDWARE OVERVIEW

Toshiba TMP68HC000P-12
Yamaha YMZ280B-F
OSC: 12.000MHz, 48.000MHz, 16.9344MHz

Listing of custom chips. (Some on scan are hard to read).

IC38    JALECO SS91022-03 9428XX001
IC31    JALECO SS91022-05 9347EX002
IC32    JALECO GS91022-05    048  9726HX002
IC30    JALECO GS91022-04 9721PD008
IC39    XILINX XC5210 PQ240C X68710M AKJ9544
IC49    XILINX XC7336 PC44ACK9633 A63458A

***************************************************************************/

ROM_START( tetrisp2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "t2p_04.rom", 0x000000, 0x080000, CRC(e67f9c51) SHA1(d8b2937699d648267b163c7c3f591426877f3701) )
	ROM_LOAD16_BYTE( "t2p_01.rom", 0x000001, 0x080000, CRC(5020a4ed) SHA1(9c0f02fe3700761771ac026a2e375144e86e5eb7) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, CRC(06f7dc64) SHA1(722c51b707b9854c0293afdff18b27ec7cae6719) )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, CRC(3e613bed) SHA1(038b5e43fa3d69654107c8093126eeb2e8fa4ddc) )
	/* If t2p_m01&2 from this board were correctly read, since they
       hold the same data of the above but with swapped halves, it
       means they had to invert the top bit of the "page select"
       register in the sprite's hardware on this board! */

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, CRC(16f7093c) SHA1(2be77c6a692c5d762f5553ae24e8c415ab194cc6) )
	ROM_LOAD( "96019-04.6",  0x400000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, CRC(34dd1bad) SHA1(9bdf1dde11f82839676400de5dd7acb06ea8cdb2) )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, CRC(a8a61954) SHA1(86c3db10b348ba1f44ff696877b8b20845fa53de) )

ROM_END


/***************************************************************************

                            Tetris Plus 2 (Japan)

(c)1997 Jaleco / The Tetris Company

TP-97222
96019 EB-00-20117-0

CPU:    68000-12
Sound:  YMZ280B-F
OSC:    12.000MHz
        48.0000MHz
        16.9344MHz

Custom: SS91022-03
        GS91022-04
        GS91022-05
        SS91022-05

***************************************************************************/

ROM_START( teplus2j )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tet2-4v2.2", 0x000000, 0x080000, CRC(5bfa32c8) SHA1(55fb2872695fcfbad13f5c0723302e72da69e44a) )	// v2.2
	ROM_LOAD16_BYTE( "tet2-1v2.2", 0x000001, 0x080000, CRC(919116d0) SHA1(3e1c0fd4c9175b2900a4717fbb9e8b591c5f534d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, CRC(06f7dc64) SHA1(722c51b707b9854c0293afdff18b27ec7cae6719) )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, CRC(3e613bed) SHA1(038b5e43fa3d69654107c8093126eeb2e8fa4ddc) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, CRC(16f7093c) SHA1(2be77c6a692c5d762f5553ae24e8c415ab194cc6) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, CRC(34dd1bad) SHA1(9bdf1dde11f82839676400de5dd7acb06ea8cdb2) )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, CRC(a8a61954) SHA1(86c3db10b348ba1f44ff696877b8b20845fa53de) )

ROM_END

/***

Rock 'n' Tread

***/
ROM_START( rockn )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "98344_1.bin", 0x000001, 0x80000, CRC(4cf79e58) SHA1(f50e596d43c9ab2072ae0476169eee2a8512fd8d) )
	ROM_LOAD16_BYTE( "98344_4.bin", 0x000000, 0x80000, CRC(caa33f79) SHA1(8ccff67091dac5ad871cae6cdb31e1fc37c1a4c2) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "98344_8.bin", 0x000002, 0x200000, CRC(fa3f6f9c) SHA1(586dcc690a1a4aa7c97932ad496382def6a074a4) )
	ROM_LOAD32_WORD( "98344_9.bin", 0x000000, 0x200000, CRC(3d12a688) SHA1(356b2ea81d960838b604c5a17cc77e79fb0e40ce) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "98344_13.bin", 0x000000, 0x200000, CRC(261b99a0) SHA1(7b3c768ae9d7429e2559fe32c1a4ff220d727e7e) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "98344_6.bin", 0x000000, 0x100000, CRC(5551717f) SHA1(64943a9a68ad4074f3f5128d7796e4f03baa14d5) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "98344_10.bin", 0x000000, 0x080000, CRC(918663a8) SHA1(aedacb741c986ef8159385cfef866cb7e3ef6cb6) )

	/* from the bootleg set, are they right for this? */
	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(c354f753) SHA1(bf538c02e2162a93d8c6793a1211e21480156223)  ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) // BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(5b42999e) SHA1(376c773f292eae8b75db11bad3cb6ec5fe48392e)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(8306f302) SHA1(8c0437d7ab8d74d4d15f4a641d30602e39cdd99d)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(3fda842c) SHA1(2b9e7c548b689bab491237e36a2dcf4782a81d79)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(86d4f289) SHA1(908490ab0cf8d33cf3e127f71edee3bece70b86d)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(f8dbf47d) SHA1(f19f7ae26e3b8af17a4e66e6722dd2f5c36d33f8)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(525aff97) SHA1(b18e5bdf67d3a89f39c59f4f9bd3bb608dacc7f7)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(5bd8bb95) SHA1(3b33c42778f7d50ca1513d37e7bc4a4efcc3cf82)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(304c1643) SHA1(0be090077e00d4b9abce2fac4821c630b6a40f22)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(78c22c56) SHA1(eb48d188d25538a1d381ca760f8e98096ee12bfe)  ) // bank 2
	ROM_LOAD( "sound10", 0x3400000, 0x0400000, CRC(d5e8d8a5) SHA1(df7db3c8b110ce1aa85e627537afb744c98877bd)  ) // bank 3
	ROM_LOAD( "sound11", 0x3800000, 0x0400000, CRC(569ef4dd) SHA1(777f8a3aef741655555364d00a1eaa472ac4b922)  ) // bank 3
	ROM_LOAD( "sound12", 0x3c00000, 0x0400000, CRC(aae8d59c) SHA1(ccca1f511ce0ea8d452f3b1d24350b5cee402ad2)  ) // bank 3
	ROM_LOAD( "sound13", 0x4000000, 0x0400000, CRC(9ec1459b) SHA1(10e08a47636dec431cdb8e105cf61287fe9c6637)  ) // bank 4
	ROM_LOAD( "sound14", 0x4400000, 0x0400000, CRC(b26f9a81) SHA1(0d1c8e382eb5877f9a748ff289be97cbdb73b0cc)  ) // bank 4
ROM_END

/***************************************************************************

                            Rock'n Tread 1 (Japan)
                            Rock'n Tread 2 (Japan)
                            Rock'n MegaSession (Japan)
                            Rock'n 3 (Japan)
                            Rock'n 4 (Japan)

(c)1997 Jaleco

CPU:    68000-12
Sound:  YMZ280B-F
OSC:    12.000MHz
        48.0000MHz
        16.9344MHz

Custom: SS91022-03
        GS91022-04
        GS91022-05
        SS91022-05

***************************************************************************/

ROM_START( rockna )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg1", 0x000001, 0x80000, CRC(6078fa48) SHA1(e98c1a1abf026f2d5b5035ccbc9d412a08ca1f02) )
	ROM_LOAD16_BYTE( "prg0", 0x000000, 0x80000, CRC(c8310bd0) SHA1(1efee954cc94b668b7d9f28a099b8d1c83d3093f) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "98344_8.bin", 0x000002, 0x200000, CRC(fa3f6f9c) SHA1(586dcc690a1a4aa7c97932ad496382def6a074a4) )
	ROM_LOAD32_WORD( "98344_9.bin", 0x000000, 0x200000, CRC(3d12a688) SHA1(356b2ea81d960838b604c5a17cc77e79fb0e40ce) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "98344_13.bin", 0x000000, 0x200000, CRC(261b99a0) SHA1(7b3c768ae9d7429e2559fe32c1a4ff220d727e7e) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "98344_6.bin", 0x000000, 0x100000, CRC(5551717f) SHA1(64943a9a68ad4074f3f5128d7796e4f03baa14d5) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "98344_10.bin", 0x000000, 0x080000, CRC(918663a8) SHA1(aedacb741c986ef8159385cfef866cb7e3ef6cb6) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(c354f753) SHA1(bf538c02e2162a93d8c6793a1211e21480156223)  ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) // BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(5b42999e) SHA1(376c773f292eae8b75db11bad3cb6ec5fe48392e)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(8306f302) SHA1(8c0437d7ab8d74d4d15f4a641d30602e39cdd99d)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(3fda842c) SHA1(2b9e7c548b689bab491237e36a2dcf4782a81d79)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(86d4f289) SHA1(908490ab0cf8d33cf3e127f71edee3bece70b86d)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(f8dbf47d) SHA1(f19f7ae26e3b8af17a4e66e6722dd2f5c36d33f8)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(525aff97) SHA1(b18e5bdf67d3a89f39c59f4f9bd3bb608dacc7f7)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(5bd8bb95) SHA1(3b33c42778f7d50ca1513d37e7bc4a4efcc3cf82)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(304c1643) SHA1(0be090077e00d4b9abce2fac4821c630b6a40f22)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(78c22c56) SHA1(eb48d188d25538a1d381ca760f8e98096ee12bfe)  ) // bank 2
	ROM_LOAD( "sound10", 0x3400000, 0x0400000, CRC(d5e8d8a5) SHA1(df7db3c8b110ce1aa85e627537afb744c98877bd)  ) // bank 3
	ROM_LOAD( "sound11", 0x3800000, 0x0400000, CRC(569ef4dd) SHA1(777f8a3aef741655555364d00a1eaa472ac4b922)  ) // bank 3
	ROM_LOAD( "sound12", 0x3c00000, 0x0400000, CRC(aae8d59c) SHA1(ccca1f511ce0ea8d452f3b1d24350b5cee402ad2)  ) // bank 3
	ROM_LOAD( "sound13", 0x4000000, 0x0400000, CRC(9ec1459b) SHA1(10e08a47636dec431cdb8e105cf61287fe9c6637)  ) // bank 4
	ROM_LOAD( "sound14", 0x4400000, 0x0400000, CRC(b26f9a81) SHA1(0d1c8e382eb5877f9a748ff289be97cbdb73b0cc)  ) // bank 4

ROM_END


ROM_START( rockn2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg1", 0x000001, 0x80000, CRC(854b5a45) SHA1(91496bc511fef1d552d2bd00b82d2470eae94528) )
	ROM_LOAD16_BYTE( "prg0", 0x000000, 0x80000, CRC(4665bbd2) SHA1(3562c67b81a32d178a8bcb872e676bf7284855d7) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr1", 0x000002, 0x200000, CRC(673ce2c2) SHA1(6c0a13de386b02a7f3a86e8128374938ede2525c) )
	ROM_LOAD32_WORD( "spr0", 0x000000, 0x200000, CRC(9d3968cf) SHA1(11c96e7685ab8c1b416396238ec5c12e7819385f) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "back", 0x000000, 0x200000, CRC(e35c55b3) SHA1(a18367c28befc3f71823f1d4ab2126ad6f8a28fc)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot", 0x000000, 0x200000, CRC(241d7449) SHA1(9fcc2d128d7be273836460313c0e73c81e33c9cb)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "front", 0x000000, 0x080000, CRC(ae74d5b3) SHA1(07aa6ee540a783e3f2a8710a7095d922cff1d443)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(4e9611a3) SHA1(2a9b1d5afc0ea9a3285f9fc6b49a1c3abd8cd2a5)  ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(ec600f13) SHA1(151cb0a16782c8bba223d0f6881b80c1e43bc9bc)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(8306f302) SHA1(8c0437d7ab8d74d4d15f4a641d30602e39cdd99d)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(3fda842c) SHA1(2b9e7c548b689bab491237e36a2dcf4782a81d79)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(86d4f289) SHA1(908490ab0cf8d33cf3e127f71edee3bece70b86d)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(f8dbf47d) SHA1(f19f7ae26e3b8af17a4e66e6722dd2f5c36d33f8)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(06f7bd63) SHA1(d8b27212ebba99f5129483550aeac5b86ff2a1d2)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(22f042f6) SHA1(649bdf43dd698150992e68b23fd758bca56c615b)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(dd294d8e) SHA1(49d889d341ab6167d9741340eb27902923b6cb42)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(8fedee6e) SHA1(540c01d1c5f410abb1f86f33a5a532208946cb7c)  ) // bank 2
	ROM_LOAD( "sound10", 0x3400000, 0x0400000, CRC(01292f11) SHA1(da88b14bf8df34e7574cf8c9f5dd385db13ab34c)  ) // bank 3
	ROM_LOAD( "sound11", 0x3800000, 0x0400000, CRC(20dc76ba) SHA1(078397f2de54d4ca91035dce11419ac0d934fbfa)  ) // bank 3
	ROM_LOAD( "sound12", 0x3c00000, 0x0400000, CRC(11fff0bc) SHA1(2767fcc3a5d3200750b011c97a83073719a9325f)  ) // bank 3
	ROM_LOAD( "sound13", 0x4000000, 0x0400000, CRC(2367dd18) SHA1(b58f757ce4c832c5462637f4e08d7be511ca0c96)  ) // bank 4
	ROM_LOAD( "sound14", 0x4400000, 0x0400000, CRC(75ced8c0) SHA1(fda17464767be073a36c117f5212411b66197dd9)  ) // bank 4
	ROM_LOAD( "sound15", 0x4800000, 0x0400000, CRC(aeaca380) SHA1(1c389911aa766abec389b1c79a1542759ac58b9f)  ) // bank 4
	ROM_LOAD( "sound16", 0x4c00000, 0x0400000, CRC(21d50e32) SHA1(24eaceb7c0b868b6e8fc16b403dae2427e422bf6)  ) // bank 5
	ROM_LOAD( "sound17", 0x5000000, 0x0400000, CRC(de785a2a) SHA1(1f5ae46ac9476a31a431ce0f0cf124e1c8c930a6)  ) // bank 5
	ROM_LOAD( "sound18", 0x5400000, 0x0400000, CRC(18cabb1e) SHA1(c769820e2e84eff0e4ce956236656ae757e3299c)  ) // bank 5
	ROM_LOAD( "sound19", 0x5800000, 0x0400000, CRC(33c89e53) SHA1(7d216f5db6b30c9b05a9a77030498ff68ae6fbad)  ) // bank 6
	ROM_LOAD( "sound20", 0x5c00000, 0x0400000, CRC(89c1b088) SHA1(9b4118815959a5fb65b2a293015f592a46f4126f)  ) // bank 6
	ROM_LOAD( "sound21", 0x6000000, 0x0400000, CRC(13db74bd) SHA1(ab87438bbac97d46b1b8195b61dca1d72172a621)  ) // bank 6

ROM_END


ROM_START( rockn3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg1", 0x000001, 0x80000, CRC(abc6ab4a) SHA1(2f1983b95cd9e42d709edac5613b1f0b450df4ba) )
	ROM_LOAD16_BYTE( "prg0", 0x000000, 0x80000, CRC(3ecba46e) SHA1(64ff5b7932a8d8dc01c649b9dcc1d55cf1e43387) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr1", 0x000002, 0x200000, CRC(468bf696) SHA1(d58e399ff876ab0f4ef52aaa85d86d72db307b6a) )
	ROM_LOAD32_WORD( "spr0", 0x000000, 0x200000, CRC(8a61fc18) SHA1(4e895a2014e711d044ed5d8bff8a91766f14b307) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "back", 0x000000, 0x200000, CRC(e01bf471) SHA1(4485c71770bdb8800ded4afb37814c2d287b78be)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot", 0x000000, 0x200000, CRC(4e146de5) SHA1(5971cbb91da5fde652786d82d0143197518bad9b)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "front", 0x000000, 0x080000, CRC(8100039e) SHA1(e07b1e2f3cbcb1c086edd628d20423ecd4f74860)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(e2f69042) SHA1(deb361a53ed6a9033e21c2f805f327cc3e9b11c6)  ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) 		 // BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(b328b18f) SHA1(22edebcabd6c8ed65d8c9e501621991d404c430d)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(f46438e3) SHA1(718f54fc0e3689f5ab29bef2ec13eb2aa9b117fc)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(b979e887) SHA1(10852ceb1b9e24fb87cf9339bc9fb4ae066a1221)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(0bb2c212) SHA1(4f8ab3c96c3e1aa337a3fe871cffc04ec603f8c0)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(3116e437) SHA1(f1b06592a6f0eba92eb4511d3ca03a3bb51e8c9d)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(26b37ef6) SHA1(f7090f3ec81f0c651c53d460b476e63f52dd06dc)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(1dd3f4e3) SHA1(8474e00b962368164c717e5fe2e926852f3b4426)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(a1b03d67) SHA1(95f89a37e97d62706e15fd5571ff2e70dd98fee2)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(35107aac) SHA1(d56a66e15c46c33cf6c9c28edf48b730b681d21a)  ) // bank 2
	ROM_LOAD( "sound10", 0x3400000, 0x0400000, CRC(059ec592) SHA1(205210af558eb7e8e1399b2a506ef0285c5feda3)  ) // bank 3
	ROM_LOAD( "sound11", 0x3800000, 0x0400000, CRC(84d4badb) SHA1(fc20f97a008f000a49e7cadd559789516643704a)  ) // bank 3
	ROM_LOAD( "sound12", 0x3c00000, 0x0400000, CRC(4527a9b7) SHA1(a73ebece5c84bf14f8d25bbd869b7b43b1fcd042)  ) // bank 3
	ROM_LOAD( "sound13", 0x4000000, 0x0400000, CRC(bfa4b7ce) SHA1(4100f2deabb8994e8e3ff897a1db13693ab64c11)  ) // bank 4
	ROM_LOAD( "sound14", 0x4400000, 0x0400000, CRC(a2ccd2ce) SHA1(fc6325219f7b8e68c22a129f5ec4e900e326fb9d)  ) // bank 4
	ROM_LOAD( "sound15", 0x4800000, 0x0400000, CRC(95baf678) SHA1(f7b39a3379f16df0560a22d4f42165ebbe05cebe)  ) // bank 4
	ROM_LOAD( "sound16", 0x4c00000, 0x0400000, CRC(5883c84b) SHA1(54aec4e1e2f5edc198aebc4788caf5062f9a5b6c)  ) // bank 5
	ROM_LOAD( "sound17", 0x5000000, 0x0400000, CRC(f92098ce) SHA1(9b13cd37ad5d7baf36b20218c4bced956084ec45)  ) // bank 5
	ROM_LOAD( "sound18", 0x5400000, 0x0400000, CRC(dbb2c228) SHA1(f7cd24026236e2c616376c695b9e986cc221f36d)  ) // bank 5
	ROM_LOAD( "sound19", 0x5800000, 0x0400000, CRC(9efdae1c) SHA1(6158a1804fbaa9ce27ae7e12cfda5f49084b4998)  ) // bank 6
	ROM_LOAD( "sound20", 0x5c00000, 0x0400000, CRC(5f301b83) SHA1(e24e85c43a62871360545aa42dfa439045334b79)  ) // bank 6

ROM_END


ROM_START( rockn4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "prg1", 0x000001, 0x80000, CRC(c666caea) SHA1(57018de40d71fe214a6b5cc33c8ad5e88622d010) )
	ROM_LOAD16_BYTE( "prg0", 0x000000, 0x80000, CRC(cc94e557) SHA1(d38abed04239d9eecf1b1be7a9f765a1b7aa0d8d) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr1", 0x000002, 0x200000, CRC(5eeae537) SHA1(6bb8c658a2985c3919f0590a0147eead995c01c9) )
	ROM_LOAD32_WORD( "spr0", 0x000000, 0x200000, CRC(3fedddc9) SHA1(4bd8f402ecf8e6255326927e825179fa6d300e73) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "back", 0x000000, 0x200000, CRC(ead41e79) SHA1(9c24b1e52b6ed43d5b5a1caf48f2974b8fa61f4a)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot", 0x000000, 0x200000, CRC(eb16fc67) SHA1(5be40f2c9a5693785268eafcfcf348f147533463)  )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "front", 0x000000, 0x100000, CRC(37d50259) SHA1(fd02f98a981470c47889f0b2f813ce59373a4b42)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(918ea8eb) SHA1(0cd82859634635b6ce49db36fb91ed3365a101eb)  ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(c548e51e) SHA1(4fe1e35c9ed4366dce98b4f4c00f94e202ef15dc)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(ffda0253) SHA1(9b8ae98accc2f72a1cd881086f89e647e4904ad9)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(1f813af5) SHA1(a72d842e39b9fc955a2fc6721673b34b1b591e4a)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(035c4ff3) SHA1(9290c49244dc45ad5d6543775c5f2cc507e54e77)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(0f01f7b0) SHA1(e0c6daa1606dd5aaac59a7ae75d76e937e9c0151)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(31574b1c) SHA1(a08b50b4c4f2be32892b7534f2192101f8af6762)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(388e2c91) SHA1(493ef760858a82cbc38de59c4db3f273c0ddfdfb)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(6e7e3f23) SHA1(4b9b959f79254d0633f1c4324b7ee6a17e222308)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(39fa512f) SHA1(d07426bc74492496756b67b8ded1b507726720c7)  ) // bank 2
ROM_END


ROM_START( rocknms )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "mast_prg1", 0x000001, 0x80000, CRC(c36674f8) SHA1(8aeb19fcd6f786c9d76a72abee4b607d29fb7d56) )
	ROM_LOAD16_BYTE( "mast_prg0", 0x000000, 0x80000, CRC(69382065) SHA1(2d528c2954556d440e790db209a2e3563580296a) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "slav_prg1", 0x000001, 0x80000, CRC(769e2245) SHA1(5e6b5456fb213da887be4ef3739685360f6fdae5) )
	ROM_LOAD16_BYTE( "slav_prg0", 0x000000, 0x80000, CRC(55b8df65) SHA1(7744e7a75904174843fc6e3d54324839c6cf104d) )

	ROM_REGION( 0x0800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "mast_spr1", 0x000002, 0x400000, CRC(520152dc) SHA1(619a55352c0dab914f6188d66272a24495b5d1d4)  )
	ROM_LOAD32_WORD( "mast_spr0", 0x000000, 0x400000, CRC(1caad02a) SHA1(00c3fc849d1f633874fee30f7d0caf0c62735c50)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "mast_back", 0x000000, 0x200000, CRC(1ca30e3f) SHA1(763c9dd287c186b6ca8ecb88c3ce29d68fea9179)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "mast_rot", 0x000000, 0x200000, CRC(1f29b622) SHA1(aab6aafb98fa732266675daa63dc4c0d2084bcbd)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "mast_front", 0x000000, 0x080000, CRC(a4717579) SHA1(cf28c0f19713ebf9f8fd5d55d654c1cd2e8cd73d)  )

	ROM_REGION( 0x800000, REGION_GFX5, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "slav_spr1", 0x000002, 0x400000, CRC(3f8124b0) SHA1(c9ab89f559551d2298d28e107b2d44d312e53216) )
	ROM_LOAD32_WORD( "slav_spr0", 0x000000, 0x400000, CRC(48a7f5b1) SHA1(4724856bde3cf975efc3be407b60693a69a39365) )

	ROM_REGION( 0x200000, REGION_GFX6, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "slav_back", 0x000000, 0x200000, CRC(f0a28e32) SHA1(517b98dee6ec201bab02a3c81b0937ed462a626e) )

	ROM_REGION( 0x200000, REGION_GFX7, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "slav_rot", 0x000000, 0x200000, CRC(0bab21f4) SHA1(afd3f32d7bb99b3f566b302fce11059ae8788715) )

	ROM_REGION( 0x080000, REGION_GFX8, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "slav_front", 0x000000, 0x080000,  CRC(b65734a7) SHA1(80190e260ed32cb3355f0604722b85eb659483d0) )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sound00", 0x0000000, 0x0400000, CRC(8bafae71) SHA1(db74accd4bc1bfeb4a3341a0fd572b81287f1278)  ) // COMMON AREA
	ROM_FILL(                0x0400000, 0x0c00000, 0xffffffff ) 		// BANK AREA
	ROM_LOAD( "sound01", 0x1000000, 0x0400000, CRC(eec0589b) SHA1(f54c1c7e7741100a1398ebd45aef4755171d9965)  ) // bank 0
	ROM_LOAD( "sound02", 0x1400000, 0x0400000, CRC(564aa972) SHA1(b19e960fd79647e5bcca509982c9887decb92bc6)  ) // bank 0
	ROM_LOAD( "sound03", 0x1800000, 0x0400000, CRC(940302d0) SHA1(b28c2bb1a9b8cea0b6963ffa5d3ac26d90b0bffc)  ) // bank 0
	ROM_LOAD( "sound04", 0x1c00000, 0x0400000, CRC(766db7f8) SHA1(41cfcac2e8d4307f75c56d57431b841e6d64b23c)  ) // bank 1
	ROM_LOAD( "sound05", 0x2000000, 0x0400000, CRC(3a3002f9) SHA1(27b24b8a34a0b919e051e81a10e87aa300b11d8f)  ) // bank 1
	ROM_LOAD( "sound06", 0x2400000, 0x0400000, CRC(06b04df9) SHA1(4bfc7c05843b4533f238f5360230cb71d7a66d56)  ) // bank 1
	ROM_LOAD( "sound07", 0x2800000, 0x0400000, CRC(da74305e) SHA1(9dfb744f36ac8b3661006921dc482e941711f389)  ) // bank 2
	ROM_LOAD( "sound08", 0x2c00000, 0x0400000, CRC(b5a0aa48) SHA1(2deb2c1c97c259f5e79e9dc3cd8859548549a189)  ) // bank 2
	ROM_LOAD( "sound09", 0x3000000, 0x0400000, CRC(0fd4a088) SHA1(5c1ea8a14dee7ee885ce0c86fb463741599db44d)  ) // bank 2
	ROM_LOAD( "sound10", 0x3400000, 0x0400000, CRC(33c89e53) SHA1(7d216f5db6b30c9b05a9a77030498ff68ae6fbad)  ) // bank 3
	ROM_LOAD( "sound11", 0x3800000, 0x0400000, CRC(f9256a3f) SHA1(a3ec0845497d349c97222a1f986c252c8ca781e7)  ) // bank 3
	ROM_LOAD( "sound12", 0x3c00000, 0x0400000, CRC(b0a09f3e) SHA1(d2e37eb935d7ef7e887ff79a49bc11da11c31f3c)  ) // bank 3
	ROM_LOAD( "sound13", 0x4000000, 0x0400000, CRC(d5cee673) SHA1(85194c73c43b69bccbcc895f147d5251bb039c2a)  ) // bank 4
	ROM_LOAD( "sound14", 0x4400000, 0x0400000, CRC(b394aa8a) SHA1(68541d5d98e2d59d6a3096f0c10b74b6f5803722)  ) // bank 4
	ROM_LOAD( "sound15", 0x4800000, 0x0400000, CRC(6c791501) SHA1(8c67f070651493d6f7a2ef7b8a5f9e12c0181f67)  ) // bank 4
	ROM_LOAD( "sound16", 0x4c00000, 0x0400000, CRC(fe80159e) SHA1(b6a980d4f62dfeaa6f51a99518aa4d483fe338e5)  ) // bank 5
	ROM_LOAD( "sound17", 0x5000000, 0x0400000, CRC(142c1159) SHA1(dfabbe69119c84040d6368561e93514ce7bb91db)  ) // bank 5
	ROM_LOAD( "sound18", 0x5400000, 0x0400000, CRC(cc595d85) SHA1(5f725771d79e71d62b64bb18e2a51b839a6e4c7f)  ) // bank 5
	ROM_LOAD( "sound19", 0x5800000, 0x0400000, CRC(82b085a3) SHA1(5a5f2ed90d659bbad710c23b9df2a7dbb3c9acfe)  ) // bank 6
	ROM_LOAD( "sound20", 0x5c00000, 0x0400000, CRC(dd5e9680) SHA1(5a2826641ad75757ce4a583e0ea901d54d20ffca)  ) // bank 6
ROM_END

/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 1997, tetrisp2, 0,        tetrisp2, tetrisp2, 0,       ROT0,   "Jaleco / The Tetris Company", "Tetris Plus 2 (World?)", GAME_SUPPORTS_SAVE )
GAME( 1997, teplus2j, tetrisp2, tetrisp2, teplus2j, 0,       ROT0,   "Jaleco / The Tetris Company", "Tetris Plus 2 (Japan)", GAME_SUPPORTS_SAVE )

GAME( 1999, rockn,    0,        rockn,    rockn,   rockn,    ROT270, "Jaleco", "Rock'n Tread (Japan)", GAME_SUPPORTS_SAVE)
GAME( 1999, rockna,   rockn,    rockn,    rockn,   rockn1,   ROT270, "Jaleco", "Rock'n Tread (Japan, alternate)", GAME_SUPPORTS_SAVE)
GAME( 1999, rockn2,   0,        rockn2,   rockn,   rockn2,   ROT270, "Jaleco", "Rock'n Tread 2 (Japan)", GAME_SUPPORTS_SAVE)
GAME( 1999, rocknms,  0,        rocknms,  rocknms, rocknms,  ROT0,   "Jaleco", "Rock'n MegaSession (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
GAME( 1999, rockn3,   0,        rockn2,   rockn,   rockn3,   ROT270, "Jaleco", "Rock'n 3 (Japan)", GAME_SUPPORTS_SAVE)
GAME( 2000, rockn4,   0,        rockn2,   rockn,   rockn3,   ROT270, "Jaleco (PCCWJ)", "Rock'n 4 (Japan, prototype)", GAME_SUPPORTS_SAVE)
