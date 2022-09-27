/************************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@esplexo.co.uk)
 *
 *  (c) K.Wilkins 2000
 *
 ***********************************************************************
 *
 * DST_CRFILTER          - Simple CR filter & also highpass filter
 * DST_FILTER1           - Generic 1st order filter
 * DST_FILTER2           - Generic 2nd order filter
 * DST_OP_AMP_FILT       - Op Amp filter circuits
 * DST_RCFILTER          - Simple RC filter & also lowpass filter
 * DST_RCDISC            - Simple discharging RC
 * DST_RCDISC2           - Simple charge R1/C, discharge R0/C
 * DST_RCDISC3           - Simple charge R1/c, discharge R0*R1/(R0+R1)/C
 * DST_RCDISC4           - Various charge/discharge circuits
 * DST_RCDISC5           - Diode in series with R//C
 *
 ************************************************************************/

struct dss_filter1_context
{
	double x1;		/* x[k-1], previous input value */
	double y1;		/* y[k-1], previous output value */
	double a1;		/* digital filter coefficients, denominator */
	double b0, b1;		/* digital filter coefficients, numerator */
};

struct dss_filter2_context
{
	double x1, x2;		/* x[k-1], x[k-2], previous 2 input values */
	double y1, y2;		/* y[k-1], y[k-2], previous 2 output values */
	double a1, a2;		/* digital filter coefficients, denominator */
	double b0, b1, b2;	/* digital filter coefficients, numerator */
};

struct dst_op_amp_filt_context
{
	int		type;		// What kind of filter
	int		is_norton;	// 1 = Norton op-amps
	double	vRef;
	double	vP;
	double	vN;
	double	rTotal;		// All input resistance in parallel.
	double	iFixed;		// Current supplied by r3 & r4 if used.
	double	exponentC1;
	double	exponentC2;
	double	exponentC3;
	double	rRatio;		// divide ratio of resistance network
	double	vC1;		// Charge on C1
	double	vC1b;		// Charge on C1, part of C1 charge if needed
	double	vC2;		// Charge on C2
	double	vC3;		// Charge on C2
	double	gain;		// Gain of the filter
	double  x1, x2;		/* x[k-1], x[k-2], previous 2 input values */
	double  y1, y2;		/* y[k-1], y[k-2], previous 2 output values */
	double  a1,a2;		/* digital filter coefficients, denominator */
	double  b0,b1,b2;	/* digital filter coefficients, numerator */
};

struct dst_rcdisc_context
{
	int state;
	double t;           // time
	double exponent0;
	double exponent1;
};

struct dst_rcdisc4_context
{
	int		type;
	double	max_out;
	double	vC1;
	double	v[2];
	double	exp[2];
};

struct dst_rcfilter_context
{
	double	exponent;
	double	vCap;
};


/************************************************************************
 *
 * DST_CRFILTER - Usage of node_description values for CR filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 * input[4]    - Voltage reference. Usually 0V.
 *
 ************************************************************************/
#define DST_CRFILTER__ENABLE		(*(node->input[0]))
#define DST_CRFILTER__IN		(*(node->input[1]))
#define DST_CRFILTER__R			(*(node->input[2]))
#define DST_CRFILTER__C			(*(node->input[3]))
#define DST_CRFILTER__VREF		(*(node->input[4]))

void dst_crfilter_step(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	if(DST_CRFILTER__ENABLE)
	{
		node->output = DST_CRFILTER__IN - context->vCap;
		context->vCap += ((DST_CRFILTER__IN - DST_CRFILTER__VREF) - context->vCap) * context->exponent;
	}
	else
	{
		node->output = 0;
	}
}

