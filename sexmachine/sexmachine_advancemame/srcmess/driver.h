/***************************************************************************

    driver.h

    Include this with all MAME files. Includes all the core system pieces.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __DRIVER_H__
#define __DRIVER_H__


/***************************************************************************
    MACROS (must be *before* the includes below)
***************************************************************************/

#define DRIVER_INIT(name)		void init_##name(void)

#define NVRAM_HANDLER(name)		void nvram_handler_##name(mame_file *file, int read_or_write)

#define MEMCARD_HANDLER(name)	void memcard_handler_##name(mame_file *file, int action)

#define MACHINE_START(name)		int machine_start_##name(void)
#define MACHINE_RESET(name)		void machine_reset_##name(void)

#define SOUND_START(name)		int sound_start_##name(void)
#define SOUND_RESET(name)		void sound_reset_##name(void)

#define VIDEO_START(name)		int video_start_##name(void)
#define VIDEO_RESET(name)		void video_reset_##name(void)

#define PALETTE_INIT(name)		void palette_init_##name(UINT16 *colortable, const UINT8 *color_prom)
#define VIDEO_EOF(name)			void video_eof_##name(void)
#ifdef MESS
#define VIDEO_UPDATE(name)		void video_update_##name(int screen, mame_bitmap *bitmap, const rectangle *cliprect, int *do_skip)
#else
#define VIDEO_UPDATE(name)		void video_update_##name(int screen, mame_bitmap *bitmap, const rectangle *cliprect)
#endif

/* NULL versions */
#define init_NULL				NULL
#define nvram_handler_NULL 		NULL
#define memcard_handler_NULL	NULL
#define machine_start_NULL 		NULL
#define machine_reset_NULL 		NULL
#define sound_start_NULL 		NULL
#define sound_reset_NULL 		NULL
#define video_start_NULL 		NULL
#define video_reset_NULL 		NULL
#define palette_init_NULL		NULL
#define video_eof_NULL 			NULL
#define video_update_NULL 		NULL



/***************************************************************************
    INCLUDES
***************************************************************************/

#include "mamecore.h"
#include "memory.h"
#include "mame.h"
#include "fileio.h"
#include "drawgfx.h"
#include "video.h"
#include "palette.h"
#include "cpuintrf.h"
#include "sndintrf.h"
#include "sound.h"
#include "input.h"
#include "inptport.h"
#include "usrintrf.h"
#include "tilemap.h"
#include "state.h"
#include "romload.h"
#include "machine/generic.h"
#include "sndhrdw/generic.h"
#include "vidhrdw/generic.h"

#ifdef MESS
#include "messdrv.h"
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* maxima */
#define MAX_CPU			8
#define MAX_SOUND		32
#define MAX_SPEAKER 	4

/* watchdog constants */
#define DEFAULT_60HZ_3S_VBLANK_WATCHDOG	180
#define DEFAULT_30HZ_3S_VBLANK_WATCHDOG	90



/* ----- VBLANK constants ----- */

/*
    VBLANK is the period when the video beam is outside of the visible area
    and returns from the bottom-right to the top-left of the screen to
    prepare for a new video frame. The duration of VBLANK is an important
    factor in how the game renders itself. In many cases, the game does
    video-related operations during this time, such as swapping buffers or
    updating sprite lists.

    Below are some predefined, TOTALLY ARBITRARY values for vblank_duration,
    which should be OK for most cases. I have NO IDEA how accurate they are
    compared to the real hardware, they could be completely wrong.
*/

/* The default is to have no VBLANK timing -- this is historical, and a bad idea */
#define DEFAULT_60HZ_VBLANK_DURATION		0
#define DEFAULT_30HZ_VBLANK_DURATION		0

/* If you use IPT_VBLANK, you need a duration different from 0 */
#define DEFAULT_REAL_60HZ_VBLANK_DURATION	2500
#define DEFAULT_REAL_30HZ_VBLANK_DURATION	2500



/* ----- flags for video_attributes ----- */

