/* from Andrew Scott (ascott@utkux.utcc.utk.edu) */

/*
  updated by BUT
  - corrected music tempo (not confirmed Satan of Saturn and clone)
  - adjusted music freq (except Satan of Saturn and clone)
  - adjusted music waveform
  - support playing flag for music channel 0
  - support HD38880 speech by samples
*/


#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "sound/sn76477.h"
#include "sound/samples.h"

#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif

extern WRITE8_HANDLER( rockola_flipscreen_w );

#define TONE_VOLUME	50
#define CHANNELS	3

#define SAMPLE_RATE	(Machine->sample_rate)
#define FRAC_BITS	16
#define FRAC_ONE	(1 << FRAC_BITS)
#define FRAC_MASK	(FRAC_ONE - 1)

typedef struct tone
{
	int	mute;
	int	offset;
	int	base;
	int	mask;
	INT32	sample_rate;
	INT32	sample_step;
	INT32	sample_cur;
	INT16	form[16];
} TONE;

static TONE tone_channels[CHANNELS];
static INT32 tone_clock_expire;
static INT32 tone_clock;
static sound_stream * tone_stream;

static int Sound0StopOnRollover;
static UINT8 LastPort1;

const char *sasuke_sample_names[] =
{
	"*sasuke",

	// SN76477 and discrete
	"hit.wav",
	"boss_start.wav",
	"shot.wav",
	"boss_attack.wav",

	0
};

const char *vanguard_sample_names[] =
{
	"*vanguard",

	// SN76477 and discrete
	"fire.wav",
	"explsion.wav",

	// HD38880 speech
	"vg_voi-0.wav",
	"vg_voi-1.wav",
	"vg_voi-2.wav",
	"vg_voi-3.wav",
	"vg_voi-4.wav",
	"vg_voi-5.wav",
	"vg_voi-6.wav",
	"vg_voi-7.wav",
	"vg_voi-8.wav",
	"vg_voi-9.wav",
	"vg_voi-a.wav",
	"vg_voi-b.wav",
	"vg_voi-c.wav",
	"vg_voi-d.wav",
	"vg_voi-e.wav",
	"vg_voi-f.wav",

	0
};

const char *fantasy_sample_names[] =
{
	"*fantasy",

	// HD38880 speech
	"ft_voi-0.wav",
	"ft_voi-1.wav",
	"ft_voi-2.wav",
	"ft_voi-3.wav",
	"ft_voi-4.wav",
	"ft_voi-5.wav",
	"ft_voi-6.wav",
	"ft_voi-7.wav",
	"ft_voi-8.wav",
	"ft_voi-9.wav",
	"ft_voi-a.wav",
	"ft_voi-b.wav",

	0
};


INLINE void validate_tone_channel(int channel)
{
	if (!tone_channels[channel].mute)
	{
		UINT8 *ROM = memory_region(REGION_SOUND1);
		UINT8 romdata = ROM[tone_channels[channel].base + tone_channels[channel].offset];

		if (romdata != 0xff)
			tone_channels[channel].sample_step = tone_channels[channel].sample_rate / (256 - romdata);
		else
			tone_channels[channel].sample_step = 0;
	}
}

static void rockola_tone_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int len)
{
	stream_sample_t *buffer = outputs[0];
	int i;

	for (i = 0; i < CHANNELS; i++)
		validate_tone_channel(i);

	while (len-- > 0)
	{
		INT32 data = 0;

		for (i = 0; i < CHANNELS; i++)
		{
			TONE *voice = &tone_channels[i];
			INT16 *form = voice->form;

			if (!voice->mute && voice->sample_step)
			{
				int cur_pos = voice->sample_cur + voice->sample_step;
				int prev = form[(voice->sample_cur >> FRAC_BITS) & 15];
				int cur = form[(cur_pos >> FRAC_BITS) & 15];

				/* interpolate */
				data += ((INT32)prev * (FRAC_ONE - (cur_pos & FRAC_MASK))
				        + (INT32)cur * (cur_pos & FRAC_MASK)) >> FRAC_BITS;

				voice->sample_cur = cur_pos;
			}
		}

		*buffer++ = data;

		tone_clock += FRAC_ONE;
		if (tone_clock >= tone_clock_expire)
		{
			for (i = 0; i < CHANNELS; i++)
			{
				tone_channels[i].offset++;
				tone_channels[i].offset &= tone_channels[i].mask;

				validate_tone_channel(i);
			}

			if (tone_channels[0].offset == 0 && Sound0StopOnRollover)
				tone_channels[0].mute = 1;

			tone_clock -= tone_clock_expire;
		}

	}
}


