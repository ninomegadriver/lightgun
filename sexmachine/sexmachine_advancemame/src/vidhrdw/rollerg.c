#include "driver.h"
#include "vidhrdw/konamiic.h"


static int bg_colorbase,sprite_colorbase,zoom_colorbase;



/***************************************************************************

  Callbacks for the K053245

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority_mask)
{
#if 0
if (code_pressed(KEYCODE_Q) && (*color & 0x80)) *color = rand();
if (code_pressed(KEYCODE_W) && (*color & 0x40)) *color = rand();
if (code_pressed(KEYCODE_E) && (*color & 0x20)) *color = rand();
if (code_pressed(KEYCODE_R) && (*color & 0x10)) *color = rand();
#endif
	*priority_mask = (*color & 0x10) ? 0 : 0x02;
	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

static void zoom_callback(int *code,int *color)
{
	tile_info.flags = TILE_FLIPYX((*color & 0xc0) >> 6);
	*code |= ((*color & 0x0f) << 8);
	*color = zoom_colorbase + ((*color & 0x30) >> 4);
}



/***************************************************************************

    Start the video hardware emulation.

***************************************************************************/

VIDEO_START( rollerg )
{
	bg_colorbase = 16;
	sprite_colorbase = 16;
	zoom_colorbase = 0;

	if (K053245_vh_start(0,REGION_GFX1,NORMAL_PLANE_ORDER,sprite_callback))
		return 1;
	if (K051316_vh_start_0(REGION_GFX2,4,TILEMAP_TRANSPARENT,0,zoom_callback))
		return 1;

	K051316_set_offset(0, 22, 1);
	return 0;
}



/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( rollerg )
{
	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],cliprect);
	K051316_zoom_draw_0(bitmap,cliprect,0,1);
	K053245_sprites_draw(0,bitmap,cliprect);
}
