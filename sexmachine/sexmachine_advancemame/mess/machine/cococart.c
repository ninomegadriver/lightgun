/***************************************************************************

	cococart.c

	Code for emulating CoCo cartridges, like the disk drive cartridge

***************************************************************************/
  
#include "includes/cococart.h"
#include "machine/wd17xx.h"
#include "devices/coco_vhd.h"
#include "ds1315.h"
#include "m6242b.h"
#include "includes/coco.h"
#include "sound/dac.h"

static const struct cartridge_callback *cartcallbacks;

/***************************************************************************
  Floppy disk controller
 ***************************************************************************
 * The CoCo and Dragon both use the Western Digital 1793 floppy disk
 * controller.  The wd1793's variables are mapped to $ff48-$ff4b on the CoCo
 * and on $ff40-$ff43 on the Dragon.  In addition, there is another register
 * called DSKREG that controls the interface with the wd1793.  DSKREG is
 * detailed below:  But they appear to be
 *
 * References:
 *		CoCo:	Disk Basic Unravelled
 *		Dragon:	Inferences from the PC-Dragon source code
 *              DragonDos Controller, Disk and File Formats by Graham E Kinns
 *
 * ---------------------------------------------------------------------------
 * DSKREG - the control register
 * CoCo ($ff40)                                    Dragon ($ff48)
 *
 * Bit                                              Bit
 *	7 halt enable flag                               7 not used
 *	6 drive select #3                                6 not used
 *	5 density (0=single, 1=double)                   5 NMI enable flag
 *    and NMI enable flag
 *	4 write precompensation                          4 write precompensation
 *	3 drive motor activation                         3 single density enable
 *	2 drive select #2                                2 drive motor activation
 *	1 drive select #1                                1 drive select high bit
 *	0 drive select #0                                0 drive select low bit
 *
 * Reading from $ff48-$ff4f clears bit 7
 * of DSKREG ($ff40)
 * ---------------------------------------------------------------------------
 */

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

static int	dskreg;
static void	coco_fdc_callback(int event);
static void	dragon_fdc_callback(int event);
static int	drq_state;
static int	intrq_state;

#define       COCO_HALTENABLE   (dskreg & 0x80)
#define   SET_COCO_HALTENABLE    dskreg &= 0x80
#define CLEAR_COCO_HALTENABLE    dskreg &= ~0x80

#define        COCO_NMIENABLE   (dskreg & 0x20)

#define      DRAGON_NMIENABLE   (dskreg & 0x20)

enum {
	DSK_DMK,
	DSK_BASIC
};

static void coco_fdc_init(const struct cartridge_callback *callbacks)
{
    wd179x_init(WD_TYPE_1773, coco_fdc_callback);
	dskreg = 0;
	cartcallbacks = callbacks;
	drq_state = ASSERT_LINE;
	intrq_state = CLEAR_LINE;
}

static void dragon_fdc_init(const struct cartridge_callback *callbacks)
{
    wd179x_init(WD_TYPE_179X,dragon_fdc_callback);
	dskreg = 0;
	cartcallbacks = callbacks;

}

static void raise_nmi(int dummy)
{
	LOG(("cococart: Raising NMI (and clearing halt), source: wd179x INTRQ\n" ));

	cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
}

static void raise_halt(int dummy)
{
	LOG(("cococart: Raising HALT\n" ));

	coco_set_halt_line(ASSERT_LINE);
}

static void coco_fdc_callback(int event)
{
	switch(event) {
	case WD179X_IRQ_CLR:
		intrq_state = CLEAR_LINE;
		cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
		break;

	case WD179X_IRQ_SET:
		intrq_state = ASSERT_LINE;
		CLEAR_COCO_HALTENABLE;
		coco_set_halt_line(CLEAR_LINE);
		if( COCO_NMIENABLE )
			timer_set( TIME_IN_USEC(0), 0, raise_nmi);
		else
			cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
		break;

	case WD179X_DRQ_CLR:
		drq_state = CLEAR_LINE;
		if( COCO_HALTENABLE )
			timer_set( TIME_IN_CYCLES(7,0), 0, raise_halt);
		else
			coco_set_halt_line(CLEAR_LINE);
		break;
	case WD179X_DRQ_SET:
		drq_state = ASSERT_LINE;
		coco_set_halt_line(CLEAR_LINE);
		break;
	}
}

static void dragon_fdc_callback(int event)
{
	switch(event) {
	case WD179X_IRQ_CLR:
		LOG(("dragon_fdc_callback(): WD179X_IRQ_CLR\n" ));
		cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
		break;
	case WD179X_IRQ_SET:
		LOG(("dragon_fdc_callback(): WD179X_IRQ_SET\n" ));
		if (DRAGON_NMIENABLE)
			timer_set( TIME_IN_USEC(0), 0, raise_nmi);
		break;
	case WD179X_DRQ_CLR:
		LOG(("dragon_fdc_callback(): WD179X_DRQ_CLR\n" ));
		cartcallbacks->setcartline(CARTLINE_CLEAR);
		break;
	case WD179X_DRQ_SET:
		LOG(("dragon_fdc_callback(): WD179X_DRQ_SET\n" ));
		cartcallbacks->setcartline(CARTLINE_ASSERTED);
		break;
	}
}