void dst_crfilter_reset(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (DST_CRFILTER__R * DST_CRFILTER__C * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = DST_CRFILTER__IN;
}


/************************************************************************
 *
 * DST_FILTER1 - Generic 1st order filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Frequency value (initialization only)
 * input[3]    - Filter type (initialization only)
 *
 ************************************************************************/
#define DST_FILTER1__ENABLE	(*(node->input[0]))
#define DST_FILTER1__IN		(*(node->input[1]))
#define DST_FILTER1__FREQ	(*(node->input[2]))
#define DST_FILTER1__TYPE	(*(node->input[3]))

static void calculate_filter1_coefficients(double fc, double type,
										   double *a1, double *b0, double *b1)
{
	double den, w, two_over_T;

	/* calculate digital filter coefficents */
	/*w = 2.0*M_PI*fc; no pre-warping */
	w = discrete_current_context->sample_rate*2.0*tan(M_PI*fc/discrete_current_context->sample_rate); /* pre-warping */
	two_over_T = 2.0*discrete_current_context->sample_rate;

	den = w + two_over_T;
	*a1 = (w - two_over_T)/den;
	if (type == DISC_FILTER_LOWPASS)
	{
		*b0 = *b1 = w/den;
	}
	else if (type == DISC_FILTER_HIGHPASS)
	{
		*b0 = two_over_T/den;
		*b1 = -(*b0);
	}
	else
	{
		discrete_log("calculate_filter1_coefficients() - Invalid filter type for 1st order filter.");
	}
}

void dst_filter1_step(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (DST_FILTER1__ENABLE == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 + context->b0*gain*DST_FILTER1__IN + context->b1*context->x1;

	context->x1 = gain*DST_FILTER1__IN;
	context->y1 = node->output;
}

void dst_filter1_reset(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;

	calculate_filter1_coefficients(DST_FILTER1__FREQ, DST_FILTER1__TYPE, &context->a1, &context->b0, &context->b1);
	node->output=0;
}


/************************************************************************
 *
 * DST_FILTER2 - Generic 2nd order filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Frequency value (initialization only)
 * input[3]    - Damping value (initialization only)
 * input[4]    - Filter type (initialization only)
 *
 ************************************************************************/
#define DST_FILTER2__ENABLE	(*(node->input[0]))
#define DST_FILTER2__IN		(*(node->input[1]))
#define DST_FILTER2__FREQ	(*(node->input[2]))
#define DST_FILTER2__DAMP	(*(node->input[3]))
#define DST_FILTER2__TYPE	(*(node->input[4]))

static void calculate_filter2_coefficients(double fc, double d, double type,
										   double *a1, double *a2,
										   double *b0, double *b1, double *b2)
{
	double w;	/* cutoff freq, in radians/sec */
	double w_squared;
	double den;	/* temp variable */
	double two_over_T = 2*discrete_current_context->sample_rate;
	double two_over_T_squared = two_over_T * two_over_T;

	/* calculate digital filter coefficents */
	/*w = 2.0*M_PI*fc; no pre-warping */
	w = discrete_current_context->sample_rate*2.0*tan(M_PI*fc/discrete_current_context->sample_rate); /* pre-warping */
	w_squared = w*w;

	den = two_over_T_squared + d*w*two_over_T + w_squared;

	*a1 = 2.0*(-two_over_T_squared + w_squared)/den;
	*a2 = (two_over_T_squared - d*w*two_over_T + w_squared)/den;

	if (type == DISC_FILTER_LOWPASS)
	{
		*b0 = *b2 = w_squared/den;
		*b1 = 2.0*(*b0);
	}
	else if (type == DISC_FILTER_BANDPASS)
	{
		*b0 = d*w*two_over_T/den;
		*b1 = 0.0;
		*b2 = -(*b0);
	}
	else if (type == DISC_FILTER_HIGHPASS)
	{
		*b0 = *b2 = two_over_T_squared/den;
		*b1 = -2.0*(*b0);
	}
	else
	{
		discrete_log("calculate_filter2_coefficients() - Invalid filter type for 2nd order filter.");
	}
}

void dst_filter2_step(struct node_description *node)
{
	struct dss_filter2_context *context = node->context;
	double gain = 1.0;

	if (DST_FILTER2__ENABLE == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 - context->a2*context->y2 +
					context->b0*gain*DST_FILTER2__IN + context->b1*context->x1 + context->b2*context->x2;

	context->x2 = context->x1;
	context->x1 = gain * DST_FILTER2__IN;
	context->y2 = context->y1;
	context->y1 = node->output;
}

void dst_filter2_reset(struct node_description *node)
{
	struct dss_filter2_context *context = node->context;

	calculate_filter2_coefficients(DST_FILTER2__FREQ, DST_FILTER2__DAMP, DST_FILTER2__TYPE,
								   &context->a1, &context->a2,
								   &context->b0, &context->b1, &context->b2);
	node->output=0;
}


/************************************************************************
 *
 * DST_OP_AMP_FILT - Op Amp filter circuit RC filter
 *
 * input[0]    - Enable input value
 * input[1]    - IN0 node
 * input[2]    - IN1 node
 * input[3]    - Filter Type
 *
 * also passed discrete_op_amp_filt_info structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DST_OP_AMP_FILT__ENABLE	(*(node->input[0]))
#define DST_OP_AMP_FILT__INP1	(*(node->input[1]))
#define DST_OP_AMP_FILT__INP2	(*(node->input[2]))
#define DST_OP_AMP_FILT__TYPE	(*(node->input[3]))

void dst_op_amp_filt_step(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	double i, v=0;

	if (DST_OP_AMP_FILT__ENABLE)
	{
		if (context->is_norton)
		{
			v = DST_OP_AMP_FILT__INP1 - context->vRef;
			if (v < 0) v = 0;
		}
		else
		{
			/* Millman the input voltages. */
			i = context->iFixed;
			i += (DST_OP_AMP_FILT__INP1 - context->vRef) / info->r1;
			if (info->r2 != 0)
				i += (DST_OP_AMP_FILT__INP2 - context->vRef) / info->r2;
			v = i * context->rTotal;
		}

		switch (context->type)
		{
			case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
				node->output = (v - context->vC1) * context->gain + info->vRef;
				context->vC1 += (v - context->vC1) * context->exponentC1;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
				node->output = (v - context->vC2);
				context->vC2 += (v - context->vC2) * context->exponentC2;
				context->vC1 += (node->output - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				context->vC2 += (context->vC1 - context->vC2) * context->exponentC2;
				v = context->vC2;
				node->output = v - context->vC3;
				context->vC3 += (v - context->vC3) * context->exponentC3;
				i = node->output / context->rTotal;
				node->output = (context->iFixed - i) * info->rF;
				break;

			case DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | DISC_OP_AMP_IS_NORTON:
				node->output = v - context->vC1;
				context->vC1 += (v - context->vC1) * context->exponentC1;
				i = node->output / context->rTotal;
				node->output = (context->iFixed - i) * info->rF;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
				node->output = -context->a1*context->y1 - context->a2*context->y2 +
								context->b0*v + context->b1*context->x1 + context->b2*context->x2 +
								info->vRef;
				context->x2 = context->x1;
				context->x1 = v;
				context->y2 = context->y1;
				break;
		}

		/* Clip the output to the voltage rails.
         * This way we get the original distortion in all it's glory.
         */
		if (node->output > context->vP) node->output = context->vP;
		if (node->output < context->vN) node->output = context->vN;
		context->y1 = node->output - info->vRef;
	}
	else
		node->output = 0;

}

void dst_op_amp_filt_reset(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	/* Convert the passed filter type into an int for easy use. */
	context->type = (int)DST_OP_AMP_FILT__TYPE & DISC_OP_AMP_FILTER_TYPE_MASK;
	context->is_norton = (int)DST_OP_AMP_FILT__TYPE & DISC_OP_AMP_IS_NORTON;

	if (context->is_norton)
	{
		context->rTotal = info->r1;
		if (context->type == (DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON))
			context->rTotal += info->r2 +  info->r3;

		/* Setup the current to the + input. */
		context->iFixed = (info->vP - OP_AMP_NORTON_VBE) / info->r4;

		/* Inputs are .5V above ground. */
		context->vRef = OP_AMP_NORTON_VBE;
		/* Set the output max. */
		context->vP =  info->vP - OP_AMP_NORTON_VBE;
		context->vN =  info->vN;
	}
	else
	{
		context->vRef = info->vRef;
		/* Set the output max. */
		context->vP =  info->vP - OP_AMP_VP_RAIL_OFFSET;
		context->vN =  info->vN + OP_AMP_VP_RAIL_OFFSET;

		/* Work out the input resistance.  It is all input and bias resistors in parallel. */
		context->rTotal  = 1.0 / info->r1;			// There has to be an R1.  Otherwise the table is wrong.
		if (info->r2 != 0) context->rTotal += 1.0 / info->r2;
		if (info->r3 != 0) context->rTotal += 1.0 / info->r3;
		context->rTotal = 1.0 / context->rTotal;

		context->iFixed = 0;

		context->rRatio = info->rF / (context->rTotal + info->rF);
		context->gain = -info->rF / context->rTotal;
	}

	switch (context->type)
	{
		case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
			context->exponentC1 = -1.0 / (info->rF * info->c1 * discrete_current_context->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = 0;
			break;
		case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
			context->exponentC1 = -1.0 / (context->rTotal * info->c1 * discrete_current_context->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = 0;
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
			context->exponentC1 = -1.0 / (info->rF * info->c1 * discrete_current_context->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = -1.0 / (context->rTotal * info->c2 * discrete_current_context->sample_rate);
			context->exponentC2 = 1.0 - exp(context->exponentC2);
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
		{
			double fc = 1.0 / (2 * M_PI * sqrt(context->rTotal * info->rF * info->c1 * info->c2));
			double d = (info->c1 + info->c2) / sqrt(info->rF / context->rTotal * info->c1 * info->c2);
			double gain = -info->rF / context->rTotal * info->c2 / (info->c1 + info->c2);

			calculate_filter2_coefficients(fc, d, DISC_FILTER_BANDPASS,
										   &context->a1, &context->a2,
										   &context->b0, &context->b1, &context->b2);
			context->b0 *= gain;
			context->b1 *= gain;
			context->b2 *= gain;
			break;
		}
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON:
			context->exponentC1 = -1.0 / ((1.0 / (1.0 / info->r1 + 1.0 / (info->r2 + info->r3 + info->r4))) * info->c1 * discrete_current_context->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			context->exponentC2 = -1.0 / ((1.0 / (1.0 / (info->r1 + info->r2) + 1.0 / (info->r3 + info->r4))) * info->c2 * discrete_current_context->sample_rate);
			context->exponentC2 = 1.0 - exp(context->exponentC2);
			context->exponentC3 = -1.0 / ((info->r1 + info->r2 + info->r3 + info->r4) * info->c3 * discrete_current_context->sample_rate);
			context->exponentC3 = 1.0 - exp(context->exponentC3);
			break;
		case DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | DISC_OP_AMP_IS_NORTON:
			context->exponentC1 = -1.0 / (info->r1 * info->c1 * discrete_current_context->sample_rate);
			context->exponentC1 = 1.0 - exp(context->exponentC1);
			break;
	}

	/* At startup there is no charge on the caps and output is 0V in relation to vRef. */
	context->vC1 = 0;
	context->vC1b = 0;
	context->vC2 = 0;
	context->vC3 = 0;

	node->output = info->vRef;
}


/************************************************************************
 *
 * DST_RCDISC -   Usage of node_description values for RC discharge
 *                (inverse slope of DST_RCFILTER)
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC__ENABLE	(*(node->input[0]))
#define DST_RCDISC__IN		(*(node->input[1]))
#define DST_RCDISC__R		(*(node->input[2]))
#define DST_RCDISC__C		(*(node->input[3]))

void dst_rcdisc_step(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	switch (context->state) {
		case 0:     /* waiting for trigger  */
			if(DST_RCDISC__ENABLE) {
				context->state = 1;
				context->t = 0;
			}
			node->output=0;
			break;

		case 1:
			if (DST_RCDISC__ENABLE) {
				node->output = DST_RCDISC__IN * exp(context->t / context->exponent0);
				context->t += discrete_current_context->sample_time;
				} else {
					context->state = 0;
			}
		}
}

void dst_rcdisc_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->exponent0=-1.0 * DST_RCDISC__R * DST_RCDISC__C;
}


/************************************************************************
 *
 * DST_RCDISC2 -  Usage of node_description values for RC discharge
 *                Has switchable charge resistor/input
 *
 * input[0]    - Switch input value
 * input[1]    - input[0] value
 * input[2]    - Resistor0 value (initialization only)
 * input[3]    - input[1] value
 * input[4]    - Resistor1 value (initialization only)
 * input[5]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC2__ENABLE	(*(node->input[0]))
#define DST_RCDISC2__IN0	(*(node->input[1]))
#define DST_RCDISC2__R0		(*(node->input[2]))
#define DST_RCDISC2__IN1	(*(node->input[3]))
#define DST_RCDISC2__R1		(*(node->input[4]))
#define DST_RCDISC2__C		(*(node->input[5]))

void dst_rcdisc2_step(struct node_description *node)
{
	double diff;
	struct dst_rcdisc_context *context = node->context;

	/* Works differently to other as we are always on, no enable */
	/* exponential based in difference between input/output   */

	diff = ((DST_RCDISC2__ENABLE == 0) ? DST_RCDISC2__IN0 : DST_RCDISC2__IN1) - node->output;
	diff = diff - (diff * exp(discrete_current_context->sample_time / ((DST_RCDISC2__ENABLE == 0) ? context->exponent0 : context->exponent1)));
	node->output += diff;
}

void dst_rcdisc2_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->exponent0=-1.0 * DST_RCDISC2__R0 * DST_RCDISC2__C;
	context->exponent1=-1.0 * DST_RCDISC2__R1 * DST_RCDISC2__C;
}

