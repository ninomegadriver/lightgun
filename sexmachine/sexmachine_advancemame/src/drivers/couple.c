/*******************************************************************************************

The Couples (c) 1986/1988 The Couples (?)

Preliminary driver by Angelo Salese

This seems the second revision of the Merit.c HW,with the rom banking,the backup ram &
some sort of protection/funky ram banking which involves a program uploaded into
the backup ram itself.

TODO:
-Fix graphics bugs;
-Support the C.R.T.C 6845 features;
-Inputs are sometimes buggy;
-Colors;
-Merge with the Merit.c driver;
-Uncertain Manufacturer & Year of production,is it Merit,it is an hack/bootleg running
on that board or Merit got bankrupt and can't use his trademark?
- update 2006018 by f205v: in-game grafics clearly displays Merit simbol, for this reason
I changed manufacturer to "Merit"

********************************************************************************************

Main CPU : Z80B CPU

Sound : AY3-8910 + 2 x Nec D8255AC-2

Other : MC6845P

Battery backup ram (6264)

Provided to you by Thierry (ShinobiZ) & Gerald (COY)

*******************************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "vidhrdw/crtc6845.h"

static tilemap *bg_tilemap;
static UINT8 *vram_lo,*vram_hi;
static UINT8 *backup_ram;

//before 53 after 71 then 6b
/*
Hi VRAM   Lo VRAM
1111 11
5432 1098 7654 3210
x-x- ---- ---- ---- extra tile number.
-x-- ---- ---- ---- rom selector?
---x xxxx ---- ---- color?
---- ---- xxxx xxxx tile number
*/

static void get_tile_info(int tile_index)
{
	UINT16 vram_data = (((vram_hi[tile_index] & 0xff) << 8) | (vram_lo[tile_index] & 0xff));
	UINT16 region = (vram_data & 0x4000) >> 14;
	UINT16 code = ((vram_data & 0xff) | ((vram_data & 0x8000) >> 7) | ((vram_data & 0x2000) >> 4));
	UINT16 color = (vram_data & 0x0f00) >> 8;

	SET_TILE_INFO(region, code, color, 0)
}

VIDEO_START( couple )
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);

	if(!bg_tilemap)
		return 1;

	return 0;
}

VIDEO_UPDATE( couple )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
}

