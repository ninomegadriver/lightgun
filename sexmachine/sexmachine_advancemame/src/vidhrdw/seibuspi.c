#include "driver.h"

static tilemap *text_layer;
static tilemap *back_layer;
static tilemap *mid_layer;
static tilemap *fore_layer;

UINT32 *spi_scrollram;

static UINT32 layer_bank;
static UINT32 layer_enable;
static UINT32 video_dma_length;
static UINT32 video_dma_address;

static int rf2_layer_bank[3];
extern UINT32 *spimainram;
static UINT32 *tilemap_ram;
static UINT32 *palette_ram;
static UINT32 *sprite_ram;

static int mid_layer_offset;
static int fore_layer_offset;
static int text_layer_offset;

static UINT32 bg_fore_layer_position;

static UINT8 alpha_table[6144];

READ32_HANDLER( spi_layer_bank_r )
{
	return layer_bank;
}

WRITE32_HANDLER( spi_layer_bank_w )
{
	COMBINE_DATA( &layer_bank );

	if (layer_bank & 0x80000000) {
		fore_layer_offset = 0x1000 / 4;
		mid_layer_offset = 0x2000 / 4;
		text_layer_offset = 0x3000 / 4;
	}
	else {
		fore_layer_offset = 0x800 / 4;
		mid_layer_offset = 0x1000 / 4;
		text_layer_offset = 0x1800 / 4;
	}
}

void rf2_set_layer_banks(int banks)
{
	if (rf2_layer_bank[0] != BIT(banks,0))
	{
		rf2_layer_bank[0] = BIT(banks,0);
		tilemap_mark_all_tiles_dirty(back_layer);
	}

	if (rf2_layer_bank[1] != BIT(banks,1))
	{
		rf2_layer_bank[1] = BIT(banks,1);
		tilemap_mark_all_tiles_dirty(mid_layer);
	}

	if (rf2_layer_bank[2] != BIT(banks,2))
	{
		rf2_layer_bank[2] = BIT(banks,2);
		tilemap_mark_all_tiles_dirty(fore_layer);
	}
}

READ32_HANDLER( spi_layer_enable_r )
{
	return layer_enable;
}

WRITE32_HANDLER( spi_layer_enable_w )
{
	COMBINE_DATA( &layer_enable );
	tilemap_set_enable(back_layer, (layer_enable & 0x1) ^ 0x1);
	tilemap_set_enable(mid_layer, ((layer_enable >> 1) & 0x1) ^ 0x1);
	tilemap_set_enable(fore_layer, ((layer_enable >> 2) & 0x1) ^ 0x1);
}

