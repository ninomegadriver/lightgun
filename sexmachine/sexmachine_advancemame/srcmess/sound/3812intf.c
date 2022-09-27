/******************************************************************************
* FILE
*   Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*   Ernesto Corvi
*
* UPDATE LOG
*   JB  28-04-2002  Fixed simultaneous usage of all three different chip types.
*                       Used real sample rate when resample filter is active.
*       AAT 12-28-2001  Protected Y8950 from accessing unmapped port and keyboard handlers.
*   CHS 1999-01-09  Fixes new ym3812 emulation interface.
*   CHS 1998-10-23  Mame streaming sound chip update
*   EC  1998        Created Interface
*
* NOTES
*
******************************************************************************/
#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "3812intf.h"
#include "fm.h"
#include "sound/fmopl.h"


#if (HAS_YM3812)

struct ym3812_info
{
	sound_stream *	stream;
	mame_timer *	timer[2];
	void *			chip;
	const struct YM3812interface *intf;
};

static void IRQHandler_3812(void *param,int irq)
{
	struct ym3812_info *info = param;
	if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
static void timer_callback_3812_0(void *param)
{
	struct ym3812_info *info = param;
	YM3812TimerOver(info->chip,0);
}

static void timer_callback_3812_1(void *param)
{
	struct ym3812_info *info = param;
	YM3812TimerOver(info->chip,1);
}

static void TimerHandler_3812(void *param,int c,double period)
{
	struct ym3812_info *info = param;
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust_ptr(info->timer[c], period, 0);
	}
}


static void ym3812_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct ym3812_info *info = param;
	YM3812UpdateOne(info->chip, buffer[0], length);
}

static void _stream_update_3812(void * param, int interval)
{
	struct ym3812_info *info = param;
	stream_update(info->stream, interval);
}


static void *ym3812_start(int sndindex, int clock, const void *config)
{
	static const struct YM3812interface dummy = { 0 };
	int rate = Machine->sample_rate;
	struct ym3812_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config ? config : &dummy;

	rate = clock/72;

	/* stream system initialize */
	info->chip = YM3812Init(clock,rate);
	if (!info->chip)
		return NULL;

	info->stream = stream_create(0,1,rate,info,ym3812_stream_update);

	/* YM3812 setup */
	YM3812SetTimerHandler (info->chip, TimerHandler_3812, info);
	YM3812SetIRQHandler   (info->chip, IRQHandler_3812, info);
	YM3812SetUpdateHandler(info->chip, _stream_update_3812, info);

	info->timer[0] = timer_alloc_ptr(timer_callback_3812_0, info);
	info->timer[1] = timer_alloc_ptr(timer_callback_3812_1, info);

	return info;
}

static void ym3812_stop(void *token)
{
	struct ym3812_info *info = token;
	YM3812Shutdown(info->chip);
}

static void ym3812_reset(void *token)
{
	struct ym3812_info *info = token;
	YM3812ResetChip(info->chip);
}

WRITE8_HANDLER( YM3812_control_port_0_w ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 0);
	YM3812Write(info->chip, 0, data);
}
WRITE8_HANDLER( YM3812_write_port_0_w ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 0);
	YM3812Write(info->chip, 1, data);
}
READ8_HANDLER( YM3812_status_port_0_r ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 0);
	return YM3812Read(info->chip, 0);
}
READ8_HANDLER( YM3812_read_port_0_r ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 0);
	return YM3812Read(info->chip, 1);
}


