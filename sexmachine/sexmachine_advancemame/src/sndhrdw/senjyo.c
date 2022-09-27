#include "driver.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "sound/samples.h"
#include <math.h>


/* z80 pio */
static void pio_interrupt(int state)
{
	cpunum_set_input_line(1, 0, state);
}

static z80pio_interface pio_intf =
{
	pio_interrupt,
	0,
	0
};

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpunum_set_input_line(1, 0, state);
}

static z80ctc_interface ctc_intf =
{
	0,               /* clock (filled in from the CPU 0 clock */
	NOTIMER_2,       /* timer disables */
	ctc_interrupt,   /* interrupt handler */
	z80ctc_0_trg1_w, /* ZC/TO0 callback */
	0,               /* ZC/TO1 callback */
	0                /* ZC/TO2 callback */
};


/* single tone generator */
#define SINGLE_LENGTH 10000
#define SINGLE_DIVIDER 8

static INT16 *_single;
static int single_rate = 1000;
static int single_volume = 0;


WRITE8_HANDLER( senjyo_volume_w )
{
	single_volume = data & 0x0f;
	sample_set_volume(0,single_volume / 15.0);
}


static void senjyo_sh_update(int param)
{
	double period;

	/* ctc2 timer single tone generator frequency */
	period = z80ctc_getperiod (0, 2);
	if( period != 0 ) single_rate = (int)(1.0 / period );
	else single_rate = 0;

	sample_set_freq(0,single_rate);
}


void senjyo_sh_start(void)
{
    int i;

	/* z80 ctc init */
	ctc_intf.baseclock = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (0, &ctc_intf);

	/* z80 pio init */
	z80pio_init (0, &pio_intf);

	_single = (INT16 *)auto_malloc(SINGLE_LENGTH*2);

	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		_single[i] = ((i/SINGLE_DIVIDER)&0x01)*127*256;

	/* CTC2 single tone generator */
	sample_set_volume(0,0);
	sample_start_raw(0,_single,SINGLE_LENGTH,single_rate,1);

	timer_pulse(TIME_IN_HZ(Machine->refresh_rate), 0, senjyo_sh_update);
}
