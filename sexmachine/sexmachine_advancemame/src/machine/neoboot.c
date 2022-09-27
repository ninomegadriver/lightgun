/***************************************************************************

    NeoGeo *bootleg* Encryption (& Some Protection)

    Many of the NeoGeo bootlegs use their own form of encryption and
    protection, presumably to make them harder for other bootleggser to
    copy.  This encryption often involves non-trivial scrambling of the
    program roms and the games are protected using an Altera chip which
    provides some kind of rom overlay, patching parts of the code.
    The graphics roms are usually scrambled in a different way to the
    official SNK cartridges too.

***************************************************************************/

#include "driver.h"
#include "neogeo.h"

/* General Bootleg Functions - used by more than 1 game */

void neogeo_bootleg_cx_decrypt( void )
{
	int i;
	int cx_size = memory_region_length( REGION_GFX3 );
	UINT8 *rom = memory_region( REGION_GFX3 );
	UINT8 *buf = malloc( cx_size );

	memcpy( buf, rom, cx_size );

	for( i = 0; i < cx_size / 0x40; i++ ){
		memcpy( &rom[ i * 0x40 ], &buf[ (i ^ 1) * 0x40 ], 0x40 );
	}

	free( buf );
}

void neogeo_bootleg_sx_decrypt( int value )
{
	int sx_size = memory_region_length( REGION_GFX1 );
	UINT8 *rom = memory_region( REGION_GFX1 );
	int i;

	if (value == 1)
	{
		UINT8 *buf = malloc( sx_size );
		memcpy( buf, rom, sx_size );

		for( i = 0; i < sx_size; i += 0x10 )
		{
			memcpy( &rom[ i ], &buf[ i + 8 ], 8 );
			memcpy( &rom[ i + 8 ], &buf[ i ], 8 );
		}
		free( buf );
	}
	else if (value == 2)
	{
		for( i = 0; i < sx_size; i++ )
			rom[ i ] = BITSWAP8( rom[ i ], 7, 6, 0, 4, 3, 2, 1, 5 );
	}
}


/* The King of Gladiator (The King of Fighters '97 bootleg) */

/* the protection patching here may be incomplete */

// Thanks to Razoola for the info

void kog_px_decrypt( void )
{
	/* the protection chip does some *very* strange things to the rom */
	UINT8 *src = memory_region(REGION_CPU1);
	UINT8 *dst = malloc( 0x600000 );
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	int i;
	static const unsigned int sec[] = { 0x3, 0x8, 0x7, 0xC, 0x1, 0xA, 0x6, 0xD };

	for (i = 0; i < 8; i++){
		memcpy (dst + i * 0x20000, src + sec[i] * 0x20000, 0x20000);
	}

	memcpy (dst + 0x0007A6, src + 0x0407A6, 0x000006);
	memcpy (dst + 0x0007C6, src + 0x0407C6, 0x000006);
	memcpy (dst + 0x0007E6, src + 0x0407E6, 0x000006);
	memcpy (dst + 0x090000, src + 0x040000, 0x004000);
	memcpy (dst + 0x100000, src + 0x200000, 0x400000);
	memcpy (src, dst, 0x600000);
	free (dst);

	for (i = 0x90000/2; i < 0x94000/2; i++){
		if (((rom[i]&0xFFBF) == 0x4EB9 || rom[i] == 0x43F9) && !rom[i + 1])
			rom[i + 1] = 0x0009;

		if (rom[i] == 0x4EB8)
			rom[i] = 0x6100;
	}

	rom[0x007A8/2] = 0x0009;
	rom[0x007C8/2] = 0x0009;
	rom[0x007E8/2] = 0x0009;
	rom[0x93408/2] = 0xF168;
	rom[0x9340C/2] = 0xFB7A;
	rom[0x924AC/2] = 0x0009;
	rom[0x9251C/2] = 0x0009;
	rom[0x93966/2] = 0xFFDA;
	rom[0x93974/2] = 0xFFCC;
	rom[0x93982/2] = 0xFFBE;
	rom[0x93990/2] = 0xFFB0;
	rom[0x9399E/2] = 0xFFA2;
	rom[0x939AC/2] = 0xFF94;
	rom[0x939BA/2] = 0xFF86;
	rom[0x939C8/2] = 0xFF78;
	rom[0x939D4/2] = 0xFA5C;
	rom[0x939E0/2] = 0xFA50;
	rom[0x939EC/2] = 0xFA44;
	rom[0x939F8/2] = 0xFA38;
	rom[0x93A04/2] = 0xFA2C;
	rom[0x93A10/2] = 0xFA20;
	rom[0x93A1C/2] = 0xFA14;
	rom[0x93A28/2] = 0xFA08;
	rom[0x93A34/2] = 0xF9FC;
	rom[0x93A40/2] = 0xF9F0;
	rom[0x93A4C/2] = 0xFD14;
	rom[0x93A58/2] = 0xFD08;
	rom[0x93A66/2] = 0xF9CA;
	rom[0x93A72/2] = 0xF9BE;

}