/************************************************************************
 *
 * DST_RCDISC3 -  Usage of node_description values for RC discharge
 *
 *
 * input[0]    - Enable
 * input[1]    - input  value
 * input[2]    - Resistor0 value (initialization only)
 * input[4]    - Resistor1 value (initialization only)
 * input[5]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC3__ENABLE	(*(node->input[0]))
#define DST_RCDISC3__IN		(*(node->input[1]))
#define DST_RCDISC3__R1		(*(node->input[2]))
#define DST_RCDISC3__R2		(*(node->input[3]))
#define DST_RCDISC3__C		(*(node->input[4]))

void dst_rcdisc3_step(struct node_description *node)
{
	double diff;
	struct dst_rcdisc_context *context = node->context;

	/* Exponential based in difference between input/output   */

	if(DST_RCDISC3__ENABLE)
	{
		diff = DST_RCDISC3__IN - node->output;
		if( diff > 0 )
		{
			diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent0));
		} else if( diff < 0)
		{
			if(diff < -0.5)
				diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent1));
			else
				diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent0));
		}
		node->output += diff;
	}
	else
	{
		node->output=0;
	}
}

void dst_rcdisc3_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->exponent0=-1.0 * DST_RCDISC3__R1 * DST_RCDISC3__C;
	context->exponent1=-1.0 *(DST_RCDISC3__R1 * DST_RCDISC3__R2)/( DST_RCDISC3__R1 + DST_RCDISC3__R2)* DST_RCDISC3__C;
}


