/*
  fairchild f3853 smi static ram interface
  with integrated interrupt controller and timer

  databook found at www.freetradezone.com
*/

#include "f3853.h"
#include "driver.h"
#include "timer.h"

/*
  the smi does not have DC0 and DC1, only DC0
  it is not reacting to the cpus DC0/DC1 swap instruction
  --> might lead to 2 devices having reacting to the same DC0 address
  and placing their bytes to the databus!
*/

/*
   8 bit shift register
   feedback in0 = not ( (out3 xor out4) xor (out5 xor out7) )
   interrupt at 0xfe
   0xff stops register (0xfe never reached!)
*/
static UINT8 f3853_value_to_cycle[0x100];

static struct {
    F3853_CONFIG config;

    UINT8 high,low; // bit 7 set to 0 for timer interrupt, to 1 for external interrupt
    bool external_enable;
    bool timer_enable;

    bool request_flipflop;

    bool priority_line; /* inverted level*/
    bool external_interrupt_line;/* inverted level */

    void *timer;
} f3853= { { 0 } };
#define INTERRUPT_VECTOR(external) (external?f3853.low|(f3853.high<<8)|0x80 \
					:(f3853.low|(f3853.high<<8))&~0x80)

static void f3853_set_interrupt_request_line(void)
{
    if (!f3853.config.interrupt_request)
		return;

	if (f3853.external_enable&&!f3853.priority_line)
		f3853.config.interrupt_request(INTERRUPT_VECTOR(TRUE), TRUE);
	else if (f3853.timer_enable&&!f3853.priority_line&&f3853.request_flipflop)
		f3853.config.interrupt_request(INTERRUPT_VECTOR(FALSE), TRUE);
	else
		f3853.config.interrupt_request(0, FALSE);
}

static void f3853_timer_callback(int param);

static void f3853_timer_start(UINT8 value)
{
	timer_reset(f3853.timer, 
		(value != 0xff) ?
			f3853_value_to_cycle[value]*31/(double)f3853.config.frequency :
			TIME_NEVER);
}

static void f3853_timer_callback(int param)
{
    if (f3853.timer_enable)
	{
		f3853.request_flipflop = TRUE;
		f3853_set_interrupt_request_line();
    }
    f3853_timer_start(0xfe);
}


void f3853_init(F3853_CONFIG *config)
{
	UINT8 reg=0xfe;
	int i;

	for (i=254/*known to get 0xfe after 255 cycles*/; i>=0; i--)
	{
		bool o7=reg&0x80?TRUE:FALSE, o5=reg&0x20?TRUE:FALSE,
			o4=reg&0x10?TRUE:FALSE, o3=reg&8?TRUE:FALSE;
		f3853_value_to_cycle[reg]=i;
		reg<<=1;
		if (!((o7!=o5)!=(o4!=o3))) reg|=1;
	}

	f3853.config=*config;

	f3853.priority_line=FALSE;
	f3853.external_interrupt_line=TRUE;
	f3853.timer = timer_alloc(f3853_timer_callback);
}

void f3853_reset(void)
{
    // registers indeterminate
}

void f3853_set_external_interrupt_in_line(bool level)
{
    if (f3853.external_interrupt_line&&!level&& f3853.external_enable)
	f3853.request_flipflop = TRUE;
    f3853.external_interrupt_line = level;
    f3853_set_interrupt_request_line();
}

void f3853_set_priority_in_line(bool level)
{
    f3853.priority_line=level;
    f3853_set_interrupt_request_line();
}

 READ8_HANDLER(f3853_r)
{
    UINT8 data=0;
    switch (offset) {
    case 0:
		data=f3853.high;
		break;
    case 1:
		data=f3853.low;
		break;
    case 2: // interrupt control; not readable
    case 3: // timer; not readable
		break;
    }
    return data;
}

WRITE8_HANDLER(f3853_w)
{
	switch (offset) {
	case 0:
		f3853.high=data;
		break;
	case 1:
		f3853.low=data;
		break;
	case 2: //interrupt control
		f3853.external_enable = ((data&3)==1);
		f3853.timer_enable = ((data&3)==3);
		f3853_set_interrupt_request_line();
		break;
	case 3: //timer
		f3853.request_flipflop=FALSE;
		f3853_set_interrupt_request_line();
		f3853_timer_start(data);
		break;
	}
}
