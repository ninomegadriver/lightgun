/**
 * video hardware for Namco System22
 *
 * todo:
 *
 * - fog bugfixes
 *
 * - SPOT
 *
 * - sprite
 *          xy offset
 *          clipping to window
 *          eliminate garbage (air combat)
 *
 *
 *******************************
czram[0] = 1fff 1fdf 1fbf 1f9f 1f7f 1f5f 1f3f 1f1f 1eff 1edf 1ebf 1e9f 1e7f 1e5f 1e3f 1e1f 1dff 1ddf 1dbf 1d9f 1d7f 1d5f 1d3f 1d1f 1cff 1cdf 1cbf 1c9f 1c7f 1c5f 1c3f 1c1f 1bff 1bdf 1bbf 1b9f 1b7f 1b5f 1b3f 1b1f 1aff 1adf 1abf 1a9f 1a7f 1a5f 1a3f 1a1f 19ff 19df 19bf 199f 197f 195f 193f 191f 18ff 18df 18bf 189f 187f 185f 183f 181f 17ff 17df 17bf 179f 177f 175f 173f 171f 16ff 16df 16bf 169f 167f 165f 163f 161f 15ff 15df 15bf 159f 157f 155f 153f 151f 14ff 14df 14bf 149f 147f 145f 143f 141f 13ff 13df 13bf 139f 137f 135f 133f 131f 12ff 12df 12bf 129f 127f 125f 123f 121f 11ff 11df 11bf 119f 117f 115f 113f 111f 10ff 10df 10bf 109f 107f 105f 103f 101f 0fff 0fdf 0fbf 0f9f 0f7f 0f5f 0f3f 0f1f 0eff 0edf 0ebf 0e9f 0e7f 0e5f 0e3f 0e1f 0dff 0ddf 0dbf 0d9f 0d7f 0d5f 0d3f 0d1f 0cff 0cdf 0cbf 0c9f 0c7f 0c5f 0c3f 0c1f 0bff 0bdf 0bbf 0b9f 0b7f 0b5f 0b3f 0b1f 0aff 0adf 0abf 0a9f 0a7f 0a5f 0a3f 0a1f 09ff 09df 09bf 099f 097f 095f 093f 091f 08ff 08df 08bf 089f 087f 085f 083f 081f 07ff 07df 07bf 079f 077f 075f 073f 071f 06ff 06df 06bf 069f 067f 065f 063f 061f 05ff 05df 05bf 059f 057f 055f 053f 051f 04ff 04df 04bf 049f 047f 045f 043f 041f 03ff 03df 03bf 039f 037f 035f 033f 031f 02ff 02df 02bf 029f 027f 025f 023f 021f 01ff 01df 01bf 019f 017f 015f 013f 011f 00ff 00df 00bf 009f 007f 005f 003f 001f
czram[1] = 0000 0000 0000 0001 0002 0003 0005 0007 0009 000b 000e 0011 0014 0017 001b 001f 0023 0027 002c 0031 0036 003b 0041 0047 004d 0053 005a 0061 0068 006f 0077 007f 0087 008f 0098 00a1 00aa 00b3 00bd 00c7 00d1 00db 00e6 00f1 00fc 0107 0113 011f 012b 0137 0144 0151 015e 016b 0179 0187 0195 01a3 01b2 01c1 01d0 01df 01ef 01ff 020f 021f 0230 0241 0252 0263 0275 0287 0299 02ab 02be 02d1 02e4 02f7 030b 031f 0333 0347 035c 0371 0386 039b 03b1 03c7 03dd 03f3 040a 0421 0438 044f 0467 047f 0497 04af 04c8 04e1 04fa 0513 052d 0547 0561 057b 0596 05b1 05cc 05e7 0603 061f 063b 0657 0674 0691 06ae 06cb 06e9 0707 0725 0743 0762 0781 07a0 07bf 07df 07ff 081f 083f 0860 0881 08a2 08c3 08e5 0907 0929 094b 096e 0991 09b4 09d7 09fb 0a1f 0a43 0a67 0a8c 0ab1 0ad6 0afb 0b21 0b47 0b6d 0b93 0bba 0be1 0c08 0c2f 0c57 0c7f 0ca7 0ccf 0cf8 0d21 0d4a 0d73 0d9d 0dc7 0df1 0e1b 0e46 0e71 0e9c 0ec7 0ef3 0f1f 0f4b 0f77 0fa4 0fd1 0ffe 102b 1059 1087 10b5 10e3 1112 1141 1170 119f 11cf 11ff 122f 125f 1290 12c1 12f2 1323 1355 1387 13b9 13eb 141e 1451 1484 14b7 14eb 151f 1553 1587 15bc 15f1 1626 165b 1691 16c7 16fd 1733 176a 17a1 17d8 180f 1847 187f 18b7 18ef 1928 1961 199a 19d3 1a0d 1a47 1a81 1abb 1af6 1b31 1b6c 1ba7 1be3 1c1f 1c5b 1c97 1cd4 1d11 1d4e 1d8b 1dc9 1e07 1e45 1e83 1ec2 1f01 1f40 1f7f 1fbf 1fff
czram[2] = 003f 007f 00be 00fd 013c 017b 01b9 01f7 0235 0273 02b0 02ed 032a 0367 03a3 03df 041b 0457 0492 04cd 0508 0543 057d 05b7 05f1 062b 0664 069d 06d6 070f 0747 077f 07b7 07ef 0826 085d 0894 08cb 0901 0937 096d 09a3 09d8 0a0d 0a42 0a77 0aab 0adf 0b13 0b47 0b7a 0bad 0be0 0c13 0c45 0c77 0ca9 0cdb 0d0c 0d3d 0d6e 0d9f 0dcf 0dff 0e2f 0e5f 0e8e 0ebd 0eec 0f1b 0f49 0f77 0fa5 0fd3 1000 102d 105a 1087 10b3 10df 110b 1137 1162 118d 11b8 11e3 120d 1237 1261 128b 12b4 12dd 1306 132f 1357 137f 13a7 13cf 13f6 141d 1444 146b 1491 14b7 14dd 1503 1528 154d 1572 1597 15bb 15df 1603 1627 164a 166d 1690 16b3 16d5 16f7 1719 173b 175c 177d 179e 17bf 17df 17ff 181f 183f 185e 187d 189c 18bb 18d9 18f7 1915 1933 1950 196d 198a 19a7 19c3 19df 19fb 1a17 1a32 1a4d 1a68 1a83 1a9d 1ab7 1ad1 1aeb 1b04 1b1d 1b36 1b4f 1b67 1b7f 1b97 1baf 1bc6 1bdd 1bf4 1c0b 1c21 1c37 1c4d 1c63 1c78 1c8d 1ca2 1cb7 1ccb 1cdf 1cf3 1d07 1d1a 1d2d 1d40 1d53 1d65 1d77 1d89 1d9b 1dac 1dbd 1dce 1ddf 1def 1dff 1e0f 1e1f 1e2e 1e3d 1e4c 1e5b 1e69 1e77 1e85 1e93 1ea0 1ead 1eba 1ec7 1ed3 1edf 1eeb 1ef7 1f02 1f0d 1f18 1f23 1f2d 1f37 1f41 1f4b 1f54 1f5d 1f66 1f6f 1f77 1f7f 1f87 1f8f 1f96 1f9d 1fa4 1fab 1fb1 1fb7 1fbd 1fc3 1fc8 1fcd 1fd2 1fd7 1fdb 1fdf 1fe3 1fe7 1fea 1fed 1ff0 1ff3 1ff5 1ff7 1ff9 1ffb 1ffc 1ffd 1ffe 1fff 1fff 1fff
czram[3] = 0000 001f 003f 005f 007f 009f 00bf 00df 00ff 011f 013f 015f 017f 019f 01bf 01df 01ff 021f 023f 025f 027f 029f 02bf 02df 02ff 031f 033f 035f 037f 039f 03bf 03df 03ff 041f 043f 045f 047f 049f 04bf 04df 04ff 051f 053f 055f 057f 059f 05bf 05df 05ff 061f 063f 065f 067f 069f 06bf 06df 06ff 071f 073f 075f 077f 079f 07bf 07df 07ff 081f 083f 085f 087f 089f 08bf 08df 08ff 091f 093f 095f 097f 099f 09bf 09df 09ff 0a1f 0a3f 0a5f 0a7f 0a9f 0abf 0adf 0aff 0b1f 0b3f 0b5f 0b7f 0b9f 0bbf 0bdf 0bff 0c1f 0c3f 0c5f 0c7f 0c9f 0cbf 0cdf 0cff 0d1f 0d3f 0d5f 0d7f 0d9f 0dbf 0ddf 0dff 0e1f 0e3f 0e5f 0e7f 0e9f 0ebf 0edf 0eff 0f1f 0f3f 0f5f 0f7f 0f9f 0fbf 0fdf 0fff 101f 103f 105f 107f 109f 10bf 10df 10ff 111f 113f 115f 117f 119f 11bf 11df 11ff 121f 123f 125f 127f 129f 12bf 12df 12ff 131f 133f 135f 137f 139f 13bf 13df 13ff 141f 143f 145f 147f 149f 14bf 14df 14ff 151f 153f 155f 157f 159f 15bf 15df 15ff 161f 163f 165f 167f 169f 16bf 16df 16ff 171f 173f 175f 177f 179f 17bf 17df 17ff 181f 183f 185f 187f 189f 18bf 18df 18ff 191f 193f 195f 197f 199f 19bf 19df 19ff 1a1f 1a3f 1a5f 1a7f 1a9f 1abf 1adf 1aff 1b1f 1b3f 1b5f 1b7f 1b9f 1bbf 1bdf 1bff 1c1f 1c3f 1c5f 1c7f 1c9f 1cbf 1cdf 1cff 1d1f 1d3f 1d5f 1d7f 1d9f 1dbf 1ddf 1dff 1e1f 1e3f 1e5f 1e7f 1e9f 1ebf 1edf 1eff 1f1f 1f3f 1f5f 1f7f 1f9f 1fbf 1fdf

CZ (NORMAL) 00810000: 00000000 00000000 75550000 00e40000
CZ (OFFSET) 00810000: 7fff8000 7fff8000 75550000 00e40000
CZ (OFF)    00810000: 00000000 00000000 31110000 00e40000

SPOT TABLE test
03F282: 13FC 0000 0082 4011        move.b  #$0, $824011.l
03F28A: 13FC 0000 0082 4015        move.b  #$0, $824015.l
03F292: 13FC 0080 0082 400D        move.b  #$80, $82400d.l
03F29A: 13FC 0001 0082 400E        move.b  #$1, $82400e.l
03F2A2: 13FC 0001 0082 4021        move.b  #$1, $824021.l
03F2AA: 33FC 4038 0080 0000        move.w  #$4038, $800000.l
03F2B2: 06B9 0000 0001 00E0 AB08   addi.l  #$1, $e0ab08.l
*/

#include "driver.h"
#include "namcos22.h"
#include <math.h>

static int mbSuperSystem22; /* used to conditionally support Super System22-specific features */
static int mbSpotlightEnable;
static UINT16 *namcos22_czram[4];

static UINT8
nthbyte( const UINT32 *pSource, int offs )
{
	pSource += offs/4;
	return (pSource[0]<<((offs&3)*8))>>24;
}

static UINT16
nthword( const UINT32 *pSource, int offs )
{
	pSource += offs/2;
	return (pSource[0]<<((offs&1)*16))>>16;
}

static struct
{
	int target;
	int rFogColor;
	int gFogColor;
	int bFogColor;
	int rFogColor2;
	int gFogColor2;
	int bFogColor2;
	int rBackColor;
	int gBackColor;
	int bBackColor;
	int rFadeColor;
	int gFadeColor;
	int bFadeColor;
	int fadeFactor;
	int spot_translucency;
	int poly_translucency;
	int text_translucency;
	int palBase;
} mixer;