/* Kof 10th Anniversary (bootleg of King of Fighters 2002 */

/* this uses RAM based tiles for the text layer, however the implementation
  is incomplete, at the moment the S data is copied from the program rom on
  start-up instead */

static UINT16 kof10thExtraRAMB[0x01000];

static void kof10thBankswitch(unsigned int nBank)
{
	nBank = 0x100000 + ((nBank & 7) << 20);
	if (nBank >= 0x700000)
		nBank = 0x100000;
	neogeo_set_cpu1_second_bank(nBank);
}

READ16_HANDLER( kof10th_RAMB_r )
{
	return kof10thExtraRAMB[offset];
}

WRITE16_HANDLER( kof10th_custom_w )
{
	if (!kof10thExtraRAMB[0xFFE]) { // Write to RAM bank A
		UINT16 *prom = (UINT16*)memory_region( REGION_CPU1 );
		COMBINE_DATA(&prom[(0xE0000/2) + (offset & 0xFFFF)]);
	} else { // Write S data on-the-fly
		UINT8 *srom = memory_region( REGION_GFX1 );
		srom[offset] = BITSWAP8(data,7,6,0,4,3,2,1,5);
		decodechar(Machine->gfx[0], offset>>5, srom, Machine->drv->gfxdecodeinfo[0].gfxlayout);
	}
}

static WRITE16_HANDLER( kof10th_bankswitch_w )
{
	if (offset >= 0x5F000) {
		if (offset == 0x5FFF8) { // Standard bankswitch
			kof10thBankswitch(data);
		} else if (offset == 0x5FFFC && kof10thExtraRAMB[0xFFC] != data) { // Special bankswitch
			UINT8 *src = memory_region( REGION_CPU1 );
			memcpy (src + 0x10000,  src + ((data & 1) ? 0x810000 : 0x710000), 0xcffff);
		}
		COMBINE_DATA(&kof10thExtraRAMB[offset & 0xFFF]);
	}
}

void install_kof10th_protection ( void )
{
	memory_install_read16_handler(0,  ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof10th_RAMB_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x23ffff, 0, 0, kof10th_custom_w);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x240000, 0x2fffff, 0, 0, kof10th_bankswitch_w);
}

void decrypt_kof10th( void )
{
	int i, j;
	UINT8 *dst = malloc(0x900000);
	UINT8 *src = memory_region( REGION_CPU1 );

	if (dst) {
		memcpy(dst + 0x000000, src + 0x700000, 0x100000); // Correct (Verified in Uni-bios)
		memcpy(dst + 0x100000, src + 0x000000, 0x800000);

		for (i = 0; i < 0x900000; i++) {
			j = BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,2,9,8,7,1,5,4,3,10,6,0);
			src[j] = dst[i];
		}

		free(dst);
	}

	// Altera protection chip patches these over P ROM
	((UINT16*)src)[0x0124/2] = 0x000d; // Enables XOR for RAM moves, forces SoftDIPs, and USA region
	((UINT16*)src)[0x0126/2] = 0xf7a8;

	((UINT16*)src)[0x8bf4/2] = 0x4ef9; // Run code to change "S" data
	((UINT16*)src)[0x8bf6/2] = 0x000d;
	((UINT16*)src)[0x8bf8/2] = 0xf980;
}

