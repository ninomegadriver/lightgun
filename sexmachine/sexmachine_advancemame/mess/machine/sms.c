#include <stdio.h>
#include "driver.h"
#include "image.h"
#include "includes/sms.h"
#include "sound/2413intf.h"

UINT8 smsRomPageCount;
UINT8 smsBiosPageCount;
UINT8 smsFMDetect;
UINT8 smsVersion;
int smsPaused;

UINT8 biosPort;

UINT8 *BIOS;
UINT8 *ROM;

UINT8 *sms_mapper_ram;
UINT8 sms_mapper[4];
UINT8 *sms_banking_bios[5]; /* we are going to use 1-4, same as bank numbers */
UINT8 *sms_banking_cart[5]; /* we are going to use 1-4, same as bank numbers */
UINT8 *sms_banking_none[5]; /* we are going to use 1-4, same as bank numbers */
UINT8 smsNVRam[NVRAM_SIZE];
int smsNVRAMSaved = 0;
UINT8 ggSIO[5] = { 0x7F, 0xFF, 0x00, 0xFF, 0x00 };

WRITE8_HANDLER(sms_fm_detect_w) {
	if ( HAS_FM ) {
		smsFMDetect = (data & 0x01);
	}
}

 READ8_HANDLER(sms_fm_detect_r) {
	if (biosPort & IO_CHIP) {
		return (0xFF);
	}

	if ( HAS_FM ) {
		return smsFMDetect;
	} else {
		return readinputport(0);
	}
}

WRITE8_HANDLER(sms_version_w) {
	if ((data & 0x01) && (data & 0x04)) {
		smsVersion = (data & 0xA0);
	}
}

 READ8_HANDLER(sms_version_r) {
	UINT8 temp;

	if (biosPort & IO_CHIP) {
		return (0xFF);
	}

	/* Move bits 7,5 of port 3F into bits 7, 6 */
	temp = (smsVersion & 0x80) | (smsVersion & 0x20) << 1;

	/* Inverse version detect value for Japanese machines */
	if ( IS_REGION_JAPAN ) {
		temp ^= 0xC0;
	}

	/* Merge version data with input port #2 data */
	temp = (temp & 0xC0) | (readinputport(1) & 0x3F);

	return (temp);
}

void check_pause_button( void ) {
	if ( ! IS_GAMEGEAR ) {
		if ( ! (readinputport(2) & 0x80) ) {
			if ( ! smsPaused ) {
				cpunum_set_input_line( 0, INPUT_LINE_NMI, ASSERT_LINE );
				cpunum_set_input_line( 0, INPUT_LINE_NMI, CLEAR_LINE );
			}
			smsPaused = 1;
		} else {
			smsPaused = 0;
		}
	}
}

 READ8_HANDLER(sms_input_port_0_r) {
	if (biosPort & IO_CHIP) {
		return (0xFF);
	} else {
		return readinputport(0);
	}
}

WRITE8_HANDLER(sms_YM2413_register_port_0_w) {
	if ( HAS_FM ) {
		YM2413_register_port_0_w(offset, (data & 0x3F));
	}
}

WRITE8_HANDLER(sms_YM2413_data_port_0_w) {
	if ( HAS_FM ) {
		logerror("data_port_0_w %x %x\n", offset, data);
		YM2413_data_port_0_w(offset, data);
	}
}

 READ8_HANDLER(gg_input_port_2_r) {
	//logerror("joy 2 read, val: %02x, pc: %04x\n", (( IS_REGION_JAPAN ? 0x00 : 0x40) | (readinputport(2) & 0x80)), activecpu_get_pc());
	return (( IS_REGION_JAPAN ? 0x00 : 0x40 ) | (readinputport(2) & 0x80));
}

 READ8_HANDLER(sms_mapper_r)
{
	return sms_mapper[offset];
}

