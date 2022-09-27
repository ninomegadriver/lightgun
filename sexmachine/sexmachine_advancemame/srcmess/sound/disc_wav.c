/************************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@esplexo.co.uk)
 *
 *  (c) K.Wilkins 2000
 *
 ************************************************************************
 *
 * DSS_COUNTER        - External clock Binary Counter
 * DSS_LFSR_NOISE     - Linear Feedback Shift Register Noise
 * DSS_NOISE          - Noise Source - Random source
 * DSS_NOTE           - Note/tone generator
 * DSS_OP_AMP_OSC     - Op Amp oscillator circuits
 * DSS_SAWTOOTHWAVE   - Sawtooth waveform generator
 * DSS_SCHMITT_OSC    - Schmitt Feedback Oscillator
 * DSS_SINEWAVE       - Sinewave generator source code
 * DSS_SQUAREWAVE     - Squarewave generator source code
 * DSS_SQUAREWFIX     - Squarewave generator - fixed frequency
 * DSS_SQUAREWAVE2    - Squarewave generator - by tOn/tOff
 * DSS_TRIANGLEWAVE   - Triangle waveform generator
 *
 ************************************************************************/

struct dss_adsr_context
{
	double phase;
};

struct dss_counter_context
{
	int		clock_type;
	int		out_type;
	int		is_7492;
	int		last;		// Last clock state
	int		count;		// current count
	double	t_clock;	// fixed counter clock in seconds
	double	t_left;		// time unused during last sample in seconds
};

struct dss_lfsr_context
{
	unsigned int	lfsr_reg;
	int				last;		// Last clock state
	double			t_clock;	// fixed counter clock in seconds
	double			t_left;		// time unused during last sample in seconds
	double			sampleStep;
	double			shiftStep;
	double			t;
};

struct dss_noise_context
{
	double phase;
};

struct dss_note_context
{
	int		clock_type;
	int		out_type;
	int		last;		// Last clock state
	double	t_clock;	// fixed counter clock in seconds
	double	t_left;		// time unused during last sample in seconds
	int		max1;		// Max 1 Count stored as int for easy use.
	int		max2;		// Max 2 Count stored as int for easy use.
	int		count1;		// current count1
	int		count2;		// current count2
};

struct dss_op_amp_osc_context
{
	int		flip_flop;		// flip/flop output state
	int		flip_flopXOR;	// flip_flop ^ flip_flopXOR, 0 = discharge, 1 = charge
	int		type;
	int		is_squarewave;
	double	thresholdLow;	// falling threshold
	double	thresholdHigh;	// rising threshold
	double	iCharge[2];		// charge/discharge currents
	double	vCap;			// current capacitor voltage
	double	rTotal;			// all input resistors in parallel
	double	iFixed;			// fixed current athe the input
	double	temp1;			// Multi purpose
	double	temp2;			// Multi purpose
	double	temp3;			// Multi purpose
};

struct dss_sawtoothwave_context
{
	double	phase;
	int		type;
};

struct dss_schmitt_osc_context
{
	double	ratioIn;		// ratio of total charging voltage that comes from the input
	double	ratioFeedback;	// ratio of total charging voltage that comes from the feedback
	double	vCap;			// current capacitor voltage
	double	rc;				// r*c
	double	exponent;
	int		state;			// state of the output
};

struct dss_sinewave_context
{
	double phase;
};

struct dss_squarewave_context
{
	double phase;
	double trigger;
};

struct dss_squarewfix_context
{
	int		flip_flop;
	double	sampleStep;
	double	tLeft;
	double	tOff;
	double	tOn;
};

struct dss_trianglewave_context
{
	double phase;
};


/************************************************************************
 *
 * DSS_COUNTER - External clock Binary Counter
 *
 * input0    - Enable input value
 * input1    - Reset input (active high)
 * input2    - Clock Input
 * input3    - Max count
 * input4    - Direction - 0=down, 1=up
 * input5    - Reset Value
 * input6    - Clock type
 *
 * Jan 2004, D Renaud.
 ************************************************************************/
#define DSS_COUNTER__ENABLE		(*(node->input[0]))
#define DSS_COUNTER__RESET		(*(node->input[1]))
#define DSS_COUNTER__CLOCK		(*(node->input[2]))
#define DSS_COUNTER__MAX		(*(node->input[3]))
#define DSS_COUNTER__DIR		(*(node->input[4]))
#define DSS_COUNTER__INIT		(*(node->input[5]))
#define DSS_COUNTER__CLOCK_TYPE	(*(node->input[6]))

const int disc_7492_count[6] = {0x00, 0x01, 0x02, 0x04, 0x05, 0x06};

void dss_counter_step(struct node_description *node)
{
	struct dss_counter_context *context = node->context;
	double cycles;
	int clock = 0, last_count, inc = 0;
	int max = DSS_COUNTER__MAX;
	double xTime = 0;

	if (context->clock_type == DISC_CLK_IS_FREQ)
	{
		/* We need to keep clocking the internal clock even if disabled. */
		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
		if (inc) xTime = context->t_left / discrete_current_context->sample_time;
	}
	else
	{
		clock = (int)DSS_COUNTER__CLOCK;
		xTime = DSS_COUNTER__CLOCK - clock;
	}


	/* If reset enabled then set output to the reset value.  No xTime in reset. */
	if (DSS_COUNTER__RESET)
	{
		context->count = DSS_COUNTER__INIT;
		node->output = context->is_7492 ? 0 : context->count;
		return;
	}

	/*
     * Only count if module is enabled.
     * This has the effect of holding the output at it's current value.
     */
	if (DSS_COUNTER__ENABLE)
	{
		last_count = context->count;

		switch (context->clock_type)
		{
			case DISC_CLK_ON_F_EDGE:
			case DISC_CLK_ON_R_EDGE:
				/* See if the clock has toggled to the proper edge */
				clock = (clock != 0);
				if (context->last != clock)
				{
					context->last = clock;
					if (context->clock_type == clock)
					{
						/* Toggled */
						inc = 1;
					}
				}
				break;

		case DISC_CLK_BY_COUNT:
				/* Clock number of times specified. */
				inc = clock;
				break;
		}

		for (clock = 0; clock < inc; clock++)
		{
			context->count += DSS_COUNTER__DIR ? 1 : -1; // up/down
			if (context->count < 0) context->count = max;
			if (context->count > max) context->count = 0;
		}

		node->output = context->is_7492 ? disc_7492_count[context->count] : context->count;

		if (context->count != last_count)
		{
			/* the xTime is only output if the output changed. */
			switch (context->out_type)
			{
				case DISC_OUT_IS_ENERGY:
					if (xTime != 0)
						node->output = (context->count > last_count) ? (last_count + xTime) : (last_count - xTime);
					break;
				case DISC_OUT_HAS_XTIME:
					node->output += xTime;
					break;
			}
		}
	}
	else
		node->output = context->count;
}