WRITE8_HANDLER( YM3812_control_port_1_w ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 1);
	YM3812Write(info->chip, 0, data);
}
WRITE8_HANDLER( YM3812_write_port_1_w ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 1);
	YM3812Write(info->chip, 1, data);
}
READ8_HANDLER( YM3812_status_port_1_r ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 1);
	return YM3812Read(info->chip, 0);
}
READ8_HANDLER( YM3812_read_port_1_r ) {
	struct ym3812_info *info = sndti_token(SOUND_YM3812, 1);
	return YM3812Read(info->chip, 1);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ym3812_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ym3812_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ym3812_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ym3812_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = ym3812_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = ym3812_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YM3812";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

#endif


#if (HAS_YM3526)

struct ym3526_info
{
	sound_stream *	stream;
	mame_timer *	timer[2];
	void *			chip;
	const struct YM3526interface *intf;
};


/* IRQ Handler */
static void IRQHandler_3526(void *param,int irq)
{
	struct ym3526_info *info = param;
	if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
/* Timer overflow callback from timer.c */
static void timer_callback_3526_0(void *param)
{
	struct ym3526_info *info = param;
	YM3526TimerOver(info->chip,0);
}
static void timer_callback_3526_1(void *param)
{
	struct ym3526_info *info = param;
	YM3526TimerOver(info->chip,1);
}
/* TimerHandler from fm.c */
static void TimerHandler_3526(void *param,int c,double period)
{
	struct ym3526_info *info = param;
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust_ptr(info->timer[c], period, 0);
	}
}


static void ym3526_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct ym3526_info *info = param;
	YM3526UpdateOne(info->chip, buffer[0], length);
}

static void _stream_update_3526(void *param, int interval)
{
	struct ym3526_info *info = param;
	stream_update(info->stream, interval);
}


static void *ym3526_start(int sndindex, int clock, const void *config)
{
	static const struct YM3526interface dummy = { 0 };
	int rate = Machine->sample_rate;
	struct ym3526_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config ? config : &dummy;

	rate = clock/72;

	/* stream system initialize */
	info->chip = YM3526Init(clock,rate);
	if (!info->chip)
		return NULL;

	info->stream = stream_create(0,1,rate,info,ym3526_stream_update);
	/* YM3526 setup */
	YM3526SetTimerHandler (info->chip, TimerHandler_3526, info);
	YM3526SetIRQHandler   (info->chip, IRQHandler_3526, info);
	YM3526SetUpdateHandler(info->chip, _stream_update_3526, info);

	info->timer[0] = timer_alloc_ptr(timer_callback_3526_0, info);
	info->timer[1] = timer_alloc_ptr(timer_callback_3526_1, info);

	return info;
}

static void ym3526_stop(void *token)
{
	struct ym3526_info *info = token;
	YM3526Shutdown(info->chip);
}

static void ym3526_reset(void *token)
{
	struct ym3526_info *info = token;
	YM3526ResetChip(info->chip);
}

WRITE8_HANDLER( YM3526_control_port_0_w ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 0);
	YM3526Write(info->chip, 0, data);
}
WRITE8_HANDLER( YM3526_write_port_0_w ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 0);
	YM3526Write(info->chip, 1, data);
}
READ8_HANDLER( YM3526_status_port_0_r ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 0);
	return YM3526Read(info->chip, 0);
}
READ8_HANDLER( YM3526_read_port_0_r ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 0);
	return YM3526Read(info->chip, 1);
}


WRITE8_HANDLER( YM3526_control_port_1_w ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 1);
	YM3526Write(info->chip, 0, data);
}
WRITE8_HANDLER( YM3526_write_port_1_w ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 1);
	YM3526Write(info->chip, 1, data);
}
READ8_HANDLER( YM3526_status_port_1_r ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 1);
	return YM3526Read(info->chip, 0);
}
READ8_HANDLER( YM3526_read_port_1_r ) {
	struct ym3526_info *info = sndti_token(SOUND_YM3526, 1);
	return YM3526Read(info->chip, 1);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ym3526_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ym3526_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ym3526_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ym3526_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = ym3526_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = ym3526_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YM3526";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

#endif


#if (HAS_Y8950)

struct y8950_info
{
	sound_stream *	stream;
	mame_timer *	timer[2];
	void *			chip;
	const struct Y8950interface *intf;
	int				index;
};

static void IRQHandler_8950(void *param,int irq)
{
	struct y8950_info *info = param;
	if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
static void timer_callback_8950_0(void *param)
{
	struct y8950_info *info = param;
	Y8950TimerOver(info->chip,0);
}
static void timer_callback_8950_1(void *param)
{
	struct y8950_info *info = param;
	Y8950TimerOver(info->chip,1);
}
static void TimerHandler_8950(void *param,int c,double period)
{
	struct y8950_info *info = param;
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust_ptr(info->timer[c], period, 0);
	}
}


static unsigned char Y8950PortHandler_r(void *param)
{
	struct y8950_info *info = param;
	if (info->intf->portread)
		return info->intf->portread(info->index);
	return 0;
}

static void Y8950PortHandler_w(void *param,unsigned char data)
{
	struct y8950_info *info = param;
	if (info->intf->portwrite)
		info->intf->portwrite(info->index,data);
}

static unsigned char Y8950KeyboardHandler_r(void *param)
{
	struct y8950_info *info = param;
	if (info->intf->keyboardread)
		return info->intf->keyboardread(info->index);
	return 0;
}

static void Y8950KeyboardHandler_w(void *param,unsigned char data)
{
	struct y8950_info *info = param;
	if (info->intf->keyboardwrite)
		info->intf->keyboardwrite(info->index,data);
}

static void y8950_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct y8950_info *info = param;
	Y8950UpdateOne(info->chip, buffer[0], length);
}

