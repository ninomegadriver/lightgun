/***************************************************************************

    Atari System 2 hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "slapstic.h"
#include "atarisy2.h"



/*************************************
 *
 *  Globals we own
 *
 *************************************/

UINT16 *atarisy2_slapstic;



/*************************************
 *
 *  Statics
 *
 *************************************/

static void *yscroll_reset_timer;
static UINT32 playfield_tile_bank[2];
static UINT32 videobank;
static UINT16 *vram;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void reset_yscroll_callback(int param);



/*************************************
 *
 *  Tilemap callbacks
 *
 *************************************/

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha[tile_index];
	int code = data & 0x3ff;
	int color = (data >> 13) & 0x07;
	SET_TILE_INFO(2, code, color, 0);
}


static void get_playfield_tile_info(int tile_index)
{
	UINT16 data = atarigen_playfield[tile_index];
	int code = playfield_tile_bank[(data >> 10) & 1] + (data & 0x3ff);
	int color = (data >> 11) & 7;
	SET_TILE_INFO(0, code, color, 0);
	tile_info.priority = (~data >> 14) & 3;
}



/*************************************
 *
 *  Video system start
 *
 *************************************/

VIDEO_START( atarisy2 )
{
	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		0,					/* pixel offset for SLIPs */
		0,					/* maximum number of links to visit/scanline (0=all) */

		0x00,				/* base palette entry */
		0x40,				/* maximum number of colors */
		15,					/* transparent pen index */

		{{ 0,0,0,0x07f8 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x07ff,0,0 }},	/* mask for the code index */
		{{ 0x0007,0,0,0 }},	/* mask for the upper code index */
		{{ 0,0,0,0x3000 }},	/* mask for the color */
		{{ 0,0,0xffc0,0 }},	/* mask for the X position */
		{{ 0x7fc0,0,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0x3800,0,0 }},	/* mask for the height, in tiles */
		{{ 0,0x4000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0,0xc000 }},	/* mask for the priority */
		{{ 0,0x8000,0,0 }},	/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the special value */
		0,					/* resulting value to indicate "special" */
		0					/* callback routine for special entries */
	};

	/* allocate banked memory */
	vram = auto_malloc(0x8000);
	memset(vram, 0, 0x8000);
	atarigen_alpha = &vram[0x0000];
	atarimo_0_spriteram = &vram[0x0c00];
	atarigen_playfield = &vram[0x2000];

	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 128,64);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		return 1;

	/* initialize the alphanumerics */
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,48);
	if (!atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);

	/* reset the statics */
	yscroll_reset_timer = timer_alloc(reset_yscroll_callback);
	videobank = 0;
	return 0;
}



/*************************************
 *
 *  Scroll/playfield bank write
 *
 *************************************/

WRITE16_HANDLER( atarisy2_xscroll_w )
{
	UINT16 oldscroll = *atarigen_xscroll;
	UINT16 newscroll = oldscroll;
	COMBINE_DATA(&newscroll);

	/* if anything has changed, force a partial update */
	if (newscroll != oldscroll)
		force_partial_update(cpu_getscanline());

	/* update the playfield scrolling - hscroll is clocked on the following scanline */
	tilemap_set_scrollx(atarigen_playfield_tilemap, 0, newscroll >> 6);

	/* update the playfield banking */
	if (playfield_tile_bank[0] != (newscroll & 0x0f) * 0x400)
	{
		playfield_tile_bank[0] = (newscroll & 0x0f) * 0x400;
		tilemap_mark_all_tiles_dirty(atarigen_playfield_tilemap);
	}

	/* update the data */
	*atarigen_xscroll = newscroll;
}


static void reset_yscroll_callback(int newscroll)
{
	tilemap_set_scrolly(atarigen_playfield_tilemap, 0, newscroll);
}


WRITE16_HANDLER( atarisy2_yscroll_w )
{
	UINT16 oldscroll = *atarigen_yscroll;
	UINT16 newscroll = oldscroll;
	COMBINE_DATA(&newscroll);

	/* if anything has changed, force a partial update */
	if (newscroll != oldscroll)
		force_partial_update(cpu_getscanline());

	/* if bit 4 is zero, the scroll value is clocked in right away */
	if (!(newscroll & 0x10))
		tilemap_set_scrolly(atarigen_playfield_tilemap, 0, (newscroll >> 6) - cpu_getscanline());
	else
		timer_adjust(yscroll_reset_timer, cpu_getscanlinetime(0), newscroll >> 6, 0);

	/* update the playfield banking */
	if (playfield_tile_bank[1] != (newscroll & 0x0f) * 0x400)
	{
		playfield_tile_bank[1] = (newscroll & 0x0f) * 0x400;
		tilemap_mark_all_tiles_dirty(atarigen_playfield_tilemap);
	}

	/* update the data */
	*atarigen_yscroll = newscroll;
}