void dss_counter_reset(struct node_description *node)
{
	struct dss_counter_context *context = node->context;

	context->clock_type = (int)DSS_COUNTER__CLOCK_TYPE;
	if (context->clock_type == DISC_COUNTER_IS_7492)
	{
		context->clock_type = DISC_CLK_ON_F_EDGE;
		context->is_7492 = 1;
	}
	else
		context->is_7492 = 0;
	if ((context->clock_type < DISC_CLK_ON_F_EDGE) || (context->clock_type > DISC_CLK_IS_FREQ))
		discrete_log("Invalid clock type passed in NODE_%d\n", node->node - NODE_START);
	context->last = 0;
	if (context->clock_type == DISC_CLK_IS_FREQ) context->t_clock = 1.0 / DSS_COUNTER__CLOCK;
	context->t_left = 0;
	context->count = DSS_COUNTER__INIT; /* count starts at reset value */
	node->output = DSS_COUNTER__INIT;
}


/************************************************************************
 *
 * DSS_LFSR_NOISE - Usage of node_description values for LFSR noise gen
 *
 * input0    - Enable input value
 * input1    - Register reset
 * input2    - Clock Input
 * input3    - Amplitude input value
 * input4    - Input feed bit
 * input5    - Bias
 *
 * also passed dss_lfsr_context structure
 *
 ************************************************************************/
#define DSS_LFSR_NOISE__ENABLE	(*(node->input[0]))
#define DSS_LFSR_NOISE__RESET	(*(node->input[1]))
#define DSS_LFSR_NOISE__CLOCK	(*(node->input[2]))
#define DSS_LFSR_NOISE__AMP		(*(node->input[3]))
#define DSS_LFSR_NOISE__FEED	(*(node->input[4]))
#define DSS_LFSR_NOISE__BIAS	(*(node->input[5]))

int	dss_lfsr_function(int myfunc,int in0,int in1,int bitmask)
{
	int retval;

	in0&=bitmask;
	in1&=bitmask;

	switch(myfunc)
	{
		case DISC_LFSR_XOR:
			retval=in0^in1;
			break;
		case DISC_LFSR_OR:
			retval=in0|in1;
			break;
		case DISC_LFSR_AND:
			retval=in0&in1;
			break;
		case DISC_LFSR_XNOR:
			retval=in0^in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_NOR:
			retval=in0|in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_NAND:
			retval=in0&in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_IN0:
			retval=in0;
			break;
		case DISC_LFSR_IN1:
			retval=in1;
			break;
		case DISC_LFSR_NOT_IN0:
			retval=in0^bitmask;
			break;
		case DISC_LFSR_NOT_IN1:
			retval=in1^bitmask;
			break;
		case DISC_LFSR_REPLACE:
			retval=in0&~in1;
			retval=retval|in1;
			break;
		case DISC_LFSR_XOR_INV_IN0:
			retval = in0^bitmask; /* invert in0 */
			retval = retval^in1;  /* xor in1 */
			break;
		case DISC_LFSR_XOR_INV_IN1:
			retval = in1^bitmask; /* invert in1 */
			retval = retval^in0;  /* xor in0 */
			break;
		default:
			discrete_log("dss_lfsr_function - Invalid function type passed");
			retval=0;
			break;
	}
	return retval;
}

/* reset prototype so that it can be used in init function */
void dss_lfsr_reset(struct node_description *node);

