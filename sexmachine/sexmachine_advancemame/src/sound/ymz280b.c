/**********************************************************************************************
 *
 *   Yamaha YMZ280B driver
 *   by Aaron Giles
 *
 **********************************************************************************************/


#include <math.h>

#include "sndintrf.h"
#include "streams.h"
#include "ymz280b.h"


#define MAX_SAMPLE_CHUNK	10000
#define MAKE_WAVS			0

#define FRAC_BITS			14
#define FRAC_ONE			(1 << FRAC_BITS)
#define FRAC_MASK			(FRAC_ONE - 1)

#define INTERNAL_BUFFER_SIZE 	(1 << 15)
#define INTERNAL_SAMPLE_RATE 	(chip->master_clock * 2.0)

#if MAKE_WAVS
#include "wavwrite.h"
#endif


/* struct describing a single playing ADPCM voice */
struct YMZ280BVoice
{
	UINT8 playing;			/* 1 if we are actively playing */

	UINT8 keyon;			/* 1 if the key is on */
	UINT8 looping;			/* 1 if looping is enabled */
	UINT8 mode;				/* current playback mode */
	UINT16 fnum;			/* frequency */
	UINT8 level;			/* output level */
	UINT8 pan;				/* panning */

	UINT32 start;			/* start address, in nibbles */
	UINT32 stop;			/* stop address, in nibbles */
	UINT32 loop_start;		/* loop start address, in nibbles */
	UINT32 loop_end;		/* loop end address, in nibbles */
	UINT32 position;		/* current position, in nibbles */

	INT32 signal;			/* current ADPCM signal */
	INT32 step;				/* current ADPCM step */

	INT32 loop_signal;		/* signal at loop start */
	INT32 loop_step;		/* step at loop start */
	UINT32 loop_count;		/* number of loops so far */

	INT32 output_left;		/* output volume (left) */
	INT32 output_right;		/* output volume (right) */
	INT32 output_step;		/* step value for frequency conversion */
	INT32 output_pos;		/* current fractional position */
	INT16 last_sample;		/* last sample output */
	INT16 curr_sample;		/* current sample target */
	UINT8 irq_schedule;		/* 1 if the IRQ state is updated by timer */
};

struct YMZ280BChip
{
	sound_stream * stream;			/* which stream are we using */
	UINT8 *region_base;				/* pointer to the base of the region */
	UINT8 current_register;			/* currently accessible register */
	UINT8 status_register;			/* current status register */
	UINT8 irq_state;				/* current IRQ state */
	UINT8 irq_mask;					/* current IRQ mask */
	UINT8 irq_enable;				/* current IRQ enable */
	UINT8 keyon_enable;				/* key on enable */
	double master_clock;			/* master clock frequency */
	void (*irq_callback)(int);		/* IRQ callback */
	struct YMZ280BVoice	voice[8];	/* the 8 voices */
	UINT32 rom_readback_addr;		/* where the CPU can read the ROM */
	read8_handler ext_ram_read;		/* external RAM read handler */
	write8_handler ext_ram_write;	/* external RAM write handler */

#if MAKE_WAVS
	void *		wavresample;			/* resampled waveform */
#endif

	INT16 *scratch;
};

/* step size index shift table */
static const int index_scale[8] = { 0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266 };

/* lookup table for the precomputed difference */
static int diff_lookup[16];

/* timer callback */
static void update_irq_state_timer_0(void *param);
static void update_irq_state_timer_1(void *param);
static void update_irq_state_timer_2(void *param);
static void update_irq_state_timer_3(void *param);
static void update_irq_state_timer_4(void *param);
static void update_irq_state_timer_5(void *param);
static void update_irq_state_timer_6(void *param);
static void update_irq_state_timer_7(void *param);

static void (*update_irq_state_cb[])(void *) =
{
	update_irq_state_timer_0,
	update_irq_state_timer_1,
	update_irq_state_timer_2,
	update_irq_state_timer_3,
	update_irq_state_timer_4,
	update_irq_state_timer_5,
	update_irq_state_timer_6,
	update_irq_state_timer_7
};



