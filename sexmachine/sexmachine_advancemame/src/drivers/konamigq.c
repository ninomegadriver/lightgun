/***************************************************************************

  Konami GQ System - Arcade PSX Hardware
  ======================================
  Driver by R. Belmont & smf

  Crypt Killer
  Konami, 1995

  PCB Layout
  ----------

  GQ420  PWB354905B
  |----------------------------------------------------------|
  |CN14     420A01.2G  420A02.3M           CN6  CN7   CN8    |
  |056602    6264                                            |
  |LA4705    6264         058141  424800                     |
  |          68000                                           |
  |  18.432MHz      PAL   056800  424800                     |
  |  32MHz    48MHz PAL           424800                     |
  |  RESET_SW             TMS57002                           |
  |                               M514256                    |
  |                               M514256                    |
  |J  000180                                                 |
  |A                              MACH110                    |
  |M         53.693175MHz         (000619A)                  |
  |M         KM4216V256                                      |
  |A         KM4216V256     CXD8538Q        PAL (000613)     |
  |          KM4216V256                     PAL (000612)     |
  | TEST_SW  KM4216V256                     PAL (000618)     |
  |   CXD2923AR   67.7376MHz                                 |
  | DIPSW(8)                   93C46       NCR 53CF96-2      |
  | KM48V514  KM48V514         420B03.27P    HD_LED          |
  | KM48V514  KM48V514                                       |
  | CN3                CXD8530BQ      TOSHIBA MK1924FBV      |
  | KM48V514  KM48V514                (U420UAA04)            |
  | KM48V514  KM48V514                                       |
  |----------------------------------------------------------|

  Notes:
        CN6, CN7, CN8: For connection of guns.
        CN3 : For connection of extra controls/buttons.
        CN14: For connection of additional speaker for stereo output.
        68000 clock: 8.000MHz
        Vsync: 60Hz
*/

#include "driver.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/konamigx.h"
#include "machine/eeprom.h"
#include "machine/am53cf96.h"
#include "harddisk.h"
#include "sound/k054539.h"

/* Sound */

static UINT8 sndto000[ 16 ];
static UINT8 sndtor3k[ 16 ];

static WRITE32_HANDLER( soundr3k_w )
{
	if( ACCESSING_MSW32 )
	{
		sndto000[ ( offset << 1 ) + 1 ] = data >> 16;
		if( offset == 3 )
		{
			cpunum_set_input_line( 1, 1, HOLD_LINE );
		}
	}
	if( ACCESSING_LSW32 )
	{
		sndto000[ offset << 1 ] = data;
	}
}

static READ32_HANDLER( soundr3k_r )
{
	UINT32 data;

	data = ( sndtor3k[ ( offset << 1 ) + 1 ] << 16 ) | sndtor3k[ offset << 1 ];

	/* hack to help the main program start up */
	if( offset == 1 )
	{
		data = 0;
	}

	return data;
}

/* UART */

static WRITE32_HANDLER( mb89371_w )
{
}

static READ32_HANDLER( mb89371_r )
{
	return 0xffffffff;
}

/* Inputs */

#define GUNX( a ) ( ( readinputport( a ) * 319 ) / 0xff )
#define GUNY( a ) ( ( readinputport( a ) * 239 ) / 0xff )

static READ32_HANDLER( gun1_x_r )
{
	return GUNX( 5 ) + 125;
}

static READ32_HANDLER( gun1_y_r )
{
	return GUNY( 6 );
}

static READ32_HANDLER( gun2_x_r )
{
	return GUNX( 7 ) + 125;
}

static READ32_HANDLER( gun2_y_r )
{
	return GUNY( 8 );
}

static READ32_HANDLER( gun3_x_r )
{
	return GUNX( 9 ) + 125;
}

static READ32_HANDLER( gun3_y_r )
{
	return GUNY( 10 );
}

static READ32_HANDLER( read_inputs_0 )
{
	return ( readinputport( 2 ) << 16 ) | readinputport( 1 );
}

static READ32_HANDLER( read_inputs_1 )
{
	return ( readinputport( 0 ) << 16 ) | readinputport( 3 );
}

/* EEPROM */

static NVRAM_HANDLER( konamigq_93C46 )
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
			static UINT8 def_eeprom[ 128 ] =
			{
				0x29, 0x2b, 0x52, 0x56, 0x20, 0x94, 0x41, 0x55, 0x00, 0x41, 0x14, 0x14, 0x00, 0x03, 0x01, 0x01,
				0x01, 0x03, 0x00, 0x00, 0x07, 0x07, 0x00, 0x01, 0xaa, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
			};

			EEPROM_set_data( def_eeprom, 128 );
		}
	}
}