static void
UpdateVideoMixer( void )
{
	memset( &mixer, 0, sizeof(mixer) );
	if( mbSuperSystem22 )
	{
/*
           0 1 2 3  4 5 6 7  8 9 a b  c d e f 10       14       18       1c
00824000: ffffff00 00000000 0000007f 00ff0000 1000ff00 0f000000 00ff007f 00010007 // time crisis
00824000: ffffff00 00000000 1830407f 00800000 0000007f 0f000000 0000037f 00010007 // trans sprite
00824000: ffffff00 00000000 3040307f 00000000 0080007f 0f000000 0000037f 00010007 // trans poly
00824000: ffffff00 00000000 1800187f 00800000 0080007f 0f000000 0000037f 00010007 // trans poly(2)
00824000: ffffff00 00000000 1800187f 00000000 0000007f 0f800000 0000037f 00010007 // trans text
*/
		mixer.rFogColor         = nthbyte( namcos22_gamma, 0x05 );
		mixer.gFogColor         = nthbyte( namcos22_gamma, 0x06 );
		mixer.bFogColor         = nthbyte( namcos22_gamma, 0x07 );
		mixer.rBackColor        = nthbyte( namcos22_gamma, 0x08 );
		mixer.gBackColor        = nthbyte( namcos22_gamma, 0x09 );
		mixer.bBackColor        = nthbyte( namcos22_gamma, 0x0a );
		mixer.spot_translucency = nthbyte( namcos22_gamma, 0x0d );
		mixer.poly_translucency = nthbyte( namcos22_gamma, 0x11 );
		mixer.text_translucency = nthbyte( namcos22_gamma, 0x15 );
		mixer.rFadeColor        = nthbyte( namcos22_gamma, 0x16 );
		mixer.gFadeColor        = nthbyte( namcos22_gamma, 0x17 );
		mixer.bFadeColor        = nthbyte( namcos22_gamma, 0x18 );
		mixer.fadeFactor        = nthbyte( namcos22_gamma, 0x19 );
		mixer.target            = nthbyte( namcos22_gamma, 0x1a );
		mixer.palBase           = nthbyte( namcos22_gamma, 0x1b );
	}
	else
	{
/*
90020000: 4f030000 7f00007f 4d4d4d42 0c00c0c0
90020010: c0010001 00010000 00000000 00000000
90020080: 00010101 01010102 00000000 00000000
900200c0: 00000000 00000000 00000000 03000000
90020100: fff35000 00000000 00000000 00000000
90020180: ff713700 00000000 00000000 00000000
90020200: ff100000 00000000 00000000 00000000
*/
		mixer.palBase     = 0x7f;
		mixer.target      = 0x7;//nthbyte( namcos22_gamma, 0x0002 )*256 + nthbyte( namcos22_gamma, 0x0003 );
		mixer.rFadeColor  = nthbyte( namcos22_gamma, 0x0011 )*256 + nthbyte( namcos22_gamma, 0x0012 );
		mixer.gFadeColor  = nthbyte( namcos22_gamma, 0x0013 )*256 + nthbyte( namcos22_gamma, 0x0014 );
		mixer.bFadeColor  = nthbyte( namcos22_gamma, 0x0015 )*256 + nthbyte( namcos22_gamma, 0x0016 );

		mixer.fadeFactor  = 0x100 - mixer.rFadeColor; // hack
		mixer.rFadeColor  = 0;
		mixer.gFadeColor  = 0;
		mixer.bFadeColor  = 0;

		mixer.rFogColor   = nthbyte( namcos22_gamma, 0x0100 );
		mixer.rFogColor2  = nthbyte( namcos22_gamma, 0x0101 );
		mixer.gFogColor   = nthbyte( namcos22_gamma, 0x0180 );
		mixer.gFogColor2  = nthbyte( namcos22_gamma, 0x0181 );
		mixer.bFogColor   = nthbyte( namcos22_gamma, 0x0200 );
		mixer.bFogColor2  = nthbyte( namcos22_gamma, 0x0201 );

/*  +0x0002.w   Fader Enable(?) (0: disabled)
 *  +0x0011.w   Display Fader (R) (0x0100 = 1.0)
 *  +0x0013.w   Display Fader (G) (0x0100 = 1.0)
 *  +0x0015.w   Display Fader (B) (0x0100 = 1.0)
 *  +0x0100.b   Fog1 Color (R) (world fogging)
 *  +0x0101.b   Fog2 Color (R) (used for heating of brake-disc on RV1)
 *  +0x0180.b   Fog1 Color (G)
 *  +0x0181.b   Fog2 Color (G)
 *  +0x0200.b   Fog1 Color (B)
 *  +0x0201.b   Fog2 Color (B)
 */
	}
}

static UINT8
Clamp256( int v )
{
   if( v<0 )
   {
      v = 0;
   }
   else if( v>255 )
   {
      v = 255;
   }
   return v;
} /* Clamp256 */

static void Dump( FILE *f, unsigned addr1, unsigned addr2, const char *name );

static struct
{
   int cx,cy;
   rectangle scissor;
} mClip;

static void
poly3d_Clip( float vx, float vy, float vw, float vh )
{
   int cx = 320+vx;
   int cy = 240+vy;
   mClip.cx = cx;
   mClip.cy = cy;
   mClip.scissor.min_x = cx + vw;
   mClip.scissor.min_y = cy + vh;
   mClip.scissor.max_x = cx - vw;
   mClip.scissor.max_y = cy - vh;
   if( mClip.scissor.min_x<0 )   mClip.scissor.min_x = 0;
   if( mClip.scissor.min_y<0 )   mClip.scissor.min_y = 0;
   if( mClip.scissor.max_x>639 ) mClip.scissor.max_x = 639;
   if( mClip.scissor.max_y>479 ) mClip.scissor.max_y = 479;
}

static void
poly3d_NoClip( void )
{
   mClip.cx = 640/2;
   mClip.cy = 480/2;
   mClip.scissor.min_x = 0;
   mClip.scissor.max_x = 639;
   mClip.scissor.min_y = 0;
   mClip.scissor.max_x = 479;
}

typedef struct
{
   float x,y,z;
   int u,v; /* 0..0xfff */
   int bri; /* 0..0xff */
} Poly3dVertex;

#define MIN_Z (10.0f)

typedef struct
{
	float x,y;
	float u,v,i,z;
} vertex;

typedef struct
{
	float x;
	float u,v,i,z;
} edge;

#define SWAP(A,B) { const void *temp = A; A = B; B = temp; }

static UINT16 *mpTextureTileMap16;
static UINT8 *mpTextureTileMapAttr;
static UINT8 *mpTextureTileData;
static UINT8 mXYAttrToPixel[16][16][16];

INLINE unsigned texel( unsigned x, unsigned y )
{
	unsigned offs = ((y&0xfff0)<<4)|((x&0xff0)>>4);
	unsigned tile = mpTextureTileMap16[offs];
	return mpTextureTileData[(tile<<8)|mXYAttrToPixel[mpTextureTileMapAttr[offs]][x&0xf][y&0xf]];
} /* texel */

typedef void drawscanline_t(
	mame_bitmap *bitmap,
	const rectangle *clip,
	const edge *e1,
	const edge *e2,
	int sy );

static void
renderscanline_uvi_full( mame_bitmap *bitmap, const rectangle *clip, const edge *e1, const edge *e2, int sy, int color, int bn, UINT16 flags, int cmode )
{
	int fadeEnable = (mixer.target&1) && mixer.fadeFactor;
	int fogDisable = color&0x80;
	const pen_t *pens = &Machine->pens[(color&0x7f)<<8];
   int fogDensity = 0;
	int prioverchar = (cmode&7)==1;

	if( e1->x > e2->x )
	{
		SWAP(e1,e2);
	}

	{
		const UINT8 *pCharPri = (UINT8 *)priority_bitmap->line[sy];
		UINT32 *pDest = (UINT32 *)bitmap->line[sy];
		int x0 = (int)e1->x;
		int x1 = (int)e2->x;
		int w = x1-x0;
		if( w )
		{
			float u = e1->u; /* u/z */
			float v = e1->v; /* v/z */
			float i = e1->i; /* i/z */
			float z = e1->z; /* 1/z */
			float oow = 1.0f / w;
			float du = (e2->u - e1->u)*oow;
			float dv = (e2->v - e1->v)*oow;
			float dz = (e2->z - e1->z)*oow;
			float di = (e2->i - e1->i)*oow;
			int x, crop;
			crop = clip->min_x - x0;
			if( crop>0 )
			{
				u += crop*du;
				v += crop*dv;
				i += crop*di;
				z += crop*dz;
				x0 = clip->min_x;
			}
			if( x1>clip->max_x )
			{
				x1 = clip->max_x;
			}
         bn *= 0x1000;

         if( mbSuperSystem22 )
         {
				if( !fogDisable )
				{
					int cz = flags>>8;
					static const int cztype_remap[4] = { 3,1,2,0 };
	            int cztype = flags&3;
	            if( nthword(namcos22_czattr,4)&(0x4000>>(cztype*4)) )
	            {
	               int fogDelta = (INT16)nthword(namcos22_czattr, cztype);
			//          if( fogDelta == 0x8000 ) fogDelta = -0x7fff;
	               //cz = Clamp256(cz+fogDelta);
	               fogDensity = fogDelta + namcos22_czram[cztype_remap[cztype]][cz];
						if( fogDensity<0x0000 )
						{
							fogDensity = 0x0000;
						}
						else if( fogDensity>0x1fff )
						{
							fogDensity = 0x1fff;
						}
					}
				}
         }
			else
			{
				fogDisable = 1;
			}

         for( x=x0; x<x1; x++ )
			{
				if( pCharPri[x]==0 || prioverchar )
				{
					float ooz = 1.0f/z;
					int pen = texel((int)(u * ooz),bn+(int)(v*ooz));
					switch( cmode )
					{
					case 0x2:
						 pen = 0xe0|(pen>>4);
						 break;
					case 0x3:
						pen = 0xe0|(pen&0xf);
						break;
					case 0x4:
						pen = 0xec|(pen>>6);
						break;
					case 0x5:
						pen = 0xec|((pen>>4)&3);
						break;
					case 0x6:
						pen = 0xec|((pen>>2)&3);
						break;
					case 0x7:
						pen = 0xec|(pen&3);
						break;
					case 0xa:
						pen = 0xf0|(pen>>4);
						break;
					case 0xb:
						pen = 0xf0|(pen&0xf);
						break;
					case 0xc:
						pen = 0xfc|(pen>>6);
						break;
					case 0xd:
						pen = 0xfc|((pen>>4)&3);
						break;
					case 0xe:
						pen = 0xfc|((pen>>2)&3);
						break;
					case 0xf:
						pen = 0xfc|(pen&3);
						break;
					default:
						break;
					}

					{
						UINT32 rgb = pens[pen];
						int shade = i*ooz;
	   				int r = rgb>>16;
						int g = (rgb>>8)&0xff;
						int b = rgb&0xff;
		            r = r*shade/0x40;
		            if( r>0xff ) r = 0xff;
			         g = g*shade/0x40;
				      if( g>0xff ) g = 0xff;
					   b = b*shade/0x40;
		   			if( b>0xff ) b = 0xff;
						if( !fogDisable )
						{
						   int fogDensity2 = 0x2000 - fogDensity;
						   r = (r*fogDensity2 + fogDensity*mixer.rFogColor)>>13;
						   g = (g*fogDensity2 + fogDensity*mixer.gFogColor)>>13;
						   b = (b*fogDensity2 + fogDensity*mixer.bFogColor)>>13;
						}
						if( fadeEnable )
						{
							int fade2 = 0x100-mixer.fadeFactor;
							r = (r*fade2+mixer.fadeFactor*mixer.rFadeColor)>>8;
							g = (g*fade2+mixer.fadeFactor*mixer.gFadeColor)>>8;
							b = (b*fade2+mixer.fadeFactor*mixer.bFadeColor)>>8;
						}
						if( prioverchar )
						{
							UINT32 color = pDest[x];
							int tr = color>>16;
							int tg = (color>>8)&0xff;
							int tb = color&0xff;
							int trans1 = 0x100 - mixer.poly_translucency;
							r = (tr*mixer.poly_translucency + r*trans1)/0x100;
							g = (tg*mixer.poly_translucency + g*trans1)/0x100;
							b = (tb*mixer.poly_translucency + b*trans1)/0x100;
						}
						rgb = (r<<16)|(g<<8)|b;
						pDest[x] = rgb;
					}
				}

            u += du;
				v += dv;
				i += di;
				z += dz;
			}
		}
	}
} /* renderscanline_uvi_full */

