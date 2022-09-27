/***************************************************************************

Some Dynax games using the second version of their blitter

driver by Luca Elia and Nicola Salmoria

CPU:    Z80
Sound:  various
VDP:    HD46505SP (6845) (CRT controller)
Custom: TC17G032AP-0246 (blitter)

----------------------------------------------------------------------------------------
Year + Game                 Board(s)                    Sound                       Palette
----------------------------------------------------------------------------------------
88 Hana no Mai              D1610088L1                  AY8912 YM2203        M5205  PROM
88 Hana Kochou              D201901L2 + D201901L1-0     AY8912 YM2203        M5205  PROM
89 Hana Oriduru             D2304268L                   AY8912        YM2413 M5205  RAM
89 Dragon Punch             D24?                               YM2203               PROM
89 Mahjong Friday           D2607198L1                                YM2413        PROM
89 Sports Match             D31?                               YM2203               PROM
90 Jong Tou Ki (1)          D1505178-A + D2711078L-B    AY8912 YM2203        M5205  PROM
90 Mahjong Campus Hunting   D3312108L1-1 + D23SUB1      AY8912        YM2413 M5205  RAM
90 7jigen no Youseitachi    D3707198L1 + D23SUB1        AY8912        YM2413 M5205  RAM
90 Mahjong Electron Base                                AY8912        YM2413        RAM
90 Neruton Haikujiradan     D4005208L1-1 + D4508308L-2  AY8912        YM2413 M5205  RAM
91 Mahjong Yarunara         D5512068L1-1 + D4508308L-2  AY8912        YM2413 M5205  RAM
91 Mahjong Angels           D5512068L1-1 + D6107068L-1  AY8912        YM2413 M5205  RAM
91 Mahjong Dial Q2          D5212298L-1                               YM2413        PROM
92 Quiz TV Gassyuukoku Q&Q  D5512068L1-2 + D6410288L-1  AY8912        YM2413 M5205  RAM
94 Maya                                                       YM2203                PROM
9? Inca                                                      TM2203               PROM
----------------------------------------------------------------------------------------
(1) quite different from the others: it has a slave Z80 and *two* blitters

Notes:
- In some games (drgpunch etc) there's a more complete service mode. To enter
  it, set the service mode dip switch and reset keeping start1 pressed.
  In hnkochou, keep F2 pressed and reset.

- sprtmtch and drgpunch are "clones", but the gfx are very different; sprtmtch
  is a trimmed down version, without all animations between levels.

- according to the readme, mjfriday should have a M5205. However there don't seem to be
  accesses to it, and looking at the ROMs I don't see ADPCM data. Note that apart from a
  minor difference in the memory map mjfriday and mjdialq2 are identical, and mjdialq2
  doesn't have a 5205 either. Therefore, I think it's either a mistake in the readme or
  the chip is on the board but unused.

TODO:
- Inputs are grossly mapped, especially for the card games.

- In the interleaved games, "reverse write" test in service mode is wrong due to
  interleaving. Correct behaviour? None of these games has a "flip screen" dip
  switch, while mjfriday and mjdialq2, which aren't interleaved, have it.

- Palette banking is not correct, see quiztvqq cross hatch test.

- Rom banking of the blitter roms: it can only address 0x100000 bytes.

- Scrolling / wrap enable is not correct in hnoridur type hardware. See the dynax
  logo in neruton: it has to do with writes to c3/c4 and there are 2 additional
  scroll registers at 64/66.

- 7jigen: priority 0x30 is ok when used in the "gals check", but is wrong during
  attract mode, where the girl is hidden by the background. Another possible
  priority issue in attract mode is when the balls scroll over the devil.


***************************************************************************/

#include "driver.h"
#include "includes/dynax.h"
#include "sound/ay8910.h"
#include "sound/2203intf.h"
#include "sound/3812intf.h"
#include "sound/msm5205.h"
#include "sound/2413intf.h"

/***************************************************************************


                                Interrupts


***************************************************************************/

/***************************************************************************
                                Sports Match
***************************************************************************/

UINT8 dynax_blitter_irq;
UINT8 dynax_sound_irq;
UINT8 dynax_vblank_irq;

/* It runs in IM 0, thus needs an opcode on the data bus */
void sprtmtch_update_irq(void)
{
	int irq	=	((dynax_sound_irq)   ? 0x08 : 0) |
				((dynax_vblank_irq)  ? 0x10 : 0) |
				((dynax_blitter_irq) ? 0x20 : 0) ;
	cpunum_set_input_line_and_vector(0, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xc7 | irq); /* rst $xx */
}

WRITE8_HANDLER( dynax_vblank_ack_w )
{
	dynax_vblank_irq = 0;
	sprtmtch_update_irq();
}

WRITE8_HANDLER( dynax_blitter_ack_w )
{
	dynax_blitter_irq = 0;
	sprtmtch_update_irq();
}

INTERRUPT_GEN( sprtmtch_vblank_interrupt )
{
	dynax_vblank_irq = 1;
	sprtmtch_update_irq();
}

void sprtmtch_sound_callback(int state)
{
	dynax_sound_irq = state;
	sprtmtch_update_irq();
}


/***************************************************************************
                            Jantouki - Main CPU
***************************************************************************/

UINT8 dynax_blitter2_irq;

/* It runs in IM 0, thus needs an opcode on the data bus */
void jantouki_update_irq(void)
{
	int irq	=	((dynax_blitter_irq)	? 0x08 : 0) |
				((dynax_blitter2_irq)	? 0x10 : 0) |
				((dynax_vblank_irq)		? 0x20 : 0) ;
	cpunum_set_input_line_and_vector(0, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xc7 | irq); /* rst $xx */
}

WRITE8_HANDLER( jantouki_vblank_ack_w )
{
	dynax_vblank_irq = 0;
	jantouki_update_irq();
}

WRITE8_HANDLER( jantouki_blitter_ack_w )
{
	dynax_blitter_irq = data;
	jantouki_update_irq();
}

WRITE8_HANDLER( jantouki_blitter2_ack_w )
{
	dynax_blitter2_irq = data;
	jantouki_update_irq();
}

INTERRUPT_GEN( jantouki_vblank_interrupt )
{
	dynax_vblank_irq = 1;
	jantouki_update_irq();
}


/***************************************************************************
                            Jantouki - Sound CPU
***************************************************************************/

UINT8 dynax_soundlatch_irq;
UINT8 dynax_sound_vblank_irq;

void jantouki_sound_update_irq(void)
{
	int irq	=	((dynax_sound_irq)			? 0x08 : 0) |
				((dynax_soundlatch_irq)		? 0x10 : 0) |
				((dynax_sound_vblank_irq)	? 0x20 : 0) ;
	cpunum_set_input_line_and_vector(1, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xc7 | irq); /* rst $xx */
}

INTERRUPT_GEN( jantouki_sound_vblank_interrupt )
{
	dynax_sound_vblank_irq = 1;
	jantouki_sound_update_irq();
}

WRITE8_HANDLER( jantouki_sound_vblank_ack_w )
{
	dynax_sound_vblank_irq = 0;
	jantouki_sound_update_irq();
}

void jantouki_sound_callback(int state)
{
	dynax_sound_irq = state;
	jantouki_sound_update_irq();
}


/***************************************************************************


                                Memory Maps


***************************************************************************/

/***************************************************************************
                                Sports Match
***************************************************************************/

static WRITE8_HANDLER( dynax_coincounter_0_w )
{
	coin_counter_w(0, data);
}
static WRITE8_HANDLER( dynax_coincounter_1_w )
{
	coin_counter_w(1, data);
}

static READ8_HANDLER( ret_ff )	{	return 0xff;	}


static int keyb;

static READ8_HANDLER( hanamai_keyboard_0_r )
{
	int res = 0x3f;

	/* the game reads all rows at once (keyb = 0) to check if a key is pressed */
	if (~keyb & 0x01) res &= readinputport(3);
	if (~keyb & 0x02) res &= readinputport(4);
	if (~keyb & 0x04) res &= readinputport(5);
	if (~keyb & 0x08) res &= readinputport(6);
	if (~keyb & 0x10) res &= readinputport(7);

	return res;
}

static READ8_HANDLER( hanamai_keyboard_1_r )
{
	return 0x3f;
}

static WRITE8_HANDLER( hanamai_keyboard_w )
{
	keyb = data;
}


static WRITE8_HANDLER( dynax_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&ROM[0x08000+0x8000*(data & 0x0f)]);
}

static WRITE8_HANDLER( jantouki_sound_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU2);
	memory_set_bankptr(2,&ROM[0x08000+0x8000*data]);
}


static int hnoridur_bank;

static WRITE8_HANDLER( hnoridur_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1) + 0x10000 + 0x8000*data;
//logerror("%04x: rom bank = %02x\n",activecpu_get_pc(),data);
	memory_set_bankptr(1,ROM);
	hnoridur_bank = data;
}

static UINT8 palette_ram[16*256*2];
static int palbank;

static WRITE8_HANDLER( hnoridur_palbank_w )
{
	palbank = data & 0x0f;
	dynax_blit_palbank_w(0,data);
}

static WRITE8_HANDLER( hnoridur_palette_w )
{
	switch (hnoridur_bank)
	{
		case 0x10:
			if (offset >= 0x100) return;
			palette_ram[256*palbank + offset + 16*256] = data;
			break;

		case 0x14:
			if (offset >= 0x100) return;
			palette_ram[256*palbank + offset] = data;
			break;

		// hnoridur: R/W RAM
		case 0x18:
		{
			UINT8 *RAM = memory_region(REGION_CPU1) + 0x10000 + hnoridur_bank * 0x8000;
			RAM[offset] = data;
			return;
		}

		default:
			ui_popup("palette_w with bank = %02x",hnoridur_bank);
			break;
	}

	{
		int x =	(palette_ram[256*palbank + offset]<<8) + palette_ram[256*palbank + offset + 16*256];
		/* The bits are in reverse order! */
		int r = BITSWAP8((x >>  0) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int g = BITSWAP8((x >>  5) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int b = BITSWAP8((x >> 10) & 0x1f, 7,6,5, 0,1,2,3,4 );
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(256*palbank + offset,r,g,b);
	}
}

static WRITE8_HANDLER( yarunara_palette_w )
{
	int addr = 512*palbank + offset;

	switch (hnoridur_bank)
	{
		case 0x10:
			palette_ram[addr] = data;
			break;

		case 0x1c:	// RTC
			return;

		default:
			ui_popup("palette_w with bank = %02x",hnoridur_bank);
			return;
	}

	{
		int br = palette_ram[addr & ~0x10];		// bbbrrrrr
		int bg = palette_ram[addr | 0x10];		// bb0ggggg
		int r = br & 0x1f;
		int g = bg & 0x1f;
		int b = ((bg & 0xc0)>>3) | ((br & 0xe0)>>5);
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color( 256*palbank + ((offset&0xf)|((offset&0x1e0)>>1)) ,r,g,b);
	}
}

static WRITE8_HANDLER( nanajign_palette_w )
{
	switch (hnoridur_bank)
	{
		case 0x10:
			palette_ram[256*palbank + offset + 16*256] = data;
			break;

		case 0x14:
			palette_ram[256*palbank + offset] = data;
			break;

		default:
			ui_popup("palette_w with bank = %02x",hnoridur_bank);
			break;
	}

	{
		int bg = palette_ram[256*palbank + offset];
		int br = palette_ram[256*palbank + offset + 16*256];
		int r = br & 0x1f;
		int g = bg & 0x1f;
		int b = ((bg & 0xc0)>>3) | ((br & 0xe0)>>5);
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(256*palbank + offset,r,g,b);
	}
}

static int msm5205next;
static int resetkludge;

static void adpcm_int(int data)
{
	static int toggle;

	MSM5205_data_w(0,msm5205next >> 4);
	msm5205next<<=4;

	toggle = 1 - toggle;
	if (toggle)
	{
		if (resetkludge)	// don't know what's wrong, but NMIs when the 5205 is reset make the game crash
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}
static void adpcm_int_cpu1(int data)
{
	static int toggle;

	MSM5205_data_w(0,msm5205next >> 4);
	msm5205next<<=4;

	toggle = 1 - toggle;
	if (toggle)
	{
		if (resetkludge)	// don't know what's wrong, but NMIs when the 5205 is reset make the game crash
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);	// cpu1
	}
}


static WRITE8_HANDLER( adpcm_data_w )
{
	msm5205next = data;
}

static WRITE8_HANDLER( adpcm_reset_w )
{
	resetkludge = data & 1;
	MSM5205_reset_w(0,~data & 1);
}

static MACHINE_RESET( adpcm )
{
	/* start with the MSM5205 reset */
	resetkludge = 0;
	MSM5205_reset_w(0,1);
}

static WRITE8_HANDLER( yarunara_layer_half_w )
{
	hanamai_layer_half_w(0,data >> 1);
}
static WRITE8_HANDLER( yarunara_layer_half2_w )
{
	hnoridur_layer_half2_w(0,data >> 1);
}
WRITE8_HANDLER( nanajign_layer_half_w )
{
	hanamai_layer_half_w(0,~data);
}


static ADDRESS_MAP_START( sprtmtch_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sprtmtch_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hnoridur_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( hnoridur_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(hnoridur_palette_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mcnpshnt_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( mcnpshnt_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(hnoridur_palette_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mjdialq2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( mjdialq2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x0800, 0x0fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1000, 0x1fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x2000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( yarunara_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM		)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM		)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM		)	// NVRAM
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1	)	// ROM (Banked)
ADDRESS_MAP_END
static ADDRESS_MAP_START( yarunara_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM				)	// ROM
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM				)	// RAM
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0x81ff) AM_WRITE(yarunara_palette_w	)	// Palette or RTC
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM				)	// ROM (Banked)
ADDRESS_MAP_END


static ADDRESS_MAP_START( nanajign_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( nanajign_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0x80ff) AM_WRITE(nanajign_palette_w	)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM				)	// ROM (Banked)
ADDRESS_MAP_END


static ADDRESS_MAP_START( jantouki_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( jantouki_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jantouki_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK2)
ADDRESS_MAP_END
static ADDRESS_MAP_START( jantouki_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hanamai_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x60, 0x60) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x61, 0x61) AM_READ(hanamai_keyboard_1_r	)	// P2
	AM_RANGE(0x62, 0x62) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x63, 0x63) AM_READ(ret_ff					)	// ?
	AM_RANGE(0x78, 0x78) AM_READ(YM2203_status_port_0_r	)	// YM2203
	AM_RANGE(0x79, 0x79) AM_READ(YM2203_read_port_0_r	)	// 2 x DSW
ADDRESS_MAP_END
static ADDRESS_MAP_START( hanamai_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x20, 0x20) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
	AM_RANGE(0x41, 0x47) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0x50, 0x50) AM_WRITE(dynax_rombank_w			)	// BANK ROM Select  hnkochou only
	AM_RANGE(0x64, 0x64) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
	AM_RANGE(0x65, 0x65) AM_WRITE(dynax_rombank_w			)	// BANK ROM Select  hanamai only
	AM_RANGE(0x66, 0x66) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack
	AM_RANGE(0x67, 0x67) AM_WRITE(adpcm_data_w				)	// MSM5205 data
	AM_RANGE(0x68, 0x68) AM_WRITE(dynax_layer_enable_w		)	// Layers Enable
	AM_RANGE(0x69, 0x69) AM_WRITE(hanamai_priority_w			)	// layer priority
	AM_RANGE(0x6a, 0x6a) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x6b, 0x6b) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x6c, 0x6c) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes (Low Bits)
	AM_RANGE(0x6d, 0x6d) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x6e, 0x6e) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x70, 0x70) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x71, 0x71) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x72, 0x72) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x73, 0x73) AM_WRITE(dynax_coincounter_1_w		)	//
	AM_RANGE(0x74, 0x74) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x76, 0x76) AM_WRITE(dynax_blit_palbank_w		)	// Layers Palettes (High Bit)
	AM_RANGE(0x77, 0x77) AM_WRITE(hanamai_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0x78, 0x78) AM_WRITE(YM2203_control_port_0_w	)	// YM2203
	AM_RANGE(0x79, 0x79) AM_WRITE(YM2203_write_port_0_w		)	//
	AM_RANGE(0x7a, 0x7a) AM_WRITE(AY8910_control_port_0_w	)	// AY8910
	AM_RANGE(0x7b, 0x7b) AM_WRITE(AY8910_write_port_0_w		)	//