static WRITE32_HANDLER( eeprom_w )
{
	EEPROM_write_bit( ( data & 0x01 ) ? 1 : 0 );
	EEPROM_set_clock_line( ( data & 0x04 ) ? ASSERT_LINE : CLEAR_LINE );
	EEPROM_set_cs_line( ( data & 0x02 ) ? CLEAR_LINE : ASSERT_LINE );
	cpunum_set_input_line(1, INPUT_LINE_RESET, ( data & 0x40 ) ? CLEAR_LINE : ASSERT_LINE );
}

static READ32_HANDLER( eeprom_r )
{
	return ( EEPROM_read_bit() << 16 ) | readinputport( 4 );
}

/* PCM RAM */

static UINT8 *m_p_n_pcmram;

static WRITE32_HANDLER( pcmram_w )
{
	if( ACCESSING_LSB32 )
	{
		m_p_n_pcmram[ offset << 1 ] = data;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		m_p_n_pcmram[ ( offset << 1 ) + 1 ] = data >> 16;
	}
}

static READ32_HANDLER( pcmram_r )
{
	return ( m_p_n_pcmram[ ( offset << 1 ) + 1 ] << 16 ) | m_p_n_pcmram[ offset << 1 ];
}

/* Video */

static VIDEO_UPDATE( konamigq )
{
	video_update_psx( screen, bitmap, cliprect );

	draw_crosshair( bitmap, GUNX( 5 ), GUNY( 6 ), cliprect, 0 );
	draw_crosshair( bitmap, GUNX( 7 ), GUNY( 8 ), cliprect, 1 );
	draw_crosshair( bitmap, GUNX( 9 ), GUNY( 10 ), cliprect, 2 );
}

static ADDRESS_MAP_START( konamigq_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f00001f) AM_READWRITE(am53cf96_r, am53cf96_w)
	AM_RANGE(0x1f100000, 0x1f10000f) AM_WRITE(soundr3k_w)
	AM_RANGE(0x1f100010, 0x1f10001f) AM_READ(soundr3k_r)
	AM_RANGE(0x1f180000, 0x1f180003) AM_WRITE(eeprom_w)
	AM_RANGE(0x1f198000, 0x1f198003) AM_WRITENOP    		/* cabinet lamps? */
	AM_RANGE(0x1f1a0000, 0x1f1a0003) AM_WRITENOP    		/* indicates gun trigger */
	AM_RANGE(0x1f200000, 0x1f200003) AM_READ(gun1_x_r)
	AM_RANGE(0x1f208000, 0x1f208003) AM_READ(gun1_y_r)
	AM_RANGE(0x1f210000, 0x1f210003) AM_READ(gun2_x_r)
	AM_RANGE(0x1f218000, 0x1f218003) AM_READ(gun2_y_r)
	AM_RANGE(0x1f220000, 0x1f220003) AM_READ(gun3_x_r)
	AM_RANGE(0x1f228000, 0x1f228003) AM_READ(gun3_y_r)
	AM_RANGE(0x1f230000, 0x1f230003) AM_READ(read_inputs_0)
	AM_RANGE(0x1f230004, 0x1f230007) AM_READ(read_inputs_1)
	AM_RANGE(0x1f238000, 0x1f238003) AM_READ(eeprom_r)
	AM_RANGE(0x1f300000, 0x1f5fffff) AM_READWRITE(pcmram_r, pcmram_w)
	AM_RANGE(0x1f680000, 0x1f68001f) AM_READWRITE(mb89371_r, mb89371_w)
	AM_RANGE(0x1f780000, 0x1f780003) AM_WRITENOP /* watchdog? */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READNOP
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_NOP
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

/* SOUND CPU */

static READ16_HANDLER( dual539_r )
{
	UINT16 data;

	data = 0;
	if( ACCESSING_LSB16 )
	{
		data |= K054539_1_r( offset );
	}
	if( ACCESSING_MSB16 )
	{
		data |= K054539_0_r( offset ) << 8;
	}
	return data;
}

static WRITE16_HANDLER( dual539_w )
{
	if( ACCESSING_LSB16 )
	{
		K054539_1_w( offset, data );
	}
	if( ACCESSING_MSB16 )
	{
		K054539_0_w( offset, data >> 8 );
	}
}

static READ16_HANDLER( sndcomm68k_r )
{
	return sndto000[ offset ];
}

static WRITE16_HANDLER( sndcomm68k_w )
{
	sndtor3k[ offset ] = data;
}