INLINE void update_irq_state(struct YMZ280BChip *chip)
{
	int irq_bits = chip->status_register & chip->irq_mask;

	/* always off if the enable is off */
	if (!chip->irq_enable)
		irq_bits = 0;

	/* update the state if changed */
	if (irq_bits && !chip->irq_state)
	{
		chip->irq_state = 1;
		if (chip->irq_callback)
			(*chip->irq_callback)(1);
		else logerror("YMZ280B: IRQ generated, but no callback specified!");
	}
	else if (!irq_bits && chip->irq_state)
	{
		chip->irq_state = 0;
		if (chip->irq_callback)
			(*chip->irq_callback)(0);
		else logerror("YMZ280B: IRQ generated, but no callback specified!");
	}
}


INLINE void update_step(struct YMZ280BChip *chip, struct YMZ280BVoice *voice)
{
	double frequency;

	/* compute the frequency */
	if (voice->mode == 1)
		frequency = chip->master_clock * (double)((voice->fnum & 0x0ff) + 1) * (1.0 / 256.0);
	else
		frequency = chip->master_clock * (double)((voice->fnum & 0x1ff) + 1) * (1.0 / 256.0);
	voice->output_step = (UINT32)(frequency * (double)FRAC_ONE / INTERNAL_SAMPLE_RATE);
}


INLINE void update_volumes(struct YMZ280BVoice *voice)
{
	if (voice->pan == 8)
	{
		voice->output_left = voice->level;
		voice->output_right = voice->level;
	}
	else if (voice->pan < 8)
	{
		voice->output_left = voice->level;
		voice->output_right = voice->level * voice->pan / 8;
	}
	else
	{
		voice->output_left = voice->level * (15 - voice->pan) / 8;
		voice->output_right = voice->level;
	}
}


static void YMZ280B_state_save_update_step(void *param)
{
	struct YMZ280BChip *chip = param;
	int j;
	for (j = 0; j < 8; j++)
	{
		struct YMZ280BVoice *voice = &chip->voice[j];
		update_step(chip, voice);
		if(voice->irq_schedule)
			timer_set_ptr(0, chip, update_irq_state_cb[j]);
	}
}


static void update_irq_state_timer_common(void *param, int voicenum)
{
	struct YMZ280BChip *chip = param;
	struct YMZ280BVoice *voice = &chip->voice[voicenum];

	if(!voice->irq_schedule) return;

	voice->playing = 0;
	chip->status_register |= 1 << voicenum;
	update_irq_state(chip);
	voice->irq_schedule = 0;
}

static void update_irq_state_timer_0(void *param) { update_irq_state_timer_common(param, 0); }
static void update_irq_state_timer_1(void *param) { update_irq_state_timer_common(param, 1); }
static void update_irq_state_timer_2(void *param) { update_irq_state_timer_common(param, 2); }
static void update_irq_state_timer_3(void *param) { update_irq_state_timer_common(param, 3); }
static void update_irq_state_timer_4(void *param) { update_irq_state_timer_common(param, 4); }
static void update_irq_state_timer_5(void *param) { update_irq_state_timer_common(param, 5); }
static void update_irq_state_timer_6(void *param) { update_irq_state_timer_common(param, 6); }
static void update_irq_state_timer_7(void *param) { update_irq_state_timer_common(param, 7); }


/**********************************************************************************************

     compute_tables -- compute the difference tables

***********************************************************************************************/

static void compute_tables(void)
{
	int nib;

	/* loop over all nibbles and compute the difference */
	for (nib = 0; nib < 16; nib++)
	{
		int value = (nib & 0x07) * 2 + 1;
		diff_lookup[nib] = (nib & 0x08) ? -value : value;
	}
}



/**********************************************************************************************

     generate_adpcm -- general ADPCM decoding routine

***********************************************************************************************/

