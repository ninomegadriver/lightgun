/*********************************************************

Irem GA20 PCM Sound Chip

It's not currently known whether this chip is stereo.


Revisions:

04-15-2002 Acho A. Tang
- rewrote channel mixing
- added prelimenary volume and sample rate emulation

05-30-2002 Acho A. Tang
- applied hyperbolic gain control to volume and used
  a musical-note style progression in sample rate
  calculation(still very inaccurate)

02-18-2004 R. Belmont
- sample rate calculation reverse-engineered.
  Thanks to Fujix, Yasuhiro Ogawa, the Guru, and Tormod
  for real PCB samples that made this possible.

*********************************************************/
#include <math.h>
#include "sndintrf.h"
#include "streams.h"
#include "iremga20.h"

//AT
#define MAX_VOL 256

#define MIX_CH(CH) \
	if (play[CH]) \
	{ \
		eax = pos[CH]; \
		ebx = eax; \
		eax >>= 8; \
		eax = *(INT8 *)(esi + eax); \
		eax *= vol[CH]; \
		ebx += rate[CH]; \
		pos[CH] = ebx; \
		ebx = (ebx < end[CH]); \
		play[CH] = ebx; \
		edx += eax; \
	}
//ZT

struct IremGA20_channel_def
{
	UINT32 rate;
	UINT32 size;
	UINT32 start;
	UINT32 pos;
	UINT32 end;
	UINT32 volume;
	UINT32 pan;
	UINT32 effect;
	UINT32 play;
};

struct IremGA20_chip_def
{
	const struct IremGA20_interface *intf;
	UINT8 *rom;
	INT32 rom_size;
	sound_stream * stream;
	INT32 regs[0x40];
	struct IremGA20_channel_def channel[4];
	INT32 sr_table[256];
};

void IremGA20_update( void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length )
{
	struct IremGA20_chip_def *chip = param;
	UINT32 rate[4], pos[4], end[4], vol[4], play[4];
	UINT32 esi;
	stream_sample_t *edi, *ebp;
	int eax, ebx, ecx, edx;

	/* precache some values */
	for (ecx=0; ecx<4; ecx++)
	{
		rate[ecx] = chip->channel[ecx].rate;
		pos[ecx] = chip->channel[ecx].pos;
		end[ecx] = (chip->channel[ecx].end - 0x20) << 8;
		vol[ecx] = chip->channel[ecx].volume;
		play[ecx] = chip->channel[ecx].play;
	}

	ecx = length;
	esi = (UINT32)chip->rom;
	edi = buffer[0];
	ebp = buffer[1];
	edi += ecx;
	ebp += ecx;
	ecx = -ecx;

	for (; ecx; ecx++)
	{
		edx = 0;

		MIX_CH(0);
		MIX_CH(1);
		MIX_CH(2);
		MIX_CH(3);

		edx >>= 2;
		edi[ecx] = edx;
		ebp[ecx] = edx;
	}

	/* update the regs now */
	for (ecx=0; ecx< 4; ecx++)
	{
		chip->channel[ecx].pos = pos[ecx];
		chip->channel[ecx].play = play[ecx];
	}
}
//ZT

WRITE8_HANDLER( IremGA20_w )
{
	struct IremGA20_chip_def *chip = sndti_token(SOUND_IREMGA20, 0);
	int channel;

	//logerror("GA20:  Offset %02x, data %04x\n",offset,data);

	stream_update(chip->stream, 0);

	channel = offset >> 4;

	chip->regs[offset] = data;

	switch (offset & 0xf)
	{
	case 0: /* start address low */
		chip->channel[channel].start = ((chip->channel[channel].start)&0xff000) | (data<<4);
	break;

	case 2: /* start address high */
		chip->channel[channel].start = ((chip->channel[channel].start)&0x00ff0) | (data<<12);
	break;

	case 4: /* end address low */
		chip->channel[channel].end = ((chip->channel[channel].end)&0xff000) | (data<<4);
	break;

	case 6: /* end address high */
		chip->channel[channel].end = ((chip->channel[channel].end)&0x00ff0) | (data<<12);
	break;

	case 8:
		chip->channel[channel].rate = (chip->sr_table[data]<<8) / Machine->sample_rate;
	break;

	case 0xa: //AT: gain control
		chip->channel[channel].volume = (data * MAX_VOL) / (data + 10);
	break;

	case 0xc: //AT: this is always written 2(enabling both channels?)
		chip->channel[channel].play = data;
		chip->channel[channel].pos = chip->channel[channel].start << 8;
	break;
	}
}

READ8_HANDLER( IremGA20_r )
{
	struct IremGA20_chip_def *chip = sndti_token(SOUND_IREMGA20, 0);
	int channel;

	stream_update(chip->stream, 0);

	channel = offset >> 4;

	switch (offset & 0xf)
	{
		case 0xe:	// voice status.  bit 0 is 1 if active. (routine around 0xccc in rtypeleo)
			return chip->channel[channel].play ? 1 : 0;
			break;

		default:
			logerror("GA20: read unk. register %d, channel %d\n", offset & 0xf, channel);
			break;
	}

	return 0;
}

static void iremga20_reset( void *_chip )
{
	struct IremGA20_chip_def *chip = _chip;
	int i;

	for( i = 0; i < 4; i++ ) {
	chip->channel[i].rate = 0;
	chip->channel[i].size = 0;
	chip->channel[i].start = 0;
	chip->channel[i].pos = 0;
	chip->channel[i].end = 0;
	chip->channel[i].volume = 0;
	chip->channel[i].pan = 0;
	chip->channel[i].effect = 0;
	chip->channel[i].play = 0;
	}
}


static void *iremga20_start(int sndindex, int clock, const void *config)
{
	struct IremGA20_chip_def *chip;
	int i;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	/* Initialize our chip structure */
	chip->intf = config;
	chip->rom = memory_region(chip->intf->region);
	chip->rom_size = memory_region_length(chip->intf->region);

	/* Initialize our pitch table */
	for (i = 0; i < 255; i++)
	{
		chip->sr_table[i] = (clock / (256-i) / 4);
	}

	/* change signedness of PCM samples in advance */
	for (i=0; i<chip->rom_size; i++)
		chip->rom[i] -= 0x80;

	iremga20_reset(chip);

	for ( i = 0; i < 0x40; i++ )
	chip->regs[i] = 0;

	chip->stream = stream_create( 0, 2, Machine->sample_rate, chip, IremGA20_update );

	state_save_register_item_array("irem_ga20", sndindex, chip->regs);
	for (i = 0; i < 4; i++)
	{
		char buf[20];
		sprintf(buf, "irem_ga20.ch%d", i);
		state_save_register_item(buf, sndindex, chip->channel[i].rate);
		state_save_register_item(buf, sndindex, chip->channel[i].size);
		state_save_register_item(buf, sndindex, chip->channel[i].start);
		state_save_register_item(buf, sndindex, chip->channel[i].pos);
		state_save_register_item(buf, sndindex, chip->channel[i].end);
		state_save_register_item(buf, sndindex, chip->channel[i].volume);
		state_save_register_item(buf, sndindex, chip->channel[i].pan);
		state_save_register_item(buf, sndindex, chip->channel[i].effect);
		state_save_register_item(buf, sndindex, chip->channel[i].play);
	}

	return chip;
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void iremga20_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void iremga20_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = iremga20_set_info;		break;
		case SNDINFO_PTR_START:							info->start = iremga20_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = iremga20_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Irem GA20";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Irem custom";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