/* 68000 memory handling */
static ADDRESS_MAP_START( sndreadmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x2004ff) AM_READ(dual539_r)
	AM_RANGE(0x300000, 0x300001) AM_READ(tms57002_data_word_r)
	AM_RANGE(0x400010, 0x40001f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x500000, 0x500001) AM_READ(tms57002_status_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sndwritemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x2004ff) AM_WRITE(dual539_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(tms57002_data_word_w)
	AM_RANGE(0x400000, 0x40000f) AM_WRITE(sndcomm68k_w)
	AM_RANGE(0x500000, 0x500001) AM_WRITE(tms57002_control_word_w)
	AM_RANGE(0x580000, 0x580001) AM_WRITE(MWA16_NOP) /* ?? */
ADDRESS_MAP_END

static struct K054539interface k054539_interface =
{
	REGION_SOUND1
};

/* SCSI */

static UINT8 sector_buffer[ 512 ];

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
		am53cf96_read_data( n_this * 4, sector_buffer );
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
}

static void scsi_irq(void)
{
	psx_irq_set(0x400);
}

static SCSIConfigTable dev_table =
{
	1, /* 1 SCSI device */
	{
		{ SCSI_ID_0, 0, SCSI_DEVICE_HARDDISK } /* SCSI ID 0, using CHD 0, and it's a HDD */
	}
};

static struct AM53CF96interface scsi_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi_irq,		/* command completion IRQ */
};

static DRIVER_INIT( konamigq )
{
	psx_driver_init();

	m_p_n_pcmram = memory_region( REGION_SOUND1 ) + 0x80000;
}

static MACHINE_START( konamigq )
{
	/* init the scsi controller and hook up it's DMA */
	am53cf96_init(&scsi_intf);
	psx_dma_install_read_handler(5, scsi_dma_read);
	psx_dma_install_write_handler(5, scsi_dma_write);

	state_save_register_global_pointer(m_p_n_pcmram, 0x380000);
	state_save_register_global_array(sndto000);
	state_save_register_global_array(sndtor3k);
	state_save_register_global_array(sector_buffer);
	return 0;
}

static MACHINE_RESET( konamigq )
{
	psx_machine_init();
	tms57002_init();
}

static MACHINE_DRIVER_START( konamigq )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( konamigq_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD_TAG( "sound", M68000, 8000000 )
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP( sndreadmem, sndwritemem )
	MDRV_CPU_PERIODIC_INT( irq2_line_hold, TIME_IN_HZ(480) )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_START( konamigq )
	MDRV_MACHINE_RESET( konamigq )
	MDRV_NVRAM_HANDLER( konamigq_93C46 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1 )
	MDRV_VIDEO_UPDATE( konamigq )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

INPUT_PORTS_START( konamigq )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 4 */
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Stereo ))
	PORT_DIPSETTING(    0x01, DEF_STR( Stereo ))
	PORT_DIPSETTING(    0x00, DEF_STR( Mono ))
	PORT_DIPNAME( 0x02, 0x00, "Stage Set" )
	PORT_DIPSETTING(    0x02, "Endless" )
	PORT_DIPSETTING(    0x00, "6st End" )
	PORT_DIPNAME( 0x04, 0x04, "Mirror" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Woofer" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Number of Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism (2p only)" )
	PORT_DIPSETTING(    0x20, "Common" )
	PORT_DIPSETTING(    0x00, "Independent" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 5 */
	PORT_START /* mask default type                     sens delta min max */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	/* IN 6 */
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	/* IN 7 */
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	/* IN 8 */
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	/* IN 9 */
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(3)

	/* IN 10 */
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(3)
INPUT_PORTS_END

ROM_START( cryptklr )
	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* 68000 sound program */
	ROM_LOAD16_WORD_SWAP( "420a01.2g", 0x000000, 0x080000, CRC(84fc2613) SHA1(e06f4284614d33c76529eb43b168d095200a9eac) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "420a02.3m",    0x000000, 0x080000, CRC(2169c3c4) SHA1(6d525f10385791e19eb1897d18f0bab319640162) )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) /* bios */
	ROM_LOAD( "420b03.27p",   0x0000000, 0x080000, CRC(aab391b1) SHA1(bf9dc7c0c8168c22a4be266fe6a66d3738df916b) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "420uaa04", 0, MD5(179464886f58a2e14b284e3813227a86) SHA1(18fe867c44982bacf0d3ff8453487cd06425a6b7) )
ROM_END

GAME( 1995, cryptklr, 0, konamigq, konamigq, konamigq, ROT0, "Konami", "Crypt Killer (GQ420 UAA)", GAME_IMPERFECT_GRAPHICS )
