/*
	TODO:
	. 5 has some issues, RAM banking needs hardware flags to determine size
	. 9 & 10 have minor probs: needs PPU to emulate sprite caching Most visible on Punch-Out! (9)
	. 13 needs banked VRAM. -see Videomation
	. 64 has some IRQ problems - see Skull & Crossbones
	. 67 display issues, but vrom fixed
	. 70 (ark2j) starts on round 0 - is this right? Yep!
	. 82 has chr-rom banking problems. Also mapper is in the middle of sram, which is unemulated.
	. 96 is preliminary.
	. 118 mirroring is a guess, chr-rom banking is likely different
	. 228 seems wrong
	. 229 is preliminary

	AD&D Hillsfar (mapper 1) seems to be broken. Not sure what's up there

	Also, remember that the MMC does not equal the mapper #. In particular, Mapper 4 is
	really MMC3, Mapper 9 is MMC2 and Mapper 10 is MMC4. Makes perfect sense, right?
*/

#include <string.h>

#include "cpu/m6502/m6502.h"
#include "vidhrdw/ppu2c03b.h"
#include "osdepend.h"
#include "driver.h"
#include "includes/nes.h"
#include "nes_mmc.h"
#include "sound/nes_apu.h"

#define LOG_MMC	1
#define LOG_FDS	1

/* Global variables */
int prg_mask;

int IRQ_enable, IRQ_enable_latch;
UINT16 IRQ_count, IRQ_count_latch;
UINT8 IRQ_status;
UINT8 IRQ_mode_jaleco;

int MMC1_extended;	/* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

write8_handler mmc_write_low;
read8_handler mmc_read_low;
write8_handler mmc_write_mid;
read8_handler mmc_read_mid;
write8_handler mmc_write;
/*void (*ppu_latch)(offs_t offset);*/

static int vrom_bank[16];
static int mult1, mult2;

/* Local variables */
static int MMC1_Size_16k;
static int MMC1_High;
static int MMC1_reg;
static int MMC1_reg_count;
static int MMC1_Switch_Low, MMC1_SizeVrom_4k;
static int MMC1_bank1, MMC1_bank2, MMC1_bank3, MMC1_bank4;
static int MMC1_extended_bank;
static int MMC1_extended_base;
static int MMC1_extended_swap;

static int MMC2_bank0, MMC2_bank0_hi, MMC2_bank0_latch, MMC2_bank1, MMC2_bank1_hi, MMC2_bank1_latch;

static int MMC3_cmd;
static int MMC3_prg0, MMC3_prg1;
static int MMC3_chr[6];

static int MMC5_rom_bank_mode;
static int MMC5_vrom_bank_mode;
static int MMC5_vram_protect;
static int MMC5_scanline;
UINT8 MMC5_vram[0x400];
int MMC5_vram_control;

static int mapper41_chr, mapper41_reg2;

static int mapper_warning;

WRITE8_HANDLER( nes_low_mapper_w )
{
	if (mmc_write_low) (*mmc_write_low)(offset, data);
	else
	{
		logerror("Unimplemented LOW mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (! mapper_warning)
		{
			logerror ("This game is writing to the low mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}
#endif
	}
}

/* Handle mapper reads from $4100-$5fff */
 READ8_HANDLER( nes_low_mapper_r )
{
	if (mmc_read_low)
		return (*mmc_read_low)(offset);
	else
		logerror("low mapper area read, addr: %04x\n", offset + 0x4100);

	return 0;
}

WRITE8_HANDLER ( nes_mid_mapper_w )
{
	if (mmc_write_mid) (*mmc_write_mid)(offset, data);
	else if (nes.mid_ram_enable)
		battery_ram[offset] = data;
//	else
	{
		logerror("Unimplemented MID mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (! mapper_warning)
		{
			logerror ("This game is writing to the MID mapper area but no mapper is set. You may get better results by switching to a valid mapper or changing the battery flag for this ROM.\n");
			mapper_warning = 1;
		}
#endif
	}
}

 READ8_HANDLER ( nes_mid_mapper_r )
{
	if ((nes.mid_ram_enable) || (nes.mapper == 5))
		return battery_ram[offset];
	else
		return 0;
}

WRITE8_HANDLER ( nes_mapper_w )
{
	if (mmc_write) (*mmc_write)(offset, data);
	else
	{
		logerror("Unimplemented mapper write, offset: %04x, data: %02x\n", offset, data);
#if 1
		if (! mapper_warning)
		{
			logerror("This game is writing to the mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}

		switch (offset)
		{
			/* Hacked mapper for the 24-in-1 NES_ROM. */
			/* It's really 35-in-1, and it's mostly different versions of Battle City. Unfortunately, the vrom dumps are bad */
			case 0x7fde:
				data &= (nes.prg_chunks - 1);
				memory_set_bankptr (3, &nes.rom[data * 0x4000 + 0x10000]);
				memory_set_bankptr (4, &nes.rom[data * 0x4000 + 0x12000]);
				break;
		}
#endif
	}
}

/*
 * Some helpful routines used by the mappers
 */

static void prg8_89 (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0x8000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr (1, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ab (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0xa000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr (2, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_cd (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0xc000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr (3, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ef (int bank)
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0xe000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr (4, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg16_89ab (int bank)
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0x8000], &nes.rom[bank * 0x4000 + 0x10000], 0x4000);
	else
	{
		memory_set_bankptr (1, &nes.rom[bank * 0x4000 + 0x10000]);
		memory_set_bankptr (2, &nes.rom[bank * 0x4000 + 0x12000]);
	}
}

static void prg16_cdef (int bank)
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0xc000], &nes.rom[bank * 0x4000 + 0x10000], 0x4000);
	else
	{
		memory_set_bankptr (3, &nes.rom[bank * 0x4000 + 0x10000]);
		memory_set_bankptr (4, &nes.rom[bank * 0x4000 + 0x12000]);
	}
}

static void prg32 (int bank)
{
	/* assumes that bank references a 32k chunk */
	bank &= ((nes.prg_chunks >> 1) - 1);
	if (nes.slow_banking)
		memcpy (&nes.rom[0x8000], &nes.rom[bank * 0x8000 + 0x10000], 0x8000);
	else
	{
		memory_set_bankptr (1, &nes.rom[bank * 0x8000 + 0x10000]);
		memory_set_bankptr (2, &nes.rom[bank * 0x8000 + 0x12000]);
		memory_set_bankptr (3, &nes.rom[bank * 0x8000 + 0x14000]);
		memory_set_bankptr (4, &nes.rom[bank * 0x8000 + 0x16000]);
	}
}

static void chr8 (int bank)
{
	bank &= (nes.chr_chunks - 1);
	ppu2c03b_set_videorom_bank(0, 0, 8, bank, 512);
}

static void chr4_0 (int bank)
{
	bank &= ((nes.chr_chunks << 1) - 1);
	ppu2c03b_set_videorom_bank(0, 0, 4, bank, 256);
}

static void chr4_4 (int bank)
{
	bank &= ((nes.chr_chunks << 1) - 1);
	ppu2c03b_set_videorom_bank(0, 4, 4, bank, 256);
}

static void chr2_0 (int bank)
{
	bank &= ((nes.chr_chunks << 2) - 1);
	ppu2c03b_set_videorom_bank(0, 0, 2, bank, 128);
}

static void chr2_2 (int bank)
{
	bank &= ((nes.chr_chunks << 2) - 1);
	ppu2c03b_set_videorom_bank(0, 2, 2, bank, 128);
}

static void chr2_4 (int bank)
{
	bank &= ((nes.chr_chunks << 2) - 1);
	ppu2c03b_set_videorom_bank(0, 4, 2, bank, 128);
}

static void chr2_6 (int bank)
{
	bank &= ((nes.chr_chunks << 2) - 1);
	ppu2c03b_set_videorom_bank(0, 6, 2, bank, 128);
}
static void chr1_0 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 0, 1, bank, 64);
}
static void chr1_1 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 1, 1, bank, 64);
}
static void chr1_2 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 2, 1, bank, 64);
}
static void chr1_3 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 3, 1, bank, 64);
}
static void chr1_4 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 4, 1, bank, 64);
}
static void chr1_5 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 5, 1, bank, 64);
}
static void chr1_6 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 6, 1, bank, 64);
}
static void chr1_7 (int bank)
{
	bank &= ((nes.chr_chunks << 3) - 1);
	ppu2c03b_set_videorom_bank(0, 7, 1, bank, 64);
}


static WRITE8_HANDLER( mapper1_w )
{
	int reg;

	/* Find which register we are dealing with */
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */
	reg = (offset >> 13);

	if (data & 0x80)
	{
		MMC1_reg_count = 0;
		MMC1_reg = 0;

		/* Set these to their defaults - needed for Robocop 3, Dynowars */
		MMC1_Size_16k = 1;
		MMC1_Switch_Low = 1;
		/* TODO: should we switch banks at this time also? */
#ifdef LOG_MMC
		logerror("=== MMC1 regs reset to default\n");
#endif
		return;
	}

	if (MMC1_reg_count < 5)
	{
		if (MMC1_reg_count == 0) MMC1_reg = 0;
		MMC1_reg >>= 1;
		MMC1_reg |= (data & 0x01) ? 0x10 : 0x00;
		MMC1_reg_count ++;
	}

	if (MMC1_reg_count == 5)
	{
//		logerror("   MMC1 reg#%02x val:%02x\n", offset, MMC1_reg);
		switch (reg)
		{
			case 0:
				MMC1_Switch_Low = MMC1_reg & 0x04;
				MMC1_Size_16k =   MMC1_reg & 0x08;

				switch (MMC1_reg & 0x03)
				{
					case 0: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
					case 1: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
					case 2: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
					case 3: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				}

				MMC1_SizeVrom_4k = (MMC1_reg & 0x10);
#ifdef LOG_MMC
				logerror("   MMC1 reg #1 val:%02x\n", MMC1_reg);
					logerror("\t\tBank Size: ");
					if (MMC1_Size_16k)
						logerror("16k\n");
					else logerror("32k\n");

					logerror("\t\tBank Select: ");
					if (MMC1_Switch_Low)
						logerror("$8000\n");
					else logerror("$C000\n");

					logerror("\t\tVROM Bankswitch Size Select: ");
					if (MMC1_SizeVrom_4k)
						logerror("4k\n");
					else logerror("8k\n");

					logerror ("\t\tMirroring: %d\n", MMC1_reg & 0x03);
#endif
				break;

			case 1:
				MMC1_extended_bank = (MMC1_extended_bank & ~0x01) | ((MMC1_reg & 0x10) >> 4);
				if (MMC1_extended == 2)
				{
					/* MMC1_SizeVrom_4k determines if we use the special 256k bank register */
				 	if (!MMC1_SizeVrom_4k)
				 	{
						/* Pick 1st or 4th 256k bank */
						MMC1_extended_base = 0xc0000 * (MMC1_extended_bank & 0x01) + 0x10000;
						memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
						logerror("MMC1_extended 1024k bank (no reg) select: %02x\n", MMC1_extended_bank);
#endif
					}
					else
					{
						/* Set 256k bank based on the 256k bank select register */
						if (MMC1_extended_swap)
						{
							MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
							memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
							logerror("MMC1_extended 1024k bank (reg 1) select: %02x\n", MMC1_extended_bank);
#endif
							MMC1_extended_swap = 0;
						}
						else MMC1_extended_swap = 1;
					}
				}
				else if (MMC1_extended == 1 && nes.chr_chunks == 0)
				{
					/* Pick 1st or 2nd 256k bank */
					MMC1_extended_base = 0x40000 * (MMC1_extended_bank & 0x01) + 0x10000;
					memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
					memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
					logerror("MMC1_extended 512k bank select: %02x\n", MMC1_extended_bank & 0x01);
#endif
				}
				else if (nes.chr_chunks > 0)
				{
//					logerror("MMC1_SizeVrom_4k: %02x bank:%02x\n", MMC1_SizeVrom_4k, MMC1_reg);

					if (!MMC1_SizeVrom_4k)
					{
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
						ppu2c03b_set_videorom_bank(0, 0, 8, bank, 256);
#ifdef LOG_MMC
						logerror("MMC1 8k VROM switch: %02x\n", MMC1_reg);
#endif
					}
					else
					{
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
						ppu2c03b_set_videorom_bank(0, 0, 4, bank, 256);
#ifdef LOG_MMC
						logerror("MMC1 4k VROM switch (low): %02x\n", MMC1_reg);
#endif
					}
				}
				break;
			case 2:
//				logerror("MMC1_Reg_2: %02x\n",MMC1_Reg_2);
				MMC1_extended_bank = (MMC1_extended_bank & ~0x02) | ((MMC1_reg & 0x10) >> 3);
				if (MMC1_extended == 2 && MMC1_SizeVrom_4k)
				{
					if (MMC1_extended_swap)
					{
						/* Set 256k bank based on the 256k bank select register */
						MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
						memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
#ifdef LOG_MMC
						logerror("MMC1_extended 1024k bank (reg 2) select: %02x\n", MMC1_extended_bank);
#endif
						MMC1_extended_swap = 0;
					}
					else
						MMC1_extended_swap = 1;
				}
				if (MMC1_SizeVrom_4k)
				{
					int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
					ppu2c03b_set_videorom_bank(0, 4, 4, bank, 256);
#ifdef LOG_MMC
					logerror("MMC1 4k VROM switch (high): %02x\n", MMC1_reg);
#endif
				}
				break;
			case 3:
				/* Switching 1 32k bank of PRG ROM */
				MMC1_reg &= 0x0f;
				if (!MMC1_Size_16k)
				{
					int bank = MMC1_reg & (nes.prg_chunks - 1);

					MMC1_bank1 = bank * 0x4000;
					MMC1_bank2 = bank * 0x4000 + 0x2000;
					memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					if (!MMC1_extended)
					{
						MMC1_bank3 = bank * 0x4000 + 0x4000;
						MMC1_bank4 = bank * 0x4000 + 0x6000;
						memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
					}
#ifdef LOG_MMC
					logerror("MMC1 32k bank select: %02x\n", MMC1_reg);
#endif
				}
				else
				/* Switching one 16k bank */
				{
					if (MMC1_Switch_Low)
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						MMC1_bank1 = bank * 0x4000;
						MMC1_bank2 = bank * 0x4000 + 0x2000;

						memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						if (!MMC1_extended)
						{
							MMC1_bank3 = MMC1_High;
							MMC1_bank4 = MMC1_High + 0x2000;
							memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
#ifdef LOG_MMC
						logerror("MMC1 16k-low bank select: %02x\n", MMC1_reg);
#endif
					}
					else
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						if (!MMC1_extended)
						{

							MMC1_bank1 = 0;
							MMC1_bank2 = 0x2000;
							MMC1_bank3 = bank * 0x4000;
							MMC1_bank4 = bank * 0x4000 + 0x2000;

							memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
#ifdef LOG_MMC
						logerror("MMC1 16k-high bank select: %02x\n", MMC1_reg);
#endif
					}
				}

#ifdef LOG_MMC
				logerror("-- page1: %06x\n", MMC1_bank1);
				logerror("-- page2: %06x\n", MMC1_bank2);
				logerror("-- page3: %06x\n", MMC1_bank3);
				logerror("-- page4: %06x\n", MMC1_bank4);
#endif
				break;
		}
		MMC1_reg_count = 0;
	}
}

