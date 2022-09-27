/***************************************************************************

  2612intf.c

  The YM2612 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "sound/fm.h"
#include "sound/2612intf.h"


#ifdef BUILD_YM2612

struct ym2612_info
{
	sound_stream *	stream;
	mame_timer *	timer[2];
	double			lastfired[2];
	void *			chip;
	const struct YM2612interface *intf;
};

/*------------------------- TM2612 -------------------------------*/
/* IRQ Handler */
static void IRQHandler(void *param,int irq)
{
	struct ym2612_info *info = param;
	if(info->intf->handler) info->intf->handler(irq);
}

/* Timer overflow callback from timer.c */
static void timer_callback_2612_0(void *param)
{
	struct ym2612_info *info = param;
	info->lastfired[0] = timer_get_time();
	YM2612TimerOver(info->chip,0);
}

static void timer_callback_2612_1(void *param)
{
	struct ym2612_info *info = param;
	info->lastfired[1] = timer_get_time();
	YM2612TimerOver(info->chip,1);
}

/* TimerHandler from fm.c */
static void TimerHandler(void *param,int c,int count,double stepTime)
{
	struct ym2612_info *info = param;
	if( count == 0 )
	{	/* Reset FM Timer */
		timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;
		double slack;

		slack = timer_get_time() - info->lastfired[c];
		/* hackish way to make bstars intro sync without */
		/* breaking sonicwi2 command 0x35 */
		if (slack < 0.000050) slack = 0;

		if (!timer_enable(info->timer[c], 1))
			timer_adjust_ptr(info->timer[c], timeSec - slack, 0);
	}
}

/* update request from fm.c */
void YM2612UpdateRequest(void *param)
{
	struct ym2612_info *info = param;
	stream_update(info->stream,100);
}

/***********************************************************/
/*    YM2612                                               */
/***********************************************************/

static void ym2612_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **buffers, int length)
{
	struct ym2612_info *info = param;
	YM2612UpdateOne(info->chip, buffers, length);
}


static void ym2612_postload(void *param)
{
	struct ym2612_info *info = param;
	YM2612Postload(info->chip);
}


static void *ym2612_start(int sndindex, int clock, const void *config)
{
	static const struct YM2612interface dummy = { 0 };
	int rate = Machine->sample_rate;
	struct ym2612_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config ? config : &dummy;

	/* FM init */
	/* Timer Handler set */
	info->timer[0] =timer_alloc_ptr(timer_callback_2612_0, info);
	info->timer[1] =timer_alloc_ptr(timer_callback_2612_1, info);

	/* stream system initialize */
	info->stream = stream_create(0,2,rate,info,ym2612_stream_update);

	/**** initialize YM2612 ****/
	info->chip = YM2612Init(info,sndindex,clock,rate,TimerHandler,IRQHandler);

	state_save_register_func_postload_ptr(ym2612_postload, info);

	if (info->chip)
		return info;
	/* error */
	return NULL;
}


static void ym2612_stop(void *token)
{
	struct ym2612_info *info = token;
	YM2612Shutdown(info->chip);
}

static void ym2612_reset(void *token)
{
	struct ym2612_info *info = token;
	YM2612ResetChip(info->chip);
}


/************************************************/
/* Status Read for YM2612 - Chip 0              */
/************************************************/
READ8_HANDLER( YM2612_status_port_0_A_r )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  return YM2612Read(info->chip,0);
}

READ8_HANDLER( YM2612_status_port_0_B_r )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  return YM2612Read(info->chip,2);
}

/************************************************/
/* Status Read for YM2612 - Chip 1              */
/************************************************/
READ8_HANDLER( YM2612_status_port_1_A_r ) {
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  return YM2612Read(info->chip,0);
}

READ8_HANDLER( YM2612_status_port_1_B_r ) {
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  return YM2612Read(info->chip,2);
}

/************************************************/
/* Port Read for YM2612 - Chip 0                */
/************************************************/
READ8_HANDLER( YM2612_read_port_0_r ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  return YM2612Read(info->chip,1);
}

