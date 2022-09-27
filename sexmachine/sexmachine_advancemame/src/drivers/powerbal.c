/*

Power Balls  (c) 1994 Playmark
Magic Sticks (c) 1995 Playmark

driver by David Haywood & Pierpaolo Prazzoli

TODO:

Magic Sticks:
 - ticket dispenser
 - other inputs ?

*/

#include "driver.h"
#include "machine/eeprom.h"
#include "sound/okim6295.h"

WRITE16_HANDLER( bigtwin_paletteram_w );

static tilemap *bg_tilemap;
static UINT16 *magicstk_videoram;
static int magicstk_tilebank;

static int bg_yoffset;
static int xoffset;
static int yoffset;

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	0,				/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx",	/* unlock command */
	0,				/* enable_multi_read */
	5				/* reset_delay (otherwise wbeachvl will hang when saving settings) */
};

static NVRAM_HANDLER( magicstk )
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
		{
			UINT8 init[128];
			memset(init,0,128);
			EEPROM_set_data(init,128);
		}
	}
}

static READ16_HANDLER( magicstk_port2_r )
{
	return (input_port_2_r(0) & 0xfe) | EEPROM_read_bit();
}

static WRITE16_HANDLER( magicstk_coin_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,data & 0x20);

		EEPROM_set_cs_line((data & 8) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_write_bit(data & 2);
		EEPROM_set_clock_line((data & 4) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static WRITE16_HANDLER( magicstk_bgvideoram_w )
{
	COMBINE_DATA(&magicstk_videoram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE16_HANDLER( tile_banking_w )
{
	if(((data >> 12) & 0x0f) != magicstk_tilebank)
	{
		magicstk_tilebank = (data >> 12) & 0x0f;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

static WRITE16_HANDLER( oki_banking )
{
	if(data & 3)
	{
		int addr = 0x40000 * ((data & 3) - 1);

		if(addr < memory_region_length(REGION_SOUND1))
			OKIM6295_set_bank_base(0, addr);
	}
}

static ADDRESS_MAP_START( magicstk_main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x088000, 0x0883ff) AM_RAM AM_WRITE(bigtwin_paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x094000, 0x094001) AM_WRITENOP
	AM_RANGE(0x094002, 0x094003) AM_WRITENOP
	AM_RANGE(0x094004, 0x094005) AM_WRITE(tile_banking_w)
	AM_RANGE(0x098180, 0x09917f) AM_READWRITE(MRA16_RAM, magicstk_bgvideoram_w) AM_BASE(&magicstk_videoram)
	AM_RANGE(0x0c2010, 0x0c2011) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c2012, 0x0c2013) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c2014, 0x0c2015) AM_READWRITE(magicstk_port2_r, magicstk_coin_eeprom_w)
	AM_RANGE(0x0c2016, 0x0c2017) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0c2018, 0x0c2019) AM_READ(input_port_4_word_r)
	AM_RANGE(0x0c201c, 0x0c201d) AM_WRITE(oki_banking)
	AM_RANGE(0x0c201e, 0x0c201f) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x0c4000, 0x0c4001) AM_WRITENOP
	AM_RANGE(0x0f0000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x100fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( powerbal_main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x088000, 0x0883ff) AM_RAM AM_WRITE(bigtwin_paletteram_w) AM_BASE(&paletteram16)
	AM_RANGE(0x094000, 0x094001) AM_WRITENOP
	AM_RANGE(0x094002, 0x094003) AM_WRITENOP
	AM_RANGE(0x094004, 0x094005) AM_WRITE(tile_banking_w)
	AM_RANGE(0x098000, 0x098fff) AM_READWRITE(MRA16_RAM, magicstk_bgvideoram_w) AM_BASE(&magicstk_videoram)
	AM_RANGE(0x099000, 0x09bfff) AM_RAM // not used
	AM_RANGE(0x0c2010, 0x0c2011) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c2012, 0x0c2013) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c2014, 0x0c2015) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0c2016, 0x0c2017) AM_READ(input_port_3_word_r)
	AM_RANGE(0x0c2018, 0x0c2019) AM_READ(input_port_4_word_r)
	AM_RANGE(0x0c201c, 0x0c201d) AM_WRITE(oki_banking)
	AM_RANGE(0x0c201e, 0x0c201f) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x0c4000, 0x0c4001) AM_WRITENOP
	AM_RANGE(0x0f0000, 0x0fffff) AM_RAM
	AM_RANGE(0x101000, 0x101fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x102000, 0x10200d) AM_WRITENOP // not used scroll regs?
	AM_RANGE(0x103000, 0x103fff) AM_RAM
