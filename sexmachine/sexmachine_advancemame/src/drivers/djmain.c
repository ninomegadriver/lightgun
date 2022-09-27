/*
 *  Beatmania DJ Main Board (GX753)
 *
 *  Product numbers:
 *  GQ753 beatmania (first release in 1997)
 *  Gx853 beatmania 2nd MIX (1998)
 *  Gx825 beatmania 3rd MIX
 *  Gx858 beatmania complete MIX (1999)
 *  Gx847 beatmania 4th MIX
 *  Gx981 beatmania 5th MIX
 *  Gx993 beatmania Club MIX (2000)
 *  Gx988 beatmania complete MIX 2
 *  Gx995 beatmania featuring Dreams Come True
 *  GxA05 beatmania CORE REMIX
 *  GxA21 beatmania 6th MIX (2001)
 *  GxB07 beatmania 7th MIX
 *  GxC01 beatmania THE FINAL (2002)
 *
 *  Gx803 Pop'n Music 1 (1998)
 *  Gx831 Pop'n Music 2
 *  Gx980 Pop'n Music 3 (1999)
 *
 *  ????? Pop'n Stage
 *  Gx970 Pop'n Stage EX (1999)
 *
 *  Chips:
 *  15a:    MC68EC020FG25
 *  25b:    001642
 *  18d:    055555 (priority encoder)
 *   5f:    056766 (sprites)
 *  18f:    056832 (tiles)
 *  22f:    058143 = 054156 (tiles)
 *  12j:    058141 = 054539 (x2) (2 sound chips in one)
 *
 *  TODO:
 *  - correct FPS
 *
 */

#include "driver.h"
#include "artwork.h"
#include "cpu/m68000/m68000.h"
#include "machine/idectrl.h"
#include "sound/k054539.h"
#include "vidhrdw/konamiic.h"


extern UINT32 *djmain_obj_ram;

VIDEO_UPDATE( djmain );
VIDEO_START( djmain );


static int sndram_bank;
static UINT8 *sndram;

static int turntable_select;
static UINT8 turntable_last_pos[2];
static UINT16 turntable_pos[2];

static UINT8 pending_vb_int;
static UINT16 v_ctrl;
static UINT32 obj_regs[0xa0/4];

#define DISABLE_VB_INT	(!(v_ctrl & 0x8000))



/*************************************
 *
 *  68k CPU memory handlers
 *
 *************************************/

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

 	r = (data >>  0) & 0xff;
	g = (data >>  8) & 0xff;
	b = (data >> 16) & 0xff;

	palette_set_color(offset, r, g, b);
}


//---------

static void sndram_set_bank(void)
{
	sndram = memory_region(REGION_SOUND1) + 0x80000 * sndram_bank;
}

static WRITE32_HANDLER( sndram_bank_w )
{
	if (ACCESSING_MSW32)
	{
		sndram_bank = (data >> 16) & 0x1f;
		sndram_set_bank();
	}
}

static READ32_HANDLER( sndram_r )
{
	UINT32 data = 0;

	if ((mem_mask & 0xff000000) == 0)
		data |= sndram[offset * 4] << 24;

	if ((mem_mask & 0x00ff0000) == 0)
		data |= sndram[offset * 4 + 1] << 16;

	if ((mem_mask & 0x0000ff00) == 0)
		data |= sndram[offset * 4 + 2] << 8;

	if ((mem_mask & 0x000000ff) == 0)
		data |= sndram[offset * 4 + 3];

	return data;
}

static WRITE32_HANDLER( sndram_w )
{
	if ((mem_mask & 0xff000000) == 0)
		sndram[offset * 4] = data >> 24;

	if ((mem_mask & 0x00ff0000) == 0)
		sndram[offset * 4 + 1] = data >> 16;

	if ((mem_mask & 0x0000ff00) == 0)
		sndram[offset * 4 + 2] = data >> 8;

	if ((mem_mask & 0x000000ff) == 0)
		sndram[offset * 4 + 3] = data;
}


//---------

static READ16_HANDLER( dual539_16_r )
{
	UINT16 ret = 0;

	if (ACCESSING_LSB16)
		ret |= K054539_1_r(offset);
	if (ACCESSING_MSB16)
		ret |= K054539_0_r(offset)<<8;

	return ret;
}

static WRITE16_HANDLER( dual539_16_w )
{
	if (ACCESSING_LSB16)
		K054539_1_w(offset, data);
	if (ACCESSING_MSB16)
		K054539_0_w(offset, data>>8);
}

static READ32_HANDLER( dual539_r )
{
	UINT32 data = 0;

	if (~mem_mask & 0xffff0000)
		data |= dual539_16_r(offset * 2, mem_mask >> 16) << 16;
	if (~mem_mask & 0x0000ffff)
		data |= dual539_16_r(offset * 2 + 1, mem_mask);

	return data;
}

static WRITE32_HANDLER( dual539_w )
{
	if (~mem_mask & 0xffff0000)
		dual539_16_w(offset * 2, data >> 16, mem_mask >> 16);
	if (~mem_mask & 0x0000ffff)
		dual539_16_w(offset * 2 + 1, data, mem_mask);
}


//---------

static READ32_HANDLER( obj_ctrl_r )
{
	// read obj_regs[0x0c/4]: unknown
	// read obj_regs[0x24/4]: unknown

	return obj_regs[offset];
}

static WRITE32_HANDLER( obj_ctrl_w )
{
	// write obj_regs[0x28/4]: bank for rom readthrough

	COMBINE_DATA(&obj_regs[offset]);
}

static READ32_HANDLER( obj_rom_r )
{
	UINT8 *mem8 = memory_region(REGION_GFX1);
	int bank = obj_regs[0x28/4] >> 16;

	offset += bank * 0x200;
	offset *= 4;

	if (~mem_mask & 0x0000ffff)
		offset += 2;

	if (~mem_mask & 0xff00ff00)
		offset++;

	return mem8[offset] * 0x01010101;
}


//---------

static WRITE32_HANDLER( v_ctrl_w )
{
	if (ACCESSING_MSW32)
	{
		data >>= 16;
		mem_mask >>= 16;
		COMBINE_DATA(&v_ctrl);

		if (pending_vb_int && !DISABLE_VB_INT)
		{
			pending_vb_int = 0;
			cpunum_set_input_line(0, MC68000_IRQ_4, HOLD_LINE);
		}
	}
}

static READ32_HANDLER( v_rom_r )
{
	UINT8 *mem8 = memory_region(REGION_GFX2);
	int bank = K056832_word_r(0x34/2, 0xffff);

	offset *= 2;

	if (!ACCESSING_MSB32)
		offset += 1;

	offset += bank * 0x800 * 4;

	if (v_ctrl & 0x020)
		offset += 0x800 * 2;

	return mem8[offset] * 0x01010000;
}


//---------

static READ32_HANDLER( inp1_r )
{
	UINT32 result = (readinputport(5)<<24) | (readinputport(2)<<16) | (readinputport(1)<<8) | readinputport(0);

	return result;
}

static READ32_HANDLER( inp2_r )
{
	return (readinputport(3)<<24) | (readinputport(4)<<16) | 0xffff;
}

static READ32_HANDLER( turntable_r )
{
	UINT32 result = 0;

	if (!(mem_mask & 0x0000ff00))
	{
		UINT8 pos;
		int delta;

		pos = readinputport(6 + turntable_select);
		delta = pos - turntable_last_pos[turntable_select];
		if (delta < -128)
			delta += 256;
		if (delta > 128)
			delta -= 256;

		turntable_pos[turntable_select] += delta * 70;
		turntable_last_pos[turntable_select] = pos;

		result |= turntable_pos[turntable_select] & 0xff00;
	}

	return result;
}

static WRITE32_HANDLER( turntable_select_w )
{
	if (!(mem_mask & 0x00ff0000))
		turntable_select = (data >> 19) & 1;
}


//---------

#define IDE_STD_OFFSET	(0x1f0/2)
#define IDE_ALT_OFFSET	(0x3f6/2)

static READ32_HANDLER( ide_std_r )
{
	if (ACCESSING_LSB32)
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x00ff) >> 8;
	else
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x0000) << 16;
}

