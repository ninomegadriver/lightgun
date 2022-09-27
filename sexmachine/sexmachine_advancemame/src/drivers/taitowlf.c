/*  Taito Wolf System

    Driver by Ville Linde
*/

#include "driver.h"
#include "memconv.h"
#include "machine/8237dma.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/mc146818.h"
#include "machine/pcshare.h"
#include "machine/pci.h"
#include "machine/8042kbdc.h"
#include "machine/pckeybrd.h"
#include "machine/idectrl.h"
#include "cpu/i386/i386.h"

static UINT32 *cga_ram;
static UINT32 *bios_ram;

static int cga_palette[16][3] =
{
	{ 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0xaa }, { 0x00, 0xaa, 0x00 }, { 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0x00 }, { 0xaa, 0x00, 0xaa }, { 0xaa, 0x55, 0x00 }, { 0xaa, 0xaa, 0xaa },
	{ 0x55, 0x55, 0x55 }, { 0x55, 0x55, 0xff }, { 0x55, 0xff, 0x55 }, { 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0x55 }, { 0xff, 0x55, 0xff }, { 0xff, 0xff, 0x55 }, { 0xff, 0xff, 0xff },
};

static VIDEO_START(taitowlf)
{
	int i;
	for (i=0; i < 16; i++)
	{
		palette_set_color(i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2]);
	}
	return 0;
}

static void draw_char(mame_bitmap *bitmap, const rectangle *cliprect, const gfx_element *gfx, int ch, int att, int x, int y)
{
	int i,j;
	UINT8 *dp;
	int index = 0;
	dp = gfx->gfxdata + ch * gfx->char_modulo;

	for (j=y; j < y+8; j++)
	{
		UINT16 *p = bitmap->line[j];
		for (i=x; i < x+8; i++)
		{
			UINT8 pen = dp[index++];
			if (pen)
			{
				p[i] = gfx->colortable[att & 0xf];
			}
			else
			{
				p[i] = gfx->colortable[(att >> 4) & 0x7];
			}
		}
	}
}

static VIDEO_UPDATE(taitowlf)
{
	int i, j;
	const gfx_element *gfx = Machine->gfx[0];
	UINT32 *cga = cga_ram;
	int index = 0;

	fillbitmap(bitmap, 0, cliprect);

	for (j=0; j < 25; j++)
	{
		for (i=0; i < 80; i+=2)
		{
			int att0 = (cga[index] >> 8) & 0xff;
			int ch0 = (cga[index] >> 0) & 0xff;
			int att1 = (cga[index] >> 24) & 0xff;
			int ch1 = (cga[index] >> 16) & 0xff;

			draw_char(bitmap, cliprect, gfx, ch0, att0, i*8, j*8);
			draw_char(bitmap, cliprect, gfx, ch1, att1, (i*8)+8, j*8);
			index++;
		}
	}
}

static READ8_HANDLER(at_dma8237_1_r)
{
	return dma8237_1_r(offset / 2);
}

static WRITE8_HANDLER(at_dma8237_1_w)
{
	dma8237_1_w(offset / 2, data);
}

static READ32_HANDLER(at32_dma8237_1_r)
{
	return read32le_with_read8_handler(at_dma8237_1_r, offset, mem_mask);
}

static WRITE32_HANDLER(at32_dma8237_1_w)
{
	write32le_with_write8_handler(at_dma8237_1_w, offset, data, mem_mask);
}



// Intel 82439TX System Controller (MXTC)
static UINT8 mxtc_config_reg[256];

static UINT8 mxtc_config_r(int function, int reg)
{
//  printf("MXTC: read %d, %02X\n", function, reg);

	return mxtc_config_reg[reg];
}