void dss_lfsr_step(struct node_description *node)
{
	const struct discrete_lfsr_desc *lfsr_desc = node->custom;
	struct dss_lfsr_context *context = node->context;
	double cycles;
	int clock, inc = 0;
	int fb0,fb1,fbresult;

	if (lfsr_desc->clock_type == DISC_CLK_IS_FREQ)
	{
		/* We need to keep clocking the internal clock even if disabled. */
		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
	}

	/* Reset everything if necessary */
	if((DSS_LFSR_NOISE__RESET ? 1 : 0) == ((lfsr_desc->flags & DISC_LFSR_FLAG_RESET_TYPE_H) ? 1 : 0))
	{
		dss_lfsr_reset(node);
		return;
	}

	switch (lfsr_desc->clock_type)
	{
		case DISC_CLK_ON_F_EDGE:
		case DISC_CLK_ON_R_EDGE:
			/* See if the clock has toggled to the proper edge */
			clock = (DSS_LFSR_NOISE__CLOCK != 0);
			if (context->last != clock)
			{
				context->last = clock;
				if (lfsr_desc->clock_type == clock)
				{
					/* Toggled */
					inc = 1;
				}
			}
			break;

		case DISC_CLK_BY_COUNT:
			/* Clock number of times specified. */
			inc = (int)DSS_LFSR_NOISE__CLOCK;
			break;
	}

	for (clock = 0; clock < inc; clock++)
	{
		/* Fetch the last feedback result */
		fbresult=((context->lfsr_reg)>>(lfsr_desc->bitlength))&0x01;

		/* Stage 2 feedback combine fbresultNew with infeed bit */
		fbresult=dss_lfsr_function(lfsr_desc->feedback_function1,fbresult,((DSS_LFSR_NOISE__FEED)?0x01:0x00),0x01);

		/* Stage 3 first we setup where the bit is going to be shifted into */
		fbresult=fbresult*lfsr_desc->feedback_function2_mask;
		/* Then we left shift the register, */
		context->lfsr_reg=(context->lfsr_reg)<<1;
		/* Now move the fbresult into the shift register and mask it to the bitlength */
		context->lfsr_reg=dss_lfsr_function(lfsr_desc->feedback_function2,fbresult, (context->lfsr_reg), ((1<<(lfsr_desc->bitlength))-1));

		/* Now get and store the new feedback result */
		/* Fetch the feedback bits */
		fb0=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel0))&0x01;
		fb1=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel1))&0x01;
		/* Now do the combo on them */
		fbresult=dss_lfsr_function(lfsr_desc->feedback_function0,fb0,fb1,0x01);
		context->lfsr_reg=dss_lfsr_function(DISC_LFSR_REPLACE,(context->lfsr_reg), fbresult<<(lfsr_desc->bitlength), ((2<<(lfsr_desc->bitlength))-1));

		/* Now select the output bit */
		node->output=((context->lfsr_reg)>>(lfsr_desc->output_bit))&0x01;

		/* Final inversion if required */
		if(lfsr_desc->flags & DISC_LFSR_FLAG_OUT_INVERT) node->output=(node->output)?0.0:1.0;

		/* Gain stage */
		node->output=(node->output)?(DSS_LFSR_NOISE__AMP)/2:-(DSS_LFSR_NOISE__AMP)/2;
		/* Bias input as required */
		node->output=node->output+DSS_LFSR_NOISE__BIAS;
	}

	if(!DSS_LFSR_NOISE__ENABLE)
	{
		node->output=0;
	}
}

void dss_lfsr_reset(struct node_description *node)
{
	const struct discrete_lfsr_desc *lfsr_desc = node->custom;
	struct dss_lfsr_context *context = node->context;
	int fb0,fb1,fbresult;

	if ((lfsr_desc->clock_type < DISC_CLK_ON_F_EDGE) || (lfsr_desc->clock_type > DISC_CLK_IS_FREQ))
		discrete_log("Invalid clock type passed in NODE_%d\n", node->node - NODE_START);
	context->last = (DSS_COUNTER__CLOCK != 0);
	if (lfsr_desc->clock_type == DISC_CLK_IS_FREQ) context->t_clock = 1.0 / DSS_LFSR_NOISE__CLOCK;
	context->t_left = 0;

	context->lfsr_reg=lfsr_desc->reset_value;

	/* Now get and store the new feedback result */
	/* Fetch the feedback bits */
	fb0=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel0))&0x01;
	fb1=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel1))&0x01;
	/* Now do the combo on them */
	fbresult=dss_lfsr_function(lfsr_desc->feedback_function0,fb0,fb1,0x01);
	context->lfsr_reg=dss_lfsr_function(DISC_LFSR_REPLACE,(context->lfsr_reg), fbresult<<(lfsr_desc->bitlength), ((2<<(lfsr_desc->bitlength))-1));

	/* Now select and setup the output bit */
	node->output=((context->lfsr_reg)>>(lfsr_desc->output_bit))&0x01;

	/* Final inversion if required */
	if(lfsr_desc->flags&DISC_LFSR_FLAG_OUT_INVERT) node->output=(node->output)?0.0:1.0;

	/* Gain stage */
	node->output=(node->output)?(DSS_LFSR_NOISE__AMP)/2:-(DSS_LFSR_NOISE__AMP)/2;
	/* Bias input as required */
	node->output=node->output+DSS_LFSR_NOISE__BIAS;
}


/************************************************************************
 *
 * DSS_NOISE - Usage of node_description values for white nose generator
 *
 * input0    - Enable input value
 * input1    - Noise sample frequency
 * input2    - Amplitude input value
 * input3    - DC Bias value
 *
 ************************************************************************/
#define DSS_NOISE__ENABLE	(*(node->input[0]))
#define DSS_NOISE__FREQ		(*(node->input[1]))
#define DSS_NOISE__AMP		(*(node->input[2]))
#define DSS_NOISE__BIAS		(*(node->input[3]))

void dss_noise_step(struct node_description *node)
{
	struct dss_noise_context *context = node->context;

	if(DSS_NOISE__ENABLE)
	{
		/* Only sample noise on rollover to next cycle */
		if(context->phase>(2.0*M_PI))
		{
			int newval=rand() & 0x7fff;
			node->output=DSS_NOISE__AMP*(1-(newval/16384.0));

			/* Add DC Bias component */
			node->output=node->output+DSS_NOISE__BIAS;
		}
	}
	else
	{
		node->output=0;
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*DSS_NOISE__FREQ)/discrete_current_context->sample_rate)),2.0*M_PI);
}


void dss_noise_reset(struct node_description *node)
{
	struct dss_noise_context *context = node->context;
	context->phase=0;
	dss_noise_step(node);
}


