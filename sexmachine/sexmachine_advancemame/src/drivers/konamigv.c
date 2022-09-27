/***************************************************************************

  Konami GV System (aka "Baby Phoenix") - Arcade PSX Hardware
  ===========================================================
  Driver by R. Belmont & smf


Known Dumps
-----------

Game       Description                  Mother Board   Code       Version       Date   Time

pbball96   Powerful Pro Baseball '96    GV999          GV017   JAPAN 1.03   96.05.27  18:00
hyperath   Hyper Athlete                ZV610          GV021   JAPAN 1.00   96.06.09  19:00
susume     Susume! Taisen Puzzle-Dama   ZV610          GV027   JAPAN 1.20   96.03.04  12:00
btchamp    Beat the Champ               GV999          GV053   UAA01        ?
kdeadeye   Dead Eye                     GV999          GV054   UA01         ?
weddingr   Wedding Rhapsody             ?              GX624   JAA          97.05.29   9:12
nagano98   Winter Olypmics in Nagano 98 GV999          GX720   EAA01 1.03   98.01.08  10:45
simpbowl   Simpsons Bowling             ?              GQ829   UAA          ?

PCB Layouts
-----------

ZV610 PWB301331
|---------------------------------------|
|   000180       056602      LM324   CN8|
|CN2                                    |
|                                       |
|      999A01.7E                     CN6|
|                         CXD2922BQ     |
|      10E                KM416V256BLT-7|
|                                       |
|J     12E                              |
|A CXD2923AR     058239                 |
|M                                      |
|M                     CXD8530BQ        |
|A   D482445LGW-A70            93CF96-2 |
|               CXD8514Q               S|
|    D482445LGW-A70                    C|
|                      67.7376MHz      S|
|         53.693175MHz                 I|
|                                 32MHz |
|    93C46   KM48V514BJ-6  KM48V514BJ-6 |
|            KM48V514BJ-6  KM48V514BJ-6 |
|    CN5      CN3                001231 |
|---------------------------------------|

GV999 PWB301949A
|---------------------------------------|
|                056602      LM324   CN8|
|CN2                                    |
|TEST_SW                                |
|      999A01.7E                     CN6|
|MC44200                  CXD2925Q      |
|      9E                 TC51V4260BJ-80|
|                                       |
|J     12E                              |
|A               058239                 |
|M  53.693175MHz                        |
|M                     CXD8530CQ        |
|A                             93CF96-2 |
|      CXD8561Q                        S|
|              KM4132G271Q-12          C|
|                      67.7376MHz      S|
|         53.693175MHz                 I|
|                                 32MHz |
|    93C46   KM48V514BJ-6  KM48V514BJ-6 |
|            KM48V514BJ-6  KM48V514BJ-6 |
|    CN5      CN3                001231 |
|---------------------------------------|

Notes:
      - Simpsons Bowling uses an unknown board revision with 4 x 16M FLASHROMs & a trackball.

      - 000180 is used for driving the RGB output. It's a very thin piece of very brittle ceramic
        containing a circuit, a LM1203 chip, some smt transistors/caps/resistors etc (let's just say
        placing this thing on the edge of the PCB wasn't a good design choice!)
        On GV999, it has been replaced by three transistors and a MC44200.

      - 056602 seems to be some sort of A/D converter (another ceramic thing containing caps/resistors/transistors and a chip)

      - CXD2922 and CXD2925 are SPU's.

      - The BIOS on ZV610 and GV999 is identical. It's a 4M MASK ROM, compatible with 27C040.

      - The CD contains one MODE 1 data track and several Redbook audio tracks which are streamed to the speaker via CN8.

      - The ZV and GV PCB's are virtually identical aside from some minor component shuffling and the RGB output mechanism.
        However note that the GPU revision is different between the two boards and so are some of the other Sony IC's.

      - CN8 used to connect redbook audio output from CD drive to PCB.

      - CN6 used to connect power to CD drive.

      - CN2 used for extra speaker connection for stereo output.

      - CN3, CN5 used for connecting 3rd and 4th player controls.

      - 001231, 058239 are PALCE16V8H PALs.

      - 10E, 12E are unpopulated positions for 16M TSOP56 FLASHROMs (10E is 9E on GV999).

      - If the CD is swapped to another GV game, the game will boot but will stop with an error '25C MBAD' (the EEPROM is 25C)
        So the games can not be swapped by simply exchanging CDs because the EEPROM will not re-init itself if the CDROM is swapped.
        This appears to be some form of mild protection to stop operators swapping CD's.
        However it is possible to swap games to another PCB by exchanging the CD _AND_ the EEPROM from another PCB which belongs
        to that same game. It won't work with a blank EEPROM or a different games' EEPROM.
*/