static void mxtc_config_w(int function, int reg, UINT8 data)
{
//  printf("MXTC: write %d, %02X, %02X at %08X\n", function, reg, data, activecpu_get_pc());

	switch(reg)
	{
		case 0x59:		// PAM0
		{
			if (data & 0x10)		// enable RAM access to region 0xf0000 - 0xfffff
			{
				memory_set_bankptr(1, bios_ram);
			}
			else					// disable RAM access (reads go to BIOS ROM)
			{
				memory_set_bankptr(1, memory_region(REGION_USER1) + 0x30000);
			}
			break;
		}
	}

	mxtc_config_reg[reg] = data;
}

static void intel82439tx_init(void)
{
	mxtc_config_reg[0x60] = 0x02;
	mxtc_config_reg[0x61] = 0x02;
	mxtc_config_reg[0x62] = 0x02;
	mxtc_config_reg[0x63] = 0x02;
	mxtc_config_reg[0x64] = 0x02;
	mxtc_config_reg[0x65] = 0x02;
}

static UINT32 intel82439tx_pci_r(int function, int reg, UINT32 mem_mask)
{
	UINT32 r = 0;
	if (!(mem_mask & 0xff000000))
	{
		r |= mxtc_config_r(function, reg + 3) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= mxtc_config_r(function, reg + 2) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= mxtc_config_r(function, reg + 1) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= mxtc_config_r(function, reg + 0) << 0;
	}
	return r;
}