/************************************************/
/* Port Read for YM2612 - Chip 1                */
/************************************************/
READ8_HANDLER( YM2612_read_port_1_r ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  return YM2612Read(info->chip,1);
}

/************************************************/
/* Control Write for YM2612 - Chip 0            */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM2612_control_port_0_A_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  YM2612Write(info->chip,0,data);
}

WRITE8_HANDLER( YM2612_control_port_0_B_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  YM2612Write(info->chip,2,data);
}

/************************************************/
/* Control Write for YM2612 - Chip 1            */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM2612_control_port_1_A_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  YM2612Write(info->chip,0,data);
}

WRITE8_HANDLER( YM2612_control_port_1_B_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  YM2612Write(info->chip,2,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 0               */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM2612_data_port_0_A_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  YM2612Write(info->chip,1,data);
}

WRITE8_HANDLER( YM2612_data_port_0_B_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM2612,0);
  YM2612Write(info->chip,3,data);
}

/************************************************/
/* Data Write for YM2612 - Chip 1               */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM2612_data_port_1_A_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  YM2612Write(info->chip,1,data);
}
WRITE8_HANDLER( YM2612_data_port_1_B_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM2612,1);
  YM2612Write(info->chip,3,data);
}


/************************************************/
/* Status Read for YM3438 - Chip 0              */
/************************************************/
READ8_HANDLER( YM3438_status_port_0_A_r )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  return YM2612Read(info->chip,0);
}

READ8_HANDLER( YM3438_status_port_0_B_r )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  return YM2612Read(info->chip,2);
}

/************************************************/
/* Status Read for YM3438 - Chip 1              */
/************************************************/
READ8_HANDLER( YM3438_status_port_1_A_r ) {
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  return YM2612Read(info->chip,0);
}

READ8_HANDLER( YM3438_status_port_1_B_r ) {
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  return YM2612Read(info->chip,2);
}

/************************************************/
/* Port Read for YM3438 - Chip 0                */
/************************************************/
READ8_HANDLER( YM3438_read_port_0_r ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  return YM2612Read(info->chip,1);
}

/************************************************/
/* Port Read for YM3438 - Chip 1                */
/************************************************/
READ8_HANDLER( YM3438_read_port_1_r ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  return YM2612Read(info->chip,1);
}

/************************************************/
/* Control Write for YM3438 - Chip 0            */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM3438_control_port_0_A_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  YM2612Write(info->chip,0,data);
}

WRITE8_HANDLER( YM3438_control_port_0_B_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  YM2612Write(info->chip,2,data);
}

/************************************************/
/* Control Write for YM3438 - Chip 1            */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM3438_control_port_1_A_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  YM2612Write(info->chip,0,data);
}

WRITE8_HANDLER( YM3438_control_port_1_B_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  YM2612Write(info->chip,2,data);
}

/************************************************/
/* Data Write for YM3438 - Chip 0               */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM3438_data_port_0_A_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  YM2612Write(info->chip,1,data);
}

WRITE8_HANDLER( YM3438_data_port_0_B_w )
{
  struct ym2612_info *info = sndti_token(SOUND_YM3438,0);
  YM2612Write(info->chip,3,data);
}

/************************************************/
/* Data Write for YM3438 - Chip 1               */
/* Consists of 2 addresses                      */
/************************************************/
WRITE8_HANDLER( YM3438_data_port_1_A_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  YM2612Write(info->chip,1,data);
}
WRITE8_HANDLER( YM3438_data_port_1_B_w ){
  struct ym2612_info *info = sndti_token(SOUND_YM3438,1);
  YM2612Write(info->chip,3,data);
}

/**************** end of file ****************/

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ym2612_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ym2612_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ym2612_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ym2612_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = ym2612_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = ym2612_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YM2612";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ym3438_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ym3438_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ym3438_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ym2612_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = ym2612_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = ym2612_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YM3438";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

#endif
