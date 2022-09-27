/*******************************************************************************************

Alien Command (c) 1993 Jaleco

driver by Angelo Salese

seems like a particular version of the NMK16 board.

TODO:
-Understand what "devices" area needs to make this working...

-Complete sound

-Back tilemap paging is likely to be incorrect...


m68k irq table vectors
lev 1 : 0x64 : 0000 04f0 - rte
lev 2 : 0x68 : 0000 044a - vblank
lev 3 : 0x6c : 0000 0484 - "dynamic color change" (?)
lev 4 : 0x70 : 0000 04f0 - rte
lev 5 : 0x74 : 0000 04f0 - rte
lev 6 : 0x78 : 0000 04f0 - rte
lev 7 : 0x7c : 0000 04f0 - rte

===========================================================================================

Jaleco Alien Command
Redemption Video Game with Guns

2/7/99

Hardware Specs: 68000 at 12Mhz and OKI6295

JALMR17  BIN       524,288  02-07-99  1:17a JALMR17.BIN
JALCF2   BIN     1,048,576  02-07-99  1:10a JALCF2.BIN
JALCF3   BIN       131,072  02-07-99  1:12a JALCF3.BIN
JALCF4   BIN       131,072  02-07-99  1:13a JALCF4.BIN
JALCF5   BIN       524,288  02-07-99  1:15a JALCF5.BIN
JALCF6   BIN       131,072  02-07-99  1:14a JALCF6.BIN
JALGP1   BIN       524,288  02-07-99  1:21a JALGP1.BIN
JALGP2   BIN       524,288  02-07-99  1:24a JALGP2.BIN
JALGP3   BIN       524,288  02-07-99  1:20a JALGP3.BIN
JALGP4   BIN       524,288  02-07-99  1:23a JALGP4.BIN
JALGP5   BIN       524,288  02-07-99  1:19a JALGP5.BIN
JALGP6   BIN       524,288  02-07-99  1:23a JALGP6.BIN
JALGP7   BIN       524,288  02-07-99  1:19a JALGP7.BIN
JALGP8   BIN       524,288  02-07-99  1:22a JALGP8.BIN
JALMR14  BIN       524,288  02-07-99  1:17a JALMR14.BIN
JALCF1   BIN     1,048,576  02-07-99  1:11a JALCF1.BIN


*******************************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"

static tilemap *tx_tilemap,*bg_tilemap;
static UINT16 *ac_txvram,*ac_bgvram;
static UINT16 *ac_vregs;

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static void ac_get_bg_tile_info(int tile_index)
{
	int code = ac_bgvram[tile_index];
	SET_TILE_INFO(
			1,
			code & 0xfff,
			(code & 0xf000) >> 12,
			0)
}

static void ac_get_tx_tile_info(int tile_index)
{
	int code = ac_txvram[tile_index];
	SET_TILE_INFO(
			0,
			code & 0xfff,
			(code & 0xf000) >> 12,
			0)
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int priority, int pri_mask)
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		if (!(spriteram16[offs+0] & 0x1000))
		{
			int sx = (spriteram16[offs+3] & 0x0ff);
			int code = spriteram16[offs+6];
			int color = spriteram16[offs+7];
			int w = (spriteram16[offs+0] & 0x0f);
			int h = ((spriteram16[offs+0] & 0xf0) >> 4);
			/*Unlike NMK16,the given Y offset is where the sprite must end instead
            of where it starts.*/
			int sy = (spriteram16[offs+4] & 0x0ff) - ((h+1)*0x10);
/**/		int pri = spriteram16[offs+5];
/**/		int flipy = ((spriteram16[offs+1] & 0x0200) >> 9);
/**/		int flipx = ((spriteram16[offs+1] & 0x0100) >> 8);

			int xx,yy,x;
			int delta = 16;

			flipx ^= flip_screen;
			flipy ^= flip_screen;

			if ((pri&pri_mask)!=priority) continue;

			if (flip_screen)
			{
				sx = 368 - sx;
				sy = 240 - sy;
				delta = -16;
			}

			yy = h;
			do
			{
				x = sx;
				xx = w;
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							code,
							color,
							flipx, flipy,
							((x + 16) & 0x1ff) - 16,sy & 0x1ff,
							cliprect,TRANSPARENCY_PEN,15);

					code++;
					x += delta;
				} while (--xx >= 0);

				sy += delta;
			} while (--yy >= 0);
		}
	}
}


