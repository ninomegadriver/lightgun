/*********************************************************

    Konami 053260 PCM Sound Chip

*********************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "k053260.h"

/* 2004-02-28: Fixed ppcm decoding. Games sound much better now.*/

#define LOG 0

#define BASE_SHIFT	16

struct K053260_channel_def {
	unsigned long		rate;
	unsigned long		size;
	unsigned long		start;
	unsigned long		bank;
	unsigned long		volume;
	int					play;
	unsigned long		pan;
	unsigned long		pos;
	int					loop;
	int					ppcm; /* packed PCM ( 4 bit signed ) */
	int					ppcm_data;
};
struct K053260_chip_def {
	sound_stream *					channel;
	int								mode;
	int								regs[0x30];
	unsigned char					*rom;
	int								rom_size;
	unsigned long					*delta_table;
	struct K053260_channel_def		channels[4];
	const struct K053260_interface	*intf;
};


static void InitDeltaTable( struct K053260_chip_def *ic, int clock ) {
	int		i;
	double	base = ( double )Machine->sample_rate;
	double	max = (double)(clock); /* Hz */
	unsigned long val;

	for( i = 0; i < 0x1000; i++ ) {
		double v = ( double )( 0x1000 - i );
		double target = (max) / v;
		double fixed = ( double )( 1 << BASE_SHIFT );

		if ( target && base ) {
			target = fixed / ( base / target );
			val = ( unsigned long )target;
			if ( val == 0 )
				val = 1;
		} else
			val = 1;

		ic->delta_table[i] = val;
	}
}

static void K053260_reset( void *chip ) {
	struct K053260_chip_def *ic = chip;
	int i;

	for( i = 0; i < 4; i++ ) {
		ic->channels[i].rate = 0;
		ic->channels[i].size = 0;
		ic->channels[i].start = 0;
		ic->channels[i].bank = 0;
		ic->channels[i].volume = 0;
		ic->channels[i].play = 0;
		ic->channels[i].pan = 0;
		ic->channels[i].pos = 0;
		ic->channels[i].loop = 0;
		ic->channels[i].ppcm = 0;
		ic->channels[i].ppcm_data = 0;
	}
}