static void intel82439tx_pci_w(int function, int reg, UINT32 data, UINT32 mem_mask)
{
	if (!(mem_mask & 0xff000000))
	{
		mxtc_config_w(function, reg + 3, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		mxtc_config_w(function, reg + 2, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		mxtc_config_w(function, reg + 1, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		mxtc_config_w(function, reg + 0, (data >> 0) & 0xff);
	}
}

// Intel 82371AB PCI-to-ISA / IDE bridge (PIIX4)
static UINT8 piix4_config_reg[4][256];

static UINT8 piix4_config_r(int function, int reg)
{
//  printf("PIIX4: read %d, %02X\n", function, reg);
	return piix4_config_reg[function][reg];
}

static void piix4_config_w(int function, int reg, UINT8 data)
{
//  printf("PIIX4: write %d, %02X, %02X at %08X\n", function, reg, data, activecpu_get_pc());
	piix4_config_reg[function][reg] = data;
}

static UINT32 intel82371ab_pci_r(int function, int reg, UINT32 mem_mask)
{
	UINT32 r = 0;
	if (!(mem_mask & 0xff000000))
	{
		r |= piix4_config_r(function, reg + 3) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= piix4_config_r(function, reg + 2) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= piix4_config_r(function, reg + 1) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= piix4_config_r(function, reg + 0) << 0;
	}
	return r;
}

static void intel82371ab_pci_w(int function, int reg, UINT32 data, UINT32 mem_mask)
{
	if (!(mem_mask & 0xff000000))
	{
		piix4_config_w(function, reg + 3, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		piix4_config_w(function, reg + 2, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		piix4_config_w(function, reg + 1, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		piix4_config_w(function, reg + 0, (data >> 0) & 0xff);
	}
}

// ISA Plug-n-Play
static WRITE32_HANDLER( pnp_config_w )
{
	if (!(mem_mask & 0x0000ff00))
	{
//      printf("PNP Config: %02X\n", (data >> 8) & 0xff);
	}
}

static WRITE32_HANDLER( pnp_data_w )
{
	if (!(mem_mask & 0x0000ff00))
	{
//      printf("PNP Data: %02X\n", (data >> 8) & 0xff);
	}
}



static READ32_HANDLER( ide0_r )
{
	return ide_controller32_0_r(0x1f0/4 + offset, mem_mask);
}

static WRITE32_HANDLER( ide0_w )
{
	ide_controller32_0_w(0x1f0/4 + offset, data, mem_mask);
}

static READ32_HANDLER( fdc_r )
{
	return ide_controller32_0_r(0x3f0/4 + offset, mem_mask);
}

static WRITE32_HANDLER( fdc_w )
{
	//printf("FDC: write %08X, %08X, %08X\n", data, offset, mem_mask);
	ide_controller32_0_w(0x3f0/4 + offset, data, mem_mask);
}



static WRITE32_HANDLER(bios_ram_w)
{
	if (mxtc_config_reg[0x59] & 0x20)		// write to RAM if this region is write-enabled
	{
		COMBINE_DATA(bios_ram + offset);
	}
}

/*****************************************************************************/

static ADDRESS_MAP_START( taitowlf_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0009ffff) AM_RAM
	AM_RANGE(0x000a0000, 0x000affff) AM_RAM
	AM_RANGE(0x000b0000, 0x000b7fff) AM_RAM AM_BASE(&cga_ram)
	AM_RANGE(0x000e0000, 0x000effff) AM_RAM
	AM_RANGE(0x000f0000, 0x000fffff) AM_ROMBANK(1)
	AM_RANGE(0x000f0000, 0x000fffff) AM_WRITE(bios_ram_w)
	AM_RANGE(0x00100000, 0x01ffffff) AM_RAM
	AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)	/* System BIOS */
ADDRESS_MAP_END

static ADDRESS_MAP_START(taitowlf_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_32le_0_r,			dma8237_32le_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_32le_0_r,			pic8259_32le_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_32le_0_r,			pit8253_32le_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(kbdc8042_32le_r,			kbdc8042_32le_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32le_r,		mc146818_port32le_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page32_r,				at_page32_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_32le_1_r,			pic8259_32le_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at32_dma8237_1_r,			at32_dma8237_1_w)
	AM_RANGE(0x00e8, 0x00eb) AM_NOP
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(ide0_r, ide0_w)
	AM_RANGE(0x0300, 0x03af) AM_NOP
	AM_RANGE(0x03b0, 0x03df) AM_NOP
	AM_RANGE(0x0278, 0x027b) AM_WRITE(pnp_config_w)
	AM_RANGE(0x03f0, 0x03ff) AM_READWRITE(fdc_r, fdc_w)
	AM_RANGE(0x0a78, 0x0a7b) AM_WRITE(pnp_data_w)
	AM_RANGE(0x0cf8, 0x0cff) AM_READWRITE(pci_32le_r,				pci_32le_w)
ADDRESS_MAP_END

/*****************************************************************************/

static const gfx_layout CGA_charlayout =
{
	8,8,					/* 8 x 16 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8,
	  0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static const gfx_decode CGA_gfxdecodeinfo[] =
{
/* Support up to four CGA fonts */
	{ REGION_GFX1, 0x0000, &CGA_charlayout,              0, 256 },   /* Font 0 */
	{ REGION_GFX1, 0x0800, &CGA_charlayout,              0, 256 },   /* Font 1 */
	{ REGION_GFX1, 0x1000, &CGA_charlayout,              0, 256 },   /* Font 2 */
	{ REGION_GFX1, 0x1800, &CGA_charlayout,              0, 256 },   /* Font 3*/
    { -1 } /* end of array */
};

#define AT_KEYB_HELPER(bit, text, key1) \
	PORT_BIT( bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(text) PORT_CODE(key1)

INPUT_PORTS_START(taitowlf)
	PORT_START	/* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	AT_KEYB_HELPER( 0x0002, "Esc",          KEYCODE_Q           ) /* Esc                         01  81 */

	PORT_START	/* IN5 */
	AT_KEYB_HELPER( 0x0020, "Y",            KEYCODE_Y           ) /* Y                           15  95 */
	AT_KEYB_HELPER( 0x1000, "Enter",        KEYCODE_ENTER       ) /* Enter                       1C  9C */

	PORT_START	/* IN6 */

	PORT_START	/* IN7 */
	AT_KEYB_HELPER( 0x0002, "N",            KEYCODE_N           ) /* N                           31  B1 */
	AT_KEYB_HELPER( 0x0800, "F1",           KEYCODE_S           ) /* F1                          3B  BB */

	PORT_START	/* IN8 */

	PORT_START	/* IN9 */

	PORT_START	/* IN10 */
	AT_KEYB_HELPER( 0x0040, "(MF2)Cursor Up",		KEYCODE_UP          ) /* Up                          67  e7 */
	AT_KEYB_HELPER( 0x0080, "(MF2)Page Up",			KEYCODE_PGUP        ) /* Page Up                     68  e8 */
	AT_KEYB_HELPER( 0x0100, "(MF2)Cursor Left",		KEYCODE_LEFT        ) /* Left                        69  e9 */
	AT_KEYB_HELPER( 0x0200, "(MF2)Cursor Right",	KEYCODE_RIGHT       ) /* Right                       6a  ea */
	AT_KEYB_HELPER( 0x0800, "(MF2)Cursor Down",		KEYCODE_DOWN        ) /* Down                        6c  ec */
	AT_KEYB_HELPER( 0x1000, "(MF2)Page Down",		KEYCODE_PGDN        ) /* Page Down                   6d  ed */
	AT_KEYB_HELPER( 0x4000, "Del",       		    KEYCODE_A           ) /* Delete                      6f  ef */

	PORT_START	/* IN11 */
INPUT_PORTS_END

static int irq_callback(int irqline)
{
	int r = 0;
	r = pic8259_acknowledge(1);
	if (r==0)
	{
		r = pic8259_acknowledge(0);
	}
	return r;
}

static MACHINE_RESET(taitowlf)
{
	memory_set_bankptr(1, memory_region(REGION_USER1) + 0x30000);

	dma8237_reset();
	cpunum_set_irq_callback(0, irq_callback);
}

static MACHINE_DRIVER_START(taitowlf)

	/* basic machine hardware */
	MDRV_CPU_ADD(PENTIUM, 200000000)
	MDRV_CPU_PROGRAM_MAP(taitowlf_map, 0)
	MDRV_CPU_IO_MAP(taitowlf_io, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_RESET(taitowlf)

	MDRV_NVRAM_HANDLER( mc146818 )

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 639, 0, 199)
	MDRV_GFXDECODE(CGA_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)

	MDRV_VIDEO_START(taitowlf)
	MDRV_VIDEO_UPDATE(taitowlf)

MACHINE_DRIVER_END

static struct pci_device_info intel82439tx =
{
	intel82439tx_pci_r,
	intel82439tx_pci_w
};

static struct pci_device_info intel82371ab =
{
	intel82371ab_pci_r,
	intel82371ab_pci_w
};

static void set_gate_a20(int a20)
{
	cpunum_set_input_line(0, INPUT_LINE_A20, a20);
}

static void keyboard_interrupt(int state)
{
	pic8259_set_irq_line(0, 1, state);
}

static void ide_interrupt(int state)
{
	pic8259_set_irq_line(1, 6, state);
}

static struct kbdc8042_interface at8042 =
{
	KBDC8042_AT386, set_gate_a20, keyboard_interrupt
};

static struct ide_interface ide_intf =
{
	ide_interrupt
};

static DRIVER_INIT( taitowlf )
{
	bios_ram = auto_malloc(0x10000);

	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_DMA8237_AT | PCCOMMON_TIMER_8254);
	mc146818_init(MC146818_STANDARD);

	intel82439tx_init();

	pci_init();
	pci_add_device(0, 0, &intel82439tx);
	pci_add_device(0, 7, &intel82371ab);

	kbdc8042_init(&at8042);

	ide_controller_init(0, &ide_intf);
}

/*****************************************************************************/

ROM_START(pf2012)
	ROM_REGION32_LE(0x40000, REGION_USER1, 0)
	ROM_LOAD("p5tx-la.bin", 0x00000, 0x40000, CRC(072e6d51) SHA1(70414349b37e478fc28ecbaba47ad1033ae583b7))

	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))

	ROM_REGION32_LE(0x400000, REGION_USER3, 0)		// Program ROM disk
	ROM_LOAD("u1.bin", 0x000000, 0x200000, CRC(8f4c09cb) SHA1(0969a92fec819868881683c580f9e01cbedf4ad2))
	ROM_LOAD("u2.bin", 0x200000, 0x200000, CRC(59881781) SHA1(85ff074ab2a922eac37cf96f0bf153a2dac55aa4))

	ROM_REGION32_LE(0x4000000, REGION_USER4, 0)		// Data ROM disk
	ROM_LOAD("e59-01.u20", 0x0000000, 0x800000, CRC(701d3a9a) SHA1(34c9f34f4da34bb8eed85a4efd1d9eea47a21d77) )
	ROM_LOAD("e59-02.u23", 0x0800000, 0x800000, CRC(626df682) SHA1(35bb4f91201734ce7ccdc640a75030aaca3d1151) )
	ROM_LOAD("e59-03.u26", 0x1000000, 0x800000, CRC(74e4efde) SHA1(630235c2e4a11f615b5f3b8c93e1e645da09eefe) )
	ROM_LOAD("e59-04.u21", 0x1800000, 0x800000, CRC(c900e8df) SHA1(93c06b8f5082e33f0dcc41f1be6a79283de16c40) )
	ROM_LOAD("e59-05.u24", 0x2000000, 0x800000, CRC(85b0954c) SHA1(1b533d5888d56d1510c79f790e4fa708f77e836f) )
	ROM_LOAD("e59-06.u27", 0x2800000, 0x800000, CRC(0573a113) SHA1(ee76a71dfd31289a9a5428653a36d01d914fc5d9) )
	ROM_LOAD("e59-07.u22", 0x3000000, 0x800000, CRC(1f0ddcdc) SHA1(72ffe08f5effab093bdfe9863f8a11f80e914272) )
	ROM_LOAD("e59-08.u25", 0x3800000, 0x800000, CRC(8db38ffd) SHA1(4b71ea86fb774ba6a8ac45abf4191af64af007e7) )

	ROM_REGION(0x1400000, REGION_SOUND1, 0)			// ZOOM sample data
	ROM_LOAD("e59-09.u29", 0x0000000, 0x800000, CRC(d0da5c50) SHA1(56fb3c38f35244720d32a44fed28e6b58c7851f7) )
	ROM_LOAD("e59-10.u32", 0x0800000, 0x800000, CRC(4c0e0a5c) SHA1(6454befa3a1dd532eb2a760129dcd7e611508730) )
	ROM_LOAD("e59-11.u33", 0x1000000, 0x400000, CRC(c90a896d) SHA1(2b62992f20e4ca9634e7953fe2c553906de44f04) )

	ROM_REGION(0x180000, REGION_CPU2, 0)			// MN10200 program
	ROM_LOAD("e59-12.u13", 0x000000, 0x80000, CRC(9a473a7e) SHA1(b0ec7b0ae2b33a32da98899aa79d44e8e318ceb7) )
	ROM_LOAD("e59-13.u15", 0x080000, 0x80000, CRC(77719880) SHA1(8382dd2dfb0dae60a3831ed6d3ff08539e2d94eb) )
	ROM_LOAD("e59-14.u14", 0x100000, 0x40000, CRC(d440887c) SHA1(d965871860d757bc9111e9adb2303a633c662d6b) )
	ROM_LOAD("e59-15.u16", 0x140000, 0x40000, CRC(eae8e523) SHA1(8a054d3ded7248a7906c4f0bec755ddce53e2023) )

	ROM_REGION(0x20000, REGION_USER5, 0)			// bootscreen
	ROM_LOAD("e58-04.u71", 0x000000, 0x20000, CRC(500e6113) SHA1(93226706517c02e336f96bdf9443785158e7becf) )
ROM_END

/*****************************************************************************/

GAME(1997, pf2012, 0,	taitowlf, taitowlf, taitowlf,	ROT0,   "Taito",  "Psychic Force 2012", GAME_NOT_WORKING | GAME_NO_SOUND);