/************************************************************************
 *
 * DSS_NOTE - Note/tone generator
 *
 * input0    - Enable input value
 * input1    - Clock Input
 * input2    - data value
 * input3    - Max count 1
 * input4    - Max count 2
 * input5    - Clock type
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
 #define DSS_NOTE__ENABLE		(*(node->input[0]))
 #define DSS_NOTE__CLOCK		(*(node->input[1]))
 #define DSS_NOTE__DATA			(*(node->input[2]))
 #define DSS_NOTE__MAX1			(*(node->input[3]))
 #define DSS_NOTE__MAX2			(*(node->input[4]))
 #define DSS_NOTE__CLOCK_TYPE	(*(node->input[5]))

void dss_note_step(struct node_description *node)
{
	struct dss_note_context *context = node->context;

	double cycles;
	int clock = 0, last_count2, inc = 0;
	double xTime = 0;

	if (context->clock_type == DISC_CLK_IS_FREQ)
	{
		/* We need to keep clocking the internal clock even if disabled. */
		cycles = (context->t_left + discrete_current_context->sample_time) / context->t_clock;
		inc = (int)cycles;
		context->t_left = (cycles - inc) * context->t_clock;
		if (inc) xTime = context->t_left / discrete_current_context->sample_time;
	}
	else
	{
		/* Seperate clock info from xTime info. */
		clock = (int)DSS_NOTE__CLOCK;
		xTime = DSS_NOTE__CLOCK - clock;
	}

	if (DSS_NOTE__ENABLE)
	{
		last_count2 = context->count2;

		switch (context->clock_type)
		{
			case DISC_CLK_ON_F_EDGE:
			case DISC_CLK_ON_R_EDGE:
				/* See if the clock has toggled to the proper edge */
				clock = (clock != 0);
				if (context->last != clock)
				{
					context->last = clock;
					if (context->clock_type == clock)
					{
						/* Toggled */
						inc = 1;
					}
				}
				break;

			case DISC_CLK_BY_COUNT:
				/* Clock number of times specified. */
				inc = clock;
				break;
		}

		/* Count output as long as the data loaded is not already equal to max 1 count. */
		if (DSS_NOTE__DATA != DSS_NOTE__MAX1)
		{
			for (clock = 0; clock < inc; clock++)
			{
				context->count1++;
				if (context->count1 > context->max1)
				{
					/* Max 1 count reached.  Load Data into counter. */
					context->count1 = (int)DSS_NOTE__DATA;
					context->count2 += 1;
					if (context->count2 > context->max2) context->count2 = 0;
				}
			}
		}

		node->output = context->count2;
		if (context->count2 != last_count2)
		{
			/* the xTime is only output if the output changed. */
			switch (context->out_type)
			{
				case DISC_OUT_IS_ENERGY:
					if (xTime != 0)
						node->output = (context->count2 > last_count2) ? (last_count2 + xTime) : (last_count2 - xTime);
					break;
				case DISC_OUT_HAS_XTIME:
					node->output += xTime;
					break;
			}
		}
	}
	else
		node->output = 0;
}

void dss_note_reset(struct node_description *node)
{
	struct dss_note_context *context = node->context;

	context->clock_type = (int)DSS_NOTE__CLOCK_TYPE & DISC_CLK_MASK;
	context->out_type =  (int)DSS_NOTE__CLOCK_TYPE & DISC_OUT_MASK;
	context->last = (DSS_NOTE__CLOCK != 0);
	if (context->clock_type == DISC_CLK_IS_FREQ) context->t_clock = 1.0 / DSS_NOTE__CLOCK;
	context->t_left = 0;

	context->count1 = (int)DSS_NOTE__DATA;
	context->count2 = 0;
	context->max1 = (int)DSS_NOTE__MAX1;
	context->max2 = (int)DSS_NOTE__MAX2;
	node->output = 0;
}

/************************************************************************
 *
 * DSS_OP_AMP_OSC - Op Amp Oscillators
 *
 * input0    - Enable input value
 * input1    - vMod1 (if needed)
 * input2    - vMod2 (if needed)
 *
 * also passed discrete_op_amp_osc_info structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DSS_OP_AMP_OSC__ENABLE	(*(node->input[0]))
#define DSS_OP_AMP_OSC__VMOD1	(*(node->input[1]))
#define DSS_OP_AMP_OSC__VMOD2	(*(node->input[2]))

void dss_op_amp_osc_step(struct node_description *node)
{
	const struct discrete_op_amp_osc_info *info = node->custom;
	struct dss_op_amp_osc_context *context = node->context;


	double i;			// Charging current created by vIn
	double v = 0;		// all input voltages mixed
	double dt;			// change in time
	double vC;			// Current voltage on capacitor, before dt
	double vCnext = 0;	// Voltage on capacitor, after dt

	dt = discrete_current_context->sample_time;	// Change in time
	vC = context->vCap;	// Set to voltage before change

	/* work out the charge currents for the VCOs. */
	switch (context->type)
	{
		case DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON:
			/* Millman the input voltages. */
			if (info->r7 == 0)
			{
				/* No r7 means that the modulation circuit is fed directly into the circuit. */
				v = DSS_OP_AMP_OSC__VMOD1;
			}
			else
			{
				/* we need to mix any bias and all modulation voltages together. */
				i = context->iFixed;
				i += DSS_OP_AMP_OSC__VMOD1 / info->r7;
				if (info->r8 != 0)
					i += DSS_OP_AMP_OSC__VMOD2 / info->r8;
				v = i * context->rTotal;
			}

			/* Work out the charge rates. */
			context->iCharge[0] = (v - OP_AMP_NORTON_VBE) / info->r1;
			context->iCharge[1] = (v - OP_AMP_NORTON_VBE) / info->r2 - context->iCharge[0];
			break;

		case DISC_OP_AMP_OSCILLATOR_VCO_1:
			/* Work out the charge rates. */
			i = DSS_OP_AMP_OSC__VMOD1 * context->temp1;		// i is not a current.  It is being used as a temp variable.
			context->iCharge[0] = (DSS_OP_AMP_OSC__VMOD1 - i) / info->r1;
			context->iCharge[1] = (i - (DSS_OP_AMP_OSC__VMOD1 * context->temp2)) / context->temp3;
			break;
	}

	if (DSS_OP_AMP_OSC__ENABLE)
	{
		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->flip_flop ^ context->flip_flopXOR)
			{
				/* Charging */
				/* iC=C*dv/dt  works out to dv=iC*dt/C */
				vCnext = vC + (context->iCharge[1] * dt / info->c);
				dt = 0;

				/* has it charged past upper limit? */
				if (vCnext >= context->thresholdHigh)
				{
					if (vCnext > context->thresholdHigh)
					{
						/* calculate the overshoot time */
						dt = info->c * (vCnext - context->thresholdHigh) / context->iCharge[1];
					}
					vC = context->thresholdHigh;
					context->flip_flop = !context->flip_flop;
				}
			}
			else
			{
				/* Discharging */
				vCnext = vC - (context->iCharge[0] * dt / info->c);
				dt = 0;

				/* has it discharged past lower limit? */
				if (vCnext <= context->thresholdLow)
				{
					if (vCnext < context->thresholdLow)
					{
						/* calculate the overshoot time */
						dt = info->c * (context->thresholdLow - vCnext) / context->iCharge[0];
					}
					vC = context->thresholdLow;
					context->flip_flop = !context->flip_flop;
				}
			}
		} while(dt);

		context->vCap = vCnext;

		if (context->is_squarewave)
			node->output = (info->vP - (context->type == DISC_OP_AMP_OSCILLATOR_VCO_1) ? OP_AMP_VP_RAIL_OFFSET : OP_AMP_NORTON_VBE) * context->flip_flop;
		else
			node->output = context->vCap;
	}
	else
	{
		/* we are disabled */
		if (context->type & DISC_OP_AMP_OSCILLATOR_VCO_MASK)
		{
			context->flip_flop = (context->type == DISC_OP_AMP_OSCILLATOR_VCO_1) ? 1 : 0;
			/* if we get disabled, then the cap keeps discharging to 0. */
			if (context->is_squarewave)
				node->output = 0;
			else
			{
				context->vCap = vC - (context->iCharge[0] * dt / info->c);
				if (context->vCap < 0) context->vCap = 0;
				node->output = context->vCap;
			}
		}
		else
			/* we will just output 0 for the regular oscilators because they have no real disable. */
			node->output = 0;
	}
}