VIDEO_START( acommand )
{
	tx_tilemap = tilemap_create(ac_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,512,32);
	bg_tilemap = tilemap_create(ac_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,16);

	if(!tx_tilemap || !bg_tilemap)
		return 1;

	ac_vregs = auto_malloc(0x80);

	tilemap_set_transparent_pen(tx_tilemap,15);

	return 0;
}


/*copied from drivers/gticlub.c,to be completed*/
#define LED_ON		0x01c00
#define SHOW_LEDS	0

#if SHOW_LEDS
static void draw_led(mame_bitmap *bitmap, int x, int y,UINT8 value)
{
	plot_box(bitmap, x, y, 5, 9, 0x00000000);

	/* Top */
	if( (value & 0x40) == 0 ) {
		plot_pixel(bitmap, x+1, y+0, LED_ON);
		plot_pixel(bitmap, x+2, y+0, LED_ON);
		plot_pixel(bitmap, x+3, y+0, LED_ON);
	}
	/* Middle */
	if( (value & 0x01) == 0 ) {
		plot_pixel(bitmap, x+1, y+4, LED_ON);
		plot_pixel(bitmap, x+2, y+4, LED_ON);
		plot_pixel(bitmap, x+3, y+4, LED_ON);
	}
	/* Bottom */
	if( (value & 0x08) == 0 ) {
		plot_pixel(bitmap, x+1, y+8, LED_ON);
		plot_pixel(bitmap, x+2, y+8, LED_ON);
		plot_pixel(bitmap, x+3, y+8, LED_ON);
	}
	/* Top Left */
	if( (value & 0x02) == 0 ) {
		plot_pixel(bitmap, x+0, y+1, LED_ON);
		plot_pixel(bitmap, x+0, y+2, LED_ON);
		plot_pixel(bitmap, x+0, y+3, LED_ON);
	}
	/* Top Right */
	if( (value & 0x20) == 0 ) {
		plot_pixel(bitmap, x+4, y+1, LED_ON);
		plot_pixel(bitmap, x+4, y+2, LED_ON);
		plot_pixel(bitmap, x+4, y+3, LED_ON);
	}
	/* Bottom Left */
	if( (value & 0x04) == 0 ) {
		plot_pixel(bitmap, x+0, y+5, LED_ON);
		plot_pixel(bitmap, x+0, y+6, LED_ON);
		plot_pixel(bitmap, x+0, y+7, LED_ON);
	}
	/* Bottom Right */
	if( (value & 0x10) == 0 ) {
		plot_pixel(bitmap, x+4, y+5, LED_ON);
		plot_pixel(bitmap, x+4, y+6, LED_ON);
		plot_pixel(bitmap, x+4, y+7, LED_ON);
	}
}
#endif
static UINT16 led0;

VIDEO_UPDATE( acommand )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect,0,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);

	#if SHOW_LEDS
		draw_led(bitmap, 3, 53, (led0 & 0xff00) >> 8);
		ui_popup("%04x",led0);
	#endif
}


WRITE16_HANDLER( ac_bgvram_w )
{
	int oldword = ac_bgvram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		ac_bgvram[offset] = newword;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE16_HANDLER( ac_txvram_w )
{
	int oldword = ac_txvram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		ac_txvram[offset] = newword;
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}

static WRITE16_HANDLER(ac_bgscroll_w)
{
	switch(offset)
	{
		case 0: tilemap_set_scrollx(bg_tilemap,0,data); break;
		case 1: tilemap_set_scrolly(bg_tilemap,0,data); break;
		case 2: /*BG_TILEMAP priority?*/ break;
	}
}

static WRITE16_HANDLER(ac_txscroll_w)
{
	switch(offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data); break;
		case 2: /*TX_TILEMAP priority?*/ break;
	}
}

/******************************************************************************************/

static UINT16 *ac_devram;