/************************************************************************
 *
 * DST_RCDISC4 -  Various charge/discharge circuits
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - R1 Resistor value (initialization only)
 * input[2]    - R2 Resistor value (initialization only)
 * input[4]    - C1 Capacitor Value (initialization only)
 * input[4]    - vP power source (initialization only)
 * input[4]    - circuit type (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC4__ENABLE	(*(node->input[0]))
#define DST_RCDISC4__IN		(*(node->input[1]))
#define DST_RCDISC4__R1		(*(node->input[2]))
#define DST_RCDISC4__R2		(*(node->input[3]))
#define DST_RCDISC4__R3		(*(node->input[4]))
#define DST_RCDISC4__C1		(*(node->input[5]))
#define DST_RCDISC4__VP		(*(node->input[6]))
#define DST_RCDISC4__TYPE	(*(node->input[7]))

void dst_rcdisc4_step(struct node_description *node)
{
	struct dst_rcdisc4_context *context = node->context;
	int inp1 = (DST_RCDISC4__IN == 0) ? 0 : 1;

	if (DST_RCDISC4__ENABLE == 0)
	{
		node->output = 0;
		return;
	}

	switch (context->type)
	{
		case 1:
		case 3:
			context->vC1 += ((context->v[inp1] - context->vC1) * context->exp[inp1]);
			node->output = context->vC1;
			break;
	}

	/* clip output */
	if (node->output > context->max_out) node->output = context->max_out;
	if (node->output < 0) node->output = 0;
}

