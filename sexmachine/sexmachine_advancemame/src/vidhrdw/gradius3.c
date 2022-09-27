#include "driver.h"
#include "vidhrdw/konamiic.h"


#define TOTAL_CHARS 0x1000
#define TOTAL_SPRITES 0x4000

UINT16 *gradius3_gfxram;
int gradius3_priority;
static int layer_colorbase[3],sprite_colorbase;
static int dirtygfx;
static unsigned char *dirtychar;



/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void gradius3_tile_callback(int layer,int bank,int *code,int *color)
{
	/* (color & 0x02) is flip y handled internally by the 052109 */
	*code |= ((*color & 0x01) << 8) | ((*color & 0x1c) << 7);
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}



/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void gradius3_sprite_callback(int *code,int *color,int *priority_mask,int *shadow)
{
	#define L0 0xaa
	#define L1 0xcc
	#define L2 0xf0
	static int primask[2][4] =
	{
		{ L0|L2, L0, L0|L2, L0|L1|L2 },
		{ L1|L2, L2, 0,     L0|L1|L2 }
	};
	int pri = ((*color & 0x60) >> 5);
	if (gradius3_priority == 0) *priority_mask = primask[0][pri];
	else *priority_mask = primask[1][pri];

	*code |= (*color & 0x01) << 13;
	*color = sprite_colorbase + ((*color & 0x1e) >> 1);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( gradius3 )
{
	int i;
	static const gfx_layout spritelayout =
	{
		16,16,
		TOTAL_SPRITES,
		4,
		{ 0, 1, 2, 3 },
		{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
			32*8+2*4, 32*8+3*4, 32*8+0*4, 32*8+1*4, 32*8+6*4, 32*8+7*4, 32*8+4*4, 32*8+5*4 },
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
		128*8
	};


	layer_colorbase[0] = 0;
	layer_colorbase[1] = 32;
	layer_colorbase[2] = 48;
	sprite_colorbase = 16;
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,gradius3_tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,REVERSE_PLANE_ORDER,gradius3_sprite_callback))
		return 1;

	K052109_set_layer_offsets(2, -2, 0);

	/* re-decode the sprites because the ROMs are connected to the custom IC differently
       from how they are connected to the CPU. */
	for (i = 0;i < TOTAL_SPRITES;i++)
		decodechar(Machine->gfx[1],i,memory_region(REGION_GFX2),&spritelayout);

	dirtychar = auto_malloc(TOTAL_CHARS);
	memset(dirtychar,1,TOTAL_CHARS);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ16_HANDLER( gradius3_gfxrom_r )
{
	UINT8 *gfxdata = memory_region(REGION_GFX2);

	return (gfxdata[2*offset+1] << 8) | gfxdata[2*offset];
}

READ16_HANDLER( gradius3_gfxram_r )
{
	return gradius3_gfxram[offset];
}

WRITE16_HANDLER( gradius3_gfxram_w )
{
	int oldword = gradius3_gfxram[offset];
	COMBINE_DATA(&gradius3_gfxram[offset]);
	if (oldword != gradius3_gfxram[offset])
	{
		dirtygfx = 1;
		dirtychar[offset / 16] = 1;
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( gradius3 )
{
	static const gfx_layout charlayout =
	{
		8,8,
		TOTAL_CHARS,
		4,
		{ 0, 1, 2, 3 },
#ifdef LSB_FIRST
		{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
#else
		{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
#endif
		{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
		32*8
	};

	/* TODO: this kludge enforces the char banks. For some reason, they don't work otherwise. */
	K052109_w(0x1d80,0x10);
	K052109_w(0x1f00,0x32);

	if (dirtygfx)
	{
		int i;

		dirtygfx = 0;

		for (i = 0;i < TOTAL_CHARS;i++)
		{
			if (dirtychar[i])
			{
				dirtychar[i] = 0;
				decodechar(Machine->gfx[0],i,(UINT8 *)gradius3_gfxram,&charlayout);
			}
		}

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	K052109_tilemap_update();

	fillbitmap(priority_bitmap,0,cliprect);
	if (gradius3_priority == 0)
	{
		tilemap_draw(bitmap,cliprect,K052109_tilemap[1],TILEMAP_IGNORE_TRANSPARENCY,2);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[2],0,4);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[0],0,1);
	}
	else
	{
		tilemap_draw(bitmap,cliprect,K052109_tilemap[0],TILEMAP_IGNORE_TRANSPARENCY,1);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[1],0,2);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[2],0,4);
	}

	K051960_sprites_draw(bitmap,cliprect,-1,-1);
}