static void sasuke_build_waveform(int mask)
{
	int bit0, bit1, bit2, bit3;
	int base;
	int i;

	mask &= 7;

	//logerror("0: wave form = %d\n", mask);
	bit0 = bit1 = bit3 = 0;
	bit2 = 1;

	if (mask & 1)
		bit0 = 1;
	if (mask & 2)
		bit1 = 1;
	if (mask & 4)
		bit3 = 1;

	base = (bit0 + bit1 + bit2 + bit3 + 1) / 2;

	for (i = 0; i < 16; i++)
	{
		int data = 0;

		if (i & 1)
			data += bit0;
		if (i & 2)
			data += bit1;
		if (i & 4)
			data += bit2;
		if (i & 8)
			data += bit3;

		//logerror(" %3d\n", data);
		tone_channels[0].form[i] = data - base;
	}

	for (i = 0; i < 16; i++)
		tone_channels[0].form[i] *= 65535 / 16;
}

static void satansat_build_waveform(int mask)
{
	int bit0, bit1, bit2, bit3;
	int base;
	int i;

	mask &= 7;

	//logerror("1: wave form = %d\n", mask);
	bit0 = bit1 = bit2 = 1;
	bit3 = 0;

	if (mask & 1)
		bit3 = 1;

	base = (bit0 + bit1 + bit2 + bit3 + 1) / 2;

	for (i = 0; i < 16; i++)
	{
		int data = 0;

		if (i & 1)
			data += bit0;
		if (i & 2)
			data += bit1;
		if (i & 4)
			data += bit2;
		if (i & 8)
			data += bit3;

		//logerror(" %3d\n", data);
		tone_channels[1].form[i] = data - base;
	}

	for (i = 0; i < 16; i++)
		tone_channels[1].form[i] *= 65535 / 16;
}

static void build_waveform(int channel, int mask)
{
	int bit0, bit1, bit2, bit3;
	int base;
	int i;

	mask &= 15;

	//logerror("%d: wave form = %d\n", channel, mask);
	bit0 = bit1 = bit2 = bit3 = 0;

	// bit 3
	if (mask & (1 | 2))
		bit3 = 8;
	else if (mask & 4)
		bit3 = 4;
	else if (mask & 8)
		bit3 = 2;

	// bit 2
	if (mask & 4)
		bit2 = 8;
	else if (mask & (2 | 8))
		bit2 = 4;

	// bit 1
	if (mask & 8)
		bit1 = 8;
	else if (mask & 4)
		bit1 = 4;
	else if (mask & 2)
		bit1 = 2;

	// bit 0
	bit0 = bit1 / 2;

	if (bit0 + bit1 + bit2 + bit3 < 16)
	{
		bit0 *= 2;
		bit1 *= 2;
		bit2 *= 2;
		bit3 *= 2;
	}

	base = (bit0 + bit1 + bit2 + bit3 + 1) / 2;

	for (i = 0; i < 16; i++)
	{
		/* special channel for fantasy */
		if (channel == 2)
		{
			tone_channels[channel].form[i] = (i & 8) ? 7 : -8;
		}
		else
		{
			int data = 0;

			if (i & 1)
				data += bit0;
			if (i & 2)
				data += bit1;
			if (i & 4)
				data += bit2;
			if (i & 8)
				data += bit3;

			//logerror(" %3d\n", data);
			tone_channels[channel].form[i] = data - base;
		}
	}

	for (i = 0; i < 16; i++)
		tone_channels[channel].form[i] *= 65535 / 160;
}

void rockola_set_music_freq(int freq)
{
	int i;

	for (i = 0; i < CHANNELS; i++)
	{
		tone_channels[i].mute = 1;
		tone_channels[i].offset = 0;
		tone_channels[i].base = i * 0x800;
		tone_channels[i].mask = 0xff;
		tone_channels[i].sample_step = 0;
		tone_channels[i].sample_cur = 0;
		tone_channels[i].sample_rate = (double)(freq * 8) / SAMPLE_RATE * FRAC_ONE;

		build_waveform(i, 1);
	}
}

void rockola_set_music_clock(double clock_time)
{
	tone_clock_expire = clock_time * SAMPLE_RATE * FRAC_ONE;
	tone_clock = 0;
}