void dst_rcdisc4_reset(struct node_description *node)
{
	struct dst_rcdisc4_context *context = node->context;
	double	v, i, r, rT;

	context->type = 0;
	/* some error checking. */
	if (DST_RCDISC4__R1 <= 0 || DST_RCDISC4__R2 <= 0 || DST_RCDISC4__C1 <= 0 || (DST_RCDISC4__R3 <= 0 &&  context->type == 1))
	{
		discrete_log("Invalid component values in NODE_%d.\n", node->node - NODE_00);
		return;
	}
	if (DST_RCDISC4__VP < 3)
	{
		discrete_log("vP must be >= 3V in NODE_%d.\n", node->node - NODE_00);
		return;
	}
	if (DST_RCDISC4__TYPE < 1 || DST_RCDISC4__TYPE > 3)
	{
		discrete_log("Invalid circuit type in NODE_%d.\n", node->node - NODE_00);
		return;
	}

	context->vC1 = 0;
	/* store type as integer */
	context->type = (int)DST_RCDISC4__TYPE;
	/* setup the maximum op-amp output. */
	context->max_out = DST_RCDISC4__VP - OP_AMP_VP_RAIL_OFFSET;

	switch (context->type)
	{
		case 1:
			/* We will simulate this as a voltage divider with 2 states depending
             * on the input.  But we have to take the diodes into account.
             */
			v = DST_RCDISC4__VP - .5;	/* diode drop */

			/* When the input is 1, both R1 & R3 are basically in parallel. */
			r = 1.0 / (1.0 / DST_RCDISC4__R1 + 1.0 / DST_RCDISC4__R3);
			rT = DST_RCDISC4__R2 + r;
			i = v / rT;
			context->v[1] = i * r + .5;
			rT = 1.0 / (1.0 / DST_RCDISC4__R2 + 1.0 / r);
			context->exp[1] = -1.0 / (rT * DST_RCDISC4__C1 * discrete_current_context->sample_rate);
			context->exp[1] = 1.0 - exp(context->exp[1]);

			/* When the input is 0, R1 is out of circuit. */
			rT = DST_RCDISC4__R2 + DST_RCDISC4__R3;
			i = v / rT;
			context->v[0] = i * DST_RCDISC4__R3 + .5;
			rT = 1.0 / (1.0 / DST_RCDISC4__R2 + 1.0 / DST_RCDISC4__R3);
			context->exp[0] = -1.0 / (rT * DST_RCDISC4__C1 * discrete_current_context->sample_rate);
			context->exp[0] = 1.0 - exp(context->exp[0]);
			break;

		case 3:
			/* We will simulate this as a voltage divider with 2 states depending
             * on the input.  The 1k pullup is in parallel with the internal TTL
             * resistance, so we will just use .5k in series with R1.
             */
			r = 500.0 + DST_RCDISC4__R1;
			context->v[1] = DST_RCDISC4__R2 / (r + DST_RCDISC4__R2) * (5.0 - 0.5);
			rT = 1.0 / ( 1.0 / r + 1.0 / DST_RCDISC4__R2);
			context->exp[1] = -1.0 / (rT * DST_RCDISC4__C1 * discrete_current_context->sample_rate);
			context->exp[1] = 1.0 - exp(context->exp[1]);

			/* When the input is 0, R1 is out of circuit. */
			context->v[0] = 0;
			context->exp[0] = -1.0 / (DST_RCDISC4__R2 * DST_RCDISC4__C1 * discrete_current_context->sample_rate);
			context->exp[0] = 1.0 - exp(context->exp[0]);
			break;
	}
}