void decrypt_kf10thep(void)
{
	int i;
	UINT16 *rom = (UINT16*)memory_region(REGION_CPU1);
	UINT8  *src = memory_region(REGION_CPU1);
	UINT16 *buf = (UINT16*)memory_region(REGION_USER4);
	UINT8 *srom = (UINT8*)memory_region(REGION_GFX1);
	UINT8 *sbuf = malloc(0x20000);

	UINT8 *dst = (UINT8*)malloc(0x200000);

	memcpy(dst,buf,0x200000);
	memcpy(src+0x000000,dst+0x060000,0x20000);
	memcpy(src+0x020000,dst+0x100000,0x20000);
	memcpy(src+0x040000,dst+0x0e0000,0x20000);
	memcpy(src+0x060000,dst+0x180000,0x20000);
	memcpy(src+0x080000,dst+0x020000,0x20000);
	memcpy(src+0x0a0000,dst+0x140000,0x20000);
	memcpy(src+0x0c0000,dst+0x0c0000,0x20000);
	memcpy(src+0x0e0000,dst+0x1a0000,0x20000);
	memcpy(src+0x0002e0,dst+0x0402e0,0x6a); // copy banked code to a new memory region
	memcpy(src+0x0f92bc,dst+0x0492bc,0xb9e); // copy banked code to a new memory region
	for (i=0xf92bc/2;i < 0xf9e58/2 ;i++)
	{
		if (rom[i+0] == 0x4eb9 && rom[i+1] == 0x0000) rom[i+1] = 0x000F; // correct JSR in moved code
		if (rom[i+0] == 0x4ef9 && rom[i+1] == 0x0000) rom[i+1] = 0x000F; // correct JMP in moved code
	}
	rom[0x00342/2] = 0x000f;
	free(dst);

	for (i=0;i<0x20000;i++)
		sbuf[i]=srom[i^0x8];

	memcpy(srom,sbuf,0x20000);
	free(sbuf);
}

static void kf2k5uni_px_decrypt( void )
{
	int i, j, ofst;
	unsigned char *src = memory_region( REGION_CPU1 );
	unsigned char *dst = (unsigned char*)malloc(0x80);

	for (i = 0; i < 0x800000; i+=0x80)
	{
		for (j = 0; j < 0x80; j+=2)
		{
			ofst = BITSWAP8(j, 0, 3, 4, 5, 6, 1, 2, 7);
			memcpy(dst + j, src + i + ofst, 2);
		}
		memcpy(src + i, dst, 0x80);
	}
	free(dst);

	memcpy(src, src + 0x600000, 0x100000); // Seems to be the same as kof10th
}

static void kf2k5uni_sx_decrypt( void )
{
	int i;
	UINT8 *srom = memory_region( REGION_GFX1 );

	for (i = 0; i < 0x20000; i++)
		srom[i] = BITSWAP8(srom[i], 4, 5, 6, 7, 0, 1, 2, 3);
}

static void kf2k5uni_mx_decrypt( void )
{
	int i;
	UINT8 *mrom = memory_region( REGION_CPU2 );

	for (i = 0; i < 0x30000; i++)
		mrom[i] = BITSWAP8(mrom[i], 4, 5, 6, 7, 0, 1, 2, 3);
}

void decrypt_kf2k5uni(void)
{
	kf2k5uni_px_decrypt();
	kf2k5uni_sx_decrypt();
	kf2k5uni_mx_decrypt();
}

/* Kof2002 Magic Plus */

void kf2k2mp_decrypt( void )
{
	int i,j;

	unsigned char *src = memory_region(REGION_CPU1);
	unsigned char *dst = (unsigned char*)malloc(0x80);

	memcpy(src, src + 0x300000, 0x500000);

	if (dst)
	{
		for (i = 0; i < 0x800000; i+=0x80)
		{
			for (j = 0; j < 0x80 / 2; j++)
			{
				int ofst = BITSWAP8( j, 6, 7, 2, 3, 4, 5, 0, 1 );
				memcpy(dst + j * 2, src + i + ofst * 2, 2);
			}
			memcpy(src + i, dst, 0x80);
		}
	}
	free(dst);
}

/* Kof2002 Magic Plus 2 */


void kof2km2_px_decrypt( void )
{
	unsigned char *src = memory_region(REGION_CPU1);
	unsigned char *dst = (unsigned char*)malloc(0x600000);

	memcpy (dst + 0x000000, src + 0x1C0000, 0x040000);
	memcpy (dst + 0x040000, src + 0x140000, 0x080000);
	memcpy (dst + 0x0C0000, src + 0x100000, 0x040000);
	memcpy (dst + 0x100000, src + 0x200000, 0x400000);
	memcpy (src + 0x000000, dst + 0x000000, 0x600000);
	free (dst);
}


/* Crouching Tiger Hidden Dragon 2003 (bootleg of King of Fighters 2001 */