void dss_op_amp_osc_reset(struct node_description *node)
{
	const struct discrete_op_amp_osc_info *info = node->custom;
	struct dss_op_amp_osc_context *context = node->context;

	double i1 = 0;	// inverting input current
	double i2 = 0;	// non-inverting input current

	context->is_squarewave = (info->type & DISC_OP_AMP_OSCILLATOR_OUT_SQW) * (info->vP - OP_AMP_NORTON_VBE);
	context->type = info->type & DISC_OP_AMP_OSCILLATOR_TYPE_MASK;

	switch (context->type)
	{
		case DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON:
			/* Work out the charge rates. */
			context->iCharge[0] = (info->vP - OP_AMP_NORTON_VBE) / info->r1;
			/* Output of the schmitt is vP - OP_AMP_NORTON_VBE */
			context->iCharge[1] = (info->vP - OP_AMP_NORTON_VBE - OP_AMP_NORTON_VBE) / info->r2 - context->iCharge[0];
			/* Charges while FlipFlop High */
			context->flip_flopXOR = 0;
			/* Work out the Inverting Schmitt thresholds. */
			i1 = (info->vP - OP_AMP_NORTON_VBE) / info->r5;
			i2 = (info->vP - OP_AMP_NORTON_VBE - OP_AMP_NORTON_VBE) / info->r4;
			context->thresholdLow = i1 * info->r3 + OP_AMP_NORTON_VBE;
			context->thresholdHigh = (i1 + i2) * info->r3 + OP_AMP_NORTON_VBE;
			/* There is no charge on the cap so the schmitt inverter goes high at init. */
			context->flip_flop = 1;
			break;

		case DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON:
			/* The charge rates vary depending on vMod so they are not precalculated. */
			/* But we can precalculate the fixed currents. */
			context->iFixed = 0;
			if (info->r6 != 0) context->iFixed += info->vP / info->r6;
			context->iFixed += OP_AMP_NORTON_VBE / info->r1;
			context->iFixed += OP_AMP_NORTON_VBE / info->r2;
			/* Work out the input resistance to be used later to calculate the Millman voltage. */
			context->rTotal = 1.0 / info->r1 + 1.0 / info->r2 + 1.0 / info->r7;
			if (info->r6) context->rTotal += 1.0 / info->r6;
			if (info->r8) context->rTotal += 1.0 / info->r8;
			context->rTotal = 1.0 / context->rTotal;
			/* Charges while FlipFlop Low */
			context->flip_flopXOR = 1;
			/* Work out the Non-inverting Schmitt thresholds. */
			i1 = (info->vP - OP_AMP_NORTON_VBE) / info->r5;
			i2 = (info->vP - OP_AMP_NORTON_VBE - OP_AMP_NORTON_VBE) / info->r4;
			context->thresholdLow = (i1 - i2) * info->r3 + OP_AMP_NORTON_VBE;
			context->thresholdHigh = i1 * info->r3 + OP_AMP_NORTON_VBE;
			/* There is no charge on the cap so the schmitt goes low at init. */
			context->flip_flop = 0;
			break;

		case DISC_OP_AMP_OSCILLATOR_VCO_1:
			/* The charge rates vary depending on vMod so they are not precalculated. */
			/* Charges while FlipFlop High */
			context->flip_flopXOR = 0;
			/* Work out the Non-inverting Schmitt thresholds. */
			context->temp1 = (info->vP / 2) / info->r4;
			context->temp2 = (info->vP - OP_AMP_VP_RAIL_OFFSET) / info->r3;
			context->temp3 = 1.0 / (1.0 / info->r3 + 1.0 / info->r4);
			context->thresholdLow = context->temp1 * context->temp3;
			context->thresholdHigh = (context->temp1 + context->temp2) * context->temp3;
			/* There is no charge on the cap so the schmitt goes high at init. */
			context->flip_flop = 1;
			/* Setup some commonly used stuff */
			context->temp1 = info->r5 / (info->r2 + info->r5);			// voltage ratio across r5
			context->temp2 = info->r6 / (info->r1 + info->r6);			// voltage ratio across r6
			context->temp3 = 1.0 / (1.0 / info->r1 + 1.0 / info->r6);	// input resistance when r6 switched in
			break;
	}

	context->vCap = 0;

	dss_op_amp_osc_step(node);
}


/************************************************************************
 *
 * DSS_SAWTOOTHWAVE - Usage of node_description values for step function
 *
 * input0    - Enable input value
 * input1    - Frequency input value
 * input2    - Amplitde input value
 * input3    - DC Bias Value
 * input4    - Gradient
 * input5    - Initial Phase
 *
 ************************************************************************/
