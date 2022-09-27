#include "driver.h"

#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

UINT8 *gunsmoke_scrollx;
UINT8 *gunsmoke_scrolly;

static int chon, objon, bgon;
static int sprite3bank;

static tilemap *bg_tilemap, *fg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Gunsmoke has three 256x4 palette PROMs (one per gun) and a lot ;-) of
  256x4 lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
PALETTE_INIT( gunsmoke )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;

		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;

		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;

		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i, r, g, b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */

	/* characters use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 64;
	color_prom += 128;	/* skip the bottom half of the PROM - not used */

	/* background tiles use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = color_prom[0] + 16 * (color_prom[256] & 0x03);
		color_prom++;
	}
	color_prom += TOTAL_COLORS(1);

	/* sprites use colors 128-255 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = color_prom[0] + 16 * (color_prom[256] & 0x07) + 128;
		color_prom++;
	}
	color_prom += TOTAL_COLORS(2);
}

WRITE8_HANDLER( gunsmoke_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( gunsmoke_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( gunsmoke_c804_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* bits 0 and 1 are for coin counters */
	coin_counter_w(1, data & 0x01);
	coin_counter_w(0, data & 0x02);

	/* bits 2 and 3 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0c) * 0x1000;
	memory_set_bankptr(1, &RAM[bankaddress]);

	/* bit 5 resets the sound CPU? - we ignore it */

	/* bit 6 flips screen */
	flip_screen_set(data & 0x40);

	/* bit 7 enables characters? */
	chon = data & 0x80;
}

WRITE8_HANDLER( gunsmoke_d806_w )
{
	/* bits 0-2 select the sprite 3 bank */
	sprite3bank = data & 0x07;

	/* bit 4 enables bg 1? */
	bgon = data & 0x10;

	/* bit 5 enables sprites? */
	objon = data & 0x20;
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX4);

	int offs = tile_index * 2;
	int attr = tilerom[offs + 1];
	int code = tilerom[offs] + ((attr & 0x01) << 8);
	int color = (attr & 0x3c) >> 2;
	int flags = TILE_FLIPYX((attr & 0xc0) >> 6);

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0xe0) << 2);
	int color = attr & 0x1f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( gunsmoke )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 2048, 8);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT_COLOR, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0x4f);

	return 0;
}

static void gunsmoke_draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;

	for (offs = spriteram_size - 32; offs >= 0; offs -= 32)
	{
		int attr = spriteram[offs + 1];
		int bank = (attr & 0xc0) >> 6;
		int code = spriteram[offs];
		int color = attr & 0x0f;
		int flipx = 0;
		int flipy = attr & 0x10;
		int sx = spriteram[offs + 3] - ((attr & 0x20) << 3);
		int sy = spriteram[offs + 2];

		if (bank == 3) bank += sprite3bank;
		code += 256 * bank;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[2], code, color, flipx, flipy,
			sx, sy, cliprect, TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( gunsmoke )
{
	tilemap_set_scrollx(bg_tilemap, 0, gunsmoke_scrollx[0] + 256 * gunsmoke_scrollx[1]);
	tilemap_set_scrolly(bg_tilemap, 0, gunsmoke_scrolly[0]);

	if (bgon)
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	else
		fillbitmap(bitmap, get_black_pen(), cliprect);

	if (objon) gunsmoke_draw_sprites(bitmap, cliprect);
	if (chon)  tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}