void *rockola_sh_start(int clock, const struct CustomSound_interface *config)
{
	// adjusted
	rockola_set_music_freq(43000);

	// 38.99 Hz update (according to schematic)
	rockola_set_music_clock(M_LN2 * (RES_K(18) * 2 + RES_K(1)) * CAP_U(1));

	tone_stream = stream_create(0, 1, SAMPLE_RATE, NULL, rockola_tone_update);

	return auto_malloc(1);
}

int rockola_music0_playing(void)
{
	return tone_channels[0].mute;
}


WRITE8_HANDLER( sasuke_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
            bit description

            0   hit (ic52)
            1   boss start (ic51)
            2   shot
            3   boss attack (ic48?)
            4   ??
            5
            6
            7   reset counter
        */

		if ((~data & 0x01) && (LastPort1 & 0x01))
			sample_start(0, 0, 0);
		if ((~data & 0x02) && (LastPort1 & 0x02))
			sample_start(1, 1, 0);
		if ((~data & 0x04) && (LastPort1 & 0x04))
			sample_start(2, 2, 0);
		if ((~data & 0x08) && (LastPort1 & 0x08))
			sample_start(3, 3, 0);

		if ((data & 0x80) && (~LastPort1 & 0x80))
		{
			tone_channels[0].offset = 0;
			tone_channels[0].mute = 0;
		}

		if ((~data & 0x80) && (LastPort1 & 0x80))
			tone_channels[0].mute = 1;

		LastPort1 = data;
		break;

	case 1:
		/*
            bit description

            0
            1   wave form
            2   wave form
            3   wave form
            4   MUSIC A8
            5   MUSIC A9
            6   MUSIC A10
            7
        */

		/* select tune in ROM based on sound command byte */
		tone_channels[0].base = 0x0000 + ((data & 0x70) << 4);
		tone_channels[0].mask = 0xff;

		Sound0StopOnRollover = 1;

		/* bit 1-3 sound0 waveform control */
		sasuke_build_waveform((data & 0x0e) >> 1);
		break;
	}
}

WRITE8_HANDLER( satansat_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
            bit description

        */

		/* bit 0 = analog sound trigger */

		/* bit 1 = to 76477 */

		/* bit 2 = analog sound trigger */
		if (data & 0x04 && !(LastPort1 & 0x04))
			sample_start(0, 1, 0);

		if (data & 0x08)
		{
			tone_channels[0].mute = 1;
			tone_channels[0].offset = 0;
		}

		/* bit 4-6 sound0 waveform control */
		sasuke_build_waveform((data & 0x70) >> 4);

		/* bit 7 sound1 waveform control */
		satansat_build_waveform((data & 0x80) >> 7);

		LastPort1 = data;
		break;
	case 1:
		/*
            bit description

        */

		/* select tune in ROM based on sound command byte */
		tone_channels[0].base = 0x0000 + ((data & 0x0e) << 7);
		tone_channels[0].mask = 0xff;
		tone_channels[1].base = 0x0800 + ((data & 0x60) << 4);
		tone_channels[1].mask = 0x1ff;

		Sound0StopOnRollover = 1;

		if (data & 0x01)
			tone_channels[0].mute = 0;

		if (data & 0x10)
			tone_channels[1].mute = 0;
		else
		{
			tone_channels[1].mute = 1;
			tone_channels[1].offset = 0;
		}

		/* bit 7 = ? */
		break;
	}
}

WRITE8_HANDLER( vanguard_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
            bit description

            0   MUSIC A10
            1   MUSIC A9
            2   MUSIC A8
            3   LS05 PORT 1
            4   LS04 PORT 2
            5   SHOT A
            6   SHOT B
            7   BOMB
        */

		/* select musical tune in ROM based on sound command byte */
		tone_channels[0].base = ((data & 0x07) << 8);
		tone_channels[0].mask = 0xff;

		Sound0StopOnRollover = 1;

		/* play noise samples requested by sound command byte */
		/* SHOT A */
		if (data & 0x20 && !(LastPort1 & 0x20))
			sample_start(1, 0, 0);
		else if (!(data & 0x20) && LastPort1 & 0x20)
			sample_stop(1);

		/* BOMB */
		if (data & 0x80 && !(LastPort1 & 0x80))
			sample_start(2, 1, 0);

		if (data & 0x08)
		{
			tone_channels[0].mute = 1;
			tone_channels[0].offset = 0;
		}

		if (data & 0x10)
		{
			tone_channels[0].mute = 0;
		}

		/* SHOT B */
		SN76477_enable_w(1, (data & 0x40) ? 0 : 1);

		LastPort1 = data;
		break;
	case 1:
		/*
            bit description

            0   MUSIC A10
            1   MUSIC A9
            2   MUSIC A8
            3   LS04 PORT 3
            4   EXTP A (HD38880 external pitch control A)
            5   EXTP B (HD38880 external pitch control B)
            6
            7
        */

		/* select tune in ROM based on sound command byte */
		tone_channels[1].base = 0x0800 + ((data & 0x07) << 8);
		tone_channels[1].mask = 0xff;

		if (data & 0x08)
			tone_channels[1].mute = 0;
		else
		{
			tone_channels[1].mute = 1;
			tone_channels[1].offset = 0;
		}
		break;
	case 2:
		/*
            bit description

            0   AS 1    (sound0 waveform)
            1   AS 2    (sound0 waveform)
            2   AS 4    (sound0 waveform)
            3   AS 3    (sound0 waveform)
            4   AS 5    (sound1 waveform)
            5   AS 6    (sound1 waveform)
            6   AS 7    (sound1 waveform)
            7   AS 8    (sound1 waveform)
        */

		build_waveform(0, (data & 0x3) | ((data & 4) << 1) | ((data & 8) >> 1));
		build_waveform(1, data >> 4);
	}
}