WRITE32_HANDLER( tilemap_dma_start_w )
{
	int i;
	int index = (video_dma_address / 4) - 0x200;

	if (layer_bank & 0x80000000)
	{
		/* back layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i] != tile) {
				tilemap_ram[i] = tile;
				tilemap_mark_tile_dirty( back_layer, (i * 2) );
				tilemap_mark_tile_dirty( back_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* back layer row scroll */
		memcpy(&tilemap_ram[0x800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* fore layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+fore_layer_offset] != tile) {
				tilemap_ram[i+fore_layer_offset] = tile;
				tilemap_mark_tile_dirty( fore_layer, (i * 2) );
				tilemap_mark_tile_dirty( fore_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* fore layer row scroll */
		memcpy(&tilemap_ram[0x1800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* mid layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+mid_layer_offset] != tile) {
				tilemap_ram[i+mid_layer_offset] = tile;
				tilemap_mark_tile_dirty( mid_layer, (i * 2) );
				tilemap_mark_tile_dirty( mid_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* mid layer row scroll */
		memcpy(&tilemap_ram[0x1800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* text layer */
		for (i=0; i < 0x1000/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+text_layer_offset] != tile) {
				tilemap_ram[i+text_layer_offset] = tile;
				tilemap_mark_tile_dirty( text_layer, (i * 2) );
				tilemap_mark_tile_dirty( text_layer, (i * 2) + 1 );
			}
			index++;
		}
	}
	else
	{
		/* back layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i] != tile) {
				tilemap_ram[i] = tile;
				tilemap_mark_tile_dirty( back_layer, (i * 2) );
				tilemap_mark_tile_dirty( back_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* fore layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+fore_layer_offset] != tile) {
				tilemap_ram[i+fore_layer_offset] = tile;
				tilemap_mark_tile_dirty( fore_layer, (i * 2) );
				tilemap_mark_tile_dirty( fore_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* mid layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+mid_layer_offset] != tile) {
				tilemap_ram[i+mid_layer_offset] = tile;
				tilemap_mark_tile_dirty( mid_layer, (i * 2) );
				tilemap_mark_tile_dirty( mid_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* text layer */
		for (i=0; i < 0x1000/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+text_layer_offset] != tile) {
				tilemap_ram[i+text_layer_offset] = tile;
				tilemap_mark_tile_dirty( text_layer, (i * 2) );
				tilemap_mark_tile_dirty( text_layer, (i * 2) + 1 );
			}
			index++;
		}
	}
}

WRITE32_HANDLER( palette_dma_start_w )
{
	int i;
	for (i=0; i < ((video_dma_length+1) * 2) / 4; i++)
	{
		int r1,g1,b1,r2,g2,b2;

		UINT32 color = spimainram[(video_dma_address / 4) + i - 0x200];
		if (palette_ram[i] != color) {
			palette_ram[i] = color;
			b1 = ((palette_ram[i] & 0x7c000000) >>26) << 3;
			g1 = ((palette_ram[i] & 0x03e00000) >>21) << 3;
			r1 = ((palette_ram[i] & 0x001f0000) >>16) << 3;
			b2 = ((palette_ram[i] & 0x00007c00) >>10) << 3;
			g2 = ((palette_ram[i] & 0x000003e0) >>5) << 3;
			r2 = ((palette_ram[i] & 0x0000001f) >>0) << 3;

			palette_set_color( (i * 2), r2, g2, b2 );
			palette_set_color( (i * 2) + 1, r1, g1, b1 );
		}
	}
}

WRITE32_HANDLER( sprite_dma_start_w )
{
	memcpy( sprite_ram, &spimainram[(video_dma_address / 4) - 0x200], 0x1000);
}

WRITE32_HANDLER( video_dma_length_w )
{
	COMBINE_DATA( &video_dma_length );
}

WRITE32_HANDLER( video_dma_address_w )
{
	COMBINE_DATA( &video_dma_address );
}

static void draw_blend_gfx(mame_bitmap *bitmap, const rectangle *cliprect, const gfx_element *gfx, unsigned int code, unsigned int color, int flipx, int flipy, int sx, int sy)
{
	UINT8 *dp;
	int i, j;
	int x1, x2;
	int y1, y2;
	int px, py;
	int xd = 1, yd = 1;

	int width = gfx->width;
	int height = gfx->height;

	x1 = sx;
	x2 = sx + width - 1;
	y1 = sy;
	y2 = sy + height - 1;

	if (x1 > cliprect->max_x || x2 < cliprect->min_x)
	{
		return;
	}
	if (y1 > cliprect->max_y || y2 < cliprect->min_y)
	{
		return;
	}

	px = 0;
	py = 0;

	if (flipx)
	{
		xd = -xd;
		px = width - 1;
	}
	if (flipy)
	{
		yd = -yd;
		py = height - 1;
	}

	// clip x
	if (x1 < cliprect->min_x)
	{
		if (flipx)
		{
			px = width - (cliprect->min_x - x1) - 1;
		}
		else
		{
			px = (cliprect->min_x - x1);
		}
		x1 = cliprect->min_x;
	}
	if (x2 > cliprect->max_x)
	{
		x2 = cliprect->max_x;
	}

	// clip y
	if (y1 < cliprect->min_y)
	{
		if (flipy)
		{
			py = height - (cliprect->min_y - y1) - 1;
		}
		else
		{
			py = (cliprect->min_y - y1);
		}
		y1 = cliprect->min_y;
	}
	if (y2 > cliprect->max_y)
	{
		y2 = cliprect->max_y;
	}

	if (gfx->total_elements <= 0x10000)
	{
		code &= 0xffff;
	}

	dp = gfx->gfxdata + code * gfx->char_modulo;

	// draw
	for (j=y1; j <= y2; j++)
	{
		UINT16 *p = bitmap->line[j];
		int dp_i = (py * width) + px;
		py += yd;

		for (i=x1; i <= x2; i++)
		{
			UINT8 pen = dp[dp_i];
			if (pen != 0x3f)
			{
				int global_pen = pen + color*64;
				UINT8 alpha = alpha_table[global_pen];
				if (alpha)
				{
					p[i] = alpha_blend16(p[i], gfx->colortable[global_pen]);
				}
				else
				{
					p[i] = gfx->colortable[global_pen];
				}
			}
			dp_i += xd;
		}
	}
}

static int sprite_xtable[2][8] =
{
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 }
};
static int sprite_ytable[2][8] =
{
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 }
};

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int pri_mask)
{
	INT16 xpos, ypos;
	int tile_num, color;
	int width, height;
	int flip_x = 0, flip_y = 0;
	int a;
	int priority;
	int x,y, x1, y1;
	const gfx_element *gfx = Machine->gfx[2];

	if( layer_enable & 0x10 )
		return;

	for( a = 0x400 - 2; a >= 0; a -= 2 ) {
		tile_num = (sprite_ram[a + 0] >> 16) & 0xffff;
		if( sprite_ram[a + 1] & 0x1000 )
			tile_num |= 0x10000;

		if( !tile_num )
			continue;

		priority = (sprite_ram[a + 0] >> 6) & 0x3;
		if( pri_mask != priority )
			continue;

		xpos = sprite_ram[a + 1] & 0x3ff;
		if( xpos & 0x200 )
			xpos |= 0xfc00;
		ypos = (sprite_ram[a + 1] >> 16) & 0x1ff;
		if( ypos & 0x100 )
			ypos |= 0xfe00;
		color = (sprite_ram[a + 0] & 0x3f);

		width = ((sprite_ram[a + 0] >> 8) & 0x7) + 1;
		height = ((sprite_ram[a + 0] >> 12) & 0x7) + 1;
		flip_x = (sprite_ram[a + 0] >> 11) & 0x1;
		flip_y = (sprite_ram[a + 0] >> 15) & 0x1;
		x1 = 0;
		y1 = 0;

		if( flip_x ) {
			x1 = 8 - width;
			width = width + x1;
		}
		if( flip_y ) {
			y1 = 8 - height;
			height = height + y1;
		}

		for( x=x1; x < width; x++ ) {
			for( y=y1; y < height; y++ ) {
				draw_blend_gfx(bitmap, cliprect, gfx, tile_num, color, flip_x, flip_y, xpos + sprite_xtable[flip_x][x], ypos + sprite_ytable[flip_y][y]);

				/* xpos seems to wrap-around to 0 at 512 */
				if( (xpos + (16 * x) + 16) >= 512 ) {
					draw_blend_gfx(bitmap, cliprect, gfx, tile_num, color, flip_x, flip_y, xpos - 512 + sprite_xtable[flip_x][x], ypos + sprite_ytable[flip_y][y]);
				}

				tile_num++;
			}
		}
	}
}

