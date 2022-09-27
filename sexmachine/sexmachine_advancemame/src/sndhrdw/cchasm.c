/***************************************************************************

    Cinematronics Cosmic Chasm hardware

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "cchasm.h"
#include "sound/ay8910.h"

static int sound_flags;

READ8_HANDLER( cchasm_snd_io_r )
{
    int coin;

    switch (offset & 0x61 )
    {
    case 0x00:
        coin = (input_port_3_r (offset) >> 4) & 0x7;
        if (coin != 0x7) coin |= 0x8;
        return sound_flags | coin;

    case 0x01:
        return AY8910_read_port_0_r (offset);

    case 0x21:
        return AY8910_read_port_1_r (offset);

    case 0x40:
        return soundlatch_r (offset);

    case 0x41:
        sound_flags &= ~0x80;
        z80ctc_0_trg2_w (0, 0);
        return soundlatch2_r (offset);
    default:
        logerror("Read from unmapped internal IO device at 0x%x\n", offset + 0x6000);
        return 0;
    }
}

WRITE8_HANDLER( cchasm_snd_io_w )
{
    switch (offset & 0x61 )
    {
    case 0x00:
        AY8910_control_port_0_w (offset, data);
        break;

    case 0x01:
        AY8910_write_port_0_w (offset, data);
        break;

    case 0x20:
        AY8910_control_port_1_w (offset, data);
        break;

    case 0x21:
        AY8910_write_port_1_w (offset, data);
        break;

    case 0x40:
        soundlatch3_w (offset, data);
        break;

    case 0x41:
        sound_flags |= 0x40;
        soundlatch4_w (offset, data);
        cpunum_set_input_line(0, 1, HOLD_LINE);
        break;

    case 0x61:
        z80ctc_0_trg0_w (0, 0);
        break;

    default:
        logerror("Write %x to unmapped internal IO device at 0x%x\n", data, offset + 0x6000);
    }
}

WRITE16_HANDLER( cchasm_io_w )
{
    static int led;

	if (ACCESSING_MSB)
	{
		data >>= 8;
		switch (offset & 0xf)
		{
		case 0:
			soundlatch_w (offset, data);
			break;
		case 1:
			sound_flags |= 0x80;
			soundlatch2_w (offset, data);
			z80ctc_0_trg2_w (0, 1);
			cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
			break;
		case 2:
			led = data;
			break;
		}
	}
}

READ16_HANDLER( cchasm_io_r )
{
	switch (offset & 0xf)
	{
	case 0x0:
		return soundlatch3_r (offset) << 8;
	case 0x1:
		sound_flags &= ~0x40;
		return soundlatch4_r (offset) << 8;
	case 0x2:
		return (sound_flags| (input_port_3_r (offset) & 0x07) | 0x08) << 8;
	case 0x5:
		return input_port_2_r (offset) << 8;
	case 0x8:
		return input_port_1_r (offset) << 8;
	default:
		return 0xff << 8;
	}
}

static sound_stream * channel[2];
static int channel_active[2];
static int output[2];

static void ctc_interrupt (int state)
{
	cpunum_set_input_line(1, 0, state);
}

static WRITE8_HANDLER( ctc_timer_1_w )
{

    if (data) /* rising edge */
    {
        output[0] ^= 0x7f;
        channel_active[0] = 1;
        stream_update(channel[0], 0);
    }
}

static WRITE8_HANDLER( ctc_timer_2_w )
{

    if (data) /* rising edge */
    {
        output[1] ^= 0x7f;
        channel_active[1] = 1;
        stream_update(channel[1], 0);
    }
}

static z80ctc_interface ctc_intf =
{
	0,               /* clock (filled in from the CPU 0 clock */
	0,               /* timer disables */
	ctc_interrupt,   /* interrupt handler */
	0,               /* ZC/TO0 callback */
	ctc_timer_1_w,     /* ZC/TO1 callback */
	ctc_timer_2_w      /* ZC/TO2 callback */
};

static void tone_update(void *param,stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	stream_sample_t *buffer = outputs[0];
	int num = (int)param;
	INT16 out = 0;

	if (channel_active[num])
		out = output[num] << 8;

	while (length--) *(buffer++) = out;
	channel_active[num] = 0;
}

void cchasm_sh_update(int param)
{
    if ((input_port_3_r (0) & 0x70) != 0x70)
        z80ctc_0_trg0_w (0, 1);
}

void *cchasm_sh_start(int clock, const struct CustomSound_interface *config)
{
    sound_flags = 0;
    output[0] = 0; output[1] = 0;

    channel[0] = stream_create(0, 1, Machine->sample_rate, (void *)0, tone_update);
    channel[1] = stream_create(0, 1, Machine->sample_rate, (void *)1, tone_update);

	ctc_intf.baseclock = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (0, &ctc_intf);

	timer_pulse(TIME_IN_HZ(Machine->drv->frames_per_second), 0, cchasm_sh_update);

	return auto_malloc(1);
}