static void set_coco_dskreg(int data)
{
	UINT8 drive = 0;
	UINT8 head = 0;
	int motor_mask = 0;

	LOG(("set_coco_dskreg(): %c%c%c%c%c%c%c%c ($%02x)\n",
									data & 0x80 ? 'H' : 'h',
									data & 0x40 ? '3' : '.',
									data & 0x20 ? 'D' : 'S',
									data & 0x10 ? 'P' : 'p',
									data & 0x08 ? 'M' : 'm',
									data & 0x04 ? '2' : '.',
									data & 0x02 ? '1' : '.',
									data & 0x01 ? '0' : '.',
								data ));

		/* An email from John Kowalski informed me that if the DS3 is
		 * high, and one of the other drive bits is selected (DS0-DS2), then the
		 * second side of DS0, DS1, or DS2 is selected.  If multiple bits are
		 * selected in other situations, then both drives are selected, and any
		 * read signals get yucky.
		 */

		motor_mask = 0x08;

		if (data & 0x04)
			drive = 2;
		else if (data & 0x02)
			drive = 1;
		else if (data & 0x01)
			drive = 0;
		else if (data & 0x40)
			drive = 3;
		else
			motor_mask = 0;

		head = ((data & 0x40) && (drive != 3)) ? 1 : 0;

	dskreg = data;

	if( COCO_HALTENABLE && (drq_state == CLEAR_LINE) )
		timer_set( TIME_IN_CYCLES(7,0), 0, raise_halt);
	else
		coco_set_halt_line(CLEAR_LINE);

	if( COCO_NMIENABLE  && (intrq_state == ASSERT_LINE) )
	{
		CLEAR_COCO_HALTENABLE;
		coco_set_halt_line(CLEAR_LINE);
		timer_set( TIME_IN_USEC(0), 0, raise_nmi);
	}
	else
		cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);

	wd179x_set_drive(drive);
	wd179x_set_side(head);
	wd179x_set_density( (dskreg & 0x20) ? DEN_MFM_LO : DEN_FM_LO );
}

static void set_dragon_dskreg(int data)
{
	LOG(("set_dragon_dskreg(): %c%c%c%c%c%c%c%c ($%02x)\n",
								data & 0x80 ? 'X' : 'x',
								data & 0x40 ? 'X' : 'x',
								data & 0x20 ? 'N' : 'n',
								data & 0x10 ? 'P' : 'p',
								data & 0x08 ? 'S' : 'D',
								data & 0x04 ? 'M' : 'm',
								data & 0x02 ? '1' : '0',
								data & 0x01 ? '1' : '0',
								data ));

	if (data & 0x04)
		wd179x_set_drive( data & 0x03 );

	wd179x_set_density( (data & 0x08) ? DEN_FM_LO: DEN_MFM_LO );
	dskreg = data;
}

/* ---------------------------------------------------- */

typedef enum
{
	RTC_DISTO	= 0x00,
	RTC_CLOUD9	= 0x01
} rtc_type_t;

static rtc_type_t real_time_clock(void)
{
	return (rtc_type_t) (int) readinputportbytag_safe("real_time_clock", 0x00);
}



/* ---------------------------------------------------- */

 READ8_HANDLER(coco_floppy_r)
{
	int result = 0;

	switch(offset & 0xef) {
	case 8:
		result = wd179x_status_r(0);
		break;
	case 9:
		result = wd179x_track_r(0);
		break;
	case 10:
		result = wd179x_sector_r(0);
		break;
	case 11:
		result = wd179x_data_r(0);
		break;
	default:
		result = coco_vhd_io_r( offset );
		break;
	}

	switch(real_time_clock())
	{
		case RTC_DISTO:
			/* This is the real time clock in Disto's many products */
			if( offset == ( 0xff50-0xff40 ) )
				result = m6242_data_r( 0 );
			break;
			
		case RTC_CLOUD9:
			/* This is the realtime clock in the TC^3 SCSI Controller */
			if( offset == ( 0xff79-0xff40 ) )
				ds1315_r_1( offset );

			if( offset == ( 0xff78-0xff40 ) )
				ds1315_r_0( offset );

			if( offset == ( 0xff7c-0xff40 ) )
				result = ds1315_r_data( offset );
			break;
	}

/*	logerror("SCS read:  address %4.4X, data %2.2X\n", 0xff40+offset, result );*/

	return result;
}

WRITE8_HANDLER(coco_floppy_w)
{
	switch(offset & 0xef) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		set_coco_dskreg(data);
		break;
	case 8:
		wd179x_command_w(0, data);
		break;
	case 9:
		wd179x_track_w(0, data);
		break;
	case 10:
		wd179x_sector_w(0, data);
		break;
	case 11:
		wd179x_data_w(0, data);
		break;
	};

	switch(real_time_clock())
	{
		case RTC_DISTO:
			/* This is the real time clock in Disto's many products */
			if( offset == ( 0xff50-0xff40 ) )
				m6242_data_w( 0, data );

			if( offset == ( 0xff51-0xff40 ) )
				m6242_address_w( 0, data );
			break;
			
		default:
			break;
	}

	coco_vhd_io_w( offset, data );