WRITE8_HANDLER(sms_mapper_w)
{
	int page;
	UINT8 *SOURCE_BIOS;
	UINT8 *SOURCE_CART;
	UINT8 *SOURCE;

	offset &= 3;

	sms_mapper[offset] = data;
	sms_mapper_ram[offset] = data;

	if ( ROM ) {
		SOURCE_CART = ROM + ( (smsRomPageCount > 0) ? data % smsRomPageCount : 0 ) * 0x4000;
	} else {
		SOURCE_CART = memory_region(REGION_CPU1);
	}
	if ( BIOS ) {
		SOURCE_BIOS = BIOS + ( (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0 ) * 0x4000;
	} else {
		SOURCE_BIOS = memory_region(REGION_CPU1);
	}

	if (biosPort & IO_BIOS_ROM || IS_GAMEGEAR )
	{
		if (!(biosPort & IO_CARTRIDGE) || IS_GAMEGEAR )
		{
			page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;
			if ( ! ROM )
				return;
			SOURCE = SOURCE_CART;
		}
		else
		{
			/* nothing to page in */
			return;
		}
	}
	else
	{
		page = (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0;
		if ( ! BIOS )
			return;
		SOURCE = SOURCE_BIOS;
	}

	switch(offset) {
		case 0: /* Control */
			/* Is it ram or rom? */
			if (data & 0x08) { /* it's ram */
				smsNVRAMSaved = 1;			/* SRAM should be saved on exit. */
				if (data & 0x04) {
#ifdef LOG_PAGING
					logerror("ram 1 paged.\n");
#endif
					SOURCE = smsNVRam + 0x4000;
				} else {
#ifdef LOG_PAGING
					logerror("ram 0 paged.\n");
#endif
					SOURCE = smsNVRam;
				}
				sms_banking_bios[4] = SOURCE;
				sms_banking_cart[4] = SOURCE;
				memory_set_bankptr( 4, SOURCE );
			} else { /* it's rom */
				if ( ROM ) {
					SOURCE_CART = ROM + ( (smsRomPageCount > 0) ? sms_mapper[3] % smsRomPageCount : 0 ) * 0x4000;
					sms_banking_cart[4] = SOURCE_CART;
				}
				if ( BIOS ) {
					SOURCE_BIOS = BIOS + ( (smsBiosPageCount > 0) ? sms_mapper[3] % smsBiosPageCount : 0 ) * 0x4000;
					sms_banking_bios[4] = SOURCE_BIOS;
				}
				if (biosPort & IO_BIOS_ROM) {
					page = (smsRomPageCount > 0) ? sms_mapper[3] % smsRomPageCount : 0;
					SOURCE = sms_banking_cart[4];
				} else {
					page = (smsBiosPageCount > 0) ? sms_mapper[3] % smsBiosPageCount : 0;
					SOURCE = sms_banking_bios[4];
				}
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				memory_set_bankptr( 4, SOURCE );
			}
			break;
		case 1: /* Select 16k ROM bank for 0400-3FFF */
#ifdef LOG_PAGING
			logerror("rom 0 paged in %x.\n", page);
#endif
			sms_banking_bios[2] = SOURCE_BIOS + 0x0400;
			sms_banking_cart[2] = SOURCE_CART + 0x0400;
			memory_set_bankptr( 2, SOURCE + 0x0400 );
			break;
		case 2: /* Select 16k ROM bank for 4000-7FFF */
#ifdef LOG_PAGING
			logerror("rom 1 paged in %x.\n", page);
#endif
			sms_banking_bios[3] = SOURCE_BIOS;
			sms_banking_cart[3] = SOURCE_CART;
			memory_set_bankptr( 3, SOURCE );
			break;
		case 3: /* Select 16k ROM bank for 8000-BFFF */
			/* Is it ram or rom? */
			if (!(sms_mapper[0] & 0x08)) { /* it's rom */
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				sms_banking_bios[4] = SOURCE_BIOS;
				sms_banking_cart[4] = SOURCE_CART;
				memory_set_bankptr( 4, SOURCE );
			}
			break;
	}
}

WRITE8_HANDLER(sms_bios_w) {
	biosPort = data;

	logerror("bios write %02x, pc: %04x\n", data, activecpu_get_pc());

	setup_rom();
}

WRITE8_HANDLER(sms_cartram_w) {
	int page;

	if (sms_mapper[0] & 0x08) {
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (sms_mapper[0] & 0x04) {
			smsNVRam[offset + 0x4000] = data;
		} else {
			smsNVRam[offset] = data;
		}
	} else {
		if (offset == 0) { /* Codemasters mapper */
			if (biosPort & IO_BIOS_ROM) {
				page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;
			} else {
				return;
			}
			if ( ! ROM )
				return;
			sms_banking_cart[4] = ROM + page * 0x4000;
			memory_set_bankptr( 4, sms_banking_cart[4] );
#ifdef LOG_PAGING
			logerror("rom 2 paged in %x codemasters.\n", page);
#endif
		} else if (offset == 0x2000) { /* Dodgeball King mapper */
			if (biosPort & IO_BIOS_ROM) {
				page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;
			} else {
				return;
			}
			if ( ! ROM )
				return;
			sms_banking_cart[4] = ROM + page * 0x4000;
			memory_set_bankptr( 4, sms_banking_cart[4] );
#ifdef LOG_PAGING
			logerror("rom 2 paged in %x dodgeball king.\n", page);
#endif
		} else {
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}

/*WRITE8_HANDLER(sms_ram_w) {
	UINT8 *RAM = memory_region(REGION_CPU1);

	RAM[0xC000 + (offset & 0x1FFF)] = data;
	if ((offset & 0x1FFF) <= 0x1FFB) {
		RAM[0xE000 + (offset & 0x1FFF)] = data;
	}
}*/

WRITE8_HANDLER(gg_sio_w) {
	logerror("*** write %02X to SIO register #%d\n", data, offset);

	ggSIO[offset & 0x07] = data;
	switch(offset & 7) {
		case 0x00: /* Parallel Data */
			break;
		case 0x01: /* Data Direction/ NMI Enable */
			break;
		case 0x02: /* Serial Output */
			break;
		case 0x03: /* Serial Input */
			break;
		case 0x04: /* Serial Control / Status */
			break;
	}
}

 READ8_HANDLER(gg_sio_r) {
	logerror("*** read SIO register #%d\n", offset);

	switch(offset & 7) {
		case 0x00: /* Parallel Data */
			break;
		case 0x01: /* Data Direction/ NMI Enable */
			break;
		case 0x02: /* Serial Output */
			break;
		case 0x03: /* Serial Input */
			break;
		case 0x04: /* Serial Control / Status */
			break;
	}

	return ggSIO[offset];
}

 READ8_HANDLER(gg_psg_r) {
	return 0xFF;
}

WRITE8_HANDLER(gg_psg_w) {
	logerror("write %02X to psg at offset #%d.\n",data , offset);

	/* D7 = Noise Left */
	/* D6 = Tone3 Left */
	/* D5 = Tone2 Left */
	/* D4 = Tone1 Left */

	/* D3 = Noise Right */
	/* D2 = Tone3 Right */
	/* D1 = Tone2 Right */
	/* D0 = Tone1 Right */
}

NVRAM_HANDLER(sms) {
	if (file) {
		if (read_or_write) {
			if (smsNVRAMSaved) {
				mame_fwrite(file, smsNVRam, sizeof(UINT8) * NVRAM_SIZE);
			}
		} else {
			mame_fread(file, smsNVRam, sizeof(UINT8) * NVRAM_SIZE);
		}
	} else {
		/* initially zero out SRAM */
		memset(smsNVRam, 0, sizeof(UINT8) * NVRAM_SIZE);
	}
}

void setup_rom(void)
{
	/* 1. set up bank pointers to point to nothing */
	memory_set_bankptr( 1, sms_banking_none[1] );
	memory_set_bankptr( 2, sms_banking_none[2] );
	memory_set_bankptr( 3, sms_banking_none[3] );
	memory_set_bankptr( 4, sms_banking_none[4] );

	/* 2. check and set up expansion port */
	if (!(biosPort & IO_EXPANSION) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_CARD)) {
		/* TODO: Implement me */
		logerror( "Switching to unsupported expansion port.\n" );
	}

	/* 3. check and set up card rom */
	if (!(biosPort & IO_CARD) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION)) {
		/* TODO: Implement me */
		logerror( "Switching to unsupported card rom port.\n" );
	}

	/* 4. check and set up cartridge rom */
	if ( ( !(biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION) && (biosPort & IO_CARD) ) || IS_GAMEGEAR ) {
		memory_set_bankptr( 1, sms_banking_cart[1] );
		memory_set_bankptr( 2, sms_banking_cart[2] );
		memory_set_bankptr( 3, sms_banking_cart[3] );
		memory_set_bankptr( 4, sms_banking_cart[4] );
		logerror( "Switched in cartridge rom.\n" );
	}

	/* 5. check and set up bios rom */
	if ( !(biosPort & IO_BIOS_ROM) ) {
		/* 0x0400 bioses */
		if ( HAS_BIOS_0400 ) {
			memory_set_bankptr( 1, sms_banking_bios[1] );
			logerror( "Switched in 0x0400 bios.\n" );
		}
		/* 0x2000 bioses */
		if ( HAS_BIOS_2000 ) {
			memory_set_bankptr( 1, sms_banking_bios[1] );
			memory_set_bankptr( 2, sms_banking_bios[2] );
			logerror( "Switched in 0x2000 bios.\n" );
		}
		if ( HAS_BIOS_FULL ) {
			memory_set_bankptr( 1, sms_banking_bios[1] );
			memory_set_bankptr( 2, sms_banking_bios[2] );
			memory_set_bankptr( 3, sms_banking_bios[3] );
			memory_set_bankptr( 4, sms_banking_bios[4] );
			logerror( "Switched in full bios.\n" );
		}
	}
}

static int sms_verify_cart(UINT8 *magic, int size) {
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000) {
		if (!strncmp((char*)&magic[0x7FF0], "TMR SEGA", 8)) {
			/* Technically, it should be this, but remove for now until verified:
			if (!strcmp(sysname, "gamegear")) {
				if ((unsigned char)magic[0x7FFD] < 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			if (!strcmp(sysname, "sms")) {
				if ((unsigned char)magic[0x7FFD] >= 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			*/
			retval = IMAGE_VERIFY_PASS;
		}
	}

		/* Check at $81f0 also */
		//if (!retval) {
	 //	 if (!strncmp(&magic[0x81f0], "TMR SEGA", 8)) {
				/* Technically, it should be this, but remove for now until verified:
				if (!strcmp(sysname, "gamegear")) {
					if ((unsigned char)magic[0x81fd] < 0x50)
						retval = IMAGE_VERIFY_PASS;
				}
				if (!strcmp(sysname, "sms")) {
					if ((unsigned char)magic[0x81fd] >= 0x50)
						retval = IMAGE_VERIFY_PASS;
				}
				*/
		 //	 retval = IMAGE_VERIFY_PASS;
		//	}
		//}

	return retval;
}

DEVICE_INIT( sms_cart )
{
	biosPort = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if ( ! IS_GAMEGEAR && ! HAS_BIOS ) {
		biosPort &= ~(IO_CARTRIDGE);
		biosPort |= IO_BIOS_ROM;
	}

	return INIT_PASS;
}

DEVICE_LOAD( sms_cart )
{
	int size;

	/* Get file size */
	size = mame_fsize(file);

	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		mame_fseek(file, 512, SEEK_SET);
		size -= 512;
	}

	if ( ! size ) {
		logerror("ROM image too small!\n");
		return INIT_FAIL;
	}

	/* Create a new memory region to hold the ROM. */
	/* Make sure the region holds only complete (0x4000) rom banks */
	if (new_memory_region(REGION_USER2, ((size&0x3FFF) ? (((size>>14)+1)<<14) : size), ROM_REQUIRED)) {
		logerror("Memory allocation failed reading roms!\n");
		return INIT_FAIL;
	}
	ROM = memory_region(REGION_USER2);

	/* Load ROM banks */
	size = mame_fread(file, ROM, size);

	/* check the image */
	if ( ! HAS_BIOS ) {
		if (sms_verify_cart(ROM, size) == IMAGE_VERIFY_FAIL) {
			logerror("Warning loading image: sms_verify_cart failed\n");
		}
	}

	return INIT_PASS;
}