static void get_text_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs + text_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0, tile, color, 0)
}

static void get_back_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;

	if( rf2_layer_bank[0] )
		tile |= 0x4000;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs + mid_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;
	tile |= 0x2000;

	if( rf2_layer_bank[1] )
		tile |= 0x4000;

	SET_TILE_INFO(1, tile, color + 16, 0)
}

static void get_fore_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs + fore_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;
	tile |= bg_fore_layer_position;

	if( rf2_layer_bank[2] )
		tile |= 0x4000;

	tile |= ((layer_bank >> 27) & 0x1) << 13;

	SET_TILE_INFO(1, tile, color + 8, 0)
}

VIDEO_START( spi )
{
	int i;
	int region_length;

	text_layer	= tilemap_create( get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32 );
	back_layer	= tilemap_create( get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,16,32,32 );
	mid_layer	= tilemap_create( get_mid_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );
	fore_layer	= tilemap_create( get_fore_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );

	tilemap_set_transparent_pen(text_layer, 31);
	tilemap_set_transparent_pen(mid_layer, 63);
	tilemap_set_transparent_pen(fore_layer, 63);

	tilemap_ram = auto_malloc(0x4000);
	palette_ram = auto_malloc(0x3000);
	sprite_ram = auto_malloc(0x1000);
	memset(tilemap_ram, 0, 0x4000);
	memset(palette_ram, 0, 0x3000);
	memset(sprite_ram, 0, 0x1000);

	for (i=0; i < 6144; i++) {
		palette_set_color(i, 0, 0, 0);
	}

	memset(alpha_table, 0, 6144 * sizeof(UINT8));

	// sprites
	//for (i = 1792; i < 1808; i++) { alpha_table[i] = 1; } // breaks rdft
	for (i = 1840; i < 1856; i++) { alpha_table[i] = 1; }
	for (i = 1920; i < 1952; i++) { alpha_table[i] = 1; }
	//for (i = 1984; i < 2048; i++) { alpha_table[i] = 1; } // breaks batlball
	//for (i = 3840; i < 3904; i++) { alpha_table[i] = 1; } // breaks rdft
	for (i = 4032; i < 4096; i++) { alpha_table[i] = 1; }

	// mid layer
	for (i = 4960; i < 4992; i++) { alpha_table[i] = 1; }	// breaks ejanhs
	for (i = 5040; i < 5056; i++) { alpha_table[i] = 1; }	// breaks ejanhs
	for (i = 5104; i < 5120; i++) { alpha_table[i] = 1; }
	// fore layer
	for (i = 5552; i < 5568; i++) { alpha_table[i] = 1; }	// breaks ejanhs
	for (i = 5616; i < 5632; i++) { alpha_table[i] = 1; }	// breaks ejanhs
	// text layer
	for (i = 6000; i < 6016; i++) { alpha_table[i] = 1; }
	for (i = 6128; i < 6144; i++) { alpha_table[i] = 1; }

	region_length = memory_region_length(REGION_GFX2);

	if (region_length <= 0x300000)
	{
		bg_fore_layer_position = 0x2000;
	}
	else if (region_length <= 0x600000)
	{
		bg_fore_layer_position = 0x4000;
	}
	else
	{
		bg_fore_layer_position = 0x8000;
	}

	return 0;
}