static WRITE8_HANDLER( couple_vram_lo_w )
{
	vram_lo[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE8_HANDLER( couple_vram_hi_w )
{
	vram_hi[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static READ8_HANDLER( dummy_inputs_r )
{
	logerror("Read %02x @ %06x\n",offset,activecpu_get_pc());
	return 0xff;
}

/*static UINT8 unk_data;

static READ8_HANDLER( unk_r )
{
    return unk_data;
}

static WRITE8_HANDLER( unk_w )
{
    unk_data = data;
    ui_popup("%04x",data);
}*/

/*Actually clear RAM stuff,writes 0x90 at start-up.*/
static WRITE8_HANDLER( merit_prot_w )
{
	/*This is likely to be wrong here but I don't know where to put it,It ensures
      that the parent set don't crash when jumps to $b011*/
	backup_ram[0x1011] = 0xc9; //ret
	//unk_data = data;
	//if(data == 0xc9)
	//ui_popup("%02x",data);
}

#if 0
static struct AY8910interface ay8910_interface =
{
	input_port_4_r,
};
#endif

static ppi8255_interface ppi8255_intf =
{
	2,
	{ input_port_0_r,   input_port_1_r },	/* Port A read */
	{ input_port_2_r,   input_port_3_r },	/* Port B read */
	{ NULL,   			input_port_4_r },	/* Port C read */
	{ NULL,   			NULL           },	/* Port A write */
	{ NULL,   			NULL           },	/* Port B write */
	{ NULL,   			NULL           },	/* Port C write */
};

static ADDRESS_MAP_START( merit_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0x9fff ) AM_ROMBANK(1)
	AM_RANGE( 0xa000, 0xbfff ) AM_RAM AM_BASE(&backup_ram)
	AM_RANGE( 0xc004, 0xc007 ) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE( 0xc008, 0xc00a ) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE( 0xc00b, 0xc00b ) AM_WRITE(merit_prot_w)
//  AM_RANGE( 0xc000, 0xc00f ) AM_READ(dummy_inputs_r)
//  AM_RANGE( 0xc008, 0xc008 ) AM_READ(input_port_0_r)
//  AM_RANGE( 0xc00a, 0xc00a ) AM_READ(input_port_1_r)
  	AM_RANGE( 0xe000, 0xe000 ) AM_WRITE(crtc6845_address_w)
  	AM_RANGE( 0xe001, 0xe001 ) AM_WRITE(crtc6845_register_w)
	AM_RANGE( 0xe800, 0xefff ) AM_READWRITE(MRA8_RAM, couple_vram_hi_w) AM_BASE(&vram_hi)
	AM_RANGE( 0xf000, 0xf7ff ) AM_READWRITE(MRA8_RAM, couple_vram_lo_w) AM_BASE(&vram_lo)
	AM_RANGE( 0xf800, 0xfbff ) AM_RAM /*extra VRAM?*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( merit_io, ADDRESS_SPACE_IO, 8 )
//  ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xc00c, 0xc00c) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc10c, 0xc10c) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END

static MACHINE_RESET( couple )
{
   ppi8255_init(&ppi8255_intf);
}

static PALETTE_INIT( couple )
{
	int i;

	/*TODO:color table not yet understood.Need a snapshot of the real thing.*/
	/*RRGGGBBB*/
	for (i = 0;i < 0xff;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		bit0 = 1;
		bit1 = 1;
		bit2 = (color_prom[i] >> 6) & 0x01;
		bit3 = (color_prom[i] >> 7) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		bit0 = 1;
		bit1 = (color_prom[i] >> 3) & 0x01;
		bit2 = (color_prom[i] >> 4) & 0x01;
		bit3 = (color_prom[i] >> 5) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		bit0 = 1;
		bit1 = (color_prom[i] >> 0) & 0x01;
		bit2 = (color_prom[i] >> 1) & 0x01;
		bit3 = (color_prom[i] >> 2) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;

		palette_set_color(i,r,g,b);
		//color_prom++;
	}
}

#define PORT_INPUTS \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_IMPULSE(1) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_IMPULSE(1) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_IMPULSE(1) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_IMPULSE(1) \

#define PORT_DSW \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x00, "Number of Attempts" ) \
	PORT_DIPSETTING(    0x01, "99" ) \
	PORT_DIPSETTING(    0x00, "9" ) \
	PORT_DIPNAME( 0x02, 0x02, "Tries Per Coin" ) \
	PORT_DIPSETTING(    0x02, "3" ) \
	PORT_DIPSETTING(    0x00, "2" ) \
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) ) \
/*2 Coins for 2 Credits?I think this is an invalid setting,it doesn't even work correctly*/ \
/*  PORT_DIPSETTING(    0x00, DEF_STR( 2C_2C ) ) */ \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) ) \
	PORT_DIPNAME( 0x20, 0x00, "Sound" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \

INPUT_PORTS_START( couple )
	PORT_INPUTS

	PORT_DSW
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Clear RAM" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "4" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*Different DSWs*/
INPUT_PORTS_START( couplep )
	PORT_INPUTS

	PORT_DSW
	PORT_DIPNAME( 0x40, 0x40, "Bonus Play" )
	PORT_DIPSETTING(    0x40, "at 150.000" )
	PORT_DIPSETTING(    0x00, "at 200.000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "4" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout tiles8x8x3_layout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x3_layout, 0, 32 },
	{ REGION_GFX2, 0, &tiles8x8x4_layout, 0, 16 },
	{ REGION_GFX1, 8, &tiles8x8x3_layout, 0, 32 }, //flipped tiles
	{ REGION_GFX2, 8, &tiles8x8x4_layout, 0, 16 }, //flipped tiles
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( couple )
	MDRV_CPU_ADD(Z80,18432000/6)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(merit_mem,0)
	MDRV_CPU_IO_MAP(merit_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(couple)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_PALETTE_INIT(couple)

	MDRV_VIDEO_START(couple)
	MDRV_VIDEO_UPDATE(couple)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 4000000)
//  MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( couple )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1.1d",  0x00000, 0x8000, CRC(bc70337a) SHA1(ffc484bc3965f0780d3fa5d8801af27a7164a417) )
	ROM_LOAD( "2.1e",  0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	/*Only 0x00-0xff data seems color prom*/
	ROM_LOAD( "7.7a",  0x00000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) )
ROM_END

/*f205v's dump,same except for the first z80 rom,first noticeable differences are that
it doesn't jump to the backup ram area and it gives an extra play if you reach a certain
amount of points (there is a dip switch to select the trigger: 150.000 or 200.000*/
ROM_START( couplep )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1_1d",  0x00000, 0x8000, CRC(4601ace6) SHA1(a824ceebf8b9ce77ef2c8e92636e4261f2ae0420) )  /* original label: 1.1d */
	ROM_LOAD( "2.1e",  0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	/*Only 0x00-0xff data seems color prom*/
	ROM_LOAD( "7.7a",  0x00000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) )
ROM_END

/*f205v's dump,this one looks like an intermediate release between set1 and set2;
it has same dips as set1, but remaining machine code is the same as set2*/
ROM_START( couplei )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1-1d",  0x00000, 0x8000, CRC(760fa29e) SHA1(a37a1562028d9615adff3d2ef88e0156354c720a) )  /* original label: 1.1d */
	ROM_LOAD( "2.1e",  0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	/*Only 0x00-0xff data seems color prom*/
	ROM_LOAD( "7.7a",  0x00000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) )
ROM_END

static DRIVER_INIT( couple )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	#if 0 //quick rom compare test
	{
		static int i,r;
		r = 0;
		for(i=0;i<0x2000;i++)
		{
			if(ROM[0x14000+i] == ROM[0x16000+i])
				r++;
		}
		printf("%02x (in HEX) identical bytes (no offset done)\n",r);
	}
	#endif

	/*The banked rom isn't a *real* banking,it's just a strange rom hook-up,the 2nd
      and the 3rd halves are 100% identical(!),unless it's an error of TWO different
      dumpers it's just the way it is,a.k.a. it's an "hardware" banking.
    update 20060118 by f205v: now we have 3 dumps from 3 different boards and they
    all behave the same...*/
	memory_set_bankptr(1,ROM + 0x10000 + (0x2000 * 2));
}

/*Year is uncertain:
title screen says "1988"
PCB marking (set 2) and (set 3) says "230188"
service mode says "6221-51 U5-0 12/02/86" */

GAME( 1988, couple,  0,      couple, couple,  couple, ROT0, "Merit", "The Couples (Set 1)", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_NOT_WORKING)
GAME( 1988, couplep, couple, couple, couplep, couple, ROT0, "Merit", "The Couples (Set 2)", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_NOT_WORKING)
GAME( 1988, couplei, couple, couple, couple,  couple, ROT0, "Merit", "The Couples (Set 3)", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_NOT_WORKING)