//  AM_RANGE(0x7c, 0x7c) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x7d, 0x7d) AM_WRITE(MWA8_NOP                  )   // CRT Controller
ADDRESS_MAP_END


static ADDRESS_MAP_START( hnoridur_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x21, 0x21) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x22, 0x22) AM_READ(hanamai_keyboard_1_r	)	// P2
	AM_RANGE(0x23, 0x23) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x24, 0x24) AM_READ(input_port_1_r			)	// DSW2
	AM_RANGE(0x25, 0x25) AM_READ(input_port_9_r			)	// DSW4
	AM_RANGE(0x26, 0x26) AM_READ(input_port_8_r			)	// DSW3
	AM_RANGE(0x36, 0x36) AM_READ(AY8910_read_port_0_r	)	// AY8910, DSW1
	AM_RANGE(0x57, 0x57) AM_READ(ret_ff					)	// ?
ADDRESS_MAP_END
static ADDRESS_MAP_START( hnoridur_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x07) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0x20, 0x20) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
	AM_RANGE(0x30, 0x30) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x32, 0x32) AM_WRITE(adpcm_data_w				)	// MSM5205 data
	AM_RANGE(0x34, 0x34) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x35, 0x35) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x38, 0x38) AM_WRITE(AY8910_write_port_0_w		)	// AY8910
	AM_RANGE(0x3a, 0x3a) AM_WRITE(AY8910_control_port_0_w	)	//
//  AM_RANGE(0x10, 0x10) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x11, 0x11) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x40, 0x40) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x41, 0x41) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x42, 0x42) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes
	AM_RANGE(0x43, 0x43) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x44, 0x44) AM_WRITE(hanamai_priority_w			)	// layer priority and enable
	AM_RANGE(0x45, 0x45) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x46, 0x46) AM_WRITE(MWA8_NOP					)	// Blitter ROM bank (TODO)
	AM_RANGE(0x47, 0x47) AM_WRITE(hnoridur_palbank_w			)
	AM_RANGE(0x50, 0x50) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x51, 0x51) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
	AM_RANGE(0x54, 0x54) AM_WRITE(hnoridur_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x55, 0x55) AM_WRITE(MWA8_NOP						)	// ? VBlank IRQ Ack
	AM_RANGE(0x56, 0x56) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack
	AM_RANGE(0x60, 0x60) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x61, 0x61) AM_WRITE(hanamai_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0x62, 0x62) AM_WRITE(hnoridur_layer_half2_w		)	//
	AM_RANGE(0x67, 0x67) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x70, 0x70) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x71, 0x71) AM_WRITE(dynax_coincounter_1_w		)	//
ADDRESS_MAP_END


// Almost identical to hnoridur
static ADDRESS_MAP_START( mcnpshnt_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x21, 0x21) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x22, 0x22) AM_READ(hanamai_keyboard_1_r	)	// P2
	AM_RANGE(0x23, 0x23) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x24, 0x24) AM_READ(input_port_0_r			)	// DSW2
	AM_RANGE(0x26, 0x26) AM_READ(input_port_1_r			)	// DSW3
	AM_RANGE(0x57, 0x57) AM_READ(ret_ff					)	// ?
ADDRESS_MAP_END
static ADDRESS_MAP_START( mcnpshnt_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x07) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
//  AM_RANGE(0x10, 0x10) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x11, 0x11) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x20, 0x20) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
	AM_RANGE(0x30, 0x30) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x32, 0x32) AM_WRITE(adpcm_data_w				)	// MSM5205 data
	AM_RANGE(0x34, 0x34) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x35, 0x35) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x38, 0x38) AM_WRITE(AY8910_write_port_0_w		)	// AY8910
	AM_RANGE(0x3a, 0x3a) AM_WRITE(AY8910_control_port_0_w	)	//
	AM_RANGE(0x40, 0x40) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x41, 0x41) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x42, 0x42) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes
	AM_RANGE(0x43, 0x43) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x44, 0x44) AM_WRITE(hanamai_priority_w			)	// layer priority and enable
	AM_RANGE(0x45, 0x45) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x47, 0x47) AM_WRITE(hnoridur_palbank_w			)
	AM_RANGE(0x50, 0x50) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x51, 0x51) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
	AM_RANGE(0x54, 0x54) AM_WRITE(hnoridur_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x56, 0x56) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack
	AM_RANGE(0x60, 0x60) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x61, 0x61) AM_WRITE(nanajign_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0x62, 0x62) AM_WRITE(hnoridur_layer_half2_w		)	//
	AM_RANGE(0x67, 0x67) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x70, 0x70) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x71, 0x71) AM_WRITE(dynax_coincounter_1_w		)	//
ADDRESS_MAP_END


static ADDRESS_MAP_START( sprtmtch_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_READ(YM2203_status_port_0_r	)	// YM2203
	AM_RANGE(0x11, 0x11) AM_READ(YM2203_read_port_0_r	)	// 2 x DSW
	AM_RANGE(0x20, 0x20) AM_READ(input_port_0_r			)	// P1
	AM_RANGE(0x21, 0x21) AM_READ(input_port_1_r			)	// P2
	AM_RANGE(0x22, 0x22) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x23, 0x23) AM_READ(ret_ff					)	// ?
ADDRESS_MAP_END
static ADDRESS_MAP_START( sprtmtch_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x07) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0x10, 0x10) AM_WRITE(YM2203_control_port_0_w	)	// YM2203
	AM_RANGE(0x11, 0x11) AM_WRITE(YM2203_write_port_0_w		)	//
//  AM_RANGE(0x12, 0x12) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x13, 0x13) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x30, 0x30) AM_WRITE(dynax_layer_enable_w		)	// Layers Enable
	AM_RANGE(0x31, 0x31) AM_WRITE(dynax_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x32, 0x32) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x33, 0x33) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x34, 0x34) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes (Low Bits)
	AM_RANGE(0x35, 0x35) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x36, 0x36) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x37, 0x37) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack
//  AM_RANGE(0x40, 0x40) AM_WRITE(adpcm_reset_w             )   // MSM5205 reset
	AM_RANGE(0x41, 0x41) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x42, 0x42) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x43, 0x43) AM_WRITE(dynax_coincounter_1_w		)	//
	AM_RANGE(0x44, 0x44) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x45, 0x45) AM_WRITE(dynax_blit_palbank_w		)	// Layers Palettes (High Bit)
ADDRESS_MAP_END



static ADDRESS_MAP_START( mjfriday_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x63, 0x63) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x62, 0x62) AM_READ(hanamai_keyboard_1_r	)	// P2
	AM_RANGE(0x61, 0x61) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x64, 0x64) AM_READ(input_port_0_r			)	// DSW
	AM_RANGE(0x67, 0x67) AM_READ(input_port_1_r			)	// DSW
ADDRESS_MAP_END
static ADDRESS_MAP_START( mjfriday_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x41, 0x47) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
//  AM_RANGE(0x50, 0x50) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x51, 0x51) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x60, 0x60) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
	AM_RANGE(0x70, 0x70) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x71, 0x71) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x00, 0x00) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x01, 0x01) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes (Low Bits)
	AM_RANGE(0x02, 0x02) AM_WRITE(dynax_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x03, 0x03) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x10, 0x11) AM_WRITE(mjdialq2_blit_dest_w		)	// Destination Layer
	AM_RANGE(0x12, 0x12) AM_WRITE(dynax_blit_palbank_w		)	// Layers Palettes (High Bit)
	AM_RANGE(0x13, 0x13) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x14, 0x14) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x15, 0x15) AM_WRITE(dynax_coincounter_1_w		)	//
	AM_RANGE(0x16, 0x17) AM_WRITE(mjdialq2_layer_enable_w	)	// Layers Enable
//  AM_RANGE(0x80, 0x80) AM_WRITE(MWA8_NOP                  )   // IRQ ack?
ADDRESS_MAP_END


static ADDRESS_MAP_START( nanajign_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x11, 0x11) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x12, 0x12) AM_READ(hanamai_keyboard_1_r	)	// P2
	AM_RANGE(0x13, 0x13) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x14, 0x14) AM_READ(input_port_0_r			)	// DSW1
	AM_RANGE(0x15, 0x15) AM_READ(input_port_1_r			)	// DSW2
	AM_RANGE(0x16, 0x16) AM_READ(input_port_8_r			)	// DSW3
ADDRESS_MAP_END
static ADDRESS_MAP_START( nanajign_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x02, 0x02) AM_WRITE(adpcm_data_w				)	// MSM5205 data
	AM_RANGE(0x04, 0x04) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x05, 0x05) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x08, 0x08) AM_WRITE(AY8910_write_port_0_w		)	// AY8910
	AM_RANGE(0x0a, 0x0a) AM_WRITE(AY8910_control_port_0_w	)	//
	AM_RANGE(0x10, 0x10) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
//  AM_RANGE(0x20, 0x21) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x31, 0x37) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0x40, 0x40) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counter
	AM_RANGE(0x50, 0x50) AM_WRITE(dynax_flipscreen_w			)	// Flip Screen
	AM_RANGE(0x51, 0x51) AM_WRITE(hanamai_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0x52, 0x52) AM_WRITE(hnoridur_layer_half2_w		)	//
	AM_RANGE(0x57, 0x57) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x60, 0x60) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x62, 0x62) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
	AM_RANGE(0x6a, 0x6a) AM_WRITE(hnoridur_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x6c, 0x6c) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack
	AM_RANGE(0x70, 0x70) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x71, 0x71) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x72, 0x72) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes
	AM_RANGE(0x73, 0x73) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x74, 0x74) AM_WRITE(hanamai_priority_w			)	// layer priority and enable
	AM_RANGE(0x75, 0x75) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x77, 0x77) AM_WRITE(hnoridur_palbank_w			)
ADDRESS_MAP_END



/***************************************************************************
                    Yarunara / Quiz TV Q&Q / Mahjong Angels
***************************************************************************/

static UINT8 yarunara_select, yarunara_ip;
static WRITE8_HANDLER( yarunara_input_w )
{
	switch (offset)
	{
		case 0:	yarunara_select = data;
				yarunara_ip = 0;
				break;

		case 1:	break;
	}

}

static READ8_HANDLER( yarunara_input_r )
{
	switch (offset)
	{
		case 0:
		{
			switch( yarunara_select )
			{
				case 0x00:
					return readinputportbytag("IN2");	// coins

				case 0x02:
					return 0xff;	// bit 7 must be 1. Bit 2?

				default:
					return 0xff;
			}
		}

		case 1:
		{
			switch( yarunara_select )
			{
				// player 2
				case 0x01:	//quiztvqq
				case 0x81:
					return readinputport(3 + 5 + yarunara_ip++);

				// player 1
				case 0x02:	//quiztvqq
				case 0x82:
					return readinputport(3 + yarunara_ip++);

				default:
					return 0xff;
			}
		}
	}
	return 0xff;
}

static WRITE8_HANDLER( yarunara_rombank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1) + 0x10000 + 0x8000 * data;
	memory_set_bankptr(1, rom);

	hnoridur_bank = data;
	if (data == 0x1c)
		rom[0x0d] = 0x00;	// RTC
}

WRITE8_HANDLER( yarunara_flipscreen_w )
{
	dynax_flipscreen_w(0,(data&2)?1:0);
}

static ADDRESS_MAP_START( yarunara_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x03) AM_READ(yarunara_input_r	)	// Controls
	AM_RANGE(0x4c, 0x4c) AM_READ(input_port_0_r		)	// DSW 1
	AM_RANGE(0x4f, 0x4f) AM_READ(input_port_1_r		)	// DSW 2
ADDRESS_MAP_END

static ADDRESS_MAP_START( yarunara_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(yarunara_input_w			)	// Controls
	AM_RANGE(0x11, 0x17) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0x20, 0x20) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x22, 0x22) AM_WRITE(adpcm_data_w				)	// MSM5205 data
	AM_RANGE(0x24, 0x24) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x25, 0x25) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x28, 0x28) AM_WRITE(AY8910_write_port_0_w		)	// AY8910
	AM_RANGE(0x2a, 0x2a) AM_WRITE(AY8910_control_port_0_w	)	//
	AM_RANGE(0x48, 0x48) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x49, 0x49) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
	AM_RANGE(0x4a, 0x4a) AM_WRITE(yarunara_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x4b, 0x4b) AM_WRITE(dynax_vblank_ack_w			)	// VBlank IRQ Ack

	AM_RANGE(0x50, 0x50) AM_WRITE(yarunara_flipscreen_w		)
	AM_RANGE(0x51, 0x51) AM_WRITE(yarunara_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0x52, 0x52) AM_WRITE(yarunara_layer_half2_w		)	//
	// 53 ?
	// 54 ?
	AM_RANGE(0x57, 0x57) AM_WRITE(dynax_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x68, 0x68) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x69, 0x69) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x6a, 0x6a) AM_WRITE(dynax_blit_palette01_w		)	// Layers Palettes
	AM_RANGE(0x6b, 0x6b) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x6c, 0x6c) AM_WRITE(hanamai_priority_w			)	// layer priority and enable
	AM_RANGE(0x6d, 0x6d) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	// 6e ?
ADDRESS_MAP_END


/***************************************************************************
                            Jantouki - Main CPU
***************************************************************************/

UINT8 dynax_soundlatch_ack;
UINT8 dynax_soundlatch_full;
static UINT8 latch;

READ8_HANDLER( jantouki_soundlatch_ack_r )
{
	return (dynax_soundlatch_ack) ? 0x80 : 0;
}

WRITE8_HANDLER( jantouki_soundlatch_w )
{
	dynax_soundlatch_ack = 1;
	dynax_soundlatch_full = 1;
	dynax_soundlatch_irq = 1;
	latch = data;
	jantouki_sound_update_irq();
}

READ8_HANDLER( jantouki_blitter_busy_r )
{
	return 0;	// bit 0 & 1
}