/* descrambling information from razoola */
static void cthd2003_neogeo_gfx_address_fix_do(int start, int end, int bit3shift, int bit2shift, int bit1shift, int bit0shift)
{
	int i,j;
	int tilesize=128;

	UINT8* rom = (UINT8*)malloc(16*tilesize);	// 16 tiles buffer
	UINT8* realrom = memory_region(REGION_GFX3) + start*tilesize;

	for (i = 0; i < (end-start)/16; i++) {
		for (j = 0; j < 16; j++) {
			int offset = (((j&1)>>0)<<bit0shift)
					+(((j&2)>>1)<<bit1shift)
					+(((j&4)>>2)<<bit2shift)
					+(((j&8)>>3)<<bit3shift);

			memcpy(rom+j*tilesize, realrom+offset*tilesize, tilesize);
		}
		memcpy(realrom,rom,tilesize*16);
		realrom+=16*tilesize;
	}
	free(rom);
}

static void cthd2003_neogeo_gfx_address_fix(int start, int end)
{
	cthd2003_neogeo_gfx_address_fix_do(start+512*0, end+512*0, 0,3,2,1);
	cthd2003_neogeo_gfx_address_fix_do(start+512*1, end+512*1, 1,0,3,2);
	cthd2003_neogeo_gfx_address_fix_do(start+512*2, end+512*2, 2,1,0,3);
	// skip 3 & 4
	cthd2003_neogeo_gfx_address_fix_do(start+512*5, end+512*5, 0,1,2,3);
	cthd2003_neogeo_gfx_address_fix_do(start+512*6, end+512*6, 0,1,2,3);
	cthd2003_neogeo_gfx_address_fix_do(start+512*7, end+512*7, 0,2,3,1);
}

static void cthd2003_c(int pow)
{
	int i;

	for (i=0; i<=192; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);

	for (i=200; i<=392; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);

	for (i=400; i<=592; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);

	for (i=600; i<=792; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);

	for (i=800; i<=992; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);

	for (i=1000; i<=1016; i+=8)
		cthd2003_neogeo_gfx_address_fix(i*512,i*512+512);
}

void decrypt_cthd2003(void)
{
	UINT8 *romdata = memory_region(REGION_GFX1);
	UINT8*tmp = (UINT8*)malloc(8*128*128);

	memcpy(tmp+8*0*128, romdata+8*0*128, 8*32*128);
	memcpy(tmp+8*32*128, romdata+8*64*128, 8*32*128);
	memcpy(tmp+8*64*128, romdata+8*32*128, 8*32*128);
	memcpy(tmp+8*96*128, romdata+8*96*128, 8*32*128);
	memcpy(romdata, tmp, 8*128*128);

	romdata = memory_region(REGION_CPU2)+0x10000;
 	memcpy(tmp+8*0*128, romdata+8*0*128, 8*32*128);
	memcpy(tmp+8*32*128, romdata+8*64*128, 8*32*128);
 	memcpy(tmp+8*64*128, romdata+8*32*128, 8*32*128);
	memcpy(tmp+8*96*128, romdata+8*96*128, 8*32*128);
	memcpy(romdata, tmp, 8*128*128);

 	free(tmp);

	memcpy(romdata-0x10000,romdata,0x10000);;

	cthd2003_c(0);
}

static WRITE16_HANDLER ( cthd2003_bankswitch_w )
{
	int bankaddress;
	static int cthd2003_banks[8] =
	{
		1,0,1,0,1,0,3,2,
	};
	if (offset == 0)
	{
		bankaddress = 0x100000 + cthd2003_banks[data&7]*0x100000;
		neogeo_set_cpu1_second_bank(bankaddress);
	}
}

void patch_cthd2003( void )
{
	/* patches thanks to razoola */
	int i;
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);

	/* special ROM banking handler */
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff0, 0x2fffff, 0, 0, cthd2003_bankswitch_w);

	// theres still a problem on the character select screen but it seems to be related to cpu core timing issues,
	// overclocking the 68k prevents it.

	// fix garbage on s1 layer over everything
	mem16[0xf415a/2] = 0x4ef9;
	mem16[0xf415c/2] = 0x000f;
	mem16[0xf415e/2] = 0x4cf2;
	// Fix corruption in attract mode before title screen
	for (i=0x1ae290/2;i < 0x1ae8d0/2; i=i+1)
	{
		mem16[i] = 0x0000;
	}

	// Fix for title page
	for (i=0x1f8ef0/2;i < 0x1fa1f0/2; i=i+2)
	{
		mem16[i] -= 0x7000;
		mem16[i+1] -= 0x0010;
 	}

	// Fix for green dots on title page
	for (i=0xac500/2;i < 0xac520/2; i=i+1)
	{
		mem16[i] = 0xFFFF;
	}
 	// Fix for blanks as screen change level end clear
	mem16[0x991d0/2] = 0xdd03;
	mem16[0x99306/2] = 0xdd03;
	mem16[0x99354/2] = 0xdd03;
 	mem16[0x9943e/2] = 0xdd03;
}

