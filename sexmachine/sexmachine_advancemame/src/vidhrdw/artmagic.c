/***************************************************************************

    Art & Magic hardware

***************************************************************************/

#include "driver.h"
#include "profiler.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "artmagic.h"


#define INSTANT_BLIT		1


UINT16 *artmagic_vram0;
UINT16 *artmagic_vram1;

/* decryption parameters */
int artmagic_xor[16], artmagic_is_stoneball;

static UINT16 *blitter_base;
static UINT32 blitter_mask;
static UINT16 blitter_data[8];
static UINT8 blitter_page;

#if (!INSTANT_BLIT)
static double blitter_busy_until;
#endif



/*************************************
 *
 *  Inlines
 *
 *************************************/

INLINE UINT16 *address_to_vram(offs_t *address)
{
	offs_t original = *address;
	*address = TOWORD(original & 0x001fffff);
	if (original >= 0x00000000 && original < 0x001fffff)
		return artmagic_vram0;
	else if (original >= 0x00400000 && original < 0x005fffff)
		return artmagic_vram1;
	return NULL;
}



/*************************************
 *
 *  Video start
 *
 *************************************/

VIDEO_START( artmagic )
{
	blitter_base = (UINT16 *)memory_region(REGION_GFX1);
	blitter_mask = memory_region_length(REGION_GFX1)/2 - 1;
	return 0;
}



/*************************************
 *
 *  Shift register transfers
 *
 *************************************/

void artmagic_to_shiftreg(offs_t address, UINT16 *data)
{
	UINT16 *vram = address_to_vram(&address);
	if (vram)
		memcpy(data, &vram[address], TOBYTE(0x2000));
}


void artmagic_from_shiftreg(offs_t address, UINT16 *data)
{
	UINT16 *vram = address_to_vram(&address);
	if (vram)
		memcpy(&vram[address], data, TOBYTE(0x2000));
}



/*************************************
 *
 *  Custom blitter
 *
 *************************************/

