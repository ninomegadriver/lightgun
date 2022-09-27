#include "sndintrf.h"
#include "streams.h"
#include "dac.h"
#include <math.h>


/* default to 4x oversampling */
#define DEFAULT_DAC_SAMPLE_RATE (44100 * 4)


struct dac_info
{
	sound_stream *channel;
	int		output;
	int		UnsignedVolTable[256];
	int		SignedVolTable[256];
};



static void DAC_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	struct dac_info *info = param;
	stream_sample_t *buffer = _buffer[0];
	int out = info->output;

	while (length--) *(buffer++) = out;
}


void DAC_data_w(int num,UINT8 data)
{
	struct dac_info *info = sndti_token(SOUND_DAC, num);
	int out = info->UnsignedVolTable[data];

	if (info->output != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(info->channel,0);
		info->output = out;
	}
}


void DAC_signed_data_w(int num,UINT8 data)
{
	struct dac_info *info = sndti_token(SOUND_DAC, num);
	int out = info->SignedVolTable[data];

	if (info->output != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(info->channel,0);
		info->output = out;
	}
}


void DAC_data_16_w(int num,UINT16 data)
{
	struct dac_info *info = sndti_token(SOUND_DAC, num);
	int out = data >> 1;		/* range      0..32767 */

	if (info->output != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(info->channel,0);
		info->output = out;
	}
}


void DAC_signed_data_16_w(int num,UINT16 data)
{
	struct dac_info *info = sndti_token(SOUND_DAC, num);
	int out = data - 0x8000;	/* range -32768..32767 */

	if (info->output != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(info->channel,0);
		info->output = out;
	}
}


static void DAC_build_voltable(struct dac_info *info)
{
	int i;


	/* build volume table (linear) */
	for (i = 0;i < 256;i++)
	{
		info->UnsignedVolTable[i] = i * 0x101 / 2;	/* range      0..32767 */
		info->SignedVolTable[i] = i * 0x101 - 0x8000;	/* range -32768..32767 */
	}
}


static void *dac_start(int sndindex, int clock, const void *config)
{
	struct dac_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	DAC_build_voltable(info);

	info->channel = stream_create(0,1,clock ? clock : DEFAULT_DAC_SAMPLE_RATE,info,DAC_update);
	info->output = 0;

	state_save_register_item("dac", sndindex, info->output);

	return info;
}



WRITE8_HANDLER( DAC_0_data_w )
{
	DAC_data_w(0,data);
}

WRITE8_HANDLER( DAC_1_data_w )
{
	DAC_data_w(1,data);
}

WRITE8_HANDLER( DAC_0_signed_data_w )
{
	DAC_signed_data_w(0,data);
}

WRITE8_HANDLER( DAC_1_signed_data_w )
{
	DAC_signed_data_w(1,data);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void dac_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void dac_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = dac_set_info;			break;
		case SNDINFO_PTR_START:							info->start = dac_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "DAC";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "TI Speech";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