static void ct2k3sp_sx_decrypt( void )
{
	int rom_size = memory_region_length( REGION_GFX1 );
	UINT8 *rom = memory_region( REGION_GFX1 );
	UINT8 *buf = malloc( rom_size );
	int i;
	int ofst;

	memcpy( buf, rom, rom_size );

	for( i = 0; i < rom_size; i++ ){
		ofst = BITSWAP24( (i & 0x1ffff), 23, 22, 21, 20, 19, 18, 17,  3,
									      0,  1,  4,  2, 13, 14, 16, 15,
										  5,  6, 11, 10,  9,  8,  7, 12 );

		ofst += (i >> 17) << 17;

		rom[ i ] = buf[ ofst ];
	}

	memcpy( buf, rom, rom_size );

	memcpy( &rom[ 0x08000 ], &buf[ 0x10000 ], 0x8000 );
	memcpy( &rom[ 0x10000 ], &buf[ 0x08000 ], 0x8000 );
	memcpy( &rom[ 0x28000 ], &buf[ 0x30000 ], 0x8000 );
	memcpy( &rom[ 0x30000 ], &buf[ 0x28000 ], 0x8000 );

	free( buf );
}


void decrypt_ct2k3sp(void)
{
	UINT8 *romdata = memory_region(REGION_CPU2)+0x10000;
	UINT8*tmp = (UINT8*)malloc(8*128*128);
	memcpy(tmp+8*0*128, romdata+8*0*128, 8*32*128);
	memcpy(tmp+8*32*128, romdata+8*64*128, 8*32*128);
	memcpy(tmp+8*64*128, romdata+8*32*128, 8*32*128);
	memcpy(tmp+8*96*128, romdata+8*96*128, 8*32*128);
	memcpy(romdata, tmp, 8*128*128);

	free(tmp);
	memcpy(romdata-0x10000,romdata,0x10000);
	ct2k3sp_sx_decrypt();
	cthd2003_c(0);
}


/* King of Fighters Special Edition 2004 (bootleg of King of Fighters 2002) */

void decrypt_kof2k4se_68k( void )
{
	UINT8 *src = memory_region(REGION_CPU1)+0x100000;
	UINT8 *dst = malloc(0x400000);
	int i;
	static const unsigned int sec[] = {0x300000,0x200000,0x100000,0x000000};
	if (dst)
	{
		memcpy(dst,src,0x400000);

		for(i = 0; i < 4; ++i)
		{
		memcpy(src+i*0x100000,dst+sec[i],0x100000);
		}
			free(dst);
	}
}

/* Lans2004 (bootleg of Shock Troopers 2) */

void lans2004_vx_decrypt( void )
{
	int i;
	UINT8 *rom = memory_region( REGION_SOUND1 );
	for (i = 0; i < 0xA00000; i++)
		rom[i] = BITSWAP8(rom[i], 0, 1, 5, 4, 3, 2, 6, 7);
}

void lans2004_decrypt_68k( void )
{
	/* Descrambling P ROMs - Thanks to Razoola for the info */
	int i;
	UINT8 *src = memory_region( REGION_CPU1 );
	UINT16 *rom = (UINT16*)memory_region( REGION_CPU1 );
	UINT8 *dst = malloc(0x600000);

	if (dst) {
		static const int sec[] = { 0x3, 0x8, 0x7, 0xC, 0x1, 0xA, 0x6, 0xD };

		for (i = 0; i < 8; i++)
			memcpy (dst + i * 0x20000, src + sec[i] * 0x20000, 0x20000);

		memcpy (dst + 0x0BBB00, src + 0x045B00, 0x001710);
		memcpy (dst + 0x02FFF0, src + 0x1A92BE, 0x000010);
		memcpy (dst + 0x100000, src + 0x200000, 0x400000);
		memcpy (src, dst, 0x600000);
		free (dst);
	}

	for (i = 0xBBB00/2; i < 0xBE000/2; i++) {
		if ((((rom[i]&0xFFBF)==0x4EB9) || ((rom[i]&0xFFBF)==0x43B9)) && (rom[i+1]==0x0000)) {
			rom[i + 1] = 0x000B;
			rom[i + 2] += 0x6000;
		}
	}

	rom[0x2D15C/2] = 0x000B;
	rom[0x2D15E/2] = 0xBB00;
	rom[0x2D1E4/2] = 0x6002;
	rom[0x2EA7E/2] = 0x6002;
	rom[0xBBCD0/2] = 0x6002;
	rom[0xBBDF2/2] = 0x6002;
	rom[0xBBE42/2] = 0x6002;
}