/************************************************************************
 *
 * DST_RCDISC5 -  Diode in series with R//C
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC5__ENABLE	(*(node->input[0]))
#define DST_RCDISC5__IN		(*(node->input[1]))
#define DST_RCDISC5__R		(*(node->input[2]))
#define DST_RCDISC5__C		(*(node->input[3]))

void dst_rcdisc5_step(struct node_description *node)
{
	double diff,u;
	struct dst_rcdisc_context *context = node->context;

	/* Exponential based in difference between input/output   */

	if(DST_RCDISC5__ENABLE)
	{
	    u = DST_RCDISC5__IN - 0.7; /* Diode drop */
		if( u < 0)
			u = 0;

		diff = u - node->output;

		if(diff < 0)
			diff = diff - (diff * exp(discrete_current_context->sample_time / context->exponent0));
		node->output += diff;
	}
	else
	{
		node->output=0;
	}
}

void dst_rcdisc5_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->exponent0=-1.0 * DST_RCDISC5__R * DST_RCDISC5__C;
}

/************************************************************************
 *
 * DST_RCFILTER - Usage of node_description values for RC filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 * input[4]    - Voltage reference. Usually 0V.
 *
 ************************************************************************/
#define DST_RCFILTER__ENABLE		(*(node->input[0]))
#define DST_RCFILTER__VIN		(*(node->input[1]))
#define DST_RCFILTER__R			(*(node->input[2]))
#define DST_RCFILTER__C			(*(node->input[3]))
#define DST_RCFILTER__VREF		(*(node->input[4]))