/*************************************
 *
 *  Palette RAM write handler
 *
 *************************************/

WRITE16_HANDLER( atarisy2_paletteram_w )
{
	static const int intensity_table[16] =
	{
		#define ZB 115
		#define Z3 78
		#define Z2 37
		#define Z1 17
		#define Z0 9
		0, ZB+Z0, ZB+Z1, ZB+Z1+Z0, ZB+Z2, ZB+Z2+Z0, ZB+Z2+Z1, ZB+Z2+Z1+Z0,
		ZB+Z3, ZB+Z3+Z0, ZB+Z3+Z1, ZB+Z3+Z1+Z0,ZB+ Z3+Z2, ZB+Z3+Z2+Z0, ZB+Z3+Z2+Z1, ZB+Z3+Z2+Z1+Z0
	};
	static const int color_table[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xe, 0xf, 0xf };

	int newword, inten, red, green, blue;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	inten = intensity_table[newword & 15];
	red = (color_table[(newword >> 12) & 15] * inten) >> 4;
	green = (color_table[(newword >> 8) & 15] * inten) >> 4;
	blue = (color_table[(newword >> 4) & 15] * inten) >> 4;
	palette_set_color(offset, red, green, blue);
}



/*************************************
 *
 *  Video RAM bank read/write handlers
 *
 *************************************/

READ16_HANDLER( atarisy2_slapstic_r )
{
	int result = atarisy2_slapstic[offset];
	slapstic_tweak(offset);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak(0x1234) * 0x1000;
	return result;
}


WRITE16_HANDLER( atarisy2_slapstic_w )
{
	slapstic_tweak(offset);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak(0x1234) * 0x1000;
}



/*************************************
 *
 *  Video RAM read/write handlers
 *
 *************************************/

READ16_HANDLER( atarisy2_videoram_r )
{
	return vram[offset | videobank];
}


WRITE16_HANDLER( atarisy2_videoram_w )
{
	int offs = offset | videobank;

	/* alpharam? */
	if (offs < 0x0c00)
	{
		COMBINE_DATA(&atarigen_alpha[offs]);
		tilemap_mark_tile_dirty(atarigen_alpha_tilemap, offs);
	}

	/* spriteram? */
	else if (offs < 0x1000)
	{
		/* force an update if the link of object 0 is about to change */
		if (offs == 0x0c03)
			force_partial_update(cpu_getscanline());
		atarimo_0_spriteram_w(offs - 0x0c00, data, mem_mask);
	}

	/* playfieldram? */
	else if (offs >= 0x2000)
	{
		offs -= 0x2000;
		COMBINE_DATA(&atarigen_playfield[offs]);
		tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offs);
	}

	/* generic case */
	else
	{
		COMBINE_DATA(&vram[offs]);
	}
}



/*************************************
 *
 *  Main refresh
 *
 *************************************/

VIDEO_UPDATE( atarisy2 )
{
	struct atarimo_rect_list rectlist;
	mame_bitmap *mobitmap;
	int x, y, r;

	/* draw the playfield */
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 1, 1);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 2, 2);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 3, 3);

	/* draw and merge the MO */
	mobitmap = atarimo_render(0, cliprect, &rectlist);
	for (r = 0; r < rectlist.numrects; r++, rectlist.rect++)
		for (y = rectlist.rect->min_y; y <= rectlist.rect->max_y; y++)
		{
			UINT16 *mo = (UINT16 *)mobitmap->base + mobitmap->rowpixels * y;
			UINT16 *pf = (UINT16 *)bitmap->base + bitmap->rowpixels * y;
			UINT8 *pri = (UINT8 *)priority_bitmap->base + priority_bitmap->rowpixels * y;
			for (x = rectlist.rect->min_x; x <= rectlist.rect->max_x; x++)
				if (mo[x] != 0x0f)
				{
					int mopriority = mo[x] >> ATARIMO_PRIORITY_SHIFT;

					/* high priority PF? */
					if ((mopriority + pri[x]) & 2)
					{
						/* only gets priority if PF pen is less than 8 */
						if (!(pf[x] & 0x08))
							pf[x] = mo[x] & ATARIMO_DATA_MASK;
					}

					/* low priority */
					else
						pf[x] = mo[x] & ATARIMO_DATA_MASK;

					/* erase behind ourselves */
					mo[x] = 0x0f;
				}
		}

	/* add the alpha on top */
	tilemap_draw(bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);
}