#define DSS_SAWTOOTHWAVE__ENABLE	(*(node->input[0]))
#define DSS_SAWTOOTHWAVE__FREQ		(*(node->input[1]))
#define DSS_SAWTOOTHWAVE__AMP		(*(node->input[2]))
#define DSS_SAWTOOTHWAVE__BIAS		(*(node->input[3]))
#define DSS_SAWTOOTHWAVE__GRAD		(*(node->input[4]))
#define DSS_SAWTOOTHWAVE__PHASE		(*(node->input[5]))

void dss_sawtoothwave_step(struct node_description *node)
{
	struct dss_sawtoothwave_context *context = node->context;

	if(DSS_SAWTOOTHWAVE__ENABLE)
	{
		node->output=(context->type==0)?context->phase*(DSS_SAWTOOTHWAVE__AMP/(2.0*M_PI)):DSS_SAWTOOTHWAVE__AMP-(context->phase*(DSS_SAWTOOTHWAVE__AMP/(2.0*M_PI)));
		node->output-=DSS_SAWTOOTHWAVE__AMP/2.0;
		/* Add DC Bias component */
		node->output=node->output+DSS_SAWTOOTHWAVE__BIAS;
	}
	else
	{
		node->output=0;
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*DSS_SAWTOOTHWAVE__FREQ)/discrete_current_context->sample_rate)),2.0*M_PI);
}

void dss_sawtoothwave_reset(struct node_description *node)
{
	struct dss_sawtoothwave_context *context = node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(DSS_SAWTOOTHWAVE__PHASE/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Invert gradient depending on sawtooth type /|/|/|/|/| or |\|\|\|\|\ */
	context->type=(DSS_SAWTOOTHWAVE__GRAD)?1:0;

	/* Step the node to set the output */
	dss_sawtoothwave_step(node);
}


/************************************************************************
 *
 * DSS_SCHMITT_OSC - Schmitt feedback oscillator
 *
 * input0    - Enable input value
 * input1    - Vin
 * input2    - Amplitude
 *
 * also passed discrete_schmitt_osc_disc structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DSS_SCHMITT_OSC__ENABLE	(int)(*(node->input[0]))
#define DSS_SCHMITT_OSC__VIN	(*(node->input[1]))
#define DSS_SCHMITT_OSC__AMP	(*(node->input[2]))

void dss_schmitt_osc_step(struct node_description *node)
{
	const struct discrete_schmitt_osc_desc *info = node->custom;
	struct dss_schmitt_osc_context *context = node->context;

	double supply, vCap, new_vCap, t, exponent;

	/* We will always oscillate.  The enable just affects the output. */
	vCap = context->vCap;
	exponent = context->exponent;

	/* Keep looping until all toggling in time sample is used up. */
	do
	{
		t = 0;
		/* The charging voltage to the cap is the sum of the input voltage and the gate
         * output voltage in the ratios determined by their resistors in a divider network.
         * The input voltage is selectable as straight voltage in or logic level that will
         * use vGate as its voltage.  Note that ratioIn is just the ratio of the total
         * voltage and needs to be multipled by the input voltage.  ratioFeedback has
         * already been multiplied by vGate to save time because that voltage never changes. */
		supply = (info->options & DISC_SCHMITT_OSC_IN_IS_VOLTAGE) ? context->ratioIn * DSS_SCHMITT_OSC__VIN : (DSS_SCHMITT_OSC__VIN ? context->ratioIn * info->vGate : 0);
		supply += (context->state ? context->ratioFeedback : 0);
		new_vCap = vCap + ((supply - vCap) * exponent);
		if (context->state)
		{
			/* Charging */
			/* has it charged past upper limit? */
			if (new_vCap >= info->trshRise)
			{
				if (new_vCap > info->trshRise)
				{
					/* calculate the overshoot time */
					t = context->rc * log(1.0 / (1.0 - ((new_vCap - info->trshRise) / (info->vGate - vCap))));
					/* calculate new exponent because of reduced time */
					exponent = 1.0 - exp(-t / context->rc);
				}
				vCap = info->trshRise;
				new_vCap = info->trshRise;
				context->state = 0;
			}
		}
		else
		{
			/* Discharging */
			/* has it discharged past lower limit? */
			if (new_vCap <= info->trshFall)
			{
				if (new_vCap < info->trshFall)
				{
					/* calculate the overshoot time */
					t = context->rc * log(1.0 / (1.0 - ((info->trshFall - new_vCap) / vCap)));
					/* calculate new exponent because of reduced time */
					exponent = 1.0 - exp(-t / context->rc);
				}
				vCap = info->trshFall;
				new_vCap = info->trshFall;
				context->state = 1;
			}
		}
	} while(t);

	context->vCap = new_vCap;

	switch (info->options & DISC_SCHMITT_OSC_ENAB_MASK)
	{
		case DISC_SCHMITT_OSC_ENAB_IS_AND:
			node->output = DSS_SCHMITT_OSC__ENABLE && context->state;
			break;
		case DISC_SCHMITT_OSC_ENAB_IS_NAND:
			node->output = !(DSS_SCHMITT_OSC__ENABLE && context->state);
			break;
		case DISC_SCHMITT_OSC_ENAB_IS_OR:
			node->output = DSS_SCHMITT_OSC__ENABLE || context->state;
			break;
		case DISC_SCHMITT_OSC_ENAB_IS_NOR:
			node->output = !(DSS_SCHMITT_OSC__ENABLE || context->state);
			break;
	}
	node->output = node->output * DSS_SCHMITT_OSC__AMP;
}

