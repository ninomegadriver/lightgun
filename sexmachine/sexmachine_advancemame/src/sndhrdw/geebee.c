/****************************************************************************
 *
 * geebee.c
 *
 * sound driver
 * juergen buchmueller <pullmoll@t-online.de>, jan 2000
 *
 ****************************************************************************/

#include <math.h>
#include "driver.h"
#include "streams.h"
#include "sound/custom.h"

static void *volume_timer = NULL;
static UINT16 *decay = NULL;
static sound_stream *channel;
static int sound_latch = 0;
static int sound_signal = 0;
static int volume = 0;
static int noise = 0;

static void volume_decay(int param)
{
	if( --volume < 0 )
		volume = 0;
}

WRITE8_HANDLER( geebee_sound_w )
{
	stream_update(channel,0);
	sound_latch = data;
	volume = 0x7fff; /* set volume */
	noise = 0x0000;  /* reset noise shifter */
	/* faster decay enabled? */
	if( sound_latch & 8 )
	{
		/*
         * R24 is 10k, Rb is 0, C57 is 1uF
         * charge time t1 = 0.693 * (R24 + Rb) * C57 -> 0.22176s
         * discharge time t2 = 0.693 * (Rb) * C57 -> 0
         * Then C33 is only charged via D6 (1N914), not discharged!
         * Decay:
         * discharge C33 (1uF) through R50 (22k) -> 0.14058s
         */
		timer_adjust(volume_timer, TIME_IN_HZ(32768/0.14058), 0, TIME_IN_HZ(32768/0.14058));
	}
	else
	{
		/*
         * discharge only after R49 (100k) in the amplifier section,
         * so the volume shouldn't very fast and only when the signal
         * is gated through 6N (4066).
         * I can only guess here that the decay should be slower,
         * maybe half as fast?
         */
		timer_adjust(volume_timer, TIME_IN_HZ(32768/0.2906), 0, TIME_IN_HZ(32768/0.2906));
    }
}

static void geebee_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
    static int vcarry = 0;
    static int vcount = 0;
    stream_sample_t *buffer = outputs[0];

    while (length--)
    {
		*buffer++ = sound_signal;
		/* 1V = HSYNC = 18.432MHz / 3 / 2 / 384 = 8000Hz */
		vcarry -= 18432000 / 3 / 2 / 384;
        while (vcarry < 0)
        {
            vcarry += Machine->sample_rate;
            vcount++;
			/* noise clocked with raising edge of 2V */
			if ((vcount & 3) == 2)
			{
				/* bit0 = bit0 ^ !bit10 */
				if ((noise & 1) == ((noise >> 10) & 1))
					noise = ((noise << 1) & 0xfffe) | 1;
				else
					noise = (noise << 1) & 0xfffe;
			}
            switch (sound_latch & 7)
            {
            case 0: /* 4V */
				sound_signal = (vcount & 0x04) ? decay[volume] : 0;
                break;
            case 1: /* 8V */
				sound_signal = (vcount & 0x08) ? decay[volume] : 0;
                break;
            case 2: /* 16V */
				sound_signal = (vcount & 0x10) ? decay[volume] : 0;
                break;
            case 3: /* 32V */
				sound_signal = (vcount & 0x20) ? decay[volume] : 0;
                break;
            case 4: /* TONE1 */
				sound_signal = !(vcount & 0x01) && !(vcount & 0x10) ? decay[volume] : 0;
                break;
            case 5: /* TONE2 */
				sound_signal = !(vcount & 0x02) && !(vcount & 0x20) ? decay[volume] : 0;
                break;
            case 6: /* TONE3 */
				sound_signal = !(vcount & 0x04) && !(vcount & 0x40) ? decay[volume] : 0;
                break;
			default: /* NOISE */
				/* QH of 74164 #4V */
                sound_signal = (noise & 0x8000) ? decay[volume] : 0;
            }
        }
    }
}

void *geebee_sh_start(int clock, const struct CustomSound_interface *config)
{
	int i;

	decay = (UINT16 *)auto_malloc(32768 * sizeof(INT16));

    for( i = 0; i < 0x8000; i++ )
		decay[0x7fff-i] = (INT16) (0x7fff/exp(1.0*i/4096));

	channel = stream_create(0, 1, Machine->sample_rate, NULL, geebee_sound_update);

	volume_timer = timer_alloc(volume_decay);
    return auto_malloc(1);
}