static int generate_adpcm(struct YMZ280BVoice *voice, UINT8 *base, INT16 *buffer, int samples)
{
	int position = voice->position;
	int signal = voice->signal;
	int step = voice->step;
	int val;

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			val = base[position / 2] >> ((~position & 1) << 2);
			signal += (step * diff_lookup[val & 15]) / 8;

			/* clamp to the maximum */
			if (signal > 32767)
				signal = 32767;
			else if (signal < -32768)
				signal = -32768;

			/* adjust the step size and clamp */
			step = (step * index_scale[val & 7]) >> 8;
			if (step > 0x6000)
				step = 0x6000;
			else if (step < 0x7f)
				step = 0x7f;

			/* output to the buffer, scaling by the volume */
			*buffer++ = signal;
			samples--;

			/* next! */
			position++;
			if (position >= voice->stop)
				break;
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			val = base[position / 2] >> ((~position & 1) << 2);
			signal += (step * diff_lookup[val & 15]) / 8;

			/* clamp to the maximum */
			if (signal > 32767)
				signal = 32767;
			else if (signal < -32768)
				signal = -32768;

			/* adjust the step size and clamp */
			step = (step * index_scale[val & 7]) >> 8;
			if (step > 0x6000)
				step = 0x6000;
			else if (step < 0x7f)
				step = 0x7f;

			/* output to the buffer, scaling by the volume */
			*buffer++ = signal;
			samples--;

			/* next! */
			position++;
			if (position == voice->loop_start && voice->loop_count == 0)
			{
				voice->loop_signal = signal;
				voice->loop_step = step;
			}
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
				{
					position = voice->loop_start;
					signal = voice->loop_signal;
					step = voice->loop_step;
					voice->loop_count++;
				}
			}
			if (position >= voice->stop)
				break;
		}
	}

	/* update the parameters */
	voice->position = position;
	voice->signal = signal;
	voice->step = step;

	return samples;
}



/**********************************************************************************************

     generate_pcm8 -- general 8-bit PCM decoding routine

***********************************************************************************************/

static int generate_pcm8(struct YMZ280BVoice *voice, UINT8 *base, INT16 *buffer, int samples)
{
	int position = voice->position;
	int val;

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			val = base[position / 2];

			/* output to the buffer, scaling by the volume */
			*buffer++ = (INT8)val * 256;
			samples--;

			/* next! */
			position += 2;
			if (position >= voice->stop)
				break;
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			val = base[position / 2];

			/* output to the buffer, scaling by the volume */
			*buffer++ = (INT8)val * 256;
			samples--;

			/* next! */
			position += 2;
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
					position = voice->loop_start;
			}
			if (position >= voice->stop)
				break;
		}
	}

	/* update the parameters */
	voice->position = position;

	return samples;
}



/**********************************************************************************************

     generate_pcm16 -- general 16-bit PCM decoding routine

***********************************************************************************************/

static int generate_pcm16(struct YMZ280BVoice *voice, UINT8 *base, INT16 *buffer, int samples)
{
	int position = voice->position;
	int val;

	/* two cases: first cases is non-looping */
	if (!voice->looping)
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			val = (INT16)((base[position / 2 + 1] << 8) + base[position / 2]);

			/* output to the buffer, scaling by the volume */
			*buffer++ = val;
			samples--;

			/* next! */
			position += 4;
			if (position >= voice->stop)
				break;
		}
	}

	/* second case: looping */
	else
	{
		/* loop while we still have samples to generate */
		while (samples)
		{
			/* fetch the current value */
			val = (INT16)((base[position / 2 + 1] << 8) + base[position / 2]);

			/* output to the buffer, scaling by the volume */
			*buffer++ = val;
			samples--;

			/* next! */
			position += 4;
			if (position >= voice->loop_end)
			{
				if (voice->keyon)
					position = voice->loop_start;
			}
			if (position >= voice->stop)
				break;
		}
	}

	/* update the parameters */
	voice->position = position;

	return samples;
}