INLINE int limit( int val, int max, int min ) {
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

#define MAXOUT 0x7fff
#define MINOUT -0x8000

void K053260_update( void * param, stream_sample_t **inputs, stream_sample_t **buffer, int length ) {
	static long dpcmcnv[] = { 0,1,2,4,8,16,32,64, -128, -64, -32, -16, -8, -4, -2, -1};

	int i, j, lvol[4], rvol[4], play[4], loop[4], ppcm_data[4], ppcm[4];
	unsigned char *rom[4];
	unsigned long delta[4], end[4], pos[4];
	int dataL, dataR;
	signed char d;
	struct K053260_chip_def *ic = param;

	/* precache some values */
	for ( i = 0; i < 4; i++ ) {
		rom[i]= &ic->rom[ic->channels[i].start + ( ic->channels[i].bank << 16 )];
		delta[i] = ic->delta_table[ic->channels[i].rate];
		lvol[i] = ic->channels[i].volume * ic->channels[i].pan;
		rvol[i] = ic->channels[i].volume * ( 8 - ic->channels[i].pan );
		end[i] = ic->channels[i].size;
		pos[i] = ic->channels[i].pos;
		play[i] = ic->channels[i].play;
		loop[i] = ic->channels[i].loop;
		ppcm[i] = ic->channels[i].ppcm;
		ppcm_data[i] = ic->channels[i].ppcm_data;
		if ( ppcm[i] )
			delta[i] /= 2;
	}

		for ( j = 0; j < length; j++ ) {

			dataL = dataR = 0;

			for ( i = 0; i < 4; i++ ) {
				/* see if the voice is on */
				if ( play[i] ) {
					/* see if we're done */
					if ( ( pos[i] >> BASE_SHIFT ) >= end[i] ) {

						ppcm_data[i] = 0;
						if ( loop[i] )
							pos[i] = 0;
						else {
							play[i] = 0;
							continue;
						}
					}

					if ( ppcm[i] ) { /* Packed PCM */
						/* we only update the signal if we're starting or a real sound sample has gone by */
						/* this is all due to the dynamic sample rate convertion */
						if ( pos[i] == 0 || ( ( pos[i] ^ ( pos[i] - delta[i] ) ) & 0x8000 ) == 0x8000 )

						 {
							int newdata;
							if ( pos[i] & 0x8000 ){

								newdata = ((rom[i][pos[i] >> BASE_SHIFT]) >> 4) & 0x0f; /*high nybble*/
							}
							else{
								newdata = ( ( rom[i][pos[i] >> BASE_SHIFT] ) ) & 0x0f; /*low nybble*/
							}

							ppcm_data[i] = (( ( ppcm_data[i] * 62 ) >> 6 ) + dpcmcnv[newdata]);

							if ( ppcm_data[i] > 127 )
								ppcm_data[i] = 127;
							else
								if ( ppcm_data[i] < -128 )
									ppcm_data[i] = -128;
						}



						d = ppcm_data[i];

						pos[i] += delta[i];
					} else { /* PCM */
						d = rom[i][pos[i] >> BASE_SHIFT];

						pos[i] += delta[i];
					}

					if ( ic->mode & 2 ) {
						dataL += ( d * lvol[i] ) >> 2;
						dataR += ( d * rvol[i] ) >> 2;
					}
				}
			}

			buffer[1][j] = limit( dataL, MAXOUT, MINOUT );
			buffer[0][j] = limit( dataR, MAXOUT, MINOUT );
		}

	/* update the regs now */
	for ( i = 0; i < 4; i++ ) {
		ic->channels[i].pos = pos[i];
		ic->channels[i].play = play[i];
		ic->channels[i].ppcm_data = ppcm_data[i];
	}
}

static void *k053260_start(int sndindex, int clock, const void *config)
{
	struct K053260_chip_def *ic;
	int i;

	ic = auto_malloc(sizeof(*ic));
	memset(ic, 0, sizeof(*ic));

	/* Initialize our chip structure */
	ic->intf = config;

	ic->mode = 0;
	ic->rom = memory_region(ic->intf->region);
	ic->rom_size = memory_region_length(ic->intf->region) - 1;

	K053260_reset( ic );

	for ( i = 0; i < 0x30; i++ )
		ic->regs[i] = 0;

	ic->delta_table = ( unsigned long * )auto_malloc( 0x1000 * sizeof( unsigned long ) );

	ic->channel = stream_create( 0, 2, Machine->sample_rate, ic, K053260_update );

	InitDeltaTable( ic, clock );

	/* setup SH1 timer if necessary */
	if ( ic->intf->irq )
		timer_pulse( TIME_IN_HZ( ( clock / 32 ) ), 0, ic->intf->irq );

    return ic;
}

INLINE void check_bounds( struct K053260_chip_def *ic, int channel ) {

	int channel_start = ( ic->channels[channel].bank << 16 ) + ic->channels[channel].start;
	int channel_end = channel_start + ic->channels[channel].size - 1;

	if ( channel_start > ic->rom_size ) {
		logerror("K53260: Attempting to start playing past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		ic->channels[channel].play = 0;

		return;
	}

	if ( channel_end > ic->rom_size ) {
		logerror("K53260: Attempting to play past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		ic->channels[channel].size = ic->rom_size - channel_start;
	}
#if LOG
	logerror("K053260: Sample Start = %06x, Sample End = %06x, Sample rate = %04lx, PPCM = %s\n", channel_start, channel_end, ic->channels[channel].rate, ic->channels[channel].ppcm ? "yes" : "no" );
#endif
}

void K053260_write( int chip, offs_t offset, UINT8 data )
{
	int i, t;
	int r = offset;
	int v = data;

	struct K053260_chip_def *ic = sndti_token(SOUND_K053260, chip);

	if ( r > 0x2f ) {
		logerror("K053260: Writing past registers\n" );
		return;
	}

	stream_update( ic->channel, 0 );

	/* before we update the regs, we need to check for a latched reg */
	if ( r == 0x28 ) {
		t = ic->regs[r] ^ v;

		for ( i = 0; i < 4; i++ ) {
			if ( t & ( 1 << i ) ) {
				if ( v & ( 1 << i ) ) {
					ic->channels[i].play = 1;
					ic->channels[i].pos = 0;
					ic->channels[i].ppcm_data = 0;
					check_bounds( ic, i );
				} else
					ic->channels[i].play = 0;
			}
		}

		ic->regs[r] = v;
		return;
	}

	/* update regs */
	ic->regs[r] = v;

	/* communication registers */
	if ( r < 8 )
		return;

	/* channel setup */
	if ( r < 0x28 ) {
		int channel = ( r - 8 ) / 8;

		switch ( ( r - 8 ) & 0x07 ) {
			case 0: /* sample rate low */
				ic->channels[channel].rate &= 0x0f00;
				ic->channels[channel].rate |= v;
			break;

			case 1: /* sample rate high */
				ic->channels[channel].rate &= 0x00ff;
				ic->channels[channel].rate |= ( v & 0x0f ) << 8;
			break;

			case 2: /* size low */
				ic->channels[channel].size &= 0xff00;
				ic->channels[channel].size |= v;
			break;

			case 3: /* size high */
				ic->channels[channel].size &= 0x00ff;
				ic->channels[channel].size |= v << 8;
			break;

			case 4: /* start low */
				ic->channels[channel].start &= 0xff00;
				ic->channels[channel].start |= v;
			break;

			case 5: /* start high */
				ic->channels[channel].start &= 0x00ff;
				ic->channels[channel].start |= v << 8;
			break;

			case 6: /* bank */
				ic->channels[channel].bank = v & 0xff;
			break;

			case 7: /* volume is 7 bits. Convert to 8 bits now. */
				ic->channels[channel].volume = ( ( v & 0x7f ) << 1 ) | ( v & 1 );
			break;
		}

		return;
	}

	switch( r ) {
		case 0x2a: /* loop, ppcm */
			for ( i = 0; i < 4; i++ )
				ic->channels[i].loop = ( v & ( 1 << i ) ) != 0;

			for ( i = 4; i < 8; i++ )
				ic->channels[i-4].ppcm = ( v & ( 1 << i ) ) != 0;
		break;

		case 0x2c: /* pan */
			ic->channels[0].pan = v & 7;
			ic->channels[1].pan = ( v >> 3 ) & 7;
		break;

		case 0x2d: /* more pan */
			ic->channels[2].pan = v & 7;
			ic->channels[3].pan = ( v >> 3 ) & 7;
		break;

		case 0x2f: /* control */
			ic->mode = v & 7;
			/* bit 0 = read ROM */
			/* bit 1 = enable sound output */
			/* bit 2 = unknown */
		break;
	}
}

UINT8 K053260_read( int chip, offs_t offset )
{
	struct K053260_chip_def *ic = sndti_token(SOUND_K053260, chip);

	switch ( offset ) {
		case 0x29: /* channel status */
			{
				int i, status = 0;

				for ( i = 0; i < 4; i++ )
					status |= ic->channels[i].play << i;

				return status;
			}
		break;

		case 0x2e: /* read rom */
			if ( ic->mode & 1 ) {
				unsigned int offs = ic->channels[0].start + ( ic->channels[0].pos >> BASE_SHIFT ) + ( ic->channels[0].bank << 16 );

				ic->channels[0].pos += ( 1 << 16 );

				if ( offs > ic->rom_size ) {
					logerror("%06x: K53260: Attempting to read past rom size in rom Read Mode (offs = %06x, size = %06x).\n",activecpu_get_pc(),offs,ic->rom_size );

					return 0;
				}

				return ic->rom[offs];
			}
		break;
	}

	return ic->regs[offset];
}

/**************************************************************************************************/
/* Accesors */

READ8_HANDLER( K053260_0_r )
{
	return K053260_read( 0, offset );
}

WRITE8_HANDLER( K053260_0_w )
{
	K053260_write( 0, offset, data );
}

READ8_HANDLER( K053260_1_r )
{
	return K053260_read( 1, offset );
}

WRITE8_HANDLER( K053260_1_w )
{
	K053260_write( 1, offset, data );
}

WRITE16_HANDLER( K053260_0_lsb_w )
{
	if (ACCESSING_LSB)
		K053260_0_w (offset, data & 0xff);
}

READ16_HANDLER( K053260_0_lsb_r )
{
	return K053260_0_r(offset);
}

WRITE16_HANDLER( K053260_1_lsb_w )
{
	if (ACCESSING_LSB)
		K053260_1_w (offset, data & 0xff);
}

READ16_HANDLER( K053260_1_lsb_r )
{
	return K053260_1_r(offset);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void k053260_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void k053260_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = k053260_set_info;		break;
		case SNDINFO_PTR_START:							info->start = k053260_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "K053260";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Konami custom";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

