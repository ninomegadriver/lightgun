/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

****************************************************************************/

#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "vidhrdw/generic.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "devices/cassette.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "image.h"

static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;
static void pio_interrupt(int state);
static void mbee_fdc_callback(int);

static z80pio_interface pio_intf =
{
	pio_interrupt,	/* callback when change interrupt status */
	0,				/* portA ready active callback (do not support yet)*/
	0				/* portB ready active callback (do not support yet)*/
};

static void pio_interrupt(int state)
{
	cpunum_set_input_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

MACHINE_RESET( mbee )
{
	z80pio_init(0, &pio_intf);
    wd179x_init(WD_TYPE_179X,mbee_fdc_callback);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* PIO B data bits
 * 0	cassette data (input)
 * 1	cassette data (output)
 * 2	rs232 clock or DTR line
 * 3	rs232 CTS line (0: clear to send)
 * 4	rs232 input (0: mark)
 * 5	rs232 output (1: mark)
 * 6	speaker
 * 7	network interrupt
 */
READ8_HANDLER ( mbee_pio_r )
{
    UINT8 data = (offset & 2) ? z80pio_c_r(0, offset & 1) : z80pio_d_r(0, offset & 1);
	if( offset != 2 )
		return data;

    data |= 0x01;
	if (cassette_input(cassette_device_image()) > 0.03)
		data &= ~0x01;

    return data;
}

WRITE8_HANDLER ( mbee_pio_w )
{
	if (offset & 2)
		z80pio_c_w(0, offset & 1, data);
	else
		z80pio_d_w(0, offset & 1, data);

	if( offset == 2 )
	{
		cassette_output(cassette_device_image(), (data & 0x02) ? -1.0 : +1.0);
		speaker_level_w(0, (data >> 6) & 1);
	}
}

static void mbee_fdc_callback(int param)
{
	switch( param )
	{
	case WD179X_IRQ_CLR:
//		cpunum_set_input_line(0,0,CLEAR_LINE);
		fdc_status &= ~0x40;
        break;
	case WD179X_IRQ_SET:
//		cpunum_set_input_line(0,0,HOLD_LINE);
		fdc_status |= 0x40;
        break;
	case WD179X_DRQ_CLR:
		fdc_status &= ~0x80;
		break;
	case WD179X_DRQ_SET:
		fdc_status |= 0x80;
        break;
    }
}

 READ8_HANDLER ( mbee_fdc_status_r )
{
	logerror("mbee fdc_motor_r $%02X\n", fdc_status);
	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
	logerror("mbee fdc_motor_w $%02X\n", data);
	/* Controller latch bits
	 * 0-1	driver select
	 * 2	head select
	 * 3	density (0: FM, 1:MFM)
	 * 4-7	unused
	 */
	fdc_drv = data & 3;
	fdc_head = (data >> 2) & 1;
	fdc_den = (data >> 3) & 1;
	wd179x_set_drive(fdc_drv);
	wd179x_set_side(fdc_head);
	if (data & (1<<3))
	{
	   wd179x_set_density(DEN_FM_HI);
	}
	else
	{
	   wd179x_set_density(DEN_MFM_LO);
	}

}

void mbee_interrupt(void)
{
    /* once per frame, pulse the PIO B bit 7 */
    logerror("mbee interrupt\n");
	z80pio_p_w(0, 1, 0x80);
    z80pio_p_w(0, 1, 0x00);
}

DEVICE_LOAD( mbee_cart )
{
	int size = mame_fsize(file);
	UINT8 *mem = malloc(size);
	if( mem )
	{
		if( mame_fread(file, mem, size) == size )
		{
			memcpy(memory_region(REGION_CPU1)+0x8000, mem, size);
		}
		free(mem);
	}
	return INIT_PASS;
}