/**********************************************************************************************

     ymz280b_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void ymz280b_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct YMZ280BChip *chip = param;
	stream_sample_t *lacc = buffer[0];
	stream_sample_t *racc = buffer[1];
	int v;

	/* clear out the accumulator */
	memset(lacc, 0, length * sizeof(lacc[0]));
	memset(racc, 0, length * sizeof(racc[0]));

	/* loop over voices */
	for (v = 0; v < 8; v++)
	{
		struct YMZ280BVoice *voice = &chip->voice[v];
		INT16 prev = voice->last_sample;
		INT16 curr = voice->curr_sample;
		INT16 *curr_data = chip->scratch;
		INT32 *ldest = lacc;
		INT32 *rdest = racc;
		UINT32 new_samples, samples_left;
		UINT32 final_pos;
		int remaining = length;
		int lvol = voice->output_left;
		int rvol = voice->output_right;

		/* quick out if we're not playing and we're at 0 */
		if (!voice->playing && curr == 0)
			continue;

		/* finish off the current sample */
//      if (voice->output_pos > 0)
		{
			/* interpolate */
			while (remaining > 0 && voice->output_pos < FRAC_ONE)
			{
				int interp_sample = (((INT32)prev * (FRAC_ONE - voice->output_pos)) + ((INT32)curr * voice->output_pos)) >> FRAC_BITS;
				*ldest++ += interp_sample * lvol;
				*rdest++ += interp_sample * rvol;
				voice->output_pos += voice->output_step;
				remaining--;
			}

			/* if we're over, continue; otherwise, we're done */
			if (voice->output_pos >= FRAC_ONE)
				voice->output_pos -= FRAC_ONE;
			else
				continue;
		}

		/* compute how many new samples we need */
		final_pos = voice->output_pos + remaining * voice->output_step;
		new_samples = (final_pos + FRAC_ONE) >> FRAC_BITS;
		if (new_samples > MAX_SAMPLE_CHUNK)
			new_samples = MAX_SAMPLE_CHUNK;
		samples_left = new_samples;

		/* generate them into our buffer */
		if (voice->playing)
		{
			switch (voice->mode)
			{
				case 1:	samples_left = generate_adpcm(voice, chip->region_base, chip->scratch, new_samples);	break;
				case 2:	samples_left = generate_pcm8(voice, chip->region_base, chip->scratch, new_samples);	break;
				case 3:	samples_left = generate_pcm16(voice, chip->region_base, chip->scratch, new_samples);	break;
				default:
				case 0:	samples_left = 0; memset(chip->scratch, 0, new_samples * sizeof(chip->scratch[0]));			break;
			}
		}

		/* if there are leftovers, ramp back to 0 */
		if (samples_left)
		{
			int base = new_samples - samples_left;
			int i, t = (base == 0) ? curr : chip->scratch[base - 1];
			for (i = 0; i < samples_left; i++)
			{
				if (t < 0) t = -((-t * 15) >> 4);
				else if (t > 0) t = (t * 15) >> 4;
				chip->scratch[base + i] = t;
			}

			/* if we hit the end and IRQs are enabled, signal it */
			if (base != 0)
			{
				voice->playing = 0;

				/* set update_irq_state_timer. IRQ is signaled on next CPU execution. */
				timer_set_ptr(0, chip, update_irq_state_cb[v]);
				voice->irq_schedule = 1;
			}
		}

		/* advance forward one sample */
		prev = curr;
		curr = *curr_data++;

		/* then sample-rate convert with linear interpolation */
		while (remaining > 0)
		{
			/* interpolate */
			while (remaining > 0 && voice->output_pos < FRAC_ONE)
			{
				int interp_sample = (((INT32)prev * (FRAC_ONE - voice->output_pos)) + ((INT32)curr * voice->output_pos)) >> FRAC_BITS;
				*ldest++ += interp_sample * lvol;
				*rdest++ += interp_sample * rvol;
				voice->output_pos += voice->output_step;
				remaining--;
			}

			/* if we're over, grab the next samples */
			if (voice->output_pos >= FRAC_ONE)
			{
				voice->output_pos -= FRAC_ONE;
				prev = curr;
				curr = *curr_data++;
			}
		}

		/* remember the last samples */
		voice->last_sample = prev;
		voice->curr_sample = curr;
	}

	for (v = 0; v < length; v++)
	{
		buffer[0][v] /= 256;
		buffer[1][v] /= 256;
	}
}



