/***************************************************************************

	sid6581.c

	MAME/MESS interface for SID6581 and SID8580 chips

***************************************************************************/

#include "mame.h"
#include "sndintrf.h"
#include "sid6581.h"
#include "sid.h"



static SID6581 *get_sid(int indx)
{
	int type = sndnum_to_sndti(indx, NULL);
	assert((type == SOUND_SID6581) || (type == SOUND_SID8580));
	return (SID6581 *) sndti_token(type, indx);
}



static void sid_update(void *token,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	SID6581 *sid = (SID6581 *) token;
	sidEmuFillBuffer(sid, _buffer[0], length);
}



static void *sid_start(int sndindex, int clock, const void *config, SIDTYPE sidtype)
{
	SID6581 *sid;
	const SID6581_interface *iface = (const SID6581_interface*) config;

	sid = (SID6581 *) auto_malloc(sizeof(*sid));
	memset(sid, 0, sizeof(*sid));

	sid->mixer_channel = stream_create (0, 1, options.samplerate, (void *) sid, sid_update);
	sid->PCMfreq = options.samplerate;	
	sid->clock = clock;
	sid->ad_read = iface ? iface->ad_read : NULL;
	sid->type = sidtype;

	sid6581_init(sid);
	sidInitWaveformTables(sidtype);
	return sid;
}



static void sid_reset(void *token)
{
	SID6581 *sid = (SID6581 *) token;
	sidEmuReset(sid);
}



static void *sid6581_start(int sndindex, int clock, const void *config)
{
	return sid_start(sndindex, clock, config, MOS6581);
}



static void *sid8580_start(int sndindex, int clock, const void *config)
{
	return sid_start(sndindex, clock, config, MOS8580);
}



READ8_HANDLER ( sid6581_0_port_r )
{
	return sid6581_port_r(get_sid(0), offset);
}

READ8_HANDLER ( sid6581_1_port_r )
{
	return sid6581_port_r(get_sid(1), offset);
}

WRITE8_HANDLER ( sid6581_0_port_w )
{
	sid6581_port_w(get_sid(0), offset, data);
}

WRITE8_HANDLER ( sid6581_1_port_w )
{
	sid6581_port_w(get_sid(1), offset, data);
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void sid6581_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void sid6581_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = sid6581_set_info;		break;
		case SNDINFO_PTR_START:							info->start = sid6581_start;			break;
		case SNDINFO_PTR_STOP:							info->stop = NULL;						break;
		case SNDINFO_PTR_RESET:							info->reset = sid_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "SID6581";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "SID";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2005, The MESS Team"; break;
	}
}


void sid8580_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_START:							info->start = sid8580_start;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "SID8580";					break;
		default:										sid6581_get_info(token, state, info);	break;
	}
}