void dst_rcfilter_step(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	/************************************************************************/
	/* Next Value = PREV + (INPUT_VALUE - PREV)*(1-(EXP(-TIMEDELTA/RC)))    */
	/************************************************************************/

	if(DST_RCFILTER__ENABLE)
	{
		context->vCap += ((DST_RCFILTER__VIN - DST_RCFILTER__VREF - context->vCap) * context->exponent);
		node->output = context->vCap + DST_RCFILTER__VREF;
	}
	else
	{
		node->output=0;
	}
}

void dst_rcfilter_reset(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	context->exponent = -1.0 / (DST_RCFILTER__R * DST_RCFILTER__C * discrete_current_context->sample_rate);
	context->exponent = 1.0 - exp(context->exponent);
	context->vCap = 0;
	node->output = 0;
}


/* !!!!!!!!!!! NEW FILTERS for testing !!!!!!!!!!!!!!!!!!!!! */


/************************************************************************
 *
 * DST_RCFILTERN - Usage of node_description values for RC filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCFILTERN__ENABLE		(*(node->input[0]))
#define DST_RCFILTERN__IN		(*(node->input[1]))
#define DST_RCFILTERN__R		(*(node->input[2]))
#define DST_RCFILTERN__C		(*(node->input[3]))

void dst_rcfilterN_reset(struct node_description *node)
{
//  double f=1.0/(2*M_PI* DST_RCFILTERN__R * DST_RCFILTERN__C);

// !!!!!!!!!!!!!! CAN'T CHEAT LIKE THIS !!!!!!!!!!!!!!!!
// Put this stuff in a context
//
//  node->input[2] = f;
//  node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	dst_filter1_reset(node);
}


/************************************************************************
 *
 * DST_RCDISCN -   Usage of node_description values for RC discharge
 *                (inverse slope of DST_RCFILTER)
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISCN__ENABLE	(*(node->input[0]))
#define DST_RCDISCN__IN		(*(node->input[1]))
#define DST_RCDISCN__R		(*(node->input[2]))
#define DST_RCDISCN__C		(*(node->input[3]))

void dst_rcdiscN_reset(struct node_description *node)
{
//  double f=1.0/(2*M_PI* DST_RCDISCN__R * DST_RCDISCN__C);

// !!!!!!!!!!!!!! CAN'T CHEAT LIKE THIS !!!!!!!!!!!!!!!!
// Put this stuff in a context
//
//  node->input[2] = f;
//  node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	dst_filter1_reset(node);
}

void dst_rcdiscN_step(struct node_description *node)
{
	struct dss_filter1_context *context = node->context;
	double gain = 1.0;

	if (DST_RCDISCN__ENABLE == 0.0)
	{
		gain = 0.0;
	}

	/* A rise in the input signal results in an instant charge, */
	/* else discharge through the RC to zero */
	if (gain* DST_RCDISCN__IN > context->x1)
		node->output = gain* DST_RCDISCN__IN;
	else
		node->output = -context->a1*context->y1;

	context->x1 = gain* DST_RCDISCN__IN;
	context->y1 = node->output;
}