/* is the video hardware raser or vector base? */
#define	VIDEO_TYPE_RASTER				0x0000
#define	VIDEO_TYPE_VECTOR				0x0001

/* should VIDEO_UPDATE by called at the start of VBLANK or at the end? */
#define	VIDEO_UPDATE_BEFORE_VBLANK		0x0000
#define	VIDEO_UPDATE_AFTER_VBLANK		0x0002

/* set this to use a direct RGB bitmap rather than a palettized bitmap */
#define VIDEO_RGB_DIRECT	 			0x0004

/* set this if the color resolution of *any* component is 6 bits or more */
#define VIDEO_NEEDS_6BITS_PER_GUN		0x0008

/* automatically extend the palette creating a darker copy for shadows */
#define VIDEO_HAS_SHADOWS				0x0010

/* automatically extend the palette creating a brighter copy for highlights */
#define VIDEO_HAS_HIGHLIGHTS			0x0020

/* Mish 181099:  See comments in vidhrdw/generic.c for details */
#define VIDEO_BUFFERS_SPRITERAM			0x0040

/* In most cases we assume pixels are square (1:1 aspect ratio) but some games need */
/* different proportions, e.g. 1:2 for Blasteroids */
#define VIDEO_PIXEL_ASPECT_RATIO_MASK	0x0300
#define VIDEO_PIXEL_ASPECT_RATIO_1_1	0x0000
#define VIDEO_PIXEL_ASPECT_RATIO_1_2	0x0100
#define VIDEO_PIXEL_ASPECT_RATIO_2_1	0x0200



/* ----- flags for game drivers ----- */

#define ORIENTATION_MASK        		0x0007
#define	ORIENTATION_FLIP_X				0x0001	/* mirror everything in the X direction */
#define	ORIENTATION_FLIP_Y				0x0002	/* mirror everything in the Y direction */
#define ORIENTATION_SWAP_XY				0x0004	/* mirror along the top-left/bottom-right diagonal */

#define	ROT0							0
#define	ROT90							(ORIENTATION_SWAP_XY | ORIENTATION_FLIP_X)	/* rotate clockwise 90 degrees */
#define	ROT180							(ORIENTATION_FLIP_X | ORIENTATION_FLIP_Y)	/* rotate 180 degrees */
#define	ROT270							(ORIENTATION_SWAP_XY | ORIENTATION_FLIP_Y)	/* rotate counter-clockwise 90 degrees */

#define GAME_NOT_WORKING				0x0008
#define GAME_UNEMULATED_PROTECTION		0x0010	/* game's protection not fully emulated */
#define GAME_WRONG_COLORS				0x0020	/* colors are totally wrong */
#define GAME_IMPERFECT_COLORS			0x0040	/* colors are not 100% accurate, but close */
#define GAME_IMPERFECT_GRAPHICS			0x0080	/* graphics are wrong/incomplete */
#define GAME_NO_COCKTAIL				0x0100	/* screen flip support is missing */
#define GAME_NO_SOUND					0x0200	/* sound is missing */
#define GAME_IMPERFECT_SOUND			0x0400	/* sound is known to be wrong */
#define GAME_SUPPORTS_SAVE				0x0800	/* game supports save states */
#define NOT_A_DRIVER					0x4000	/* set by the fake "root" driver_0 and by "containers" */

#ifdef MESS
#define GAME_COMPUTER               	0x8000  /* Driver is a computer (needs full keyboard) */
#define GAME_COMPUTER_MODIFIED      	0x0800	/* Official? Hack */
#endif



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* In mamecore.h: typedef struct _machine_config machine_config; */
struct _machine_config
{
	cpu_config			cpu[MAX_CPU];				/* array of CPUs in the system */
	float				frames_per_second;			/* number of frames per second */
	int					vblank_duration;			/* duration of a VBLANK, in msec/usec */
	UINT32				cpu_slices_per_frame;		/* number of times to interleave execution per frame */
	INT32				watchdog_vblank_count;		/* number of VBLANKs until the watchdog kills us */
	double				watchdog_time;				/* length of time until the watchdog kills us */