static READ16_HANDLER( mslug5_prot_r )
{
	logerror("PC %06x: access protected\n",activecpu_get_pc());
	return 0xa0;
}

static WRITE16_HANDLER ( ms5plus_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;
	logerror("offset: %06x PC %06x: set banking %04x\n",offset,activecpu_get_pc(),data);
	if ((offset == 0)&&(data == 0xa0))
	{
		bankaddress=0xa0;
		memory_set_bankptr(4,&RAM[bankaddress]);
		//neogeo_set_cpu1_second_bank(bankaddress);
		logerror("offset: %06x PC %06x: set banking %04x\n\n",offset,activecpu_get_pc(),bankaddress);
	}
	else if(offset == 2)
	{
		data=data>>4;
		//data=data&7;
		bankaddress=data*0x100000;
		memory_set_bankptr(4,&RAM[bankaddress]);
		//neogeo_set_cpu1_second_bank(bankaddress);
		logerror("offset: %06x PC %06x: set banking %04x\n\n",offset,activecpu_get_pc(),bankaddress);
	}
}

void install_ms5plus_protection(void)
{
	// special ROM banking handler
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM,0x2ffff0, 0x2fffff,0, 0, ms5plus_bankswitch_w);
	// additional protection
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2ffff0, 0x2fffff, 0,0, mslug5_prot_r);
}



void svcboot_px_decrypt( void )
{
	static const unsigned char sec[] = {
		0x06, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00
	};
	int i;
	int size = memory_region_length( REGION_CPU1 );
	UINT8 *src = memory_region( REGION_CPU1 );
	UINT8 *dst = malloc( size );
	int ofst;
	for( i = 0; i < size / 0x100000; i++ ){
		memcpy( &dst[ i * 0x100000 ], &src[ sec[ i ] * 0x100000 ], 0x100000 );
	}
	for( i = 0; i < size / 2; i++ ){
		ofst = BITSWAP8( (i & 0x0000ff), 7, 6, 1, 0, 3, 2, 5, 4 );
		ofst += (i & 0xffff00);
		memcpy( &src[ i * 2 ], &dst[ ofst * 2 ], 0x02 );
	}
	free( dst );
}



void svcboot_cx_decrypt( void )
{
	static const unsigned char idx_tbl[ 0x10 ] = {
		0, 1, 0, 1, 2, 3, 2, 3, 3, 4, 3, 4, 4, 5, 4, 5,
	};
	static const unsigned char bitswap4_tbl[ 6 ][ 4 ] = {
		{ 3, 0, 1, 2 },
		{ 2, 3, 0, 1 },
		{ 1, 2, 3, 0 },
		{ 0, 1, 2, 3 },
		{ 3, 2, 1, 0 },
		{ 3, 0, 2, 1 },
	};
	int i;
	int size = memory_region_length( REGION_GFX3 );
	UINT8 *src = memory_region( REGION_GFX3 );
	UINT8 *dst = malloc( size );
	int ofst;
	memcpy( dst, src, size );
	for( i = 0; i < size / 0x80; i++ ){
		int idx = idx_tbl[ (i & 0xf00) >> 8 ];
		int bit0 = bitswap4_tbl[ idx ][ 0 ];
		int bit1 = bitswap4_tbl[ idx ][ 1 ];
		int bit2 = bitswap4_tbl[ idx ][ 2 ];
		int bit3 = bitswap4_tbl[ idx ][ 3 ];
		ofst = BITSWAP8( (i & 0x0000ff), 7, 6, 5, 4, bit3, bit2, bit1, bit0 );
		ofst += (i & 0xfffff00);
		memcpy( &src[ i * 0x80 ], &dst[ ofst * 0x80 ], 0x80 );
	}
	free( dst );
}