/**********************************************************************************************

     YMZ280B_sh_start -- start emulation of the YMZ280B

***********************************************************************************************/

static void *ymz280b_start(int sndindex, int clock, const void *config)
{
	const struct YMZ280Binterface *intf = config;
	struct YMZ280BChip *chip;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	chip->ext_ram_read = intf->ext_read;
	chip->ext_ram_write = intf->ext_write;

	/* compute ADPCM tables */
	compute_tables();

	/* initialize the rest of the structure */
	chip->master_clock = (double)clock / 384.0;
	chip->region_base = memory_region(intf->region);
	chip->irq_callback = intf->irq_callback;

	/* create the stream */
	chip->stream = stream_create(0, 2, INTERNAL_SAMPLE_RATE, chip, ymz280b_update);

	/* allocate memory */
	chip->scratch = auto_malloc(sizeof(chip->scratch[0]) * MAX_SAMPLE_CHUNK);

	/* state save */
	{
		int j;
		state_save_register_item("YMZ280B", sndindex, chip->current_register);
		state_save_register_item("YMZ280B", sndindex, chip->status_register);
		state_save_register_item("YMZ280B", sndindex, chip->irq_state);
		state_save_register_item("YMZ280B", sndindex, chip->irq_mask);
		state_save_register_item("YMZ280B", sndindex, chip->irq_enable);
		state_save_register_item("YMZ280B", sndindex, chip->keyon_enable);
		state_save_register_item("YMZ280B", sndindex, chip->rom_readback_addr);
		for (j = 0; j < 8; j++)
		{
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].playing);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].keyon);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].looping);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].mode);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].fnum);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].level);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].pan);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].start);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].stop);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].loop_start);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].loop_end);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].position);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].signal);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].step);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].loop_signal);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].loop_step);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].loop_count);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].output_left);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].output_right);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].output_pos);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].last_sample);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].curr_sample);
			state_save_register_item("YMZ280B.voice", sndindex*8+j, chip->voice[j].irq_schedule);
		}
	}

	state_save_register_func_postload_ptr(YMZ280B_state_save_update_step, chip);

#if MAKE_WAVS
	chip->wavresample = wav_open("resamp.wav", Machine->sample_rate, 2);
#endif

	/* success */
	return chip;
}



/**********************************************************************************************

     YMZ280B_sh_stop -- stop emulation of the YMZ280B

***********************************************************************************************/

void YMZ280B_sh_stop(void)
{
#if MAKE_WAVS
{
	int i;

	for (i = 0; i < MAX_BSMT2000; i++)
	{
		if (ymz280b[i].wavresample)
			wav_close(ymz280b[i].wavresample);
	}
}
#endif
}



/**********************************************************************************************

     write_to_register -- handle a write to the current register

***********************************************************************************************/