static WRITE8_HANDLER( jantouki_rombank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memory_set_bankptr(1,&ROM[0x8000 + 0x8000*(data&0x0f)]);
	set_led_status(0,data & 0x10);	// maybe
}

static ADDRESS_MAP_START( jantouki_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x4a, 0x4a) AM_READ(jantouki_soundlatch_ack_r	)	// Soundlatch status
	AM_RANGE(0x52, 0x52) AM_READ(hanamai_keyboard_0_r		)	// P1
	AM_RANGE(0x54, 0x54) AM_READ(input_port_2_r				)	// Coins
	AM_RANGE(0x55, 0x55) AM_READ(input_port_0_r				)	// DSW1
	AM_RANGE(0x56, 0x56) AM_READ(input_port_1_r				)	// DSW2
	AM_RANGE(0x67, 0x67) AM_READ(jantouki_blitter_busy_r		)	//
ADDRESS_MAP_END
static ADDRESS_MAP_START( jantouki_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
//  AM_RANGE(0x40, 0x41) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x48, 0x48) AM_WRITE(jantouki_rombank_w			)	// BANK ROM Select
	AM_RANGE(0x49, 0x49) AM_WRITE(jantouki_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0x4b, 0x4b) AM_WRITE(dynax_blit2_dest_w			)	// Destination Layer 2
	AM_RANGE(0x4d, 0x4d) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0x4f, 0x4f) AM_WRITE(MWA8_NOP					)	// Blitter ROM bank (TODO)
	AM_RANGE(0x50, 0x50) AM_WRITE(jantouki_vblank_ack_w		)	// VBlank IRQ Ack
	AM_RANGE(0x51, 0x51) AM_WRITE(hanamai_keyboard_w			)	// keyboard row select
	AM_RANGE(0x58, 0x58) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counter
	AM_RANGE(0x5b, 0x5b) AM_WRITE(dynax_blit2_palbank_w		)	// Layers Palettes (High Bit)
	AM_RANGE(0x5d, 0x5d) AM_WRITE(dynax_blit_palbank_w		)	//
	AM_RANGE(0x5e, 0x5e) AM_WRITE(jantouki_blitter_ack_w		)	// Blitter IRQ Ack
	AM_RANGE(0x5f, 0x5f) AM_WRITE(jantouki_blitter2_ack_w	)	// Blitter 2 IRQ Ack
	AM_RANGE(0x60, 0x60) AM_WRITE(dynax_blit_palette67_w		)	// Layers Palettes (Low Bits)
	AM_RANGE(0x61, 0x61) AM_WRITE(dynax_blit_palette45_w		)	//
	AM_RANGE(0x62, 0x62) AM_WRITE(dynax_blit_palette23_w		)	//
	AM_RANGE(0x63, 0x63) AM_WRITE(dynax_blit_palette01_w		)	//
	AM_RANGE(0x64, 0x64) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0x65, 0x65) AM_WRITE(dynax_blit2_pen_w			)	// Destination Pen 2
	AM_RANGE(0x66, 0x66) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0x69, 0x6f) AM_WRITE(jantouki_blitter2_rev2_w	)	// Blitter
	AM_RANGE(0x71, 0x77) AM_WRITE(jantouki_blitter_rev2_w	)	// Blitter
	AM_RANGE(0x78, 0x7e) AM_WRITE(jantouki_layer_enable_w	)	// Layers Enable
ADDRESS_MAP_END

/***************************************************************************
                            Jantouki - Sound CPU
***************************************************************************/

WRITE8_HANDLER( jantouki_soundlatch_ack_w )
{
	dynax_soundlatch_ack = data;
	dynax_soundlatch_irq = 0;
	jantouki_sound_update_irq();
}

READ8_HANDLER( jantouki_soundlatch_r )
{
	dynax_soundlatch_full = 0;
	return latch;
}

READ8_HANDLER( jantouki_soundlatch_status_r )
{
	return (dynax_soundlatch_full) ? 0 : 0x80;
}

static ADDRESS_MAP_START( jantouki_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x21, 0x21) AM_READ(AY8910_read_port_0_r			)	// AY8910
	AM_RANGE(0x28, 0x28) AM_READ(YM2203_status_port_0_r			)	// YM2203
	AM_RANGE(0x29, 0x29) AM_READ(YM2203_read_port_0_r			)	//
	AM_RANGE(0x50, 0x50) AM_READ(jantouki_soundlatch_status_r	)	// Soundlatch status
	AM_RANGE(0x70, 0x70) AM_READ(jantouki_soundlatch_r			)	// From Main CPU
ADDRESS_MAP_END
static ADDRESS_MAP_START( jantouki_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(jantouki_sound_rombank_w		)	// BANK ROM Select
	AM_RANGE(0x10, 0x10) AM_WRITE(jantouki_sound_vblank_ack_w	)	// VBlank IRQ Ack
	AM_RANGE(0x22, 0x22) AM_WRITE(AY8910_write_port_0_w			)	// AY8910
	AM_RANGE(0x23, 0x23) AM_WRITE(AY8910_control_port_0_w		)	//
	AM_RANGE(0x28, 0x28) AM_WRITE(YM2203_control_port_0_w		)	// YM2203
	AM_RANGE(0x29, 0x29) AM_WRITE(YM2203_write_port_0_w			)	//
	AM_RANGE(0x30, 0x30) AM_WRITE(adpcm_reset_w					)	// MSM5205 reset
	AM_RANGE(0x40, 0x40) AM_WRITE(adpcm_data_w					)	// MSM5205 data
	AM_RANGE(0x60, 0x60) AM_WRITE(jantouki_soundlatch_ack_w		)	// Soundlatch status
ADDRESS_MAP_END



/***************************************************************************
                            Mahjong Electron Base
***************************************************************************/

static READ8_HANDLER( mjelctrn_keyboard_1_r )
{
	return (hanamai_keyboard_1_r(0) & 0x3f) | (readinputport(10) ? 0x40 : 0);
}

static READ8_HANDLER( mjelctrn_dsw_r )
{
	int dsw = (keyb & 0xc0) >> 6;
	if (dsw >= 2)	dsw = dsw - 2 + 8;	// 0-3 -> IN0,IN1,IN8,IN9
	return readinputport(dsw);
}

static WRITE8_HANDLER( mjelctrn_blitter_ack_w )
{
	dynax_blitter_irq = 0;
}

static ADDRESS_MAP_START( mjelctrn_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x81, 0x81) AM_READ(input_port_2_r			)	// Coins
	AM_RANGE(0x82, 0x82) AM_READ(mjelctrn_keyboard_1_r	)	// P2
	AM_RANGE(0x83, 0x83) AM_READ(hanamai_keyboard_0_r	)	// P1
	AM_RANGE(0x84, 0x84) AM_READ(mjelctrn_dsw_r			)	// DSW x 4
	AM_RANGE(0x85, 0x85) AM_READ(ret_ff					)	// ?
ADDRESS_MAP_END
static ADDRESS_MAP_START( mjelctrn_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x04) AM_WRITE(YM2413_register_port_0_w	)	// YM2413
	AM_RANGE(0x05, 0x05) AM_WRITE(YM2413_data_port_0_w		)	//
	AM_RANGE(0x08, 0x08) AM_WRITE(AY8910_write_port_0_w		)	// AY8910
	AM_RANGE(0x0a, 0x0a) AM_WRITE(AY8910_control_port_0_w	)	//
AM_RANGE(0x11, 0x12) AM_WRITE(mjelctrn_blitter_ack_w)			//?
	//neruton
	AM_RANGE(0x00, 0x00) AM_WRITE(adpcm_reset_w				)	// MSM5205 reset
	AM_RANGE(0x02, 0x02) AM_WRITE(adpcm_data_w				)	// MSM5205 data
//  AM_RANGE(0x20, 0x20) AM_WRITE(MWA8_NOP                  )   // CRT Controller
//  AM_RANGE(0x21, 0x21) AM_WRITE(MWA8_NOP                  )   // CRT Controller
	AM_RANGE(0x40, 0x40) AM_WRITE(dynax_coincounter_0_w		)	// Coin Counters
	AM_RANGE(0x41, 0x41) AM_WRITE(dynax_coincounter_1_w		)	//
	AM_RANGE(0x60, 0x60) AM_WRITE(dynax_extra_scrollx_w		)	// screen scroll X
	AM_RANGE(0x62, 0x62) AM_WRITE(dynax_extra_scrolly_w		)	// screen scroll Y
//  AM_RANGE(0x64, 0x64) AM_WRITE(dynax_extra_scrollx_w     )   // screen scroll X
//  AM_RANGE(0x66, 0x66) AM_WRITE(dynax_extra_scrolly_w     )   // screen scroll Y
	AM_RANGE(0x6a, 0x6a) AM_WRITE(hnoridur_rombank_w		)	// BANK ROM Select
	AM_RANGE(0x80, 0x80) AM_WRITE(hanamai_keyboard_w		)	// keyboard row select
	AM_RANGE(0xa1, 0xa7) AM_WRITE(dynax_blitter_rev2_w		)	// Blitter
	AM_RANGE(0xc0, 0xc0) AM_WRITE(dynax_flipscreen_w		)	// Flip Screen
	AM_RANGE(0xc1, 0xc1) AM_WRITE(hanamai_layer_half_w		)	// half of the interleaved layer to write to
	AM_RANGE(0xc2, 0xc2) AM_WRITE(hnoridur_layer_half2_w	)	//
//  c3,c4   seem to be related to wrap around enable
	AM_RANGE(0xe0, 0xe0) AM_WRITE(dynax_blit_pen_w			)	// Destination Pen
	AM_RANGE(0xe1, 0xe1) AM_WRITE(dynax_blit_dest_w			)	// Destination Layer
	AM_RANGE(0xe2, 0xe2) AM_WRITE(dynax_blit_palette01_w	)	// Layers Palettes
	AM_RANGE(0xe3, 0xe3) AM_WRITE(dynax_blit_palette23_w	)	//
	AM_RANGE(0xe4, 0xe4) AM_WRITE(hanamai_priority_w		)	// layer priority and enable
	AM_RANGE(0xe5, 0xe5) AM_WRITE(dynax_blit_backpen_w		)	// Background Color
	AM_RANGE(0xe6, 0xe6) AM_WRITE(MWA8_NOP					)	// Blitter ROM bank (TODO)
	AM_RANGE(0xe7, 0xe7) AM_WRITE(hnoridur_palbank_w		)
ADDRESS_MAP_END

/***************************************************************************


                                Input Ports


***************************************************************************/

#define MAHJONG_PANEL\
	PORT_START_TAG("IN3")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1)\
	PORT_START_TAG("IN4")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )\
	PORT_START_TAG("IN5")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN6")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN7")\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) /*I would assume, anyway*/ \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( hanamai )
	PORT_START_TAG("DSW0")
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
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW1")
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

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START_TAG("IN4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hnkochou )
	PORT_START_TAG("DSW0")
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

	PORT_START_TAG("DSW1")
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

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )		// Pay
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW )	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )		// Note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	MAHJONG_PANEL

INPUT_PORTS_END


INPUT_PORTS_START( hnoridur )
	PORT_START_TAG("DSW0")	/* note that these are in reverse order wrt the others */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW1")
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

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START_TAG("IN4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN8")
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

	PORT_START_TAG("IN9")
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


INPUT_PORTS_START( sprtmtch )
	PORT_START_TAG("IN0")
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN2")
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(10)
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(10)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )	// Time
	PORT_DIPSETTING(    0x00, "1 (Easy)" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x05, "6" )
	PORT_DIPSETTING(    0x06, "7" )
	PORT_DIPSETTING(    0x07, "8 (Hard)" )
	PORT_DIPNAME( 0x18, 0x10, "Vs Time" )
	PORT_DIPSETTING(    0x18, "8 s" )
	PORT_DIPSETTING(    0x10, "10 s" )
	PORT_DIPSETTING(    0x08, "12 s" )
	PORT_DIPSETTING(    0x00, "14 s" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mjfriday )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "PINFU with TSUMO" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x18, "Player Strength" )
	PORT_DIPSETTING(    0x18, "Weak" )
	PORT_DIPSETTING(    0x10, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, "Strong" )
	PORT_DIPSETTING(    0x00, "Strongest" )
	PORT_DIPNAME( 0x60, 0x60, "CPU Strength" )
	PORT_DIPSETTING(    0x60, "Weak" )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Strong" )
	PORT_DIPSETTING(    0x00, "Strongest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Auto TSUMO" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
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
	PORT_DIPNAME( 0x20, 0x20, "Gfx test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "17B"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18B"
	PORT_SERVICE(0x04, IP_ACTIVE_LOW )	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "06B"
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18A"

MAHJONG_PANEL
INPUT_PORTS_END


INPUT_PORTS_START( mjdialq2 )
	PORT_START_TAG("DSW0")
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
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
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

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "17B"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18B"
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "06B"
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18A"

MAHJONG_PANEL
INPUT_PORTS_END

#define MAHJONG_BOARD\
	PORT_START_TAG("IN3")\
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )\
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )\
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )\
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )\
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_START_TAG("IN4")\
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )\
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )\
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )\
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )\
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )\
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )\
	PORT_START_TAG("IN5")\
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )\
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )\
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )\
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )\
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_START_TAG("IN6")\
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )\
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )\
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )\
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( yarunara )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )		// 1,6
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) ) 	// 3,4
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )		// 5,2
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )	// 7,0
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Scrambled comm.?" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Set Date" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "PINFU with TSUMO" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Choose Bonus (Cheat)")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// "17B"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// "18B"
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// "06B"
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	MAHJONG_BOARD
	PORT_START_TAG("IN7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)

	// Keyboard 2
	PORT_START_TAG("IN8")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN9")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN10")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN11")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN12")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( quiztvqq )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy )    )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal )  )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard )    )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Voices" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Set Date" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	// Test, during boot
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2   )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN4")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN5")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN6")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN7")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	// Keyboard 2
	PORT_START_TAG("IN8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN9")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN10")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN11")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("IN12")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( mcnpshnt )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x06, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x07, "0" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Choose Bonus (Cheat)")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	MAHJONG_BOARD

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( nanajign )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x38, "0" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x28, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x08, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )		//?
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )
	MAHJONG_BOARD

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN8")
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