static WRITE32_HANDLER( ide_std_w )
{
	if (ACCESSING_LSB32)
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data << 8, 0x00ff);
	else
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data >> 16, 0x0000);
}


static READ32_HANDLER( ide_alt_r )
{
	if (offset == 0)
		return ide_controller16_0_r(IDE_ALT_OFFSET, 0xff00) << 24;

	return 0;
}

static WRITE32_HANDLER( ide_alt_w )
{
	if (offset == 0 && !(mem_mask & 0x00ff0000))
		ide_controller16_0_w(IDE_ALT_OFFSET, data >> 24, 0xff00);
}


//---------

// light/coin blocker control

/*
 beatmania/hiphopmania
    0x5d0000 (MSW16):
    bit 0: 1P button 1 LED
        1: 1P button 2 LED
        2: 1P button 3 LED
        3: 1P button 4 LED
        4: 1P button 5 LED
        5: Right blue HIGHLIGHT (active low)
        6: 2P button 1 LED
        7: 2P button 2 LED
        8: 2P button 3 LED
        9: Left blue HIGHLIGHT  (active low)
       10: Left red HIGHLIGHT   (active low)
       11: Right red HIGHLIGHT  (active low)
    12-15: not used?        (always low)

    0x5d2000 (MSW16):
        0: 1P START button LED
        1: 2P START button LED
        2: EFFECT button LED
     3-10: not used?        (always low)
       11: SSR
       12: 2P button 4 LED
       13: 2P button 5 LED
       14: COIN BLOCKER     (active low)
       15: not used?        (always low)
*/

static WRITE32_HANDLER( light_ctrl_1_w )
{
	if (ACCESSING_MSW32)
	{
		artwork_show("right-red-hlt",  !(data & 0x08000000));	// Right red HIGHLIGHT
		artwork_show("left-red-hlt",   !(data & 0x04000000));	// Left red HIGHLIGHT
		artwork_show("left-blue-hlt",  !(data & 0x02000000));	// Left blue HIGHLIGHT
		artwork_show("right-blue-hlt", !(data & 0x00200000));	// Right blue HIGHLIGHT
	}
}

static WRITE32_HANDLER( light_ctrl_2_w )
{
	if (ACCESSING_MSW32)
	{
		artwork_show("left-ssr",       data & 0x08000000);	// SSR
		artwork_show("right-ssr",      data & 0x08000000);	// SSR
		set_led_status(0, data & 0x00010000);			// 1P START
		set_led_status(1, data & 0x00020000);			// 2P START
		set_led_status(2, data & 0x00040000);			// EFFECT
	}
}


//---------

// unknown ports :-(

static WRITE32_HANDLER( unknown590000_w )
{
	//logerror("%08X: unknown 590000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknown802000_w )
{
	//logerror("%08X: unknown 802000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknownc02000_w )
{
	//logerror("%08X: unknown c02000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Interrupt handlers
 *
 *************************************/

static INTERRUPT_GEN( vb_interrupt )
{
	pending_vb_int = 0;

	if (DISABLE_VB_INT)
	{
		pending_vb_int = 1;
		return;
	}

	//logerror("V-Blank interrupt\n");
	cpunum_set_input_line(0, MC68000_IRQ_4, HOLD_LINE);
}


static void ide_interrupt(int state)
{
	if (state != CLEAR_LINE)
	{
		//logerror("IDE interrupt asserted\n");
		cpunum_set_input_line(0, MC68000_IRQ_1, HOLD_LINE);
	}
	else
	{
		//logerror("IDE interrupt cleared\n");
		cpunum_set_input_line(0, MC68000_IRQ_1, CLEAR_LINE);
	}
}




/*************************************
 *
 *  Memory definitions
 *
 *************************************/

static ADDRESS_MAP_START( memory_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM							// PRG ROM
	AM_RANGE(0x400000, 0x40ffff) AM_RAM							// WORK RAM
	AM_RANGE(0x480000, 0x48443f) AM_READWRITE(paletteram32_r, paletteram32_w)		// COLOR RAM
	                             AM_BASE(&paletteram32)
	AM_RANGE(0x500000, 0x57ffff) AM_READWRITE(sndram_r, sndram_w)				// SOUND RAM
	AM_RANGE(0x580000, 0x58003f) AM_READWRITE(K056832_long_r, K056832_long_w)		// VIDEO REG (tilemap)
	AM_RANGE(0x590000, 0x590007) AM_WRITE(unknown590000_w)					// ??
	AM_RANGE(0x5a0000, 0x5a005f) AM_WRITE(K055555_long_w)					// 055555: priority encoder
	AM_RANGE(0x5b0000, 0x5b04ff) AM_READWRITE(dual539_r, dual539_w)				// SOUND regs
	AM_RANGE(0x5c0000, 0x5c0003) AM_READ(inp1_r)						// input port (buttons)
	AM_RANGE(0x5c8000, 0x5c8003) AM_READ(inp2_r)						// input port (buttons)
	AM_RANGE(0x5d0000, 0x5d0003) AM_WRITE(light_ctrl_1_w)					// light/coin blocker control
	AM_RANGE(0x5d2000, 0x5d2003) AM_WRITE(light_ctrl_2_w)					// light/coin blocker control
	AM_RANGE(0x5d4000, 0x5d4003) AM_WRITE(v_ctrl_w)						// VIDEO control
	AM_RANGE(0x5d6000, 0x5d6003) AM_WRITE(sndram_bank_w)					// SOUND RAM bank
	AM_RANGE(0x5e0000, 0x5e0003) AM_READWRITE(turntable_r, turntable_select_w)		// input port control (turn tables)
	AM_RANGE(0x600000, 0x601fff) AM_READ(v_rom_r)						// VIDEO ROM readthrough (for POST)
	AM_RANGE(0x801000, 0x8017ff) AM_RAM AM_BASE(&djmain_obj_ram)				// OBJECT RAM
	AM_RANGE(0x802000, 0x802fff) AM_WRITE(unknown802000_w)					// ??
	AM_RANGE(0x803000, 0x80309f) AM_READWRITE(obj_ctrl_r, obj_ctrl_w)			// OBJECT REGS
	AM_RANGE(0x803800, 0x803fff) AM_READ(obj_rom_r)						// OBJECT ROM readthrough (for POST)
	AM_RANGE(0xc00000, 0xc01fff) AM_READWRITE(K056832_ram_long_r, K056832_ram_long_w)	// VIDEO RAM (tilemap) (beatmania)
	AM_RANGE(0xc02000, 0xc02047) AM_WRITE(unknownc02000_w)					// ??
	AM_RANGE(0xd00000, 0xd0000f) AM_READWRITE(ide_std_r, ide_std_w)				// IDE control regs (hiphopmania)
	AM_RANGE(0xd4000c, 0xd4000f) AM_READWRITE(ide_alt_r, ide_alt_w)				// IDE status control reg (hiphopmania)
	AM_RANGE(0xe00000, 0xe01fff) AM_READWRITE(K056832_ram_long_r, K056832_ram_long_w)	// VIDEO RAM (tilemap) (hiphopmania)
	AM_RANGE(0xf00000, 0xf0000f) AM_READWRITE(ide_std_r, ide_std_w)				// IDE control regs (beatmania)
	AM_RANGE(0xf4000c, 0xf4000f) AM_READWRITE(ide_alt_r, ide_alt_w)				// IDE status control reg (beatmania)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

#define BEATMANIA_INPUT \
	PORT_START      /* IN 0 */ \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) \
 \
	PORT_START      /* IN 1 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_START3 ) PORT_NAME("Effect")	/* EFFECT */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) \
 \
	PORT_START      /* IN 2 */ \
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)	/* TEST SW */ \
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service")	/* SERVICE */ \
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Reset")		/* RESET SW */ \
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define BEATMANIA_TURNTABLE \
	PORT_START      /* IN 6: turn table 1P */ \
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1) \
 \
	PORT_START      /* IN 7: turn table 2P */ \
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