static void _stream_update_8950(void *param, int interval)
{
	struct y8950_info *info = param;
	stream_update(info->stream, interval);
}


static void *y8950_start(int sndindex, int clock, const void *config)
{
	static const struct Y8950interface dummy = { 0 };
	int rate = Machine->sample_rate;
	struct y8950_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config ? config : &dummy;
	info->index = sndindex;

	rate = clock/72;

	/* stream system initialize */
	info->chip = Y8950Init(clock,rate);
	if (!info->chip)
		return NULL;

	/* ADPCM ROM data */
	Y8950SetDeltaTMemory(info->chip,
		(void *)(memory_region(info->intf->rom_region)),
			memory_region_length(info->intf->rom_region) );

	info->stream = stream_create(0,1,rate,info,y8950_stream_update);

	/* port and keyboard handler */
	Y8950SetPortHandler(info->chip, Y8950PortHandler_w, Y8950PortHandler_r, info);
	Y8950SetKeyboardHandler(info->chip, Y8950KeyboardHandler_w, Y8950KeyboardHandler_r, info);

	/* Y8950 setup */
	Y8950SetTimerHandler (info->chip, TimerHandler_8950, info);
	Y8950SetIRQHandler   (info->chip, IRQHandler_8950, info);
	Y8950SetUpdateHandler(info->chip, _stream_update_8950, info);

	info->timer[0] = timer_alloc_ptr(timer_callback_8950_0, info);
	info->timer[1] = timer_alloc_ptr(timer_callback_8950_1, info);

	return info;
}

static void y8950_stop(void *token)
{
	struct y8950_info *info = token;
	Y8950Shutdown(info->chip);
}

static void y8950_reset(void *token)
{
	struct y8950_info *info = token;
	Y8950ResetChip(info->chip);
}

WRITE8_HANDLER( Y8950_control_port_0_w ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 0);
	Y8950Write(info->chip, 0, data);
}
WRITE8_HANDLER( Y8950_write_port_0_w ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 0);
	Y8950Write(info->chip, 1, data);
}
READ8_HANDLER( Y8950_status_port_0_r ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 0);
	return Y8950Read(info->chip, 0);
}
READ8_HANDLER( Y8950_read_port_0_r ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 0);
	return Y8950Read(info->chip, 1);
}


WRITE8_HANDLER( Y8950_control_port_1_w ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 1);
	Y8950Write(info->chip, 0, data);
}
WRITE8_HANDLER( Y8950_write_port_1_w ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 1);
	Y8950Write(info->chip, 1, data);
}
READ8_HANDLER( Y8950_status_port_1_r ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 1);
	return Y8950Read(info->chip, 0);
}
READ8_HANDLER( Y8950_read_port_1_r ) {
	struct y8950_info *info = sndti_token(SOUND_Y8950, 1);
	return Y8950Read(info->chip, 1);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void y8950_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void y8950_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = y8950_set_info;		break;
		case SNDINFO_PTR_START:							info->start = y8950_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = y8950_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = y8950_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Y8950";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

#endif