INPUT_PORTS_START( jantouki )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x07, "0" )	// 0 6 2
	PORT_DIPSETTING(    0x06, "1" )	// 0 6 1
	PORT_DIPSETTING(    0x05, "2" )	// 1 5 2
	PORT_DIPSETTING(    0x04, "3" )	// 1 5 1
	PORT_DIPSETTING(    0x03, "4" ) // 2 4 2
	PORT_DIPSETTING(    0x02, "5" )	// 2 4 1
	PORT_DIPSETTING(    0x01, "6" )	// 2 3 1
	PORT_DIPSETTING(    0x00, "7" )	// 2 2 1
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, "Hours" )
	PORT_DIPSETTING(    0x07, "08:30" )
	PORT_DIPSETTING(    0x06, "09:00" )
	PORT_DIPSETTING(    0x05, "09:30" )
	PORT_DIPSETTING(    0x04, "10:00" )
	PORT_DIPSETTING(    0x03, "10:30" )
	PORT_DIPSETTING(    0x02, "11:00" )
	PORT_DIPSETTING(    0x01, "11:30" )
	PORT_DIPSETTING(    0x00, "12:00" )
	PORT_DIPNAME( 0x08, 0x08, "Moles On Gal's Face" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Choose Game Mode" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Coin Every" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPSETTING(    0x20, "150" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	//*
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START_TAG("IN4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )

	PORT_START_TAG("IN5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
 	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( mjelct3 )
	PORT_START_TAG("DSW2")	//7c21 (select = 00)
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?" )
	PORT_DIPSETTING(    0x03, "0" )	// 20
	PORT_DIPSETTING(    0x00, "1" )	// 32
	PORT_DIPSETTING(    0x01, "2" )	// 64
	PORT_DIPSETTING(    0x02, "3" )	// c8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Min Pay?" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Coin Out" )//This is a skill game, isn't it?
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Win A Prize?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")	// 7c20 (select = 40)
	PORT_DIPNAME( 0x0f, 0x07, "Pay Out Rate" ) //This is a skill game, isn't it?
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "53" )
	PORT_DIPSETTING(    0x02, "56" )
	PORT_DIPSETTING(    0x03, "59" )
	PORT_DIPSETTING(    0x04, "62" )
	PORT_DIPSETTING(    0x05, "65" )
	PORT_DIPSETTING(    0x06, "68" )
	PORT_DIPSETTING(    0x07, "71" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x09, "78" )
	PORT_DIPSETTING(    0x0a, "81" )
	PORT_DIPSETTING(    0x0b, "84" )
	PORT_DIPSETTING(    0x0c, "87" )
	PORT_DIPSETTING(    0x0d, "90" )
	PORT_DIPSETTING(    0x0e, "93" )
	PORT_DIPSETTING(    0x0f, "96" )
	PORT_DIPNAME( 0x30, 0x30, "Max Bet" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE4 )	// Pay
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// 18B
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )				// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2    )	// Note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1    )	// Coin
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )	// Service

	MAHJONG_BOARD

	PORT_START	// IN7
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL )

	PORT_START_TAG("DSW3")	// 7c22 (select = 80)
	PORT_DIPNAME( 0x07, 0x07, "YAKUMAN Bonus" )
	PORT_DIPSETTING(    0x07, "Cut" )
	PORT_DIPSETTING(    0x06, "1 T" )
	PORT_DIPSETTING(    0x05, "300" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x03, "700" )
	PORT_DIPSETTING(    0x02, "1000" )