	int 				(*machine_start)(void);		/* one-time machine start callback */
	void 				(*machine_reset)(void);		/* machine reset callback */

	void 				(*nvram_handler)(mame_file *file, int read_or_write); /* NVRAM save/load callback  */
	void 				(*memcard_handler)(mame_file *file, int action); /* memory card save/load callback  */

	UINT32				video_attributes;			/* flags describing the video system */
	UINT32				aspect_x, aspect_y;			/* aspect ratio of the video screen */
	int					screen_width, screen_height;/* size of the video screen (in pixels) */
	rectangle			default_visible_area;		/* default visible area of the screen */
	const gfx_decode *	gfxdecodeinfo;				/* pointer to graphics decoding information */
	UINT32				total_colors;				/* total number of colors in the palette */
	UINT32				color_table_len;			/* length of the color indirection table */

	void 				(*init_palette)(UINT16 *colortable, const UINT8 *color_prom); /* one-time palette init callback  */
	int					(*video_start)(void);		/* one-time video start callback */
	void				(*video_reset)(void);		/* video reset callback */
	void				(*video_eof)(void);			/* end-of-frame video callback */
#ifdef MESS
	void				(*video_update)(int screen, mame_bitmap *bitmap, const rectangle *cliprect, int *do_skip);
#else
	void				(*video_update)(int screen, mame_bitmap *bitmap, const rectangle *cliprect); /* video update callback */
#endif

	sound_config		sound[MAX_SOUND];			/* array of sound chips in the system */
	speaker_config		speaker[MAX_SPEAKER];		/* array of speakers in the system */

	int					(*sound_start)(void);		/* one-time sound start callback */
	void				(*sound_reset)(void);		/* sound reset callback */
};


/* In mamecore.h: typedef struct _game_driver game_driver; */
struct _game_driver
{
	const char *		source_file;				/* set this to __FILE__ */
	const char *		parent;						/* if this is a clone, the name of the parent */
	const char *		name;						/* short (8-character) name of the game */
	const bios_entry *	bios;						/* list of names and ROM_BIOSFLAGS */
	const char *		description;				/* full name of the game */
	const char *		year;						/* year the game was released */
	const char *		manufacturer;				/* manufacturer of the game */
	void 				(*drv)(machine_config *);	/* machine driver constructor */
	void 				(*construct_ipt)(input_port_init_params *param); /* input port constructor */
	void				(*driver_init)(void);		/* DRIVER_INIT callback */
	const rom_entry *	rom;						/* pointer to list of ROMs for the game */

#ifdef MESS
	void (*sysconfig_ctor)(struct SystemConfigurationParamBlock *cfg);
	const game_driver *	compatible_with;
#endif

	UINT32				flags;						/* orientation and other flags; see defines below */
};



/***************************************************************************
    MACROS
***************************************************************************/

/***************************************************************************

    Macros for building machine drivers

***************************************************************************/

/* use this to declare external references to a machine driver */
#define MACHINE_DRIVER_EXTERN(game)										\
	void construct_##game(machine_config *machine)						\


/* start/end tags for the machine driver */
#define MACHINE_DRIVER_START(game) 										\
	void construct_##game(machine_config *machine)						\
	{																	\
		cpu_config *cpu = NULL;											\
		sound_config *sound = NULL;										\
		(void)cpu;														\
		(void)sound;													\

#define MACHINE_DRIVER_END 												\
	}																	\


/* importing data from other machine drivers */
#define MDRV_IMPORT_FROM(game) 											\
	construct_##game(machine); 											\