void svcplus_px_decrypt( void )
{
	static const int sec[] = {
		0x00, 0x03, 0x02, 0x05, 0x04, 0x01
	};
	int size = memory_region_length( REGION_CPU1 );
	UINT8 *src = memory_region( REGION_CPU1 );
	UINT8 *dst = malloc( size );
	int i;
	int ofst;
	memcpy( dst, src, size );
	for( i = 0; i < size / 2; i++ ){
		ofst = BITSWAP24( (i & 0xfffff), 0x17, 0x16, 0x15, 0x14, 0x13, 0x00, 0x01, 0x02,
										 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
										 0x07, 0x06, 0x05, 0x04, 0x03, 0x10, 0x11, 0x12 );
		ofst ^= 0x0f0007;
		ofst += (i & 0xff00000);
		memcpy( &src[ i * 0x02 ], &dst[ ofst * 0x02 ], 0x02 );
	}
	memcpy( dst, src, size );
	for( i = 0; i < 6; i++ ){
		memcpy( &src[ i * 0x100000 ], &dst[ sec[ i ] * 0x100000 ], 0x100000 );
	}
	free( dst );
}



void svcplus_px_hack( void )
{
	/* patched by the protection chip? */
	UINT8 *src = memory_region( REGION_CPU1 );
	src[ 0x0f8010 ] = 0x40;
	src[ 0x0f8011 ] = 0x04;
	src[ 0x0f8012 ] = 0x00;
	src[ 0x0f8013 ] = 0x10;
	src[ 0x0f8014 ] = 0x40;
	src[ 0x0f8015 ] = 0x46;
	src[ 0x0f8016 ] = 0xc1;
	src[ 0x0f802c ] = 0x16;
}

void svcplusa_px_decrypt( void )
{
	int i;
	static const int sec[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x00
	};
	int size = memory_region_length( REGION_CPU1 );
	UINT8 *src = memory_region( REGION_CPU1 );
	UINT8 *dst = malloc( size );
	memcpy( dst, src, size );
	for( i = 0; i < 6; i++ ){
		memcpy( &src[ i * 0x100000 ], &dst[ sec[ i ] * 0x100000 ], 0x100000 );
	}
	free( dst );
}



void svcsplus_px_decrypt( void )
{
	static const int sec[] = {
		0x06, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00
	};
	int size = memory_region_length( REGION_CPU1 );
	UINT8 *src = memory_region( REGION_CPU1 );
	UINT8 *dst = malloc( size );
	int i;
	int ofst;
	memcpy( dst, src, size );
	for( i = 0; i < size / 2; i++ ){
		ofst = BITSWAP16( (i & 0x007fff), 0x0f, 0x00, 0x08, 0x09, 0x0b, 0x0a, 0x0c, 0x0d,
										  0x04, 0x03, 0x01, 0x07, 0x06, 0x02, 0x05, 0x0e );

		ofst += (i & 0x078000);
		ofst += sec[ (i & 0xf80000) >> 19 ] << 19;
		memcpy( &src[ i * 2 ], &dst[ ofst * 2 ], 0x02 );
	}
	free( dst );
}



void svcsplus_px_hack( void )
{
	/* patched by the protection chip? */
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	mem16[0x9e90/2] = 0x000f;
	mem16[0x9e92/2] = 0xc9c0;
	mem16[0xa10c/2] = 0x4eb9;
	mem16[0xa10e/2] = 0x000e;
	mem16[0xa110/2] = 0x9750;
}

static UINT16 mv0_bank_ram[ 0x10/2 ];

static READ16_HANDLER( mv0_bankswitch_r )
{
    return mv0_bank_ram[ offset ];
}

static WRITE16_HANDLER( mv0_bankswitch_w )
{
    UINT32 bankaddress = (mv0_bank_ram[ 0 ] >> 8) + (mv0_bank_ram[ 1 ] << 8) + 0x100000;
	COMBINE_DATA( &mv0_bank_ram[ offset ] );
    neogeo_set_cpu1_second_bank( bankaddress );
}

/* I think the code for the kof2003 bootlegs is wrong, they don't work */

/***************************************************************************
 kof2003b
****************************************************************************/

/* kof2003 bootleg init info */
static UINT16 kof2003_tbl[4096];

READ16_HANDLER( kof2003_r)
{
	return kof2003_tbl[offset];
}

WRITE16_HANDLER( kof2003_w )
{
	data = COMBINE_DATA(&kof2003_tbl[offset]);
	if (offset == 0x1ff0/2 || offset == 0x1ff2/2) {
		UINT8* cr = (UINT8 *)kof2003_tbl;
		UINT32 address = (cr[0x1ff3]<<16)|(cr[0x1ff2]<<8)|cr[0x1ff1];
		UINT8 prt = cr[0x1ff2];
		UINT8* mem = (UINT8 *)memory_region(REGION_CPU1);

		cr[0x1ff0] =  0xa0;
		cr[0x1ff1] &= 0xfe;
		cr[0x1ff3] &= 0x7f;
		neogeo_set_cpu1_second_bank(address+0x100000);

		mem[0x58196] = prt;
	}
}

