/*****************************************************************************
 *
 *  POKEY chip emulator 4.5
 *  Copyright (c) 2000 by The MAME Team
 *
 *  Based on original info found in Ron Fries' Pokey emulator,
 *  with additions by Brad Oliver, Eric Smith and Juergen Buchmueller,
 *  paddle (a/d conversion) details from the Atari 400/800 Hardware Manual.
 *  Polynome algorithms according to info supplied by Perry McFarlane.
 *
 *  This code is subject to the MAME license, which besides other
 *  things means it is distributed as is, no warranties whatsoever.
 *  For more details read mame.txt that comes with MAME.
 *
 *  4.5:
 *  - changed the 9/17 bit polynomial formulas such that the values
 *    required for the Tempest Pokey protection will be found.
 *    Tempest expects the upper 4 bits of the RNG to appear in the
 *    lower 4 bits after four cycles, so there has to be a shift
 *    of 1 per cycle (which was not the case before). Bits #6-#13 of the
 *    new RNG give this expected result now, bits #0-7 of the 9 bit poly.
 *  - reading the RNG returns the shift register contents ^ 0xff.
 *    That way resetting the Pokey with SKCTL (which resets the
 *    polynome shifters to 0) returns the expected 0xff value.
 *  4.4:
 *  - reversed sample values to make OFF channels produce a zero signal.
 *    actually de-reversed them; don't remember that I reversed them ;-/
 *  4.3:
 *  - for POT inputs returning zero, immediately assert the ALLPOT
 *    bit after POTGO is written, otherwise start trigger timer
 *    depending on SK_PADDLE mode, either 1-228 scanlines or 1-2
 *    scanlines, depending on the SK_PADDLE bit of SKCTL.
 *  4.2:
 *  - half volume for channels which are inaudible (this should be
 *    close to the real thing).
 *  4.1:
 *  - default gain increased to closely match the old code.
 *  - random numbers repeat rate depends on POLY9 flag too!
 *  - verified sound output with many, many Atari 800 games,
 *    including the SUPPRESS_INAUDIBLE optimizations.
 *  4.0:
 *  - rewritten from scratch.
 *  - 16bit stream interface.
 *  - serout ready/complete delayed interrupts.
 *  - reworked pot analog/digital conversion timing.
 *  - optional non-indexing pokey update functions.
 *
 *****************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "pokey.h"

/* clear this to use Machine->sample_rate instead of the native rate */
#define OUTPUT_NATIVE		1

/*
 * Defining this produces much more (about twice as much)
 * but also more efficient code. Ideally this should be set
 * for processors with big code cache and for healthy compilers :)
 */
#ifndef BIG_SWITCH
#ifndef HEAVY_MACRO_USAGE
#define HEAVY_MACRO_USAGE   1
#endif
#else
#define HEAVY_MACRO_USAGE	BIG_SWITCH
#endif

#define SUPPRESS_INAUDIBLE	1

/* Four channels with a range of 0..32767 and volume 0..15 */
//#define POKEY_DEFAULT_GAIN (32767/15/4)

/*
 * But we raise the gain and risk clipping, the old Pokey did
 * this too. It defined POKEY_DEFAULT_GAIN 6 and this was
 * 6 * 15 * 4 = 360, 360/256 = 1.40625
 * I use 15/11 = 1.3636, so this is a little lower.
 */
#define POKEY_DEFAULT_GAIN (32767/11/4)

#define VERBOSE 		0
#define VERBOSE_SOUND	0
#define VERBOSE_TIMER	0
#define VERBOSE_POLY	0
#define VERBOSE_RAND	0

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#if VERBOSE_SOUND
#define LOG_SOUND(x) logerror x
#else
#define LOG_SOUND(x)
#endif

#if VERBOSE_TIMER
#define LOG_TIMER(x) logerror x
#else
#define LOG_TIMER(x)
#endif

#if VERBOSE_POLY
#define LOG_POLY(x) logerror x
#else
#define LOG_POLY(x)
#endif

#if VERBOSE_RAND
#define LOG_RAND(x) logerror x
#else
#define LOG_RAND(x)
#endif

#define CHAN1	0
#define CHAN2	1
#define CHAN3	2
#define CHAN4	3

#define TIMER1	0
#define TIMER2	1
#define TIMER4	2

/* values to add to the divisors for the different modes */
#define DIVADD_LOCLK		1
#define DIVADD_HICLK		4
#define DIVADD_HICLK_JOINED 7

/* AUDCx */
#define NOTPOLY5	0x80	/* selects POLY5 or direct CLOCK */
#define POLY4		0x40	/* selects POLY4 or POLY17 */
#define PURE		0x20	/* selects POLY4/17 or PURE tone */
#define VOLUME_ONLY 0x10	/* selects VOLUME OUTPUT ONLY */
#define VOLUME_MASK 0x0f	/* volume mask */

/* AUDCTL */
#define POLY9		0x80	/* selects POLY9 or POLY17 */
#define CH1_HICLK	0x40	/* selects 1.78979 MHz for Ch 1 */
#define CH3_HICLK	0x20	/* selects 1.78979 MHz for Ch 3 */
#define CH12_JOINED 0x10	/* clocks channel 1 w/channel 2 */
#define CH34_JOINED 0x08	/* clocks channel 3 w/channel 4 */
#define CH1_FILTER	0x04	/* selects channel 1 high pass filter */
#define CH2_FILTER	0x02	/* selects channel 2 high pass filter */
#define CLK_15KHZ	0x01	/* selects 15.6999 kHz or 63.9211 kHz */

/* IRQEN (D20E) */
#define IRQ_BREAK	0x80	/* BREAK key pressed interrupt */
#define IRQ_KEYBD	0x40	/* keyboard data ready interrupt */
#define IRQ_SERIN	0x20	/* serial input data ready interrupt */
#define IRQ_SEROR	0x10	/* serial output register ready interrupt */
#define IRQ_SEROC	0x08	/* serial output complete interrupt */
#define IRQ_TIMR4	0x04	/* timer channel #4 interrupt */
#define IRQ_TIMR2	0x02	/* timer channel #2 interrupt */
#define IRQ_TIMR1	0x01	/* timer channel #1 interrupt */

/* SKSTAT (R/D20F) */
#define SK_FRAME	0x80	/* serial framing error */
#define SK_OVERRUN	0x40	/* serial overrun error */
#define SK_KBERR	0x20	/* keyboard overrun error */
#define SK_SERIN	0x10	/* serial input high */
#define SK_SHIFT	0x08	/* shift key pressed */
#define SK_KEYBD	0x04	/* keyboard key pressed */
#define SK_SEROUT	0x02	/* serial output active */

/* SKCTL (W/D20F) */
#define SK_BREAK	0x80	/* serial out break signal */
#define SK_BPS		0x70	/* bits per second */
#define SK_FM		0x08	/* FM mode */
#define SK_PADDLE	0x04	/* fast paddle a/d conversion */
#define SK_RESET	0x03	/* reset serial/keyboard interface */

#define DIV_64		28		 /* divisor for 1.78979 MHz clock to 63.9211 kHz */
#define DIV_15		114 	 /* divisor for 1.78979 MHz clock to 15.6999 kHz */