/*	logerror("SCS write: address %4.4X, data %2.2X\n", 0xff40+offset, data );*/
}

 READ8_HANDLER(dragon_floppy_r)
{
	int result = 0;

	switch(offset & 0xef) {
	case 0:
		result = wd179x_status_r(0);
		break;
	case 1:
		result = wd179x_track_r(0);
		break;
	case 2:
		result = wd179x_sector_r(0);
		break;
	case 3:
		result = wd179x_data_r(0);
		break;
	default:
		break;
	}
	return result;
}

WRITE8_HANDLER(dragon_floppy_w)
{
	switch(offset & 0xef) {
	case 0:
		wd179x_command_w(0, data);

		/* disk head is encoded in the command byte */
		wd179x_set_side((data & 0x02) ? 1 : 0);
		break;
	case 1:
		wd179x_track_w(0, data);
		break;
	case 2:
		wd179x_sector_w(0, data);
		break;
	case 3:
		wd179x_data_w(0, data);
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		set_dragon_dskreg(data);
		break;
	};
}

/* ---------------------------------------------------- */

const struct cartridge_slot cartridge_fdc_coco =
{
	coco_fdc_init,
	NULL,
	coco_floppy_r,
	coco_floppy_w,
	NULL
};

const struct cartridge_slot cartridge_fdc_dragon =
{
	dragon_fdc_init,
	NULL,
	dragon_floppy_r,
	dragon_floppy_w,
	NULL
};

/* ---------------------------------------------------- */

static void cartidge_standard_init(const struct cartridge_callback *callbacks)
{
	cartcallbacks = callbacks;
	cartcallbacks->setcartline(CARTLINE_Q);
}

static WRITE8_HANDLER(cartridge_banks_io_w)
{
/* TJL- trying to turn this into a generic banking call */
	if (offset == 0 )
	{
		cartcallbacks->setbank(data);
		LOG( ("Bankswitch: set bank: %d\n", data ) );
	}
	else
		LOG( ("Bankswitch: Writing to unmapped SCS memory: $%4.4X (PC=$%4.4X)\n", 0xff40+offset, activecpu_get_pc() ) );
}

static  READ8_HANDLER(cartridge_banks_io_r)
{
/* TJL- trying to turn this into a generic banking call */
	if (offset == 0 )
		LOG( ("Bankswitch: reading bank (not implemented\n" ) );
	else
		LOG( ("Bankswitch: Reading from unmapped SCS memory: $%4.4X (PC=$%4.4X)\n", 0xff40+offset, activecpu_get_pc() ) );
		
	return 0;
}

static WRITE8_HANDLER(cartridge_std_io_w)
{
	LOG( ("Standard Cart: Writing to unmapped SCS memory: $%4.4X (PC=$%4.4X)\n", 0xff40+offset, activecpu_get_pc() ) );
}

static  READ8_HANDLER(cartridge_std_io_r)
{
	LOG( ("Standard Cart: Reading from unmapped SCS memory: $%4.4X (PC=$%4.4X)\n", 0xff40+offset, activecpu_get_pc() ) );

	return 0xff;
}

static WRITE8_HANDLER(cartridge_Orch90_io_w)
{
	if( offset == 58 )
		DAC_data_w(0, data);

	if( offset == 59 )
		DAC_data_w(0, data);

}

const struct cartridge_slot cartridge_standard =
{
	cartidge_standard_init,
	NULL,
	cartridge_std_io_r,
	cartridge_std_io_w,
	NULL
};

const struct cartridge_slot cartridge_banks =
{
	cartidge_standard_init,
	NULL,
	cartridge_banks_io_r,
	cartridge_banks_io_w,
	NULL
};

const struct cartridge_slot cartridge_Orch90 =
{
	cartidge_standard_init,
	NULL,
	NULL,
	cartridge_Orch90_io_w,
	NULL
};

/***************************************************************************
  Other hardware to do
****************************************************************************

  IDE - There is an IDE ehancement for the CoCo 3.  Here are some docs on the
  interface (info courtesy: LCB):

		The IDE interface (which is jumpered to be at either at $ff5x or $ff7x
		is mapped thusly:

		$FFx0 - 1st 8 bits of 16 bit IDE DATA register
		$FFx1 - Error (read) / Features (write) register
		$FFx2 - Sector count register
		$FFx3 - Sector # register
		$FFx4 - Low byte of cylinder #
		$FFx5 - Hi byte of cylinder #
		$FFx6 - Device/head register
		$FFx7 - Status (read) / Command (write) register
		$FFx8 - Latch containing the 2nd 8 bits of the 16 bit IDE DATA reg.

		All of these directly map to the IDE standard equivalents. No drivers
		use	the IRQ's... they all use Polled I/O.

***************************************************************************/
