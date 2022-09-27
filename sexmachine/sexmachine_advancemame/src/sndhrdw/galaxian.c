#include "driver.h"
#include "streams.h"
#include "sound/samples.h"
#include "includes/galaxian.h"
#include <math.h>

#define VERBOSE 0

#define NEW_LFO 0
#define NEW_SHOOT 1

#define XTAL		18432000

#define SOUND_CLOCK (XTAL/6/2)			/* 1.536 MHz */

#define RNG_RATE	(XTAL/3)			/* RNG clock is XTAL/3 */
#define NOISE_RATE	(XTAL/3/192/2/2)	/* 2V = 8kHz */
#define NOISE_LENGTH (NOISE_RATE*4) 	/* four seconds of noise */

#define SHOOT_RATE 2672
#define SHOOT_LENGTH 13000

#define TOOTHSAW_LENGTH 16
#define TOOTHSAW_VOLUME 36
#define STEPS 16
#define LFO_VOLUME 0.06
#define SHOOT_VOLUME 0.50
#define NOISE_VOLUME 0.50
#define NOISE_AMPLITUDE (70*256)
#define TOOTHSAW_AMPLITUDE (64*256)

/* see comments in galaxian_sh_update() */
#define MINFREQ (139-139/3)
#define MAXFREQ (139+139/3)

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

static void *lfotimer = 0;
static INT32 freq = MAXFREQ;

#define STEP 1

static void *noisetimer = 0;
static INT32 noisevolume;
static INT16 *noisewave;
static INT16 *shootwave;
#if NEW_SHOOT
static INT32 shoot_length;
static INT32 shoot_rate;
#endif

static UINT8 last_port2=0;

static INT16 tonewave[4][TOOTHSAW_LENGTH];
static INT32 pitch,vol;

static INT32 counter, countdown;
static INT32 lfobit[4];

static INT16 backgroundwave[32] =
{
   0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
   0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
  -0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,
  -0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,-0x4000,
};

#define CHANNEL_NOISE	0
#define CHANNEL_SHOOT	1
#define CHANNEL_LFO		2
static sound_stream * tone_stream;

static void lfo_timer_cb(int param);
static void galaxian_sh_update(int dummy);

static void tone_update(void *param, stream_sample_t **input, stream_sample_t **output, int length)
{
	stream_sample_t *buffer = output[0];
	int i,j;
	INT16 *w = tonewave[vol];

	/* only update if we have non-zero volume and frequency */
	if( pitch != 0xff )
	{
		for (i = 0; i < length; i++)
		{
			int mix = 0;

			for (j = 0;j < STEPS;j++)
			{
				if (countdown >= 256)
				{
					counter = (counter + 1) % TOOTHSAW_LENGTH;
					countdown = pitch;
				}
				countdown++;

				mix += w[counter];
			}
			*buffer++ = mix / STEPS;
		}
	}
	else
	{
		for( i = 0; i < length; i++ )
			*buffer++ = 0;
	}
}

WRITE8_HANDLER( galaxian_pitch_w )
{
	stream_update(tone_stream,0);

	pitch = data;
}

WRITE8_HANDLER( galaxian_vol_w )
{
	stream_update(tone_stream,0);

	/* offset 0 = bit 0, offset 1 = bit 1 */
	vol = (vol & ~(1 << offset)) | ((data & 1) << offset);
}


static void noise_timer_cb(int param)
{
	if( noisevolume > 0 )
	{
		noisevolume -= (noisevolume / 10) + 1;
		sample_set_volume(CHANNEL_NOISE,noisevolume / 100.0);
	}
}

WRITE8_HANDLER( galaxian_noise_enable_w )
{
	if( data & 1 )
	{
		timer_adjust(noisetimer, TIME_NEVER, 0, 0);
		noisevolume = 100;
		sample_set_volume(CHANNEL_NOISE,noisevolume / 100.0);
	}
	else
	{
		/* discharge C21, 22uF via 150k+22k R35/R36 */
		if (noisevolume == 100)
			timer_adjust(noisetimer, TIME_IN_USEC(0.693*(155000+22000)*22 / 100), 0, TIME_IN_USEC(0.693*(155000+22000)*22 / 100));
	}
}

WRITE8_HANDLER( galaxian_shoot_enable_w )
{
	if( data & 1 && !(last_port2 & 1) )
	{
#if NEW_SHOOT
		sample_start_raw(CHANNEL_SHOOT, shootwave, shoot_length, shoot_rate, 0);
#else
		sample_start_raw(CHANNEL_SHOOT, shootwave, SHOOT_LENGTH, 10*SHOOT_RATE, 0);
#endif
		sample_set_volume(CHANNEL_SHOOT,SHOOT_VOLUME);
	}
	last_port2=data;
}