static void
rendertri(
		mame_bitmap *bitmap,
		const rectangle *clip,
      int color,
      int bn,
		const vertex *v0,
		const vertex *v1,
		const vertex *v2,
      UINT16 flags,
		int cmode )
{
	int dy,ystart,yend,crop;

	/* first, sort so that v0->y <= v1->y <= v2->y */
	for(;;)
	{
		if( v0->y > v1->y )
		{
			SWAP(v0,v1);
		}
		else if( v1->y > v2->y )
		{
			SWAP(v1,v2);
		}
		else
		{
			break;
		}
	}

	ystart = v0->y;
	yend   = v2->y;
	dy = yend-ystart;
	if( dy )
	{
		int y;
		edge e1; /* short edge (top and bottom) */
		edge e2; /* long (common) edge */

		float oody = 1.0f / (float)dy;

		float dx2dy = (v2->x - v0->x)*oody;
		float du2dy = (v2->u - v0->u)*oody;
		float dv2dy = (v2->v - v0->v)*oody;
		float di2dy = (v2->i - v0->i)*oody;
		float dz2dy = (v2->z - v0->z)*oody;

		float dx1dy;
		float du1dy;
		float dv1dy;
		float di1dy;
		float dz1dy;

		e2.x = v0->x;
		e2.u = v0->u;
		e2.v = v0->v;
		e2.i = v0->i;
		e2.z = v0->z;
		crop = clip->min_y - ystart;
		if( crop>0 )
		{
			e2.x += dx2dy*crop;
			e2.u += du2dy*crop;
			e2.v += dv2dy*crop;
			e2.i += di2dy*crop;
			e2.z += dz2dy*crop;
		}

		ystart = v0->y;
		yend = v1->y;
		dy = yend-ystart;

		if( dy )
		{
			e1.x = v0->x;
			e1.u = v0->u;
			e1.v = v0->v;
			e1.i = v0->i;
			e1.z = v0->z;

			oody = 1.0f / (float)dy;
			dx1dy = (v1->x - v0->x)*oody;
			du1dy = (v1->u - v0->u)*oody;
			dv1dy = (v1->v - v0->v)*oody;
			di1dy = (v1->i - v0->i)*oody;
			dz1dy = (v1->z - v0->z)*oody;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline_uvi_full( bitmap, clip, &e1, &e2, y, color, bn, flags, cmode );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}

		ystart = v1->y;
		yend = v2->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v1->x;
			e1.u = v1->u;
			e1.v = v1->v;
			e1.i = v1->i;
			e1.z = v1->z;

			oody = 1.0f / (float)dy;
			dx1dy = (v2->x - v1->x)*oody;
			du1dy = (v2->u - v1->u)*oody;
			dv1dy = (v2->v - v1->v)*oody;
			di1dy = (v2->i - v1->i)*oody;
			dz1dy = (v2->z - v1->z)*oody;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline_uvi_full( bitmap, clip, &e1,&e2,y, color, bn, flags, cmode );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}
	}
} /* rendertri */

static void
ProjectPoint( const Poly3dVertex *v, vertex *pv, int bDirect )
{
	float ooz;
   if( bDirect )
   {
      ooz = v->z;
      pv->x = mClip.cx + v->x;
      pv->y = mClip.cy - v->y;
   }
   else
   {
      ooz = 1.0f/v->z;
      pv->x = mClip.cx + v->x*ooz;
   	pv->y = mClip.cy - v->y*ooz;
   }
   pv->z = ooz;
	pv->u = (v->u+0.5f)*ooz;
	pv->v = (v->v+0.5f)*ooz;
	pv->i = (v->bri+0.5f)*ooz;
} /* ProjectPoint */

static void
BlitTriHelper(
		mame_bitmap *pBitmap,
		const Poly3dVertex *v0,
		const Poly3dVertex *v1,
		const Poly3dVertex *v2,
		unsigned color,
      int bn,
      UINT16 flags,
      int bDirect,
		int cmode )
{
	vertex a,b,c;
   ProjectPoint( v0,&a,bDirect );
   ProjectPoint( v1,&b,bDirect );
   ProjectPoint( v2,&c,bDirect );
   rendertri( pBitmap, &mClip.scissor, color, bn, &a, &b, &c, flags, cmode );
} /* BlitTriHelper */

static float
interp( float x0, float ns3d_y0, float x1, float ns3d_y1 )
{
	float m = (ns3d_y1-ns3d_y0)/(x1-x0);
	float b = ns3d_y0 - m*x0;
	return m*MIN_Z+b;
}

static int
VertexEqual( const Poly3dVertex *a, const Poly3dVertex *b )
{
	return
		a->x == b->x &&
		a->y == b->y &&
		a->z == b->z;
}

static void
BlitTri(
	mame_bitmap *pBitmap,
	const Poly3dVertex *pv[3],
	int color,
   int bn,
   UINT16 flags,
   int bDirect,
	int cmode )
{
	Poly3dVertex vc[3];
	int i,j;
	int iBad = 0, iGood = 0;
	int bad_count = 0;

   if( bDirect )
   {
		BlitTriHelper( pBitmap, pv[0],pv[1],pv[2], color,bn,flags,bDirect,cmode );
      return;
   }

	/* don't bother rendering a degenerate triangle */
	if( VertexEqual(pv[0],pv[1]) ) return;
	if( VertexEqual(pv[0],pv[2]) ) return;
	if( VertexEqual(pv[1],pv[2]) ) return;

	for( i=0; i<3; i++ )
	{
		if( pv[i]->z<MIN_Z )
		{
			bad_count++;
			iBad = i;
		}
		else
		{
			iGood = i;
		}
	}

	switch( bad_count )
	{
	case 0:
		BlitTriHelper( pBitmap, pv[0],pv[1],pv[2], color,bn,flags,bDirect,cmode );
		break;

	case 1:
		vc[0] = *pv[0];
      vc[1] = *pv[1];
      vc[2] = *pv[2];

		i = (iBad+1)%3;
		vc[iBad].x   = interp( pv[i]->z,pv[i]->x,   pv[iBad]->z, pv[iBad]->x  );
		vc[iBad].y   = interp( pv[i]->z,pv[i]->y,   pv[iBad]->z, pv[iBad]->y  );
		vc[iBad].u   = interp( pv[i]->z,pv[i]->u,   pv[iBad]->z, pv[iBad]->u );
		vc[iBad].v   = interp( pv[i]->z,pv[i]->v,   pv[iBad]->z, pv[iBad]->v );
		vc[iBad].bri = interp( pv[i]->z,pv[i]->bri, pv[iBad]->z, pv[iBad]->bri );
		vc[iBad].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color,bn,flags,bDirect,cmode );

		j = (iBad+2)%3;
		vc[i].x   = interp(pv[j]->z,pv[j]->x,   pv[iBad]->z,pv[iBad]->x  );
		vc[i].y   = interp(pv[j]->z,pv[j]->y,   pv[iBad]->z,pv[iBad]->y  );
		vc[i].u   = interp(pv[j]->z,pv[j]->u,   pv[iBad]->z,pv[iBad]->u );
		vc[i].v   = interp(pv[j]->z,pv[j]->v,   pv[iBad]->z,pv[iBad]->v );
		vc[i].bri = interp(pv[j]->z,pv[j]->bri, pv[iBad]->z,pv[iBad]->bri );
		vc[i].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color,bn,flags,bDirect,cmode );
		break;

	case 2:
		vc[0] = *pv[0];
      vc[1] = *pv[1];
      vc[2] = *pv[2];

		i = (iGood+1)%3;
		vc[i].x   = interp(pv[iGood]->z,pv[iGood]->x,   pv[i]->z,pv[i]->x  );
		vc[i].y   = interp(pv[iGood]->z,pv[iGood]->y,   pv[i]->z,pv[i]->y  );
		vc[i].u   = interp(pv[iGood]->z,pv[iGood]->u,   pv[i]->z,pv[i]->u );
		vc[i].v   = interp(pv[iGood]->z,pv[iGood]->v,   pv[i]->z,pv[i]->v );
		vc[i].bri = interp(pv[iGood]->z,pv[iGood]->bri, pv[i]->z,pv[i]->bri );
		vc[i].z = MIN_Z;

		i = (iGood+2)%3;
		vc[i].x   = interp(pv[iGood]->z,pv[iGood]->x,   pv[i]->z,pv[i]->x  );
		vc[i].y   = interp(pv[iGood]->z,pv[iGood]->y,   pv[i]->z,pv[i]->y  );
		vc[i].u   = interp(pv[iGood]->z,pv[iGood]->u,   pv[i]->z,pv[i]->u );
		vc[i].v   = interp(pv[iGood]->z,pv[iGood]->v,   pv[i]->z,pv[i]->v );
		vc[i].bri = interp(pv[iGood]->z,pv[iGood]->bri, pv[i]->z,pv[i]->bri );
		vc[i].z = MIN_Z;

		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color,bn,flags,bDirect,cmode );
		break;

	case 3:
		/* wholly clipped */
		break;
	}
} /* BlitTri */

static void
poly3d_DrawQuad( mame_bitmap *pBitmap, int textureBank, int color, Poly3dVertex v[4], UINT16 flags, int bDirect, int cmode )
{
   const Poly3dVertex *pv[3];

   pv[0] = &v[0];
   pv[1] = &v[1];
   pv[2] = &v[2];
   BlitTri( pBitmap, pv, color, textureBank, flags, bDirect, cmode );

   pv[0] = &v[2];
   pv[1] = &v[3];
   pv[2] = &v[0];
   BlitTri( pBitmap, pv, color, textureBank, flags, bDirect, cmode );
}

static void
mydrawgfxzoom(
	mame_bitmap *dest_bmp,const gfx_element *gfx,
	unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
	const rectangle *clip,int transparency,int transparent_color,
	int scalex, int scaley, int z, int prioverchar )
{
	rectangle myclip;
	if (!scalex || !scaley) return;
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}
	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];
		UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
		int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
		int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;
		if (sprite_screen_width && sprite_screen_height)
		{
			int dx = (gfx->width<<16)/sprite_screen_width;
			int dy = (gfx->height<<16)/sprite_screen_height;
			int ex = sx+sprite_screen_width;
			int ey = sy+sprite_screen_height;
			int x_index_base;
			int y_index;

         int bFogEnable = 0;
         INT16 fogDelta = 0;
			int fadeEnable = (mixer.target&2) && mixer.fadeFactor;

         if( mbSuperSystem22 )
         {
            fogDelta = nthword(namcos22_czattr, 0 );
            bFogEnable = nthword(namcos22_czattr,4)&0x4000; /* ? */
         }
			else
			{
				bFogEnable = 0;
			}

			if( flipx )
			{
				x_index_base = (sprite_screen_width-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}
			if( flipy )
			{
				y_index = (sprite_screen_height-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}
			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}
			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
				int y;
				for( y=sy; y<ey; y++ )
				{
					UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
					UINT32 *dest = (UINT32 *)dest_bmp->line[y];
					const UINT8 *pCharPri = (UINT8 *)priority_bitmap->line[y];
					int x, x_index = x_index_base;
					for( x=sx; x<ex; x++ )
					{
						int pen = source[x_index>>16];
						if( pen != transparent_color )
						{
							if( pCharPri[x]==0 || prioverchar )
							{
	                     UINT32 color = pal[pen];
		                  int r = color>>16;
			               int g = (color>>8)&0xff;
				      		int b = color&0xff;
	                     if( bFogEnable && z!=0xffff )
		                  {
			                  int zc = Clamp256(fogDelta + z);
				               UINT16 fogDensity = namcos22_czram[3][zc];
					            if( fogDensity>0 )
						         {
							         int fogDensity2 = 0x2000 - fogDensity;
								      r = (r*fogDensity2 + fogDensity*mixer.rFogColor)>>13;
									   g = (g*fogDensity2 + fogDensity*mixer.gFogColor)>>13;
										b = (b*fogDensity2 + fogDensity*mixer.bFogColor)>>13;
									}
								}
	                     if( fadeEnable )
		                  {
			                  int fade2 = 0x100-mixer.fadeFactor;
				               r = (r*fade2+mixer.fadeFactor*mixer.rFadeColor)>>8;
					            g = (g*fade2+mixer.fadeFactor*mixer.gFadeColor)>>8;
						         b = (b*fade2+mixer.fadeFactor*mixer.bFadeColor)>>8;
							   }
	                     color = (r<<16)|(g<<8)|b;
		                  color = alpha_blend32(dest[x], color);
			               color&=0xffffff;
				            dest[x] = color;
							}
						}
						x_index += dx;
					}
					y_index += dy;
				}
			}
		}
	}
} /* mydrawgfxzoom */