WRITE8_HANDLER( fantasy_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
            bit description

            0   MUSIC A10
            1   MUSIC A9
            2   MUSIC A8
            3   LS04 PART 1
            4   LS04 PART 2
            5
            6
            7   BOMB
        */

		/* select musical tune in ROM based on sound command byte */
		tone_channels[0].base = 0x0000 + ((data & 0x07) << 8);
		tone_channels[0].mask = 0xff;

		Sound0StopOnRollover = 0;

		if (data & 0x08)
			tone_channels[0].mute = 0;
		else
		{
			tone_channels[0].offset = tone_channels[0].base;
			tone_channels[0].mute = 1;
		}

		if (data & 0x10)
			tone_channels[2].mute = 0;
		else
		{
			tone_channels[2].offset = 0;
			tone_channels[2].mute = 1;
		}

		/* BOMB */
		SN76477_enable_w(0, (data & 0x80) ? 0 : 1);
		/*

            In the real hardware the SN76477 enable line is grounded
            and the sound output is switched on/off by a 4066 IC.

        */

		LastPort1 = data;
		break;
	case 1:
		/*
            bit description

            0   MUSIC A10
            1   MUSIC A9
            2   MUSIC A8
            3   LS04 PART 3
            4   EXT PA (HD38880 external pitch control A)
            5   EXT PB (HD38880 external pitch control B)
            6
            7
        */

		/* select tune in ROM based on sound command byte */
		tone_channels[1].base = 0x0800 + ((data & 0x07) << 8);
		tone_channels[1].mask = 0xff;

		if (data & 0x08)
			tone_channels[1].mute = 0;
		else
		{
			tone_channels[1].mute = 1;
			tone_channels[1].offset = 0;
		}
		break;
	case 2:
		/*
            bit description

            0   AS 1    (sound0 waveform)
            1   AS 3    (sound0 waveform)
            2   AS 2    (sound0 waveform)
            3   AS 4    (sound0 waveform)
            4   AS 5    (sound1 waveform)
            5   AS 6    (sound1 waveform)
            6   AS 7    (sound1 waveform)
            7   AS 8    (sound1 waveform)
        */

		build_waveform(0, (data & 0x9) | ((data & 2) << 1) | ((data & 4) >> 1));
		build_waveform(1, data >> 4);
		break;
	case 3:
		/*
            bit description

            0   BC 1
            1   BC 2
            2   BC 3
            3   MUSIC A10
            4   MUSIC A9
            5   MUSIC A8
            6
            7   INV
        */

		/* select tune in ROM based on sound command byte */
		tone_channels[2].base = 0x1000 + ((data & 0x70) << 4);
		tone_channels[2].mask = 0xff;

		rockola_flipscreen_w(0, data);
		break;
	}
}


/*
  Hitachi HD38880 speech synthesizer chip

    I heard that this chip uses PARCOR coefficients but I don't know ROM data format.
    How do I generate samples?
*/


/* HD38880 command */
#define	HD38880_ADSET	2
#define	HD38880_READ	3
#define	HD38880_INT1	4
#define	HD38880_INT2	6
#define	HD38880_SYSPD	8
#define	HD38880_STOP	10
#define	HD38880_CONDT	11
#define	HD38880_START	12
#define	HD38880_SSTART	14