static void execute_blit(void)
{
	UINT16 *dest = blitter_page ? artmagic_vram0 : artmagic_vram1;
	int offset = ((blitter_data[1] & 0xff) << 16) | blitter_data[0];
	int color = (blitter_data[1] >> 4) & 0xf0;
	int x = (INT16)blitter_data[2];
	int y = (INT16)blitter_data[3];
	int maskx = blitter_data[6] & 0xff;
	int masky = blitter_data[6] >> 8;
	int w = ((blitter_data[7] & 0xff) + 1) * 4;
	int h = (blitter_data[7] >> 8) + 1;
	int i, j, sx, sy, last;

#if 0
{
	static UINT32 hit_list[50000];
	static int hit_index;
	static FILE *f;

	logerror("%08X:Blit from %06X to (%d,%d) %dx%d -- %04X %04X %04X %04X %04X %04X %04X %04X\n",
				activecpu_get_pc(), offset, x, y, w, h,
				blitter_data[0], blitter_data[1],
				blitter_data[2], blitter_data[3],
				blitter_data[4], blitter_data[5],
				blitter_data[6], blitter_data[7]);

	if (!f) f = fopen("artmagic.log", "w");

	for (i = 0; i < hit_index; i++)
		if (hit_list[i] == offset)
			break;
	if (i == hit_index)
	{
		int tempoffs = offset;
		hit_list[hit_index++] = offset;

		fprintf(f, "----------------------\n"
				   "%08X:Blit from %06X to (%d,%d) %dx%d -- %04X %04X %04X %04X %04X %04X %04X %04X\n",
					activecpu_get_pc(), offset, x, y, w, h,
					blitter_data[0], blitter_data[1],
					blitter_data[2], blitter_data[3],
					blitter_data[4], blitter_data[5],
					blitter_data[6], blitter_data[7]);

		fprintf(f, "\t");
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < w; j += 4)
				fprintf(f, "%04X ", blitter_base[tempoffs++]);
			fprintf(f, "\n\t");
		}
		fprintf(f, "\n\t");
		tempoffs = offset;
		for (i = 0; i < h; i++)
		{
			last = 0;
			if (i == 0)	/* first line */
			{
				/* ultennis, stonebal */
				last ^= (blitter_data[7] & 0x0001);
				if (artmagic_is_stoneball)
					last ^= ((blitter_data[0] & 0x0020) >> 3);
				else	/* ultennis */
					last ^= ((blitter_data[0] & 0x0040) >> 4);

				/* cheesech */
				last ^= ((blitter_data[7] & 0x0400) >> 9);
				last ^= ((blitter_data[0] & 0x2000) >> 10);
			}
			else	/* following lines */
			{
				int val = blitter_base[tempoffs];

				/* ultennis, stonebal */
				last ^= 4;
				last ^= ((val & 0x0400) >> 8);
				last ^= ((val & 0x5000) >> 12);

				/* cheesech */
				last ^= 8;
				last ^= ((val & 0x0800) >> 8);
				last ^= ((val & 0xa000) >> 12);
			}

			for (j = 0; j < w; j += 4)
			{
				static const char hex[] = ".123456789ABCDEF";
				int val = blitter_base[tempoffs++];
				int p1, p2, p3, p4;
				p1 = last = ((val ^ artmagic_xor[last]) >>  0) & 0xf;
				p2 = last = ((val ^ artmagic_xor[last]) >>  4) & 0xf;
				p3 = last = ((val ^ artmagic_xor[last]) >>  8) & 0xf;
				p4 = last = ((val ^ artmagic_xor[last]) >> 12) & 0xf;
				fprintf(f, "%c%c%c%c ", hex[p1], hex[p2], hex[p3], hex[p4]);
			}
			fprintf(f, "\n\t");
		}
		fprintf(f, "\n");
	}
}
#endif

	profiler_mark(PROFILER_VIDEO);

	last = 0;
	sy = y;
	for (i = 0; i < h; i++)
	{
		if ((i & 1) || !((masky << ((i/2) & 7)) & 0x80))
		{
			if (sy >= 0 && sy < 256)
			{
				int tsy = sy * TOWORD(0x2000);
				sx = x;

				/* The first pixel of every line doesn't have a previous pixel
                   to depend on, so it takes the "feed" from other bits.
                   The very first pixel blitted is also treated differently.

                   ultennis/stonebal use a different encryption from cheesech,
                   however the former only need to set bits 0 and 2 of the
                   feed (the others are irrelevant), while the latter only
                   bits 1 and 3, so I can handle both at the same time.
                 */
				last = 0;
				if (i == 0)	/* first line */
				{
					/* ultennis, stonebal */
					last ^= (blitter_data[7] & 0x0001);
					if (artmagic_is_stoneball)
						last ^= ((blitter_data[0] & 0x0020) >> 3);
					else	/* ultennis */
						last ^= (((blitter_data[0] + 1) & 0x0040) >> 4);

					/* cheesech */
					last ^= ((blitter_data[7] & 0x0400) >> 9);
					last ^= ((blitter_data[0] & 0x2000) >> 10);
				}
				else	/* following lines */
				{
					int val = blitter_base[offset & blitter_mask];

					/* ultennis, stonebal */
					last ^= 4;
					last ^= ((val & 0x0400) >> 8);
					last ^= ((val & 0x5000) >> 12);

					/* cheesech */
					last ^= 8;
					last ^= ((val & 0x0800) >> 8);
					last ^= ((val & 0xa000) >> 12);
				}

				for (j = 0; j < w; j += 4)
				{
					UINT16 val = blitter_base[(offset + j/4) & blitter_mask];
					if (sx < 508)
					{
						if (h == 1 && artmagic_is_stoneball)
							last = ((val) >>  0) & 0xf;
						else
							last = ((val ^ artmagic_xor[last]) >>  0) & 0xf;
						if (!((maskx << ((j/2) & 7)) & 0x80))
						{
							if (last && sx >= 0 && sx < 512)
								dest[tsy + sx] = color | (last);
							sx++;
						}

						if (h == 1 && artmagic_is_stoneball)
							last = ((val) >>  4) & 0xf;
						else
							last = ((val ^ artmagic_xor[last]) >>  4) & 0xf;
						{
							if (last && sx >= 0 && sx < 512)
								dest[tsy + sx] = color | (last);
							sx++;
						}

						if (h == 1 && artmagic_is_stoneball)
							last = ((val) >>  8) & 0xf;
						else
							last = ((val ^ artmagic_xor[last]) >>  8) & 0xf;
						if (!((maskx << ((j/2) & 7)) & 0x40))
						{
							if (last && sx >= 0 && sx < 512)
								dest[tsy + sx] = color | (last);
							sx++;
						}

						if (h == 1 && artmagic_is_stoneball)
							last = ((val) >> 12) & 0xf;
						else
							last = ((val ^ artmagic_xor[last]) >> 12) & 0xf;
						{
							if (last && sx >= 0 && sx < 512)
								dest[tsy + sx] = color | (last);
							sx++;
						}
					}
				}
			}
			sy++;
		}
		offset += w/4;
	}

	profiler_mark(PROFILER_END);

#if (!INSTANT_BLIT)
	blitter_busy_until = timer_get_time() + (w*h*TIME_IN_NSEC(20));
#endif
}


READ16_HANDLER( artmagic_blitter_r )
{
	/*
        bit 1 is a busy flag; loops tightly if clear
        bit 2 is tested in a similar fashion
        bit 4 reflects the page
    */
	UINT16 result = 0xffef | (blitter_page << 4);
#if (!INSTANT_BLIT)
	if (timer_get_time() < blitter_busy_until)
		result ^= 6;
#endif
	return result;
}


WRITE16_HANDLER( artmagic_blitter_w )
{
	COMBINE_DATA(&blitter_data[offset]);

	/* offset 3 triggers the blit */
	if (offset == 3)
		execute_blit();

	/* offset 4 contains the target page */
	else if (offset == 4)
		blitter_page = (data >> 1) & 1;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( artmagic )
{
	UINT32 offset, dpytap;
	UINT16 *vram;
	int x, y;

	/* get the current parameters */
	cpuintrf_push_context(0);
	offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 8;
	dpytap = tms34010_io_register_r(REG_DPYTAP, 0);
	cpuintrf_pop_context();

	/* compute the base and offset */
	vram = address_to_vram(&offset);
	if (!vram || tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	offset += cliprect->min_y * TOWORD(0x2000);
	offset += dpytap;

	/* render the bitmap */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels;
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = vram[(offset + x) & TOWORD(0x1fffff)] & 0xff;
		offset += TOWORD(0x2000);
	}
}