static void setup_banks( void ) {
	sms_banking_bios[1] = sms_banking_cart[1] = sms_banking_none[1] = memory_region(REGION_CPU1);
	sms_banking_bios[2] = sms_banking_cart[2] = sms_banking_none[2] = memory_region(REGION_CPU1) + 0x0400;
	sms_banking_bios[3] = sms_banking_cart[3] = sms_banking_none[3] = memory_region(REGION_CPU1) + 0x4000;
	sms_banking_bios[4] = sms_banking_cart[4] = sms_banking_none[4] = memory_region(REGION_CPU1) + 0x8000;

	BIOS = memory_region(REGION_USER1);
	ROM = memory_region(REGION_USER2);

	smsBiosPageCount = ( BIOS ? memory_region_length(REGION_USER1) / 0x4000 : 0 );
	smsRomPageCount = ( ROM ? memory_region_length(REGION_USER2) / 0x4000 : 0 );

	if ( ROM ) {
		sms_banking_cart[1] = ROM;
		sms_banking_cart[2] = ROM + 0x0400;
		sms_banking_cart[3] = ROM + ( ( 1 < smsRomPageCount ) ? 0x4000 : 0 );
		sms_banking_cart[4] = ROM + ( ( 2 < smsRomPageCount ) ? 0x8000 : 0 );
	}

	if ( BIOS ) {
		sms_banking_bios[1] = BIOS;
		sms_banking_bios[2] = BIOS + 0x0400;
		sms_banking_bios[3] = BIOS + ( ( 1 < smsBiosPageCount) ? 0x4000 : 0 );
		sms_banking_bios[4] = BIOS + ( ( 2 < smsBiosPageCount) ? 0x8000 : 0 );
	}
}

MACHINE_RESET(sms)
{
	smsVersion = 0x00;
	if ( HAS_FM ) {
		smsFMDetect = 0x01;
	}

	memset( memory_region(REGION_CPU1), 0xff, 0x10000 );

	sms_mapper_ram = memory_get_write_ptr( 0, ADDRESS_SPACE_PROGRAM, 0xDFFC );

	/* Initialize SIO stuff for GG */
	ggSIO[0] = 0x7F;
	ggSIO[1] = 0xFF;
	ggSIO[2] = 0x00;
	ggSIO[3] = 0xFF;
	ggSIO[4] = 0x00;

	setup_banks();

	setup_rom();
}