struct POKEYregisters
{
	INT32 counter[4];		/* channel counter */
	INT32 divisor[4];		/* channel divisor (modulo value) */
	UINT32 volume[4];		/* channel volume - derived */
	UINT8 output[4];		/* channel output signal (1 active, 0 inactive) */
	UINT8 audible[4];		/* channel plays an audible tone/effect */
	UINT32 samplerate_24_8; /* sample rate in 24.8 format */
	UINT32 samplepos_fract; /* sample position fractional part */
	UINT32 samplepos_whole; /* sample position whole part */
	UINT32 polyadjust;		/* polynome adjustment */
    UINT32 p4;              /* poly4 index */
    UINT32 p5;              /* poly5 index */
    UINT32 p9;              /* poly9 index */
    UINT32 p17;             /* poly17 index */
	UINT32 r9;				/* rand9 index */
    UINT32 r17;             /* rand17 index */
	UINT32 clockmult;		/* clock multiplier */
    sound_stream * channel; /* streams channel */
	void *timer[3]; 		/* timers for channel 1,2 and 4 events */
	double timer_period[3];	/* computed periods for these timers */
	int timer_param[3];		/* computed parameters for these timers */
    void *rtimer;           /* timer for calculating the random offset */
	void *ptimer[8];		/* pot timers */
	read8_handler pot_r[8];
	read8_handler allpot_r;
	read8_handler serin_r;
	write8_handler serout_w;
	void (*interrupt_cb)(int mask);
    UINT8 AUDF[4];          /* AUDFx (D200, D202, D204, D206) */
	UINT8 AUDC[4];			/* AUDCx (D201, D203, D205, D207) */
	UINT8 POTx[8];			/* POTx   (R/D200-D207) */
	UINT8 AUDCTL;			/* AUDCTL (W/D208) */
	UINT8 ALLPOT;			/* ALLPOT (R/D208) */
	UINT8 KBCODE;			/* KBCODE (R/D209) */
	UINT8 RANDOM;			/* RANDOM (R/D20A) */
	UINT8 SERIN;			/* SERIN  (R/D20D) */
	UINT8 SEROUT;			/* SEROUT (W/D20D) */
	UINT8 IRQST;			/* IRQST  (R/D20E) */
	UINT8 IRQEN;			/* IRQEN  (W/D20E) */
	UINT8 SKSTAT;			/* SKSTAT (R/D20F) */
	UINT8 SKCTL;			/* SKCTL  (W/D20F) */
	struct POKEYinterface intf;
	int clock;
	int index;

	UINT8 poly4[0x0f];
	UINT8 poly5[0x1f];
	UINT8 poly9[0x1ff];
	UINT8 poly17[0x1ffff];

	UINT8 rand9[0x1ff];
	UINT8 rand17[0x1ffff];
};


#define P4(chip)  chip->poly4[chip->p4]
#define P5(chip)  chip->poly5[chip->p5]
#define P9(chip)  chip->poly9[chip->p9]
#define P17(chip) chip->poly17[chip->p17]

static void pokey_timer_expire_1(void *param);
static void pokey_timer_expire_2(void *param);
static void pokey_timer_expire_4(void *param);
static void pokey_pot_trigger_0(void *param);
static void pokey_pot_trigger_1(void *param);
static void pokey_pot_trigger_2(void *param);
static void pokey_pot_trigger_3(void *param);
static void pokey_pot_trigger_4(void *param);
static void pokey_pot_trigger_5(void *param);
static void pokey_pot_trigger_6(void *param);
static void pokey_pot_trigger_7(void *param);


#define SAMPLE	-1

#define ADJUST_EVENT(chip)												\
	chip->counter[CHAN1] -= event;										\
	chip->counter[CHAN2] -= event;										\
	chip->counter[CHAN3] -= event;										\
	chip->counter[CHAN4] -= event;										\
	chip->samplepos_whole -= event;										\
	chip->polyadjust += event

#if SUPPRESS_INAUDIBLE

#define PROCESS_CHANNEL(chip,ch)                                        \
	int toggle = 0; 													\
	ADJUST_EVENT(chip); 												\
	/* reset the channel counter */ 									\
	if( chip->audible[ch] )												\
		chip->counter[ch] = chip->divisor[ch];							\
	else																\
		chip->counter[ch] = 0x7fffffff;									\
	chip->p4 = (chip->p4+chip->polyadjust)%0x0000f;						\
	chip->p5 = (chip->p5+chip->polyadjust)%0x0001f;						\
	chip->p9 = (chip->p9+chip->polyadjust)%0x001ff;						\
	chip->p17 = (chip->p17+chip->polyadjust)%0x1ffff; 					\
	chip->polyadjust = 0; 												\
	if( (chip->AUDC[ch] & NOTPOLY5) || P5(chip) ) 						\
	{																	\
		if( chip->AUDC[ch] & PURE )										\
			toggle = 1; 												\
		else															\
		if( chip->AUDC[ch] & POLY4 )									\
			toggle = chip->output[ch] == !P4(chip);						\
		else															\
		if( chip->AUDCTL & POLY9 )										\
			toggle = chip->output[ch] == !P9(chip);						\
		else															\
			toggle = chip->output[ch] == !P17(chip);					\
	}																	\
	if( toggle )														\
	{																	\
		if( chip->audible[ch] )											\
		{																\
			if( chip->output[ch] )										\
				sum -= chip->volume[ch];								\
			else														\
				sum += chip->volume[ch];								\
		}																\
		chip->output[ch] ^= 1;											\
	}																	\
	/* is this a filtering channel (3/4) and is the filter active? */	\
	if( chip->AUDCTL & ((CH1_FILTER|CH2_FILTER) & (0x10 >> ch)) ) 		\
    {                                                                   \
		if( chip->output[ch-2] )										\
        {                                                               \
			chip->output[ch-2] = 0;										\
			if( chip->audible[ch] )										\
				sum -= chip->volume[ch-2];								\
        }                                                               \
    }                                                                   \

#else

#define PROCESS_CHANNEL(chip,ch)                                        \
	int toggle = 0; 													\
	ADJUST_EVENT(chip); 												\
	/* reset the channel counter */ 									\
	chip->counter[ch] = p[chip].divisor[ch];							\
	chip->p4 = (chip->p4+chip->polyadjust)%0x0000f;						\
	chip->p5 = (chip->p5+chip->polyadjust)%0x0001f;						\
	chip->p9 = (chip->p9+chip->polyadjust)%0x001ff;						\
	chip->p17 = (chip->p17+chip->polyadjust)%0x1ffff; 					\
	chip->polyadjust = 0; 												\
	if( (chip->AUDC[ch] & NOTPOLY5) || P5(chip) ) 						\
	{																	\
		if( chip->AUDC[ch] & PURE )										\
			toggle = 1; 												\
		else															\
		if( chip->AUDC[ch] & POLY4 )									\
			toggle = chip->output[ch] == !P4(chip);						\
		else															\
		if( chip->AUDCTL & POLY9 )										\
			toggle = chip->output[ch] == !P9(chip);						\
		else															\
			toggle = chip->output[ch] == !P17(chip);					\
	}																	\
	if( toggle )														\
	{																	\
		if( chip->output[ch] )											\
			sum -= chip->volume[ch];									\
		else															\
			sum += chip->volume[ch];									\
		chip->output[ch] ^= 1;											\
	}																	\
	/* is this a filtering channel (3/4) and is the filter active? */	\
	if( chip->AUDCTL & ((CH1_FILTER|CH2_FILTER) & (0x10 >> ch)) ) 		\
    {                                                                   \
		if( chip->output[ch-2] )										\
        {                                                               \
			chip->output[ch-2] = 0;										\
			sum -= chip->volume[ch-2];									\
        }                                                               \
    }                                                                   \