void dss_schmitt_osc_reset(struct node_description *node)
{
	const struct discrete_schmitt_osc_desc *info = node->custom;
	struct dss_schmitt_osc_context *context = node->context;
	double rSource;

	/* The 2 resistors make a voltage divider, so their ratios add together
     * to make the charging voltage. */
	context->ratioIn = info->rFeedback / (info->rIn + info->rFeedback);
	context->ratioFeedback = info->rIn / (info->rIn + info->rFeedback) * info->vGate;

	/* The voltage source resistance works out to the 2 resistors in parallel.
     * So use this for the RC charge constant. */
	rSource = 1.0 / ((1.0 / info->rIn) + (1.0 / info->rFeedback));
	context->rc = rSource * info->c;
	context->exponent = -1.0 / (context->rc  * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);

	/* Cap is at 0V on power up.  Causing output to be high. */
	context->vCap = 0;
	context->state = 1;

	node->output = info->options ? 0 : DSS_SCHMITT_OSC__AMP;
}


/************************************************************************
 *
 * DSS_SINEWAVE - Usage of node_description values for step function
 *
 * input0    - Enable input value
 * input1    - Frequency input value
 * input2    - Amplitude input value
 * input3    - DC Bias
 * input4    - Starting phase
 *
 ************************************************************************/
#define DSS_SINEWAVE__ENABLE	(*(node->input[0]))
#define DSS_SINEWAVE__FREQ		(*(node->input[1]))
#define DSS_SINEWAVE__AMPL		(*(node->input[2]))
#define DSS_SINEWAVE__BIAS		(*(node->input[3]))
#define DSS_SINEWAVE__PHASE		(*(node->input[4]))

void dss_sinewave_step(struct node_description *node)
{
	struct dss_sinewave_context *context = node->context;

	/* Set the output */
	if(DSS_SINEWAVE__ENABLE)
	{
		node->output=(DSS_SINEWAVE__AMPL/2.0) * sin(context->phase);
		/* Add DC Bias component */
		node->output=node->output+DSS_SINEWAVE__BIAS;
	}
	else
	{
		node->output=0;
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*DSS_SINEWAVE__FREQ)/discrete_current_context->sample_rate)),2.0*M_PI);
}

void dss_sinewave_reset(struct node_description *node)
{
	struct dss_sinewave_context *context = node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(DSS_SINEWAVE__PHASE/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);
	/* Step the output to make it correct */
	dss_sinewave_step(node);
}


/************************************************************************
 *
 * DSS_SQUAREWAVE - Usage of node_description values for step function
 *
 * input0    - Enable input value
 * input1    - Frequency input value
 * input2    - Amplitude input value
 * input3    - Duty Cycle
 * input4    - DC Bias level
 * input5    - Start Phase
 *
 ************************************************************************/
#define DSS_SQUAREWAVE__ENABLE	(*(node->input[0]))
#define DSS_SQUAREWAVE__FREQ	(*(node->input[1]))
#define DSS_SQUAREWAVE__AMP		(*(node->input[2]))
#define DSS_SQUAREWAVE__DUTY	(*(node->input[3]))
#define DSS_SQUAREWAVE__BIAS	(*(node->input[4]))
#define DSS_SQUAREWAVE__PHASE	(*(node->input[5]))

void dss_squarewave_step(struct node_description *node)
{
	struct dss_squarewave_context *context = node->context;

	/* Establish trigger phase from duty */
	context->trigger=((100-DSS_SQUAREWAVE__DUTY)/100)*(2.0*M_PI);

	/* Set the output */
	if(DSS_SQUAREWAVE__ENABLE)
	{
		if(context->phase>context->trigger)
			node->output=(DSS_SQUAREWAVE__AMP/2.0);
		else
			node->output=-(DSS_SQUAREWAVE__AMP/2.0);

		/* Add DC Bias component */
		node->output=node->output+DSS_SQUAREWAVE__BIAS;
	}
	else
	{
		node->output=0;
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*DSS_SQUAREWAVE__FREQ)/discrete_current_context->sample_rate)),2.0*M_PI);
}

