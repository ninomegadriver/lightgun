#include "driver.h"
#include "cpu/z80/z80.h"
#include "snk.h"

static int flipscreen, sprite_flip_adjust;
static tilemap *fg_tilemap, *bg_tilemap, *tx_tilemap;
static UINT8 bg_color, fg_color, old_bg_color, old_fg_color;
static rectangle tilemap_clip;

/***************************************************************************
**
**  Palette Handling:
**
**  Each color entry is encoded by 12 bits in color proms
**
**  There are sixteen 8-color sprite palettes
**  sprite palette is selected by a nibble of spriteram
**
**  There are eight 16-color text layer character palettes
**  character palette is determined by character_number
**
**  Background and Foreground tilemap layers each have eight 16-color
**  palettes.  A palette bank select register is associated with the whole
**  layer.
**
***************************************************************************/

WRITE8_HANDLER( marvins_palette_bank_w )
{
	bg_color = data>>4;
	fg_color = data&0xf;
}

static void stuff_palette( int source_index, int dest_index, int num_colors )
{
	UINT8 *color_prom = memory_region(REGION_PROMS) + source_index;
	int i;
	for( i=0; i<num_colors; i++ )
	{
		int bit0=0,bit1,bit2,bit3;
		int red, green, blue;

		bit0 = (color_prom[0x800] >> 2) & 0x01; // ?
		bit1 = (color_prom[0x000] >> 1) & 0x01;
		bit2 = (color_prom[0x000] >> 2) & 0x01;
		bit3 = (color_prom[0x000] >> 3) & 0x01;
		red = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x800] >> 1) & 0x01; // ?
		bit1 = (color_prom[0x400] >> 2) & 0x01;
		bit2 = (color_prom[0x400] >> 3) & 0x01;
		bit3 = (color_prom[0x000] >> 0) & 0x01;
		green = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0x800] >> 0) & 0x01; // ?
		bit1 = (color_prom[0x800] >> 3) & 0x01; // ?
		bit2 = (color_prom[0x400] >> 0) & 0x01;
		bit3 = (color_prom[0x400] >> 1) & 0x01;
		blue = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color( dest_index++, red, green, blue );
		color_prom++;
	}

	for (i=0; i<=5; i++) gfx_drawmode_table[i] = DRAWMODE_SOURCE;

	gfx_drawmode_table[6] = DRAWMODE_SHADOW;
	gfx_drawmode_table[7] = DRAWMODE_NONE;
}

static void update_palette( int type )
{
	if( bg_color!=old_bg_color )
	{
		stuff_palette( 256+16*(bg_color&0x7), (0x11-type)*16, 16 );
		old_bg_color = bg_color;
	}

	if( fg_color!=old_fg_color )
	{
		stuff_palette( 128+16*(fg_color&0x7), (0x10+type)*16, 16 );
		old_fg_color = fg_color;
	}
}

/***************************************************************************
**
**  Memory Handlers
**
***************************************************************************/

WRITE8_HANDLER( marvins_spriteram_w )
{
	spriteram[offset] = data;
}

WRITE8_HANDLER( marvins_foreground_ram_w )
{
	if (offset < 0x800 && spriteram_2[offset] != data) tilemap_mark_tile_dirty(fg_tilemap,offset);

	spriteram_2[offset] = data;
}

WRITE8_HANDLER( marvins_background_ram_w )
{
	if (offset < 0x800 && spriteram_3[offset] != data) tilemap_mark_tile_dirty(bg_tilemap,offset);

	spriteram_3[offset] = data;
}

WRITE8_HANDLER( marvins_text_ram_w )
{
	if (offset < 0x400 && videoram[offset] != data) tilemap_mark_tile_dirty(tx_tilemap,offset);

	videoram[offset] = data;
}

/***************************************************************************
**
**  Callbacks for Tilemap Manager
**
***************************************************************************/

static void get_bg_tilemap_info(int tile_index)
{
	SET_TILE_INFO(
			2,
			spriteram_3[tile_index],
			0,
			0)
}

static void get_fg_tilemap_info(int tile_index)
{
	SET_TILE_INFO(
			1,
			spriteram_2[tile_index],
			0,
			0)
}

static void get_tx_tilemap_info(int tile_index)
{
	int tile_number = videoram[tile_index];
	SET_TILE_INFO(
			0,
			tile_number,
			(tile_number>>5),
			0)
}

/***************************************************************************
**
**  Video Initialization
**
***************************************************************************/