#include "driver.h"
#include "cdrom.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/eeprom.h"
#include "machine/intelfsh.h"
#include "machine/am53cf96.h"
#include "sound/psx.h"
#include "sound/cdda.h"

/* EEPROM handlers */

static NVRAM_HANDLER( konamigv_93C46 )
{
	if( read_or_write )
	{
		EEPROM_save( file );
	}
	else
	{
		EEPROM_init( &eeprom_interface_93C46 );

		if( file )
		{
			EEPROM_load( file );
		}
		else
		{
			EEPROM_set_data( memory_region( REGION_USER2 ), memory_region_length( REGION_USER2 ) );
		}
	}
}

static READ32_HANDLER( eeprom_r )
{
	return 0xffff0000 | ( EEPROM_read_bit() << 13 ) | readinputport( 0 );
}

static WRITE32_HANDLER( eeprom_w )
{
	EEPROM_write_bit((data&0x01) ? 1 : 0);
	EEPROM_set_clock_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data&0x02) ? CLEAR_LINE : ASSERT_LINE);
}

static READ32_HANDLER( inputs_0_r )
{
	return 0xffff0000 | readinputport(1);
}

static READ32_HANDLER( inputs_1_r )
{
	return 0xffff0000 | readinputport(2);
}

static WRITE32_HANDLER( mb89371_w )
{
}

static READ32_HANDLER( mb89371_r )
{
	return 0xffffffff;
}

static ADDRESS_MAP_START( konamigv_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f00001f) AM_READWRITE(am53cf96_r, am53cf96_w)
	AM_RANGE(0x1f100000, 0x1f100003) AM_READ(eeprom_r)
	AM_RANGE(0x1f100004, 0x1f100007) AM_READ(inputs_0_r)
	AM_RANGE(0x1f100008, 0x1f10000b) AM_READ(inputs_1_r)
	AM_RANGE(0x1f180000, 0x1f180003) AM_WRITE(eeprom_w)
	AM_RANGE(0x1f680000, 0x1f68001f) AM_READWRITE(mb89371_r, mb89371_w)
	AM_RANGE(0x1f780000, 0x1f780003) AM_WRITENOP /* watchdog? */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x801fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa01fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

/* SCSI */

static UINT8 sector_buffer[ 4096 ];

static void scsi_dma_read( UINT32 n_address, INT32 n_size )
{
	int i;
	int n_this;

	while( n_size > 0 )
	{
		if( n_size > sizeof( sector_buffer ) / 4 )
		{
			n_this = sizeof( sector_buffer ) / 4;
		}
		else
		{
			n_this = n_size;
		}
		if( n_this < 2048 / 4 )
		{
			/* non-READ commands */
			am53cf96_read_data( n_this * 4, sector_buffer );
		}
		else
		{
			/* assume normal 2048 byte data for now */
			am53cf96_read_data( 2048, sector_buffer );
			n_this = 2048 / 4;
		}
		n_size -= n_this;

		i = 0;
		while( n_this > 0 )
		{
			g_p_n_psxram[ n_address / 4 ] =
				( sector_buffer[ i + 0 ] << 0 ) |
				( sector_buffer[ i + 1 ] << 8 ) |
				( sector_buffer[ i + 2 ] << 16 ) |
				( sector_buffer[ i + 3 ] << 24 );
			n_address += 4;
			i += 4;
			n_this--;
		}
	}
}

static void scsi_dma_write( UINT32 n_address, INT32 n_size )
{
	int i;
	int n_this;

	while( n_size > 0 )
	{
		if( n_size > sizeof( sector_buffer ) / 4 )
		{
			n_this = sizeof( sector_buffer ) / 4;
		}
		else
		{
			n_this = n_size;
		}
		n_size -= n_this;

		i = 0;
		while( n_this > 0 )
		{
			sector_buffer[ i + 0 ] = ( g_p_n_psxram[ n_address / 4 ] >> 0 ) & 0xff;
			sector_buffer[ i + 1 ] = ( g_p_n_psxram[ n_address / 4 ] >> 8 ) & 0xff;
			sector_buffer[ i + 2 ] = ( g_p_n_psxram[ n_address / 4 ] >> 16 ) & 0xff;
			sector_buffer[ i + 3 ] = ( g_p_n_psxram[ n_address / 4 ] >> 24 ) & 0xff;
			n_address += 4;
			i += 4;
			n_this--;
		}

		am53cf96_write_data( n_this * 4, sector_buffer );
	}
}