static void
ApplyGamma( mame_bitmap *bitmap )
{
	int x,y;
	if( mbSuperSystem22 )
   { /* super system 22 */
#ifdef LSB_FIRST
#define XORPAT 0x3
#else
#define XORPAT 0x0
#endif
		const UINT8 *rlut = (const UINT8 *)&namcos22_gamma[0x100/4];
		const UINT8 *glut = (const UINT8 *)&namcos22_gamma[0x200/4];
		const UINT8 *blut = (const UINT8 *)&namcos22_gamma[0x300/4];
		for( y=0; y<bitmap->height; y++ )
		{
			UINT32 *dest = (UINT32 *)bitmap->line[y];
			for( x=0; x<bitmap->width; x++ )
			{
				int rgb = dest[x];
				int r = rlut[XORPAT^(rgb>>16)];
				int g = glut[XORPAT^((rgb>>8)&0xff)];
				int b = blut[XORPAT^(rgb&0xff)];
				dest[x] = (r<<16)|(g<<8)|b;
			}
		}
	}
	else
	{ /* system 22 */
		const UINT8 *rlut = 0x000+(const UINT8 *)memory_region(REGION_USER1);
		const UINT8 *glut = 0x100+(const UINT8 *)memory_region(REGION_USER1);
		const UINT8 *blut = 0x200+(const UINT8 *)memory_region(REGION_USER1);
		for( y=0; y<bitmap->height; y++ )
		{
			UINT32 *dest = (UINT32 *)bitmap->line[y];
			for( x=0; x<bitmap->width; x++ )
			{
				int rgb = dest[x];
				int r = rlut[rgb>>16];
				int g = glut[(rgb>>8)&0xff];
				int b = blut[rgb&0xff];
				dest[x] = (r<<16)|(g<<8)|b;
			}
		}
	}
} /* ApplyGamma */

static void
poly3d_Draw3dSprite( mame_bitmap *bitmap, gfx_element *gfx, int tileNumber, int color, int sx, int sy, int width, int height, int translucency, int zc, UINT32 pri )
{
   int flipx = 0;
   int flipy = 0;
   rectangle clip;
   clip.min_x = 0;
   clip.min_y = 0;
   clip.max_x = 640-1;
   clip.max_y = 480-1;
   alpha_set_level( 0xff - translucency );
   mydrawgfxzoom(
      bitmap,
      gfx,
      tileNumber,
      color,
      flipx, flipy,
      sx, sy,
      &clip,
      TRANSPARENCY_ALPHA, 0xff,
      (width<<16)/32,
      (height<<16)/32,
      zc, pri );
}

#define DSP_FIXED_TO_FLOAT( X ) (((INT16)(X))/(float)0x7fff)
#define SPRITERAM_SIZE (0x9b0000-0x980000)
#define CGRAM_SIZE 0x1e000
#define NUM_CG_CHARS ((CGRAM_SIZE*8)/(64*16)) /* 0x3c0 */

/* 16 bit access to DSP RAM */
static UINT16 namcos22_dspram_bank;
static UINT16 mUpperWordLatch;

static int mbDSPisActive;

/* modal rendering properties */
static INT32 mAbsolutePriority;
static INT32 mObjectShiftValue22;

static UINT16 mPrimitiveID; /* 3d primitive to render */

static float mViewMatrix[4][4];

static void
matrix3d_Multiply( float A[4][4], float B[4][4] )
{
	float temp[4][4];
	int row,col;

	for( row=0;row<4;row++ )
	{
		for(col=0;col<4;col++)
		{
			float sum = 0.0;
			int i;
			for( i=0; i<4; i++ )
			{
				sum += A[row][i]*B[i][col];
			}
			temp[row][col] = sum;
		}
	}
	memcpy( A, temp, sizeof(temp) );
} /* matrix3d_Multiply */

static void
matrix3d_Identity( float M[4][4] )
{
	int r,c;

	for( r=0; r<4; r++ )
	{
		for( c=0; c<4; c++ )
		{
			M[r][c] = (r==c)?1.0:0.0;
		}
	}
} /* matrix3d_Identity */

static void
TransformPoint( float *vx, float *vy, float *vz, float m[4][4] )
{
	float x = *vx;
	float y = *vy;
	float z = *vz;
	*vx = m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0];
	*vy = m[0][1]*x + m[1][1]*y + m[2][1]*z + m[3][1];
	*vz = m[0][2]*x + m[1][2]*y + m[2][2]*z + m[3][2];
}

static void
TransformNormal( float *nx, float *ny, float *nz, float m[4][4] )
{
	float x = *nx;
	float y = *ny;
	float z = *nz;
	*nx = m[0][0]*x + m[1][0]*y + m[2][0]*z;
	*ny = m[0][1]*x + m[1][1]*y + m[2][1]*z;
	*nz = m[0][2]*x + m[1][2]*y + m[2][2]*z;
}

#define MAX_LIT_SURFACES 32
static UINT8 mLitSurfaceInfo[MAX_LIT_SURFACES];
static INT32 mSurfaceNormalFormat;

static unsigned mLitSurfaceCount;
static unsigned mLitSurfaceIndex;

static int mPtRomSize;
static const UINT8 *mpPolyH;
static const UINT8 *mpPolyM;
static const UINT8 *mpPolyL;

static struct
{
	float zoom, vx, vy, vw, vh;
	float lx,ly,lz; /* unit vector for light direction */
	int ambient; /* 0.0..1.0 */
	int power;	/* 0.0..1.0 */
} mCamera;

typedef enum
{
   eSCENENODE_NONLEAF,
   eSCENENODE_QUAD3D,
   eSCENENODE_SPRITE
} SceneNodeType;

#define RADIX_BITS 4
#define RADIX_BUCKETS (1<<RADIX_BITS)
#define RADIX_MASK (RADIX_BUCKETS-1)

struct SceneNode
{
   SceneNodeType type;
   struct SceneNode *nextInBucket;
   union
   {
      struct
      {
         struct SceneNode *next[RADIX_BUCKETS];
      } nonleaf;

      struct
      {
         float vx,vy,vw,vh;
         int textureBank;
         int color;
         int cmode;
         int flags;
         int bDirect;
         Poly3dVertex v[4];
      } quad3d;

      struct
      {
         int tile, color, pri;
         int flipx, flipy;
			int linkType;
	      int numcols, numrows;
	      int xpos, ypos;
	      int sizex, sizey;
         int translucency;
         int cz;
      } sprite;
   } data;
};
static struct SceneNode mSceneRoot;
static struct SceneNode *mpFreeSceneNode;

static void
FreeSceneNode( struct SceneNode *node )
{
   node->nextInBucket = mpFreeSceneNode;
   mpFreeSceneNode = node;
} /* FreeSceneNode */

static struct SceneNode *
MallocSceneNode( void )
{
   struct SceneNode *node = mpFreeSceneNode;
   if( node )
   { /* use free pool */
      mpFreeSceneNode = node->nextInBucket;
   }
   else
   {
#define SCENE_NODE_POOL_SIZE 64
		struct SceneNode *pSceneNodePool = auto_malloc(sizeof(struct SceneNode)*SCENE_NODE_POOL_SIZE);
		{
			int i;
			for( i=0; i<SCENE_NODE_POOL_SIZE; i++ )
			{
				FreeSceneNode( &pSceneNodePool[i] );
			}
			return MallocSceneNode();
		}
   }
   memset( node, 0, sizeof(*node) );
   return node;
} /* MallocSceneNode */

static struct SceneNode *
NewSceneNode( UINT32 zsortvalue24, SceneNodeType type )
{
   struct SceneNode *node = &mSceneRoot;
   int i;
   for( i=0; i<24; i+=RADIX_BITS )
   {
      int hash = (zsortvalue24>>20)&RADIX_MASK;
      struct SceneNode *next = node->data.nonleaf.next[hash];
      if( !next )
      { /* lazily allocate tree node for this radix */
         next = MallocSceneNode();
         next->type = eSCENENODE_NONLEAF;
         node->data.nonleaf.next[hash] = next;
      }
      node = next;
      zsortvalue24 <<= RADIX_BITS;
   }

   if( node->type == eSCENENODE_NONLEAF )
   { /* first leaf allocation on this branch */
      node->type = type;
      return node;
   }
   else
   {
      struct SceneNode *leaf = MallocSceneNode();
      leaf->type = type;
#if 0
      leaf->nextInBucket = node->nextInBucket;
      node->nextInBucket = leaf;
#else
		/* stable insertion sort */
		leaf->nextInBucket = NULL;
		while( node->nextInBucket )
		{
			node = node->nextInBucket;
		}
		node->nextInBucket = leaf;
#endif
      return leaf;
   }
} /* NewSceneNode */

static void
RenderSprite( mame_bitmap *bitmap, struct SceneNode *node )
{
   int tile = node->data.sprite.tile;
   int col,row;
	int i = 0;
   for( row=0; row<node->data.sprite.numrows; row++ )
   {
      for( col=0; col<node->data.sprite.numcols; col++ )
   	{
         int code = tile;
         if( node->data.sprite.linkType == 0xff )
         {
            code += i;
         }
         else
         {
            code += nthword( &spriteram32[0x800/4], i+node->data.sprite.linkType*4 );
         }
         poly3d_Draw3dSprite(
               bitmap,
               Machine->gfx[GFX_SPRITE],
               code,
               node->data.sprite.color,
               node->data.sprite.xpos+col*node->data.sprite.sizex,
               node->data.sprite.ypos+row*node->data.sprite.sizey,
               node->data.sprite.sizex,
               node->data.sprite.sizey,
               node->data.sprite.translucency,
               node->data.sprite.cz,
               node->data.sprite.pri );
      	i++;
      } /* next col */
	} /* next row */
} /* RenderSprite */

static void
RenderSceneHelper( mame_bitmap *bitmap, struct SceneNode *node )
{
   if( node )
   {
      if( node->type == eSCENENODE_NONLEAF )
      {
         int i;
         for( i=RADIX_BUCKETS-1; i>=0; i-- )
         {
            RenderSceneHelper( bitmap, node->data.nonleaf.next[i] );
         }
         FreeSceneNode( node );
      }
      else
      {
         while( node )
         {
            struct SceneNode *next = node->nextInBucket;

            switch( node->type )
            {
            case eSCENENODE_QUAD3D:
               poly3d_Clip(
                  node->data.quad3d.vx,
                  node->data.quad3d.vy,
                  node->data.quad3d.vw,
                  node->data.quad3d.vh );
               poly3d_DrawQuad(
                  bitmap,
                  node->data.quad3d.textureBank,
                  node->data.quad3d.color,
                  node->data.quad3d.v,
                  node->data.quad3d.flags,
                  node->data.quad3d.bDirect,
                  node->data.quad3d.cmode );
               break;

            case eSCENENODE_SPRITE:
               poly3d_NoClip();
               RenderSprite( bitmap,node );
               break;

            default:
               fatalerror("invalid node->type");
               break;
            }
            FreeSceneNode( node );
            node = next;
         }
      }
   }
} /* RenderSceneHelper */

static void
RenderScene( mame_bitmap *bitmap )
{
   struct SceneNode *node = &mSceneRoot;
   int i;
   for( i=RADIX_BUCKETS-1; i>=0; i-- )
   {
      RenderSceneHelper( bitmap, node->data.nonleaf.next[i] );
      node->data.nonleaf.next[i] = NULL;
   }
   poly3d_NoClip();
} /* RenderScene */

static float
DspFloatToNativeFloat( UINT32 iVal )
{
   INT16 mantissa = (INT16)iVal;
   float result = mantissa;//?((float)mantissa):((float)0x10000);
	int exponent = (iVal>>16)&0xff;
	while( exponent<0x2e )
	{
		result /= 2.0;
		exponent++;
	}
	return result;
} /* DspFloatToNativeFloat */