#endif

#if OUTPUT_NATIVE
#define PROCESS_SAMPLE(chip)                                            \
    ADJUST_EVENT(chip);                                                 \
    /* adjust the sample position */                                    \
	chip->samplepos_whole++;											\
	/* store sum of output signals into the buffer */					\
	*buffer++ = (sum > 0x7fff) ? 0x7fff : sum;							\
	length--
#else
#define PROCESS_SAMPLE(chip)                                            \
    ADJUST_EVENT(chip);                                                 \
    /* adjust the sample position */                                    \
	chip->samplepos_fract += chip->samplerate_24_8; 					\
	if( chip->samplepos_fract & 0xffffff00 )							\
	{																	\
		chip->samplepos_whole += chip->samplepos_fract>>8;				\
		chip->samplepos_fract &= 0x000000ff;							\
	}																	\
	/* store sum of output signals into the buffer */					\
	*buffer++ = (sum > 0x7fff) ? 0x7fff : sum;							\
	length--
#endif

#if HEAVY_MACRO_USAGE

/*
 * This version of PROCESS_POKEY repeats the search for the minimum
 * event value without using an index to the channel. That way the
 * PROCESS_CHANNEL macros can be called with fixed values and expand
 * to much more efficient code
 */

#define PROCESS_POKEY(chip) 											\
	UINT32 sum = 0; 													\
	if( chip->output[CHAN1] ) 											\
		sum += chip->volume[CHAN1];										\
	if( chip->output[CHAN2] ) 											\
		sum += chip->volume[CHAN2];										\
	if( chip->output[CHAN3] ) 											\
		sum += chip->volume[CHAN3];										\
	if( chip->output[CHAN4] ) 											\
		sum += chip->volume[CHAN4];										\
    while( length > 0 )                                                 \
	{																	\
		if( chip->counter[CHAN1] < chip->samplepos_whole )				\
		{																\
			if( chip->counter[CHAN2] <  chip->counter[CHAN1] ) 			\
			{															\
				if( chip->counter[CHAN3] <  chip->counter[CHAN2] )		\
				{														\
					if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 	\
					{													\
						UINT32 event = chip->counter[CHAN4];			\
                        PROCESS_CHANNEL(chip,CHAN4);                    \
					}													\
					else												\
					{													\
						UINT32 event = chip->counter[CHAN3];			\
                        PROCESS_CHANNEL(chip,CHAN3);                    \
					}													\
				}														\
				else													\
				if( chip->counter[CHAN4] < chip->counter[CHAN2] )  		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
                else                                                    \
				{														\
					UINT32 event = chip->counter[CHAN2];				\
                    PROCESS_CHANNEL(chip,CHAN2);                        \
				}														\
            }                                                           \
			else														\
			if( chip->counter[CHAN3] < chip->counter[CHAN1] ) 			\
			{															\
				if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
                else                                                    \
				{														\
					UINT32 event = chip->counter[CHAN3];				\
                    PROCESS_CHANNEL(chip,CHAN3);                        \
				}														\
            }                                                           \
			else														\
			if( chip->counter[CHAN4] < chip->counter[CHAN1] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
            else                                                        \
			{															\
				UINT32 event = chip->counter[CHAN1];					\
                PROCESS_CHANNEL(chip,CHAN1);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN2] < chip->samplepos_whole )				\
		{																\
			if( chip->counter[CHAN3] < chip->counter[CHAN2] ) 			\
			{															\
				if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
				else													\
				{														\
					UINT32 event = chip->counter[CHAN3];				\
                    PROCESS_CHANNEL(chip,CHAN3);                        \
				}														\
			}															\
			else														\
			if( chip->counter[CHAN4] < chip->counter[CHAN2] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
			else														\
			{															\
				UINT32 event = chip->counter[CHAN2];					\
                PROCESS_CHANNEL(chip,CHAN2);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN3] < chip->samplepos_whole )				\
        {                                                               \
			if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
			else														\
			{															\
				UINT32 event = chip->counter[CHAN3];					\
                PROCESS_CHANNEL(chip,CHAN3);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN4] < chip->samplepos_whole )				\
		{																\
			UINT32 event = chip->counter[CHAN4];						\
            PROCESS_CHANNEL(chip,CHAN4);                                \
        }                                                               \
		else															\
		{																\
			UINT32 event = chip->samplepos_whole; 						\
			PROCESS_SAMPLE(chip);										\
		}																\
	}																	\
	timer_adjust(chip->rtimer, TIME_NEVER, 0, 0)

#else   /* no HEAVY_MACRO_USAGE */
/*
 * And this version of PROCESS_POKEY uses event and channel variables
 * so that the PROCESS_CHANNEL macro needs to index memory at runtime.
 */

#define PROCESS_POKEY(chip)                                             \
	UINT32 sum = 0; 													\
	if( chip->output[CHAN1] ) 											\
		sum += chip->volume[CHAN1];										\
	if( chip->output[CHAN2] ) 											\
		sum += chip->volume[CHAN2];										\
	if( chip->output[CHAN3] ) 											\
		sum += chip->volume[CHAN3];										\
	if( chip->output[CHAN4] ) 											\
        sum += chip->volume[CHAN4];                               		\
    while( length > 0 )                                                 \
	{																	\
		UINT32 event = chip->samplepos_whole; 							\
		UINT32 channel = SAMPLE;										\
		if( chip->counter[CHAN1] < event )								\
        {                                                               \
			event = chip->counter[CHAN1]; 								\
			channel = CHAN1;											\
		}																\
		if( chip->counter[CHAN2] < event )								\
        {                                                               \
			event = chip->counter[CHAN2]; 								\
			channel = CHAN2;											\
        }                                                               \
		if( chip->counter[CHAN3] < event )								\
        {                                                               \
			event = chip->counter[CHAN3]; 								\
			channel = CHAN3;											\
        }                                                               \
		if( chip->counter[CHAN4] < event )								\
        {                                                               \
			event = chip->counter[CHAN4]; 								\
			channel = CHAN4;											\
        }                                                               \
        if( channel == SAMPLE )                                         \
		{																\
            PROCESS_SAMPLE(chip);                                       \
        }                                                               \
		else															\
		{																\
			PROCESS_CHANNEL(chip,channel);								\
		}																\
	}																	\
	timer_adjust(chip->rtimer, TIME_NEVER, 0, 0)

#endif


void pokey_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct POKEYregisters *chip = param;
	stream_sample_t *buffer = _buffer[0];
	PROCESS_POKEY(chip);
}


static void poly_init(UINT8 *poly, int size, int left, int right, int add)
{
	int mask = (1 << size) - 1;
    int i, x = 0;

	LOG_POLY(("poly %d\n", size));
	for( i = 0; i < mask; i++ )
	{
		*poly++ = x & 1;
		LOG_POLY(("%05x: %d\n", x, x&1));
        /* calculate next bit */
		x = ((x << left) + (x >> right) + add) & mask;
	}
}

static void rand_init(UINT8 *rng, int size, int left, int right, int add)
{
    int mask = (1 << size) - 1;
    int i, x = 0;

	LOG_RAND(("rand %d\n", size));
    for( i = 0; i < mask; i++ )
	{
		if (size == 17)
			*rng = x >> 6;	/* use bits 6..13 */
		else
			*rng = x;		/* use bits 0..7 */
        LOG_RAND(("%05x: %02x\n", x, *rng));
        rng++;
        /* calculate next bit */
		x = ((x << left) + (x >> right) + add) & mask;
	}
}


static void register_for_save(struct POKEYregisters *chip, int index)
{
	state_save_register_item_array("pokey", index, chip->counter);
	state_save_register_item_array("pokey", index, chip->divisor);
	state_save_register_item_array("pokey", index, chip->volume);
	state_save_register_item_array("pokey", index, chip->output);
	state_save_register_item_array("pokey", index, chip->audible);
	state_save_register_item("pokey", index, chip->samplepos_fract);
	state_save_register_item("pokey", index, chip->samplepos_whole);
	state_save_register_item("pokey", index, chip->polyadjust);
	state_save_register_item("pokey", index, chip->p4);
	state_save_register_item("pokey", index, chip->p5);
	state_save_register_item("pokey", index, chip->p9);
	state_save_register_item("pokey", index, chip->p17);
	state_save_register_item("pokey", index, chip->r9);
	state_save_register_item("pokey", index, chip->r17);
	state_save_register_item("pokey", index, chip->clockmult);
	state_save_register_item_array("pokey", index, chip->timer_period);
	state_save_register_item_array("pokey", index, chip->timer_param);
	state_save_register_item_array("pokey", index, chip->AUDF);
	state_save_register_item_array("pokey", index, chip->AUDC);
	state_save_register_item_array("pokey", index, chip->POTx);
	state_save_register_item("pokey", index, chip->AUDCTL);
	state_save_register_item("pokey", index, chip->ALLPOT);
	state_save_register_item("pokey", index, chip->KBCODE);
	state_save_register_item("pokey", index, chip->RANDOM);
	state_save_register_item("pokey", index, chip->SERIN);
	state_save_register_item("pokey", index, chip->SEROUT);
	state_save_register_item("pokey", index, chip->IRQST);
	state_save_register_item("pokey", index, chip->IRQEN);
	state_save_register_item("pokey", index, chip->SKSTAT);
	state_save_register_item("pokey", index, chip->SKCTL);
}


static void *pokey_start(int sndindex, int clock, const void *config)
{
	int sample_rate = OUTPUT_NATIVE ? clock : Machine->sample_rate;
	struct POKEYregisters *chip;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	if (config)
		memcpy(&chip->intf, config, sizeof(struct POKEYinterface));
	chip->clock = clock;
	chip->index = sndindex;

	/* initialize the poly counters */
	poly_init(chip->poly4,   4, 3, 1, 0x00004);
	poly_init(chip->poly5,   5, 3, 2, 0x00008);
	poly_init(chip->poly9,   9, 8, 1, 0x00180);
	poly_init(chip->poly17, 17,16, 1, 0x1c000);

	/* initialize the random arrays */
	rand_init(chip->rand9,   9, 8, 1, 0x00180);
	rand_init(chip->rand17, 17,16, 1, 0x1c000);

	chip->samplerate_24_8 = (clock << 8) / sample_rate;
	chip->divisor[CHAN1] = 4;
	chip->divisor[CHAN2] = 4;
	chip->divisor[CHAN3] = 4;
	chip->divisor[CHAN4] = 4;
	chip->clockmult = DIV_64;
	chip->KBCODE = 0x09;		 /* Atari 800 'no key' */
	chip->SKCTL = SK_RESET;	 /* let the RNG run after reset */
	chip->rtimer = timer_alloc(NULL);

	chip->timer[0] = timer_alloc_ptr(pokey_timer_expire_1, chip);
	chip->timer[1] = timer_alloc_ptr(pokey_timer_expire_2, chip);
	chip->timer[2] = timer_alloc_ptr(pokey_timer_expire_4, chip);

	chip->ptimer[0] = timer_alloc_ptr(pokey_pot_trigger_0, chip);
	chip->ptimer[1] = timer_alloc_ptr(pokey_pot_trigger_1, chip);
	chip->ptimer[2] = timer_alloc_ptr(pokey_pot_trigger_2, chip);
	chip->ptimer[3] = timer_alloc_ptr(pokey_pot_trigger_3, chip);
	chip->ptimer[4] = timer_alloc_ptr(pokey_pot_trigger_4, chip);
	chip->ptimer[5] = timer_alloc_ptr(pokey_pot_trigger_5, chip);
	chip->ptimer[6] = timer_alloc_ptr(pokey_pot_trigger_6, chip);
	chip->ptimer[7] = timer_alloc_ptr(pokey_pot_trigger_7, chip);

	chip->pot_r[0] = chip->intf.pot_r[0];
	chip->pot_r[1] = chip->intf.pot_r[1];
	chip->pot_r[2] = chip->intf.pot_r[2];
	chip->pot_r[3] = chip->intf.pot_r[3];
	chip->pot_r[4] = chip->intf.pot_r[4];
	chip->pot_r[5] = chip->intf.pot_r[5];
	chip->pot_r[6] = chip->intf.pot_r[6];
	chip->pot_r[7] = chip->intf.pot_r[7];
	chip->allpot_r = chip->intf.allpot_r;
	chip->serin_r = chip->intf.serin_r;
	chip->serout_w = chip->intf.serout_w;
	chip->interrupt_cb = chip->intf.interrupt_cb;

	chip->channel = stream_create(0, 1, sample_rate, chip, pokey_update);

	register_for_save(chip, sndindex);

    return chip;
}

static void pokey_timer_expire_common(struct POKEYregisters *p, int timers)
{
	LOG_TIMER(("POKEY #%p timer %d with IRQEN $%02x\n", chip, timers, p->IRQEN));

    /* check if some of the requested timer interrupts are enabled */
	timers &= p->IRQEN;

    if( timers )
    {
		/* set the enabled timer irq status bits */
		p->IRQST |= timers;
        /* call back an application supplied function to handle the interrupt */
		if( p->interrupt_cb )
			(*p->interrupt_cb)(timers);
    }
}

static void pokey_timer_expire_1(void *param)
{
	struct POKEYregisters *chip = param;
	pokey_timer_expire_common(chip, chip->timer_param[TIMER1]);
}
static void pokey_timer_expire_2(void *param)
{
	struct POKEYregisters *chip = param;
	pokey_timer_expire_common(chip, chip->timer_param[TIMER2]);
}
static void pokey_timer_expire_4(void *param)
{
	struct POKEYregisters *chip = param;
	pokey_timer_expire_common(chip, chip->timer_param[TIMER4]);
}

#if VERBOSE_SOUND
static char *audc2str(int val)
{
	static char buff[80];
	if( val & NOTPOLY5 )
	{
		if( val & PURE )
			strcpy(buff,"pure");
		else
		if( val & POLY4 )
			strcpy(buff,"poly4");
		else
			strcpy(buff,"poly9/17");
	}
	else
	{
		if( val & PURE )
			strcpy(buff,"poly5");
		else
		if( val & POLY4 )
			strcpy(buff,"poly4+poly5");
		else
			strcpy(buff,"poly9/17+poly5");
    }
	return buff;
}

static char *audctl2str(int val)
{
	static char buff[80];
	if( val & POLY9 )
		strcpy(buff,"poly9");
	else
		strcpy(buff,"poly17");
	if( val & CH1_HICLK )
		strcat(buff,"+ch1hi");
	if( val & CH3_HICLK )
		strcat(buff,"+ch3hi");
	if( val & CH12_JOINED )
		strcat(buff,"+ch1/2");
	if( val & CH34_JOINED )
		strcat(buff,"+ch3/4");
	if( val & CH1_FILTER )
		strcat(buff,"+ch1filter");
	if( val & CH2_FILTER )
		strcat(buff,"+ch2filter");
	if( val & CLK_15KHZ )
		strcat(buff,"+clk15");
    return buff;
}
#endif

static void pokey_serin_ready(void *param)
{
	struct POKEYregisters *p = param;
    if( p->IRQEN & IRQ_SERIN )
	{
		/* set the enabled timer irq status bits */
		p->IRQST |= IRQ_SERIN;
		/* call back an application supplied function to handle the interrupt */
		if( p->interrupt_cb )
			(*p->interrupt_cb)(IRQ_SERIN);
	}
}

static void pokey_serout_ready(void *param)
{
	struct POKEYregisters *p = param;
    if( p->IRQEN & IRQ_SEROR )
	{
		p->IRQST |= IRQ_SEROR;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(IRQ_SEROR);
	}
}

static void pokey_serout_complete(void *param)
{
	struct POKEYregisters *p = param;
    if( p->IRQEN & IRQ_SEROC )
	{
		p->IRQST |= IRQ_SEROC;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(IRQ_SEROC);
	}
}

static void pokey_pot_trigger_common(struct POKEYregisters *p, int pot)
{
	LOG(("POKEY #%p POT%d triggers after %dus\n", p, pot, (int)(1000000ul*timer_timeelapsed(p->ptimer[pot]))));
	p->ALLPOT &= ~(1 << pot);	/* set the enabled timer irq status bits */
}

static void pokey_pot_trigger_0(void *param) { pokey_pot_trigger_common(param, 0); }
static void pokey_pot_trigger_1(void *param) { pokey_pot_trigger_common(param, 1); }
static void pokey_pot_trigger_2(void *param) { pokey_pot_trigger_common(param, 2); }
static void pokey_pot_trigger_3(void *param) { pokey_pot_trigger_common(param, 3); }
static void pokey_pot_trigger_4(void *param) { pokey_pot_trigger_common(param, 4); }
static void pokey_pot_trigger_5(void *param) { pokey_pot_trigger_common(param, 5); }
static void pokey_pot_trigger_6(void *param) { pokey_pot_trigger_common(param, 6); }
static void pokey_pot_trigger_7(void *param) { pokey_pot_trigger_common(param, 7); }

/* A/D conversion time:
 * In normal, slow mode (SKCTL bit SK_PADDLE is clear) the conversion
 * takes N scanlines, where N is the paddle value. A single scanline
 * takes approximately 64us to finish (1.78979MHz clock).
 * In quick mode (SK_PADDLE set) the conversion is done very fast
 * (takes two scalines) but the result is not as accurate.
 */
#define AD_TIME (double)(((p->SKCTL & SK_PADDLE) ? 64.0*2/228 : 64.0) * FREQ_17_EXACT / p->clock)

static void pokey_potgo(struct POKEYregisters *p)
{
    int pot;

	LOG(("POKEY #%d pokey_potgo\n", chip));

    p->ALLPOT = 0xff;

    for( pot = 0; pot < 8; pot++ )
	{
		p->POTx[pot] = 0xff;
		if( p->pot_r[pot] )
		{
			int r = (*p->pot_r[pot])(pot);
			LOG(("POKEY #%d pot_r(%d) returned $%02x\n", p->index, pot, r));
			if( r != -1 )
			{
				if (r > 228)
                    r = 228;
                /* final value */
                p->POTx[pot] = r;
				timer_adjust_ptr(p->ptimer[pot], TIME_IN_USEC(r * AD_TIME), 0);
			}
		}
	}
}

static int pokey_register_r(int chip, int offs)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
    int data = 0, pot;
	UINT32 adjust = 0;

#ifdef MAME_DEBUG
	if( !p )
	{
		logerror("POKEY #%d is >= number of Pokeys!\n", chip);
		return data;
	}
#endif

    switch (offs & 15)
	{
	case POT0_C: case POT1_C: case POT2_C: case POT3_C:
	case POT4_C: case POT5_C: case POT6_C: case POT7_C:
		pot = offs & 7;
		if( p->pot_r[pot] )
		{
			/*
             * If the conversion is not yet finished (ptimer running),
             * get the current value by the linear interpolation of
             * the final value using the elapsed time.
             */
			if( p->ALLPOT & (1 << pot) )
			{
				data = (UINT8)(timer_timeelapsed(p->ptimer[pot]) / AD_TIME);
				LOG(("POKEY #%d read POT%d (interpolated) $%02x\n", chip, pot, data));
            }
			else
			{
				data = p->POTx[pot];
				LOG(("POKEY #%d read POT%d (final value)  $%02x\n", chip, pot, data));
			}
		}
		else
		logerror("PC %04x: warning - read p[chip] #%d POT%d\n", activecpu_get_pc(), chip, pot);
		break;

    case ALLPOT_C:
		/****************************************************************
         * If the 2 least significant bits of SKCTL are 0, the ALLPOTs
         * are disabled (SKRESET). Thanks to MikeJ for pointing this out.
         ****************************************************************/
    	if( (p->SKCTL & SK_RESET) == 0)
    	{
    		data = 0;
			LOG(("POKEY #%d ALLPOT internal $%02x (reset)\n", chip, data));
		}
		else if( p->allpot_r )
		{
			data = (*p->allpot_r)(offs);
			LOG(("POKEY #%d ALLPOT callback $%02x\n", chip, data));
		}
		else
		{
			data = p->ALLPOT;
			LOG(("POKEY #%d ALLPOT internal $%02x\n", chip, data));
		}
		break;

	case KBCODE_C:
		data = p->KBCODE;
		break;

	case RANDOM_C:
		/****************************************************************
         * If the 2 least significant bits of SKCTL are 0, the random
         * number generator is disabled (SKRESET). Thanks to Eric Smith
         * for pointing out this critical bit of info! If the random
         * number generator is enabled, get a new random number. Take
         * the time gone since the last read into account and read the
         * new value from an appropriate offset in the rand17 table.
         ****************************************************************/
		if( p->SKCTL & SK_RESET )
		{
			adjust = (UINT32)(timer_timeelapsed(p->rtimer) * p->clock + 0.5);
			p->r9 = (p->r9 + adjust) % 0x001ff;
			p->r17 = (p->r17 + adjust) % 0x1ffff;
		}
		else
		{
			adjust = 1;
			p->r9 = 0;
			p->r17 = 0;
            LOG_RAND(("POKEY #%d rand17 freezed (SKCTL): $%02x\n", chip, p->RANDOM));
		}
		if( p->AUDCTL & POLY9 )
		{
			p->RANDOM = p->rand9[p->r9];
			LOG_RAND(("POKEY #%d adjust %u rand9[$%05x]: $%02x\n", chip, adjust, p->r9, p->RANDOM));
		}
		else
		{
			p->RANDOM = p->rand17[p->r17];
			LOG_RAND(("POKEY #%d adjust %u rand17[$%05x]: $%02x\n", chip, adjust, p->r17, p->RANDOM));
		}
		if (adjust > 0)
        	timer_adjust(p->rtimer, TIME_NEVER, 0, 0);
		data = p->RANDOM ^ 0xff;
		break;

	case SERIN_C:
		if( p->serin_r )
			p->SERIN = (*p->serin_r)(offs);
		data = p->SERIN;
		LOG(("POKEY #%d SERIN  $%02x\n", chip, data));
		break;

	case IRQST_C:
		/* IRQST is an active low input port; we keep it active high */
		/* internally to ease the (un-)masking of bits */
		data = p->IRQST ^ 0xff;
		LOG(("POKEY #%d IRQST  $%02x\n", chip, data));
		break;

	case SKSTAT_C:
		/* SKSTAT is also an active low input port */
		data = p->SKSTAT ^ 0xff;
		LOG(("POKEY #%d SKSTAT $%02x\n", chip, data));
		break;

	default:
		LOG(("POKEY #%d register $%02x\n", chip, offs));
        break;
    }
    return data;
}