/* add/modify/remove/replace CPUs */
#define MDRV_CPU_ADD_TAG(tag, type, clock)								\
	cpu = driver_add_cpu(machine, (tag), CPU_##type, (clock));			\

#define MDRV_CPU_ADD(type, clock)										\
	MDRV_CPU_ADD_TAG(NULL, type, clock)									\

#define MDRV_CPU_MODIFY(tag)											\
	cpu = driver_find_cpu(machine, tag);								\

#define MDRV_CPU_REMOVE(tag)											\
	driver_remove_cpu(machine, tag);									\
	cpu = NULL;															\

#define MDRV_CPU_REPLACE(tag, type, clock)								\
	cpu = driver_find_cpu(machine, tag);								\
	if (cpu)															\
	{																	\
		cpu->cpu_type = (CPU_##type);									\
		cpu->cpu_clock = (clock);										\
	}																	\


/* CPU parameters */
#define MDRV_CPU_FLAGS(flags)											\
	if (cpu)															\
		cpu->cpu_flags = (flags);										\

#define MDRV_CPU_CONFIG(config)											\
	if (cpu)															\
		cpu->reset_param = &(config);									\

#define MDRV_CPU_PROGRAM_MAP(readmem, writemem)							\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_PROGRAM][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_PROGRAM][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_DATA_MAP(readmem, writemem)							\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_DATA][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_DATA][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_IO_MAP(readmem, writemem)								\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_IO][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_IO][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_VBLANK_INT(func, rate)									\
	if (cpu)															\
	{																	\
		cpu->vblank_interrupt = func;									\
		cpu->vblank_interrupts_per_frame = (rate);						\
	}																	\

#define MDRV_CPU_PERIODIC_INT(func, rate)								\
	if (cpu)															\
	{																	\
		cpu->timed_interrupt = func;									\
		cpu->timed_interrupt_period = (rate);							\
	}																	\


/* core parameters */
#define MDRV_FRAMES_PER_SECOND(rate)									\
	machine->frames_per_second = (rate);								\

#define MDRV_VBLANK_DURATION(duration)									\
	machine->vblank_duration = (duration);								\

#define MDRV_INTERLEAVE(interleave)										\
	machine->cpu_slices_per_frame = (interleave);						\

#define MDRV_WATCHDOG_VBLANK_INIT(watch_count)							\
	machine->watchdog_vblank_count = (watch_count);						\

#define MDRV_WATCHDOG_TIME_INIT(time)									\
	machine->watchdog_time = (time);									\


/* core functions */
#define MDRV_MACHINE_START(name)										\
	machine->machine_start = machine_start_##name;						\

#define MDRV_MACHINE_RESET(name)										\
	machine->machine_reset = machine_reset_##name;						\

#define MDRV_NVRAM_HANDLER(name)										\
	machine->nvram_handler = nvram_handler_##name;						\

#define MDRV_MEMCARD_HANDLER(name)										\
	machine->memcard_handler = memcard_handler_##name;					\


/* core video parameters */
#define MDRV_VIDEO_ATTRIBUTES(flags)									\
	machine->video_attributes = (flags);								\

#define MDRV_ASPECT_RATIO(num, den)										\
	machine->aspect_x = (num);											\
	machine->aspect_y = (den);											\

#define MDRV_SCREEN_SIZE(width, height)									\
	machine->screen_width = (width);									\
	machine->screen_height = (height);									\

#define MDRV_VISIBLE_AREA(minx, maxx, miny, maxy)						\
	machine->default_visible_area.min_x = (minx);						\
	machine->default_visible_area.max_x = (maxx);						\
	machine->default_visible_area.min_y = (miny);						\
	machine->default_visible_area.max_y = (maxy);						\

#define MDRV_GFXDECODE(gfx)												\
	machine->gfxdecodeinfo = (gfx);										\

#define MDRV_PALETTE_LENGTH(length)										\
	machine->total_colors = (length);									\

#define MDRV_COLORTABLE_LENGTH(length)									\
	machine->color_table_len = (length);								\


/* core video functions */
#define MDRV_PALETTE_INIT(name)											\
	machine->init_palette = palette_init_##name;						\

#define MDRV_VIDEO_START(name)											\
	machine->video_start = video_start_##name;							\

#define MDRV_VIDEO_RESET(name)											\
	machine->video_reset = video_reset_##name;							\

#define MDRV_VIDEO_EOF(name)											\
	machine->video_eof = video_eof_##name;								\

#define MDRV_VIDEO_UPDATE(name)											\
	machine->video_update = video_update_##name;						\


/* add/remove speakers */
#define MDRV_SPEAKER_ADD(tag, x, y, z)									\
	driver_add_speaker(machine, (tag), (x), (y), (z));					\

#define MDRV_SPEAKER_REMOVE(tag)										\
	driver_remove_speaker(machine, (tag));								\

#define MDRV_SPEAKER_STANDARD_MONO(tag)									\
	MDRV_SPEAKER_ADD(tag, 0.0, 0.0, 1.0)								\

#define MDRV_SPEAKER_STANDARD_STEREO(tagl, tagr)						\
	MDRV_SPEAKER_ADD(tagl, -0.2, 0.0, 1.0)								\
	MDRV_SPEAKER_ADD(tagr, 0.2, 0.0, 1.0)								\


/* core sound functions */
#define MDRV_SOUND_START(name)											\
	machine->sound_start = sound_start_##name;							\

#define MDRV_SOUND_RESET(name)											\
	machine->sound_reset = sound_reset_##name;							\


/* add/remove/replace sounds */
#define MDRV_SOUND_ADD_TAG(tag, type, clock)							\
	sound = driver_add_sound(machine, (tag), SOUND_##type, (clock));	\

#define MDRV_SOUND_ADD(type, clock)										\
	MDRV_SOUND_ADD_TAG(NULL, type, clock)								\

#define MDRV_SOUND_REMOVE(tag)											\
	driver_remove_sound(machine, tag);									\

#define MDRV_SOUND_MODIFY(tag)											\
	sound = driver_find_sound(machine, tag);							\
	if (sound)															\
		sound->routes = 0;												\

#define MDRV_SOUND_CONFIG(_config)										\
	if (sound)															\
		sound->config = &(_config);										\

#define MDRV_SOUND_REPLACE(tag, type, _clock)							\
	sound = driver_find_sound(machine, tag);							\
	if (sound)															\
	{																	\
		sound->sound_type = SOUND_##type;								\
		sound->clock = (_clock);										\
		sound->config = NULL;											\
		sound->routes = 0;												\
	}																	\

#define MDRV_SOUND_ROUTE(_output, _target, _gain)						\
	if (sound)															\
	{																	\
		sound->route[sound->routes].output = (_output);					\
		sound->route[sound->routes].target = (_target);					\
		sound->route[sound->routes].gain = (_gain);						\
		sound->routes++;												\
	}																	\



/***************************************************************************

    Macros for building game drivers

***************************************************************************/

#define GAME(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)	\
game_driver driver_##NAME =					\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	(MONITOR)|(FLAGS)						\
};

#define GAMEB(YEAR,NAME,PARENT,BIOS,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)	\
game_driver driver_##NAME =					\
{											\
	__FILE__,								\
	#PARENT,								\
	#NAME,									\
	system_bios_##BIOS,						\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	(MONITOR)|(FLAGS)						\
};

/* this allows to leave the INIT field empty in the GAME() macro call */
#define init_0 0

/* this allows to leave the BIOS field empty in the GAMEB() macro call */
#define system_bios_0 0



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern const game_driver * const drivers[];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void expand_machine_driver(void (*constructor)(machine_config *), machine_config *output);

cpu_config *driver_add_cpu(machine_config *machine, const char *tag, int type, int cpuclock);
cpu_config *driver_find_cpu(machine_config *machine, const char *tag);
void driver_remove_cpu(machine_config *machine, const char *tag);

speaker_config *driver_add_speaker(machine_config *machine, const char *tag, float x, float y, float z);
speaker_config *driver_find_speaker(machine_config *machine, const char *tag);
void driver_remove_speaker(machine_config *machine, const char *tag);

sound_config *driver_add_sound(machine_config *machine, const char *tag, int type, int clock);
sound_config *driver_find_sound(machine_config *machine, const char *tag);
void driver_remove_sound(machine_config *machine, const char *tag);

const game_driver *driver_get_clone(const game_driver *driver);


#endif	/* __DRIVER_H__ */