static INT32
GetPolyData( INT32 addr )
{
	INT32 result;
	if( addr<0 || addr>=mPtRomSize )
	{
		return -1; /* HACK */
	}
	result = (mpPolyH[addr]<<16)|(mpPolyM[addr]<<8)|mpPolyL[addr];
	if( result&0x00800000 )
	{
		result |= 0xff000000; /* sign extend */
	}
	return result;
} /* GetPolyData */

UINT32
namcos22_point_rom_r( offs_t offs )
{
	return GetPolyData(offs);
}

UINT32 *namcos22_cgram;
UINT32 *namcos22_textram;
UINT32 *namcos22_polygonram;
UINT32 *namcos22_gamma;
UINT32 *namcos22_vics_data;
UINT32 *namcos22_vics_control;

/*
                         0    2    4    6    8    a    b
                      ^^^^ ^^^^ ^^^^ ^^^^                        cz offset
                                          ^^^^                   target (4==poly)
                                                    ^^^^         ????
         //00810000:  0000 0000 0000 0000 4444 0000 0000 0000 // solitar
         //00810000:  0000 0000 0000 0000 7555 0000 00e4 0000 // normal
         //00810000:  7fff 8000 7fff 8000 7555 0000 00e4 0000 // offset
         //00810000:  0000 0000 0000 0000 3111 0000 00e4 0000 // off
         //00810000:  0004 0004 0004 0004 4444 0000 0000 0000 // out pool
         //00810000:  00a4 00a4 00a4 00a4 4444 0000 0000 0000 // in pool
         //00810000:  ff80 ff80 ff80 ff80 4444 0000 0000 0000 // ending
         //00810000:  ff80 ff80 ff80 ff80 0000 0000 0000 0000 // hs entry
         //00810000:  ff01 ff01 0000 0000 0000 0000 00e4 0000 // alpine racer
*/
UINT32 *namcos22_czattr;

UINT32 *namcos22_tilemapattr;

static int cgsomethingisdirty;
static unsigned char *cgdirty;
static unsigned char *dirtypal;

READ32_HANDLER( namcos22_czram_r )
{
   int bank = nthword(namcos22_czattr,0xa/2);
   const UINT16 *czram = namcos22_czram[bank&3];
   return (czram[offset*2]<<16)|czram[offset*2+1];
}

WRITE32_HANDLER( namcos22_czram_w )
{
   int bank = nthword(namcos22_czattr,0xa/2);
   UINT16 *czram = namcos22_czram[bank&3];
   UINT32 dat = (czram[offset*2]<<16)|czram[offset*2+1];
   COMBINE_DATA( &dat );
   czram[offset*2] = dat>>16;
   czram[offset*2+1] = dat&0xffff;
}

static void
InitXYAttrToPixel( void )
{
	unsigned attr,x,y,ix,iy,temp;
	for( attr=0; attr<16; attr++ )
	{
		for( y=0; y<16; y++ )
		{
			for( x=0; x<16; x++ )
			{
				ix = x; iy = y;
				if( attr&4 ) ix = 15-ix;
				if( attr&2 ) iy = 15-iy;
				if( attr&8 ){ temp = ix; ix = iy; iy = temp; }
				mXYAttrToPixel[attr][x][y] = (iy<<4)|ix;
			}
		}
	}
} /* InitXYAttrToPixel */

static void
PatchTexture( void )
{
	int i;
	switch( namcos22_gametype )
	{
	case NAMCOS22_RIDGE_RACER:
	case NAMCOS22_RIDGE_RACER2:
	case NAMCOS22_ACE_DRIVER:
	case NAMCOS22_CYBER_COMMANDO:
   	for( i=0; i<0x100000; i++ )
   	{
   		int tile = mpTextureTileMap16[i];
   		int attr = mpTextureTileMapAttr[i];
   		if( (attr&0x1)==0 )
   		{
   			tile = (tile&0x3fff)|0x8000;
   			mpTextureTileMap16[i] = tile;
   		}
   	}
		break;

	default:
   	break;
   }
} /* PatchTexture */

void
namcos22_draw_direct_poly( const UINT16 *pSource )
{
   /**
    * word#0:
    *    x--------------- end-of-display-list marker
    *    ----xxxxxxxxxxxx priority (lo)
    *
    * word#1:
    *    ----xxxxxxxxxxxx priority (hi)
    *
    * word#2:
    *    xxxxxxxx-------- PAL (high bit is fog enable)
    *    --------xxxx---- CMODE (color mode for texture unpack)
     *    ------------xxxx BN (texture bank)
    *
    * word#3:
    *    xxxxxxxx-------- ZC
    *    --------------xx depth cueing table select
    *
    * for each vertex:
    *    xxxx xxxx // u,v
     *
    *    xxxx xxxx // sx,sy
     *
    *    xx-- ---- // BRI
     *    --xx xxxx // zpos
     */
   INT32 zsortvalue24 = ((pSource[1]&0xfff)<<12)|(pSource[0]&0xfff);
   struct SceneNode *node = NewSceneNode(zsortvalue24,eSCENENODE_QUAD3D);
   int i;
   node->data.quad3d.flags = ((pSource[3]&0x7f00)*2)|(pSource[3]&3);
	node->data.quad3d.cmode = (pSource[2]&0x00f0)>>4;
	node->data.quad3d.textureBank = pSource[2]&0xf;
   node->data.quad3d.color = (pSource[2]&0xff00)>>8;
   pSource += 4;
   for( i=0; i<4; i++ )
   {
      Poly3dVertex *p = &node->data.quad3d.v[i];

      p->u = pSource[0];
      p->v = pSource[1];
      if( mbSuperSystem22 )
      {
         p->u >>= 4;
         p->v >>= 4;
      }
      p->u &= 0xfff;
      p->v &= 0xfff;

      {
         int mantissa = (INT16)pSource[5];
         float zf = (float)mantissa;
      	int exponent = (pSource[4])&0xff;
         if( mantissa )
         {
   	      while( exponent<0x2e )
      	   {
	      	   zf /= 2.0;
		         exponent++;
	         }
            if( mbSuperSystem22 )
            {
               p->z = zf;
            }
            else
            {
               p->z = 1.0f/zf;
            }
         }
         else
         {
            zf = (float)0x10000;
            exponent = 0x40-exponent;
   	      while( exponent<0x2e )
      	   {
	      	   zf /= 2.0;
		         exponent++;
	         }
            p->z = 1.0f/zf;
         }
      }

      p->x = ((INT16)pSource[2]);
      p->y = (-(INT16)pSource[3]);
      p->bri = pSource[4]>>8;
      pSource += 6;
   }
   node->data.quad3d.bDirect = 1;
   node->data.quad3d.vx = 0;
   node->data.quad3d.vy = 0;
   node->data.quad3d.vw = -320;
   node->data.quad3d.vh = -240;
} /* namcos22_draw_direct_poly */

static int
Prepare3dTexture( void *pTilemapROM, void *pTextureROM )
{
    int i;
    if( pTilemapROM && pTextureROM )
    { /* following setup is Namco System 22 specific */
	      const UINT8 *pPackedTileAttr = 0x200000 + (UINT8 *)pTilemapROM;
	      UINT8 *pUnpackedTileAttr = auto_malloc(0x080000*2);
      	{
       	   InitXYAttrToPixel();
   	      mpTextureTileMapAttr = pUnpackedTileAttr;
   	      for( i=0; i<0x80000; i++ )
   	      {
   	         *pUnpackedTileAttr++ = (*pPackedTileAttr)>>4;
   	         *pUnpackedTileAttr++ = (*pPackedTileAttr)&0xf;
   	         pPackedTileAttr++;
   	   }
   	   mpTextureTileMap16 = pTilemapROM;
         mpTextureTileData = pTextureROM;
   	   PatchTexture();
 	      return 0;
      }
   }
   return -1;
} /* Prepare3dTexture */

static void
DrawSpritesHelper(
	mame_bitmap *bitmap,
	const rectangle *cliprect,
	const UINT32 *pSource,
	const UINT32 *pPal,
	int num_sprites,
	int deltax,
	int deltay )
{
   int i;
	for( i=0; i<num_sprites; i++ )
	{
      /*
         ----.-x--.----.----.----.----.----.---- hidden?
         ----.--xx.----.----.----.----.----.---- ?
         ----.----.xxxx.xxxx.xxxx.----.----.---- always 0xff0?
         ----.----.----.----.----.--x-.----.---- right justify
         ----.----.----.----.----.---x.----.---- bottom justify
         ----.----.----.----.----.----.x---.---- flipx
         ----.----.----.----.----.----.-xxx.---- numcols
         ----.----.----.----.----.----.----.x--- flipy
         ----.----.----.----.----.----.----.-xxx numrows
      */
		UINT32 attrs = pSource[2];
      if( (attrs&0x04000000)==0 )
		{ /* sprite is not hidden */
			INT32 zcoord = pPal[0];
         int color = pPal[1]>>16;
         int cz = pPal[1]&0xffff;
         UINT32 xypos = pSource[0];
			UINT32 size = pSource[1];
			UINT32 code = pSource[3];
			int xpos = (xypos>>16)-deltax;
			int ypos = (xypos&0xffff)-deltay;
			int sizex = size>>16;
			int sizey = size&0xffff;
			int zoomx = (1<<16)*sizex/0x20;
			int zoomy = (1<<16)*sizey/0x20;
			int flipy = attrs&0x8;
			int numrows = attrs&0x7; /* 0000 0001 1111 1111 0000 0000 fccc frrr */
			int linkType = (attrs&0x00ff0000)>>16;
			int flipx = (attrs>>4)&0x8;
			int numcols = (attrs>>4)&0x7;
			int tile = code>>16;
         int translucency = (code&0xff00)>>8;

         if( numrows==0 )
         {
            numrows = 8;
         }
			if( flipy )
			{
				ypos += sizey*(numrows-1);
				sizey = -sizey;
			}

			if( numcols==0 )
         {
            numcols = 8;
         }
			if( flipx )
			{
				xpos += sizex*(numcols-1);
				sizex = -sizex;
			}

			if( attrs & 0x0200 )
			{ /* right justify */
				xpos -= ((zoomx*numcols*0x20)>>16)-1;
         }
			if( attrs & 0x0100 )
			{ /* bottom justify */
         	ypos -= ((zoomy*numrows*0x20)>>16)-1;
         }

         {
            struct SceneNode *node = NewSceneNode(zcoord,eSCENENODE_SPRITE);
            node->data.sprite.tile = tile;
            node->data.sprite.pri = cz&0x80;
//              node->data.sprite.pri = (color&0x80);
            node->data.sprite.color = color&0x7f;
            node->data.sprite.flipx = flipx;
            node->data.sprite.flipy = flipy;
            node->data.sprite.numcols = numcols;
            node->data.sprite.numrows = numrows;
            node->data.sprite.linkType = linkType;
            node->data.sprite.xpos = xpos;
            node->data.sprite.ypos = ypos;
            node->data.sprite.sizex = sizex;
            node->data.sprite.sizey = sizey;
            node->data.sprite.translucency = translucency;
            node->data.sprite.cz = cz;
         }
  		} /* visible sprite */
		pSource += 4;
		pPal += 2;
   }
} /* DrawSpritesHelper */