#define BEATMANIA_DSW1(base,mask) \
	PORT_START      /* IN 3 */ \
	PORT_DIPNAME( 0xff, (0xff & mask) | base, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    (0xe0 & mask) | base, "1P 8C / 2P 16C / Continue 8C" ) \
	PORT_DIPSETTING(    (0xe1 & mask) | base, "1P 8C / 2P 16C / Continue 7C" ) \
	PORT_DIPSETTING(    (0xe2 & mask) | base, "1P 8C / 2P 16C / Continue 6C" ) \
	PORT_DIPSETTING(    (0xe3 & mask) | base, "1P 7C / 2P 14C / Continue 7C" ) \
	PORT_DIPSETTING(    (0xe4 & mask) | base, "1P 7C / 2P 14C / Continue 6C" ) \
	PORT_DIPSETTING(    (0xe5 & mask) | base, "1P 7C / 2P 14C / Continue 5C" ) \
	PORT_DIPSETTING(    (0xe6 & mask) | base, "1P 6C / 2P 12C / Continue 6C" ) \
	PORT_DIPSETTING(    (0xe7 & mask) | base, "1P 6C / 2P 12C / Continue 5C" ) \
	PORT_DIPSETTING(    (0xe8 & mask) | base, "1P 6C / 2P 12C / Continue 4C" ) \
	PORT_DIPSETTING(    (0xe9 & mask) | base, "1P 5C / 2P 10C / Continue 5C" ) \
	PORT_DIPSETTING(    (0xeb & mask) | base, "1P 5C / 2P 10C / Continue 3C" ) \
	PORT_DIPSETTING(    (0xea & mask) | base, "1P 5C / 2P 10C / Continue 4C" ) \
	PORT_DIPSETTING(    (0xec & mask) | base, "1P 4C / 2P 8C / Continue 4C" ) \
	PORT_DIPSETTING(    (0xed & mask) | base, "1P 4C / 2P 8C / Continue 3C" ) \
	PORT_DIPSETTING(    (0xee & mask) | base, "1P 4C / 2P 8C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xef & mask) | base, "1P 3C / 2P 6C / Continue 3C" ) \
	PORT_DIPSETTING(    (0xf0 & mask) | base, "1P 3C / 2P 6C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xf1 & mask) | base, "1P 3C / 2P 6C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xf2 & mask) | base, "1P 3C / 2P 4C / Continue 3C" ) \
	PORT_DIPSETTING(    (0xf3 & mask) | base, "1P 3C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xf4 & mask) | base, "1P 3C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xf5 & mask) | base, "1P 3C / 2P 3C / Continue 3C" ) \
	PORT_DIPSETTING(    (0xf6 & mask) | base, "1P 3C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xf7 & mask) | base, "1P 3C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xfa & mask) | base, "1P 2C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xfb & mask) | base, "1P 2C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xf8 & mask) | base, "1P 2C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xff & mask) | base, "1P 2C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xfc & mask) | base, "1P 2C / 2P 2C / Continue 2C" ) \
	PORT_DIPSETTING(    (0xfd & mask) | base, "1P 2C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xfe & mask) | base, "1P 1C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    (0xf9 & mask) | base, "1P 1C / 2P 1C / Continue 1C" ) \
	PORT_DIPSETTING(    (0x00 & mask) | base, DEF_STR( Free_Play ) )