ADDRESS_MAP_END


INPUT_PORTS_START( powerbal )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x08, DEF_STR( English ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Italian ) )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
INPUT_PORTS_END

INPUT_PORTS_START( magicstk )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x1e, 0x1e, "Coinage Mode 1" ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x02, DEF_STR( 8C_3C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_3C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPNAME( 0x06, 0x06, "Coin A Mode 2" ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPNAME( 0x18, 0x18, "Coin B Mode 2" ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) ) PORT_CONDITION("DSW1",0x01,PORTCOND_NOTEQUALS,0x01)
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Clear Counters" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Ticket" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Hopper" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0xe0, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xa0, "Hard 7" )
	PORT_DIPSETTING(    0x20, "Very Hard 6" )
	PORT_DIPSETTING(    0xc0, "Very Hard 5" )
//  PORT_DIPSETTING(    0x80, "Very Hard 4" )
//  PORT_DIPSETTING(    0x40, "Very Hard 4" )
	PORT_DIPSETTING(    0x00, "Very Hard 4" )
	PORT_DIPSETTING(    0x60, "Normal 8" )
	PORT_DIPSETTING(    0xe0, "Easy 9" )
INPUT_PORTS_END

static void powerbal_get_bg_tile_info(int tile_index)
{
	int code = (magicstk_videoram[tile_index] & 0x07ff) + magicstk_tilebank * 0x800;
	int colr = magicstk_videoram[tile_index] & 0xf000;

	if (magicstk_videoram[tile_index] & 0x800) code |= 0x8000;

	SET_TILE_INFO(1,code,colr >> 12,0)
}

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;
	int height = Machine->gfx[0]->height;

	for (offs = 4;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,code,color,flipx;

		sy = spriteram16[offs+3-4];	/* typical Playmark style... */
		if (sy & 0x8000) return;	/* end of list marker */

		flipx = sy & 0x4000;
		sx = (spriteram16[offs+1] & 0x01ff) - 16-7;
		sy = (256-8-height - sy) & 0xff;
		code = spriteram16[offs+2];
		color = (spriteram16[offs+1] & 0xf000) >> 12;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,0,
				sx + xoffset,sy + yoffset,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_START( powerbal )
{
	bg_tilemap = tilemap_create(powerbal_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,64,32);

	if (!bg_tilemap)
		return 1;

	xoffset = -20;

	tilemap_set_scrolly(bg_tilemap, 0, bg_yoffset);

	return 0;
}

VIDEO_UPDATE( powerbal )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}

static const gfx_layout magicstk_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};