static void galaxian_sh_start(void)
{
	int i, j, sweep, charge, countdown, generator, bit1, bit2;

	sample_set_volume(CHANNEL_NOISE, NOISE_VOLUME);
	sample_set_volume(CHANNEL_SHOOT, SHOOT_VOLUME);
	sample_set_volume(CHANNEL_LFO+0, LFO_VOLUME);
	sample_set_volume(CHANNEL_LFO+1, LFO_VOLUME);
	sample_set_volume(CHANNEL_LFO+2, LFO_VOLUME);

	noisewave = auto_malloc(NOISE_LENGTH * sizeof(INT16));

#if NEW_SHOOT
#define SHOOT_SEC 2
	shoot_rate = Machine->sample_rate;
	shoot_length = SHOOT_SEC * shoot_rate;
	shootwave = auto_malloc(shoot_length * sizeof(INT16));
#else
	shootwave = auto_malloc(SHOOT_LENGTH * sizeof(INT16));
#endif

	/*
     * The RNG shifter is clocked with RNG_RATE, bit 17 is
     * latched every 2V cycles (every 2nd scanline).
     * This signal is used as a noise source.
     */
	generator = 0;
	countdown = NOISE_RATE / 2;
	for( i = 0; i < NOISE_LENGTH; i++ )
	{
		countdown -= RNG_RATE;
		while( countdown < 0 )
		{
			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;
			if (bit1 ^ bit2) generator |= 1;
			countdown += NOISE_RATE;
		}
		noisewave[i] = ((generator >> 17) & 1) ? NOISE_AMPLITUDE : -NOISE_AMPLITUDE;
	}

#if NEW_SHOOT

	/* dummy */
	sweep = 100;
	charge = +2;
	j=0;
	{
#define R41__ 100000
#define R44__ 10000
#define R45__ 22000
#define R46__ 10000
#define R47__ 2200
#define R48__ 2200
#define C25__ 0.000001
#define C27__ 0.00000001
#define C28__ 0.000047
#define C29__ 0.00000001
#define IC8L3_L 0.2   /* 7400 L level */
#define IC8L3_H 4.5   /* 7400 H level */
#define NOISE_L 0.2   /* 7474 L level */
#define NOISE_H 4.5   /* 7474 H level */
/*
    key on/off time is programmable
    Therefore,  it is necessary to make separate sample with key on/off.
    And,  calculate the playback point according to the voltage of c28.
*/
#define SHOOT_KEYON_TIME 0.1  /* second */
/*
    NE555-FM input calculation is wrong.
    The frequency is not proportional to the voltage of FM input.
    And,  duty will be changed,too.
*/
#define NE555_FM_ADJUST_RATE 0.80
		/* discharge : 100K * 1uF */
		double v  = 5.0;
		double vK = (shoot_rate) ? exp(-1 / (R41__*C25__) / shoot_rate) : 0;
		/* -- SHOOT KEY port -- */
		double IC8L3 = IC8L3_L; /* key on */
		int IC8Lcnt = SHOOT_KEYON_TIME * shoot_rate; /* count for key off */
		/* C28 : KEY port capacity */
		/*       connection : 8L-3 - R47(2.2K) - C28(47u) - R48(2.2K) - C29 */
		double c28v = IC8L3_H - (IC8L3_H-(NOISE_H+NOISE_L)/2)/(R46__+R47__+R48__)*R47__;
		double c28K = (shoot_rate) ? exp(-1 / (22000 * 0.000047 ) / shoot_rate) : 0;
		/* C29 : NOISE capacity */
		/*       connection : NOISE - R46(10K) - C29(0.1u) - R48(2.2K) - C28 */
		double c29v  = IC8L3_H - (IC8L3_H-(NOISE_H+NOISE_L)/2)/(R46__+R47__+R48__)*(R47__+R48__);
		double c29K1 = (shoot_rate) ? exp(-1 / (22000  * 0.00000001 ) / shoot_rate) : 0; /* form C28   */
		double c29K2 = (shoot_rate) ? exp(-1 / (100000 * 0.00000001 ) / shoot_rate) : 0; /* from noise */
		/* NE555 timer */
		/* RA = 10K , RB = 22K , C=.01u ,FM = C29 */
		double ne555cnt = 0;
		double ne555step = (shoot_rate) ? ((1.44/((R44__+R45__*2)*C27__)) / shoot_rate) : 0;
		double ne555duty = (double)(R44__+R45__)/(R44__+R45__*2); /* t1 duty */
		double ne555sr;		/* threshold (FM) rate */
		/* NOISE source */
		double ncnt  = 0.0;
		double nstep = (shoot_rate) ? ((double)NOISE_RATE / shoot_rate) : 0;
		double noise_sh2; /* voltage level */

		for( i = 0; i < shoot_length; i++ )
		{
			/* noise port */
			noise_sh2 = noisewave[(int)ncnt % NOISE_LENGTH] == NOISE_AMPLITUDE ? NOISE_H : NOISE_L;
			ncnt+=nstep;
			/* calculate NE555 threshold level by FM input */
			ne555sr = c29v*NE555_FM_ADJUST_RATE / (5.0*2/3);
			/* calc output */
			ne555cnt += ne555step;
			if( ne555cnt >= ne555sr) ne555cnt -= ne555sr;
			if( ne555cnt < ne555sr*ne555duty )
			{
				 /* t1 time */
				shootwave[i] = v/5*0x7fff;
				/* discharge output level */
				if(IC8L3==IC8L3_H)
					v *= vK;
			}
			else
				shootwave[i] = 0;
			/* C28 charge/discharge */
			c28v += (IC8L3-c28v) - (IC8L3-c28v)*c28K;	/* from R47 */
			c28v += (c29v-c28v) - (c29v-c28v)*c28K;		/* from R48 */
			/* C29 charge/discharge */
			c29v += (c28v-c29v) - (c28v-c29v)*c29K1;	/* from R48 */
			c29v += (noise_sh2-c29v) - (noise_sh2-c29v)*c29K2;	/* from R46 */
			/* key off */
			if(IC8L3==IC8L3_L && --IC8Lcnt==0)
				IC8L3=IC8L3_H;
		}
	}
#else
	/*
     * Ra is 10k, Rb is 22k, C is 0.01uF
     * charge time t1 = 0.693 * (Ra + Rb) * C -> 221.76us
     * discharge time t2 = 0.693 * (Rb) *  C -> 152.46us
     * average period 374.22us -> 2672Hz
     * I use an array of 10 values to define some points
     * of the charge/discharge curve. The wave is modulated
     * using the charge/discharge timing of C28, a 47uF capacitor,
     * over a 2k2 resistor. This will change the frequency from
     * approx. Favg-Favg/3 up to Favg+Favg/3 down to Favg-Favg/3 again.
     */
	sweep = 100;
	charge = +2;
	countdown = sweep / 2;
	for( i = 0, j = 0; i < SHOOT_LENGTH; i++ )
	{
		#define AMP(n)	(n)*0x8000/100-0x8000
		static const int charge_discharge[10] = {
			AMP( 0), AMP(25), AMP(45), AMP(60), AMP(70), AMP(85),
			AMP(70), AMP(50), AMP(25), AMP( 0)
		};
		shootwave[i] = charge_discharge[j];
		LOG(("shoot[%5d] $%04x (sweep: %3d, j:%d)\n", i, shootwave[i] & 0xffff, sweep, j));
		/*
         * The current sweep and a 2200/10000 fraction (R45 and R48)
         * of the noise are frequency modulating the NE555 chip.
         */
		countdown -= sweep + noisewave[i % NOISE_LENGTH] / (2200*NOISE_AMPLITUDE/10000);
		while( countdown < 0 )
		{
			countdown += 100;
			j = ++j % 10;
		}
		/* sweep from 100 to 133 and down to 66 over the time of SHOOT_LENGTH */
		if( i % (SHOOT_LENGTH / 33 / 3 ) == 0 )
		{
			sweep += charge;
			if( sweep >= 133 )
				charge = -1;
		}
	}
#endif

	memset(tonewave, 0, sizeof(tonewave));

	for( i = 0; i < TOOTHSAW_LENGTH; i++ )
	{
		#define V(r0,r1) 2*TOOTHSAW_AMPLITUDE*(r0)/(r0+r1)-TOOTHSAW_AMPLITUDE
		double r0a = 1.0/1e12, r1a = 1.0/1e12;
		double r0b = 1.0/1e12, r1b = 1.0/1e12;

		/* #0: VOL1=0 and VOL2=0
         * only the 33k and the 22k resistors R51 and R50
         */
		if( i & 1 )
		{
			r1a += 1.0/33000;
			r1b += 1.0/33000;
		}
		else
		{
			r0a += 1.0/33000;
			r0b += 1.0/33000;
		}
		if( i & 4 )
		{
			r1a += 1.0/22000;
			r1b += 1.0/22000;
		}
		else
		{
			r0a += 1.0/22000;
			r0b += 1.0/22000;
		}
		tonewave[0][i] = V(1.0/r0a, 1.0/r1a);

		/* #1: VOL1=1 and VOL2=0
         * add the 10k resistor R49 for bit QC
         */
		if( i & 4 )
			r1a += 1.0/10000;
		else
			r0a += 1.0/10000;
		tonewave[1][i] = V(1.0/r0a, 1.0/r1a);

		/* #2: VOL1=0 and VOL2=1
         * add the 15k resistor R52 for bit QD
         */
		if( i & 8 )
			r1b += 1.0/15000;
		else
			r0b += 1.0/15000;
		tonewave[2][i] = V(1.0/r0b, 1.0/r1b);

		/* #3: VOL1=1 and VOL2=1
         * add the 10k resistor R49 for QC
         */
		if( i & 4 )
			r0b += 1.0/10000;
		else
			r1b += 1.0/10000;
		tonewave[3][i] = V(1.0/r0b, 1.0/r1b);
		LOG(("tone[%2d]: $%02x $%02x $%02x $%02x\n", i, tonewave[0][i], tonewave[1][i], tonewave[2][i], tonewave[3][i]));
	}

	pitch = 0xff;
	vol = 0;

	tone_stream = stream_create(0,1,SOUND_CLOCK/STEPS,NULL,tone_update);
	stream_set_output_gain(tone_stream, 0, TOOTHSAW_VOLUME / 100.0);

	sample_set_volume(CHANNEL_NOISE,0);
	sample_start_raw(CHANNEL_NOISE,noisewave,NOISE_LENGTH,NOISE_RATE,1);

	sample_set_volume(CHANNEL_SHOOT,0);
	sample_start_raw(CHANNEL_SHOOT,shootwave,SHOOT_LENGTH,SHOOT_RATE,1);

	sample_set_volume(CHANNEL_LFO+0,0);
	sample_start_raw(CHANNEL_LFO+0,backgroundwave,sizeof(backgroundwave)/2,1000,1);
	sample_set_volume(CHANNEL_LFO+1,0);
	sample_start_raw(CHANNEL_LFO+1,backgroundwave,sizeof(backgroundwave)/2,1000,1);
	sample_set_volume(CHANNEL_LFO+2,0);
	sample_start_raw(CHANNEL_LFO+2,backgroundwave,sizeof(backgroundwave)/2,1000,1);

	noisetimer = timer_alloc(noise_timer_cb);
	lfotimer = timer_alloc(lfo_timer_cb);

	timer_pulse(TIME_IN_HZ(Machine->drv->frames_per_second), 0, galaxian_sh_update);

	state_save_register_global(freq);
	state_save_register_global(noisevolume);
	state_save_register_global(last_port2);
	state_save_register_global(pitch);
	state_save_register_global(vol);
	state_save_register_global(counter);
	state_save_register_global(countdown);
	state_save_register_global_array(lfobit);
}