static void write_to_register(struct YMZ280BChip *chip, int data)
{
	struct YMZ280BVoice *voice;
	int i;

	/* force an update */
	stream_update(chip->stream, 0);

	/* lower registers follow a pattern */
	if (chip->current_register < 0x80)
	{
		voice = &chip->voice[(chip->current_register >> 2) & 7];

		switch (chip->current_register & 0xe3)
		{
			case 0x00:		/* pitch low 8 bits */
				voice->fnum = (voice->fnum & 0x100) | (data & 0xff);
				update_step(chip, voice);
				break;

			case 0x01:		/* pitch upper 1 bit, loop, key on, mode */
				voice->fnum = (voice->fnum & 0xff) | ((data & 0x01) << 8);
				voice->looping = (data & 0x10) >> 4;
				voice->mode = (data & 0x60) >> 5;
				if (!voice->keyon && (data & 0x80) && chip->keyon_enable)
				{
					voice->playing = 1;
					voice->position = voice->start;
					voice->signal = voice->loop_signal = 0;
					voice->step = voice->loop_step = 0x7f;
					voice->loop_count = 0;

					/* if update_irq_state_timer is set, cancel it. */
					voice->irq_schedule = 0;
				}
				if (voice->keyon && !(data & 0x80) && !voice->looping)
				{
					voice->playing = 0;

					/* if update_irq_state_timer is set, cancel it. */
					voice->irq_schedule = 0;
				}
				voice->keyon = (data & 0x80) >> 7;
				update_step(chip, voice);
				break;

			case 0x02:		/* total level */
				voice->level = data;
				update_volumes(voice);
				break;

			case 0x03:		/* pan */
				voice->pan = data & 0x0f;
				update_volumes(voice);
				break;

			case 0x20:		/* start address high */
				voice->start = (voice->start & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x21:		/* loop start address high */
				voice->loop_start = (voice->loop_start & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x22:		/* loop end address high */
				voice->loop_end = (voice->loop_end & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x23:		/* stop address high */
				voice->stop = (voice->stop & (0x00ffff << 1)) | (data << 17);
				break;

			case 0x40:		/* start address middle */
				voice->start = (voice->start & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x41:		/* loop start address middle */
				voice->loop_start = (voice->loop_start & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x42:		/* loop end address middle */
				voice->loop_end = (voice->loop_end & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x43:		/* stop address middle */
				voice->stop = (voice->stop & (0xff00ff << 1)) | (data << 9);
				break;

			case 0x60:		/* start address low */
				voice->start = (voice->start & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x61:		/* loop start address low */
				voice->loop_start = (voice->loop_start & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x62:		/* loop end address low */
				voice->loop_end = (voice->loop_end & (0xffff00 << 1)) | (data << 1);
				break;

			case 0x63:		/* stop address low */
				voice->stop = (voice->stop & (0xffff00 << 1)) | (data << 1);
				break;

			default:
				logerror("YMZ280B: unknown register write %02X = %02X\n", chip->current_register, data);
				break;
		}
	}

	/* upper registers are special */
	else
	{
		switch (chip->current_register)
		{
			case 0x84:		/* ROM readback / RAM write (high) */
				chip->rom_readback_addr &= 0xffff;
				chip->rom_readback_addr |= (data<<16);
				break;

			case 0x85:		/* ROM readback / RAM write (med) */
				chip->rom_readback_addr &= 0xff00ff;
				chip->rom_readback_addr |= (data<<8);
				break;

			case 0x86:		/* ROM readback / RAM write (low) */
				chip->rom_readback_addr &= 0xffff00;
				chip->rom_readback_addr |= data;
				break;

			case 0x87:		/* RAM write */
				if (chip->ext_ram_write)
					chip->ext_ram_write(chip->rom_readback_addr, data);
				else
					logerror("YMZ280B attempted RAM write to %X\n", chip->rom_readback_addr);
				break;

			case 0xfe:		/* IRQ mask */
				chip->irq_mask = data;
				update_irq_state(chip);
				break;

			case 0xff:		/* IRQ enable, test, etc */
				chip->irq_enable = (data & 0x10) >> 4;
				update_irq_state(chip);

				if (chip->keyon_enable && !(data & 0x80))
				{
					for (i = 0; i < 8; i++)
					{
						chip->voice[i].playing = 0;

						/* if update_irq_state_timer is set, cancel it. */
						chip->voice[i].irq_schedule = 0;
					}
				}
				else if (!chip->keyon_enable && (data & 0x80))
				{
					for (i = 0; i < 8; i++)
					{
						if (chip->voice[i].keyon && chip->voice[i].looping)
							chip->voice[i].playing = 1;
					}
				}
				chip->keyon_enable = (data & 0x80) >> 7;
				break;

			default:
				logerror("YMZ280B: unknown register write %02X = %02X\n", chip->current_register, data);
				break;
		}
	}
}



/**********************************************************************************************

     compute_status -- determine the status bits

***********************************************************************************************/

static int compute_status(struct YMZ280BChip *chip)
{
	UINT8 result;

	/* ROM/RAM readback? */
	if (chip->current_register == 0x86)
	{
		return chip->region_base[chip->rom_readback_addr];
	}

	/* force an update */
	stream_update(chip->stream, 0);

	result = chip->status_register;

	/* clear the IRQ state */
	chip->status_register = 0;
	update_irq_state(chip);

	return result;
}



/**********************************************************************************************

     YMZ280B_status_0_r/YMZ280B_status_1_r -- handle a read from the status register

***********************************************************************************************/

READ8_HANDLER( YMZ280B_status_0_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	return compute_status(chip);
}

READ8_HANDLER( YMZ280B_status_1_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	return compute_status(chip);
}

READ16_HANDLER( YMZ280B_status_0_lsb_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	return compute_status(chip);
}

READ16_HANDLER( YMZ280B_status_0_msb_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	return compute_status(chip) << 8;
}

READ16_HANDLER( YMZ280B_status_1_lsb_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	return compute_status(chip);
}

READ16_HANDLER( YMZ280B_status_1_msb_r )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	return compute_status(chip) << 8;
}

/**********************************************************************************************

     YMZ280B_register_0_w/YMZ280B_register_1_w -- handle a write to the register select

***********************************************************************************************/

WRITE8_HANDLER( YMZ280B_register_0_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	chip->current_register = data;
}

WRITE8_HANDLER( YMZ280B_register_1_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	chip->current_register = data;
}

WRITE16_HANDLER( YMZ280B_register_0_lsb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	if (ACCESSING_LSB)	chip->current_register = data & 0xff;
}

WRITE16_HANDLER( YMZ280B_register_0_msb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	if (ACCESSING_MSB)	chip->current_register = (data >> 8) & 0xff;
}

WRITE16_HANDLER( YMZ280B_register_1_lsb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	if (ACCESSING_LSB)	chip->current_register = data & 0xff;
}

WRITE16_HANDLER( YMZ280B_register_1_msb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	if (ACCESSING_MSB)	chip->current_register = (data >> 8) & 0xff;
}

/**********************************************************************************************

     YMZ280B_data_0_w/YMZ280B_data_1_w -- handle a write to the current register

***********************************************************************************************/

WRITE8_HANDLER( YMZ280B_data_0_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	write_to_register(chip, data);
}

WRITE8_HANDLER( YMZ280B_data_1_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	write_to_register(chip, data);
}

WRITE16_HANDLER( YMZ280B_data_0_lsb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	if (ACCESSING_LSB)	write_to_register(chip, data & 0xff);
}

WRITE16_HANDLER( YMZ280B_data_0_msb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	if (ACCESSING_MSB)	write_to_register(chip, (data >> 8) & 0xff);
}

WRITE16_HANDLER( YMZ280B_data_1_lsb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	if (ACCESSING_LSB)	write_to_register(chip, data & 0xff);
}

WRITE16_HANDLER( YMZ280B_data_1_msb_w )
{
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	if (ACCESSING_MSB)	write_to_register(chip, (data >> 8) & 0xff);
}

/**********************************************************************************************

     YMZ280B_data_0_r/YMZ280B_data_1_r -- handle an external RAM read

***********************************************************************************************/

READ8_HANDLER( YMZ280B_data_0_r )
{
	UINT8 data;
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 0);
	data = chip->ext_ram_read(chip->rom_readback_addr - 1);
	chip->rom_readback_addr++;
	return data;
}

READ8_HANDLER( YMZ280B_data_1_r )
{
	UINT8 data;
	struct YMZ280BChip *chip = sndti_token(SOUND_YMZ280B, 1);
	data = chip->ext_ram_read(chip->rom_readback_addr - 1);
	chip->rom_readback_addr++;
	return data;
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ymz280b_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ymz280b_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ymz280b_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ymz280b_start;			break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YMZ280B";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha Wavetable";			break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