#define BM1STMIX_DSW1 \
	PORT_START      /* IN 3 */ \
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x1f, 0x1f, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x1e, "1P 3C / 2P 6C / Continue 3C" ) \
	PORT_DIPSETTING(    0x01, "1P 3C / 2P 6C / Continue 2C" ) \
	PORT_DIPSETTING(    0x11, "1P 3C / 2P 6C / Continue 1C" ) \
	PORT_DIPSETTING(    0x15, "1P 3C / 2P 3C / Continue 3C" ) \
	PORT_DIPSETTING(    0x0d, "1P 3C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0x1d, "1P 3C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0x09, "1P 3C / 2P 4C / Continue 3C" ) \
	PORT_DIPSETTING(    0x19, "1P 3C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0x05, "1P 3C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0x03, "1P 2C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0x1f, "1P 2C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0x0b, "1P 2C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0x1b, "1P 2C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0x07, "1P 2C / 2P 2C / Continue 2C" ) \
	PORT_DIPSETTING(    0x17, "1P 2C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0x0f, "1P 1C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0x13, "1P 1C / 2P 1C / Continue 1C" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

#define BEATMANIA_DSW2 \
	PORT_START      /* IN 4 */ \
	PORT_DIPNAME( 0x80, 0x80, "Score Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Low ) ) \
	PORT_DIPSETTING(    0x00, "Silent" ) \
	PORT_DIPNAME( 0x10, 0x10, "Level Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x0c, 0x0c, "Normal Difficulty" ) \
	PORT_DIPSETTING(    0x08, "Level 0" ) \
	PORT_DIPSETTING(    0x0c, "Level 1" ) \
	PORT_DIPSETTING(    0x04, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" ) \
	PORT_DIPNAME( 0x03, 0x03, "Expert Difficulty" ) \
	PORT_DIPSETTING(    0x02, "Level 0" ) \
	PORT_DIPSETTING(    0x03, "Level 1" ) \
	PORT_DIPSETTING(    0x01, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" )

#define BEATMANIA_DSW2_OLD_LEVEL \
	PORT_START      /* IN 4 */ \
	PORT_DIPNAME( 0x80, 0x80, "Score Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Low ) ) \
	PORT_DIPSETTING(    0x00, "Silent" ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 2-4 */ \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x0a, "Level 0" ) \
	PORT_DIPSETTING(    0x0e, "Level 1" ) \
	PORT_DIPSETTING(    0x0d, "Level 2" ) \
	PORT_DIPSETTING(    0x0c, "Level 3" ) \
	PORT_DIPSETTING(    0x0b, "Level 4" ) \
	PORT_DIPSETTING(    0x0f, "Level 5" ) \
	PORT_DIPSETTING(    0x09, "Level 6" ) \
	PORT_DIPSETTING(    0x08, "Level 7" ) \
	PORT_DIPSETTING(    0x07, "Level 8" ) \
	PORT_DIPSETTING(    0x06, "Level 9" ) \
	PORT_DIPSETTING(    0x05, "Level 10" ) \
	PORT_DIPSETTING(    0x04, "Level 11" ) \
	PORT_DIPSETTING(    0x03, "Level 12" ) \
	PORT_DIPSETTING(    0x02, "Level 13" ) \
	PORT_DIPSETTING(    0x01, "Level 14" ) \
	PORT_DIPSETTING(    0x00, "Level 15" )

#define BM1STMIX_DSW2 \
	PORT_START      /* IN 4 */ \
	PORT_DIPNAME( 0x80, 0x80, "Enable Expert Mode" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( No ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Low ) ) \
	PORT_DIPSETTING(    0x00, "Silent" ) \
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN ) \

#define BMCOMPMX_DSW2 \
	PORT_START      /* IN 4 */ \
	PORT_DIPNAME( 0x80, 0x80, "Score Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Low ) ) \
	PORT_DIPSETTING(    0x00, "Silent" ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )		/* DSW 2-4 */ \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x0a, "Level 0" ) \
	PORT_DIPSETTING(    0x0e, "Level 1" ) \
	PORT_DIPSETTING(    0x0d, "Level 2" ) \
	PORT_DIPSETTING(    0x0c, "Level 3" ) \
	PORT_DIPSETTING(    0x0b, "Level 4" ) \
	PORT_DIPSETTING(    0x0f, "Level 5" ) \
	PORT_DIPSETTING(    0x09, "Level 6" ) \
	PORT_DIPSETTING(    0x08, "Level 7" ) \
	PORT_DIPSETTING(    0x07, "Level 8" ) \
	PORT_DIPSETTING(    0x06, "Level 9" ) \
	PORT_DIPSETTING(    0x05, "Level 10" ) \
	PORT_DIPSETTING(    0x04, "Level 11" ) \
	PORT_DIPSETTING(    0x03, "Level 12" ) \
	PORT_DIPSETTING(    0x02, "Level 13" ) \
	PORT_DIPSETTING(    0x01, "Level 14" ) \
	PORT_DIPSETTING(    0x00, "Level 15" )

#define BEATMANIA_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Normal / Event Mode Stages" ) \
	PORT_DIPSETTING(    0x0c, "4 / 1" ) \
	PORT_DIPSETTING(    0x14, "4 / 2" ) \
	PORT_DIPSETTING(    0x10, "3 / 3" ) \
	PORT_DIPSETTING(    0x1c, "4 / 4" ) \
	PORT_DIPSETTING(    0x08, "5 / 5" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-5 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-6 */

#define BM1STMIX_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define BM2NDMIX_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x39, 0x39, "Event Mode / Free Hidden Songs" ) \
	PORT_DIPSETTING(    0x39, "Off / Off" ) \
	PORT_DIPSETTING(    0x20, "Off / On" ) \
	PORT_DIPSETTING(    0x19, "1 Stages / On" ) \
	PORT_DIPSETTING(    0x09, "2 Stages / On" ) \
	PORT_DIPSETTING(    0x11, "3 Stages / On" ) \
	PORT_DIPSETTING(    0x01, "4 Stages / On" ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-4 */ \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-5 */ \

#define BMCOMPMX_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Normal / Event Mode Stages" ) \
	PORT_DIPSETTING(    0x0c, "4 / 1" ) \
	PORT_DIPSETTING(    0x14, "4 / 2" ) \
	PORT_DIPSETTING(    0x10, "3 / 3" ) \
	PORT_DIPSETTING(    0x1c, "4 / 4" ) \
	PORT_DIPSETTING(    0x08, "5 / 5" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-5 */ \
	PORT_DIPNAME( 0x01, 0x01, "Secret Expert Course" )	/* DSW 3-6 */ \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define BM4THMIX_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Normal / Event Mode Stages" ) \
	PORT_DIPSETTING(    0x0c, "4 / 1" ) \
	PORT_DIPSETTING(    0x14, "4 / 2" ) \
	PORT_DIPSETTING(    0x10, "3 / 3" ) \
	PORT_DIPSETTING(    0x1c, "4 / 4" ) \
	PORT_DIPSETTING(    0x08, "5 / 5" ) \
	PORT_DIPNAME( 0x02, 0x02, "Secret Expert Course" )	/* DSW 3-5 */ \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-6 */

#define HMCOMPM2_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Normal / Event Mode Stages" ) \
	PORT_DIPSETTING(    0x0c, "4 / 1" ) \
	PORT_DIPSETTING(    0x14, "4 / 2" ) \
	PORT_DIPSETTING(    0x10, "3 / 3" ) \
	PORT_DIPSETTING(    0x1c, "4 / 4" ) \
	PORT_DIPSETTING(    0x08, "5 / 5" ) \
	PORT_DIPNAME( 0x02, 0x02, "Game Over Mode" ) \
	PORT_DIPSETTING(    0x02, "On Stage Middle" ) \
	PORT_DIPSETTING(    0x00, "On Stage Last" ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-6 */

#define BMDCT_DSW3 \
	PORT_START      /* IN 5 */ \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Normal / Event Mode Stages" ) \
	PORT_DIPSETTING(    0x0c, "3 / 1" ) \
	PORT_DIPSETTING(    0x14, "3 / 2" ) \
	PORT_DIPSETTING(    0x10, "4 / 3" ) \
	PORT_DIPSETTING(    0x1c, "3 / 4" ) \
	PORT_DIPSETTING(    0x08, "3 / 5" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-5 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* DSW 3-6 */

INPUT_PORTS_START( beatmania )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x00, 0xff)	/* IN 3 */
	BEATMANIA_DSW2			/* IN 4 */
	BEATMANIA_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( bm1stmix )
	BEATMANIA_INPUT			/* IN 0-2 */
	BM1STMIX_DSW1			/* IN 3 */
	BM1STMIX_DSW2			/* IN 4 */
	BM1STMIX_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( bm2ndmix )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x00, 0xff)	/* IN 3 */
	BEATMANIA_DSW2_OLD_LEVEL	/* IN 4 */
	BM2NDMIX_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( bmcompmx )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x80, 0x3f)	/* IN 3 */
	BMCOMPMX_DSW2			/* IN 4 */
	BMCOMPMX_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( bm4thmix )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x40, 0x3f)	/* IN 3 */
	BEATMANIA_DSW2			/* IN 4 */
	BM4THMIX_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( hmcompm2 )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x00, 0xff)	/* IN 3 */
	BEATMANIA_DSW2			/* IN 4 */
	HMCOMPM2_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END

INPUT_PORTS_START( bmdct )
	BEATMANIA_INPUT			/* IN 0-2 */
	BEATMANIA_DSW1(0x00, 0xff)	/* IN 3 */
	BEATMANIA_DSW2			/* IN 4 */
	BMDCT_DSW3			/* IN 5 */
	BEATMANIA_TURNTABLE		/* IN 6-7 */
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout spritelayout =
{
	16, 16,	/* 16x16 characters */
	0x200000 / 128,	/* 16384 characters */
	4,	/* bit planes */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
	  4+256, 0+256, 12+256, 8+256, 20+256, 16+256, 28+256, 24+256 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  0*32+512, 1*32+512, 2*32+512, 3*32+512, 4*32+512, 5*32+512, 6*32+512, 7*32+512 },
	16*16*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,  (0x4440/4)/16 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *  IDE interfaces
 *
 *************************************/

static struct ide_interface ide_intf =
{
	ide_interrupt,
};



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct K054539interface k054539_interface =
{
	REGION_SOUND1
};



/*************************************
 *
 *  Machine-specific init
 *
 *************************************/

static MACHINE_START( djmain )
{
	UINT8 *region = memory_region(REGION_SOUND1);

	state_save_register_global(sndram_bank);
	state_save_register_global_pointer(region, 0x80000 * 32);
	state_save_register_global(pending_vb_int);
	state_save_register_global(v_ctrl);
	state_save_register_global_array(obj_regs);

	state_save_register_func_postload(sndram_set_bank);
	return 0;
}


static MACHINE_RESET( djmain )
{
	/* reset sound ram bank */
	sndram_bank = 0;
	sndram_set_bank();

	/* reset the IDE controller */
	ide_controller_reset(0);

	/* reset LEDs */
	set_led_status(0, 1);
	set_led_status(1, 1);
	set_led_status(2, 1);
}



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( djmain )

	/* basic machine hardware */
	// popn3 works 9.6 MHz or slower in some songs */
	//MDRV_CPU_ADD(M68EC020, 18432000/2)    /*  9.216 MHz!? */
	MDRV_CPU_ADD(M68EC020, 32000000/4)	/*  8.000 MHz!? */
	MDRV_CPU_PROGRAM_MAP(memory_map, 0)
	MDRV_CPU_VBLANK_INT(vb_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(djmain)
	MDRV_MACHINE_RESET(djmain)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(12, 512-12-1, 0, 384-1)
	MDRV_PALETTE_LENGTH(0x4440/4)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_START(djmain)
	MDRV_VIDEO_UPDATE(djmain)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( bm1stmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "753jab01.6a", 0x000000, 0x80000, CRC(25BF8629) SHA1(2be73f9dd25cae415c6443f221cc7d38d5555ae5) )
	ROM_LOAD16_BYTE( "753jab02.8a", 0x000001, 0x80000, CRC(6AB951DE) SHA1(a724ede03b74e9422c120fcc263e2ebcc3a3e110) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "753jaa03.19a", 0x000000, 0x80000, CRC(F2B2BCE8) SHA1(61d31b111f35e7dde89965fa43ba627c12aff11c) )
	ROM_LOAD16_BYTE( "753jaa04.20a", 0x000001, 0x80000, CRC(85A18F9D) SHA1(ecd0ab4f53e882b00176dacad5fac35345fbea66) )
	ROM_LOAD16_BYTE( "753jaa05.22a", 0x100000, 0x80000, CRC(749B1E87) SHA1(1c771c19f152ae95171e4fd51da561ba4ec5ea87) )
	ROM_LOAD16_BYTE( "753jaa06.24a", 0x100001, 0x80000, CRC(6D86B0FD) SHA1(74a255dbb1c83131717ea1fe335f12aef81d9fcc) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "753jaa07.22d", 0x000000, 0x80000, CRC(F03AB5D8) SHA1(2ad902547908208714855aa0f2b7ed493452ee5f) )
	ROM_LOAD16_BYTE( "753jaa08.23d", 0x000001, 0x80000, CRC(6559F0C8) SHA1(0d6ec4bdc22c02cb9fb8de36b0a8f7a6c983440e) )
	ROM_LOAD16_BYTE( "753jaa09.25d", 0x100000, 0x80000, CRC(B50C3DBB) SHA1(6022ea249aad0793b2279699e68087b4bc9b4ef1) )
	ROM_LOAD16_BYTE( "753jaa10.27d", 0x100001, 0x80000, CRC(391F4BFD) SHA1(791c9889ea3ce639bbfb87934a1cad9aa3c9ccde) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	// There is an alternate image
	//DISK_IMAGE( "753jaa11", 0, MD5(260c9b72f4a03055e3abad61c6225324) SHA1(2cc3e149744516bf2353a2b47d33bc9d2072b6c4) ) /* ver 1.00 JA */
	DISK_IMAGE( "753jaa11", 0, MD5(d56ec7b9877d1f26d7fc1cabed404947) SHA1(71d200d1bd3f1f3a01f4daa78dc9abcca8b8a1fb) )	/* ver 1.00 JA */
ROM_END

ROM_START( bm2ndmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "853jab01.6a", 0x000000, 0x80000, CRC(C8DF72C0) SHA1(6793b587ba0611bc3da8c4955d6a87e47a19a223) )
	ROM_LOAD16_BYTE( "853jab02.8a", 0x000001, 0x80000, CRC(BF6ACE08) SHA1(29d3fdf1c73a73a0a66fa5a4c4ac3f293cb82e37) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "853jaa03.19a", 0x000000, 0x80000, CRC(1462ED23) SHA1(fdfda3060c8d367ac2e8e43dedaba8ab9012cc77) )
	ROM_LOAD16_BYTE( "853jaa04.20a", 0x000001, 0x80000, CRC(98C9B331) SHA1(51f24b3c3773c53ff492ed9bad17c9867fd94e28) )
	ROM_LOAD16_BYTE( "853jaa05.22a", 0x100000, 0x80000, CRC(0DA3FEF9) SHA1(f9ef24144c00c054ecc4650bb79e74c57c6d6b3c) )
	ROM_LOAD16_BYTE( "853jaa06.24a", 0x100001, 0x80000, CRC(6A66978C) SHA1(460178a6f35e554a157742d77ed5ea6989fbcee1) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "853jaa07.22d", 0x000000, 0x80000, CRC(728C0010) SHA1(18888b402e0b7ccf63c7b3cb644673df1746dba7) )
	ROM_LOAD16_BYTE( "853jaa08.23d", 0x000001, 0x80000, CRC(926FC37C) SHA1(f251cba56ca201f0e748112462116cff218b66da) )
	ROM_LOAD16_BYTE( "853jaa09.25d", 0x100000, 0x80000, CRC(8584E21E) SHA1(3d1ca6de00f9ac07bbe7cd1e67093cca7bf484bb) )
	ROM_LOAD16_BYTE( "853jaa10.27d", 0x100001, 0x80000, CRC(9CB92D98) SHA1(6ace4492ba0b5a8f94a9e7b4f7126b31c6254637) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "853jaa11", 0, MD5(37281741b748bea7dfa711a956649d1e) SHA1(03d6cc5aea5920163fbaba34c4f838ca605a87e3) )	/* ver 1.10 JA */
ROM_END

ROM_START( bm2ndmxa )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "853jaa01.6a", 0x000000, 0x80000, CRC(4F0BF5D0) SHA1(4793bb411e85f2191eb703a170c16cf163ea79e7) )
	ROM_LOAD16_BYTE( "853jaa02.8a", 0x000001, 0x80000, CRC(E323925B) SHA1(1f9f52a7ab6359b617e87f8b3d7ac4269885c621) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "853jaa03.19a", 0x000000, 0x80000, CRC(1462ED23) SHA1(fdfda3060c8d367ac2e8e43dedaba8ab9012cc77) )
	ROM_LOAD16_BYTE( "853jaa04.20a", 0x000001, 0x80000, CRC(98C9B331) SHA1(51f24b3c3773c53ff492ed9bad17c9867fd94e28) )
	ROM_LOAD16_BYTE( "853jaa05.22a", 0x100000, 0x80000, CRC(0DA3FEF9) SHA1(f9ef24144c00c054ecc4650bb79e74c57c6d6b3c) )
	ROM_LOAD16_BYTE( "853jaa06.24a", 0x100001, 0x80000, CRC(6A66978C) SHA1(460178a6f35e554a157742d77ed5ea6989fbcee1) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "853jaa07.22d", 0x000000, 0x80000, CRC(728C0010) SHA1(18888b402e0b7ccf63c7b3cb644673df1746dba7) )
	ROM_LOAD16_BYTE( "853jaa08.23d", 0x000001, 0x80000, CRC(926FC37C) SHA1(f251cba56ca201f0e748112462116cff218b66da) )
	ROM_LOAD16_BYTE( "853jaa09.25d", 0x100000, 0x80000, CRC(8584E21E) SHA1(3d1ca6de00f9ac07bbe7cd1e67093cca7bf484bb) )
	ROM_LOAD16_BYTE( "853jaa10.27d", 0x100001, 0x80000, CRC(9CB92D98) SHA1(6ace4492ba0b5a8f94a9e7b4f7126b31c6254637) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "853jaa11", 0, MD5(37281741b748bea7dfa711a956649d1e) SHA1(03d6cc5aea5920163fbaba34c4f838ca605a87e3) )	/* ver 1.10 JA */
ROM_END

ROM_START( bmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858jab01.6a", 0x000000, 0x80000, CRC(92841EB5) SHA1(3a9d90a9c4b16cb7118aed2cadd3ab32919efa96) )
	ROM_LOAD16_BYTE( "858jab02.8a", 0x000001, 0x80000, CRC(7B19969C) SHA1(3545acabbf53bacc5afa72a3c5af3cd648bc2ae1) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "858jaa03.19a", 0x000000, 0x80000, CRC(8559F457) SHA1(133092994087864a6c29e9d51dcdbef2e2c2a123) )
	ROM_LOAD16_BYTE( "858jaa04.20a", 0x000001, 0x80000, CRC(770824D3) SHA1(5c21bc39f8128957d76be85bc178c96976987f5f) )
	ROM_LOAD16_BYTE( "858jaa05.22a", 0x100000, 0x80000, CRC(9CE769DA) SHA1(1fe2999f786effdd5e3e74475e8431393eb9403d) )
	ROM_LOAD16_BYTE( "858jaa06.24a", 0x100001, 0x80000, CRC(0CDE6584) SHA1(fb58d2b4f58144b71703431740c0381bb583f581) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858jaa07.22d", 0x000000, 0x80000, CRC(7D183F46) SHA1(7a1b0ccb0407b787af709bdf038d886727199e4e) )
	ROM_LOAD16_BYTE( "858jaa08.23d", 0x000001, 0x80000, CRC(C731DC8F) SHA1(1a937d76c02711b7f73743c9999456d4408ad284) )
	ROM_LOAD16_BYTE( "858jaa09.25d", 0x100000, 0x80000, CRC(0B4AD843) SHA1(c01e15053dd1975dc68db9f4e6da47062d8f9b54) )
	ROM_LOAD16_BYTE( "858jaa10.27d", 0x100001, 0x80000, CRC(00B124EE) SHA1(435d28a327c2707833a8ddfe841104df65ffa3f8) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11", 0, MD5(6c573b432c5d8b5a4fb857b0fab26cb8) SHA1(05a3f2359d089f47dcaa28ee7bc65a23efecda94) )	/* ver 1.00 JA */
ROM_END

ROM_START( hmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858uab01.6a", 0x000000, 0x80000, CRC(F9C16675) SHA1(f2b50a3544f43af6fd987256a8bd4125b95749ef) )
	ROM_LOAD16_BYTE( "858uab02.8a", 0x000001, 0x80000, CRC(4E8F1E78) SHA1(88d654de4377b584ff8a5e1f8bc81ffb293ec8a5) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "858uaa03.19a", 0x000000, 0x80000, CRC(52B51A5E) SHA1(9f01e2fcbe5a9d7f80b377c5e10f18da2c9dcc8e) )
	ROM_LOAD16_BYTE( "858uaa04.20a", 0x000001, 0x80000, CRC(A336CEE9) SHA1(0e62c0c38d86868c909b4c1790fbb7ecb2de137d) )
	ROM_LOAD16_BYTE( "858uaa05.22a", 0x100000, 0x80000, CRC(2E14CF83) SHA1(799b2162f7b11678d1d260f7e1eb841abda55a60) )
	ROM_LOAD16_BYTE( "858uaa06.24a", 0x100001, 0x80000, CRC(2BE07788) SHA1(5cc2408f907ca6156efdcbb2c10a30e9b81797f8) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858uaa07.22d", 0x000000, 0x80000, CRC(9D7C8EA0) SHA1(5ef773ade7ab12a5dc10484e8b7711c9d76fe2a1) )
	ROM_LOAD16_BYTE( "858uaa08.23d", 0x000001, 0x80000, CRC(F21C3F45) SHA1(1d7ff2c4161605b382d07900142093192aa93a48) )
	ROM_LOAD16_BYTE( "858uaa09.25d", 0x100000, 0x80000, CRC(99519886) SHA1(664f6bd953201a6e2fc123cb8b3facf72766107d) )
	ROM_LOAD16_BYTE( "858uaa10.27d", 0x100001, 0x80000, CRC(20AA7145) SHA1(eeff87eb9a9864985d751f45e843ee6e73db8cfd) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11", 0, MD5(6c573b432c5d8b5a4fb857b0fab26cb8) SHA1(05a3f2359d089f47dcaa28ee7bc65a23efecda94) )	/* ver 1.00 JA */
ROM_END

ROM_START( bm4thmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "847jaa01.6a", 0x000000, 0x80000, CRC(81138A1B) SHA1(ebe211126f871e541881e1670f56d50b058dead3) )
	ROM_LOAD16_BYTE( "847jaa02.8a", 0x000001, 0x80000, CRC(4EEB0010) SHA1(942303dfb19a4a78dd74ad24576031760553a661) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "847jaa03.19a", 0x000000, 0x80000, CRC(F447D140) SHA1(cc15b80419940d127a77765508f877421ed86ee2) )
	ROM_LOAD16_BYTE( "847jaa04.20a", 0x000001, 0x80000, CRC(EDC3E286) SHA1(341b1dc6ee1562b1ddf235a66ac96b94c482b67c) )
	ROM_LOAD16_BYTE( "847jaa05.22a", 0x100000, 0x80000, CRC(DA165B5E) SHA1(e46110590e6ab89b55f6abfbf6c53c99d28a75a9) )
	ROM_LOAD16_BYTE( "847jaa06.24a", 0x100001, 0x80000, CRC(8BFC2F28) SHA1(f8869867945d63d9f34b6228d95c5a61b193eed2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "847jab07.22d", 0x000000, 0x80000, CRC(C159E7C4) SHA1(96af0c29b2f1fef494b2223179862d16f26bb33f) )
	ROM_LOAD16_BYTE( "847jab08.23d", 0x000001, 0x80000, CRC(8FF084D6) SHA1(50cff8c701e33f2630925c1a9ae4351076912acd) )
	ROM_LOAD16_BYTE( "847jab09.25d", 0x100000, 0x80000, CRC(2E4AC9FE) SHA1(bbd4c6e0c82fc0be88f851e901e5853b6bcf775f) )
	ROM_LOAD16_BYTE( "847jab10.27d", 0x100001, 0x80000, CRC(C78516F5) SHA1(1adf5805c808dc55de14a9a9b20c3d2cf7bf414d) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "847jaa11", 0, MD5(47cb5c1b856aa11cf38f0c7ea4a7d1c3) SHA1(374d5d5340d4a8818577f9ae81021651d6ee3429) )	/* ver 1.00 JA */
ROM_END

ROM_START( bm5thmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "981jaa01.6a", 0x000000, 0x80000, CRC(03BBE7E3) SHA1(7d4ec3bc7719a3f1b81df309b5c74afaffde42ba) )
	ROM_LOAD16_BYTE( "981jaa02.8a", 0x000001, 0x80000, CRC(F4E59923) SHA1(a4983435e3f2243ea9ccc2fd5439d86c30b6f604) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "981jaa03.19a", 0x000000, 0x80000, CRC(8B7E6D72) SHA1(d470377e20e4d4935af5e57d081ce24dd9ea5793) )
	ROM_LOAD16_BYTE( "981jaa04.20a", 0x000001, 0x80000, CRC(5139988A) SHA1(2b1eb97dcbfbe6bba1352a02cf0036e9a721ab39) )
	ROM_LOAD16_BYTE( "981jaa05.22a", 0x100000, 0x80000, CRC(F370FDB9) SHA1(3a2bbdda984f2630e8ae505a8db259d9162e07a3) )
	ROM_LOAD16_BYTE( "981jaa06.24a", 0x100001, 0x80000, CRC(DA6E3813) SHA1(9163bd2cfb0a32798e797c7b4eea21e28772a206) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "981jaa07.22d", 0x000000, 0x80000, CRC(F6C72998) SHA1(e78af5b515b224c534f47abd6477dd97dc521b0d) )
	ROM_LOAD16_BYTE( "981jaa08.23d", 0x000001, 0x80000, CRC(AA4FF682) SHA1(3750e1e81b7c1a4fb419076171f20e4c36b1c544) )
	ROM_LOAD16_BYTE( "981jaa09.25d", 0x100000, 0x80000, CRC(D96D4E1C) SHA1(379aa4e82cd06490645f54dab1724c827108735d) )
	ROM_LOAD16_BYTE( "981jaa10.27d", 0x100001, 0x80000, CRC(06BEE0E4) SHA1(6eea8614cb01e7079393b9976b6fd6a52c14e3c0) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "981jaa11", 0, MD5(0058bbdcb5db054adff1c4148ef4211e) SHA1(bee71c31978ac186e5e933adcf0220a70496ca6c) )	/* ver 1.00 JA */
ROM_END

ROM_START( bmcompm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "988jaa01.6a", 0x000000, 0x80000, CRC(31BE1D4C) SHA1(ab8c2b4a2b48e3b2b549022f65afb206ab125680) )
	ROM_LOAD16_BYTE( "988jaa02.8a", 0x000001, 0x80000, CRC(0413DE32) SHA1(f819e8756e2000de5df61ad42ac01de14b7330f9) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "988jaa03.19a", 0x000000, 0x80000, CRC(C0AD86D4) SHA1(6aca5bf3fbc0bd69116e442053840660eeff0239) )
	ROM_LOAD16_BYTE( "988jaa04.20a", 0x000001, 0x80000, CRC(84801A50) SHA1(8700e4fb56941b87f8333e72e2a1c7ac9e322312) )
	ROM_LOAD16_BYTE( "988jaa05.22a", 0x100000, 0x80000, CRC(0DDF7D6D) SHA1(aa110ab64c2fbf427796dff3a817b57cf6a9440d) )
	ROM_LOAD16_BYTE( "988jaa06.24a", 0x100001, 0x80000, CRC(2A87F69E) SHA1(fe84bb50864467a83d06d34a18123ab11fb55781) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "988jaa07.22d", 0x000000, 0x80000, CRC(9E57FE24) SHA1(40bd0428227e46ebe365f2f6821b08182a0ce698) )
	ROM_LOAD16_BYTE( "988jaa08.23d", 0x000001, 0x80000, CRC(BF604CA4) SHA1(6abc81d5d9084fcf59f70a6bd57e1b36041a1072) )
	ROM_LOAD16_BYTE( "988jaa09.25d", 0x100000, 0x80000, CRC(8F3BAE7F) SHA1(c4dac14f6c7f75a2b19153e05bfe969e9eb4aca0) )
	ROM_LOAD16_BYTE( "988jaa10.27d", 0x100001, 0x80000, CRC(248BF0EE) SHA1(d89205ed57e771401bfc2c24043d200ecbd0b7fc) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "988jaa11", 0, MD5(e7d1bcc21a56db62c47f3293fbdf80d9) SHA1(00a2e258d66585002413a999afb695a3f3fe30cb) )	/* ver 1.00 JA */
ROM_END

ROM_START( hmcompm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "988uaa01.6a", 0x000000, 0x80000, CRC(5E5CC6C0) SHA1(0e7cd601d4543715cbc9f65e6fd48837179c962a) )
	ROM_LOAD16_BYTE( "988uaa02.8a", 0x000001, 0x80000, CRC(E262984A) SHA1(f47662e40f91f2addb1a4b649923c1d0ee017341) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "988uaa03.19a", 0x000000, 0x80000, CRC(D0F204C8) SHA1(866baac5a6d301d5b9cf0c14e9937ee5f435db77) )
	ROM_LOAD16_BYTE( "988uaa04.20a", 0x000001, 0x80000, CRC(74C6B3ED) SHA1(7d9b064bab3f29fc6435f6430c71208abbf9d861) )
	ROM_LOAD16_BYTE( "988uaa05.22a", 0x100000, 0x80000, CRC(6B9321CB) SHA1(449e5f85288a8c6724658050fa9521c7454a1e46) )
	ROM_LOAD16_BYTE( "988uaa06.24a", 0x100001, 0x80000, CRC(DA6E0C1E) SHA1(4ef37db6c872bccff8c27fc53cccc0b269c7aee4) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "988uaa07.22d", 0x000000, 0x80000, CRC(9217870D) SHA1(d0536a8a929c41b49cdd053205165bfb8150e0c5) )
	ROM_LOAD16_BYTE( "988uaa08.23d", 0x000001, 0x80000, CRC(77777E59) SHA1(33b5508b961a04b82c9967a3326af6bbd838b85e) )
	ROM_LOAD16_BYTE( "988uaa09.25d", 0x100000, 0x80000, CRC(C2AD6810) SHA1(706388c5acf6718297fd90e10f8a673463a0893b) )
	ROM_LOAD16_BYTE( "988uaa10.27d", 0x100001, 0x80000, CRC(DAB0F3C9) SHA1(6fd899e753e32f60262c54ab8553c686c7ef28de) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "988jaa11", 0, MD5(e7d1bcc21a56db62c47f3293fbdf80d9) SHA1(00a2e258d66585002413a999afb695a3f3fe30cb) )	/* ver 1.00 JA */
ROM_END

ROM_START( bmdct )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "995jaa01.6a", 0x000000, 0x80000, CRC(2C224169) SHA1(0608469fa0a15026f461be5141ed29bf740144ca) )
	ROM_LOAD16_BYTE( "995jaa02.8a", 0x000001, 0x80000, CRC(A2EDB472) SHA1(795e44e56dfee6c5eceb28172bc20ba5b31c366b) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "995jaa03.19a", 0x000000, 0x80000, CRC(77A7030C) SHA1(8f7988ca5c248d0846ec22c0975ae008d85e8d72) )
	ROM_LOAD16_BYTE( "995jaa04.20a", 0x000001, 0x80000, CRC(A12EA45D) SHA1(9bd48bc25c17f885d74e859de153ec49012a4e39) )
	ROM_LOAD16_BYTE( "995jaa05.22a", 0x100000, 0x80000, CRC(1493FD98) SHA1(4cae2ebccc79b21d7e21b984dc6fe10ab3013a2d) )
	ROM_LOAD16_BYTE( "995jaa06.24a", 0x100001, 0x80000, CRC(86BFF0BB) SHA1(658280f78987eaee31b60a7826db6df105601f0a) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "995jaa07.22d", 0x000000, 0x80000, CRC(CE030EDF) SHA1(1e2594a6a04559d70b09750bb665d8cd3d0288ea) )
	ROM_LOAD16_BYTE( "995jaa08.23d", 0x000001, 0x80000, CRC(375D3D17) SHA1(180cb5ad4497b3745aa9317764f237b30a678b31) )
	ROM_LOAD16_BYTE( "995jaa09.25d", 0x100000, 0x80000, CRC(1510A9C2) SHA1(daf1ab26b7b6b0fe0123b3fbee68684157c2ce51) )
	ROM_LOAD16_BYTE( "995jaa10.27d", 0x100001, 0x80000, CRC(F9E4E9F2) SHA1(fe91badf6b0baeea690d75399d8c66fabcf6d352) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "995jaa11", 0, MD5(8f5936d2b0b0914b5c88f5432c6cac21) SHA1(deed0fca533f3e56e04f9967f3f76145ca106f06) )	/* ver 1.00 JA */
ROM_END

ROM_START( bmcorerm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "a05jaa01.6a", 0x000000, 0x80000, CRC(CD6F1FC5) SHA1(237cbc17a693efb6bffffd6afb24f0944c29330c) )
	ROM_LOAD16_BYTE( "a05jaa02.8a", 0x000001, 0x80000, CRC(FE07785E) SHA1(14c652008cb509b5206fb515aad7dfe36a6fe6f4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "a05jaa03.19a", 0x000000, 0x80000, CRC(8B88932A) SHA1(df20f8323adb02d07b835da98f4a29b3142175c9) )
	ROM_LOAD16_BYTE( "a05jaa04.20a", 0x000001, 0x80000, CRC(CC72629F) SHA1(f95d06f409c7d6422d66a55c0452eb3feafc6ef0) )
	ROM_LOAD16_BYTE( "a05jaa05.22a", 0x100000, 0x80000, CRC(E241B22B) SHA1(941a76f6ac821e0984057ec7df7862b12fa657b8) )
	ROM_LOAD16_BYTE( "a05jaa06.24a", 0x100001, 0x80000, CRC(77EB08A3) SHA1(fd339aaec06916abfc928e850e33480707b5450d) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "a05jaa07.22d", 0x000000, 0x80000, CRC(4D79646D) SHA1(5f1237bbd3cb09b27babf1c5359ef6c0d80ae3a9) )
	ROM_LOAD16_BYTE( "a05jaa08.23d", 0x000001, 0x80000, CRC(F067494F) SHA1(ef031b5501556c1aa047a51604a44551b35a8b99) )
	ROM_LOAD16_BYTE( "a05jaa09.25d", 0x100000, 0x80000, CRC(1504D62C) SHA1(3c31c6625bc089235a96fe21021239f2d0c0f6e1) )
	ROM_LOAD16_BYTE( "a05jaa10.27d", 0x100001, 0x80000, CRC(99D75C36) SHA1(9599420863aa0a9492d3caeb03f8ac5fd4c3cdb2) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "a05jaa11", 0, MD5(bf63321a1bb167f35f1a1111c02874b6) SHA1(95f8e5d29cc9f9af2b1c9397a26217f685f9d5a7) )	/* ver 1.00 JA */
ROM_END

ROM_START( bm6thmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "a21jaa01.6a", 0x000000, 0x80000, CRC(6D7CCBE3) SHA1(633c69c14dfd70866664b94095fa5f21087428d8) )
	ROM_LOAD16_BYTE( "a21jaa02.8a", 0x000001, 0x80000, CRC(F10076FA) SHA1(ab9f3e75a36fdaccec411afd77f588f040db139d) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "a21jaa03.19a", 0x000000, 0x80000, CRC(CA806266) SHA1(6b5f9d5089a992347745ab6af4dadaac4e3b0742) )
	ROM_LOAD16_BYTE( "a21jaa04.20a", 0x000001, 0x80000, CRC(71124E79) SHA1(d9fd8f662ac9c29daf25acd310fd0f27051dea0b) )
	ROM_LOAD16_BYTE( "a21jaa05.22a", 0x100000, 0x80000, CRC(818E34E6) SHA1(8a9093b92392a065d0cf94d56195a6f3ca611044) )
	ROM_LOAD16_BYTE( "a21jaa06.24a", 0x100001, 0x80000, CRC(36F2043B) SHA1(d2846cc10173662029da7c5d686cf89299be2be5) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "a21jaa07.22d", 0x000000, 0x80000, CRC(841D83E1) SHA1(c85962abcc955e8f11138e03002b16afd3791f0a) )
	ROM_LOAD16_BYTE( "a21jaa08.23d", 0x000001, 0x80000, CRC(4E561919) SHA1(4b91560d9ba367c848d784db760f042d5d76e003) )
	ROM_LOAD16_BYTE( "a21jaa09.25d", 0x100000, 0x80000, CRC(181E6F70) SHA1(82c7ca3068ace9a66b614ead4b90ea6fe4017d51) )
	ROM_LOAD16_BYTE( "a21jaa10.27d", 0x100001, 0x80000, CRC(1AC33595) SHA1(3173bb8dc420487c4d427e779444a98aad37d51e) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "a21jaa11", 0, MD5(d7a02bd5db4f6fbefaceb1781d35d7a6) SHA1(9e9fb8000cfb53686d94060d7391fb2950265510) )	/* ver 1.00 JA */
ROM_END

/*************************************
 *
 *  Driver-specific init
 *
 *************************************/

static DRIVER_INIT( beatmania )
{
	UINT8 *region;

	if (new_memory_region(REGION_SOUND1, 0x80000 * 32, 0) != 0)
		return;
	region = memory_region(REGION_SOUND1);

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);
}

static UINT8 beatmania_master_password[2 + 32] =
{
	0x01, 0x00,
	0x4d, 0x47, 0x43, 0x28, 0x4b, 0x29, 0x4e, 0x4f,
	0x4d, 0x41, 0x20, 0x49, 0x4c, 0x41, 0x20, 0x4c,
	0x49, 0x52, 0x48, 0x47, 0x53, 0x54, 0x52, 0x20,
	0x53, 0x45, 0x52, 0x45, 0x45, 0x56, 0x2e, 0x44
};

static DRIVER_INIT( hmcompmx )
{
	static UINT8 hmcompmx_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3a, 0x34, 0x38, 0x2a,
		0x5a, 0x4d, 0x78, 0x3e, 0x74, 0x61, 0x6c, 0x0a,
		0x7a, 0x63, 0x19, 0x77, 0x73, 0x7d, 0x0d, 0x12,
		0x6b, 0x09, 0x02, 0x0f, 0x05, 0x00, 0x7d, 0x1b
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompmx_user_password);
}

static DRIVER_INIT( bm4thmix )
{
	static UINT8 bm4thmix_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x29, 0x4b, 0x2f, 0x2c, 0x4c, 0x32,
		0x48, 0x5d, 0x0c, 0x3e, 0x62, 0x6f, 0x7e, 0x73,
		0x67, 0x10, 0x19, 0x79, 0x6c, 0x7d, 0x00, 0x01,
		0x18, 0x06, 0x1e, 0x07, 0x77, 0x1a, 0x7d, 0x77
	};

	init_beatmania();

	ide_set_user_password(0, bm4thmix_user_password);
}

static DRIVER_INIT( bm5thmix )
{
	static UINT8 bm5thmix_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x37, 0x35, 0x4a, 0x23,
		0x5a, 0x52, 0x0c, 0x3e, 0x60, 0x04, 0x6c, 0x78,
		0x77, 0x7e, 0x74, 0x16, 0x6c, 0x7d, 0x00, 0x16,
		0x6b, 0x1a, 0x1e, 0x06, 0x04, 0x01, 0x7d, 0x1f
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bm5thmix_user_password);
}

static DRIVER_INIT( bmcompm2 )
{
	static UINT8 bmcompm2_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x3a, 0x20, 0x31, 0x3e, 0x46, 0x2c, 0x35, 0x46,
		0x48, 0x51, 0x6f, 0x3e, 0x73, 0x6b, 0x68, 0x0a,
		0x60, 0x71, 0x19, 0x6f, 0x70, 0x68, 0x07, 0x62,
		0x6b, 0x0d, 0x71, 0x0f, 0x1d, 0x10, 0x7d, 0x7a
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmcompm2_user_password);
}

static DRIVER_INIT( hmcompm2 )
{
	static UINT8 hmcompm2_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x3b, 0x39, 0x24, 0x3e, 0x4e, 0x59, 0x5c, 0x32,
		0x3b, 0x4c, 0x72, 0x57, 0x69, 0x04, 0x79, 0x65,
		0x76, 0x10, 0x6a, 0x77, 0x1f, 0x65, 0x0a, 0x16,
		0x09, 0x68, 0x71, 0x0b, 0x77, 0x15, 0x17, 0x1e
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompm2_user_password);
}

static DRIVER_INIT( bmdct )
{
	static UINT8 bmdct_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x52, 0x47, 0x30, 0x3f, 0x2f, 0x39, 0x54, 0x5e,
		0x4f, 0x4b, 0x65, 0x3e, 0x07, 0x6e, 0x6c, 0x67,
		0x7d, 0x79, 0x7b, 0x16, 0x6d, 0x73, 0x65, 0x06,
		0x0e, 0x0a, 0x05, 0x0f, 0x13, 0x74, 0x09, 0x19
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmdct_user_password);
}

static DRIVER_INIT( bmcorerm )
{
	static UINT8 bmcorerm_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3f, 0x4d, 0x4a, 0x27,
		0x5a, 0x52, 0x0c, 0x3e, 0x6a, 0x04, 0x63, 0x6f,
		0x72, 0x64, 0x72, 0x7f, 0x1f, 0x73, 0x17, 0x04,
		0x05, 0x09, 0x14, 0x0d, 0x7a, 0x74, 0x7d, 0x7a
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmcorerm_user_password);
}

static DRIVER_INIT( bm6thmix )
{
	static UINT8 bm6thmix_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3d, 0x4d, 0x4a, 0x23,
		0x5a, 0x52, 0x0c, 0x3e, 0x6a, 0x04, 0x63, 0x65,
		0x7e, 0x7f, 0x77, 0x77, 0x1f, 0x79, 0x04, 0x0f,
		0x02, 0x06, 0x09, 0x0f, 0x7a, 0x74, 0x7d, 0x7a
	};

	init_beatmania();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bm6thmix_user_password);
}


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1997, bm1stmix, 0,        djmain,   bm1stmix,  beatmania, ROT0, "Konami", "beatmania (ver JA-B)", 0 )
GAME( 1998, bm2ndmix, 0,        djmain,   bm2ndmix,  beatmania, ROT0, "Konami", "beatmania 2nd MIX (ver JA-B)", 0 )
GAME( 1998, bm2ndmxa, bm2ndmix, djmain,   bm2ndmix,  beatmania, ROT0, "Konami", "beatmania 2nd MIX (ver JA-A)", 0 )
GAME( 1999, bmcompmx, 0,        djmain,   bmcompmx,  beatmania, ROT0, "Konami", "beatmania complete MIX (ver JA-B)", 0 )
GAME( 1999, hmcompmx, bmcompmx, djmain,   bmcompmx,  hmcompmx,  ROT0, "Konami", "hiphopmania complete MIX (ver UA-B)", 0 )
GAME( 1999, bm4thmix, 0,        djmain,   bm4thmix,  bm4thmix,  ROT0, "Konami", "beatmania 4th MIX (ver JA-A)", 0 )
GAME( 1999, bm5thmix, 0,        djmain,   beatmania, bm5thmix,  ROT0, "Konami", "beatmania 5th MIX (ver JA-A)", 0 )
GAME( 2000, bmcompm2, 0,        djmain,   beatmania, bmcompm2,  ROT0, "Konami", "beatmania complete MIX 2 (ver JA-A)", 0 )
GAME( 2000, hmcompm2, bmcompm2, djmain,   hmcompm2,  hmcompm2,  ROT0, "Konami", "hiphopmania complete MIX 2 (ver UA-A)", 0 )
GAME( 2000, bmdct,    0,        djmain,   bmdct,     bmdct,     ROT0, "Konami", "beatmania f. Dreams Come True (ver JA-A)", 0 )
GAME( 2000, bmcorerm, 0,        djmain,   beatmania, bmcorerm,  ROT0, "Konami", "beatmania CORE REMIX (ver JA-A)", 0 )
GAME( 2001, bm6thmix, 0,        djmain,   beatmania, bm6thmix,  ROT0, "Konami", "beatmania 6th MIX (ver JA-A)", 0 )