void dss_squarewave_reset(struct node_description *node)
{
	struct dss_squarewave_context *context = node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(DSS_SQUAREWAVE__PHASE/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Step the output */
	dss_squarewave_step(node);
}

/************************************************************************
 *
 * DSS_SQUAREWFIX - Usage of node_description values for step function
 *
 * input0    - Enable input value
 * input1    - Frequency input value
 * input2    - Amplitude input value
 * input3    - Duty Cycle
 * input4    - DC Bias level
 * input5    - Start Phase
 *
 ************************************************************************/
#define DSS_SQUAREWFIX__ENABLE	(*(node->input[0]))
#define DSS_SQUAREWFIX__FREQ	(*(node->input[1]))
#define DSS_SQUAREWFIX__AMP		(*(node->input[2]))
#define DSS_SQUAREWFIX__DUTY	(*(node->input[3]))
#define DSS_SQUAREWFIX__BIAS	(*(node->input[4]))
#define DSS_SQUAREWFIX__PHASE	(*(node->input[5]))

void dss_squarewfix_step(struct node_description *node)
{
	struct dss_squarewfix_context *context = node->context;

	context->tLeft -= context->sampleStep;

	/* The enable input only curtails output, phase rotation still occurs */
	while (context->tLeft <= 0)
	{
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}

	if(DSS_SQUAREWFIX__ENABLE)
	{
		/* Add gain and DC Bias component */

		context->tOff = 1.0 / DSS_SQUAREWFIX__FREQ;	/* cycle time */
		context->tOn = context->tOff * (DSS_SQUAREWFIX__DUTY / 100.0);
		context->tOff -= context->tOn;

		node->output = (context->flip_flop ? DSS_SQUAREWFIX__AMP / 2.0 : -(DSS_SQUAREWFIX__AMP / 2.0)) + DSS_SQUAREWFIX__BIAS;
	}
	else
	{
		node->output=0;
	}
}

void dss_squarewfix_reset(struct node_description *node)
{
	struct dss_squarewfix_context *context = node->context;

	context->sampleStep = 1.0 / discrete_current_context->sample_rate;
	context->flip_flop = 1;

	/* Do the intial time shift and convert freq to off/on times */
	context->tOff = 1.0 / DSS_SQUAREWFIX__FREQ;	/* cycle time */
	context->tLeft = DSS_SQUAREWFIX__PHASE / 360.0;	/* convert start phase to % */
	context->tLeft = context->tLeft - (int)context->tLeft;	/* keep % between 0 & 1 */
	context->tLeft = (context->tLeft < 0) ? 1.0 + context->tLeft : context->tLeft;	/* if - then flip to + phase */
	context->tLeft *= context->tOff;
	context->tOn = context->tOff * (DSS_SQUAREWFIX__DUTY / 100.0);
	context->tOff -= context->tOn;

	context->tLeft = -context->tLeft;

	/* toggle output and work out intial time shift */
	while (context->tLeft <= 0)
	{
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}

	/* Step the output */
	dss_squarewfix_step(node);
}


/************************************************************************
 *
 * DSS_SQUAREWAVE2 - Usage of node_description values
 *
 * input0    - Enable input value
 * input1    - Amplitude input value
 * input2    - OFF Time
 * input3    - ON Time
 * input4    - DC Bias level
 * input5    - Initial Time Shift
 *
 ************************************************************************/
#define DSS_SQUAREWAVE2__ENABLE	(*(node->input[0]))
#define DSS_SQUAREWAVE2__AMP	(*(node->input[1]))
#define DSS_SQUAREWAVE2__T_OFF	(*(node->input[2]))
#define DSS_SQUAREWAVE2__T_ON	(*(node->input[3]))
#define DSS_SQUAREWAVE2__BIAS	(*(node->input[4]))
#define DSS_SQUAREWAVE2__SHIFT	(*(node->input[5]))

void dss_squarewave2_step(struct node_description *node)
{
	struct dss_squarewave_context *context = node->context;
	double newphase;

	if(DSS_SQUAREWAVE2__ENABLE)
	{
		/* Establish trigger phase from time periods */
		context->trigger=(DSS_SQUAREWAVE2__T_OFF / (DSS_SQUAREWAVE2__T_OFF + DSS_SQUAREWAVE2__T_ON)) * (2.0 * M_PI);

		/* Work out the phase step based on phase/freq & sample rate */
		/* The enable input only curtails output, phase rotation     */
		/* still occurs                                              */

		/*     phase step = 2Pi/(output period/sample period)        */
		/*                    boils out to                           */
		/*     phase step = 2Pi/(output period*sample freq)          */
		newphase = context->phase + ((2.0 * M_PI) / ((DSS_SQUAREWAVE2__T_OFF + DSS_SQUAREWAVE2__T_ON) * discrete_current_context->sample_rate));
		/* Keep the new phasor in the 2Pi range.*/
		context->phase = fmod(newphase, 2.0 * M_PI);

		if(context->phase>context->trigger)
			node->output=(DSS_SQUAREWAVE2__AMP/2.0);
		else
			node->output=-(DSS_SQUAREWAVE2__AMP/2.0);

		/* Add DC Bias component */
		node->output = node->output + DSS_SQUAREWAVE2__BIAS;
	}
	else
	{
		node->output=0;
	}
}

void dss_squarewave2_reset(struct node_description *node)
{
	struct dss_squarewave_context *context = node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	/* Only valid if we have set the on/off time                 */
	if((DSS_SQUAREWAVE2__T_OFF + DSS_SQUAREWAVE2__T_ON) != 0.0)
		start = (DSS_SQUAREWAVE2__SHIFT / (DSS_SQUAREWAVE2__T_OFF + DSS_SQUAREWAVE2__T_ON)) * (2.0 * M_PI);
	else
		start = 0.0;
	/* Make sure its always mod 2Pi */
	context->phase = fmod(start, 2.0 * M_PI);

	/* Step the output */
	dss_squarewave2_step(node);
}


/************************************************************************
 *
 * DSS_TRIANGLEWAVE - Usage of node_description values for step function
 *
 * input0    - Enable input value
 * input1    - Frequency input value
 * input2    - Amplitde input value
 * input3    - DC Bias value
 * input4    - Initial Phase
 *
 ************************************************************************/
#define DSS_TRIANGLEWAVE__ENABLE	(*(node->input[0]))
#define DSS_TRIANGLEWAVE__FREQ		(*(node->input[1]))
#define DSS_TRIANGLEWAVE__AMP		(*(node->input[2]))
#define DSS_TRIANGLEWAVE__BIAS		(*(node->input[3]))
#define DSS_TRIANGLEWAVE__PHASE		(*(node->input[4]))

void dss_trianglewave_step(struct node_description *node)
{
	struct dss_trianglewave_context *context = node->context;

	if(DSS_TRIANGLEWAVE__ENABLE)
	{
		node->output=context->phase < M_PI ? (DSS_TRIANGLEWAVE__AMP * (context->phase / (M_PI/2.0) - 1.0))/2.0 :
									(DSS_TRIANGLEWAVE__AMP * (3.0 - context->phase / (M_PI/2.0)))/2.0 ;

		/* Add DC Bias component */
		node->output=node->output+DSS_TRIANGLEWAVE__BIAS;
	}
	else
	{
		node->output=0;
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*DSS_TRIANGLEWAVE__FREQ)/discrete_current_context->sample_rate)),2.0*M_PI);
}

void dss_trianglewave_reset(struct node_description *node)
{
	struct dss_trianglewave_context *context = node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(DSS_TRIANGLEWAVE__PHASE/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Step to set the output */
	dss_trianglewave_step(node);
}


/************************************************************************
 *
 * DSS_ADSR - Attack Decay Sustain Release
 *
 * input0    - Enable input value
 * input1    - Trigger value
 * input2    - gain scaling factor
 *
 ************************************************************************/
#define DSS_ADSR__ENABLE	(*(node->input[0]))

void dss_adsrenv_step(struct node_description *node)
{
//  struct dss_adsr_context *context = node->context;

	if(DSS_ADSR__ENABLE)
	{
		node->output=0;
	}
	else
	{
		node->output=0;
	}
}


void dss_adsrenv_reset(struct node_description *node)
{
	dss_adsrenv_step(node);
}