/************************************************************************
 *
 * DST_RCDISC2N -  Usage of node_description values for RC discharge
 *                Has switchable charge resistor/input
 *
 * input[0]    - Switch input value
 * input[1]    - input[0] value
 * input[2]    - Resistor0 value (initialization only)
 * input[3]    - input[1] value
 * input[4]    - Resistor1 value (initialization only)
 * input[5]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC2N__ENABLE		(*(node->input[0]))
#define DST_RCDISC2N__IN0		(*(node->input[1]))
#define DST_RCDISC2N__R0		(*(node->input[2]))
#define DST_RCDISC2N__IN1		(*(node->input[3]))
#define DST_RCDISC2N__R1		(*(node->input[4]))
#define DST_RCDISC2N__C			(*(node->input[5]))

struct dss_rcdisc2_context
{
	double x1;			/* x[k-1], last input value */
	double y1;			/* y[k-1], last output value */
	double a1_0, b0_0, b1_0;	/* digital filter coefficients, filter #1 */
	double a1_1, b0_1, b1_1;	/* digital filter coefficients, filter #2 */
};

void dst_rcdisc2N_step(struct node_description *node)
{
	struct dss_rcdisc2_context *context = node->context;
	double input = ((DST_RCDISC2N__ENABLE == 0) ? DST_RCDISC2N__IN0 : DST_RCDISC2N__IN1);

	if (DST_RCDISC2N__ENABLE == 0)
		node->output = -context->a1_0*context->y1 + context->b0_0*input + context->b1_0*context->x1;
	else
		node->output = -context->a1_1*context->y1 + context->b0_1*input + context->b1_1*context->x1;

	context->x1 = input;
	context->y1 = node->output;
}

void dst_rcdisc2N_reset(struct node_description *node)
{
	struct dss_rcdisc2_context *context = node->context;
	double f1,f2;

	f1=1.0/(2*M_PI* DST_RCDISC2N__R0 * DST_RCDISC2N__C);
	f2=1.0/(2*M_PI* DST_RCDISC2N__R1 * DST_RCDISC2N__C);

	calculate_filter1_coefficients(f1, DISC_FILTER_LOWPASS, &context->a1_0, &context->b0_0, &context->b1_0);
	calculate_filter1_coefficients(f2, DISC_FILTER_LOWPASS, &context->a1_1, &context->b0_1, &context->b1_1);

	/* Initialize the object */
	node->output=0;
}