READ8_HANDLER( pokey1_r )
{
	return pokey_register_r(0, offset);
}

READ8_HANDLER( pokey2_r )
{
	return pokey_register_r(1, offset);
}

READ8_HANDLER( pokey3_r )
{
	return pokey_register_r(2, offset);
}

READ8_HANDLER( pokey4_r )
{
	return pokey_register_r(3, offset);
}

READ8_HANDLER( quad_pokey_r )
{
	int pokey_num = (offset >> 3) & ~0x04;
	int control = (offset & 0x20) >> 2;
	int pokey_reg = (offset % 8) | control;

	return pokey_register_r(pokey_num, pokey_reg);
}


void pokey_register_w(int chip, int offs, int data)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
	int ch_mask = 0, new_val;

#ifdef MAME_DEBUG
	if( !p )
	{
		logerror("POKEY #%d is >= number of Pokeys!\n", chip);
		return;
	}
#endif
	stream_update(p->channel, 0);

    /* determine which address was changed */
	switch (offs & 15)
    {
    case AUDF1_C:
		if( data == p->AUDF[CHAN1] )
            return;
		LOG_SOUND(("POKEY #%d AUDF1  $%02x\n", chip, data));
		p->AUDF[CHAN1] = data;
        ch_mask = 1 << CHAN1;
		if( p->AUDCTL & CH12_JOINED )		/* if ch 1&2 tied together */
            ch_mask |= 1 << CHAN2;    /* then also change on ch2 */
        break;

    case AUDC1_C:
		if( data == p->AUDC[CHAN1] )
            return;
		LOG_SOUND(("POKEY #%d AUDC1  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN1] = data;
        ch_mask = 1 << CHAN1;
        break;

    case AUDF2_C:
		if( data == p->AUDF[CHAN2] )
            return;
		LOG_SOUND(("POKEY #%d AUDF2  $%02x\n", chip, data));
		p->AUDF[CHAN2] = data;
        ch_mask = 1 << CHAN2;
        break;

    case AUDC2_C:
		if( data == p->AUDC[CHAN2] )
            return;
		LOG_SOUND(("POKEY #%d AUDC2  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN2] = data;
        ch_mask = 1 << CHAN2;
        break;

    case AUDF3_C:
		if( data == p->AUDF[CHAN3] )
            return;
		LOG_SOUND(("POKEY #%d AUDF3  $%02x\n", chip, data));
		p->AUDF[CHAN3] = data;
        ch_mask = 1 << CHAN3;

		if( p->AUDCTL & CH34_JOINED )	/* if ch 3&4 tied together */
            ch_mask |= 1 << CHAN4;  /* then also change on ch4 */
        break;

    case AUDC3_C:
		if( data == p->AUDC[CHAN3] )
            return;
		LOG_SOUND(("POKEY #%d AUDC3  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN3] = data;
        ch_mask = 1 << CHAN3;
        break;

    case AUDF4_C:
		if( data == p->AUDF[CHAN4] )
            return;
		LOG_SOUND(("POKEY #%d AUDF4  $%02x\n", chip, data));
		p->AUDF[CHAN4] = data;
        ch_mask = 1 << CHAN4;
        break;

    case AUDC4_C:
		if( data == p->AUDC[CHAN4] )
            return;
		LOG_SOUND(("POKEY #%d AUDC4  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN4] = data;
        ch_mask = 1 << CHAN4;
        break;

    case AUDCTL_C:
		if( data == p->AUDCTL )
            return;
		LOG_SOUND(("POKEY #%d AUDCTL $%02x (%s)\n", chip, data, audctl2str(data)));
		p->AUDCTL = data;
        ch_mask = 15;       /* all channels */
        /* determine the base multiplier for the 'div by n' calculations */
		p->clockmult = (p->AUDCTL & CLK_15KHZ) ? DIV_15 : DIV_64;
        break;

    case STIMER_C:
        /* first remove any existing timers */
		LOG_TIMER(("POKEY #%d STIMER $%02x\n", chip, data));

		timer_adjust_ptr(p->timer[TIMER1], TIME_NEVER, 0);
		timer_adjust_ptr(p->timer[TIMER2], TIME_NEVER, 0);
		timer_adjust_ptr(p->timer[TIMER4], TIME_NEVER, 0);

        /* reset all counters to zero (side effect) */
		p->polyadjust = 0;
		p->counter[CHAN1] = 0;
		p->counter[CHAN2] = 0;
		p->counter[CHAN3] = 0;
		p->counter[CHAN4] = 0;

        /* joined chan#1 and chan#2 ? */
		if( p->AUDCTL & CH12_JOINED )
        {
			if( p->divisor[CHAN2] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer1+2 after %d clocks\n", chip, p->divisor[CHAN2]));
				/* set timer #1 _and_ #2 event after timer_div clocks of joined CHAN1+CHAN2 */
				p->timer_period[TIMER2] = 1.0 * p->divisor[CHAN2] / p->clock;
				p->timer_param[TIMER2] = (chip<<3)|IRQ_TIMR2|IRQ_TIMR1;
				timer_adjust_ptr(p->timer[TIMER2], p->timer_period[TIMER2], p->timer_period[TIMER2]);
			}
        }
        else
        {
			if( p->divisor[CHAN1] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer1 after %d clocks\n", chip, p->divisor[CHAN1]));
				/* set timer #1 event after timer_div clocks of CHAN1 */
				p->timer_period[TIMER1] = 1.0 * p->divisor[CHAN1] / p->clock;
				p->timer_param[TIMER1] = (chip<<3)|IRQ_TIMR1;
				timer_adjust_ptr(p->timer[TIMER1], p->timer_period[TIMER1], p->timer_period[TIMER1]);
			}

			if( p->divisor[CHAN2] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer2 after %d clocks\n", chip, p->divisor[CHAN2]));
				/* set timer #2 event after timer_div clocks of CHAN2 */
				p->timer_period[TIMER2] = 1.0 * p->divisor[CHAN2] / p->clock;
				p->timer_param[TIMER2] = (chip<<3)|IRQ_TIMR2;
				timer_adjust_ptr(p->timer[TIMER2], p->timer_period[TIMER2], p->timer_period[TIMER2]);
			}
        }

		/* Note: p[chip] does not have a timer #3 */

		if( p->AUDCTL & CH34_JOINED )
        {
            /* not sure about this: if audc4 == 0000xxxx don't start timer 4 ? */
			if( p->AUDC[CHAN4] & 0xf0 )
            {
				if( p->divisor[CHAN4] > 4 )
				{
					LOG_TIMER(("POKEY #%d timer4 after %d clocks\n", chip, p->divisor[CHAN4]));
					/* set timer #4 event after timer_div clocks of CHAN4 */
					p->timer_period[TIMER4] = 1.0 * p->divisor[CHAN4] / p->clock;
					p->timer_param[TIMER4] = (chip<<3)|IRQ_TIMR4;
					timer_adjust_ptr(p->timer[TIMER4], p->timer_period[TIMER4], p->timer_period[TIMER4]);
				}
            }
        }
        else
        {
			if( p->divisor[CHAN4] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer4 after %d clocks\n", chip, p->divisor[CHAN4]));
				/* set timer #4 event after timer_div clocks of CHAN4 */
				p->timer_period[TIMER4] = 1.0 * p->divisor[CHAN4] / p->clock;
				p->timer_param[TIMER4] = (chip<<3)|IRQ_TIMR4;
				timer_adjust_ptr(p->timer[TIMER4], p->timer_period[TIMER4], p->timer_period[TIMER4]);
			}
        }

		timer_enable(p->timer[TIMER1], p->IRQEN & IRQ_TIMR1);
		timer_enable(p->timer[TIMER2], p->IRQEN & IRQ_TIMR2);
		timer_enable(p->timer[TIMER4], p->IRQEN & IRQ_TIMR4);
        break;

    case SKREST_C:
        /* reset SKSTAT */
		LOG(("POKEY #%d SKREST $%02x\n", chip, data));
		p->SKSTAT &= ~(SK_FRAME|SK_OVERRUN|SK_KBERR);
        break;

    case POTGO_C:
		LOG(("POKEY #%d POTGO  $%02x\n", chip, data));
		pokey_potgo(p);
        break;

    case SEROUT_C:
		LOG(("POKEY #%d SEROUT $%02x\n", chip, data));
		if (p->serout_w)
			(*p->serout_w)(offs, data);
		p->SKSTAT |= SK_SEROUT;
        /*
         * These are arbitrary values, tested with some custom boot
         * loaders from Ballblazer and Escape from Fractalus
         * The real times are unknown
         */
        timer_set_ptr(TIME_IN_USEC(200), p, pokey_serout_ready);
        /* 10 bits (assumption 1 start, 8 data and 1 stop bit) take how long? */
        timer_set_ptr(TIME_IN_USEC(2000), p, pokey_serout_complete);
        break;

    case IRQEN_C:
		LOG(("POKEY #%d IRQEN  $%02x\n", chip, data));

        /* acknowledge one or more IRQST bits ? */
		if( p->IRQST & ~data )
        {
            /* reset IRQST bits that are masked now */
			p->IRQST &= data;
        }
        else
        {
			/* enable/disable timers now to avoid unneeded
               breaking of the CPU cores for masked timers */
			if( p->timer[TIMER1] && ((p->IRQEN^data) & IRQ_TIMR1) )
				timer_enable(p->timer[TIMER1], data & IRQ_TIMR1);
			if( p->timer[TIMER2] && ((p->IRQEN^data) & IRQ_TIMR2) )
				timer_enable(p->timer[TIMER2], data & IRQ_TIMR2);
			if( p->timer[TIMER4] && ((p->IRQEN^data) & IRQ_TIMR4) )
				timer_enable(p->timer[TIMER4], data & IRQ_TIMR4);
        }
		/* store irq enable */
		p->IRQEN = data;
        break;

    case SKCTL_C:
		if( data == p->SKCTL )
            return;
		LOG(("POKEY #%d SKCTL  $%02x\n", chip, data));
		p->SKCTL = data;
        if( !(data & SK_RESET) )
        {
            pokey_register_w(chip, IRQEN_C,  0);
            pokey_register_w(chip, SKREST_C, 0);
        }
        break;
    }

	/************************************************************
     * As defined in the manual, the exact counter values are
     * different depending on the frequency and resolution:
     *    64 kHz or 15 kHz - AUDF + 1
     *    1.79 MHz, 8-bit  - AUDF + 4
     *    1.79 MHz, 16-bit - AUDF[CHAN1]+256*AUDF[CHAN2] + 7
     ************************************************************/

    /* only reset the channels that have changed */

    if( ch_mask & (1 << CHAN1) )
    {
        /* process channel 1 frequency */
		if( p->AUDCTL & CH1_HICLK )
			new_val = p->AUDF[CHAN1] + DIVADD_HICLK;
        else
			new_val = (p->AUDF[CHAN1] + DIVADD_LOCLK) * p->clockmult;

		LOG_SOUND(("POKEY #%d chan1 %d\n", chip, new_val));

		p->volume[CHAN1] = (p->AUDC[CHAN1] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
        p->divisor[CHAN1] = new_val;
		if( new_val < p->counter[CHAN1] )
			p->counter[CHAN1] = new_val;
		if( p->interrupt_cb && p->timer[TIMER1] )
			timer_adjust_ptr(p->timer[TIMER1], 1.0 * new_val / p->clock, p->timer_period[TIMER1]);
		p->audible[CHAN1] = !(
			(p->AUDC[CHAN1] & VOLUME_ONLY) ||
			(p->AUDC[CHAN1] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN1] & PURE) && new_val < (p->samplerate_24_8 >> 8)));
		if( !p->audible[CHAN1] )
		{
			p->output[CHAN1] = 1;
			p->counter[CHAN1] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
            p->volume[CHAN1] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN2) )
    {
        /* process channel 2 frequency */
		if( p->AUDCTL & CH12_JOINED )
        {
			if( p->AUDCTL & CH1_HICLK )
				new_val = p->AUDF[CHAN2] * 256 + p->AUDF[CHAN1] + DIVADD_HICLK_JOINED;
            else
				new_val = (p->AUDF[CHAN2] * 256 + p->AUDF[CHAN1] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan1+2 %d\n", chip, new_val));
        }
        else
		{
			new_val = (p->AUDF[CHAN2] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan2 %d\n", chip, new_val));
		}

		p->volume[CHAN2] = (p->AUDC[CHAN2] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN2] = new_val;
		if( new_val < p->counter[CHAN2] )
			p->counter[CHAN2] = new_val;
		if( p->interrupt_cb && p->timer[TIMER2] )
			timer_adjust_ptr(p->timer[TIMER2], 1.0 * new_val / p->clock, p->timer_period[TIMER2]);
		p->audible[CHAN2] = !(
			(p->AUDC[CHAN2] & VOLUME_ONLY) ||
			(p->AUDC[CHAN2] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN2] & PURE) && new_val < (p->samplerate_24_8 >> 8)));
		if( !p->audible[CHAN2] )
		{
			p->output[CHAN2] = 1;
			p->counter[CHAN2] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN2] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN3) )
    {
        /* process channel 3 frequency */
		if( p->AUDCTL & CH3_HICLK )
			new_val = p->AUDF[CHAN3] + DIVADD_HICLK;
        else
			new_val = (p->AUDF[CHAN3] + DIVADD_LOCLK) * p->clockmult;

		LOG_SOUND(("POKEY #%d chan3 %d\n", chip, new_val));

		p->volume[CHAN3] = (p->AUDC[CHAN3] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN3] = new_val;
		if( new_val < p->counter[CHAN3] )
			p->counter[CHAN3] = new_val;
		/* channel 3 does not have a timer associated */
		p->audible[CHAN3] = !(
			(p->AUDC[CHAN3] & VOLUME_ONLY) ||
			(p->AUDC[CHAN3] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN3] & PURE) && new_val < (p->samplerate_24_8 >> 8))) ||
			(p->AUDCTL & CH1_FILTER);
		if( !p->audible[CHAN3] )
		{
			p->output[CHAN3] = 1;
			p->counter[CHAN3] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN3] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN4) )
    {
        /* process channel 4 frequency */
		if( p->AUDCTL & CH34_JOINED )
        {
			if( p->AUDCTL & CH3_HICLK )
				new_val = p->AUDF[CHAN4] * 256 + p->AUDF[CHAN3] + DIVADD_HICLK_JOINED;
            else
				new_val = (p->AUDF[CHAN4] * 256 + p->AUDF[CHAN3] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan3+4 %d\n", chip, new_val));
        }
        else
		{
			new_val = (p->AUDF[CHAN4] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan4 %d\n", chip, new_val));
		}

		p->volume[CHAN4] = (p->AUDC[CHAN4] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN4] = new_val;
		if( new_val < p->counter[CHAN4] )
			p->counter[CHAN4] = new_val;
		if( p->interrupt_cb && p->timer[TIMER4] )
			timer_adjust_ptr(p->timer[TIMER4], 1.0 * new_val / p->clock, p->timer_period[TIMER4]);
		p->audible[CHAN4] = !(
			(p->AUDC[CHAN4] & VOLUME_ONLY) ||
			(p->AUDC[CHAN4] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN4] & PURE) && new_val < (p->samplerate_24_8 >> 8))) ||
			(p->AUDCTL & CH2_FILTER);
		if( !p->audible[CHAN4] )
		{
			p->output[CHAN4] = 1;
			p->counter[CHAN4] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN4] >>= 1;
        }
    }
}

WRITE8_HANDLER( pokey1_w )
{
	pokey_register_w(0,offset,data);
}

WRITE8_HANDLER( pokey2_w )
{
    pokey_register_w(1,offset,data);
}

WRITE8_HANDLER( pokey3_w )
{
    pokey_register_w(2,offset,data);
}

WRITE8_HANDLER( pokey4_w )
{
    pokey_register_w(3,offset,data);
}

WRITE8_HANDLER( quad_pokey_w )
{
    int pokey_num = (offset >> 3) & ~0x04;
    int control = (offset & 0x20) >> 2;
    int pokey_reg = (offset % 8) | control;

    pokey_register_w(pokey_num, pokey_reg, data);
}

void pokey1_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 0);
	timer_set_ptr(1.0 * after / p->clock, p, pokey_serin_ready);
}

