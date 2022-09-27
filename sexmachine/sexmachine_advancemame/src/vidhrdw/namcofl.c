/* vidhrdw/namcofl.c */

#include "driver.h"
#include "namconb1.h"
#include "namcoic.h"
#include "namcos2.h"

UINT32 *namcofl_spritebank32;
//UINT32 *namcofl_tilebank32;
UINT32 *namcofl_mcuram;

static UINT32 namcofl_sprbank;

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE UINT16
nth_word32( const UINT32 *source, int which )
{
	source += which/2;
	which ^= 1;	/* i960 is little-endian */
	if( which&1 )
	{
		return (*source)&0xffff;
	}
	else
	{
		return (*source)>>16;
	}
}

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
INLINE UINT8
nth_byte32( const UINT32 *pSource, int which )
{
		UINT32 data = pSource[which/4];

		which ^= 3;	/* i960 is little-endian */
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

static void
namcofl_install_palette( void )
{
	int pen, page, dword_offset, byte_offset;
	UINT32 r,g,b;
	UINT32 *pSource;

	/* this is unnecessarily expensive.  Better would be to mark palette entries dirty as
     * they are modified, and only process those that have changed.
     */
	pen = 0;
	for( page=0; page<4; page++ )
	{
		pSource = &paletteram32[page*0x2000/4];
		for( dword_offset=0; dword_offset<0x800/4; dword_offset++ )
		{
			r = pSource[dword_offset+0x0000/4];
			g = pSource[dword_offset+0x0800/4];
			b = pSource[dword_offset+0x1000/4];

			for( byte_offset=0; byte_offset<4; byte_offset++ )
			{
				palette_set_color( pen++, r&0xff, g&0xff, b&0xff);
				r>>=8; g>>=8; b>>=8;
			}
		}
	}
}

/**
 * MCU simulation.  It manages coinage and input ports.
 */

static UINT8 credits1, credits2;
	static int old_coin_state;

static void
handle_mcu( void )
	{
	UINT8 *IORAM = (UINT8 *)namcofl_mcuram;
	static int toggle;
	int new_coin_state = readinputport(3)&0x30;
	unsigned p1 = readinputport(0);
	unsigned p2 = readinputport(2);
	unsigned p3 = readinputport(1);
	unsigned p4 = readinputport(3);

	IORAM[BYTE4_XOR_LE(0x6000)] = p1;
	IORAM[BYTE4_XOR_LE(0x6003)] = p2;
	IORAM[BYTE4_XOR_LE(0x6005)] = p3;
	IORAM[BYTE4_XOR_LE(0x60b8)] = p4;

	IORAM[BYTE4_XOR_LE(0x6014)] = readinputport(6)-1;	// handle
	IORAM[BYTE4_XOR_LE(0x6016)] = readinputport(5);	// brake
	IORAM[BYTE4_XOR_LE(0x6018)] = readinputport(4);	// accelerator

	if (!(new_coin_state & 0x20) && (old_coin_state & 0x20))
	{
		credits1++;
	}

	if (!(new_coin_state & 0x10) && (old_coin_state & 0x10))
	{
		credits2++;
	}

	old_coin_state = new_coin_state;
	IORAM[BYTE4_XOR_LE(0x601e)] = credits1;
	IORAM[BYTE4_XOR_LE(0x6020)] = credits2;

	IORAM[BYTE4_XOR_LE(0x6009)] = 0;

	toggle ^= 1;
	if (toggle)
	{	// final lap
		IORAM[BYTE4_XOR_LE(0x6000)]|=0x80;
	}
	else
	{	// speed racer
		IORAM[BYTE4_XOR_LE(0x6000)]&=0x7f;
	}
} /* handle_mcu */

INLINE void
TilemapCB(UINT16 code, int *tile, int *mask )
{
	*tile = code;
	*mask = code;
}


VIDEO_UPDATE( namcofl )
{
	int pri;

	handle_mcu();
	namcofl_install_palette();

	fillbitmap( bitmap, get_black_pen(), cliprect );

	for( pri=0; pri<16; pri++ )
	{
		namco_roz_draw( bitmap,cliprect,pri );
		if((pri&1)==0)
			namco_tilemap_draw( bitmap, cliprect, pri>>1 );
		namco_obj_draw( bitmap, cliprect, pri );
	}

} /* namcofl_vh_screenrefresh */

// NOTE : The two low bits toggle banks (code + 0x4000) for two
//        groups of sprites.  I am unsure how to differentiate those groups
//        at this time however.

WRITE32_HANDLER(namcofl_spritebank_w)
{
	COMBINE_DATA(&namcofl_sprbank);
}

static int FLobjcode2tile( int code )
{
	if ((code & 0x2000) && (namcofl_sprbank & 2)) { code += 0x4000; }

	return code;
}

VIDEO_START( namcofl )
{
	credits1 = credits2 = 0;
	old_coin_state = readinputport(3)&0x30;

	if( namco_tilemap_init( NAMCONB1_TILEGFX, memory_region(NAMCONB1_TILEMASKREGION), TilemapCB ) == 0 )
	{
		namco_obj_init(NAMCONB1_SPRITEGFX,0x0,FLobjcode2tile);
		if( namco_roz_init(NAMCONB1_ROTGFX,NAMCONB1_ROTMASKREGION)==0 )
		{
			return 0;
		}
	}
	return -1;
} /* namcofl_vh_start */