static void scsi_irq(void)
{
	psx_irq_set(0x400);
}

static SCSIConfigTable dev_table =
{
	1, /* 1 SCSI device */
	{
		{ SCSI_ID_4, 0, SCSI_DEVICE_CDROM } /* SCSI ID 4, using CHD 0, and it's a CD-ROM */
	}
};

static struct AM53CF96interface scsi_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi_irq,		/* command completion IRQ */
};

static DRIVER_INIT( konamigv )
{
	psx_driver_init();

	/* init the scsi controller and hook up it's DMA */
	am53cf96_init(&scsi_intf);
	psx_dma_install_read_handler(5, scsi_dma_read);
	psx_dma_install_write_handler(5, scsi_dma_write);
}

static MACHINE_RESET( konamigv )
{
	psx_machine_init();

	/* also hook up CDDA audio to the CD-ROM drive */
	CDDA_set_cdrom(0, am53cf96_get_device(SCSI_ID_4));
}

static struct PSXSPUinterface konamigv_psxspu_interface =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

static MACHINE_DRIVER_START( konamigv )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( konamigv_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( konamigv )
	MDRV_NVRAM_HANDLER(konamigv_93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( konamigv_psxspu_interface )
	MDRV_SOUND_ROUTE( 0, "left", 0.75 )
	MDRV_SOUND_ROUTE( 1, "right", 0.75 )

	MDRV_SOUND_ADD( CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "left", 1.0 )
	MDRV_SOUND_ROUTE( 1, "right", 1.0 )
MACHINE_DRIVER_END

INPUT_PORTS_START( konamigv )
	/* IN0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

INPUT_PORTS_END

/* Simpsons Bowling */

static NVRAM_HANDLER( simpbowl )
{
	nvram_handler_konamigv_93C46( file, read_or_write );
	nvram_handler_intelflash( 0, file, read_or_write );
	nvram_handler_intelflash( 1, file, read_or_write );
	nvram_handler_intelflash( 2, file, read_or_write );
	nvram_handler_intelflash( 3, file, read_or_write );
}

static int flash_address;

static READ32_HANDLER( flash_r )
{
	int reg = offset*2;
	int shift = 0;

	if (mem_mask == 0x0000ffff)
	{
		reg++;
		shift = 16;
	}

	if (reg == 4)	// set odd address
	{
		flash_address |= 1;
	}

	if (reg == 0)
	{
		int chip = (flash_address >= 0x200000) ? 2 : 0;
		int ret;

		ret = intelflash_read(chip, flash_address & 0x1fffff) & 0xff;
		ret |= intelflash_read(chip+1, flash_address & 0x1fffff)<<8;
		flash_address++;

		return ret;
	}
	return 0;
}

static WRITE32_HANDLER( flash_w )
{
	int reg = offset*2;
	int chip;

	if (mem_mask == 0x0000ffff)
	{
		reg++;
		data>>= 16;
	}

	switch (reg)
	{
		case 0:
			chip = (flash_address >= 0x200000) ? 2 : 0;
			intelflash_write(chip, flash_address & 0x1fffff, data&0xff);
			intelflash_write(chip+1, flash_address & 0x1fffff, (data>>8)&0xff);
			break;

		case 1:
			flash_address = 0;
			flash_address |= (data<<1);
			break;
		case 2:
			flash_address &= 0xff00ff;
			flash_address |= (data<<8);
			break;
		case 3:
			flash_address &= 0x00ffff;
			flash_address |= (data<<15);
			break;
	}
}

static UINT16 trackball_prev[ 2 ];
static UINT32 trackball_data[ 2 ];

static READ32_HANDLER( trackball_r )
{
	if( offset == 0 && mem_mask == 0xffff0000 )
	{
		int axis;
		UINT16 diff;
		UINT16 value;

		for( axis = 0; axis < 2; axis++ )
		{
			value = readinputport( axis + 3 );
			diff = value - trackball_prev[ axis ];
			trackball_prev[ axis ] = value;
			trackball_data[ axis ] = ( ( diff & 0xf00 ) << 16 ) | ( ( diff & 0xff ) << 8 );
		}
	}
	return trackball_data[ offset ];
}

static READ32_HANDLER( unknown_r )
{
	return 0xffffffff;
}

static DRIVER_INIT( simpbowl )
{
	intelflash_init( 0, FLASH_INTEL_28F016S5, NULL );
	intelflash_init( 1, FLASH_INTEL_28F016S5, NULL );
	intelflash_init( 2, FLASH_INTEL_28F016S5, NULL );
	intelflash_init( 3, FLASH_INTEL_28F016S5, NULL );

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, flash_r );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, flash_w );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f6800c0, 0x1f6800c7, 0, 0, trackball_r );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f6800c8, 0x1f6800cb, 0, 0, unknown_r ); /* ?? */

	init_konamigv();
}