static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,          0x100, 16 },	/* colors 0x100-0x1ff */
	{ REGION_GFX1, 0, &magicstk_charlayout, 0x000, 16 },	/* colors 0x000-0x0ff */
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( powerbal )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(powerbal_main_map, 0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(61)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(powerbal)
	MDRV_VIDEO_UPDATE(powerbal)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magicstk )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(magicstk_main_map, 0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(61)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(magicstk)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(powerbal)
	MDRV_VIDEO_UPDATE(powerbal)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/*
Power Balls
Playmark, 1994

PCB Layout
----------

1-6-93
LC BMJ
|------------------------------------------------------------|
|     1                        6116                          |
|      M6295  1MHz                                     4     |
| VOL                          6116                          |
|                                                      5     |
|                                                            |
|                                      |--------|      6     |
|      2018                            |TPC1020 |            |
|                                      |        |      7     |
|J     2018                            |        |            |
|A                                     |--------|      8     |
|M                                                           |
|M         GAL16V8                                     9     |
|A                                                           |
|                                                      10    |
|                                                            |
|                                                      11    |
|  DIPSW                          GAL22V10                   |
|                                                            |
|                                 2018                       |
|  DIPSW                                                     |
|                                 2018              2018     |
|        62256     62256                                     |
|12MHz     2         3          GAL16V8             2018     |
|                                                            |
|            68000          26MHz                            |
|------------------------------------------------------------|
Notes:
      HSync - 15.45kHz
      VSync - 61Hz
      68000 clock - 12MHz
      6295 clock - 1.000MHz, sample rate = 1000000 / 132
      2018 - 2k x8 SRAM
      6116 - 2k x8 SRAM
      62256 - 32k x8 SRAM

      ROMs - 2, 3 - 27C020
             1, 4-11 - 27C040
*/

ROM_START( powerbal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "3.u67",  0x00000, 0x40000, CRC(3aecdde4) SHA1(e78373246d55f120e8d94f4606da874df439b823) )
	ROM_LOAD16_BYTE( "2.u66",  0x00001, 0x40000, CRC(a4552a19) SHA1(88b84daa1fd36d5c683cf0d6dce341aedbc360d1) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.u38",        0x000000, 0x80000, CRC(a60aa981) SHA1(46a5d2d2a353a45127a03a104e877ffd150daa92) )
	ROM_LOAD( "5.u42",        0x080000, 0x80000, CRC(966c71df) SHA1(daf4bcf3d2ef10ea9a5e2e7ea71b3783b9f5b1f0) )
	ROM_LOAD( "6.u39",        0x100000, 0x80000, CRC(668957b9) SHA1(31fc9328ff6044e17834b6d61a886a8ef2e6570c) )
	ROM_LOAD( "7.u45",        0x180000, 0x80000, CRC(f5721c66) SHA1(1e8b3a8e82da60378dad7727af21157c4059b071) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8.u86",        0x000000, 0x80000, CRC(4130694c) SHA1(581d0035ce1624568f635bd79290be6c587a2533) )
	ROM_LOAD( "9.u85",        0x080000, 0x80000, CRC(e7bcd2e7) SHA1(01a5e5ac5da2fd79a0c9088f775096b9915bae92) )
	ROM_LOAD( "10.u84",       0x100000, 0x80000, CRC(90412135) SHA1(499619c72613a1dd63a6504e39b159a18a71f4fa) )
	ROM_LOAD( "11.u83",       0x180000, 0x80000, CRC(92d7d40a) SHA1(81879945790feb9aeb45750e9b5ded3356571503) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0xc0000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "1.u16",        0x00000, 0x40000, CRC(12776dbc) SHA1(9ab9930fd581296642834d2cb4ba65264a588af3) )
	ROM_CONTINUE(             0x60000, 0x20000 )
	ROM_CONTINUE(             0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1,  0x00000, 0x80000, 0x20000)
ROM_END

ROM_START( magicstk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "12.u67", 0x00000, 0x20000, CRC(70a9c66f) SHA1(0cf4b2d0f796e35881d68adc69eca4360d6ad693) )
	ROM_LOAD16_BYTE( "11.u66", 0x00001, 0x20000, CRC(a9d7c90e) SHA1(e12517776dc14747b4bbe49f93c4d7e83e8eae01) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "13.u36",       0x00000, 0x20000, CRC(31e52562) SHA1(18ee5ba990d97690ece81e4066a9f0395ddc6f3e) )
	ROM_LOAD( "14.u42",       0x20000, 0x20000, CRC(b0d35eda) SHA1(a85d45d3b4fbacecf5aa2af9a18ba0ac9f1f9a26) )
	ROM_LOAD( "15.u39",       0x40000, 0x20000, CRC(af27004b) SHA1(b022020e6bd6fc9ec95f23b6a37911df0768856e) )
	ROM_LOAD( "16.u45",       0x60000, 0x20000, CRC(0c980db3) SHA1(212129bf86cdc73752be184e579299e03ba6862e) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "17.u86",       0x00000, 0x20000, CRC(ce238006) SHA1(3425a8125d56139fe5d220b0d9d5c9a4af1f4d58) )
	ROM_LOAD( "18.u85",       0x20000, 0x20000, CRC(3dc88bf6) SHA1(f9c04bca32bae4aa6df38635d73c6a4b8742fbd3) )
	ROM_LOAD( "19.u84",       0x40000, 0x20000, CRC(ee12d5b2) SHA1(872edff5a35d2725e3dd752a5f609aca995bfeff) )
	ROM_LOAD( "20.u83",       0x60000, 0x20000, CRC(a07f542b) SHA1(0c17629142a90687460b4c951f2062f5c7de8921) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "10.u16",       0x00000, 0x20000, CRC(1e4a03ef) SHA1(6a134daa9a6d8dbda51cab348627f078c3dde8c7) )
ROM_END

DRIVER_INIT( powerbal )
{
	bg_yoffset = 16;
	yoffset = -8;
}

DRIVER_INIT( magicstk )
{
	bg_yoffset = 0;
	yoffset = -5;
}

GAME( 1994, powerbal, 0, powerbal, powerbal, powerbal, ROT0, "Playmark", "Power Balls", 0 )
GAME( 1995, magicstk, 0, magicstk, magicstk, magicstk, ROT0, "Playmark", "Magic Sticks", 0 )