static void set_rowscroll(tilemap *layer, int scroll, INT16* rows)
{
	int i;
	int x = spi_scrollram[scroll] & 0xffff;
	int y = (spi_scrollram[scroll] >> 16) & 0xffff;
	tilemap_set_scroll_rows(layer, 512);
	for( i=0; i < 512; i++ ) {
		tilemap_set_scrollx(layer, i, x + rows[i]);
	}
	tilemap_set_scrolly(layer, 0, y);
}

static void set_scroll(tilemap *layer, int scroll)
{
	int x = spi_scrollram[scroll] & 0xffff;
	int y = (spi_scrollram[scroll] >> 16) & 0xffff;
	tilemap_set_scrollx(layer, 0, x);
	tilemap_set_scrolly(layer, 0, y);
}


static void combine_tilemap(mame_bitmap *bitmap, const rectangle *cliprect, tilemap *tile, int x, int y, int opaque, INT16 *rowscroll)
{
	int i,j;
	UINT16 *s;
	UINT16 *d;
	UINT8 *t;
	UINT32 xscroll_mask, yscroll_mask;
	mame_bitmap *pen_bitmap;
	mame_bitmap *trans_bitmap;

	pen_bitmap = tilemap_get_pixmap(tile);
	trans_bitmap = tilemap_get_transparency_bitmap(tile);
	xscroll_mask = pen_bitmap->width - 1;
	yscroll_mask = pen_bitmap->height - 1;

	alpha_set_level(0x7f);

	for (j=cliprect->min_y; j <= cliprect->max_y; j++)
	{
		int rx = x;
		if (rowscroll)
		{
			rx += rowscroll[(j+y) & yscroll_mask];
		}

		d = bitmap->line[j];
		s = pen_bitmap->line[(j+y) & yscroll_mask];
		t = trans_bitmap->line[(j+y) & yscroll_mask];
		for (i=cliprect->min_x+rx; i <= cliprect->max_x+rx; i++)
		{
			if (opaque || (t[i & xscroll_mask] & (TILE_FLAG_FG_OPAQUE | TILE_FLAG_BG_OPAQUE)))
			{
				UINT16 pen = s[i & xscroll_mask];
				UINT8 alpha = alpha_table[pen];
				if (alpha)
				{
					*d = alpha_blend16(*d, Machine->remapped_colortable[pen]);
				}
				else
				{
					*d = Machine->remapped_colortable[pen];
				}
			}
			++d;
		}
	}
}



VIDEO_UPDATE( spi )
{
	INT16 *back_rowscroll, *mid_rowscroll, *fore_rowscroll;
	if( layer_bank & 0x80000000 ) {
		back_rowscroll	= (INT16*)&tilemap_ram[0x200];
		mid_rowscroll	= (INT16*)&tilemap_ram[0x600];
		fore_rowscroll	= (INT16*)&tilemap_ram[0xa00];
	} else {
		back_rowscroll	= NULL;
		mid_rowscroll	= NULL;
		fore_rowscroll	= NULL;
	}

	if( layer_enable & 0x1 )
		fillbitmap(bitmap, 0, cliprect);

	if (!(layer_enable & 0x1))
	{
		combine_tilemap(bitmap, cliprect, back_layer, spi_scrollram[0] & 0xffff, (spi_scrollram[0] >> 16) & 0xffff, 1, back_rowscroll);
	}

	draw_sprites(bitmap, cliprect, 0);

	// if fore layer is enabled, draw priority 1 sprites behind mid layer
	if (!(layer_enable & 0x4))
	{
		draw_sprites(bitmap, cliprect, 1);
	}

	if (!(layer_enable & 0x2))
	{
		combine_tilemap(bitmap, cliprect, mid_layer, spi_scrollram[1] & 0xffff, (spi_scrollram[1] >> 16) & 0xffff, 0, mid_rowscroll);
	}

	// if fore layer is disabled, draw priority 1 sprites above mid layer
	if ((layer_enable & 0x4))
	{
		draw_sprites(bitmap, cliprect, 1);
	}

	draw_sprites(bitmap, cliprect, 2);

	if (!(layer_enable & 0x4))
	{
		combine_tilemap(bitmap, cliprect, fore_layer, spi_scrollram[2] & 0xffff, (spi_scrollram[2] >> 16) & 0xffff, 0, fore_rowscroll);
	}

	draw_sprites(bitmap, cliprect, 3);

	combine_tilemap(bitmap, cliprect, text_layer, 0, 0, 0, NULL);
}