static READ16_HANDLER(ac_devices_r)
{
//  ui_popup("(PC=%06x) read at %04x",activecpu_get_pc(),offset*2);
	switch(offset)
	{
		case 0x0008/2:
			/*
                --x- ---- ---- ---- Ticket Dispenser - 2
                ---x ---- ---- ---- Ticket Dispenser - 1
                ---- -x-- ---- ---- Right Gun HIT
                ---- ---x ---- ---- Left Gun HIT
                ---- ---- --x- ---- Service Mode (Toggle)
                ---- ---- ---x ---- Service Coin
                ---- ---- ---- x--- COIN2
                ---- ---- ---- -x-- COIN1
                ---- ---- ---- --x- (Activate Test)
                ---- ---- ---- ---x (Advance Thru Tests)
            */
			return input_port_0_word_r(0,0);
		case 0x0014/2:
			/*
                write 0x40,read (~0x08)
                write 0x08,read (~0x01)

                ---- ---- ---- ---x Boss Door - limit switch
            */
			return (ac_devram[offset]);
		case 0x0016/2:
			return OKIM6295_status_0_r(0);
		case 0x0018/2:
			/*
                ---- ---- ---- x--- Astronaut - switch
            */
			return ac_devram[offset];
		case 0x001a/2:
			return OKIM6295_status_1_r(0);
		case 0x0040/2:
			/*
                x-x- x-x- x-x- xx-- (ACTIVE HIGH?) [eori #$aaac, D0]
                ---- ---- ---- --xx Boss Door - Motor
            */
			return ac_devram[offset];
		case 0x0044/2:
			/*
                xxxx xxxx x-x- x-x- (ACTIVE HIGH?) [eori #$ffaa, D0]
            */
			return ac_devram[offset];
		case 0x005c/2:
			/*
                xxxx xxxx ---- ---- DIPSW4
                ---- ---- xxxx xxxx DIPSW3
            */
			return input_port_1_word_r(0,0);
	}
	return ac_devram[offset];
}

static WRITE16_HANDLER(ac_devices_w)
{
	COMBINE_DATA(&ac_devram[offset]);
	switch(offset)
	{
		case 0x16/2:
			if(ACCESSING_LSB)
			{
				logerror("Request to play sample %02x with rom 2\n",data);
				OKIM6295_data_0_w(0,data);
			}
			break;
		case 0x1a/2:
			if(ACCESSING_LSB)
			{
				logerror("Request to play sample %02x with rom 1\n",data);
				OKIM6295_data_1_w(0,data);
			}
			break;
		case 0x1c/2:
			/*IRQ mask?*/
			break;
		case 0x40/2:
			break;
		case 0x44/2:
			break;
		case 0x48/2:
			led0 = ac_devram[offset];
			break;
	}
	logerror("%04x %04x\n",offset*2,data);
}

/*This is always zero ATM*/
static WRITE16_HANDLER(ac_unk2_w)
{
	if(data)
		ui_popup("UNK-2 enabled %04x",data);
}

static ADDRESS_MAP_START( acommand, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x082000, 0x082005) AM_WRITE(ac_bgscroll_w)
	AM_RANGE(0x082100, 0x082105) AM_WRITE(ac_txscroll_w)
	AM_RANGE(0x082208, 0x082209) AM_WRITE(ac_unk2_w)
	AM_RANGE(0x0a0000, 0x0a3fff) AM_READWRITE(MRA16_RAM, ac_bgvram_w) AM_BASE(&ac_bgvram)
	AM_RANGE(0x0b0000, 0x0b3fff) AM_READWRITE(MRA16_RAM, ac_txvram_w) AM_BASE(&ac_txvram)
	AM_RANGE(0x0b8000, 0x0bffff) AM_READWRITE(MRA16_RAM, paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x0f0000, 0x0f7fff) AM_RAM
	AM_RANGE(0x0f8000, 0x0f8fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x0f9000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x1000ff) AM_READ(ac_devices_r) AM_WRITE(ac_devices_w) AM_BASE(&ac_devram)
ADDRESS_MAP_END