WRITE8_HANDLER( galaxian_background_enable_w )
{
	sample_set_volume(CHANNEL_LFO+offset,(data & 1) ? LFO_VOLUME : 0);
}

static void lfo_timer_cb(int param)
{
	if( freq > MINFREQ )
		freq--;
	else
		freq = MAXFREQ;
}

WRITE8_HANDLER( galaxian_lfo_freq_w )
{
#if NEW_LFO
	/* R18 1M,R17 470K,R16 220K,R15 100K */
	static const int rv[4] = { 1000000,470000,220000,100000};
	double r1,r2,Re,td;
	int i;

	if( (data & 1) == lfobit[offset] )
		return;

	/*
     * NE555 9R is setup as astable multivibrator
     * - this circuit looks LINEAR RAMP V-F converter
       I  = 1/Re * ( R1/(R1+R2)-Vbe)
       td = (2/3VCC*Re*(R1+R2)*C) / (R1*VCC-Vbe*(R1+R2))
      parts assign
       R1  : (R15* L1)|(R16* L2)|(R17* L3)|(R18* L1)
       R2  : (R15*~L1)|(R16*~L2)|(R17*~L3)|(R18*~L4)|R??(330K)
       Re  : R21(100K)
       Vbe : Q2(2SA1015)-Vbe
     * - R20(15K) and Q1 is unknown,maybe current booster.
    */

	lfobit[offset] = data & 1;

	/* R20 15K */
	r1 = 1e12;
	/* R19? 330k to gnd */
	r2 = 330000;
	//r1 = 15000;
	/* R21 100K */
	Re = 100000;
	/* register calculation */
	for(i=0;i<4;i++)
	{
		if(lfobit[i])
			r1 = (r1*rv[i])/(r1+rv[i]); /* Hi  */
		else
			r2 = (r2*rv[i])/(r2+rv[i]); /* Low */
	}

#define Vcc 5.0
#define Vbe 0.65		/* 2SA1015 */
#define Cap 0.000001	/* C15 1uF */
	td = (Vcc*2/3*Re*(r1+r2)*Cap) / (r1*Vcc - Vbe*(r1+r2) );
#undef Cap
#undef Vbe
#undef Vcc
	logerror("lfo timer bits:%d%d%d%d r1:%d, r2:%d, re: %d, td: %9.2fsec\n", lfobit[0], lfobit[1], lfobit[2], lfobit[3], (int)r1, (int)r2, (int)Re, td);
	timer_adjust(lfotimer, TIME_IN_SEC(td / (MAXFREQ-MINFREQ)), 0, TIME_IN_SEC(td / (MAXFREQ-MINFREQ)));
#else
	double r0, r1, rx = 100000.0;

	if( (data & 1) == lfobit[offset] )
		return;

	/*
     * NE555 9R is setup as astable multivibrator
     * - Ra is between 100k and ??? (open?)
     * - Rb is zero here (bridge between pins 6 and 7)
     * - C is 1uF
     * charge time t1 = 0.693 * (Ra + Rb) * C
     * discharge time t2 = 0.693 * (Rb) *  C
     * period T = t1 + t2 = 0.693 * (Ra + 2 * Rb) * C
     * -> min period: 0.693 * 100 kOhm * 1uF -> 69300 us = 14.4Hz
     * -> max period: no idea, since I don't know the max. value for Ra :(
     */

	lfobit[offset] = data & 1;

	/* R?? 330k to gnd */
	r0 = 1.0/330000;
	/* open is a very high value really ;-) */
	r1 = 1.0/1e12;

	/* R18 1M */
	if( lfobit[0] )
		r1 += 1.0/1000000;
	else
		r0 += 1.0/1000000;

	/* R17 470k */
	if( lfobit[1] )
		r1 += 1.0/470000;
	else
		r0 += 1.0/470000;

	/* R16 220k */
	if( lfobit[2] )
		r1 += 1.0/220000;
	else
		r0 += 1.0/220000;

	/* R15 100k */
	if( lfobit[3] )
		r1 += 1.0/100000;
	else
		r0 += 1.0/100000;

	r0 = 1.0/r0;
	r1 = 1.0/r1;

	/* I used an arbitrary value for max. Ra of 2M */
	rx = rx + 2000000.0 * r0 / (r0+r1);

	LOG(("lfotimer bits:%d%d%d%d r0:%d, r1:%d, rx: %d, time: %9.2fus\n", lfobit[3], lfobit[2], lfobit[1], lfobit[0], (int)r0, (int)r1, (int)rx, 0.639 * rx));
	timer_adjust(lfotimer, TIME_IN_USEC(0.639 * rx / (MAXFREQ-MINFREQ)), 0, TIME_IN_USEC(0.639 * rx / (MAXFREQ-MINFREQ)));
#endif
}

static void galaxian_sh_update(int dummy)
{
	/*
     * NE555 8R, 8S and 8T are used as pulse position modulators
     * FS1 Ra=100k, Rb=470k and C=0.01uF
     *  -> 0.693 * 1040k * 0.01uF -> 7207.2us = 139Hz
     * FS2 Ra=100k, Rb=330k and C=0.01uF
     *  -> 0.693 * 760k * 0.01uF -> 5266.8us = 190Hz
     * FS2 Ra=100k, Rb=220k and C=0.01uF
     *  -> 0.693 * 540k * 0.01uF -> 3742.2us = 267Hz
     */

	sample_set_freq(CHANNEL_LFO+0, sizeof(backgroundwave)*freq*(100+2*470)/(100+2*470) );
	sample_set_freq(CHANNEL_LFO+1, sizeof(backgroundwave)*freq*(100+2*330)/(100+2*470) );
	sample_set_freq(CHANNEL_LFO+2, sizeof(backgroundwave)*freq*(100+2*220)/(100+2*470) );
}


struct Samplesinterface galaxian_custom_interface =
{
	5,
	NULL,
	galaxian_sh_start
};
