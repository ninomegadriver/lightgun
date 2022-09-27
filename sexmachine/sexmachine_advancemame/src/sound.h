/***************************************************************************

    sound.h

    Core sound interface functions and definitions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __SOUND_H__
#define __SOUND_H__


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_ROUTES		(16)			/* maximum number of streams of any chip */
#define ALL_OUTPUTS 	(-1)			/* special value indicating all outputs for the current chip */



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* Sound route for the machine driver */
typedef struct _sound_route sound_route;
struct _sound_route
{
	int			output;					/* output ID */
	const char *target;					/* tag of the target */
	float		gain;					/* gain */
};


/* Sound configuration for the machine driver */
typedef struct _sound_config sound_config;
struct _sound_config
{
	int			sound_type;				/* what type of sound chip? */
	int			clock;					/* clock speed */
	const void *config;					/* configuration for this chip */
	const char *tag;					/* tag for this chip */
	int			routes;					/* number of routes we have */
	sound_route route[MAX_ROUTES];		/* routes for the various streams */
};


/* Speaker configuration for the machine driver */
typedef struct _speaker_config speaker_config;
struct _speaker_config
{
	const char *tag;					/* tag for this speaker */
	float		x, y, z;				/* positioning vector */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* core interfaces */
int sound_init(void);
void sound_frame_update(void);
int sound_scalebufferpos(int value);

/* global sound enable/disable */
void sound_global_enable(int enable);

/* user gain controls on speaker inputs for mixing */
int sound_get_user_gain_count(void);
void sound_set_user_gain(int index, float gain);
float sound_get_user_gain(int index);
float sound_get_default_gain(int index);
const char *sound_get_user_gain_name(int index);

/* driver gain controls on chip outputs */
void sndti_set_output_gain(int type, int index, int output, float gain);


#endif	/* __SOUND_H__ */