/* HD38880 control bits */
#define HD38880_CTP	0x10
#define HD38880_CMV	0x20
#define HD68880_SYBS	0x0f

int	hd38880_cmd;
UINT32	hd38880_addr;
int	hd38880_data_bytes;
double	hd38880_speed;


static void rockola_speech_w(UINT8 data, UINT16 *table, int start)
{
	/*
        bit description
        0   SYBS1
        1   SYBS2
        2   SYBS3
        3   SYBS4
        4   CTP
        5   CMV
        6
        7
    */

	if ((data & HD38880_CTP) && (data & HD38880_CMV))
	{
		data &= HD68880_SYBS;

		switch (hd38880_cmd)
		{
		case 0:
			switch (data)
			{
			case HD38880_START:
				logerror("speech: START\n");

				if (hd38880_data_bytes == 5 && !sample_playing(0))
				{
					int i;

					for (i = 0; i < 16; i++)
					{
						if (table[i] && table[i] == hd38880_addr)
						{
							sample_start(0, start + i, 0);
							break;
						}
					}
				}
				break;

			case HD38880_SSTART:
				logerror("speech: SSTART\n");
				break;

			case HD38880_STOP:
				sample_stop(0);
				logerror("speech: STOP\n");
				break;

			case HD38880_SYSPD:
				hd38880_cmd = data;
				break;

			case HD38880_CONDT:
				logerror("speech: CONDT\n");
				break;

			case HD38880_ADSET:
				hd38880_cmd = data;
				hd38880_addr = 0;
				hd38880_data_bytes = 0;
				break;

			case HD38880_READ:
				logerror("speech: READ\n");
				break;

			case HD38880_INT1:
				hd38880_cmd = data;
				break;

			case HD38880_INT2:
				hd38880_cmd = data;
				break;

			case 0:
				// ignore it
				break;

			default:
				logerror("speech: unknown command: 0x%x\n", data);
			}
			break;

		case HD38880_INT1:
			logerror("speech: INT1: 0x%x\n", data);

			if (data & 8)
				logerror("speech:   triangular waveform\n");
			else
				logerror("speech:   impulse waveform\n");

			logerror("speech:   %sable losing effect of vocal tract\n", data & 4 ? "en" : "dis");

			if ((data & 2) && (data & 8))
				logerror("speech:   use external pitch control\n");

			hd38880_cmd = 0;
			break;

		case HD38880_INT2:
			logerror("speech: INT2: 0x%x\n", data);

			logerror("speech:   %d bits / frame\n", data & 8 ? 48 : 96);
			logerror("speech:   %d ms / frame\n", data & 4 ? 20 : 10);
			logerror("speech:   %sable repeat\n", data & 2 ? "en" : "dis");
			logerror("speech:   %d operations\n", ((data & 8) == 0) || (data & 1) ? 10 : 8);

			hd38880_cmd = 0;
			break;

		case HD38880_SYSPD:
			hd38880_speed = ((double)(data + 1)) / 10.0;
			logerror("speech: SYSPD: %1.1f\n", hd38880_speed);
			hd38880_cmd = 0;
			break;

		case HD38880_ADSET:
			hd38880_addr |= (data << (hd38880_data_bytes++ * 4));
			if (hd38880_data_bytes == 5)
			{
				logerror("speech: ADSET: 0x%05x\n", hd38880_addr);
				hd38880_cmd = 0;
			}
			break;
		}
	}
}


/*
 vanguard/fantasy speech

 ROM data format (INT2 = 0xf):
  48 bits / frame
  20 ms / frame
  enable repeat
  10 operations
*/

WRITE8_HANDLER( vanguard_speech_w )
{
	static UINT16 vanguard_table[16] =
	{
		0x04000,
		0x04325,
		0x044a2,
		0x045b7,
		0x046ee,
		0x04838,
		0x04984,
		0x04b01,
		0x04c38,
		0x04de6,
		0x04f43,
		0x05048,
		0x05160,
		0x05289,
		0x0539e,
		0x054ce
	};

	rockola_speech_w(data, vanguard_table, 2);
}

WRITE8_HANDLER( fantasy_speech_w )
{
	static UINT16 fantasy_table[16] =
	{
		0x04000,
		0x04297,
		0x044b6,
		0x04682,
		0x04927,
		0x04be0,
		0x04cc2,
		0x04e36,
		0x05000,
		0x05163,
		0x052c9,
		0x053fd,
		0,
		0,
		0,
		0
	};

	rockola_speech_w(data, fantasy_table, 0);
}