WRITE16_HANDLER( kof2003p_w )
{
	data = COMBINE_DATA(&kof2003_tbl[offset]);
	if (offset == 0x1ff0/2 || offset == 0x1ff2/2) {
		UINT8* cr = (UINT8 *)kof2003_tbl;
		UINT32 address = (cr[0x1ff3]<<16)|(cr[0x1ff2]<<8)|cr[0x1ff0];
		UINT8 prt = cr[0x1ff2];
		UINT8* mem = (UINT8 *)memory_region(REGION_CPU1);

		cr[0x1ff0] &= 0xfe;
		cr[0x1ff3] &= 0x7f;
		neogeo_set_cpu1_second_bank(address+0x100000);

		mem[0x58196] = prt;
	}
}

void kof2003b_px_decrypt( void )
{
	int i;
	static const unsigned char sec[] = {
		0x07, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
	};

    int rom_size = 0x800000;
    UINT8 *rom = memory_region( REGION_CPU1 );
    UINT8 *buf = malloc( rom_size );
    memcpy( buf, rom, rom_size );

    for( i = 0; i < rom_size / 0x100000; i++ ){
        memcpy( &rom[ i * 0x100000 ], &buf[ sec[ i ] * 0x100000 ], 0x100000 );
    }
    free( buf );
}

void kof2003b_install_protection(void)
{
    memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003_r );
    memory_install_write16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003_w );
}


/***************************************************************************
 kof2k3pl
***************************************************************************/

void kof2k3pl_px_decrypt( void )
{
	UINT16*tmp = (UINT16*)malloc(0x100000);
	UINT16*rom = (UINT16*)memory_region( REGION_CPU1 );
	int j;
	int i;

	for (i = 0;i < 0x700000/2;i+=0x100000/2)
	{
		memcpy(tmp,&rom[i],0x100000);
		for (j = 0;j < 0x100000/2;j++)
			rom[i+j] = tmp[BITSWAP24(j,23,22,21,20,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)];
	}
	free(tmp);

	/* patched by Altera protection chip on PCB */
	rom[0xf38ac/2] = 0x4e75;
}

void kf2k3pl_install_protection(void)
{
    memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003_r );
    memory_install_write16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003p_w );
}


/***************************************************************************
 kof2k3up
***************************************************************************/


void kof2k3up_px_decrypt( void )
{
	{
		UINT8 *src = memory_region(REGION_CPU1);
		memcpy(src+0x100000, src, 0x600000);
		memcpy(src, src+0x700000, 0x100000);
	}

	{

		int ofst;
		int i;
		UINT8 *rom = memory_region( REGION_CPU1 ) + 0xfe000;
		UINT8 *buf = memory_region( REGION_CPU1 ) + 0xd0610;

		for( i = 0; i < 0x2000 / 2; i++ ){
			ofst = (i & 0xff00) + BITSWAP8( (i & 0x00ff), 7, 6, 0, 4, 3, 2, 1, 5 );
			memcpy( &rom[ i * 2 ], &buf[ ofst * 2 ], 2 );
		}
	}
}

void kof2k3up_install_protection(void)
{
    memory_install_read16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003_r );
    memory_install_write16_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2fe000, 0x2fffff, 0, 0, kof2003_w );
}

/* samsho5bl */

void samsh5bl_px_decrypt( void )
{
	int px_size = memory_region_length( REGION_CPU1 );
	UINT8 *rom = memory_region( REGION_CPU1 );
	UINT8 *buf = malloc( px_size );
	int ofst;
	int i;

	memcpy( buf, rom, px_size );

	for( i = 0; i < px_size / 2; i++ ){
		ofst = BITSWAP8( (i & 0x000ff), 7, 6, 5, 4, 3, 0, 1, 2 );
		ofst += (i & 0xfffff00);
		ofst ^= 0x060005;

		memcpy( &rom[ i * 2 ], &buf[ ofst * 2 ], 0x02 );
	}

	memcpy( buf, rom, px_size );

	memcpy( &rom[ 0x000000 ], &buf[ 0x700000 ], 0x100000 );
	memcpy( &rom[ 0x100000 ], &buf[ 0x000000 ], 0x700000 );

	free( buf );
}