static void
DrawSprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
/*
// time crisis:
00980000: 00060000 000b0053 03000200 03000000
00980010: 00200020 028004ff 032a0509 00000000
00980200: 000007ff 000007ff 000007ff 032a0509
00980210: 000007ff 000007ff 000007ff 000007ff
00980220: 000007ff 000007ff 000007ff 000007ff
00980230: 000007ff 000007ff 05000500 050a050a

// prop normal
00980000: 00060000 00040053 03000200 03000000
00980010: 00200020 028004ff 032a0509 00000000
00980200: 028004ff 032a0509 028004ff 032a0509
00980210: 028004ff 032a0509 028004ff 032a0509
00980220: 028004ff 032a0509 028004ff 032a0509
00980230: 028004ff 032a0509 028004ff 032a0509

//alpine normal / prop test (-48,-43)
00980000: 00060000 00000000 02ff0000 000007ff
00980010: 00200020 000002ff 000007ff 00000000
00980200: 000007ff 000007ff 000007ff 000007ff
00980210: 000007ff 000007ff 000007ff 000007ff
00980220: 000007ff 000007ff 000007ff 000007ff
00980230: 000007ff 000007ff 000007ff 000007ff


        0x980000:   00060000 00010000 02ff0000 000007ff
                    ^^^^                                7 = disable
                             ^^^^                       num sprites

        0x980010:   00200020 000002ff 000007ff 00000000
                    ^^^^^^^^                            character size?
                             ^^^^                       delta xpos?
                                      ^^^^              delta ypos?

        0x980200:   000007ff 000007ff       delta xpos, delta ypos?
        0x980208:   000007ff 000007ff
        0x980210:   000007ff 000007ff
        0x980218:   000007ff 000007ff
        0x980220:   000007ff 000007ff
        0x980228:   000007ff 000007ff
        0x980230:   000007ff 000007ff
        0x980238:   000007ff 000007ff

        //time crisis
        00980200:  000007ff 000007ff 000007ff 032a0509
        00980210:  000007ff 000007ff 000007ff 000007ff
        00980220:  000007ff 000007ff 000007ff 000007ff
        00980230:  000007ff 000007ff 05000500 050a050a

        0x980400:   hzoom table
        0x980600:   vzoom table

        link table:
        0x980800:   0000 0001 0002 0003 ... 03ff

        eight words per sprite:
        0x984000:   010f 007b   xpos, ypos
        0x984004:   0020 0020   size x, size y
        0x984008:   00ff 0311   00ff, chr x;chr y;flip x;flip y
        0x98400c:   0001 0000   sprite code, translucency
        ...

        additional sorting/color data for sprite:
        0x9a0000:   C381 Z (sort)
        0x9a0004:   palette, C381 ZC (depth cueing)
        ...
    */
	int num_sprites = ((spriteram32[0x04/4]>>16)&0x3ff)+1;
	const UINT32 *pSource = &spriteram32[0x4000/4];
	const UINT32 *pPal = &spriteram32[0x20000/4];
   int deltax = spriteram32[0x14/4]>>16;
	int deltay = spriteram32[0x18/4]>>16;
   int enable = spriteram32[0]>>16;

   if( spriteram32[0x14/4] == 0x000002ff &&
       spriteram32[0x18/4] == 0x000007ff )
   { /* HACK (fixes alpine racer and self test) */
      deltax = 48;
		deltay = 43;
   }

   if( enable==6 /*&& namcos22_gametype!=NAMCOS22_AIR_COMBAT22*/ )
   {
      DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay );
   }

	/* VICS RAM provides two additional banks */
	/*
        0x940000 -x------       sprite chip busy
        0x940018 xxxx----       clr.w   $940018.l

        0x940034 xxxxxxxx       0x3070b0f

        0x940040 xxxxxxxx       sprite attribute size
        0x940048 xxxxxxxx       sprite attribute list baseaddr
        0x940050 xxxxxxxx       sprite color size
        0x940058 xxxxxxxx       sprite color list baseaddr

        0x940060..0x94007c      set#2
*/

	num_sprites = (namcos22_vics_control[0x40/4]&0xffff)/0x10;
	if( num_sprites>=1 )
	{
		pSource = &namcos22_vics_data[(namcos22_vics_control[0x48/4]&0xffff)/4];
		pPal    = &namcos22_vics_data[(namcos22_vics_control[0x58/4]&0xffff)/4];
		DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay );
	}

	num_sprites = (namcos22_vics_control[0x60/4]&0xffff)/0x10;
	if( num_sprites>=1 )
	{
		pSource = &namcos22_vics_data[(namcos22_vics_control[0x68/4]&0xffff)/4];
		pPal    = &namcos22_vics_data[(namcos22_vics_control[0x78/4]&0xffff)/4];
		DrawSpritesHelper( bitmap, cliprect, pSource, pPal, num_sprites, deltax, deltay );
	}
} /* DrawSprites */

static void
UpdatePaletteS( void ) /* for Super System22 - apply gamma correction and preliminary fader support */
{
	int i;
	for( i=0; i<NAMCOS22_PALETTE_SIZE/4; i++ )
	{
		if( dirtypal[i] )
		{
         int j;
			for( j=0; j<4; j++ )
			{
				int which = i*4+j;
				int r = nthbyte(paletteram32,which+0x00000);
				int g = nthbyte(paletteram32,which+0x08000);
				int b = nthbyte(paletteram32,which+0x10000);
				palette_set_color( which,r,g,b );
			}
			dirtypal[i] = 0;
		}
	}
} /* UpdatePaletteS */

static void
UpdatePalette( void ) /* for System22 - ignore gamma/fader effects for now */
{
	int i,j;
	for( i=0; i<NAMCOS22_PALETTE_SIZE/4; i++ )
	{
		if( dirtypal[i] )
		{
			for( j=0; j<4; j++ )
			{
				int which = i*4+j;
				int r = nthbyte(paletteram32,which+0x00000);
				int g = nthbyte(paletteram32,which+0x08000);
				int b = nthbyte(paletteram32,which+0x10000);
				palette_set_color( which,r,g,b );
			}
			dirtypal[i] = 0;
		}
	}
} /* UpdatePalette */

static tilemap *bgtilemap;

static void TextTilemapGetInfo( int tile_index )
{
	UINT16 data = nthword( namcos22_textram,tile_index );
	/**
     * x---.----.----.---- blend
    * xxxx.----.----.---- palette select
    * ----.xx--.----.---- flip
    * ----.--xx.xxxx.xxxx code
    */
	SET_TILE_INFO( GFX_CHAR,data&0x03ff,data>>12,TILE_FLIPYX((data&0x0c00)>>10) );
	if( data&0x8000 )
	{
		tile_info.priority = 1;
	}
} /* TextTilemapGetInfo */

READ32_HANDLER( namcos22_textram_r )
{
	return namcos22_textram[offset];
}

WRITE32_HANDLER( namcos22_textram_w )
{
	COMBINE_DATA( &namcos22_textram[offset] );
	tilemap_mark_tile_dirty( bgtilemap, offset*2 );
	tilemap_mark_tile_dirty( bgtilemap, offset*2+1 );
}

static void
DrawTranslucentCharacters( mame_bitmap *bitmap, const rectangle *cliprect )
{
	alpha_set_level( 0xff-mixer.text_translucency ); /* ? */
	tilemap_draw( bitmap, cliprect, bgtilemap, TILEMAP_ALPHA|1, 0 );
}

static void
DrawCharacterLayer( mame_bitmap *bitmap, const rectangle *cliprect )
{
	unsigned i;
   INT32 dx = namcos22_tilemapattr[0]>>16;
   INT32 dy = namcos22_tilemapattr[0]&0xffff;
   /**
     * namcos22_tilemapattr[0x4/4] == 0x006e0000
    * namcos22_tilemapattr[0x8/4] == 0x01ff0000
     * namcos22_tilemapattr[0xe/4] == ?
    */
   if( cgsomethingisdirty )
	{
		for( i=0; i<64*64; i+=2 )
		{
			UINT32 data = namcos22_textram[i/2];
			if( cgdirty[(data>>16)&0x3ff] )
			{
				tilemap_mark_tile_dirty( bgtilemap,i );
			}
			if( cgdirty[data&0x3ff] )
			{
				tilemap_mark_tile_dirty( bgtilemap,i+1 );
			}
		}

		for( i=0; i<NUM_CG_CHARS; i++ )
		{
			if( cgdirty[i] )
			{
				decodechar( Machine->gfx[GFX_CHAR],i,(UINT8 *)namcos22_cgram,&namcos22_cg_layout );
				cgdirty[i] = 0;
			}
		}
		cgsomethingisdirty = 0;
	}

	fillbitmap(priority_bitmap,0,cliprect);
	tilemap_set_scrollx( bgtilemap,0, (dx-0x35c)&0x3ff );
	tilemap_set_scrolly( bgtilemap,0, dy&0x3ff );
	tilemap_set_palette_offset( bgtilemap, mixer.palBase*256 );
	tilemap_draw( bitmap, cliprect, bgtilemap, 0/*flags*/, 0x1/*priority*/ ); /* opaque */
} /* DrawCharacterLayer */

/*********************************************************************************************/

static int
Cap( int val, int minval, int maxval )
{
   if( val<minval )
   {
      val = minval;
   }
   else if( val>maxval )
   {
      val = maxval;
   }
   return val;
}

#define LSB21 (0x1fffff)
#define LSB18 (0x03ffff)

static INT32
Signed18( UINT32 value )
{
   INT32 offset = value&LSB18;
   if( offset&0x20000 )
   { /* sign extend */
		offset |= ~LSB18;
   }
   return offset;
}

/**
 * @brief render a single quad
 *
 * @param flags
 *     x1.----.----.---- priority over tilemap
 *     --.-xxx.----.---- representative z algorithm?
 *     --.----.-1x-.---- backface cull enable
 *     --.----.----.--1x fog enable?
 *
 *      1163 // sky
 *      1262 // score (front)
 *      1242 // score (hinge)
 *      1243 // ?
 *      1063 // n/a
 *      1243 // various (2-sided?)
 *      1263 // everything else (1-sided?)
 *      1663 // ?
 *
 * @param color
 *      -------- xxxxxxxx unused?
 *      -xxxxxxx -------- palette select
 *      x------- -------- ?
 *
 * @param polygonShiftValue22
 *    0x1fbd0 - sky+sea
 *    0x0c350 - mountins
 *    0x09c40 - boats, surf, road, buildings
 *    0x07350 - guardrail
 *    0x061a8 - red car
 */