INPUT_PORTS_START( acommand )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_decode acommand_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x2700, 16 }, /*???*/
	{ REGION_GFX2, 0, &tilelayout, 0x1800, 256 },
	{ REGION_GFX3, 0, &tilelayout, 0x1800, 256 },
	{ -1 } /* end of array */
};

static INTERRUPT_GEN( acommand_irq )
{
	if (cpu_getiloops() == 0)
		cpunum_set_input_line(0, 2, HOLD_LINE);
	else if (cpu_getiloops() == 1)
		cpunum_set_input_line(0, 3, HOLD_LINE);
}

static MACHINE_DRIVER_START( acommand )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)
	MDRV_CPU_PROGRAM_MAP(acommand,0)
	MDRV_CPU_VBLANK_INT(acommand_irq,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(acommand_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)

	MDRV_VIDEO_START(acommand)
	MDRV_VIDEO_UPDATE(acommand)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 2400000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 2400000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( acommand )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "jalcf3.bin",   0x000000, 0x020000, CRC(f031abf7) SHA1(e381742fd6a6df4ddae42ddb3a074a55dc550b3c) )
	ROM_LOAD16_BYTE( "jalcf4.bin",   0x000001, 0x020000, CRC(dd0c0540) SHA1(3e788fcb30ae725bd0ec9b57424e3946db1e946f) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "jalcf6.bin",   0x000000, 0x020000, CRC(442173d6) SHA1(56c02bc2761967040127977ecabe844fc45e2218) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "jalcf5.bin",   0x000000, 0x080000, CRC(ff0be97f) SHA1(5ccab778318dec30849d7b7f25091d4aab8bde32) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* SPR */
	ROM_LOAD16_BYTE( "jalgp1.bin",   0x000000, 0x080000, CRC(c4aeeae2) SHA1(ee0d3dd93a604f8e1a96b55c4a1cd001d49f1157) )
	ROM_LOAD16_BYTE( "jalgp2.bin",   0x000001, 0x080000, CRC(f0e4e80e) SHA1(08252ef8b5e309cce2d4654410142f4ae9e3ef22) )
	ROM_LOAD16_BYTE( "jalgp3.bin",   0x100000, 0x080000, CRC(7acebd83) SHA1(64be95186d62003b637fcdf45a9c0b7aab182116) )
	ROM_LOAD16_BYTE( "jalgp4.bin",   0x100001, 0x080000, CRC(6a6b72f3) SHA1(3ba359b1a89eb3f6664ed83d93f79d7f895d4222) )
	ROM_LOAD16_BYTE( "jalgp5.bin",   0x200000, 0x080000, CRC(65ab751d) SHA1(f2cb8701eb8c3567a1d03248e6918c5a7b5df939) )
	ROM_LOAD16_BYTE( "jalgp6.bin",   0x200001, 0x080000, CRC(24e3ab23) SHA1(d1431688e1518ba52935f6ab44b815975bec4c27) )
	ROM_LOAD16_BYTE( "jalgp7.bin",   0x300000, 0x080000, CRC(44b71098) SHA1(a6ec2573f9a266d4f8f315f6e99b12525011f512) )
	ROM_LOAD16_BYTE( "jalgp8.bin",   0x300001, 0x080000, CRC(ce0b7838) SHA1(46e34971cb62565a3948d8c0a18086648c32e13b) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "jalcf1.bin",   0x000000, 0x100000, CRC(24af21d3) SHA1(f68ab81a6c833b57ae9eef916a1c8578f3d893dd) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "jalcf2.bin",   0x000000, 0x100000, CRC(b982fd97) SHA1(35ee5b1b9be762ccfefda24d73e329ceea876deb) )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* ? these two below are identical*/
	ROM_LOAD( "jalmr14.bin",   0x000000, 0x080000, CRC(9d428fb7) SHA1(02f72938d73db932bd217620a175a05215f6016a) )
	ROM_LOAD( "jalmr17.bin",   0x080000, 0x080000, CRC(9d428fb7) SHA1(02f72938d73db932bd217620a175a05215f6016a) )
ROM_END

GAME( 1990, acommand,  0,       acommand,  acommand,  0, ROT0, "Jaleco", "Alien Command" , GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