static WRITE8_HANDLER( mapper2_w )
{
	prg16_89ab (data);
}

static WRITE8_HANDLER( mapper3_w )
{
	chr8 (data);
}

static void mapper4_set_prg (void)
{
	MMC3_prg0 &= prg_mask;
	MMC3_prg1 &= prg_mask;

	if (MMC3_cmd & 0x40)
	{
		memory_set_bankptr (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
		memory_set_bankptr (3, &nes.rom[0x2000 * (MMC3_prg0) + 0x10000]);
	}
	else
	{
		memory_set_bankptr (1, &nes.rom[0x2000 * (MMC3_prg0) + 0x10000]);
		memory_set_bankptr (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
	}
	memory_set_bankptr (2, &nes.rom[0x2000 * (MMC3_prg1) + 0x10000]);
}

static void mapper4_set_chr (void)
{
	UINT8 chr_page = (MMC3_cmd & 0x80) >> 5;
	ppu2c03b_set_videorom_bank(0, chr_page ^ 0, 2, MMC3_chr[0], 1);
	ppu2c03b_set_videorom_bank(0, chr_page ^ 2, 2, MMC3_chr[1], 1);
	ppu2c03b_set_videorom_bank(0, chr_page ^ 4, 1, MMC3_chr[2], 1);
	ppu2c03b_set_videorom_bank(0, chr_page ^ 5, 1, MMC3_chr[3], 1);
	ppu2c03b_set_videorom_bank(0, chr_page ^ 6, 1, MMC3_chr[4], 1);
	ppu2c03b_set_videorom_bank(0, chr_page ^ 7, 1, MMC3_chr[5], 1);
}

static void mapper4_irq ( int num, int scanline, int vblank, int blanked )
{
	if ((scanline < BOTTOM_VISIBLE_SCANLINE) || (scanline == ppu_scanlines_per_frame-1))
	{
//		if ((IRQ_enable) && (PPU_Control1 & 0x18))
		if ((IRQ_enable) && !blanked)
		{
			if (IRQ_count == 0)
			{
				IRQ_count = IRQ_count_latch;
				cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
			}
			IRQ_count --;
		}
	}
}

static WRITE8_HANDLER( mapper4_w )
{
	static UINT8 last_bank = 0xff;

//	logerror("mapper4_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	//only bits 14,13, and 0 matter for offset!
	switch (offset & 0x6001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg ();
				mapper4_set_chr ();
#ifdef LOG_MMC
				logerror("     MMC3 reset banks\n");
#endif
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					data &= 0xfe;
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
#ifdef LOG_MMC
					logerror("     MMC3 set vram %d: %d\n", cmd, data);
#endif
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
#ifdef LOG_MMC
					logerror("     MMC3 set vram %d: %d\n", cmd, data);
#endif
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg ();
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg ();
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
			if (data & 0x40)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
			else
			{
				if (data & 0x01)
					ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
				else
					ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
#ifdef LOG_MMC
			logerror("     MMC3 mid_ram enable: %02x\n", data);
#endif
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count: %02x\n", data);
#endif
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count latch: %02x\n", data);
#endif
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch; /* TODO: verify this */
#ifdef LOG_MMC
			logerror("     MMC3 disable irqs: %02x\n", data);
#endif
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
#ifdef LOG_MMC
			logerror("     MMC3 enable irqs: %02x\n", data);
#endif
			break;

		default:
			logerror("mapper4_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper118_w )
{
	static UINT8 last_bank = 0xff;

//	logerror("mapper4_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	//uses just bits 14, 13, and 0
	switch (offset & 0x6001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg ();
				mapper4_set_chr ();
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					data &= 0xfe;
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data * 64;
					mapper4_set_chr ();
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg ();
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg ();
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
#ifdef LOG_MMC
			logerror("     mapper 118 mirroring: %02x\n", data);
#endif
			switch (data & 0x02)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
#ifdef LOG_MMC
			logerror("     MMC3 mid_ram enable: %02x\n", data);
#endif
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count: %02x\n", data);
#endif
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
#ifdef LOG_MMC
			logerror("     MMC3 set irq count latch: %02x\n", data);
#endif
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch; /* TODO: verify this */
#ifdef LOG_MMC
			logerror("     MMC3 disable irqs: %02x\n", data);
#endif
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
#ifdef LOG_MMC
			logerror("     MMC3 enable irqs: %02x\n", data);
#endif
			break;

		default:
			logerror("mapper4_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void mapper5_irq ( int num, int scanline, int vblank, int blanked )
{
#if 1
	if (scanline == 0)
		IRQ_status |= 0x40;
	else if (scanline > BOTTOM_VISIBLE_SCANLINE)
		IRQ_status &= ~0x40;
#endif

	if (scanline == IRQ_count)
	{
		if (IRQ_enable)
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);

		IRQ_status = 0xff;
	}
}

static  READ8_HANDLER( mapper5_l_r )
{
	int retVal;

#ifdef MMC5_VRAM
	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		return MMC5_vram[offset - 0x1b00];
	}
#endif

	switch (offset)
	{
		case 0x1104: /* $5204 */
#if 0
			if (current_scanline == MMC5_scanline) return 0x80;
			else return 0x00;
#else
			retVal = IRQ_status;
			IRQ_status &= ~0x80;
			return retVal;
#endif
			break;

		case 0x1105: /* $5205 */
			return (mult1 * mult2) & 0xff;
			break;
		case 0x1106: /* $5206 */
			return ((mult1 * mult2) & 0xff00) >> 8;
			break;

		default:
			logerror("** MMC5 uncaught read, offset: %04x\n", offset + 0x4100);
			return 0x00;
			break;
	}
}

//static void mapper5_sync_vrom (int mode)
//{
//	int i;
//
//	for (i = 0; i < 8; i ++)
//		nes_vram[i] = vrom_bank[0 + (mode * 8)] * 64;
//}

static WRITE8_HANDLER( mapper5_l_w )
{
//	static int vrom_next[4];
	static int vrom_page_a;
	static int vrom_page_b;

//	logerror("Mapper 5 write, offset: %04x, data: %02x\n", offset + 0x4100, data);
	/* Send $5000-$5015 to the sound chip */
	if ((offset >= 0xf00) && (offset <= 0xf15))
	{
		NESPSG_0_w (offset & 0x1f, data);
		return;
	}

#ifdef MMC5_VRAM
	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		if (MMC5_vram_protect == 0x03)
			MMC5_vram[offset - 0x1b00] = data;
		return;
	}
#endif

	switch (offset)
	{
		case 0x1000: /* $5100 */
			MMC5_rom_bank_mode = data & 0x03;
			logerror ("MMC5 rom bank mode: %02x\n", data);
			break;

		case 0x1001: /* $5101 */
			MMC5_vrom_bank_mode = data & 0x03;
			logerror ("MMC5 vrom bank mode: %02x\n", data);
			break;

		case 0x1002: /* $5102 */
			if (data == 0x02)
				MMC5_vram_protect |= 1;
			else
				MMC5_vram_protect = 0;
			logerror ("MMC5 vram protect 1: %02x\n", data);
			break;
		case 0x1003: /* 5103 */
			if (data == 0x01)
				MMC5_vram_protect |= 2;
			else
				MMC5_vram_protect = 0;
			logerror ("MMC5 vram protect 2: %02x\n", data);
			break;

		case 0x1004: /* $5104 - Extra VRAM (EXRAM) control */
			MMC5_vram_control = data;
			logerror ("MMC5 exram control: %02x\n", data);
			break;

		case 0x1005: /* $5105 */
			ppu_mirror_custom (0, data & 0x03);
			ppu_mirror_custom (1, (data & 0x0c) >> 2);
			ppu_mirror_custom (2, (data & 0x30) >> 4);
			ppu_mirror_custom (3, (data & 0xc0) >> 6);
			break;

		/* $5106 - ?? cv3 sets to 0 */
		/* $5107 - ?? cv3 sets to 0 */

		case 0x1013: /* $5113 */
			logerror ("MMC5 mid RAM bank select: %02x\n", data & 0x07);
			memory_set_bankptr (5, &nes.wram[data * 0x2000]);
			/* The & 4 is a hack that'll tide us over for now */
			battery_ram = &nes.wram[(data & 4) * 0x2000];
			break;

		case 0x1014: /* $5114 */
			logerror ("MMC5 $5114 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						logerror ("\tROM bank select (8k, $8000): %02x\n", data);
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr (1, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $8000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr (1, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1015: /* $5115 */
			logerror ("MMC5 $5115 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x01:
				case 0x02:
					if (data & 0x80)
					{
						/* 16k switch - ROM only */
						prg16_89ab ((data & 0x7f) >> 1);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (16k, $8000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr (1, &nes.wram[((data & 4) >> 1) * 0x4000]);
						memory_set_bankptr (2, &nes.wram[((data & 4) >> 1) * 0x4000 + 0x2000]);
					}
					break;
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr (2, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $a000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr (2, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1016: /* $5116 */
			logerror ("MMC5 $5116 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x02:
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr (3, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						logerror ("\tRAM bank select (8k, $c000): %02x\n", data & 0x07);
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr (3, &nes.wram[(data & 4)* 0x2000]);
					}
					break;
			}
			break;
		case 0x1017: /* $5117 */
			logerror ("MMC5 $5117 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode);
			switch (MMC5_rom_bank_mode)
			{
				case 0x00:
					/* 32k switch - ROM only */
					prg32 (data >> 2);
					break;
				case 0x01:
					/* 16k switch - ROM only */
					prg16_cdef (data >> 1);
					break;
				case 0x02:
				case 0x03:
					/* 8k switch */
					data &= ((nes.prg_chunks << 1) - 1);
					memory_set_bankptr (4, &nes.rom[data * 0x2000 + 0x10000]);
					break;
			}
			break;
		case 0x1020: /* $5120 */
			logerror ("MMC5 $5120 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[0] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
//					nes_vram_sprite[0] = vrom_bank[0] * 64;
//					vrom_next[0] = 4;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1021: /* $5121 */
			logerror ("MMC5 $5121 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[1] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
//					nes_vram_sprite[1] = vrom_bank[0] * 64;
//					vrom_next[1] = 5;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1022: /* $5122 */
			logerror ("MMC5 $5122 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[2] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
//					nes_vram_sprite[2] = vrom_bank[0] * 64;
//					vrom_next[2] = 6;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1023: /* $5123 */
			logerror ("MMC5 $5123 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x01:
					chr4_0 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[3] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
//					nes_vram_sprite[3] = vrom_bank[0] * 64;
//					vrom_next[3] = 7;
//					vrom_page_a = 1;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1024: /* $5124 */
			logerror ("MMC5 $5124 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[4] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
//					nes_vram_sprite[4] = vrom_bank[0] * 64;
//					vrom_next[0] = 0;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1025: /* $5125 */
			logerror ("MMC5 $5125 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_4 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[5] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
//					nes_vram_sprite[5] = vrom_bank[0] * 64;
//					vrom_next[1] = 1;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1026: /* $5126 */
			logerror ("MMC5 $5126 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[6] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
//					nes_vram_sprite[6] = vrom_bank[0] * 64;
//					vrom_next[2] = 2;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1027: /* $5127 */
			logerror ("MMC5 $5127 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					chr8 (data);
					break;
				case 0x01:
					/* 4k switch */
					chr4_4 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_6 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[7] = data;
//					mapper5_sync_vrom(0);
					ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
//					nes_vram_sprite[7] = vrom_bank[0] * 64;
//					vrom_next[3] = 3;
//					vrom_page_a = 0;
//					vrom_page_b = 0;
					break;
			}
			break;
		case 0x1028: /* $5128 */
			logerror ("MMC5 $5128 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[8] = vrom_bank[12] = data;
//					nes_vram[vrom_next[0]] = data * 64;
//					nes_vram[0 + (vrom_page_a*4)] = data * 64;
//					nes_vram[0] = data * 64;
					ppu2c03b_set_videorom_bank(0, 4, 1, data, 64);
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x1029: /* $5129 */
			logerror ("MMC5 $5129 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (data);
					chr2_4 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[9] = vrom_bank[13] = data;
//					nes_vram[vrom_next[1]] = data * 64;
//					nes_vram[1 + (vrom_page_a*4)] = data * 64;
//					nes_vram[1] = data * 64;
					ppu2c03b_set_videorom_bank(0, 5, 1, data, 64);
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102a: /* $512a */
			logerror ("MMC5 $512a vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[10] = vrom_bank[14] = data;
//					nes_vram[vrom_next[2]] = data * 64;
//					nes_vram[2 + (vrom_page_a*4)] = data * 64;
//					nes_vram[2] = data * 64;
					ppu2c03b_set_videorom_bank(0, 6, 1, data, 64);
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102b: /* $512b */
			logerror ("MMC5 $512b vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode);
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					chr8 (data);
					break;
				case 0x01:
					/* 4k switch */
					chr4_0 (data);
					chr4_4 (data);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (data);
					chr2_6 (data);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[11] = vrom_bank[15] = data;
//					nes_vram[vrom_next[3]] = data * 64;
//					nes_vram[3 + (vrom_page_a*4)] = data * 64;
//					nes_vram[3] = data * 64;
					ppu2c03b_set_videorom_bank(0, 7, 1, data, 64);
//					mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;

		case 0x1103: /* $5203 */
			IRQ_count = data;
			MMC5_scanline = data;
			logerror ("MMC5 irq scanline: %d\n", IRQ_count);
			break;
		case 0x1104: /* $5204 */
			IRQ_enable = data & 0x80;
			logerror ("MMC5 irq enable: %02x\n", data);
			break;
		case 0x1105: /* $5205 */
			mult1 = data;
			break;
		case 0x1106: /* $5206 */
			mult2 = data;
			break;

		default:
			logerror("** MMC5 uncaught write, offset: %04x, data: %02x\n", offset + 0x4100, data);
			break;
	}
}

static WRITE8_HANDLER( mapper5_w )
{
	logerror("MMC5 uncaught high mapper w, %04x: %02x\n", offset, data);
}

static WRITE8_HANDLER( mapper7_w )
{
	if (data & 0x10)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);

	prg32 (data);
}

static WRITE8_HANDLER( mapper8_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 8 switch, vrom: %02x, rom: %02x\n", data & 0x07, (data >> 3));
#endif
	/* Switch 8k VROM bank */
	chr8 (data & 0x07);

	/* Switch 16k PRG bank */
	data = (data >> 3) & (nes.prg_chunks - 1);
	memory_set_bankptr (1, &nes.rom[data * 0x4000 + 0x10000]);
	memory_set_bankptr (2, &nes.rom[data * 0x4000 + 0x12000]);
}


static void mapper9_latch (offs_t offset)
{
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		logerror ("mapper9 vrom latch switch (bank 0 low): %02x\n", MMC2_bank0);
		MMC2_bank0_latch = 0xfd;
		chr4_0 (MMC2_bank0);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		logerror ("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_bank0_hi);
		MMC2_bank0_latch = 0xfe;
		chr4_0 (MMC2_bank0_hi);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		logerror ("mapper9 vrom latch switch (bank 1 low): %02x\n", MMC2_bank1);
		MMC2_bank1_latch = 0xfd;
		chr4_4 (MMC2_bank1);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		logerror ("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_bank1_hi);
		MMC2_bank1_latch = 0xfe;
		chr4_4 (MMC2_bank1_hi);
	}
}

static WRITE8_HANDLER( mapper9_w )
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 8k prg bank */
			prg8_89(data);
			break;
		case 0x3000:
			MMC2_bank0 = data;
			if (MMC2_bank0_latch == 0xfd)
				chr4_0 (MMC2_bank0);
			logerror("MMC2 VROM switch #1 (low): %02x\n", MMC2_bank0);
			break;
		case 0x4000:
			MMC2_bank0_hi = data;
			if (MMC2_bank0_latch == 0xfe)
				chr4_0 (MMC2_bank0_hi);
			logerror("MMC2 VROM switch #1 (high): %02x\n", MMC2_bank0_hi);
			break;
		case 0x5000:
			MMC2_bank1 = data;
			if (MMC2_bank1_latch == 0xfd)
				chr4_4 (MMC2_bank1);
			logerror("MMC2 VROM switch #2 (low): %02x\n", data);
			break;
		case 0x6000:
			MMC2_bank1_hi = data;
			if (MMC2_bank1_latch == 0xfe)
				chr4_4 (MMC2_bank1_hi);
			logerror("MMC2 VROM switch #2 (high): %02x\n", data);
			break;
		case 0x7000:
			if (data&1)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		default:
			logerror("MMC2 uncaught w: %04x:%02x\n", offset, data);
			break;
	}
}


static void mapper10_latch (offs_t offset)
{
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 low): %02x\n", MMC2_bank0);
		MMC2_bank0_latch = 0xfd;
		chr4_0 (MMC2_bank0);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 high): %02x\n", MMC2_bank0_hi);
		MMC2_bank0_latch = 0xfe;
		chr4_0 (MMC2_bank0_hi);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		logerror ("mapper10 vrom latch switch (bank 1 low): %02x\n", MMC2_bank1);
		MMC2_bank1_latch = 0xfd;
		chr4_4 (MMC2_bank1);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		logerror ("mapper10 vrom latch switch (bank 0 high): %02x\n", MMC2_bank1_hi);
		MMC2_bank1_latch = 0xfe;
		chr4_4 (MMC2_bank1_hi);
	}
}

static WRITE8_HANDLER( mapper10_w )
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 16k prg bank */
			prg16_89ab(data);
			break;
		case 0x3000:
			MMC2_bank0 = data;
			if (MMC2_bank0_latch == 0xfd)
				chr4_0 (MMC2_bank0);
			logerror("MMC4 VROM switch #1 (low): %02x\n", MMC2_bank0);
			break;
		case 0x4000:
			MMC2_bank0_hi = data;
			if (MMC2_bank0_latch == 0xfe)
				chr4_0 (MMC2_bank0_hi);
			logerror("MMC4 VROM switch #1 (high): %02x\n", MMC2_bank0_hi);
			break;
		case 0x5000:
			MMC2_bank1 = data;
			if (MMC2_bank1_latch == 0xfd)
				chr4_4 (MMC2_bank1);
			logerror("MMC4 VROM switch #2 (low): %02x\n", data);
			break;
		case 0x6000:
			MMC2_bank1_hi = data;
			if (MMC2_bank1_latch == 0xfe)
				chr4_4 (MMC2_bank1_hi);
			logerror("MMC4 VROM switch #2 (high): %02x\n", data);
			break;
		case 0x7000:
			if (data&1)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		default:
			logerror("MMC4 uncaught w: %04x:%02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( mapper11_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 11 switch, data: %02x\n", data);
#endif
	/* Switch 8k VROM bank */
	chr8 (data >> 4);

	/* Switch 32k prg bank */
	prg32 (data & 0x0f);
}
static WRITE8_HANDLER( mapper13_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 13 switch, data: %02x\n", data );
#endif
	//this is a place-holder call.
	//mapper doesn't work because CHR-RAM is not mapped.
	chr4_4 (data);
}

static WRITE8_HANDLER( mapper15_w )
{
	int bank = (data & (nes.prg_chunks - 1)) * 0x4000;
	int base = data & 0x80 ? 0x12000 : 0x10000;

	switch (offset)
	{
		case 0x0000:
			if (data & 0x40)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			memory_set_bankptr (1, &nes.rom[bank + base]);
			memory_set_bankptr (2, &nes.rom[bank + (base ^ 0x2000)]);
			memory_set_bankptr (3, &nes.rom[bank + (base ^ 0x4000)]);
			memory_set_bankptr (4, &nes.rom[bank + (base ^ 0x6000)]);
			break;

		case 0x0001:
			memory_set_bankptr (3, &nes.rom[bank + base]);
			memory_set_bankptr (4, &nes.rom[bank + (base ^ 0x2000)]);
			break;

		case 0x0002:
			memory_set_bankptr (1, &nes.rom[bank + base]);
			memory_set_bankptr (2, &nes.rom[bank + base]);
			memory_set_bankptr (3, &nes.rom[bank + base]);
			memory_set_bankptr (4, &nes.rom[bank + base]);
        	break;

		case 0x0003:
			if (data & 0x40)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			memory_set_bankptr (3, &nes.rom[bank + base]);
			memory_set_bankptr (4, &nes.rom[bank + (base ^ 0x2000)]);
        	break;
	}
}

static void bandai_irq ( int num, int scanline, int vblank, int blanked )
{
	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
		}
		IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper16_w )
{
	logerror ("mapper16 (mid and high) w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x000f)
	{
		//use new utility functions for bounds checks!
		case 0:
			chr1_0(data);
			break;
		case 1:
			chr1_1(data);
			break;
		case 2:
			chr1_2(data);
			break;
		case 3:
			chr1_3(data);
			break;
		case 4:
			chr1_4(data);
			break;
		case 5:
			chr1_5(data);
			break;
		case 6:
			chr1_6(data);
			break;
		case 7:
			chr1_7(data);
			break;
		case 8:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;
		case 9:
			switch (data & 0x03) {
			case 0:
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
				break;
			case 1:
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
				break;
			case 2:
				ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);
				break;
			case 3:
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
				break;
			}
			break;
		case 0x0a:
			IRQ_enable = data & 0x01;
			break;
		case 0x0b:
			IRQ_count &= 0xff00;
			IRQ_count |= data;
			break;
		case 0x0c:
			IRQ_count &= 0x00ff;
			IRQ_count |= (data << 8);
			break;
		default:
			logerror("** uncaught mapper 16 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( mapper17_l_w )
{
	switch (offset)
	{
		/* $42fe - mirroring */
		case 0x1fe:
			if (data & 0x10)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
			break;
		/* $42ff - mirroring */
		case 0x1ff:
			if (data & 0x10)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		/* $4501 - $4503 */
//		case 0x401:
//		case 0x402:
//		case 0x403:
			/* IRQ control */
//			break;
		/* $4504 - $4507 : 8k PRG-Rom switch */
		case 0x404:
		case 0x405:
		case 0x406:
		case 0x407:
			data &= ((nes.prg_chunks << 1) - 1);
//			logerror("Mapper 17 bank switch, bank: %02x, data: %02x\n", offset & 0x03, data);
			memory_set_bankptr ((offset & 0x03) + 1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		/* $4510 - $4517 : 1k CHR-Rom switch */
		case 0x410:
		case 0x411:
		case 0x412:
		case 0x413:
		case 0x414:
		case 0x415:
		case 0x416:
		case 0x417:
			data &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, offset & 0x07, 1, data, 64);
			break;
		default:
			logerror("** uncaught mapper 17 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

static void jaleco_irq ( int num, int scanline, int vblank, int blanked )
{
	if (scanline <= BOTTOM_VISIBLE_SCANLINE)
	{
		/* Increment & check the IRQ scanline counter */
		if (IRQ_enable)
		{
			IRQ_count -= 0x100;

			logerror ("scanline: %d, irq count: %04x\n", scanline, IRQ_count);
			if (IRQ_mode_jaleco & 0x08)
			{
				if ((IRQ_count & 0x0f) == 0x00)
					/* rollover every 0x10 */
					cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_mode_jaleco & 0x04)
			{
				if ((IRQ_count & 0x0ff) == 0x00)
					/* rollover every 0x100 */
					cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_mode_jaleco & 0x02)
			{
				if ((IRQ_count & 0x0fff) == 0x000)
					/* rollover every 0x1000 */
					cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_count == 0)
				/* rollover at 0x10000 */
				cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
		}
	}
	else
	{
		IRQ_count = IRQ_count_latch;
	}
}


static WRITE8_HANDLER( mapper18_w )
{
//	static int irq;
	static int bank_8000 = 0;
	static int bank_a000 = 0;
	static int bank_c000 = 0;

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 - low 4 bits */
			bank_8000 = (bank_8000 & 0xf0) | (data & 0x0f);
			bank_8000 &= prg_mask;
			memory_set_bankptr (1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $8000 - high 4 bits */
			bank_8000 = (bank_8000 & 0x0f) | (data << 4);
			bank_8000 &= prg_mask;
			memory_set_bankptr (1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0002:
			/* Switch 8k bank at $a000 - low 4 bits */
			bank_a000 = (bank_a000 & 0xf0) | (data & 0x0f);
			bank_a000 &= prg_mask;
			memory_set_bankptr (2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x0003:
			/* Switch 8k bank at $a000 - high 4 bits */
			bank_a000 = (bank_a000 & 0x0f) | (data << 4);
			bank_a000 &= prg_mask;
			memory_set_bankptr (2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x1000:
			/* Switch 8k bank at $c000 - low 4 bits */
			bank_c000 = (bank_c000 & 0xf0) | (data & 0x0f);
			bank_c000 &= prg_mask;
			memory_set_bankptr (3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;
		case 0x1001:
			/* Switch 8k bank at $c000 - high 4 bits */
			bank_c000 = (bank_c000 & 0x0f) | (data << 4);
			bank_c000 &= prg_mask;
			memory_set_bankptr (3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;

		/* $9002, 3 (1002, 3) uncaught = Jaleco Baseball writes 0 */
		/* believe it's related to battery-backed ram enable/disable */

		case 0x2000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x2001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x2002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x2003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x3001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x3003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x4000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x4001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x4002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x4003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x5001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;
		case 0x5003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;

/* LBO - these are unverified */

		case 0x6000: /* IRQ scanline counter - low byte, low nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xfff0) | (data & 0x0f);
			logerror("     Mapper 18 copy/set irq latch (l-l): %02x\n", data);
			break;
		case 0x6001: /* IRQ scanline counter - low byte, high nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xff0f) | ((data & 0x0f) << 4);
			logerror("     Mapper 18 copy/set irq latch (l-h): %02x\n", data);
			break;
		case 0x6002:
			IRQ_count_latch = (IRQ_count_latch & 0xf0ff) | ((data & 0x0f) << 9);
			logerror("     Mapper 18 copy/set irq latch (h-l): %02x\n", data);
			break;
		case 0x6003:
			IRQ_count_latch = (IRQ_count_latch & 0x0fff) | ((data & 0x0f) << 13);
			logerror("     Mapper 18 copy/set irq latch (h-h): %02x\n", data);
			break;

/* LBO - these 2 are likely wrong */

		case 0x7000: /* IRQ Control 0 */
			if (data & 0x01)
			{
//				IRQ_enable = 1;
				IRQ_count = IRQ_count_latch;
			}
			logerror("     Mapper 18 IRQ Control 0: %02x\n", data);
			break;
		case 0x7001: /* IRQ Control 1 */
			IRQ_enable = data & 0x01;
			IRQ_mode_jaleco = data & 0x0e;
			logerror("     Mapper 18 IRQ Control 1: %02x\n", data);
			break;

		case 0x7002: /* Misc */
			switch (data & 0x03)
			{
				case 0: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 1: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
				case 2: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 3: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
			}
			break;

/* $f003 uncaught, writes 0(start) & various(ingame) */
//		case 0x7003:
//			IRQ_count = data;
//			break;

		default:
			logerror("Mapper 18 uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void namcot_irq ( int num, int scanline, int vblank, int blanked )
{
	IRQ_count ++;
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (IRQ_count == 0x7fff))
	{
		cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( mapper19_l_w )
{
	switch (offset & 0x1800)
	{
		case 0x1000:
			/* low byte of IRQ */
			IRQ_count = (IRQ_count & 0x7f00) | data;
			break;
		case 0x1800:
			/* high byte of IRQ, IRQ enable in high bit */
			IRQ_count = (IRQ_count & 0xff) | ((data & 0x7f) << 8);
			IRQ_enable = data & 0x80;
			break;
	}
}

static WRITE8_HANDLER( mapper19_w )
{
	switch (offset & 0x7800)
	{
		case 0x0000:
		case 0x0800:
		case 0x1000:
		case 0x1800:
		case 0x2000:
		case 0x2800:
		case 0x3000:
		case 0x3800:
			/* Switch 1k VROM at $0000-$1c00 */
			ppu2c03b_set_videorom_bank(0, offset / 0x800, 1, data, 64);
			break;

		case 0x6000:
			/* Switch 8k bank at $8000 */
			data &= prg_mask;
			memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x6800:
			/* Switch 8k bank at $a000 */
			data &= prg_mask;
			memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x7000:
			/* Switch 8k bank at $c000 */
			data &= prg_mask;
			memory_set_bankptr (3, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
	}
}

static void fds_irq ( int num, int scanline, int vblank, int blanked )
{
	if (IRQ_enable_latch)
		cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);

	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
			IRQ_enable = 0;
			nes_fds.status0 |= 0x01;
		}
		else
			IRQ_count -= 114;
	}
}

 READ8_HANDLER ( fds_r )
{
	int ret = 0x00;
	static int last_side = 0;
	static int count = 0;

	switch (offset)
	{
		case 0x00: /* $4030 - disk status 0 */
			ret = nes_fds.status0;
			/* clear the disk IRQ detect flag */
			nes_fds.status0 &= ~0x01;
			break;
		case 0x01: /* $4031 - data latch */
			if (nes_fds.current_side)
				ret = nes_fds.data[(nes_fds.current_side-1) * 65500 + nes_fds.head_position++];
			else
				ret = 0;
			break;
		case 0x02: /* $4032 - disk status 1 */
			/* If we've switched disks, report "no disk" for a few reads */
			if (last_side != nes_fds.current_side)
			{
				ret = 1;
				count ++;
				if (count == 50)
				{
					last_side = nes_fds.current_side;
					count = 0;
				}
			}
			else
				ret = (nes_fds.current_side == 0); /* 0 if a disk is inserted */
			break;
		case 0x03: /* $4033 */
			ret = 0x80;
			break;
		default:
			ret = 0x00;
			break;
	}

#ifdef LOG_FDS
	logerror ("fds_r, address: %04x, data: %02x\n", offset + 0x4030, ret);
#endif

	return ret;
}

WRITE8_HANDLER ( fds_w )
{
	switch (offset)
	{
		case 0x00: /* $4020 */
			IRQ_count_latch &= ~0xff;
			IRQ_count_latch |= data;
			break;
		case 0x01: /* $4021 */
			IRQ_count_latch &= ~0xff00;
			IRQ_count_latch |= (data << 8);
			break;
		case 0x02: /* $4022 */
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data;
			break;
		case 0x03: /* $4023 */
			// d0 = sound io (1 = enable)
			// d1 = disk io (1 = enable)
			break;
		case 0x04: /* $4024 */
			/* write data out to disk */
			break;
		case 0x05: /* $4025 */
			nes_fds.motor_on = data & 0x01;
			if (data & 0x02) nes_fds.head_position = 0;
			nes_fds.read_mode = data & 0x04;
			if (data & 0x08)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			if ((!(data & 0x40)) && (nes_fds.write_reg & 0x40))
				nes_fds.head_position -= 2; // ???
			IRQ_enable_latch = data & 0x80;

			nes_fds.write_reg = data;
			break;
	}

#ifdef LOG_FDS
	logerror ("fds_w, address: %04x, data: %02x\n", offset + 0x4020, data);
#endif

}

static void konami_irq ( int num, int scanline, int vblank, int blanked )
{
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (++IRQ_count == 0x100))
	{
		IRQ_count = IRQ_count_latch;
		IRQ_enable = IRQ_enable_latch;
		cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( konami_vrc2a_w )
{
	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				/* Switch 8k bank at $8000 */
				data &= ((nes.prg_chunks << 1) - 1);
				memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
					case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
					case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
					case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
				}
				break;

			case 0x2000:
				/* Switch 8k bank at $a000 */
				data &= ((nes.prg_chunks << 1) - 1);
				memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			default:
				logerror("konami_vrc2a_w uncaught offset: %04x value: %02x\n", offset, data);
				break;
		}
		return;
	}

	switch (offset & 0x7000)
	{
		case 0x3000:
		case 0x3001:
		case 0x4000:
		case 0x4001:
		case 0x5000:
		case 0x5001:
		case 0x6000:
		case 0x6001:
			/* switch 1k vrom at $0000-$1c00 */
			{
				int bank = (offset & 0x7000) / 0x0800 + (offset & 0x0001);
				vrom_bank[bank] = data >> 1;
				vrom_bank[bank] &= ((nes.chr_chunks << 3) - 1);
				ppu2c03b_set_videorom_bank(0, bank, 1, vrom_bank[bank], 64);
			}
			break;

		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc2a_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( konami_vrc2b_w )
{
	UINT16 select;

//	logerror("konami_vrc2b_w offset: %04x value: %02x\n", offset, data);

	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				/* Switch 8k bank at $8000 */
				data &= ((nes.prg_chunks << 1) - 1);
				memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
					case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
					case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
					case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
				}
				break;

			case 0x2000:
				/* Switch 8k bank at $a000 */
				data &= ((nes.prg_chunks << 1) - 1);
				memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
				break;
			default:
				logerror("konami_vrc2b_w offset: %04x value: %02x\n", offset, data);
				break;
		}
		return;
	}

	/* The low 2 select bits vary from cart to cart */
	select = (offset & 0x7000) |
		(offset & 0x03) |
		((offset & 0x0c) >> 2);

	switch (select)
	{
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x3001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x3003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x4001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x4002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x4003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x5001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x5003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x6001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x6002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;
		case 0x6003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc2b_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( konami_vrc4_w )
{
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((nes.prg_chunks << 1) - 1);
			memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x1000:
			switch (data & 0x03)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
			}
			break;

		/* $1001 is uncaught */

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= ((nes.prg_chunks << 1) - 1);
			memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 0, 1, vrom_bank[0], 64);
			break;
		case 0x3001:
		case 0x3004:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x3003:
		case 0x3006:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 1, 1, vrom_bank[1], 64);
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x4002:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 2, 1, vrom_bank[2], 64);
			break;
		case 0x4001:
		case 0x4004:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x4003:
		case 0x4006:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 3, 1, vrom_bank[3], 64);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 4, 1, vrom_bank[4], 64);
			break;
		case 0x5001:
		case 0x5004:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x5003:
		case 0x5006:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 5, 1, vrom_bank[5], 64);
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x6002:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 6, 1, vrom_bank[6], 64);
			break;
		case 0x6001:
		case 0x6004:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;
		case 0x6003:
		case 0x6006:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			ppu2c03b_set_videorom_bank(0, 7, 1, vrom_bank[7], 64);
			break;
		case 0x7000:
			/* Set low 4 bits of latch */
			IRQ_count_latch &= ~0x0f;
			IRQ_count_latch |= data & 0x0f;
//			logerror("konami_vrc4 irq_latch low: %02x\n", IRQ_count_latch);
			break;
		case 0x7002:
		case 0x7040:
			/* Set high 4 bits of latch */
			IRQ_count_latch &= ~0xf0;
			IRQ_count_latch |= (data << 4) & 0xf0;
//			logerror("konami_vrc4 irq_latch high: %02x\n", IRQ_count_latch);
			break;
		case 0x7004:
		case 0x7001:
		case 0x7080:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
//			logerror("konami_vrc4 irq_count set: %02x\n", IRQ_count);
//			logerror("konami_vrc4 enable: %02x\n", IRQ_enable);
//			logerror("konami_vrc4 enable latch: %02x\n", IRQ_enable_latch);
			break;
		case 0x7006:
		case 0x7003:
		case 0x70c0:
			IRQ_enable = IRQ_enable_latch;
//			logerror("konami_vrc4 enable copy: %02x\n", IRQ_enable);
			break;
		default:
			logerror("konami_vrc4_w uncaught offset: %04x value: %02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( konami_vrc6a_w )
{
//	logerror("konami_vrc6_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 0x04: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				case 0x08: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x0c: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;
		case 0x5000:
		case 0x5001:
		case 0x5002:
		case 0x5003:
			/* switch 1k VROM at $0000-$0c00 */
			ppu2c03b_set_videorom_bank(0, (offset & 3) + 0, 1, data, 64);
			break;

		case 0x6000:
		case 0x6001:
		case 0x6002:
		case 0x6003:
			/* switch 1k VROM at $1000-$1c00 */
			ppu2c03b_set_videorom_bank(0, (offset & 3) + 4, 1, data, 64);
			break;

		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7002:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}


static WRITE8_HANDLER( konami_vrc6b_w )
{
//	logerror("konami_vrc6_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			/* Switch 16k bank at $8000 */
			prg16_89ab (data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 0x04: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				case 0x08: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x0c: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;
		case 0x5000:
			/* Switch 1k VROM at $0000 */
			ppu2c03b_set_videorom_bank(0, 0, 1, data, 64);
			break;
		case 0x5002:
			/* Switch 1k VROM at $0400 */
			ppu2c03b_set_videorom_bank(0, 1, 1, data, 64);
			break;
		case 0x5001:
			/* Switch 1k VROM at $0800 */
			ppu2c03b_set_videorom_bank(0, 2, 1, data, 64);
			break;
		case 0x5003:
			/* Switch 1k VROM at $0c00 */
			ppu2c03b_set_videorom_bank(0, 3, 1, data, 64);
			break;
		case 0x6000:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, 4, 1, data, 64);
			break;
		case 0x6002:
			/* Switch 1k VROM at $1400 */
			ppu2c03b_set_videorom_bank(0, 5, 1, data, 64);
			break;
		case 0x6001:
			/* Switch 1k VROM at $1800 */
			ppu2c03b_set_videorom_bank(0, 6, 1, data, 64);
			break;
		case 0x6003:
			/* Switch 1k VROM at $1c00 */
			ppu2c03b_set_videorom_bank(0, 7, 1, data, 64);
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper32_w )
{
	static int bankSel;

//	logerror("mapper32_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 or $c000 */
			if (bankSel)
				prg8_cd (data);
			else
				prg8_89 (data);
			break;
		case 0x1000:
			bankSel = data & 0x02;
			if (data & 0x01)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		case 0x2000:
			/* Switch 8k bank at $A000 */
			prg8_ab (data);
			break;
		case 0x3000:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, offset & 0x07, 1, data, 64);
			break;
		default:
			logerror("Uncaught mapper 32 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper33_w )
{

//	logerror("mapper33_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= ((nes.prg_chunks << 1) - 1);
			memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $A000 */
			data &= ((nes.prg_chunks << 1) - 1);
			memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		case 0x0002:
			/* Switch 2k VROM at $0000 */
			ppu2c03b_set_videorom_bank(0, 0, 2, data, 128);
			break;
		case 0x0003:
			/* Switch 2k VROM at $0800 */
			ppu2c03b_set_videorom_bank(0, 2, 2, data, 128);
			break;
		case 0x2000:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, 4, 1, data, 64);
			break;
		case 0x2001:
			/* Switch 1k VROM at $1400 */
			ppu2c03b_set_videorom_bank(0, 5, 1, data, 64);
			break;
		case 0x2002:
			/* Switch 1k VROM at $1800 */
			ppu2c03b_set_videorom_bank(0, 6, 1, data, 64);
			break;
		case 0x2003:
			/* Switch 1k VROM at $1c00 */
			ppu2c03b_set_videorom_bank(0, 7, 1, data, 64);
			break;
		default:
			logerror("Uncaught mapper 33 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper34_m_w )
{
	logerror("mapper34_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ffd:
			/* Switch 32k prg banks */
			prg32 (data);
			break;
		case 0x1ffe:
			/* Switch 4k VNES_ROM at 0x0000 */
			chr4_0 (data);
			break;
		case 0x1fff:
			/* Switch 4k VNES_ROM at 0x1000 */
			chr4_4 (data);
			break;
	}
}


static WRITE8_HANDLER( mapper34_w )
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a Mapper 34 game - the demo screens look wrong using mapper 7. */
	logerror("Mapper 34 w, offset: %04x, data: %02x\n", offset, data);

	prg32 (data);
}

static void mapper40_irq ( int num, int scanline, int vblank, int blanked )
{
	/* Decrement & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
		{
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

static WRITE8_HANDLER( mapper40_w )
{
	logerror("mapper40_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x6000)
	{
		case 0x0000:
			IRQ_enable = 0;
			IRQ_count = 37; /* Hardcoded scanline scroll */
			break;
		case 0x2000:
			IRQ_enable = 1;
			break;
		case 0x6000:
			/* Game runs code between banks, use slow but sure method */
			prg8_cd (data);
			break;
	}
}

static WRITE8_HANDLER( mapper41_m_w )
{
#ifdef LOG_MMC
	logerror("mapper41_m_w, offset: %04x, data: %02x\n", offset, data);
#endif
	if (offset & 0x20)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);

	mapper41_reg2 = offset & 0x04;
	mapper41_chr &= ~0x0c;
	mapper41_chr |= (offset & 0x18) >> 1;
	prg32 (offset & 0x07);
}

static WRITE8_HANDLER( mapper41_w )
{
#ifdef LOG_MMC
	logerror("mapper41_w, offset: %04x, data: %02x\n", offset, data);
#endif

	if (mapper41_reg2)
	{
		mapper41_chr &= ~0x03;
		mapper41_chr |= data & 0x03;
		chr8(mapper41_chr);
	}
}

static WRITE8_HANDLER( mapper64_m_w )
{
	logerror("mapper64_m_w, offset: %04x, data: %02x\n", offset, data);
}

static WRITE8_HANDLER( mapper64_w )
{
	static int cmd = 0;
	static int chr = 0;
	static int select_high;
	static int page;

/* TODO: something in the IRQ handling hoses Skull & Crossbones */

//	logerror("mapper64_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7001)
	{
		case 0x0000:
//			logerror("Mapper 64 0x8000 write value: %02x\n",data);
			cmd = data & 0x0f;
			if (data & 0x80)
				chr = 0x1000;
			else
				chr = 0x0000;

			if (data & 0x10)
			{
				ppu2c03b_set_videorom_bank(0, 1, 1, 0, 64);
				ppu2c03b_set_videorom_bank(0, 3, 1, 0, 64);
			}

			page = chr >> 10;
			/* Toggle switching between $8000/$A000/$C000 and $A000/$C000/$8000 */
			if (select_high != (data & 0x40))
			{
				if (data & 0x40)
				{
					memory_set_bankptr (1, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
				}
				else
				{
					memory_set_bankptr (3, &nes.rom[(nes.prg_chunks-1) * 0x4000 + 0x10000]);
				}
			}

			select_high = data & 0x40;
//			logerror("   Mapper 64 select_high: %02x\n", select_high);
			break;

		case 0x0001:
			switch (cmd)
			{
				case 0:
					ppu2c03b_set_videorom_bank(0, page, 2, data, 64);
					break;

				case 1:
					ppu2c03b_set_videorom_bank(0, page ^ 2, 2, data, 64);
					break;

				case 2:
					ppu2c03b_set_videorom_bank(0, page ^ 4, 2, data, 64);
					break;

				case 3:
					ppu2c03b_set_videorom_bank(0, page ^ 5, 2, data, 64);
					break;

				case 4:
					ppu2c03b_set_videorom_bank(0, page ^ 6, 2, data, 64);
					break;

				case 5:
					ppu2c03b_set_videorom_bank(0, page ^ 7, 2, data, 64);
					break;

				case 6:
					/* These damn games will go to great lengths to switch to banks which are outside the valid range */
					data &= prg_mask;
					if (select_high)
					{
						memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 6 value: %02x\n", data);
					}
					else
					{
						memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($8000) cmd 6 value: %02x\n", data);
					}
					break;
				case 7:
					data &= prg_mask;
					if (select_high)
					{
						memory_set_bankptr (3, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($C000) cmd 7 value: %02x\n", data);
					}
					else
					{
						memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 7 value: %02x\n", data);
					}
					break;
				case 8:
					/* Switch 1k VROM at $0400 */
					ppu2c03b_set_videorom_bank(0, 1, 1, data, 64);
					break;
				case 9:
					/* Switch 1k VROM at $0C00 */
					ppu2c03b_set_videorom_bank(0, 3, 1, data, 64);
					break;
				case 15:
					data &= prg_mask;
					if (select_high)
					{
						memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($C000) cmd 15 value: %02x\n", data);
					}
					else
					{
						memory_set_bankptr (3, &nes.rom[0x2000 * (data) + 0x10000]);
//						logerror("     Mapper 64 switch ($A000) cmd 15 value: %02x\n", data);
					}
					break;
			}
			cmd = 16;
			break;
		case 0x2000:
			/* Not sure if the one-screen mirroring applies to this mapper */
			if (data & 0x40)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
			else
			{
				if (data & 0x01)
					ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
				else
					ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			}
			break;
		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
			logerror("     MMC3 copy/set irq latch: %02x\n", data);
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
			logerror("     MMC3 set latch: %02x\n", data);
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			logerror("     MMC3 disable irqs: %02x\n", data);
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			logerror("     MMC3 enable irqs: %02x\n", data);
			break;

		default:
			logerror("Mapper 64 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static void irem_irq ( int num, int scanline, int vblank, int blanked )
{
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
	}
}



static WRITE8_HANDLER( mapper65_w )
{
	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= prg_mask;
			memory_set_bankptr (1, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($8000) value: %02x\n", data);
#endif
			break;
		case 0x1001:
			if (data & 0x80)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		case 0x1005:
			IRQ_count = data << 1;
			break;
		case 0x1006:
			IRQ_enable = IRQ_count;
			break;

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= prg_mask;
			memory_set_bankptr (2, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($a000) value: %02x\n", data);
#endif
			break;

		case 0x3000:
		case 0x3001:
		case 0x3002:
		case 0x3003:
		case 0x3004:
		case 0x3005:
		case 0x3006:
		case 0x3007:
			/* Switch 1k VROM at $0000-$1c00 */
			ppu2c03b_set_videorom_bank(0, offset & 7, 1, data, 64);
			break;

		case 0x4000:
			/* Switch 8k bank at $c000 */
			data &= prg_mask;
			memory_set_bankptr (3, &nes.rom[0x2000 * (data) + 0x10000]);
#ifdef LOG_MMC
			logerror("     Mapper 65 switch ($c000) value: %02x\n", data);
#endif
			break;

		default:
			logerror("Mapper 65 addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper66_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 66 switch, offset %04x, data: %02x\n", offset, data);
#endif

	prg32 ((data & 0x30) >> 4);
	chr8 (data & 0x03);
}

static void sunsoft_irq ( int num, int scanline, int vblank, int blanked )
{
	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cpunum_set_input_line (0, M6502_IRQ_LINE, HOLD_LINE);
		}
		IRQ_count -= 114;
	}
}


static WRITE8_HANDLER( mapper67_w )
{
//	logerror("mapper67_w, offset %04x, data: %02x\n", offset, data);
	switch (offset & 0x7801)
	{
//		case 0x0000: /* IRQ acknowledge */
//			IRQ_enable = 0;
//			break;
		case 0x0800:
			chr2_0 (data);
			break;
		case 0x1800:
			chr2_2 (data);
			break;
		case 0x2800:
			chr2_4 (data);
			break;
		case 0x3800:
			chr2_6 (data);
			break;
		case 0x4800:
//			nes_vram[5] = data * 64;
//			break;
		case 0x4801:
			/* IRQ count? */
			IRQ_count = IRQ_count_latch;
			IRQ_count_latch = data;
			break;
		case 0x5800:
//			chr4_0 (data);
//			break;
		case 0x5801:
			IRQ_enable = data;
			break;
		case 0x6800:
		case 0x6801:
			switch(data&3)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x7800:
		case 0x7801:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper67_w uncaught offset: %04x, data: %02x\n", offset, data);
			break;

	}
}

static void mapper68_mirror (int m68_mirror, int m0, int m1)
{
	/* The 0x20000 (128k) offset is a magic number */
	#define M68_OFFSET 0x20000

	switch (m68_mirror)
	{
		case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
		case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
		case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
		case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
		case 0x10:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
		case 0x11:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
		case 0x12:
			ppu_mirror_custom_vrom (0, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m0 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m0 << 10) + M68_OFFSET);
			break;
		case 0x13:
			ppu_mirror_custom_vrom (0, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (1, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (2, (m1 << 10) + M68_OFFSET);
			ppu_mirror_custom_vrom (3, (m1 << 10) + M68_OFFSET);
			break;
	}
}

static WRITE8_HANDLER( mapper68_w )
{
	static int m68_mirror;
	static int m0, m1;

	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 2k VROM at $0000 */
			chr2_0 (data);
			break;
		case 0x1000:
			/* Switch 2k VROM at $0800 */
			chr2_2 (data);
			break;
		case 0x2000:
			/* Switch 2k VROM at $1000 */
			chr2_4 (data);
			break;
		case 0x3000:
			/* Switch 2k VROM at $1800 */
			chr2_6 (data);
			break;
		case 0x4000:
			m0 = data;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x5000:
			m1 = data;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x6000:
			m68_mirror = data & 0x13;
			mapper68_mirror (m68_mirror, m0, m1);
			break;
		case 0x7000:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper68_w uncaught offset: %04x, data: %02x\n", offset, data);
			break;

	}
}

static WRITE8_HANDLER( mapper69_w )
{
	static int cmd = 0;

//	logerror("mapper69_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7000)
	{
		case 0x0000:
			cmd = data & 0x0f;
			break;

		case 0x2000:
			switch (cmd)
			{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					ppu2c03b_set_videorom_bank(0, cmd, 1, data, 64);
					break;

				/* TODO: deal with bankswitching/write-protecting the mid-mapper area */
				case 8:
#if 0
					if (!(data & 0x40))
					{
						memory_set_bankptr (5, &nes.rom[(data & 0x3f) * 0x2000 + 0x10000]);
					}
					else
#endif
						logerror ("mapper69_w, cmd 8, data: %02x\n", data);
					break;

				case 9:
					prg8_89 (data);
					break;
				case 10:
					prg8_ab (data);
					break;
				case 11:
					prg8_cd (data);
					break;
				case 12:
					switch (data & 0x03)
					{
						case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
						case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
						case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
						case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
					}
					break;
				case 13:
					IRQ_enable = data;
					break;
				case 14:
					IRQ_count = (IRQ_count & 0xff00) | data;
					break;
				case 15:
					IRQ_count = (IRQ_count & 0x00ff) | (data << 8);
					break;
			}
			break;

		default:
			logerror("mapper69_w uncaught %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper70_w )
{
	logerror("mapper70_w offset %04x, data: %02x\n", offset, data);

	/* TODO: is the data being written irrelevant? */

	prg16_89ab ((data & 0xf0) >> 4);

	chr8 (data & 0x0f);

#if 1
	if (data & 0x80)
//		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH);
	else
//		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
		ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);
#endif
}

static WRITE8_HANDLER( mapper71_m_w )
{
	logerror("mapper71_m_w offset: %04x, data: %02x\n", offset, data);

	prg16_89ab (data);
}

static WRITE8_HANDLER( mapper71_w )
{
	logerror("mapper71_w offset: %04x, data: %02x\n", offset, data);

	if (offset >= 0x4000)
		prg16_89ab (data);
}

static WRITE8_HANDLER( mapper72_w )
{
	logerror("mapper72_w, offset %04x, data: %02x\n", offset, data);

	if(data&0x80)
		prg16_89ab (data & 0x0f);
	if(data&0x40)
		chr8 (data & 0x0f);

}

static WRITE8_HANDLER( mapper73_w )
{
	switch (offset & 0x7000)
	{
		case 0x0000:
		case 0x1000:
			/* dunno which address controls these */
			IRQ_count_latch = data;
			IRQ_enable_latch = data;
			break;
		case 0x2000:
			IRQ_enable = data;
			break;
		case 0x3000:
			IRQ_count &= ~0x0f;
			IRQ_count |= data & 0x0f;
			break;
		case 0x4000:
			IRQ_count &= ~0xf0;
			IRQ_count |= (data & 0x0f) << 4;
			break;
		case 0x7000:
			prg16_89ab (data);
			break;
		default:
			logerror("mapper73_w uncaught, offset %04x, data: %02x\n", offset, data);
			break;
	}
}


static WRITE8_HANDLER( mapper75_w )
{
	logerror("mapper75_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset & 0x7000)
	{
		case 0x0000:
			prg8_89 (data);
			break;
		case 0x1000: /* TODO: verify */
			if (data & 0x01)
				ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			else
				ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			/* vrom banking as well? */
			break;
		case 0x2000:
			prg8_ab (data);
			break;
		case 0x4000:
			prg8_cd (data);
			break;
		case 0x6000:
			chr4_0 (data);
			break;
		case 0x7000:
			chr4_4 (data);
			break;
	}
}

static WRITE8_HANDLER( mapper76_w )
{
	static int cmd=0;
	logerror("mapper76_w, offset: %04x, data: %02x\n", offset, data);
	switch(offset&0x7001)
	{
	case 0: cmd=data;break;
	case 1:
		{
			switch(cmd&7)
			{
			case 2:chr2_0(data);break;
			case 3:chr2_2(data);break;
			case 4:chr2_4(data);break;
			case 5:chr2_6(data);break;
			case 6:(cmd&0x40)?prg8_cd(data):prg8_89(data);break;
			case 7:prg8_ab(data);break;
			default:logerror("mapper76 unsupported command: %02x, data: %02x\n", cmd, data);break;
			}
		}
	case 0x2000: (data&1)?ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ):ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);break;
	default: logerror("mapper76 unmapped write, offset: %04x, data: %02x\n", offset, data);break;
	}
}
static WRITE8_HANDLER( mapper77_w )
{

/* Mapper is busted */

	logerror("mapper77_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset)
	{
		case 0x7c84:
//			prg8_89 (data);
			break;
		case 0x7c86:
//			prg8_89 (data);
//			prg8_ab (data);
//			prg8_cd (data);
//			prg16_89ab (data); /* red screen */
//			prg16_cdef (data);
			break;
	}
}

static WRITE8_HANDLER( mapper78_w )
{
	logerror("mapper78_w, offset: %04x, data: %02x\n", offset, data);
	/* Switch 8k VROM bank */
	chr8 ((data & 0xf0) >> 4);

	/* Switch 16k ROM bank */
	prg16_89ab (data & 0x0f);
}

static WRITE8_HANDLER( mapper79_l_w )
{
	logerror("mapper79_l_w: %04x:%02x\n", offset, data);

	if ((offset-0x100) & 0x0100)
	{
		/* Select 8k VROM bank */
		chr8 (data & 0x07);

		/* Select 32k ROM bank? */
		prg32 ((data & 0x08) >> 3);
	}
}

static WRITE8_HANDLER( mapper79_w )
{
	logerror("mapper79_w, offset: %04x, data: %02x\n", offset, data);
}

static WRITE8_HANDLER( mapper80_m_w )
{
	logerror("mapper80_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 */
			chr2_0 ((data & 0x7f) >> 1);
			if (data & 0x80)
			{
				/* Horizontal, $2000-$27ff */
				ppu_mirror_custom (0, 0);
				ppu_mirror_custom (1, 0);
			}
			else
			{
				/* Vertical, $2000-$27ff */
				ppu_mirror_custom (0, 0);
				ppu_mirror_custom (1, 1);
			}
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0000 */
			chr2_2 ((data & 0x7f) >> 1);
			if (data & 0x80)
			{
				/* Horizontal, $2800-$2fff */
				ppu_mirror_custom (2, 0);
				ppu_mirror_custom (3, 0);
			}
			else
			{
				/* Vertical, $2800-$2fff */
				ppu_mirror_custom (2, 0);
				ppu_mirror_custom (3, 1);
			}
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, 4, 1, data, 64);
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			ppu2c03b_set_videorom_bank(0, 5, 1, data, 64);
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			ppu2c03b_set_videorom_bank(0, 6, 1, data, 64);
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			ppu2c03b_set_videorom_bank(0, 7, 1, data, 64);
			break;
		case 0x1efa: case 0x1efb:
			/* Switch 8k ROM at $8000 */
			prg8_89 (data);
			break;
		case 0x1efc: case 0x1efd:
			/* Switch 8k ROM at $a000 */
			prg8_ab (data);
			break;
		case 0x1efe: case 0x1eff:
			/* Switch 8k ROM at $c000 */
			prg8_cd (data);
			break;
		default:
			logerror("mapper80_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper82_m_w )
{
	static int vrom_switch;

	/* This mapper has problems */

	logerror("mapper82_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 or $1000 */
			if (vrom_switch)
				chr2_4 (data);
			else
				chr2_0 (data);
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0800 or $1800 */
			if (vrom_switch)
				chr2_6 (data);
			else
				chr2_2 (data);
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, 4 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			ppu2c03b_set_videorom_bank(0, 5 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			ppu2c03b_set_videorom_bank(0, 6 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			ppu2c03b_set_videorom_bank(0, 7 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef6:
			//doc says 1= swapped. Causes in-game issues, but mostly fixes title screen
			vrom_switch = ((data & 0x02) << 1);
			break;

		case 0x1efa:
			/* Switch 8k ROM at $8000 */
			prg8_89 (data >> 2);
			break;
		case 0x1efb:
			/* Switch 8k ROM at $a000 */
			prg8_ab (data >> 2);
			break;
		case 0x1efc:
			/* Switch 8k ROM at $c000 */
			prg8_cd (data >> 2);
			break;
		default:
			logerror("mapper82_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

static WRITE8_HANDLER( konami_vrc7_w )
{
//	logerror("konami_vrc7_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline);

	switch (offset & 0x7018)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			prg8_89 (data);
			break;
		case 0x0008: case 0x0010: case 0x0018:
			/* Switch 8k bank at $a000 */
			prg8_ab (data);
			break;

		case 0x1000:
			/* Switch 8k bank at $c000 */
			prg8_cd (data);
			break;

/* TODO: there are sound regs in here */

		case 0x2000:
			/* Switch 1k VROM at $0000 */
			ppu2c03b_set_videorom_bank(0, 0, 1, data, 64);
			break;
		case 0x2008: case 0x2010: case 0x2018:
			/* Switch 1k VROM at $0400 */
			ppu2c03b_set_videorom_bank(0, 1, 1, data, 64);
			break;
		case 0x3000:
			/* Switch 1k VROM at $0800 */
			ppu2c03b_set_videorom_bank(0, 2, 1, data, 64);
			break;
		case 0x3008: case 0x3010: case 0x3018:
			/* Switch 1k VROM at $0c00 */
			ppu2c03b_set_videorom_bank(0, 3, 1, data, 64);
			break;
		case 0x4000:
			/* Switch 1k VROM at $1000 */
			ppu2c03b_set_videorom_bank(0, 4, 1, data, 64);
			break;
		case 0x4008: case 0x4010: case 0x4018:
			/* Switch 1k VROM at $1400 */
			ppu2c03b_set_videorom_bank(0, 5, 1, data, 64);
			break;
		case 0x5000:
			/* Switch 1k VROM at $1800 */
			ppu2c03b_set_videorom_bank(0, 6, 1, data, 64);
			break;
		case 0x5008: case 0x5010: case 0x5018:
			/* Switch 1k VROM at $1c00 */
			ppu2c03b_set_videorom_bank(0, 7, 1, data, 64);
			break;

		case 0x6000:
			switch (data & 0x03)
			{
				case 0x00: ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT); break;
				case 0x01: ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ); break;
				case 0x02: ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW); break;
				case 0x03: ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x6008: case 0x6010: case 0x6018:
			IRQ_count_latch = data;
			break;
		case 0x7000:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7008: case 0x7010: case 0x7018:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc7_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

static WRITE8_HANDLER( mapper86_w )
{
	logerror("mapper86_w, offset: %04x, data: %02x\n", offset, data);
	switch (offset)
	{
		case 0x7541:
			prg16_89ab (data >> 4);
//			prg16_cdef (data >> 4);
			break;
		case 0x7d41:
//			prg16_89ab (data >> 4);
//			prg16_cdef (data >> 4);
			break;
	}
}

static WRITE8_HANDLER( mapper87_m_w )
{
	logerror("mapper87_m_w %04x:%02x\n", offset, data);

	/* TODO: verify */
	chr8 (data >> 1);
}
static WRITE8_HANDLER( mapper89_w )
{
	int chr;
	logerror("mapper89_m_w %04x:%02x\n", offset, data);
	prg16_89ab((data>>4)&7);
	chr=(data&7)|((data&0x80)?8:0);
	chr8(chr);
	(data&8)?ppu2c03b_set_mirroring(0, PPU_MIRROR_HIGH):ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);

}

static WRITE8_HANDLER( mapper91_m_w )
{
	logerror ("mapper91_m_w, offset: %04x, data: %02x\n", offset, data);

	switch (offset & 0x7000)
	{
		case 0x0000:
			switch (offset & 0x03)
			{
				case 0x00: chr2_0 (data); break;
				case 0x01: chr2_2 (data); break;
				case 0x02: chr2_4 (data); break;
				case 0x03: chr2_6 (data); break;
			}
			break;
		case 0x1000:
			switch (offset & 0x01)
			{
				case 0x00: prg8_89 (data); break;
				case 0x01: prg8_ab (data); break;
			}
			break;
		default:
			logerror("mapper91_m_w uncaught addr: %04x value: %02x\n", offset + 0x6000, data);
			break;
	}
}
static WRITE8_HANDLER( mapper92_w )
{
	logerror("mapper92_w, offset %04x, data: %02x\n", offset, data);

	if(data&0x80)
		prg16_cdef (data & 0x0f);
	if(data&0x40)
		chr8 (data & 0x0f);

}

static WRITE8_HANDLER( mapper93_m_w )
{
#ifdef LOG_MMC
	logerror("mapper93_m_w %04x:%02x\n", offset, data);
#endif

	prg16_89ab (data);
}

static WRITE8_HANDLER( mapper93_w )
{
#ifdef LOG_MMC
	logerror("mapper93_w %04x:%02x\n", offset, data);
#endif
	/* The high nibble appears to be the same prg bank as */
	/* was written to the mid-area mapper */
}

static WRITE8_HANDLER( mapper94_w )
{
#ifdef LOG_MMC
	logerror("mapper94_w %04x:%02x\n", offset, data);
#endif

	prg16_89ab (data >> 2);
}

static WRITE8_HANDLER( mapper95_w )
{
#ifdef LOG_MMC
	logerror("mapper95_w %04x:%02x\n", offset, data);
#endif

	switch (offset)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
//			prg8_89 (data);
//			prg8_ab (data);
//			prg16_89ab (data);
			break;
		case 0x0001:
			/* Switch 8k bank at $a000 */
			prg8_ab (data >> 1);
			break;
	}
}
static WRITE8_HANDLER( mapper96_w )
{
	prg32 (data);
}

static WRITE8_HANDLER( mapper97_w )
{
	if(offset&0x4000)
		return;
	if(data&0x80)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
	else ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	prg16_cdef(data&0x0F);
}
static WRITE8_HANDLER( mapper101_m_w )
{
#ifdef LOG_MMC
	logerror("mapper101_m_w %04x:%02x\n", offset, data);
#endif

	chr8 (data);
}

static WRITE8_HANDLER( mapper101_w )
{
#ifdef LOG_MMC
	logerror("mapper101_w %04x:%02x\n", offset, data);
#endif

	/* ??? */
}
static WRITE8_HANDLER( mapper113_l_w )
{
#ifdef LOG_MMC
	logerror("mapper113_w %04x:%02x\n", offset, data);
#endif
	//I think this is the right value
	if(offset>0xFF)
		return;
	prg32(data>>3);
	chr8(data&7);
}
static WRITE8_HANDLER( mapper133_l_w )
{
#ifdef LOG_MMC
	logerror("mapper133_w %04x:%02x\n", offset, data);
#endif
	//I think this is the right value
	if(offset!=0x20)
		return;
	prg32(data>>2);
	chr8(data&3);
}

static WRITE8_HANDLER( mapper144_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 144 switch, data: %02x\n", data);
#endif

	//just like mapper 11, except bit 0 taken from ROM
	//and 0x8000 ignored?

	if(0==(offset&0x7FFF))
		return;

	data = data|(program_read_byte_8(offset)&1);

	/* Switch 8k VROM bank */
	chr8 (data >> 4);

	/* Switch 32k prg bank */
	prg32 (data & 0x07);
}
static WRITE8_HANDLER( mapper180_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 180 switch, data: %02x\n", data);
#endif

	prg16_cdef(data&7);
}
static WRITE8_HANDLER( mapper184_m_w )
{
#ifdef LOG_MMC
	logerror("* Mapper 184 switch, data: %02x\n", data);
#endif

	if(0==offset)
	{
		chr4_0(data&0x0F);
		chr4_4((data>>4));
	}
}

static WRITE8_HANDLER( mapper225_w )
{
	int hi_bank;
	int size_16;
	int bank;

#ifdef LOG_MMC
	logerror ("mapper225_w, offset: %04x, data: %02x\n", offset, data);
#endif

	chr8 (offset & 0x3f);
	hi_bank = offset & 0x40;
	size_16 = offset & 0x1000;
	bank = (offset & 0xf80) >> 7;
	if (size_16)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab (bank);
		prg16_cdef (bank);
	}
	else
		prg32 (bank);

	if (offset & 0x2000)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
}

static WRITE8_HANDLER( mapper226_w )
{
	int hi_bank;
	int size_16;
	int bank;
	static int reg0, reg1;

#ifdef LOG_MMC
	logerror ("mapper226_w, offset: %04x, data: %02x\n", offset, data);
#endif

	if (offset & 0x01)
	{
		reg1 = data;
	}
	else
	{
		reg0 = data;
	}

	hi_bank = reg0 & 0x01;
	size_16 = reg0 & 0x20;
	if (reg0 & 0x40)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);

	bank = ((reg0 & 0x1e) >> 1) | ((reg1 & 0x01) << 4);

	if (size_16)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab (bank);
		prg16_cdef (bank);
	}
	else
		prg32 (bank);
}

static WRITE8_HANDLER( mapper227_w )
{
	int hi_bank;
	int size_32;
	int bank;

#ifdef LOG_MMC
	logerror ("mapper227_w, offset: %04x, data: %02x\n", offset, data);
#endif

	hi_bank = offset & 0x04;
	size_32 = offset & 0x01;
	bank = ((offset & 0x78) >> 3) | ((offset & 0x0100) >> 4);
	if (!size_32)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab (bank);
		prg16_cdef (bank);
	}
	else
		prg32 (bank);

	if (!(offset & 0x80))
	{
		if (offset & 0x200)
			prg16_cdef ((bank >> 2) + 7);
		else
			prg16_cdef (bank >> 2);
	}

	if (offset & 0x02)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
}

static WRITE8_HANDLER( mapper228_w )
{
	/* The address lines double as data */
	/* --mPPppppPP-cccc */
	int bank;
	int chr;

#ifdef LOG_MMC
	logerror ("mapper228_w, offset: %04x, data: %02x\n", offset, data);
#endif

	/* Determine low 4 bits of program bank */
	bank = (offset & 0x780) >> 7;

#if 1
	/* Determine high 2 bits of program bank */
	switch (offset & 0x1800)
	{
		case 0x0800:
			bank |= 0x10;
			break;
		case 0x1800:
			bank |= 0x20;
			break;
	}
#else
	bank |= (offset & 0x1800) >> 7;
#endif

	/* see if the bank value is 16k or 32k */
	if (offset & 0x20)
	{
		/* 16k bank value, adjust */
		bank <<= 1;
		if (offset & 0x40)
			bank ++;

		prg16_89ab (bank);
		prg16_cdef (bank);
	}
	else
	{
		prg32 (bank);
	}

	if (offset & 0x2000)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);

	/* Now handle vrom banking */
	chr = (data & 0x03) + ((offset & 0x0f) << 2);
	chr8 (chr);
}

static WRITE8_HANDLER( mapper229_w )
{
#ifdef LOG_MMC
	logerror ("mapper229_w, offset: %04x, data: %02x\n", offset, data);
#endif

	if (offset & 0x20)
		ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
	else
		ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);

	if ((offset & 0x1e) == 0)
	{
		prg32 (0);
		chr8 (0);
	}
	else
	{
		prg16_89ab (offset & 0x1f);
		prg16_89ab (offset & 0x1f);
		chr8 (offset);
	}
}

static WRITE8_HANDLER( mapper231_w )
{
	int bank;

#ifdef LOG_MMC
	logerror("mapper231_w, offset: %04x, data: %02x\n", offset, data);
#endif

	bank = (data & 0x03) | ((data & 0x80) >> 5);
	prg32 (bank);
	chr8 ((data & 0x70) >> 4);
}

/*
// mapper_reset
//
// Resets the mapper bankswitch areas to their defaults. It returns a value "err"
// that indicates if it was successful. Possible values for err are:
//
// 0 = success
// 1 = no mapper found
// 2 = mapper not supported
*/
int mapper_reset (int mapperNum)
{
	int err = 0;
	const mmc *mapper;

	/* Set the vram bank-switch values to the default */
	ppu2c03b_set_videorom_bank(0, 0, 8, 0, 64);
	
	/* Set the mapper irq callback */
	mapper = nes_mapper_lookup(mapperNum);
	ppu2c03b_set_scanline_callback (0, mapper ? mapper->mmc_irq : NULL);

	mapper_warning = 0;
	/* 8k mask */
	prg_mask = ((nes.prg_chunks << 1) - 1);

	MMC5_vram_control = 0;

	if (mapperNum != 20)
	{
		/* Point the WRAM/battery area to the first RAM bank */
		memory_set_bankptr (5, &nes.wram[0x0000]);
	}

	switch (mapperNum)
	{
		case 0:
			err = 1; /* No mapper found */
			prg32(0);
			break;
		case 1:
			/* Reset the latch */
			MMC1_reg = 0;
			MMC1_reg_count = 0;

			MMC1_Size_16k = 1;
			MMC1_Switch_Low = 1;
			MMC1_SizeVrom_4k = 0;
			MMC1_extended_bank = 0;
			MMC1_extended_swap = 0;
			MMC1_extended_base = 0x10000;
			MMC1_extended = ((nes.prg_chunks << 4) + nes.chr_chunks * 8) >> 9;

			if (!MMC1_extended)
				/* Set it to the end of the prg rom */
				MMC1_High = (nes.prg_chunks - 1) * 0x4000;
			else
				/* Set it to the end of the first 256k bank */
				MMC1_High = 15 * 0x4000;

			MMC1_bank1 = 0;
			MMC1_bank2 = 0x2000;
			MMC1_bank3 = MMC1_High;
			MMC1_bank4 = MMC1_High + 0x2000;

			memory_set_bankptr (1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
			memory_set_bankptr (2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
			memory_set_bankptr (3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
			memory_set_bankptr (4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
			logerror("-- page1: %06x\n", MMC1_bank1);
			logerror("-- page2: %06x\n", MMC1_bank2);
			logerror("-- page3: %06x\n", MMC1_bank3);
			logerror("-- page4: %06x\n", MMC1_bank4);
			break;
		case 2:
			/* These games don't switch VROM, but some ROMs incorrectly have CHR chunks */
			nes.chr_chunks = 0;
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 13:
		case 3:
			/* Doesn't bank-switch */
			prg32(0);
			break;
		case 4:
		case 118:
			/* Can switch 8k prg banks */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch = 0;
			MMC3_prg0 = 0xfe;
			MMC3_prg1 = 0xff;
			MMC3_cmd = 0;
			prg16_89ab (nes.prg_chunks-1);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 5:
			/* Can switch 8k prg banks, but they are saved as 16k in size */
			MMC5_rom_bank_mode = 3;
			MMC5_vrom_bank_mode = 0;
			MMC5_vram_protect = 0;
			IRQ_enable = 0;
			IRQ_count = 0;
			nes.mid_ram_enable = 0;
			prg16_89ab (nes.prg_chunks-2);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 96:
		case 8:
			/* Switches 16k banks at $8000, 1st 2 16k banks loaded on reset */
			prg32(0);
			break;
		case 9:
			/* Can switch 8k prg banks */
			/* Note that the iNES header defines the number of banks as 8k in size, rather than 16k */
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			prg8_89(0);
			//ugly hack to deal with iNES header usage of chunk count.
			prg8_ab((nes.prg_chunks<<1)-3);
			prg8_cd((nes.prg_chunks<<1)-2);
			prg8_ef((nes.prg_chunks<<1)-1);
			break;
		case 10:
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 11:
			/* Switches 32k banks, 1st 32k bank loaded on reset (?) May be more like mapper 7... */
			prg32(0);
			break;
		case 15:
			/* Can switch 8k prg banks */
			prg32(0);
			break;
		case 16:
		case 17:
		case 18:
		case 19:
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 20:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			nes_fds.motor_on = 0;
			nes_fds.door_closed = 0;
			nes_fds.current_side = 1;
			nes_fds.head_position = 0;
			nes_fds.status0 = 0;
			nes_fds.read_mode = nes_fds.write_reg = 0;
			break;
		case 21:
		case 25:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			/* fall through */
		case 22:
		case 23:
		case 32:
		case 33:
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 24:
		case 26:
		case 73:
		case 75:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 34:
			/* Can switch 32k prg banks */
			prg32(0);
			break;
		case 40:
			IRQ_enable = 0;
			IRQ_count = 0;
			/* Who's your daddy? */
			memcpy (&nes.rom[0x6000], &nes.rom[6 * 0x2000 + 0x10000], 0x2000);
			prg8_89 (4);
			prg8_ab (5);
			prg8_cd (6);
			prg8_ef (7);
			break;
		case 64:
			/* Can switch 3 8k prg banks */
			prg16_89ab (nes.prg_chunks-1);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 65:
			IRQ_enable = 0;
			IRQ_count = 0;
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 41:
		case 66:
			/* Can switch 32k prgm banks */
			prg32(0);
			break;
		case 70:
//		case 86:
			prg16_89ab (nes.prg_chunks-2);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 67:
		case 68:
		case 69:
		case 71:
		case 72:
		case 77:
		case 78:
		case 92:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 76:
			prg8_89(0);
			prg8_ab(1);
			//cd is bankable, but this should init all banks just fine.
			prg16_cdef(nes.prg_chunks-1);
			chr2_0(0);
			chr2_2(1);
			chr2_4(2);
			chr2_6(3);
			break;
		case 79:
			/* Mirroring always horizontal...? */
//			Mirroring = 1;
			prg32(0);
			break;
		case 80:
		case 82:
		case 85:
		case 86:
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
//		case 70:
		case 87:
		case 228:
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 89:
			prg16_89ab(0);
			prg16_cdef(nes.prg_chunks-1);
			ppu2c03b_set_mirroring(0, PPU_MIRROR_LOW);
			break;

		case 91:
			ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			prg16_89ab (nes.prg_chunks-1);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 93:
		case 94:
		case 95:
		case 101:
			prg16_89ab (0);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 97:
			prg16_89ab (nes.prg_chunks-1);
			prg16_cdef (nes.prg_chunks-1);
			break;
		case 113:
		case 133:
			prg32(0);
			break;
		case 184:
		case 144:
			prg32(0);
			break;
		case 180:
			prg16_89ab(0);
			prg16_cdef(0);
		case 225:
		case 226:
		case 227:
		case 229:
			prg16_89ab (0);
			prg16_cdef (0);
			break;
		case 231:
			prg16_89ab (nes.prg_chunks-2);
			prg16_cdef (nes.prg_chunks-1);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}

//	if (nes.chr_chunks)
//		memcpy (videoram, nes.vrom, 0x2000);

	return err;
}



static mmc mmc_list[] =
{
/*	INES   DESC						LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, IRQ */
	{ 0, "No Mapper",				NULL, NULL, NULL, NULL, NULL, NULL },
	{ 1, "MMC1",					NULL, NULL, NULL, mapper1_w, NULL, NULL },
	{ 2, "74161/32 ROM sw",			NULL, NULL, NULL, mapper2_w, NULL, NULL },
	{ 3, "74161/32 VROM sw",		NULL, NULL, NULL, mapper3_w, NULL, NULL },
	{ 4, "MMC3",					NULL, NULL, NULL, mapper4_w, NULL, mapper4_irq },
	{ 5, "MMC5",					mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, mapper5_irq },
	{ 7, "ROM Switch",				NULL, NULL, NULL, mapper7_w, NULL, NULL },
	{ 8, "FFE F3xxxx",				NULL, NULL, NULL, mapper8_w, NULL, NULL },
	{ 9, "MMC2",					NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL},
	{ 10, "MMC4",					NULL, NULL, NULL, mapper10_w, mapper10_latch, NULL },
	{ 11, "Color Dreams Mapper",	NULL, NULL, NULL, mapper11_w, NULL, NULL },
	{ 13, "CP-ROM",					NULL, NULL,	NULL, mapper13_w, NULL, NULL }, //needs CHR-RAM hooked up
	{ 15, "100-in-1",				NULL, NULL, NULL, mapper15_w, NULL, NULL },
	{ 16, "Bandai",					NULL, NULL, mapper16_w, mapper16_w, NULL, bandai_irq },
	{ 17, "FFE F8xxx",				mapper17_l_w, NULL, NULL, NULL, NULL, mapper4_irq },
	{ 18, "Jaleco",					NULL, NULL, NULL, mapper18_w, NULL, jaleco_irq },
	{ 19, "Namco 106",				mapper19_l_w, NULL, NULL, mapper19_w, NULL, namcot_irq },
	{ 20, "Famicom Disk System",	NULL, NULL, NULL, NULL, NULL, fds_irq },
	{ 21, "Konami VRC 4",			NULL, NULL, NULL, konami_vrc4_w, NULL, konami_irq },
	{ 22, "Konami VRC 2a",			NULL, NULL, NULL, konami_vrc2a_w, NULL, konami_irq },
	{ 23, "Konami VRC 2b",			NULL, NULL, NULL, konami_vrc2b_w, NULL, konami_irq },
	{ 24, "Konami VRC 6a",			NULL, NULL, NULL, konami_vrc6a_w, NULL, konami_irq },
	{ 25, "Konami VRC 4",			NULL, NULL, NULL, konami_vrc4_w, NULL, konami_irq },
	{ 26, "Konami VRC 6b",			NULL, NULL, NULL, konami_vrc6b_w, NULL, konami_irq },
	{ 32, "Irem G-101",				NULL, NULL, NULL, mapper32_w, NULL, NULL },
	{ 33, "Taito TC0190",			NULL, NULL, NULL, mapper33_w, NULL, NULL },
	{ 34, "Nina-1",					NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL },
	{ 40, "SMB2j (bootleg)",		NULL, NULL, NULL, mapper40_w, NULL, mapper40_irq },
	{ 41, "Caltron 6-in-1",			NULL, NULL, mapper41_m_w, mapper41_w, NULL, NULL },
// 42 - "Mario Baby" pirate cart
	{ 64, "Tengen",					NULL, NULL, mapper64_m_w, mapper64_w, NULL, mapper4_irq },
	{ 65, "Irem H3001",				NULL, NULL, NULL, mapper65_w, NULL, irem_irq },
	{ 66, "74161/32 Jaleco",		NULL, NULL, NULL, mapper66_w, NULL, NULL },
	{ 67, "SunSoft 3",				NULL, NULL, NULL, mapper67_w, NULL, mapper4_irq },
	{ 68, "SunSoft 4",				NULL, NULL, NULL, mapper68_w, NULL, NULL },
	{ 69, "SunSoft 5",				NULL, NULL, NULL, mapper69_w, NULL, sunsoft_irq },
	{ 70, "74161/32 Bandai",		NULL, NULL, NULL, mapper70_w, NULL, NULL },
	{ 71, "Camerica",				NULL, NULL, mapper71_m_w, mapper71_w, NULL, NULL },
	{ 72, "74161/32 Jaleco",		NULL, NULL, NULL, mapper72_w, NULL, NULL },
	{ 73, "Konami VRC 3",			NULL, NULL, NULL, mapper73_w, NULL, konami_irq },
	{ 75, "Konami VRC 1",			NULL, NULL, NULL, mapper75_w, NULL, NULL },
	{ 76, "Namco 109",				NULL, NULL, NULL, mapper76_w, NULL, NULL },
	{ 77, "74161/32 ?",				NULL, NULL, NULL, mapper77_w, NULL, NULL },
	{ 78, "74161/32 Irem",			NULL, NULL, NULL, mapper78_w, NULL, NULL },
	{ 79, "Nina-3 (AVE)",			mapper79_l_w, NULL, mapper79_w, mapper79_w, NULL, NULL },
	{ 80, "Taito X1-005",			NULL, NULL, mapper80_m_w, NULL, NULL, NULL },
	{ 82, "Taito C075",				NULL, NULL, mapper82_m_w, NULL, NULL, NULL },
// 83
	{ 84, "Pasofami",				NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, "Konami VRC 7",			NULL, NULL, NULL, konami_vrc7_w, NULL, konami_irq },
	{ 86, "Jaleco Early Mapper 2",				NULL, NULL, NULL, mapper86_w, NULL, NULL },
	{ 87, "74161/32 VROM sw-a",		NULL, NULL, mapper87_m_w, NULL, NULL, NULL },
	{ 88, "Namco 118",				NULL, NULL, NULL, mapper4_w, NULL, mapper4_irq },
	{ 89, "Sunsoft Early",			NULL, NULL, NULL, mapper89_w, NULL, NULL },
// 90 - pirate mapper
	{ 91, "HK-SF3 (bootleg)",		NULL, NULL, mapper91_m_w, NULL, NULL, NULL },
	{ 92, "Jaleco Early Mapper 1",	NULL, NULL, NULL, mapper92_w, NULL, NULL },
	{ 93, "Sunsoft LS161",			NULL, NULL, mapper93_m_w, mapper93_w, NULL, NULL },
	{ 94, "Capcom LS161",			NULL, NULL, NULL, mapper94_w, NULL, NULL },
	{ 95, "Namco ??",				NULL, NULL, NULL, mapper95_w, NULL, NULL },
	{ 96, "74161/32",			NULL, NULL, NULL, mapper96_w, NULL, NULL },
	{ 97, "Irem - PRG HI",			NULL, NULL, NULL, mapper97_w, NULL, NULL },
// 99 - vs. system
// 100 - images hacked to work with nesticle
	{ 101, "?? LS161",				NULL, NULL, mapper101_m_w, mapper101_w, NULL, NULL },
	{ 113, "Sachen/Hacker/Nina",	mapper113_l_w, NULL, NULL, NULL, NULL, NULL },
	{ 118, "MMC3?",					NULL, NULL, NULL, mapper118_w, NULL, mapper4_irq },
// 119 - Pinbot
	{ 133, "Sachen",				mapper133_l_w, NULL, NULL, NULL, NULL, NULL },
	{ 144, "AGCI 50282",			NULL, NULL, NULL, mapper144_w, NULL, NULL }, //Death Race only
	{ 180, "Nihon Bussan - PRG HI",	NULL, NULL, NULL, mapper180_w, NULL, NULL },
	{ 184, "Sunsoft VROM/4K",		NULL, NULL, mapper184_m_w, NULL, NULL, NULL },
	{ 225, "72-in-1 bootleg",		NULL, NULL, NULL, mapper225_w, NULL, NULL },
	{ 226, "76-in-1 bootleg",		NULL, NULL, NULL, mapper226_w, NULL, NULL },
	{ 227, "1200-in-1 bootleg",		NULL, NULL, NULL, mapper227_w, NULL, NULL },
	{ 228, "Action 52",				NULL, NULL, NULL, mapper228_w, NULL, NULL },
	{ 229, "31-in-1",				NULL, NULL, NULL, mapper229_w, NULL, NULL },
//	{ 230, "22-in-1",				NULL, NULL, NULL, mapper230_w, NULL, NULL },
	{ 231, "Nina-7 (AVE)",			NULL, NULL, NULL, mapper231_w, NULL, NULL }
// 234 - maxi-15
};



const mmc *nes_mapper_lookup(int mapper)
{
	int i;
	for (i = 0; i < sizeof(mmc_list) / sizeof(mmc_list[0]); i++)
	{
		if (mmc_list[i].iNesMapper == nes.mapper)
			return &mmc_list[i];
	}
	return NULL;
}