static MACHINE_DRIVER_START( simpbowl )
	MDRV_IMPORT_FROM( konamigv )
	MDRV_NVRAM_HANDLER( simpbowl )
MACHINE_DRIVER_END

INPUT_PORTS_START( simpbowl )
	/* IN0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	/* IN3 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_REVERSE PORT_PLAYER(1)

	/* IN4 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(1)

INPUT_PORTS_END

/* Beat the Champ */

static READ32_HANDLER( btcflash_r )
{
	if (mem_mask == 0xffff0000)
	{
		return intelflash_read(0, offset*2);
	}
	else if (mem_mask == 0x0000ffff)
	{
		return intelflash_read(0, (offset*2)+1)<<16;
	}

	return 0;
}

static WRITE32_HANDLER( btcflash_w )
{
	if (mem_mask == 0xffff0000)
	{
		intelflash_write(0, offset*2, data&0xffff);
	}
	else if (mem_mask == 0x0000ffff)
	{
		intelflash_write(0, (offset*2)+1, (data>>16)&0xffff);
	}
}

static UINT16 btc_trackball_prev[ 4 ];
static UINT32 btc_trackball_data[ 4 ];

static READ32_HANDLER( btc_trackball_r )
{
//  printf( "r %08x %08x %08x\n", activecpu_get_pc(), offset, mem_mask );

	if( offset == 1 && mem_mask == 0x0000ffff )
	{
		int axis;
		UINT16 diff;
		UINT16 value;

		for( axis = 0; axis < 4; axis++ )
		{
			value = readinputport( axis + 3 );
			diff = value - btc_trackball_prev[ axis ];
			btc_trackball_prev[ axis ] = value;
			btc_trackball_data[ axis ] = ( ( diff & 0xf00 ) << 16 ) | ( ( diff & 0xff ) << 8 );
		}
	}
	return btc_trackball_data[ offset ] | ( btc_trackball_data[ offset + 2 ] >> 8 );
}

static WRITE32_HANDLER( btc_trackball_w )
{
//  printf( "w %08x %08x %08x %08x\n", activecpu_get_pc(), offset, data, mem_mask );
}

static NVRAM_HANDLER( btchamp )
{
	nvram_handler_konamigv_93C46( file, read_or_write );
	nvram_handler_intelflash( 0, file, read_or_write );
}

static DRIVER_INIT( btchamp )
{
	intelflash_init( 0, FLASH_SHARP_LH28F400, NULL );

	memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, btc_trackball_r );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, btc_trackball_w );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f6800e0, 0x1f6800e3, 0, 0, MWA32_NOP );
	memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0x1f380000, 0x1f3fffff, 0, 0, btcflash_r );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f380000, 0x1f3fffff, 0, 0, btcflash_w );

	init_konamigv();
}

static MACHINE_DRIVER_START( btchamp )
	MDRV_IMPORT_FROM( konamigv )
	MDRV_NVRAM_HANDLER( btchamp )
MACHINE_DRIVER_END