//  PORT_DIPSETTING(    0x01, "1000" )
//  PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPNAME( 0x08, 0x08, "YAKU times" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Win Rate?" )
	PORT_DIPSETTING(    0x10, DEF_STR( High ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPNAME( 0x20, 0x20, "Draw New Tile (Part 3 Only)" )
	PORT_DIPSETTING(    0x00, "Automatic" )
	PORT_DIPSETTING(    0x20, "Manual" )
	PORT_DIPNAME( 0x40, 0x40, "DonDen Key" )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "Flip Flop" )
	PORT_DIPNAME( 0x80, 0x00, "Subtitle" )
	PORT_DIPSETTING(    0x80, "None (Part 2)" )
	PORT_DIPSETTING(    0x00, "Super Express (Part 3)" )

	PORT_START_TAG("DSW4")	// 7c23 (select = c0)
	PORT_DIPNAME( 0x01, 0x01, "Last Chance" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Pay Rate?" )
	PORT_DIPSETTING(    0x02, DEF_STR( High ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPNAME( 0x04, 0x04, "Choose Bonus" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "In-Game Bet?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "In-Game Music" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Select Girl" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Moles On Gal's Face" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("FAKE")	// IN10 - Fake DSW
	PORT_DIPNAME( 0xff, 0xff, "Allow Bets" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xff, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( mjelctrn )
	PORT_START_TAG("DSW2")	// 7c21 (select = 00)
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?" )
	PORT_DIPSETTING(    0x03, "0" )	// 20
	PORT_DIPSETTING(    0x00, "1" )	// 32
	PORT_DIPSETTING(    0x01, "2" )	// 64
	PORT_DIPSETTING(    0x02, "3" )	// c8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Min Pay?" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Coin Out" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Win A Prize?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1") // 7c20 (select = 40)
	PORT_DIPNAME( 0x0f, 0x07, "Pay Out Rate" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "53" )
	PORT_DIPSETTING(    0x02, "56" )
	PORT_DIPSETTING(    0x03, "59" )
	PORT_DIPSETTING(    0x04, "62" )
	PORT_DIPSETTING(    0x05, "65" )
	PORT_DIPSETTING(    0x06, "68" )
	PORT_DIPSETTING(    0x07, "71" )
	PORT_DIPSETTING(    0x08, "75" )
	PORT_DIPSETTING(    0x09, "78" )
	PORT_DIPSETTING(    0x0a, "81" )
	PORT_DIPSETTING(    0x0b, "84" )
	PORT_DIPSETTING(    0x0c, "87" )
	PORT_DIPSETTING(    0x0d, "90" )
	PORT_DIPSETTING(    0x0e, "93" )
	PORT_DIPSETTING(    0x0f, "96" )
	PORT_DIPNAME( 0x30, 0x30, "Max Bet" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE4 )	// Pay
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// 18B
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )				// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2    )	// Note
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1    )	// Coin
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )	// Service

	MAHJONG_BOARD

	PORT_START_TAG("IN7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL )

	PORT_START_TAG("DSW3") // 7c22 (select = 80)
	PORT_DIPNAME( 0x07, 0x07, "YAKUMAN Bonus" )
	PORT_DIPSETTING(    0x07, "Cut" )
	PORT_DIPSETTING(    0x06, "1 T" )
	PORT_DIPSETTING(    0x05, "300" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x03, "700" )
	PORT_DIPSETTING(    0x02, "1000" )
//  PORT_DIPSETTING(    0x01, "1000" )
//  PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPNAME( 0x08, 0x08, "YAKU times" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Win Rate?" )
	PORT_DIPSETTING(    0x10, DEF_STR( High ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPNAME( 0x20, 0x20, "Draw New Tile (Part 4 Only)" )
	PORT_DIPSETTING(    0x00, "Automatic" )
	PORT_DIPSETTING(    0x20, "Manual" )
	PORT_DIPNAME( 0x40, 0x40, "DonDen Key" )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "Flip Flop" )
	PORT_DIPNAME( 0x80, 0x00, "Subtitle" )
	PORT_DIPSETTING(    0x80, "None (Part 2)" )
	PORT_DIPSETTING(    0x00, "???? (Part 4)" )

	PORT_START_TAG("DSW4") // 7c23 (select = c0)
	PORT_DIPNAME( 0x01, 0x01, "Last Chance" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Pay Rate?" )
	PORT_DIPSETTING(    0x02, DEF_STR( High ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPNAME( 0x04, 0x04, "Choose Bonus" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "In-Game Bet?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "In-Game Music" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Select Girl" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Girls" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("FAKE")	// IN10 - Fake DSW
	PORT_DIPNAME( 0xff, 0xff, "Allow Bets" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xff, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( neruton )
	PORT_START_TAG("DSW2") //6a77 (select = 00)
	PORT_DIPNAME( 0x07, 0x07, "Hours" )
	PORT_DIPSETTING(    0x07, "08:30" )
	PORT_DIPSETTING(    0x06, "09:00" )
	PORT_DIPSETTING(    0x05, "09:30" )
	PORT_DIPSETTING(    0x04, "10:00" )
	PORT_DIPSETTING(    0x03, "10:30" )
	PORT_DIPSETTING(    0x02, "11:00" )
	PORT_DIPSETTING(    0x01, "11:30" )
	PORT_DIPSETTING(    0x00, "12:00" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPSETTING(    0x20, "60" )
	PORT_DIPNAME( 0x40, 0x40, "See Opponent's Tiles (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW1")	// 6a76 (select = 40)
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "PINFU with TSUMO" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x38, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x38, "1 (Easy)" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hard)" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// 17B
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// 18B
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F1)	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// 06B
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1    )	// Coin
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )	// 18A

MAHJONG_PANEL
INPUT_PORTS_END


/***************************************************************************


                                Machine Drivers


***************************************************************************/

/***************************************************************************
                                Hana no Mai
***************************************************************************/

static struct YM2203interface hanamai_ym2203_interface =
{
	input_port_1_r,				/* Port A Read: DSW */
	input_port_0_r,				/* Port B Read: DSW */
	0,							/* Port A Write */
	0,							/* Port B Write */
	sprtmtch_sound_callback		/* IRQ handler */
};

static struct MSM5205interface hanamai_msm5205_interface =
{
	adpcm_int,			/* IRQ handler */
	MSM5205_S48_4B		/* 8 KHz, 4 Bits  */
};



static MACHINE_DRIVER_START( hanamai )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_PROGRAM_MAP(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_IO_MAP(hanamai_readport,hanamai_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(adpcm)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(hanamai)
	MDRV_VIDEO_UPDATE(hanamai)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 22000000 / 8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(YM2203, 22000000 / 8)
	MDRV_SOUND_CONFIG(hanamai_ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(hanamai_msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************
                                Hana Oriduru
***************************************************************************/

static struct AY8910interface hnoridur_ay8910_interface =
{
	input_port_0_r		/* Port A Read: DSW */
};

static MACHINE_DRIVER_START( hnoridur )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_PROGRAM_MAP(hnoridur_readmem,hnoridur_writemem)
	MDRV_CPU_IO_MAP(hnoridur_readport,hnoridur_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(adpcm)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(16*256)

	MDRV_VIDEO_START(hnoridur)
	MDRV_VIDEO_UPDATE(hnoridur)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 22000000 / 8)
	MDRV_SOUND_CONFIG(hnoridur_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(YM2413, 3580000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(hanamai_msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                Sports Match
***************************************************************************/

static struct YM2203interface sprtmtch_ym2203_interface =
{
	input_port_3_r,				/* Port A Read: DSW */
	input_port_4_r,				/* Port B Read: DSW */
	0,							/* Port A Write */
	0,							/* Port B Write */
	sprtmtch_sound_callback,	/* IRQ handler */
};

static MACHINE_DRIVER_START( sprtmtch )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_PROGRAM_MAP(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_IO_MAP(sprtmtch_readport,sprtmtch_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(sprtmtch)
	MDRV_VIDEO_UPDATE(sprtmtch)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 22000000 / 8)
	MDRV_SOUND_CONFIG(sprtmtch_ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                            Mahjong Friday
***************************************************************************/

static MACHINE_DRIVER_START( mjfriday )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,24000000/4)	/* 6 MHz? */
	MDRV_CPU_PROGRAM_MAP(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_IO_MAP(mjfriday_readport,mjfriday_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(mjdialq2)
	MDRV_VIDEO_UPDATE(mjdialq2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2413, 24000000/6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                            Mahjong Dial Q2
***************************************************************************/

static MACHINE_DRIVER_START( mjdialq2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( mjfriday )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mjdialq2_readmem,mjdialq2_writemem)
MACHINE_DRIVER_END


/***************************************************************************
                    Yarunara / Quiz TV Q&Q / Mahjong Angels
***************************************************************************/

/* the old code here didn't work..
  what was it trying to do?
  set an irq and clear it before its even taken? */

INTERRUPT_GEN( yarunara_clock_interrupt )
{
	static int i=0;
	i^=1;

	if (i==1)
	{
		dynax_sound_irq = 0;
	}
	else
	{
		dynax_sound_irq = 1;
	}

	sprtmtch_update_irq();
}

static MACHINE_DRIVER_START( yarunara )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( hnoridur )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(yarunara_readmem,yarunara_writemem)
	MDRV_CPU_IO_MAP(yarunara_readport,yarunara_writeport)
	MDRV_CPU_PERIODIC_INT(yarunara_clock_interrupt,TIME_IN_HZ(60))	// RTC

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_VISIBLE_AREA(0, 336-1, 8, 256-1-8-1)
MACHINE_DRIVER_END


/***************************************************************************
                            Mahjong Campus Hunting
***************************************************************************/

static MACHINE_DRIVER_START( mcnpshnt )

	MDRV_IMPORT_FROM( hnoridur )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mcnpshnt_readmem,mcnpshnt_writemem)
	MDRV_CPU_IO_MAP(mcnpshnt_readport,mcnpshnt_writeport)

	MDRV_VIDEO_START(mcnpshnt)	// different priorities
MACHINE_DRIVER_END


/***************************************************************************
                            7jigen
***************************************************************************/

static MACHINE_DRIVER_START( nanajign )

	MDRV_IMPORT_FROM( hnoridur )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(nanajign_readmem,nanajign_writemem)
	MDRV_CPU_IO_MAP(nanajign_readport,nanajign_writeport)
MACHINE_DRIVER_END


/***************************************************************************
                                Jantouki
***************************************************************************/

// dual monitor, 2 CPU's, 2 blitters

static struct YM2203interface jantouki_ym2203_interface =
{
	0,							/* Port A Read: DSW */
	0,							/* Port B Read: DSW */
	0,							/* Port A Write */
	0,							/* Port B Write */
	jantouki_sound_callback		/* IRQ handler */
};

static struct MSM5205interface jantouki_msm5205_interface =
{
	adpcm_int_cpu1,			/* IRQ handler */
	MSM5205_S48_4B		/* 8 KHz, 4 Bits  */
};

static MACHINE_DRIVER_START( jantouki )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_PROGRAM_MAP(jantouki_readmem,jantouki_writemem)
	MDRV_CPU_IO_MAP(jantouki_readport,jantouki_writeport)
	MDRV_CPU_VBLANK_INT(jantouki_vblank_interrupt, 1)	/* IM 0 needs an opcode on the data bus */

	MDRV_CPU_ADD_TAG("sound",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_PROGRAM_MAP(jantouki_sound_readmem,jantouki_sound_writemem)
	MDRV_CPU_IO_MAP(jantouki_sound_readport,jantouki_sound_writeport)
	MDRV_CPU_VBLANK_INT(jantouki_sound_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(adpcm)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(4,6)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 16+240+240-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(jantouki)
	MDRV_VIDEO_UPDATE(jantouki)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 22000000 / 8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD(YM2203, 22000000 / 8)
	MDRV_SOUND_CONFIG(jantouki_ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(jantouki_msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                            Mahjong Electron Base
***************************************************************************/

/*  It runs in IM 2, thus needs a vector on the data bus:
    0xfa and 0xfc are very similar, they should be triggered by the blitter
    0xf8 is vblank  */
void mjelctrn_update_irq(void)
{
	dynax_blitter_irq = 1;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xfa);
}

INTERRUPT_GEN( mjelctrn_vblank_interrupt )
{
	// This is a kludge to avoid losing blitter interrupts
	// there should be a vblank ack mechanism
	if (!dynax_blitter_irq)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xf8);
}

static MACHINE_DRIVER_START( mjelctrn )

	MDRV_IMPORT_FROM( hnoridur )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(nanajign_readmem,nanajign_writemem)
	MDRV_CPU_IO_MAP(mjelctrn_readport,mjelctrn_writeport)
	MDRV_CPU_VBLANK_INT(mjelctrn_vblank_interrupt,1)	/* IM 2 needs a vector on the data bus */

	MDRV_VIDEO_START(mjelctrn)
MACHINE_DRIVER_END


/***************************************************************************
                                    Neruton
***************************************************************************/

/*  It runs in IM 2, thus needs a vector on the data bus:
    0x42 and 0x44 are very similar, they should be triggered by the blitter
    0x40 is vblank
    0x46 is a periodic irq? */
void neruton_update_irq(void)
{
	dynax_blitter_irq = 1;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x42);
}

INTERRUPT_GEN( neruton_vblank_interrupt )
{
	// This is a kludge to avoid losing blitter interrupts
	// there should be a vblank ack mechanism
	if (dynax_blitter_irq)	return;

	switch(cpu_getiloops())
	{
		case 0:		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x40);	break;
		default:	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x46);
	}
}

static MACHINE_DRIVER_START( neruton )

	MDRV_IMPORT_FROM( mjelctrn )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(neruton_vblank_interrupt,1+10)	/* IM 2 needs a vector on the data bus */

	MDRV_VIDEO_START(neruton)
MACHINE_DRIVER_END

/***************************************************************************


                                ROMs Loading


***************************************************************************/


/***************************************************************************

Hana no Mai
(c)1988 Dynax

D1610088L1

CPU:    Z80-A
Sound:  AY-3-8912A
    YM2203C
    M5205
OSC:    22.000MHz
Custom: (TC17G032AP-0246)

***************************************************************************/

ROM_START( hanamai )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "1611.13a", 0x00000, 0x10000, CRC(5ca0b073) SHA1(56b64077e7967fdbb87a7685ca9662cc7881b5ec) )
	ROM_LOAD( "1610.14a", 0x48000, 0x10000, CRC(b20024aa) SHA1(bb6ce9821c1edbf7d4cfadc58a2b257755856937) )

	ROM_REGION( 0x140000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "1604.12e", 0x000000, 0x20000, CRC(3b8362f7) SHA1(166ce1fe5c48b02b728cf9b90f3dfae4461a3e2c) )
	ROM_LOAD( "1609.12c", 0x020000, 0x20000, CRC(91c5d211) SHA1(343ee4273e8075ba9c2f545c1ee2e403623e3185) )
	ROM_LOAD( "1603.13e", 0x040000, 0x20000, CRC(16a2a680) SHA1(7cd5de9a36fd05261d23f0a0e90d871b132368f0) )
	ROM_LOAD( "1608.13c", 0x060000, 0x20000, CRC(793dd4f8) SHA1(57c32fae553ba9d37c2ffdbfa96ede113f7bcccb) )
	ROM_LOAD( "1602.14e", 0x080000, 0x20000, CRC(3189a45f) SHA1(06e8cb1b6dd6d7e00a7270d4c76b894aba2e7734) )
	ROM_LOAD( "1607.14c", 0x0a0000, 0x20000, CRC(a58edfd0) SHA1(8112b0e0bf8c5bdd1d2e3338d23fe36cf3972a43) )
	ROM_LOAD( "1601.15e", 0x0c0000, 0x20000, CRC(84ff07af) SHA1(d7259056c4e09171aa8b9342ebaf3b8a3490613a) )
	ROM_LOAD( "1606.15c", 0x0e0000, 0x10000, CRC(ce7146c1) SHA1(dc2e202a67d1618538eb04248c1b2c7d7f62151e) )
	ROM_LOAD( "1605.10e", 0x100000, 0x10000, CRC(0f4fd9e4) SHA1(8bdc8b46bf4dafead25a5adaebb74d547386ce23) )
	ROM_LOAD( "1612.10c", 0x120000, 0x10000, CRC(8d9fb6e1) SHA1(2763f73069147d62fd46bb961b64cc9598687a28) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.3j",  0x000, 0x200, CRC(7b0618a5) SHA1(df3aadcc7d54fab0c07f85d20c138a45798644e4) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.4j",  0x200, 0x200, CRC(9cfcdd2d) SHA1(a649e9381754c4a19ccecc6e558067cc3ff27f91) )
ROM_END


/***************************************************************************

Hana Kochou (Hana no Mai BET ver.)
(c)1988 Dynax

D201901L2
D201901L1-0

CPU:    Z80-A
Sound:  AY-3-8912A
    YM2203C
    M5205
OSC:    22.000MHz
VDP:    HD46505SP
Custom: (TC17G032AP-0246)

***************************************************************************/

ROM_START( hnkochou )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2009.s4a", 0x00000, 0x10000, CRC(b3657123) SHA1(3385edb2055abc7be3abb030509c6ac71907a5f3) )
	ROM_LOAD( "2008.s3a", 0x18000, 0x10000, CRC(1c009be0) SHA1(0f950d2685f8b67f37065e19deae0cf0cb9594f1) )

	ROM_REGION( 0x0e0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2004.12e", 0x000000, 0x20000, CRC(178aa996) SHA1(0bc8b7c093ed46a91c5781c03ae707d3dafaeef4) )
	ROM_LOAD( "2005.12c", 0x020000, 0x20000, CRC(ca57cac5) SHA1(7a55e1cb5cee5a38c67199f589e0c7ae5cd907a0) )
	ROM_LOAD( "2003.13e", 0x040000, 0x20000, CRC(092edf8d) SHA1(3d030462f96edbb0fa4efcc2a5302c17661dce54) )
	ROM_LOAD( "2006.13c", 0x060000, 0x20000, CRC(610c22ec) SHA1(af47bf94e01d1a83aa2a7195c906e13038057c35) )
	ROM_LOAD( "2002.14e", 0x080000, 0x20000, CRC(759b717d) SHA1(fd719fd792459789b559a1e99173144322605b8e) )
	ROM_LOAD( "2007.14c", 0x0a0000, 0x20000, CRC(d0f22355) SHA1(7b027930624ff1f883d620a8e78f962e821f4b23) )
	ROM_LOAD( "2001.15e", 0x0c0000, 0x20000, CRC(09ace2b5) SHA1(1756e3a52523557aa481c6bd6cdf168567af82ff) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.3j",  0x000, 0x200, CRC(7b0618a5) SHA1(df3aadcc7d54fab0c07f85d20c138a45798644e4) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.4j",  0x200, 0x200, CRC(9cfcdd2d) SHA1(a649e9381754c4a19ccecc6e558067cc3ff27f91) )
ROM_END



/***************************************************************************

Hana Oriduru
(c)1989 Dynax
D2304268L

CPU  : Z80B
Sound: AY-3-8912A YM2413 M5205
OSC  : 22MHz (X1, near main CPU), 384KHz (X2, near M5205)
       3.58MHz (X3, Sound section)

CRT Controller: HD46505SP (6845)
Custom chip: DYNAX TC17G032AP-0246 JAPAN 8929EAI

***************************************************************************/

ROM_START( hnoridur )
	ROM_REGION( 0x10000 + 0x19*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2309.12",  0x00000, 0x20000, CRC(5517dd68) SHA1(3da27032a412b51b67e852b61166c2fdc138a370) )
	ROM_RELOAD(           0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2302.21",  0x000000, 0x20000, CRC(9dde2d59) SHA1(96df4ba97ee9611d9a3c7bcaae9cd97815a7b8a5) )
	ROM_LOAD( "2303.22",  0x020000, 0x20000, CRC(1ac59443) SHA1(e70fe6184e7090cf7229d83b87db65f7715de2a8) )
	ROM_LOAD( "2301.20",  0x040000, 0x20000, CRC(24391ddc) SHA1(6a2e3fae4b6d0b1d8073306f37c9fdaa04b69eb8) )
	ROM_LOAD( "2304.1",   0x060000, 0x20000, BAD_DUMP CRC(9792d9fa) SHA1(5835d3d97a5b0a6594654010c8061238cd0514f6) )
	ROM_LOAD( "2305.2",   0x080000, 0x20000, CRC(249d360a) SHA1(688fced1298c345a18314d2c88664c757a2de35c) )
	ROM_LOAD( "2306.3",   0x0a0000, 0x20000, CRC(014a4945) SHA1(0cd747787a81226fd4937616a6ce45af731a4049) )
	ROM_LOAD( "2307.4",   0x0c0000, 0x20000, CRC(8b6f8a2d) SHA1(c5f3ec64a7ea3edc556182f42e6da4842d88e0ba) )
	ROM_LOAD( "2308.5",   0x0e0000, 0x20000, CRC(6f996e6e) SHA1(c2b916afbfd257417f0383ad261f3720a027fdd9) )
ROM_END



/***************************************************************************

Sports Match
Dynax 1989

                     5563
                     3101
        SW2 SW1
                             3103
         YM2203              3102
                     16V8
                     Z80         DYNAX
         22MHz

           6845
                         53462
      17G                53462
      18G                53462
                         53462
                         53462
                         53462

***************************************************************************/

ROM_START( drgpunch )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2401.3d", 0x00000, 0x10000, CRC(b310709c) SHA1(6ad6cfb54856f65a888ac44e694890f32f26e049) )
	ROM_LOAD( "2402.6d", 0x28000, 0x10000, CRC(d21ed237) SHA1(7e1c7b40c300578132ebd79cbad9f7976cc85947) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2403.6c", 0x00000, 0x20000, CRC(b936f202) SHA1(4920d29a814ebdd74ce6f780cf821c8cb8142d9f) )
	ROM_LOAD( "2404.5c", 0x20000, 0x20000, CRC(2ee0683a) SHA1(e29225e08be11f6971fa04ce2715be914d29976b) )
	ROM_LOAD( "2405.3c", 0x40000, 0x20000, CRC(aefbe192) SHA1(9ed0ec7d6357eedec80a90364f196e43a5bfee03) )
	ROM_LOAD( "2406.1c", 0x60000, 0x20000, CRC(e137f96e) SHA1(c652f3cb17a56127d30a516af75154e15d7ce6c0) )
	ROM_LOAD( "2407.6a", 0x80000, 0x20000, CRC(f3f1b065) SHA1(531317e4d1ab5db60595ca3327234a6bdea79ce9) )
	ROM_LOAD( "2408.5a", 0xa0000, 0x20000, CRC(3a91e2b9) SHA1(b762c38ff2ebbd4ed832ca772973a15dd4a4ad73) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.18g", 0x000, 0x200, CRC(9adccc33) SHA1(acf4d5a28430378dbccc1b9fa0b6391cc8149fee) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.17g", 0x200, 0x200, CRC(324fa9cf) SHA1(a03e23d9a9687dec4c23a8e41254a3f4b70c7e25) )
ROM_END

ROM_START( sprtmtch )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "3101.3d", 0x00000, 0x08000, CRC(d8fa9638) SHA1(9851d38b6b3f56cf3cc101419c24f8d5f97950a9) )
	ROM_CONTINUE(        0x28000, 0x08000 )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "3102.6c", 0x00000, 0x20000, CRC(46f90e59) SHA1(be4411c3cfa8c8ab26eba935289df0f0fd545b62) )
	ROM_LOAD( "3103.5c", 0x20000, 0x20000, CRC(ad29d7bd) SHA1(09ab84164e5cd14b595f33d129863735901aa922) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "18g", 0x000, 0x200, CRC(dcc4e0dd) SHA1(4e0fb8fd7192bf32247966742df4b80585f32c37) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "17g", 0x200, 0x200, CRC(5443ebfb) SHA1(5b63220a3f6520e353db99b06e645640d1cfde2f) )
ROM_END


/***************************************************************************

Mahjong Friday
(c)1989 Dynax
D2607198L1

CPU  : Zilog Z0840006PSC (Z80)
Sound: YM2413 M5205
OSC  : 24MHz (X1)

CRT Controller: HD46505SP (6845)
Custom chip: DYNAX TC17G032AP-0246 JAPAN 8828EAI

***************************************************************************/

ROM_START( mjfriday )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2606.2b",  0x00000, 0x10000, CRC(00e0e0d3) SHA1(89fa4d684ec36d5e974e39294efd65a9fd832517) )
	ROM_LOAD( "2605.2c",  0x28000, 0x10000, CRC(5459ebda) SHA1(86e51f0c120de87be8f51b498a562360e6b242b8) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2601.2h",  0x000000, 0x20000, CRC(70a01fc7) SHA1(0ed2f7c258f3cd82229bea7514d262fca57bd925) )
	ROM_LOAD( "2602.2g",  0x020000, 0x20000, CRC(d9167c10) SHA1(0fa34a065b3ffd5d35d03275bdcdf753045d6491) )
	ROM_LOAD( "2603.2f",  0x040000, 0x20000, CRC(11892916) SHA1(0680ab77fc1a2cdb78637bf0c506f03ca514014b) )
	ROM_LOAD( "2604.2e",  0x060000, 0x20000, CRC(3cc1a65d) SHA1(221dc17042e46e58dc4634eef798568747aef3a2) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "d26_2.9e", 0x000, 0x200, CRC(d6db5c60) SHA1(89ee10d092011c2c4eaab2c097aa88f5bb98bb97) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "d26_1.8e", 0x200, 0x200, CRC(af5edf32) SHA1(7202e0aa1ee3f22e3c5fb69a88db455a241929c5) )
ROM_END


/***************************************************************************

Maya
Promat, 1994

PCB Layout
----------

    6845      6264   3
 DSW1  DSW2    1     4
   YM2203      2     5
   3014B

              Z80
  22.1184MHz

 PROM1  TPC1020  D41264
 PROM2            (x6)


Notes:
      Z80 Clock: 5.522MHz
          HSync: 15.925 kHz
          VSync: 60Hz


***************************************************************************/

ROM_START( maya )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "1.17e", 0x00000, 0x10000, CRC(5aaa015e) SHA1(b84d02b1b6c07636176f226fef09a034d00445f0) )
	ROM_LOAD( "2.15e", 0x28000, 0x10000, CRC(7ea5b49a) SHA1(aaae848669d9f88c0660f46cc801e4eb0f5e3b89) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD( "3.18g", 0x00000, 0x40000, CRC(8534af04) SHA1(b9bc94541776b5c0c6bf0ecc63ffef914756376e) )
	ROM_LOAD( "4.17g", 0x40000, 0x40000, CRC(ab85ce5e) SHA1(845b846e0fb8c9fcd1540960cda006fdac364fea) )
	ROM_LOAD( "5.15g", 0x80000, 0x40000, CRC(c4316dec) SHA1(2e727a491a71eb1f4d9f338cc6ec76e03f7b46fd) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "prom2.5b",  0x000, 0x200, CRC(d276bf61) SHA1(987058b37182a54a360a80a2f073b000606a11c9) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "prom1.6b",  0x200, 0x200, CRC(e38eb360) SHA1(739960dd57ec3305edd57aa63816a81ddfbebf3e) )
ROM_END

ROM_START( inca )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "am27c512.1", 0x00000, 0x10000, CRC(b0d513f7) SHA1(65ef4702302bbfc7c7a77f7353120ee3f5c94b31) )
	ROM_LOAD( "2.15e", 0x28000, 0x10000, CRC(7ea5b49a) SHA1(aaae848669d9f88c0660f46cc801e4eb0f5e3b89) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD( "m27c2001.3", 0x00000, 0x40000, CRC(3a3c54ea) SHA1(2743c6a66d3eff60080c9183fa2bf9081207ac6b) )
	ROM_LOAD( "am27c020.4", 0x40000, 0x40000, CRC(d3571d63) SHA1(5f0abb0da19af34bbd3eb93270311e824839deb4) )
	ROM_LOAD( "m27c2001.5", 0x80000, 0x40000, CRC(bde60c29) SHA1(3ff7fbd5978bec27ff2ecf5977f640c66058e45d) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "n82s147n.2",  0x000, 0x200, CRC(268bd9d3) SHA1(1f77d9dc58ab29f013ee21d7ec521b90be72610d) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "n82s147n.1",  0x200, 0x200, CRC(618dbeb3) SHA1(10c8a558430fd1c2cabf9133d3e4f0a5f80eab83) )
ROM_END


static DRIVER_INIT( maya )
{
	/* Address lines scrambling on 1 z80 rom */
	int i;
	UINT8	*gfx = (UINT8 *)memory_region(REGION_GFX1);
	UINT8	*rom = memory_region(REGION_CPU1) + 0x28000,
			*end = rom + 0x10000;
	for (;rom < end; rom+=8)
	{
		UINT8 temp[8];
		temp[0] = rom[0];	temp[1] = rom[1];	temp[2] = rom[2];	temp[3] = rom[3];
		temp[4] = rom[4];	temp[5] = rom[5];	temp[6] = rom[6];	temp[7] = rom[7];

		rom[0] = temp[0];	rom[1] = temp[4];	rom[2] = temp[1];	rom[3] = temp[5];
		rom[4] = temp[2];	rom[5] = temp[6];	rom[6] = temp[3];	rom[7] = temp[7];
	}

	/* Address lines scrambling on the blitter data roms */
	rom = malloc(0xc0000);
	memcpy(rom, gfx, 0xc0000);
	for (i = 0; i < 0xc0000; i++)
		gfx[i] = rom[BITSWAP24(i,23,22,21,20,19,18,14,15, 16,17,13,12,11,10,9,8, 7,6,5,4,3,2,1,0)];
	free(rom);
}


/***************************************************************************

Mahjong Dial Q2
(c)1991 Dynax
D5212298L-1

CPU  : Z80
Sound: YM2413
OSC  : (240-100 624R001 KSSOB)?
Other: TC17G032AP-0246
CRT Controller: HD46505SP (6845)

***************************************************************************/

ROM_START( mjdialq2 )
	ROM_REGION( 0x78000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "5201.2b", 0x00000, 0x10000, CRC(5186c2df) SHA1(f05ae3fd5e6c39f3bf2263eaba645d89c454bd70) )
	ROM_RELOAD(          0x10000, 0x08000 )				// 1
	ROM_CONTINUE(        0x20000, 0x08000 )				// 3
	ROM_LOAD( "5202.2c", 0x30000, 0x08000, CRC(8e8b0038) SHA1(44130bb29b569610826e1fc7e4b2822f0e1034b1) )	// 5
	ROM_CONTINUE(        0x70000, 0x08000 )				// d

	ROM_REGION( 0xa0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "5207.2h", 0x00000, 0x20000, CRC(7794cd62) SHA1(7fa2fd50d7c975c381dda36f505df0e152196bb5) )
	ROM_LOAD( "5206.2g", 0x20000, 0x20000, CRC(9e810021) SHA1(cf1052c96b9da3abb263be1ce8481aeded2c5d00) )
	ROM_LOAD( "5205.2f", 0x40000, 0x20000, CRC(8c05572f) SHA1(544a5eb8b989fb1195986ed856da04350941ef59) )
	ROM_LOAD( "5204.2e", 0x60000, 0x20000, CRC(958ef9ab) SHA1(ec768c587dc9e6b691564b6b35abbece252bcd28) )
	ROM_LOAD( "5203.2d", 0x80000, 0x20000, CRC(706072d7) SHA1(d4692296d234b824961a94390e6d646ed9a7d5fd) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "d52-2.9e", 0x000, 0x200, CRC(18585ce3) SHA1(7f2e20bb09c1d810910094a6b19e5151666d74ac) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "d52-1.8e", 0x200, 0x200, CRC(8868247a) SHA1(97652025c411b379dfab576dc7f2d8d0d61d0828) )
ROM_END


/***************************************************************************

Mahjong Yarunara
(c)1991 Dynax
D5512068L1-1
D4508308L-2 (sub board)

CPU  : Z80B
Sound: AY-3-8912A YM2413 M5205
OSC  : 22.000MHz (near main CPU), 14.31818MHz (Sound section)
       YC-38 (X1), 384KHz (X2, M5205)

ROMs (all ROMs are 541000 = 27C010 compatible):
5501M.2D     [d86fade5]
5502M.4D     [1ef09ff0]
5503M.8C     [9276a10a]
5504M.9C     [6ac42304]
5505M.10C    [b2ca9838]
5506M.11C    [161058fd]
5507M.13C    [7de17b26]
5508M.16C    [ced3155b]
5509M.17C    [ca46ed48]

Subboard ROMs (5515M is 27C040, others are 541000):
5510M.2A     [bb9c71e1]
5511M.3A     [40ee77d8]
5512M.4A     [b4220316]
5513M.1B     [32b7bcbd]
5514M.2B     [ac714bb7]
5515M.4B     [ef130237]

PALs (not dumped):
D55A.4E
D55B.11F
D55C.16N
D55D.17D
D55EH.6A

CRT controller:
HD46505SP (6845)

Real time clock:
OKI M6242B

Custom chip:
DYNAX NL-001 WD10100

***************************************************************************/

ROM_START( yarunara )
	ROM_REGION( 0x10000 + 0x1d*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "5501m.2d",  0x00000, 0x20000, CRC(d86fade5) SHA1(4ae5e22972eb4ead9aa4a455ff1a18e128c33ed6) )
	ROM_RELOAD(            0x10000, 0x20000 )
	ROM_LOAD( "5502m.4d",  0x30000, 0x20000, CRC(1ef09ff0) SHA1(bbedcc1c0f5b43c78e0c3ce0fc1a3c28025562ec) )

	ROM_REGION( 0x3a0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "5507m.13c", 0x000000, 0x80000, CRC(7de17b26) SHA1(326667063ab045ac50e850f2f7821a65317879ad) )

	ROM_LOAD( "5508m.16c", 0x100000, 0x20000, CRC(ced3155b) SHA1(658e3947781f1be2ee87b43952999281c66683a6) )
	ROM_LOAD( "5509m.17c", 0x120000, 0x20000, CRC(ca46ed48) SHA1(0769ac0b211181b7b57033f09f72828c885186cc) )
	ROM_LOAD( "5506m.11c", 0x140000, 0x20000, CRC(161058fd) SHA1(cfc21abdc036e874d34bfa3c60486a5ab87cf9cd) )
	ROM_LOAD( "5505m.10c", 0x160000, 0x20000, CRC(b2ca9838) SHA1(7104697802a0466fab40414a467146a224eb6a74) )
	ROM_LOAD( "5504m.9c",  0x180000, 0x20000, CRC(6ac42304) SHA1(ce822da6d61e68578c08c9f1d0af1557c64ac5ae) )
	ROM_LOAD( "5503m.8c",  0x1a0000, 0x20000, CRC(9276a10a) SHA1(5a68fff20631a2002509d6cace06b5a9fa0e75d2) )

	ROM_LOAD( "5515m.4b",  0x200000, 0x80000, CRC(ef130237) SHA1(2c8f7a15249115b2cdcb3a8e0896ea8601e323d9) )

	ROM_LOAD( "5514m.2b",  0x300000, 0x20000, CRC(ac714bb7) SHA1(64056cbed9d0c4f68611921754c3e6a9bb14f7cc) )
	ROM_LOAD( "5513m.1b",  0x320000, 0x20000, CRC(32b7bcbd) SHA1(13277ae3f158da332e69c6f4f8828dfabbf3ea0a) )
	ROM_LOAD( "5512m.4a",  0x340000, 0x20000, CRC(b4220316) SHA1(b0797c9c6ab226520d29c780ea709f62e02dd268) )
	ROM_LOAD( "5511m.3a",  0x360000, 0x20000, CRC(40ee77d8) SHA1(e0dd9750d8b7b7dd9695a8365bdc926bd6d9f886) )
	ROM_LOAD( "5510m.2a",  0x380000, 0x20000, CRC(bb9c71e1) SHA1(21f2977196aaa27b76ee6547a08aba8da7aba76c) )
ROM_END


/***************************************************************************

Quiz TV Gassyuukoku Q&Q (JPN ver.)
(c)1992 Dynax

DX-BASE (Dynax Motherboard System) D5512068L1-2
D6410288L-1 (SUB)

6401.2D   prg. / samples
6402.4D
6403.5D

6404.S2A  chr.
6405.S2B
6406.S2C
6407.S2D
6408.S2E
6409.S2F
6410.S2G
6411.S2H
6412.S3A
6413.S3B

***************************************************************************/

ROM_START( quiztvqq )
	ROM_REGION( 0x10000 + 0x28*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "6401.2d",      0x000000, 0x020000, CRC(ce0e237c) SHA1(fd94a45052e3a68ef8cda2853b911a9993675fa6) )
	// 14-17
	ROM_RELOAD(               0x0b0000, 0x020000 )
	// 04-07
	ROM_LOAD( "6402.4d",      0x030000, 0x020000, CRC(cf7a9aa8) SHA1(9eaa8b318479e82cbcf133c227c61be92d282996) )
	// 24-27
	ROM_CONTINUE(             0x130000, 0x020000 )
	// 08-0b
	ROM_LOAD( "6403.5d",      0x050000, 0x020000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	ROM_REGION( 0x1a0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "6404.s2a",     0x000000, 0x080000, CRC(996ebe0f) SHA1(6492644aa14b0c2859add31878b5a8d7870981c8) )
	ROM_LOAD( "6405.s2b",     0x080000, 0x020000, CRC(666bfb03) SHA1(e345a198d3e1bc69f22c6f43869ffa2b1501c4ad) )
	ROM_LOAD( "6406.s2c",     0x0a0000, 0x020000, CRC(006871ef) SHA1(ebf78b2e46e26d98a7d8952bd29e78c893243c7a) )
	ROM_LOAD( "6407.s2d",     0x0c0000, 0x020000, CRC(9cc61541) SHA1(a3c0e06c6ad77cb7b2e86a70c2e27e6a74c35f12) )
	ROM_LOAD( "6408.s2e",     0x0e0000, 0x020000, CRC(65a98946) SHA1(4528b300fa3b01d992cf50e87430105463ea3fbd) )
	ROM_LOAD( "6409.s2f",     0x100000, 0x020000, CRC(d5d11061) SHA1(c7ab5aedde6998d62427cc7c4bcf767e9b832a60) )
	ROM_LOAD( "6410.s2g",     0x120000, 0x020000, CRC(bd769d46) SHA1(46f1f9e36f7b5f8deec5f7cce8c0992178ad3be0) )
	ROM_LOAD( "6411.s2h",     0x140000, 0x020000, CRC(7bd43065) SHA1(13b4fcc4155f555ec0c7fbb2f3bb6c19c2788cf5) )
	ROM_LOAD( "6412.s3a",     0x160000, 0x020000, CRC(43e645f3) SHA1(67a2975d4428142a2fbfd1d7b20139a15757bfb4) )
	ROM_LOAD( "6413.s3b",     0x180000, 0x020000, CRC(f7b81238) SHA1(447d983971bed978816dd836504ffcfae0023a69) )
ROM_END


/***************************************************************************

Mahjong Angels (Comic Theater Vol.2)
(c)1991 Dynax

DX-BASE (Dynax Motherboard System) D5512068L1-1
D6107068L-1 (SUB)

612-01.2D   prg. / samples
612-02.4D
612-03.5D

612-04.S1A  chr.
612-05.S2A
612-06.S1B
612-07.S2B
612-08.S3C
612-09.S4C
612-10.S5C
612-11.S6C

***************************************************************************/

ROM_START( mjangels )
	ROM_REGION( 0x10000 + 0x28*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "612-01.2d",    0x000000, 0x020000, CRC(cd353ba9) SHA1(8344dc5dd482ad6d36aa1e6b5824a09a3627dc65) )
	// 00-03
	ROM_RELOAD(               0x010000, 0x20000 )
	// 0c-0f
	ROM_RELOAD(               0x070000, 0x20000 )
	// 24-27
	ROM_RELOAD(               0x130000, 0x20000 )
	// 04-07
	ROM_LOAD( "612-02.4d",    0x030000, 0x020000, CRC(c1be70f9) SHA1(a0bd266b15c1677e5f41411217ca91d25c75e347) )
	// 08-0b
	ROM_LOAD( "612-03.5d",    0x050000, 0x020000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	ROM_REGION( 0x1c0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "612-04.s1a",   0x000000, 0x080000, CRC(c9b568a0) SHA1(e6c68ee4871020ded48e8a92546a8183a25f331f) )
	ROM_LOAD( "612-05.s2a",   0x080000, 0x040000, CRC(2ed51c5d) SHA1(0d912f8dc64f8fae35ca61cc0a938187a13ab328) )

	ROM_LOAD( "612-06.s1b",   0x100000, 0x020000, CRC(8612904d) SHA1(5386e93ad16146ce4e48e81df304e8bf9d2db199) )
	ROM_LOAD( "612-07.s2b",   0x120000, 0x020000, CRC(0828c59d) SHA1(60c451de062c9e0000875022329450a55e913a3c) )
	ROM_LOAD( "612-11.s6c",   0x140000, 0x020000, CRC(473b6fcd) SHA1(1b99b1370bc739f0f00671c6b6cbb3255d581b55) )
	ROM_LOAD( "612-10.s5c",   0x160000, 0x020000, CRC(bf1edb0e) SHA1(932ca328c5968529d52b2c629b6bb866cfa1e784) )
	ROM_LOAD( "612-09.s4c",   0x180000, 0x020000, CRC(8345999e) SHA1(c70c731ababcb28752dd4961d6dc54d43cb6bcba) )
	ROM_LOAD( "612-08.s3c",   0x1a0000, 0x020000, CRC(aad88516) SHA1(e6c7ef3325a17b2945530847998d314685c39f5d) )
ROM_END


/***************************************************************************

Mahjong Campus Hunting
(c)1990 Dynax
D3312108L1-1
D23SUB BOARD1 (sub board)

CPU  : Z80B
Sound: AY-3-8912A YM2413 M5205
OSC  : 22MHz (X1, near main CPU), 384KHz (X2, near M5205)
       3.58MHz (X3, Sound section)

ROMs:
3309.20      [0c7d72f0] OKI M271000ZB
3310.21      [28f5f194]  |
3311.22      [cddbf667]  |
3312.1       [cf0afbb5]  |
3313.2       [36e25beb]  |
3314.3       [f1cf01bc]  |
3315.4       [7cac01c7] /

3316.10      [44006ee5] M5M27C101P
3317.11      [4bb62bb4] /
3318.12      [e3b457a8] 27C010

Subboard ROMs:
3301.1B      [8ec98d60] OKI M271000ZB
3302.2B      [d7024f2d]  |
3303.3B      [01548edc]  |
3304.4B      [deef9a4e]  |
3305.1A      [8a9ebab8]  |
3306.2A      [86afcc80]  |
3307.3A      [07dbaf8a]  |
3308.4A      [a2cac53d] /

PALs:
D33A.24 (16L8)
D33B.79 (16L8)
D33C.67 (16R8)

CRT Controller:
HD46505SP (6845)

Custom chip:
DYNAX TC17G032AP-0246 JAPAN 8951EAY

***************************************************************************/

ROM_START( mcnpshnt )
	ROM_REGION( 0x10000 + 0xc*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "3318.12", 0x000000, 0x020000, CRC(e3b457a8) SHA1(b768895797157cad029ac1f652a838ecf6587d4f) )
	ROM_RELOAD(          0x010000, 0x020000 )
	ROM_LOAD( "3317.11", 0x030000, 0x020000, CRC(4bb62bb4) SHA1(0de5605cecb1e729a5b5b866274395945cf88aa3) )
	ROM_LOAD( "3316.10", 0x050000, 0x020000, CRC(44006ee5) SHA1(287ffd095755dc2a1e40e667723985c9052fdcdf) )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "3310.21", 0x000000, 0x020000, CRC(28f5f194) SHA1(ceb4605b25c49b6e6087e2e2f5db608d7e3ed0b2) )
	ROM_LOAD( "3311.22", 0x020000, 0x020000, CRC(cddbf667) SHA1(fbf94b8fdbe09cec5469c5f09d28e4d206763f90) )
	ROM_LOAD( "3309.20", 0x040000, 0x020000, CRC(0c7d72f0) SHA1(cbd0f29a31eab565b0e31fe1612e73164e6c61b4) )
	ROM_LOAD( "3312.1",  0x060000, 0x020000, CRC(cf0afbb5) SHA1(da340d49fb9513014f619124af56c115932cf18c) )
	ROM_LOAD( "3313.2",  0x080000, 0x020000, CRC(36e25beb) SHA1(21577849b356192d32d990d02d03092aa344e92e) )
	ROM_LOAD( "3314.3",  0x0a0000, 0x020000, CRC(f1cf01bc) SHA1(fb02593d8b772b5e0128017998a0e15fc0708898) )
	ROM_LOAD( "3315.4",  0x0c0000, 0x020000, CRC(7cac01c7) SHA1(cee5f157a23087b97709ff860078572b389e60cb) )

	ROM_LOAD( "3301.1b", 0x200000, 0x020000, CRC(8ec98d60) SHA1(e98d947096abb78e91c3013ede9eae7719b1d7b9) )
	ROM_LOAD( "3302.2b", 0x220000, 0x020000, CRC(d7024f2d) SHA1(49dfc26dc91a8632459852968766a5263be138eb) )
	ROM_LOAD( "3303.3b", 0x240000, 0x020000, CRC(01548edc) SHA1(a64b509a744dd010997d5b2cd4d12d2767dde6c8) )
	ROM_LOAD( "3304.4b", 0x260000, 0x020000, CRC(deef9a4e) SHA1(e0be7ba644e383d669a5ff1eb24c46461cc586a5) )
	ROM_LOAD( "3308.4a", 0x280000, 0x020000, CRC(a2cac53d) SHA1(fc580a85c94748afc1bbc49e25662e5a5cc8bb36) )
	ROM_LOAD( "3307.3a", 0x2a0000, 0x020000, CRC(07dbaf8a) SHA1(99f995b71ca116d2e5587e08f9b0b4493d96937b) )
	ROM_LOAD( "3306.2a", 0x2c0000, 0x020000, CRC(86afcc80) SHA1(e5d818761bb375b6c862546e238b2c6cf13898a8) )
	ROM_LOAD( "3305.1a", 0x2e0000, 0x020000, CRC(8a9ebab8) SHA1(755c40a64541518b27cfa94959feb5de6f55b358) )
ROM_END


/***************************************************************************

7jigen no Youseitachi (Mahjong 7 Dimensions)
(c)1990 Dynax

D3707198L1
D23SUB BOARD1

CPU:    Z80-B
Sound:  AY-3-8912A
    YM2413
    M5205
OSC:    22.000MHz
    3.58MHz
    384KHz
VDP:    HD46505SP
Custom: (TC17G032AP-0246)


3701.1A   prg.

3702.3A   samples
3703.4A

3704.S1B  chr.
3705.S2B
3706.S3B
3707.S4B
3708.S1A
3709.S2A
3710.S3A
3711.S4A
3712.14A
3713.16A
3714.17A
3715.18A
3716.20A
3717.17B

***************************************************************************/

ROM_START( 7jigen )
	ROM_REGION( 0x10000 + 0xc*0x8000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "3701.1a",      0x000000, 0x020000, CRC(ee8ab3c4) SHA1(9ccc9e9697dd452cd28e38c81cebea0b862f0642) )
	ROM_RELOAD(               0x010000, 0x020000 )
	ROM_LOAD( "3702.3a",      0x030000, 0x020000, CRC(4e43a0bb) SHA1(d98a1ab43dcfab3d2a17f99db797f7bfa17e5ecc) )
	ROM_LOAD( "3703.4a",      0x050000, 0x020000, CRC(ec77b564) SHA1(5e9d5540b300e88c3ecdb53bca38830621eb0382) )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "3713.16a",     0x000000, 0x020000, CRC(f3a745d2) SHA1(37b55e2c290b165a5afaf4c7b8539bb57dd0d927) )
	ROM_LOAD( "3712.14a",     0x020000, 0x020000, CRC(88786680) SHA1(34a31448a9f3e287d7c7fe478736771c5ef259e2) )
	ROM_LOAD( "3715.18a",     0x040000, 0x020000, CRC(19f7ab13) SHA1(ac11e43981e8667c2637b66d93ac052fb27e521d) )
	ROM_LOAD( "3716.20a",     0x060000, 0x020000, CRC(4f0c7f06) SHA1(e0bbbb69cdd16932778e0b2f67e7ed068991a0b9) )
	ROM_LOAD( "3717.17b",     0x080000, 0x020000, CRC(960cfd62) SHA1(df8ee9eb8617a5e8605170d404872e1c6f0987f0) )
	ROM_LOAD( "3714.17a",     0x0a0000, 0x020000, CRC(44ba5e35) SHA1(0c5c2b2a78aa397ea3d1264821ff717d093b81ae) )

	ROM_LOAD( "3704.s1b",     0x200000, 0x020000, CRC(26348ae4) SHA1(3659d18608848c58ad980a79bc1c29da238a5604) )
	ROM_LOAD( "3705.s2b",     0x220000, 0x020000, CRC(5b5ea036) SHA1(187a7f6356ead05d8e3d9f5efa82554004429780) )
	ROM_LOAD( "3706.s3b",     0x240000, 0x020000, CRC(7fdfb600) SHA1(ce4485e43ee6bf63b4e8e3bb91267295995c736f) )
	ROM_LOAD( "3707.s4b",     0x260000, 0x020000, CRC(67fa83ea) SHA1(f8b0012aaaf125b7266dbf1ae7df23d04d484e54) )
	ROM_LOAD( "3711.s4a",     0x280000, 0x020000, CRC(f1d4399d) SHA1(866af46900a4b04db69c838b7ec7e347a5fadd3d) )
	ROM_LOAD( "3710.s3a",     0x2a0000, 0x020000, CRC(0a92af7c) SHA1(4383dc8f3019b3b2716d32e1c91b0ac5b1e367c3) )
	ROM_LOAD( "3709.s2a",     0x2c0000, 0x020000, CRC(86f27f1c) SHA1(43b829597993d3043d5bbb0a468f603910638b87) )
	ROM_LOAD( "3708.s1a",     0x2e0000, 0x020000, CRC(8082d0ac) SHA1(44d708f8e307b782105082092edd3ea9affd2329) )
ROM_END


/***************************************************************************

Jantouki
(c)1989 Dynax

D1505178-A (main board)
D2711078L-B (ROM board)

CPU:    Z80-B
Sound:  Z80-B
        AY-3-8912A
        YM2203C
        M5205
OSC:    22.000MHz
VDP:    HD46505SP
Custom: (TC17G032AP-0246) x2


2702.6D  MROM1  main prg.
2701.6C  MROM2

2705.6G  SROM1  sound prg.
2704.6F  SROM2  sound data
2703.6E  SROM3

2709.3G  BROM1  chr.
2710.3F  BROM2
2711.3E  BROM3
2712.3D  BROM4
2713.3C  BROM5
2706.5G  BROM6
2707.5F  BROM7
2708.5E  BROM8

2718.1G  AROM1  chr.
2719.1F  AROM2
2720.1E  AROM3
2721.1D  AROM4
2722.1C  AROM5
2714.2G  AROM6
2715.2F  AROM7
2716.2E  AROM8
2717.2D  AROM9

27-1_19H.18G    color
27-2.20H.19G
***************************************************************************/

ROM_START( jantouki )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2702.6d", 0x000000, 0x010000, CRC(9e9bea93) SHA1(c8b1a0621d3dae37d809bdbaa4ed4af73847b714) )
	ROM_LOAD( "2701.6c", 0x010000, 0x010000, CRC(a58bc982) SHA1(5cdea3cdf3eaacb6bdf6ddb68e3d57fe53d70bb9) )

	ROM_REGION( 0x68000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "2705.6g", 0x000000, 0x010000, CRC(9d21e4af) SHA1(454601f4cb89da53c6881f4d8109d3c0babcfe5e) )
	// banks 4-b:
	ROM_LOAD( "2704.6f", 0x028000, 0x020000, CRC(4bb62bb4) SHA1(0de5605cecb1e729a5b5b866274395945cf88aa3) )
	ROM_LOAD( "2703.6e", 0x048000, 0x020000, CRC(44006ee5) SHA1(287ffd095755dc2a1e40e667723985c9052fdcdf) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2709.3g", 0x000000, 0x020000, CRC(e6dd4853) SHA1(85394e34eee95cd4430d062b3dbdfbe066c661b6) )
	ROM_LOAD( "2710.3f", 0x020000, 0x020000, CRC(7ef4d92f) SHA1(414e26242e824f5d4c40a039a3f3486f84338325) )
	ROM_LOAD( "2711.3e", 0x040000, 0x020000, CRC(8bfee4c2) SHA1(7c0e7535f7d7cd7f665e7925ff0cdab6b96a4b83) )
	ROM_LOAD( "2712.3d", 0x060000, 0x020000, CRC(6ecd4913) SHA1(00a2355d6cb1643b7cc964e702a4ac5cfe7906c5) )
	ROM_LOAD( "2713.3c", 0x080000, 0x020000, CRC(33272f5d) SHA1(8a23ef0e6ad24905fd5c249e8ea8560ec29a585c) )
	ROM_LOAD( "2706.5g", 0x0a0000, 0x020000, CRC(fd72b190) SHA1(3d790dc1e40cbf963d8413ea91e518e19973734d) )
	ROM_LOAD( "2707.5f", 0x0c0000, 0x020000, CRC(4ec7a81e) SHA1(a6227ca2b648ebc1a5a5f6fbfc6412c44752b77d) )
	ROM_LOAD( "2708.5e", 0x0e0000, 0x020000, CRC(45845dc9) SHA1(cec3f82e3440f724f59d8386c8d2b0e030703ed5) )

	ROM_REGION( 0x120000, REGION_GFX2, 0 )	/* blitter 2 data */
	ROM_LOAD( "2718.1g", 0x000000, 0x020000, CRC(65608d7e) SHA1(28a960450d2d1cfb314c574123c2fbc61f2ded51) )
	ROM_LOAD( "2719.1f", 0x020000, 0x020000, CRC(4cbc9361) SHA1(320d3ce504ad2e27937e7e3a761c672a22749658) )
	ROM_LOAD( "2720.1e", 0x040000, 0x020000, CRC(4c9a25e5) SHA1(0298a5dad034b1ac113f6e07f4e9334ed6e0e89b) )
	ROM_LOAD( "2721.1d", 0x060000, 0x020000, CRC(715c864a) SHA1(a4b436ddeaa161d6661063b6de503f07ecc5894a) )
	ROM_LOAD( "2722.1c", 0x080000, 0x020000, CRC(cc0b0cd7) SHA1(ccd3ff1cafbcaf87439a6dfe38b5057febc15012) )
	ROM_LOAD( "2714.2g", 0x0a0000, 0x020000, CRC(17341b6b) SHA1(0ae43e53429e9561a00ea9597299477f2c7ddf4b) )
	ROM_LOAD( "2715.2f", 0x0c0000, 0x020000, CRC(486b7138) SHA1(623ddb0e9a9444cf0e920b78562a4748fa1c54d9) )
	ROM_LOAD( "2716.2e", 0x0e0000, 0x020000, CRC(f388b0da) SHA1(4c04509eeda3f82bf6f8940a406e17423d0210a0) )
	ROM_LOAD( "2717.2d", 0x100000, 0x020000, CRC(3666bead) SHA1(2067bb894b76be2b51649bb1144e84e6ff0ab378) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "27-2_20h.19g", 0x000000, 0x000200, CRC(32d3f091) SHA1(ab9e8f467fc85357fb900bceae32909ce1f2d9c1) )
	ROM_LOAD( "27-1_19h.18g", 0x000200, 0x000200, CRC(9382a2a1) SHA1(0d14eb85017f87ddbe66e4f6443028e91540b36e) )
ROM_END


/***************************************************************************

    Mahjong Electron Base

***************************************************************************/

ROM_START( mjelctrn )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "u27b-020", 0x00000, 0x20000, CRC(7773d382) SHA1(1d2ae799677e99c7cba09b0a2c49bb9310232e80) )
	ROM_CONTINUE(         0x00000, 0x20000 )
	ROM_RELOAD(           0x10000, 0x20000 )
	ROM_CONTINUE(         0x28000, 0x08000 )
	ROM_CONTINUE(         0x20000, 0x08000 )
	ROM_CONTINUE(         0x18000, 0x08000 )
	ROM_CONTINUE(         0x10000, 0x08000 )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "eb-01.rom", 0x000000, 0x100000, CRC(e5c41448) SHA1(b8322e32b0cb3d771316c9c4f7be91de6e422a24) )
	ROM_LOAD( "eb-02.rom", 0x100000, 0x080000, CRC(e1f1b431) SHA1(04a612aff4c30cb8ea741f228bfa7e4289acfee8) )
	ROM_LOAD( "mj-1c020",  0x140000, 0x040000, CRC(f8e8d91b) SHA1(409e276157b328e7bbba5dda6a4c7adc020d519a) )
ROM_END

ROM_START( mjelct3 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "se-3010", 0x00000, 0x20000, CRC(370347e7) SHA1(2dc9f1fde4efaaff887722aae6507d7e9fac8eb6) )
	ROM_RELOAD(          0x10000, 0x08000 )
	ROM_CONTINUE(        0x28000, 0x08000 )
	ROM_CONTINUE(        0x20000, 0x08000 )
	ROM_CONTINUE(        0x18000, 0x08000 )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "eb-01.rom", 0x000000, 0x100000, CRC(e5c41448) SHA1(b8322e32b0cb3d771316c9c4f7be91de6e422a24) )
	ROM_LOAD( "eb-02.rom", 0x100000, 0x080000, CRC(e1f1b431) SHA1(04a612aff4c30cb8ea741f228bfa7e4289acfee8) )
ROM_END

ROM_START( mjelct3a )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "dz-00.rom", 0x00000, 0x20000, CRC(d28358f7) SHA1(995c16e0865048069f79411574256a88d58c6be9) )
	ROM_RELOAD(            0x10000, 0x08000 )
	ROM_CONTINUE(          0x28000, 0x08000 )
	ROM_CONTINUE(          0x20000, 0x08000 )
	ROM_CONTINUE(          0x18000, 0x08000 )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "eb-01.rom", 0x000000, 0x100000, CRC(e5c41448) SHA1(b8322e32b0cb3d771316c9c4f7be91de6e422a24) )
	ROM_LOAD( "eb-02.rom", 0x100000, 0x080000, CRC(e1f1b431) SHA1(04a612aff4c30cb8ea741f228bfa7e4289acfee8) )
ROM_END

/*

Sea Hunter Penguin

CPU
1x Z8400A (main)
1x YM2203C (sound)
1x blank DIP40 with GND on pin 1,22 and +5 on pin 20
1x oscillator 22.1184MHz
ROMs

2x 27512 (u43,u45)
6x 27C010

ROM u43.8d contains

MODEL:SEA HUNTER(EXT)
PROGRAM BY:WSAC SYSTEMS,.

DATE:1995/02/20

VER:1.30

*/

ROM_START( shpeng )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "u43.8d", 0x00000, 0x10000, CRC(6b993f68) SHA1(4d3ad750e23be93342c61c454498d432e40587bb) )
	ROM_LOAD( "u45.9d", 0x28000, 0x10000, CRC(6e79a1d1) SHA1(a72706425bcbd0faee4cf0220942fdcf510d4e89) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "u110.1j", 0x00000, 0x20000, CRC(31abac75) SHA1(20e91496ccb379d9449925b5aaca3532caaf9522) ) // ok! - main sprites etc.
	ROM_LOAD( "u111.0j", 0x20000, 0x20000, CRC(0672bfc9) SHA1(ea35af45cdfa72ae1e7dc13a09ed1db09c0062ec) ) // ok?
	ROM_LOAD( "u84.1h",  0x40000, 0x20000, CRC(7b476fac) SHA1(6b61b675fbfcc17a77b9757ea330f8d3e8751633) )
	ROM_LOAD( "u804.0h", 0x60000, 0x20000, CRC(abb2f1c3) SHA1(9faccbba26c0540d9edbd76ca8bf67069db0bb53) )
	ROM_LOAD( "u74.1g",  0x80000, 0x20000, CRC(2ac46b6e) SHA1(0046ee7ede1acff45e64c85a9fca8fc8efa31026) )
	ROM_LOAD( "u704.0g", 0xa0000, 0x20000, CRC(b062c928) SHA1(8c43689a1b8c444f91acbc7371eda744874eb538) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "n82s147n.u13", 0x000, 0x200, CRC(29b6415b) SHA1(8085ff3265cda2d564da3dff609eb05ff02fae49) ) // FIXED BITS (0xxxxxxx)  (Ok)
	ROM_LOAD( "n82s147n.u12", 0x200, 0x200, BAD_DUMP CRC(7b940daa) SHA1(3903ebef644b2519aebbbb6d16872441b283c780) ) // BADADDR xxx-xxxxx  (Bad Read, Prom has a broken leg!)

	/* this rom doesn't belong here, it is from Dragon Punch, but shpeng hardware and game code is a hack
       of dragon punch.  This rom is better than the bad dump above for the sprite colours, although the
       colours on the intro/cutscenes are wrong */
	ROM_LOAD_OPTIONAL( "1.17g", 0x200, 0x200, CRC(324fa9cf) SHA1(a03e23d9a9687dec4c23a8e41254a3f4b70c7e25) )
ROM_END


// Decrypted by yong
static DRIVER_INIT( mjelct3 )
{
	int i;
	UINT8	*rom = memory_region(REGION_CPU1);
	size_t  size = memory_region_length(REGION_CPU1);
	UINT8	*rom1 = malloc(size);
	if (rom1)
	{
		memcpy(rom1,rom,size);
		for (i = 0; i < size; i++)
			rom[i] = BITSWAP8(rom1[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8, 1,6,5,4,3,2,7, 0)], 7,6, 1,4,3,2,5,0);
		free(rom1);
	}
}

static DRIVER_INIT( mjelct3a )
{
	int i,j;
	UINT8	*rom = memory_region(REGION_CPU1);
	size_t  size = memory_region_length(REGION_CPU1);
	UINT8	*rom1 = malloc(size);
	if (rom1)
	{
		memcpy(rom1,rom,size);
		for (i = 0; i < size; i++)
		{
			j = i & ~0x7e00;
			switch(i & 0x7000)
			{
				case 0x0000:	j |= 0x0400;	break;
				case 0x1000:	j |= 0x4400;	break;
				case 0x2000:	j |= 0x4200;	break;
				case 0x3000:	j |= 0x0200;	break;
				case 0x4000:	j |= 0x4600;	break;
				case 0x5000:	j |= 0x4000;	break;
//              case 0x6000:    j |= 0x0000;    break;
				case 0x7000:	j |= 0x0600;	break;
			}
			switch(i & 0x0e00)
			{
				case 0x0000:	j |= 0x2000;	break;
				case 0x0200:	j |= 0x3800;	break;
				case 0x0400:	j |= 0x2800;	break;
				case 0x0600:	j |= 0x0800;	break;
				case 0x0800:	j |= 0x1800;	break;
//              case 0x0a00:    j |= 0x0000;    break;
				case 0x0c00:	j |= 0x1000;	break;
				case 0x0e00:	j |= 0x3000;	break;
			}
			rom[j] = rom1[i];
		}
		free(rom1);
	}

	init_mjelct3();
}


/***************************************************************************

Neruton Haikujiradan
(c)1990 Dynax / Yukiyoshi Tokoro (Illustration)
D4005208L1-1
D4508308L-2 (sub board)

CPU  : Z80?
Sound: AY-3-8912A YM2413 M5205
OSC  : 22MHz (near main CPU), 3.58MHz (Sound section)

ROMs (all ROMs are 27C010 compatible):
4501B.1A     [0e53eeee]
4502.3A      [c296293f]
4511.11A     [c4a96b6e]
4512.13A     [d7ebbcb9]
4513.14A     [e3bed454]
4514.15A     [ee258483]
4515.17A     [3bce0ca1]
4516.18A     [ee6b7e3b]
4517.19A     [b31f9694]
4518.17C     [fa88668e]
4519.18C     [68aca5f3]
4520.19C     [7bb2b298]

Subboard ROMs:
4503.1A      [dcbe2805]
4504.2A      [7b3387af]
4505.3A      [6f9fd275]
4506.4A      [6eac8b3c]
4507.1B      [106e6133]
4508.2B      [5c451ed4]
4509.3B      [4e1e6a2d]
4510.4B      [455305a1]


PALs:
10B (?)
10E (?)
15E (?)
D45SUB.6A (16L8)

CRT Controller:
HD46505SP (6845)

***************************************************************************/

ROM_START( neruton )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "4501b.1a",     0x000000, 0x020000, CRC(0e53eeee) SHA1(883138618a11295bfac148da4a092e01d92229b3) )
	ROM_RELOAD(               0x010000, 0x020000 )
	ROM_LOAD( "4502.3a",      0x030000, 0x020000, CRC(c296293f) SHA1(466e87f7eca102568f1f00c6ba77dacc3df300dd) )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "4511.11a",     0x000000, 0x020000, CRC(c4a96b6e) SHA1(15a6776509e0d30929f6a261798afe7dc0401d4e) )
	ROM_LOAD( "4512.13a",     0x020000, 0x020000, CRC(d7ebbcb9) SHA1(b8edd8b93eca8d36056c02f8b69ff8313c9ab120) )
	ROM_LOAD( "4513.14a",     0x040000, 0x020000, CRC(e3bed454) SHA1(03a66d31b8f41abc4ce83ebe22f8d14414d92152) )
	ROM_LOAD( "4514.15a",     0x060000, 0x020000, CRC(ee258483) SHA1(8c685fee4eaff5978f0ec222c33d55123a8fa496) )
	ROM_LOAD( "4515.17a",     0x080000, 0x020000, CRC(3bce0ca1) SHA1(1d0bb379077c52a63aa982bbe77f89df7b5b7b14) )
	ROM_LOAD( "4516.18a",     0x0a0000, 0x020000, CRC(ee6b7e3b) SHA1(5290fad850c7a52039cd9d26082bff8615bf3797) )
	ROM_LOAD( "4517.19a",     0x0c0000, 0x020000, CRC(b31f9694) SHA1(f22fc44908be4f1ef8dada57860f95ee74495605) )
	ROM_LOAD( "4519.18c",     0x0e0000, 0x020000, CRC(68aca5f3) SHA1(f03328362777e6d536f730bc3b52371d5daca54e) )
	ROM_LOAD( "4520.19c",     0x100000, 0x020000, CRC(7bb2b298) SHA1(643d21f6a45640bad5ec84af9745339487a7408c) )
	ROM_LOAD( "4518.17c",     0x120000, 0x020000, CRC(fa88668e) SHA1(fce80a8badacf39f30c36952cbe0a1491b8faef1) )

	ROM_LOAD( "4510.4b",      0x200000, 0x020000, CRC(455305a1) SHA1(103e1eaac485b37786a1d1d411819788ed385467) )
	ROM_LOAD( "4509.3b",      0x220000, 0x020000, CRC(4e1e6a2d) SHA1(04c71dd11594921142b6aa9554c0fe1b40254463) )
	ROM_LOAD( "4508.2b",      0x240000, 0x020000, CRC(5c451ed4) SHA1(59a27ddfae541cb61dafb32bdb5de8ddbc5abb8d) )
	ROM_LOAD( "4507.1b",      0x260000, 0x020000, CRC(106e6133) SHA1(d08deb17ea82fe43e458a11eea26ce98c26c51c1) )
	ROM_LOAD( "4506.4a",      0x280000, 0x020000, CRC(6eac8b3c) SHA1(70dbe3af582384571872e7b6b51df4192daed227) )
	ROM_LOAD( "4505.3a",      0x2a0000, 0x020000, CRC(6f9fd275) SHA1(123a928dcb60624d61a55b2fef25156975ba26c9) )
	ROM_LOAD( "4504.2a",      0x2c0000, 0x020000, CRC(7b3387af) SHA1(403cf67287469ae6ce9a7f662f6d82f62dac349b) )
	ROM_LOAD( "4503.1a",      0x2e0000, 0x020000, CRC(dcbe2805) SHA1(713edd2e3c950bc689446441eb85197bb7b1eb89) )
ROM_END


/***************************************************************************


                                Game Drivers


***************************************************************************/

GAME( 1988, hanamai,  0,        hanamai,  hanamai,  0,        ROT180, "Dynax",                   "Hana no Mai (Japan)"                                  , 0 )
GAME( 1989, hnkochou, hanamai,  hanamai,  hnkochou, 0,        ROT180, "Dynax",                   "Hana Kochou [BET] (Japan)"                            , 0 )
GAME( 1989, hnoridur, 0,        hnoridur, hnoridur, 0,        ROT180, "Dynax",                   "Hana Oriduru (Japan)",                                 GAME_IMPERFECT_GRAPHICS ) // 1 rom is bad
GAME( 1989, drgpunch, 0,        sprtmtch, sprtmtch, 0,        ROT0,   "Dynax",                   "Dragon Punch (Japan)"                                 , 0 )
GAME( 1989, sprtmtch, drgpunch, sprtmtch, sprtmtch, 0,        ROT0,   "Dynax (Fabtek license)",  "Sports Match"                                         , 0 )
GAME( 1989, mjfriday, 0,        mjfriday, mjfriday, 0,        ROT180, "Dynax",                   "Mahjong Friday (Japan)"                               , 0 )
GAME( 1990, mcnpshnt, 0,        mcnpshnt, mcnpshnt, 0,        ROT0,   "Dynax",                   "Mahjong Campus Hunting (Japan)"                       , 0 )
GAME( 1990, 7jigen,   0,        nanajign, nanajign, 0,        ROT180, "Dynax",                   "7jigen no Youseitachi - Mahjong 7 Dimensions (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1991, mjdialq2, 0,        mjdialq2, mjdialq2, 0,        ROT180, "Dynax",                   "Mahjong Dial Q2 (Japan)"                              , 0 )
GAME( 1991, yarunara, 0,        yarunara, yarunara, 0,        ROT180, "Dynax",                   "Mahjong Yarunara (Japan)"                             , 0 )
GAME( 1991, mjangels, 0,        yarunara, yarunara, 0,        ROT180, "Dynax",                   "Mahjong Angels - Comic Theater Vol.2 (Japan)"         , 0 )
GAME( 1992, quiztvqq, 0,        yarunara, quiztvqq, 0,        ROT180, "Dynax",                   "Quiz TV Gassyuukoku Q&Q (Japan)"                      , 0 )
GAME( 1994, maya,     0,        sprtmtch, sprtmtch, maya,     ROT0,   "Promat",                  "Maya"                                                 , 0 )
GAME( 199?, inca,     maya,     sprtmtch, sprtmtch, maya,     ROT0,   "<unknown>",               "Inca"                                                 , 0 )
GAME( 1990, jantouki, 0,        jantouki, jantouki, 0,        ROT0,   "Dynax",                   "Jong Tou Ki (Japan)"                                  , 0 )
GAME( 1993, mjelctrn, 0,        mjelctrn, mjelctrn, mjelct3,  ROT180, "Dynax",                   "Mahjong Electron Base (parts 2 & 4, Japan)"           , 0 )
GAME( 1990, mjelct3,  mjelctrn, mjelctrn, mjelct3,  mjelct3,  ROT180, "Dynax",                   "Mahjong Electron Base (parts 2 & 3, Japan)"           , 0 )
GAME( 1990, mjelct3a, mjelctrn, mjelctrn, mjelct3,  mjelct3a, ROT180, "Dynax",                   "Mahjong Electron Base (parts 2 & 3, alt., Japan)"     , 0 )
GAME( 1990, neruton,  0,        neruton,  neruton,  mjelct3,  ROT180, "Dynax / Yukiyoshi Tokoro","Mahjong Neruton Haikujirada (Japan)",                         GAME_IMPERFECT_GRAPHICS )	// e.g. dynax logo
/* not a dynax board */
GAME( 1995, shpeng,   0,        sprtmtch, sprtmtch, 0,        ROT0,   "WSAC Systems?",                   "Sea Hunter Penguin"                                 , GAME_WRONG_COLORS ) // proms?