void pokey2_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 1);
	timer_set_ptr(1.0 * after / p->clock, p, pokey_serin_ready);
}

void pokey3_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 2);
	timer_set_ptr(1.0 * after / p->clock, p, pokey_serin_ready);
}

void pokey4_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 3);
	timer_set_ptr(1.0 * after / p->clock, p, pokey_serin_ready);
}

void pokey_break_w(int chip, int shift)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
    if( shift )                     /* shift code ? */
		p->SKSTAT |= SK_SHIFT;
	else
		p->SKSTAT &= ~SK_SHIFT;
	/* check if the break IRQ is enabled */
	if( p->IRQEN & IRQ_BREAK )
	{
		/* set break IRQ status and call back the interrupt handler */
		p->IRQST |= IRQ_BREAK;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(IRQ_BREAK);
    }
}

void pokey1_break_w(int shift)
{
	pokey_break_w(0, shift);
}

void pokey2_break_w(int shift)
{
	pokey_break_w(1, shift);
}

void pokey3_break_w(int shift)
{
	pokey_break_w(2, shift);
}

void pokey4_break_w(int shift)
{
	pokey_break_w(3, shift);
}

void pokey_kbcode_w(int chip, int kbcode, int make)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
    /* make code ? */
	if( make )
	{
		p->KBCODE = kbcode;
		p->SKSTAT |= SK_KEYBD;
		if( kbcode & 0x40 ) 		/* shift code ? */
			p->SKSTAT |= SK_SHIFT;
		else
			p->SKSTAT &= ~SK_SHIFT;

		if( p->IRQEN & IRQ_KEYBD )
		{
			/* last interrupt not acknowledged ? */
			if( p->IRQST & IRQ_KEYBD )
				p->SKSTAT |= SK_KBERR;
			p->IRQST |= IRQ_KEYBD;
			if( p->interrupt_cb )
				(*p->interrupt_cb)(IRQ_KEYBD);
		}
	}
	else
	{
		p->KBCODE = kbcode;
		p->SKSTAT &= ~SK_KEYBD;
    }
}

void pokey1_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(0, kbcode, make);
}

void pokey2_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(1, kbcode, make);
}

void pokey3_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(2, kbcode, make);
}

void pokey4_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(3, kbcode, make);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void pokey_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void pokey_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = pokey_set_info;		break;
		case SNDINFO_PTR_START:							info->start = pokey_start;				break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "POKEY";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Atari custom";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