INPUT_PORTS_START( btchamp )
	/* IN0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	/* IN3 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_REVERSE PORT_PLAYER(1)

	/* IN4 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(1)

	/* IN5 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_REVERSE PORT_PLAYER(2)

	/* IN6 */
	PORT_START
	PORT_BIT( 0x7ff, 0x0000, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(2)

INPUT_PORTS_END

/*
Dead Eye

Top board:
    PWB402610
    Xilinx XC3020A
    Xilinx 1718DPC
    74F244N (2 of these)
    LVT245SS (2 of theses)

CD:
    P/N 002715
    054
    UA
    A01
*/

static READ32_HANDLER( kdeadeye_0_r )
{
	return readinputport( 3 );
}

static READ32_HANDLER( kdeadeye_1_r )
{
	return readinputport( 4 );
}

static READ32_HANDLER( kdeadeye_2_r )
{
	return readinputport( 5 );
}

static READ32_HANDLER( kdeadeye_3_r )
{
	return readinputport( 6 );
}

static READ32_HANDLER( kdeadeye_4_r )
{
	return readinputport( 7 );
}

static WRITE32_HANDLER( kdeadeye_0_w )
{
}

static int kdeadeye_crosshair_x( int port )
{
	const int portmin = 0x004c;
	const int portmax = 0x01bb;
	int x = ( readinputport( port ) - portmin );
	x *= ( Machine->visible_area.max_x - Machine->visible_area.min_x );
	x /= ( portmax - portmin );
	return Machine->visible_area.min_x + x;
}

static int kdeadeye_crosshair_y( int port )
{
	const int portmin = 0x0000;
	const int portmax = 0x00ef;
	int y = ( readinputport( port ) - portmin );
	y *= ( Machine->visible_area.max_y - Machine->visible_area.min_y );
	y /= ( portmax - portmin );
	return Machine->visible_area.min_y + y;
}

static VIDEO_UPDATE( kdeadeye )
{
	video_update_psx( screen, bitmap, cliprect );

	draw_crosshair( bitmap, kdeadeye_crosshair_x( 3 ), kdeadeye_crosshair_y( 4 ), cliprect, 0 );
	draw_crosshair( bitmap, kdeadeye_crosshair_x( 5 ), kdeadeye_crosshair_y( 6 ), cliprect, 1 );
}

static DRIVER_INIT( kdeadeye )
{
	intelflash_init( 0, FLASH_SHARP_LH28F400, NULL );

	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f680083, 0, 0, kdeadeye_0_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f680090, 0x1f680093, 0, 0, kdeadeye_1_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f6800a0, 0x1f6800a3, 0, 0, kdeadeye_2_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f6800b0, 0x1f6800b3, 0, 0, kdeadeye_3_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f6800c0, 0x1f6800c3, 0, 0, kdeadeye_4_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f6800e0, 0x1f6800e3, 0, 0, kdeadeye_0_w );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f380000, 0x1f3fffff, 0, 0, btcflash_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f380000, 0x1f3fffff, 0, 0, btcflash_w );

	init_konamigv();
}

static MACHINE_DRIVER_START( kdeadeye )
	MDRV_IMPORT_FROM( konamigv )
	MDRV_VIDEO_UPDATE( kdeadeye )
	MDRV_NVRAM_HANDLER( btchamp )
MACHINE_DRIVER_END

INPUT_PORTS_START( kdeadeye )
	/* IN0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN3 */
	PORT_START
	PORT_BIT( 0xffff, 0x0100, IPT_LIGHTGUN_X ) PORT_MINMAX( 0x004c, 0x01bb ) PORT_SENSITIVITY( 100 ) PORT_KEYDELTA( 5 ) PORT_PLAYER( 1 )

	/* IN4 */
	PORT_START
	PORT_BIT( 0xffff, 0x0077, IPT_LIGHTGUN_Y ) PORT_MINMAX( 0x0000, 0x00ef ) PORT_SENSITIVITY( 100 ) PORT_KEYDELTA( 5 ) PORT_PLAYER( 1 )

	/* IN5 */
	PORT_START
	PORT_BIT( 0xffff, 0x0100, IPT_LIGHTGUN_X ) PORT_MINMAX( 0x004c, 0x01bb ) PORT_SENSITIVITY( 100 ) PORT_KEYDELTA( 5 ) PORT_PLAYER( 2 )

	/* IN6 */
	PORT_START
	PORT_BIT( 0xffff, 0x0077, IPT_LIGHTGUN_Y ) PORT_MINMAX( 0x0000, 0x00ef ) PORT_SENSITIVITY( 100 ) PORT_KEYDELTA( 5 ) PORT_PLAYER( 2 )

	/* IN7 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER( 1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER( 2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

#define GV_BIOS	\
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )	\
	ROM_LOAD( "999a01.7e",   0x0000000, 0x080000, CRC(ad498d2d) SHA1(02a82a2fe1fba0404517c3602324bfa64e23e478) )

ROM_START( konamigv )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, ROMREGION_ERASE00 ) /* default eeprom */
ROM_END