static void
BlitQuadHelper(
		mame_bitmap *pBitmap,
		unsigned color,
		unsigned addr,
		float m[4][4],
		INT32 polygonShiftValue22, /* 22 bits */
		int flags,
		int packetFormat )
{
   int absolutePriority = mAbsolutePriority;
   UINT32 zsortvalue24;
   float zmin = 0.0f;
   float zmax = 0.0f;
	Poly3dVertex v[4];
	int i;
   int bBackFace = 0;

	for( i=0; i<4; i++ )
	{
		Poly3dVertex *pVerTex = &v[i];
		pVerTex->x = GetPolyData(  8+i*3+addr );
		pVerTex->y = GetPolyData(  9+i*3+addr );
		pVerTex->z = GetPolyData( 10+i*3+addr );
		TransformPoint( &pVerTex->x, &pVerTex->y, &pVerTex->z, m );
	} /* for( i=0; i<4; i++ ) */

   if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
       (v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
		 (v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 &&

       (v[0].x*((v[2].z*v[3].y)-(v[2].y*v[3].z)))+
		 (v[0].y*((v[2].x*v[3].z)-(v[2].z*v[3].x)))+
		 (v[0].z*((v[2].y*v[3].x)-(v[2].x*v[3].y))) >= 0 )
   {
      bBackFace = 1;
   }

   if( bBackFace && (flags&0x0020) )
	{ /* backface cull one-sided polygons */
		return;
	}

   for( i=0; i<4; i++ )
	{
		Poly3dVertex *pVerTex = &v[i];
      int bri;

		pVerTex->u = GetPolyData(  0+i*2+addr );
		pVerTex->v = GetPolyData(  1+i*2+addr );

		if( i==0 || pVerTex->z > zmax ) zmax = pVerTex->z;
		if( i==0 || pVerTex->z < zmin ) zmin = pVerTex->z;

      if( mLitSurfaceCount )
      {
      	bri = mLitSurfaceInfo[mLitSurfaceIndex%mLitSurfaceCount];
         if( mSurfaceNormalFormat == 0x6666 )
         {
            if( i==3 )
            {
               mLitSurfaceIndex++;
            }
         }
         else if( mSurfaceNormalFormat == 0x4000 )
         {
            mLitSurfaceIndex++;
         }
         else
         {
            logerror( "unknown normal format: 0x%x\n", mSurfaceNormalFormat );
         }
      } /* pLitSurfaceInfo */
      else if( packetFormat & 0x40 )
      {
         bri = (GetPolyData(i+addr)>>16)&0xff;
      }
		else
		{
			bri = 0x40;
		}
      pVerTex->bri = bri;
	} /* for( i=0; i<4; i++ ) */

   if( zmin<0.0f ) zmin = 0.0f;
   if( zmax<0.0f ) zmax = 0.0f;

   switch( (flags&0x0f00)>>8 )
   {
   case 0:
       zsortvalue24 = (INT32)zmin;
       break;

   case 1:
       zsortvalue24 = (INT32)zmax;
       break;

   case 2:
   default:
       zsortvalue24 = (INT32)((zmin+zmax)/2.0f);
       break;
   }

   /* relative: representative z + shift values
    * 1x.xxxx.xxxxxxxx.xxxxxxxx fixed z value
    * 0x.xx--.--------.-------- absolute priority shift
    * 0-.--xx.xxxxxxxx.xxxxxxxx z-representative value shift
    */
   if( polygonShiftValue22 & 0x200000 )
   {
      zsortvalue24 = polygonShiftValue22 & LSB21;
   }
   else
   {
      zsortvalue24 += Signed18( polygonShiftValue22 );
      absolutePriority += (polygonShiftValue22&0x1c0000)>>18;
   }
   if( mObjectShiftValue22 & 0x200000 )
   {
      zsortvalue24 = mObjectShiftValue22 & LSB21;
   }
   else
   {
      zsortvalue24 += Signed18( mObjectShiftValue22 );
      absolutePriority += (mObjectShiftValue22&0x1c0000)>>18;
   }
   absolutePriority &= 7;
   zsortvalue24 = Cap(zsortvalue24,0,0x1fffff);
   zsortvalue24 |= (absolutePriority<<21);

   {
      struct SceneNode *node = NewSceneNode(zsortvalue24,eSCENENODE_QUAD3D);
		node->data.quad3d.cmode = (v[0].u>>12)&0xf;
      node->data.quad3d.textureBank = (v[0].v>>12)&0xf;
      node->data.quad3d.color = (color>>8)&0xff;

      {
         INT32 cz = (INT32)((zmin+zmax)/2.0f);
         cz = Clamp256(cz/0x2000);
         node->data.quad3d.flags = (cz<<8)|(flags&3);
      }

      for( i=0; i<4; i++ )
      {
         Poly3dVertex *p = &node->data.quad3d.v[i];
         p->x     = v[i].x*mCamera.zoom;
         p->y     = v[i].y*mCamera.zoom;
         p->z     = v[i].z;
         p->u     = v[i].u&0xfff;
         p->v     = v[i].v&0xfff;
         p->bri   = v[i].bri;
      }
      node->data.quad3d.bDirect = 0;
      node->data.quad3d.vx = mCamera.vx;
      node->data.quad3d.vy = mCamera.vy;
      node->data.quad3d.vw = mCamera.vw;
      node->data.quad3d.vh = mCamera.vh;
   }
} /* BlitQuadHelper */

static void
RegisterNormals( INT32 addr, float m[4][4] )
{
   int i;
   for( i=0; i<4; i++ )
   {
         float nx = DSP_FIXED_TO_FLOAT(GetPolyData(addr+i*3+0));
		   float ny = DSP_FIXED_TO_FLOAT(GetPolyData(addr+i*3+1));
		   float nz = DSP_FIXED_TO_FLOAT(GetPolyData(addr+i*3+2));
			float dotproduct;

			/* transform normal vector */
			TransformNormal( &nx, &ny, &nz, m );
			dotproduct = nx*mCamera.lx + ny*mCamera.ly + nz*mCamera.lz;
			if( dotproduct<0.0f )
         {
            dotproduct = 0.0f;
         }
         mLitSurfaceInfo[mLitSurfaceCount++] = mCamera.ambient + mCamera.power*dotproduct;
   }
} /* RegisterNormals */

static void
BlitQuads( mame_bitmap *pBitmap, INT32 addr, float m[4][4], INT32 base )
{
   int numAdditionalNormals = 0;
	int chunkLength = GetPolyData(addr++);
	int finish = addr + chunkLength;
	if( chunkLength>0x100 )
	{
		fatalerror( "bad packet length" );
	}

	while( addr<finish )
	{
		int packetLength = GetPolyData( addr++ );
		int packetFormat = GetPolyData( addr+0 );
		int flags, color, bias;

		/**
         * packetFormat:
         *      800000 final packet in chunk
         *      080000 ?
         *      020000 color word exists?
         *      010000 z-offset word exists?
         *      002000 ?
         *      001000 z-offset word exists?
         *      000400 ?
         *      000080 tex# or UV or CMODE?
         *      000040 use I
         *      000001 ?
         *
         * flags:
         *      1042 (always set)
         *      0c00 depth-cueing mode (brake-disc(2108)=001e43, model font)
         *      0200 usually 1
         *      0100 ?
         *      0040 1 ... polygon palette?
         *      0020 cull backface
         *      0002 ?
         *      0001 ?
         *
         * color:
         *      ff0000 type?
         *      008000 depth-cueing off
         *      007f00 palette#
         */
		switch( packetLength )
		{
		case 0x17:
			/**
             * word 0: opcode (8a24c0)
             * word 1: flags
             * word 2: color
             */
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias = 0;
			BlitQuadHelper( pBitmap,color,addr+3,m,bias,flags,packetFormat );
			break;

		case 0x18:
			/**
          * word 0: opcode (0b3480 for first N-1 quads or 8b3480 for final quad in primitive)
          * word 1: flags
          * word 2: color
          * word 3: depth bias
          */
			flags = GetPolyData(addr+1);
			color = GetPolyData(addr+2);
			bias  = GetPolyData(addr+3);
			BlitQuadHelper( pBitmap,color,addr+4,m,bias,flags,packetFormat );
			break;

		case 0x10: /* vertex lighting */
			/*
            333401 (opcode)
                000000  [count] [type]
                000000  000000  007fff // normal vector
                000000  000000  007fff // normal vector
                000000  000000  007fff // normal vector
                000000  000000  007fff // normal vector
            */
         numAdditionalNormals = GetPolyData(addr+2);
         mSurfaceNormalFormat = GetPolyData(addr+3);
         mLitSurfaceCount = 0;
         mLitSurfaceIndex = 0;
         RegisterNormals( addr+4, m );
			break;

		case 0x0d: /* additional normals */
			/*
            300401 (opcode)
                007b09 ffdd04 0004c2
                007a08 ffd968 0001c1
                ff8354 ffe401 000790
                ff84f7 ffdd04 0004c2
            */
         RegisterNormals( addr+1, m );
         break;

		default:
			break;
		}
		addr += packetLength;
	}
} /* BlitQuads */

static void
BlitPolyObject( mame_bitmap *pBitmap, int code, float M[4][4] )
{
	unsigned addr1 = GetPolyData(code);
   mLitSurfaceCount = 0;
   mLitSurfaceIndex = 0;
	for(;;)
	{
		INT32 addr2 = GetPolyData(addr1++);
		if( addr2<0 )
		{
			break;
		}
		BlitQuads( pBitmap, addr2, M, code );
	}
} /* BlitPolyObject */

/*******************************************************************************/

READ32_HANDLER( namcos22_dspram_r )
{
	return namcos22_polygonram[offset];
}

WRITE32_HANDLER( namcos22_dspram_w )
{
	COMBINE_DATA( &namcos22_polygonram[offset] );
}

/*******************************************************************************/

/**
 * master DSP can write directly to render device via port 0xc.
 * This is used for "direct drawn" polygons, and "direct draw from point rom"
 * feature - both opcodes exist in Ridge Racer's display-list processing
 *
 * record format:
 *  header (3 words)
 *      polygonShiftValue22
 *      color
 *      flags
 *
 *  per-vertex data (4*6 words)
 *      u,v
 *      sx,sy
 *      intensity;z.exponent
 *      z.mantissa
 *
 * master DSP can specify 3d objects indirectly (along with view transforms),
 * via the "transmit" PDP opcode.  the "render device" sends quad data to the slave DSP
 * viewspace clipping and projection
 *
 * most "3d object" references are 0x45 and greater.  references less than 0x45 are "special"
 * commands, using a similar point rom format.  the point rom header may point to point ram.
 *
 * slave DSP reads records via port 4
 * its primary purpose is applying lighting calculations
 * the slave DSP forwards draw commands to a "draw device"
 */

/*******************************************************************************/

/**
 * 0xfffd
 * 0x0: transform
 * 0x1
 * 0x2
 * 0x5: transform
 * >=0x45: draw primitive
 */
static void
HandleBB0003( const INT32 *pSource )
{
   /*
        bb0003 or 3b0003

        14.00c8            light.ambient     light.power
        01.0000            ?                 light.dx
        06.5a82            window priority   light.dy
        00.a57e            ?                 light.dz

        c8.0081            vx=200,vy=129
        29.6092            zoom = 772.5625
        1e.95f8 1e.95f8            0.5858154296875   0.5858154296875 // 452
        1e.b079 1e.b079            0.6893463134765   0.6893463134765 // 532
        29.58e8                   711.25 (border? see time crisis)

        7ffe 0000 0000
        0000 7ffe 0000
        0000 0000 7ffe
    */
	mCamera.ambient = pSource[0x1]>>16;
	mCamera.power   = pSource[0x1]&0xffff;

	mCamera.lx       = DSP_FIXED_TO_FLOAT(pSource[0x2]);
	mCamera.ly       = DSP_FIXED_TO_FLOAT(pSource[0x3]);
	mCamera.lz       = DSP_FIXED_TO_FLOAT(pSource[0x4]);

   mAbsolutePriority = pSource[0x3]>>16;
   mCamera.vx      = (INT16)(pSource[5]>>16);
   mCamera.vy      = (INT16)pSource[5];
   mCamera.zoom    = DspFloatToNativeFloat(pSource[6]);
   mCamera.vw      = DspFloatToNativeFloat(pSource[7])*mCamera.zoom;
   mCamera.vh      = DspFloatToNativeFloat(pSource[9])*mCamera.zoom;

	mViewMatrix[0][0] = DSP_FIXED_TO_FLOAT(pSource[0x0c]);
	mViewMatrix[1][0] = DSP_FIXED_TO_FLOAT(pSource[0x0d]);
	mViewMatrix[2][0] = DSP_FIXED_TO_FLOAT(pSource[0x0e]);

	mViewMatrix[0][1] = DSP_FIXED_TO_FLOAT(pSource[0x0f]);
	mViewMatrix[1][1] = DSP_FIXED_TO_FLOAT(pSource[0x10]);
	mViewMatrix[2][1] = DSP_FIXED_TO_FLOAT(pSource[0x11]);

	mViewMatrix[0][2] = DSP_FIXED_TO_FLOAT(pSource[0x12]);
	mViewMatrix[1][2] = DSP_FIXED_TO_FLOAT(pSource[0x13]);
	mViewMatrix[2][2] = DSP_FIXED_TO_FLOAT(pSource[0x14]);

	TransformNormal( &mCamera.lx, &mCamera.ly, &mCamera.lz, mViewMatrix );
} /* HandleBB0003 */

static void
Handle200002( mame_bitmap *bitmap, const INT32 *pSource )
{
	if( mPrimitiveID>=0x45 )
	{
		float m[4][4]; /* row major */

		matrix3d_Identity( m );

		m[0][0] = DSP_FIXED_TO_FLOAT(pSource[0x1]);
		m[1][0] = DSP_FIXED_TO_FLOAT(pSource[0x2]);
		m[2][0] = DSP_FIXED_TO_FLOAT(pSource[0x3]);

		m[0][1] = DSP_FIXED_TO_FLOAT(pSource[0x4]);
		m[1][1] = DSP_FIXED_TO_FLOAT(pSource[0x5]);
		m[2][1] = DSP_FIXED_TO_FLOAT(pSource[0x6]);

		m[0][2] = DSP_FIXED_TO_FLOAT(pSource[0x7]);
		m[1][2] = DSP_FIXED_TO_FLOAT(pSource[0x8]);
		m[2][2] = DSP_FIXED_TO_FLOAT(pSource[0x9]);

		m[3][0] = pSource[0xa]; /* xpos */
		m[3][1] = pSource[0xb]; /* ypos */
		m[3][2] = pSource[0xc]; /* zpos */

      matrix3d_Multiply( m, mViewMatrix );
		BlitPolyObject( bitmap, mPrimitiveID, m );
	}
   else if( mPrimitiveID !=0 && mPrimitiveID !=2 )
   {
      logerror( "Handle200002:unk code=0x%x\n", mPrimitiveID );
   }
} /* Handle200002 */

static void
Handle300000( const INT32 *pSource )
{ /* set view transform */
	mViewMatrix[0][0] = DSP_FIXED_TO_FLOAT(pSource[1]);
	mViewMatrix[1][0] = DSP_FIXED_TO_FLOAT(pSource[2]);
	mViewMatrix[2][0] = DSP_FIXED_TO_FLOAT(pSource[3]);

	mViewMatrix[0][1] = DSP_FIXED_TO_FLOAT(pSource[4]);
	mViewMatrix[1][1] = DSP_FIXED_TO_FLOAT(pSource[5]);
	mViewMatrix[2][1] = DSP_FIXED_TO_FLOAT(pSource[6]);

	mViewMatrix[0][2] = DSP_FIXED_TO_FLOAT(pSource[7]);
	mViewMatrix[1][2] = DSP_FIXED_TO_FLOAT(pSource[8]);
	mViewMatrix[2][2] = DSP_FIXED_TO_FLOAT(pSource[9]);
} /* Handle300000 */

static void
Handle233002( const INT32 *pSource )
{ /* set modal rendering options */
   /*
    00233002
       00000000 // zc adjust?
       0003dd00 // z bias adjust
       001fffff // far plane?
       00007fff 00000000 00000000
       00000000 00007fff 00000000
       00000000 00000000 00007fff
       00000000 00000000 00000000
   */
   mObjectShiftValue22 = pSource[2];
} /* Handle233002 */

static void
SimulateSlaveDSP( mame_bitmap *bitmap )
{
	const INT32 *pSource = 0x300 + (INT32 *)namcos22_polygonram;
	INT16 len;

	matrix3d_Identity( mViewMatrix );

	if( mbSuperSystem22 )
	{
		pSource += 4; /* FFFE 0400 */
	}
	else
	{
		pSource--;
	}

	for(;;)
	{
		INT16 marker, next;
		mPrimitiveID = *pSource++;
		len  = (INT16)*pSource++;

		switch( len )
		{
		case 0x15:
			HandleBB0003( pSource ); /* define viewport */
			break;

		case 0x10:
			Handle233002( pSource ); /* set modal rendering options */
			break;

		case 0x0a:
			Handle300000( pSource ); /* modify view transform */
			break;

		case 0x0d:
			Handle200002( bitmap, pSource ); /* render primitive */
			break;

		default:
         logerror( "unk 3d data(%d) addr=0x%x!", len, pSource-(INT32*)namcos22_polygonram );
         {
            int i;
            for( i=0; i<len; i++ )
            {
               logerror( " %06x", pSource[i]&0xffffff );
            }
            logerror( "\n" );
         }
			return;
		}

		/* hackery! commands should be streamed, not parsed here */
		pSource += len;
		marker = (INT16)*pSource++; /* always 0xffff */
		next   = (INT16)*pSource++; /* link to next command */
		if( (next&0x7fff) != (pSource - (INT32 *)namcos22_polygonram) )
		{ /* end of list */
			break;
		}
	} /* for(;;) */
} /* SimulateSlaveDSP */

static void
DrawPolygons( mame_bitmap *bitmap )
{
	if( mbDSPisActive )
	{
		SimulateSlaveDSP( bitmap );
	}
} /* DrawPolygons */

void
namcos22_enable_slave_simulation( void )
{
	mbDSPisActive = 1;
}

/*********************************************************************************************/

READ32_HANDLER( namcos22_cgram_r )
{
	return namcos22_cgram[offset];
}

WRITE32_HANDLER( namcos22_cgram_w )
{
	COMBINE_DATA( &namcos22_cgram[offset] );
	cgdirty[offset/32] = 1;
	cgsomethingisdirty = 1;
}

READ32_HANDLER( namcos22_gamma_r )
{
	return namcos22_gamma[offset];
}

WRITE32_HANDLER( namcos22_gamma_w )
{
	COMBINE_DATA( &namcos22_gamma[offset] );
}

READ32_HANDLER( namcos22_paletteram_r )
{
	return paletteram32[offset];
}

WRITE32_HANDLER( namcos22_paletteram_w )
{
	COMBINE_DATA( &paletteram32[offset] );
	dirtypal[offset&(0x7fff/4)] = 1;
}

static int
video_start_common( void )
{
	bgtilemap = tilemap_create( TextTilemapGetInfo,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64 );
	if( bgtilemap )
	{
		tilemap_set_transparent_pen( bgtilemap, 0xf );
	}

	mbDSPisActive = 0;
	memset( namcos22_polygonram, 0xcc, 0x20000 );

	if( Prepare3dTexture(
		memory_region(REGION_TEXTURE_TILEMAP),
      Machine->gfx[GFX_TEXTURE_TILE]->gfxdata ) == 0 )
	{
		dirtypal = auto_malloc(NAMCOS22_PALETTE_SIZE/4);
		{
			cgdirty = auto_malloc( 0x400 );
			{
				mPtRomSize = memory_region_length(REGION_POINTROM)/3;
				mpPolyL = memory_region(REGION_POINTROM);
				mpPolyM = mpPolyL + mPtRomSize;
				mpPolyH = mpPolyM + mPtRomSize;
				return 0; /* no error */
			}
		}
	}
	return -1; /* error */
}

VIDEO_START( namcos22 )
{
   int result;
   mbSuperSystem22 = 0;
   result = video_start_common();
   return result;
}

VIDEO_START( namcos22s )
{
   int result;
   mbSuperSystem22 = 1;
   namcos22_czram[0] = auto_malloc( 0x200 );
   namcos22_czram[1] = auto_malloc( 0x200 );
   namcos22_czram[2] = auto_malloc( 0x200 );
   namcos22_czram[3] = auto_malloc( 0x200 );
   result = video_start_common();
   return result;
}

VIDEO_UPDATE( namcos22s )
{
	int beamx,beamy;
	UINT32 bgColor;
	UpdateVideoMixer();
	bgColor = (mixer.rBackColor<<16)|(mixer.gBackColor<<8)|mixer.bBackColor;
   fillbitmap( bitmap, bgColor, cliprect );
   UpdatePaletteS();
   DrawCharacterLayer( bitmap, cliprect );
	DrawPolygons( bitmap );
   DrawSprites( bitmap, cliprect );
   RenderScene( bitmap );
	DrawTranslucentCharacters( bitmap, cliprect );
	ApplyGamma( bitmap );

	if( namcos22_gametype == NAMCOS22_TIME_CRISIS )
	{
		beamx = ((readinputport(1))*640)/256;
		beamy = ((readinputport(2))*480)/256;
		draw_crosshair( bitmap, beamx, beamy, cliprect, 0 );
	}

#ifdef MAME_DEBUG
   if( code_pressed(KEYCODE_D) )
   {
      FILE *f = fopen( "dump.txt", "wb" );
      if( f )
      {
         {
            int i,bank;
            for( bank=0; bank<4; bank++ )
            {
               fprintf( f, "czram[%d] =", bank );
               for( i=0; i<256; i++ )
               {
                  fprintf( f, " %04x", namcos22_czram[bank][i] );
               }
               fprintf( f, "\n" );
            }
         }

	      Dump(f,0x810000, 0x81000f, "cz attr" );
			Dump(f,0x820000, 0x8202ff, "unk_ac" );
	      Dump(f,0x824000, 0x8243ff, "gamma");
			Dump(f,0x828000, 0x83ffff, "palette" );
         Dump(f,0x8a0000, 0x8a000f, "tilemap_attr");
         Dump(f,0x880000, 0x89ffff, "cgram/textram");
         Dump(f,0x900000, 0x90ffff, "vics_data");
         Dump(f,0x940000, 0x94007f, "vics_control");
         Dump(f,0x980000, 0x9affff, "sprite374" );
         Dump(f,0xc00000, 0xc1ffff, "polygonram");
         fclose( f );
      }
      while( code_pressed(KEYCODE_D) ){}
   }
#endif
}

VIDEO_UPDATE( namcos22 )
{
	UpdateVideoMixer();
   fillbitmap( bitmap, get_black_pen(), cliprect );
	UpdatePalette();
	DrawCharacterLayer( bitmap, cliprect );
   DrawPolygons( bitmap );
   RenderScene(bitmap);
	DrawTranslucentCharacters( bitmap, cliprect );
	ApplyGamma( bitmap );

#ifdef MAME_DEBUG
   if( code_pressed(KEYCODE_D) )
   {
      FILE *f = fopen( "dump.txt", "wb" );
      if( f )
      {
//        Dump(f,0x90000000, 0x90000003, "led?" );
//         Dump(f,0x90010000, 0x90017fff, "cz_ram");
//         Dump(f,0x900a0000, 0x900a000f, "tilemap_attr");
         Dump(f,0x90020000, 0x90027fff, "gamma");
			    //   0x90020000, 0x90027fff
//         Dump(f,0x70000000, 0x7001ffff, "polygonram");
         fclose( f );
      }
      while( code_pressed(KEYCODE_D) ){}
   }
#endif
}

WRITE16_HANDLER( namcos22_dspram16_bank_w )
{
	COMBINE_DATA( &namcos22_dspram_bank );
}

READ16_HANDLER( namcos22_dspram16_r )
{
	UINT32 value = namcos22_polygonram[offset];
	switch( namcos22_dspram_bank )
	{
	case 0:
		value &= 0xffff;
		break;

	case 1:
		value>>=16;
		break;

	case 2:
		mUpperWordLatch = value>>16;
		value &= 0xffff;
		break;

	default:
		break;
	}
	return (UINT16)value;
} /* namcos22_dspram16_r */

WRITE16_HANDLER( namcos22_dspram16_w )
{
	UINT32 value = namcos22_polygonram[offset];
	UINT16 lo = value&0xffff;
	UINT16 hi = value>>16;
	switch( namcos22_dspram_bank )
	{
	case 0:
		COMBINE_DATA( &lo );
		break;

	case 1:
		COMBINE_DATA( &hi );
		break;

	case 2:
		COMBINE_DATA( &lo );
		hi = mUpperWordLatch;
		break;

	default:
		break;
	}
	namcos22_polygonram[offset] = (hi<<16)|lo;
} /* namcos22_dspram16_w */

static void
Dump( FILE *f, unsigned addr1, unsigned addr2, const char *name )
{
   unsigned addr;
   fprintf( f, "%s:\n", name );
   for( addr=addr1; addr<=addr2; addr+=16 )
   {
      unsigned char data[16];
      int bHasNonZero = 0;
      int i;
      for( i=0; i<16; i++ )
      {
         data[i] = cpunum_read_byte( 0, addr+i );
         if( data[i] )
         {
            bHasNonZero = 1;
         }
      }
      if( bHasNonZero )
      {
         fprintf( f,"%08x:", addr );
         for( i=0; i<16; i++ )
         {
            if( (i&0x03)==0 )
            {
               fprintf( f, " " );
            }
            fprintf( f, "%02x", data[i] );
         }
         fprintf( f, "\n" );
      }
   }
   fprintf( f, "\n" );
}

/**
 * 4038 spot enable?
 * 0828 pre-initialization
 * 0838 post-initialization
 **********************************************
 * upload:
 *   #bits data
 *    0010 FEC0
 *    0010 FF10
 *    0004 0004
 *    0004 000E
 *    0003 0007
 *    0002 0002
 *    0002 0003
 *    0001 0001
 *    0001 0001
 *    0001 0000
 *    0001 0001
 *    0001 0001
 *    0001 0000
 *    0001 0000
 *    0001 0000
 *    0001 0001
 **********************************************
 *    0008 00EA // 0x0ff
 *    000A 0364 // 0x3ff
 *    000A 027F // 0x3ff
 *    0003 0005 // 0x007
 *    0001 0001 // 0x001
 *    0001 0001 // 0x001
 *    0001 0001 // 0x001
 **********************************************
 */
WRITE32_HANDLER(namcos22_port800000_w)
{
   /* 00000011011111110000100011111111001001111110111110110001 */
   UINT16 word = data>>16;
   logerror( "%x: C304/C399: 0x%04x\n", activecpu_get_previouspc(), word );
   if( word == 0x4038 )
   {
      mbSpotlightEnable = 1;
   }
   else
   {
      mbSpotlightEnable = 0;
   }
} /* namcos22_port800000_w */