VIDEO_START( marvins )
{
	flipscreen = -1; old_bg_color = old_fg_color = -1;

	stuff_palette( 0, 0, 16*8 ); /* load sprite colors */
	stuff_palette( 16*8*3, 16*8, 16*8 ); /* load text colors */

	fg_tilemap = tilemap_create(get_fg_tilemap_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	bg_tilemap = tilemap_create(get_bg_tilemap_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	tx_tilemap = tilemap_create(get_tx_tilemap_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fg_tilemap || !bg_tilemap || !tx_tilemap)
		return 1;

	{
		tilemap_clip = Machine->visible_area;
		if (snk_gamegroup != 1) // not Mad Crasher
		{
			tilemap_clip.max_x-=16;
			tilemap_clip.min_x+=16;
		}

		tilemap_set_transparent_pen(fg_tilemap,0xf);
		tilemap_set_transparent_pen(bg_tilemap,0xf);
		tilemap_set_transparent_pen(tx_tilemap,0xf);

		switch (snk_gamegroup)
		{
			case 0:	// Marvin's Maze
				tilemap_set_scrolldx( bg_tilemap, 271, 287 );
				tilemap_set_scrolldy( bg_tilemap, 0, -40 );

				tilemap_set_scrolldx( fg_tilemap, 15, 13+18 );
				tilemap_set_scrolldy( fg_tilemap, 0, -40 );

				sprite_flip_adjust = 256+182+1;
			break;

			/*
                Old settings for the the following games:

                tilemap_set_scrolldx( bg_tilemap, -16, -10 );
                tilemap_set_scrolldy( bg_tilemap,   0, -40 );

                tilemap_set_scrolldx( fg_tilemap,  16,  22 );
                tilemap_set_scrolldy( fg_tilemap,   0, -40 );

                Note that while the new settings are more accurate they cannot handle flipscreen.
            */
			case 1:	// Mad Crasher
				tilemap_set_scrolldx( bg_tilemap,256,  0 );
				tilemap_set_scrolldy( bg_tilemap, 12,  0 );

				tilemap_set_scrolldx( fg_tilemap,  0,  0 );
				tilemap_set_scrolldy( fg_tilemap,  6,  0 );
			break;

			case 2:	// VanguardII
				tilemap_set_scrolldx( bg_tilemap,  7,  0 );
				tilemap_set_scrolldy( bg_tilemap,-20,  0 );

				tilemap_set_scrolldx( fg_tilemap, 15,  0 );
				tilemap_set_scrolldy( fg_tilemap,  0,  0 );

				sprite_flip_adjust = 256+182;
			break;
		}

		tilemap_set_scrolldx( tx_tilemap, 16, 16 );
		tilemap_set_scrolldy( tx_tilemap, 0, 0 );

		return 0;
	}
}

/***************************************************************************
**
**  Screen Refresh
**
***************************************************************************/

static void draw_status( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const UINT8 *base = videoram+0x400;
	const gfx_element *gfx = Machine->gfx[0];
	int row;
	for( row=0; row<4; row++ )
	{
		int sy,sx = (row&1)*8;
		const UINT8 *source = base + (row&1)*32;
		if( row>1 )
		{
			sx+=256+16;
		}
		else
		{
			source+=30*32;
		}

		for( sy=0; sy<256; sy+=8 )
		{
			int tile_number = *source++;
			drawgfx( bitmap, gfx,
			    tile_number, tile_number>>5,
			    0,0, /* no flip */
			    sx,sy,
			    cliprect,
			    TRANSPARENCY_NONE, 0xf );
		}
	}
}

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect, int scrollx, int scrolly,
		int priority, UINT8 sprite_partition )
{
	const gfx_element *gfx = Machine->gfx[3];
	const UINT8 *source, *finish;

	if( sprite_partition>0x64 ) sprite_partition = 0x64;

	if( priority )
	{
		source = spriteram + sprite_partition;
		finish = spriteram + 0x64;
	}
	else
	{
		source = spriteram;
		finish = spriteram + sprite_partition;
	}

	while( source<finish )
	{
		int attributes = source[3]; /* Y?F? CCCC */
		int tile_number = source[1];
		int sy = source[0] - scrolly;
		int sx = source[2] - scrollx + ((attributes&0x80)?256:0);
		int color = attributes&0xf;
		int flipy = (attributes&0x20);
		int flipx = 0;

		if( flipscreen )
		{
			if( flipy )
			{
			    flipx = 1; flipy = 0;
			}
			else
			{
			    flipx = flipy = 1;
			}
			sx = sprite_flip_adjust-sx;
			sy = 246-sy;
		}

		sx = (256-sx) & 0x1ff;
		sy = sy & 0xff;
		if (sx > 512-16) sx -= 512;
		if (sy > 256-16) sy -= 256;

		drawgfx( bitmap,gfx,
			tile_number,
			color,
			flipx, flipy,
			sx, sy,
			cliprect,TRANSPARENCY_PEN_TABLE,7);

		source+=4;
	}
}

VIDEO_UPDATE( marvins )
{
	cpuintrf_push_context(0);
	{
		UINT8 sprite_partition = program_read_byte(0xfe00);

		int attributes = program_read_byte(0x8600); /* 0x20: normal, 0xa0: video flipped */
		int scroll_attributes = program_read_byte(0xff00);
		int sprite_scrolly = program_read_byte(0xf800);
		int sprite_scrollx = program_read_byte(0xf900);

		int bg_scrolly = program_read_byte(0xfa00);
		int bg_scrollx = program_read_byte(0xfb00);
		int fg_scrolly = program_read_byte(0xfc00);
		int fg_scrollx = program_read_byte(0xfd00);

		rectangle finalclip = tilemap_clip;
		sect_rect(&finalclip, cliprect);

		if( (scroll_attributes & 4)==0 ) bg_scrollx += 256;
		if( scroll_attributes & 1 ) sprite_scrollx += 256;
		if( scroll_attributes & 2 ) fg_scrollx += 256;

		/* palette bank for background/foreground is set by a memory-write handler */
		update_palette(0);

		if( flipscreen != (attributes&0x80) )
		{
			flipscreen = attributes&0x80;
			tilemap_set_flip( ALL_TILEMAPS, flipscreen?TILEMAP_FLIPY|TILEMAP_FLIPX:0);
		}

		tilemap_set_scrollx( bg_tilemap,    0, bg_scrollx );
		tilemap_set_scrolly( bg_tilemap,    0, bg_scrolly );
		tilemap_set_scrollx( fg_tilemap,    0, fg_scrollx );
		tilemap_set_scrolly( fg_tilemap,    0, fg_scrolly );
		tilemap_set_scrollx( tx_tilemap,  0, 0 );
		tilemap_set_scrolly( tx_tilemap,  0, 0 );

		tilemap_draw( bitmap,&finalclip,fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY ,0);
		draw_sprites( bitmap,cliprect, sprite_scrollx+29+1, sprite_scrolly+16, 0, sprite_partition );
		tilemap_draw( bitmap,&finalclip,bg_tilemap,0 ,0);
		draw_sprites( bitmap,cliprect, sprite_scrollx+29+1, sprite_scrolly+16, 1, sprite_partition );
		tilemap_draw( bitmap,&finalclip,tx_tilemap,0 ,0);
		draw_status( bitmap,cliprect );
	}
	cpuintrf_pop_context();
}

VIDEO_UPDATE( madcrash )
{
/***************************************************************************
**
**  Game Specific Initialization
**
**  madcrash_vreg defines an offset for the video registers which is
**  different in Mad Crasher and Vanguard II.
**
**  init_sound defines the location of the polled sound CPU busy bit,
**  which also varies across games.
**
***************************************************************************/

	cpuintrf_push_context(0);
	{
		int madcrash_vreg = (snk_gamegroup == 1) ? 0x00 : 0xf1; // Mad Crasher=0x00, VanguardII=0xf1

		UINT8 sprite_partition = program_read_byte(0xfa00);

		int attributes = program_read_byte(0x8600+madcrash_vreg); /* 0x20: normal, 0xa0: video flipped */
		int bg_scrolly = program_read_byte(0xf800+madcrash_vreg);
		int bg_scrollx = program_read_byte(0xf900+madcrash_vreg);
		int scroll_attributes = program_read_byte(0xfb00+madcrash_vreg);
		int sprite_scrolly = program_read_byte(0xfc00+madcrash_vreg);
		int sprite_scrollx = program_read_byte(0xfd00+madcrash_vreg);
		int fg_scrolly = program_read_byte(0xfe00+madcrash_vreg);
		int fg_scrollx = program_read_byte(0xff00+madcrash_vreg);

		rectangle finalclip = tilemap_clip;
		sect_rect(&finalclip, cliprect);

		if( (scroll_attributes & 4)==0 ) bg_scrollx += 256;
		if( scroll_attributes & 1 ) sprite_scrollx += 256;
		if( scroll_attributes & 2 ) fg_scrollx += 256;

		marvins_palette_bank_w(0, program_read_byte(0xc800+madcrash_vreg));
		update_palette(1);

		if( flipscreen != (attributes&0x80) )
		{
			flipscreen = attributes&0x80;
			tilemap_set_flip( ALL_TILEMAPS, flipscreen?TILEMAP_FLIPY|TILEMAP_FLIPX:0);
		}

		tilemap_set_scrollx( bg_tilemap, 0, bg_scrollx );
		tilemap_set_scrolly( bg_tilemap, 0, bg_scrolly );
		tilemap_set_scrollx( fg_tilemap, 0, fg_scrollx );
		tilemap_set_scrolly( fg_tilemap, 0, fg_scrolly );
		tilemap_set_scrollx( tx_tilemap,  0, 0 );
		tilemap_set_scrolly( tx_tilemap,  0, 0 );

		tilemap_draw( bitmap,&finalclip,bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY ,0);
		draw_sprites( bitmap,cliprect, sprite_scrollx+29, sprite_scrolly+17, 0, sprite_partition );
		tilemap_draw( bitmap,&finalclip,fg_tilemap,0 ,0);
		draw_sprites( bitmap,cliprect, sprite_scrollx+29, sprite_scrolly+17, 1, sprite_partition );
		tilemap_draw( bitmap,&finalclip,tx_tilemap,0 ,0);
		draw_status( bitmap,cliprect );
	}
	cpuintrf_pop_context();
}
