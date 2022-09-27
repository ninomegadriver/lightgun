#include "driver.h"
#include "vidhrdw/konamiic.h"

static int layer_colorbase[2];
extern int bladestl_spritebank;

PALETTE_INIT( bladestl )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	/* build the lookup table for sprites. Palette is dynamic. */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = 0x20 + (*(color_prom++) & 0x0f);
}

/***************************************************************************

  Callback for the K007342

***************************************************************************/

static void tile_callback(int layer, int bank, int *code, int *color)
{
	*code |= ((*color & 0x0f) << 8) | ((*color & 0x40) << 6);
	*color = layer_colorbase[layer];
}

/***************************************************************************

  Callback for the K007420

***************************************************************************/

static void sprite_callback(int *code,int *color)
{
	*code |= ((*color & 0xc0) << 2) + bladestl_spritebank;
	*code = (*code << 2) | ((*color & 0x30) >> 4);
	*color = 0 + (*color & 0x0f);
}


/***************************************************************************

    Start the video hardware emulation.

***************************************************************************/

VIDEO_START( bladestl )
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 1;

	if (K007342_vh_start(0,tile_callback))
		return 1;

	if (K007420_vh_start(1,sprite_callback))
		return 1;

	return 0;
}

/***************************************************************************

  Screen Refresh

***************************************************************************/

VIDEO_UPDATE( bladestl )
{
	K007342_tilemap_update();

	K007342_tilemap_draw( bitmap, cliprect, 1, TILEMAP_IGNORE_TRANSPARENCY ,0);
	K007420_sprites_draw( bitmap, cliprect );
	K007342_tilemap_draw( bitmap, cliprect, 1, 1 | TILEMAP_IGNORE_TRANSPARENCY ,0);
	K007342_tilemap_draw( bitmap, cliprect, 0, 0 ,0);
	K007342_tilemap_draw( bitmap, cliprect, 0, 1 ,0);
}
