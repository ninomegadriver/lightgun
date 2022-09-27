#include "driver.h"

UINT8 *skykid_textram, *skykid_videoram, *skykid_spriteram;

static tilemap *bg_tilemap,*tx_tilemap;
static int priority,scroll_x,scroll_y;


/***************************************************************************

    Convert the color PROMs.

    The palette PROMs are connected to the RGB output this way:

    bit 3   -- 220 ohm resistor  -- RED/GREEN/BLUE
            -- 470 ohm resistor  -- RED/GREEN/BLUE
            -- 1  kohm resistor  -- RED/GREEN/BLUE
    bit 0   -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( skykid )
{
	int i;
	int bit0,bit1,bit2,bit3,r,g,b;
	int totcolors = Machine->drv->total_colors;

	for (i = 0; i < totcolors; i++)
	{
		/* red component */
		bit0 = (color_prom[totcolors*0] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*0] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*0] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*0] >> 3) & 0x01;
		r = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* green component */
		bit0 = (color_prom[totcolors*1] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*1] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*1] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*1] >> 3) & 0x01;
		g = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* blue component */
		bit0 = (color_prom[totcolors*2] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*2] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*2] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*2] >> 3) & 0x01;
		b = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* text palette */
	for (i = 0; i < 64*4; i++)
		*(colortable++) = i;

	color_prom += 2*totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/* tiles lookup table */
	for (i = 0; i < 128*4; i++)
		*(colortable++) = *(color_prom++);

	/* sprites lookup table */
	for (i = 0;i < 64*8;i++)
		*(colortable++) = *(color_prom++);
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/* convert from 32x32 to 36x28 */
static UINT32 tx_tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	int offs;

	row += 2;
	col -= 2;
	if (col & 0x20)
		offs = row + ((col & 0x1f) << 5);
	else
		offs = col + (row << 5);

	return offs;
}

static void tx_get_tile_info(int tile_index)
{
	/* the hardware has two character sets, one normal and one flipped. When
       screen is flipped, character flip is done by selecting the 2nd character set.
       We reproduce this here, but since the tilemap system automatically flips
       characters when screen is flipped, we have to flip them back. */
	SET_TILE_INFO(
			0,
			skykid_textram[tile_index] | (flip_screen ? 0x100 : 0),
			skykid_textram[tile_index + 0x400] & 0x3f,
			flip_screen ? (TILE_FLIPY | TILE_FLIPX) : 0)
}


static void bg_get_tile_info(int tile_index)
{
	int code = skykid_videoram[tile_index];
	int attr = skykid_videoram[tile_index+0x800];

	SET_TILE_INFO(
			1,
			code + ((attr & 0x01) << 8),
			((attr & 0x7e) >> 1) | ((attr & 0x01) << 6),
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( skykid )
{
	tx_tilemap = tilemap_create(tx_get_tile_info,tx_tilemap_scan,  TILEMAP_TRANSPARENT,8,8,36,28);
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,32);

	if (!tx_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0);

	spriteram = skykid_spriteram + 0x780;
	spriteram_2 = spriteram + 0x0800;
	spriteram_3 = spriteram_2 + 0x0800;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ8_HANDLER( skykid_videoram_r )
{
	return skykid_videoram[offset];
}

WRITE8_HANDLER( skykid_videoram_w )
{
	if (skykid_videoram[offset] != data)
	{
		skykid_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset & 0x7ff);
	}
}

READ8_HANDLER( skykid_textram_r )
{
	return skykid_textram[offset];
}

WRITE8_HANDLER( skykid_textram_w )
{
	if (skykid_textram[offset] != data)
	{
		skykid_textram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0x3ff);
	}
}

WRITE8_HANDLER( skykid_scroll_x_w )
{
	scroll_x = offset;
}

WRITE8_HANDLER( skykid_scroll_y_w )
{
	scroll_y = offset;
}

WRITE8_HANDLER( skykid_flipscreen_priority_w )
{
	priority = data;
	flip_screen_set(offset & 1);
}



/***************************************************************************

  Display Refresh

***************************************************************************/

/* the sprite generator IC is the same as Mappy */
static void skykid_draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < 0x80;offs += 2)
	{
		static int gfx_offs[2][2] =
		{
			{ 0, 1 },
			{ 2, 3 }
		};
		int sprite = spriteram[offs] + ((spriteram_3[offs] & 0x80) << 1);
		int color = (spriteram[offs+1] & 0x3f);
		int sx = (spriteram_2[offs+1]) + 0x100*(spriteram_3[offs+1] & 1) - 71;
		int sy = 256 - spriteram_2[offs] - 7;
		int flipx = (spriteram_3[offs] & 0x01);
		int flipy = (spriteram_3[offs] & 0x02) >> 1;
		int sizex = (spriteram_3[offs] & 0x04) >> 2;
		int sizey = (spriteram_3[offs] & 0x08) >> 3;
		int x,y;

		sprite &= ~sizex;
		sprite &= ~(sizey << 1);

		if (flip_screen)
		{
			flipx ^= 1;
			flipy ^= 1;
		}

		sy -= 16 * sizey;
		sy = (sy & 0xff) - 32;	// fix wraparound

		for (y = 0;y <= sizey;y++)
		{
			for (x = 0;x <= sizex;x++)
			{
				drawgfx(bitmap,Machine->gfx[2],
					sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
					color,
					flipx,flipy,
					sx + 16*x,sy + 16*y,
					cliprect,TRANSPARENCY_COLOR,0xff);
			}
		}
	}
}


VIDEO_UPDATE( skykid )
{
	if (flip_screen)
	{
		tilemap_set_scrollx(bg_tilemap, 0, 189 - (scroll_x ^ 1));
		tilemap_set_scrolly(bg_tilemap, 0, 7 - scroll_y);
	}
	else
	{
		tilemap_set_scrollx(bg_tilemap, 0, scroll_x + 35);
		tilemap_set_scrolly(bg_tilemap, 0, scroll_y + 25);
	}

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	if ((priority & 0xf0) != 0x50)
		skykid_draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);

	if ((priority & 0xf0) == 0x50)
		skykid_draw_sprites(bitmap,cliprect);
}