ROM_START( susume )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "susume.25c",   0x000000, 0x000080, CRC(52f17df7) SHA1(b8ad7787b0692713439d7d9bebfa0c801c806006) )
	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "susume", 0, MD5(6645e48204b32d8619b5ba3169f515ab) SHA1(2e83d3a0b45ed740e2ab27d259347b20b87a3eaf) )
ROM_END

ROM_START( hyperath )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "hyperath.25c", 0x000000, 0x000080, CRC(20a8c435) SHA1(a0f203a999757fba68b391c525ac4b9684a57ba9) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "hyperath", 0, MD5(a777d62bc998768e53d3d764d96cd990) SHA1(dfe0a68258cf33ca09639a752611302b361698e8) )
ROM_END

ROM_START( pbball96 )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "pbball96.25c", 0x000000, 0x000080, CRC(405a7fc9) SHA1(e2d978f49748ba3c4a425188abcd3d272ec23907) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "pbball96", 0, MD5(5962a38e2af6299659f53613956cd9ed) SHA1(a056138fc68bd1580b1de89b622b159150413a3f) )
ROM_END

ROM_START( weddingr )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "weddingr.25c", 0x000000, 0x000080, CRC(b90509a0) SHA1(41510a0ceded81dcb26a70eba97636d38d3742c3) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "weddingr", 0, MD5(cacc28156b037e13098f3624ae92ab85) SHA1(e6481c367ad24aa285e51c01221a169b1b74b15f) )
ROM_END

ROM_START( simpbowl )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "simpbowl.25c", 0x000000, 0x000080, CRC(2c61050c) SHA1(16ae7f81cbe841c429c5c7326cf83e87db1782bf) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "simpbowl", 0, MD5(47702fc060f3f1fbb2dba84ea3544a4a) SHA1(791ce11b0645fd5c3f5b30483bad879f26bb97db) )
ROM_END

ROM_START( btchamp )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "btchmp.25c", 0x000000, 0x000080, CRC(6d02ea54) SHA1(d3babf481fd89db3aec17f589d0d3d999a2aa6e1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "btchamp", 0, MD5(edc387207bc878b3a4044441e77d25f7) SHA1(e1a75a034d83cffa44268eb30653ba334cc6252d) )
ROM_END

ROM_START( kdeadeye )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "kdeadeye.25c", 0x000000, 0x000080, CRC(3935d2df) SHA1(cbb855c475269077803c380dbc3621e522efe51e) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "kdeadeye", 0, MD5(5109d61ab8791a6d622499b51e613a8c) SHA1(2b413a2a22e1959fb4f71b67ba51c6c8e0d58970) )
ROM_END

ROM_START( nagano98 )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "nagano98.25c",  0x000000, 0x000080, CRC(b64b7451) SHA1(a77a37e0cc580934d1e7e05d523bae0acd2c1480) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "nagano98", 0, MD5(cbedbd2953b70f214e72179b2cc0dcd8) SHA1(21d14864cdd34c6e052f3577f8d805dce49fcab6) )
ROM_END

/* BIOS placeholder */
GAME( 1995, konamigv, 0, konamigv, konamigv, konamigv, ROT0, "Konami", "Baby Phoenix/GV System", NOT_A_DRIVER )

GAME( 1996, pbball96, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Powerful Baseball '96 (GV017 JAPAN 1.03)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1996, hyperath, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Hyper Athlete (GV021 JAPAN 1.00)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1996, susume,   konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Susume! Taisen Puzzle-Dama (GV027 JAPAN 1.20)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1996, btchamp,  konamigv, btchamp,  btchamp,  btchamp,  ROT0, "Konami", "Beat the Champ (GV053 UAA01)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1996, kdeadeye, konamigv, kdeadeye, kdeadeye, kdeadeye, ROT0, "Konami", "Dead Eye (GV054 UA01)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1997, weddingr, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Wedding Rhapsody (GX624 JAA)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 1998, nagano98, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Nagano Winter Olympics '98 (GX720 EAA)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 2000, simpbowl, konamigv, simpbowl, simpbowl, simpbowl, ROT0, "Konami", "Simpsons Bowling (GQ829 UAA)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
