/***************************************************************************

    osdepend.h

    OS-dependent code interface.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __OSDEPEND_H__
#define __OSDEPEND_H__

#include "mamecore.h"
#include <stdarg.h>

int osd_init(void);

#ifdef NEW_DEBUGGER
void osd_wait_for_debugger(void);
#endif


/******************************************************************************

    Display

******************************************************************************/

/* these are the parameters passed into osd_create_display */
/* In mamecore.h: typedef struct _osd_create_params osd_create_params; */
struct _osd_create_params
{
	int width, height;			/* width and height */
	int aspect_x, aspect_y;		/* aspect ratio X:Y */
	int depth;					/* depth, either 16(palette), 15(RGB) or 32(RGB) */
	int colors;					/* colors in the palette (including UI) */
	float fps;					/* frame rate */
	int video_attributes;		/* video flags from driver */
};



/*
  Create a display screen, or window, of the given dimensions (or larger). It is
  acceptable to create a smaller display if necessary, in that case the user must
  have a way to move the visibility window around.

  The params contains all the information the
  Attributes are the ones defined in driver.h, they can be used to perform
  optimizations, e.g. dirty rectangle handling if the game supports it, or faster
  blitting routines with fixed palette if the game doesn't change the palette at
  run time. The VIDEO_PIXEL_ASPECT_RATIO flags should be honored to produce a
  display of correct proportions.
  Orientation is the screen orientation (as defined in driver.h) which will be done
  by the core. This can be used to select thinner screen modes for vertical games
  (ORIENTATION_SWAP_XY set), or even to ask the user to rotate the monitor if it's
  a pivot model. Note that the OS dependent code must NOT perform any rotation,
  this is done entirely in the core.
  Depth can be 8 or 16 for palettized modes, meaning that the core will store in the
  bitmaps logical pens which will have to be remapped through a palette at blit time,
  and 15 or 32 for direct mapped modes, meaning that the bitmaps will contain RGB
  triplets (555 or 888). For direct mapped modes, the VIDEO_RGB_DIRECT flag is set
  in the attributes field.

  Returns 0 on success.
*/
int osd_create_display(const osd_create_params *params, UINT32 *rgb_components);
void osd_close_display(void);


/*
  osd_skip_this_frame() must return 0 if the current frame will be displayed.
  This can be used by drivers to skip cpu intensive processing for skipped
  frames, so the function must return a consistent result throughout the
  current frame. The function MUST NOT check timers and dynamically determine
  whether to display the frame: such calculations must be done in
  osd_update_video_and_audio(), and they must affect the FOLLOWING frames, not
  the current one. At the end of osd_update_video_and_audio(), the code must
  already know exactly whether the next frame will be skipped or not.
*/
int osd_skip_this_frame(void);


/*
  Update video and audio. game_bitmap contains the game display, while
  debug_bitmap an image of the debugger window (if the debugger is active; NULL
  otherwise). They can be shown one at a time, or in two separate windows,
  depending on the OS limitations. If only one is shown, the user must be able
  to toggle between the two by pressing IPT_UI_TOGGLE_DEBUG; moreover,
  osd_debugger_focus() will be used by the core to force the display of a
  specific bitmap, e.g. the debugger one when the debugger becomes active.

  leds_status is a bitmask of lit LEDs, usually player start lamps. They can be
  simulated using the keyboard LEDs, or in other ways e.g. by placing graphics
  on the window title bar.
*/
void osd_update_video_and_audio(struct _mame_display *display);


/*
  Provides a hook to allow the OSD system to override processing of a
  snapshot.  This function will either return a new bitmap, for which the
  caller is responsible for freeing.
*/
mame_bitmap *osd_override_snapshot(mame_bitmap *bitmap, rectangle *bounds);

/*
  Returns a pointer to the text to display when the FPS display is toggled.
  This normally includes information about the frameskip, FPS, and percentage
  of full game speed.
*/
const char *osd_get_fps_text(const performance_info *performance);



/******************************************************************************

    Sound

******************************************************************************/

/*
  osd_start_audio_stream() is called at the start of the emulation to initialize
  the output stream, then osd_update_audio_stream() is called every frame to
  feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

  The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
  When the stream is stereo, left and right samples are alternated in the
  stream.

  osd_start_audio_stream() and osd_update_audio_stream() must return the number
  of samples (or couples of samples, when using stereo) required for next frame.
  This will be around Machine->sample_rate / Machine->drv->frames_per_second,
  the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
  maintain audio and video in sync when using vsync. Note that sound emulation,
  especially when DACs are involved, greatly depends on the number of samples
  per frame to be roughly constant, so the returned value must always stay close
  to the reference value of Machine->sample_rate / Machine->drv->frames_per_second.
  Of course that value is not necessarily an integer so at least a +/- 1
  adjustment is necessary to avoid drifting over time.
*/
int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

/*
  control master volume. attenuation is the attenuation in dB (a negative
  number). To convert from dB to a linear volume scale do the following:
    volume = MAX_VOLUME;
    while (attenuation++ < 0)
        volume /= 1.122018454;      //  = (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation);
int osd_get_mastervolume(void);

void osd_sound_enable(int enable);



/******************************************************************************

    Controls

******************************************************************************/

typedef UINT32 os_code;

typedef struct _os_code_info os_code_info;
struct _os_code_info
{
	char *			name;			/* OS dependant name; 0 terminates the list */
	os_code		oscode;			/* OS dependant code */
	input_code	inputcode;		/* CODE_xxx equivalent from input.h, or one of CODE_OTHER_* if n/a */
};

/*
  return a list of all available inputs (see input.h)
*/
const os_code_info *osd_get_code_list(void);

/*
  return the value of the specified input. digital inputs return 0 or 1. analog
  inputs should return a value between -65536 and +65536. oscode is the OS dependent
  code specified in the list returned by osd_get_code_list().
*/
INT32 osd_get_code_value(os_code oscode);

/*
  Return the Unicode value of the most recently pressed key. This
  function is used only by text-entry routines in the user interface and should
  not be used by drivers. The value returned is in the range of the first 256
  bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

  Set flush to 1 to clear the buffer before entering text. This will avoid
  having prior UI and game keys leak into the text entry.
*/
int osd_readkey_unicode(int flush);

/*
  inptport.c defines some general purpose defaults for key and joystick bindings.
  They may be further adjusted by the OS dependent code to better match the
  available keyboard, e.g. one could map pause to the Pause key instead of P, or
  snapshot to PrtScr instead of F12. Of course the user can further change the
  settings to anything he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_list(input_port_default_entry *defaults);


/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration(void);
/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void);
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void);
/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void);
/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void);



/******************************************************************************

    File I/O

******************************************************************************/

/* inp header */
typedef struct _inp_header inp_header;
struct _inp_header
{
	char name[9];      /* 8 bytes for game->name + NUL */
	char version[3];   /* byte[0] = 0, byte[1] = version byte[2] = beta_version */
	char reserved[20]; /* for future use, possible store game options? */
};


/* These values are returned by osd_get_path_info */
enum
{
	PATH_NOT_FOUND,
	PATH_IS_FILE,
	PATH_IS_DIRECTORY
};

/* These values are returned as error codes by osd_fopen() */
enum _osd_file_error
{
	FILEERR_SUCCESS,
	FILEERR_FAILURE,
	FILEERR_OUT_OF_MEMORY,
	FILEERR_NOT_FOUND,
	FILEERR_ACCESS_DENIED,
	FILEERR_ALREADY_OPEN,
	FILEERR_TOO_MANY_FILES
};

typedef struct _osd_file osd_file;

/* Return the number of paths for a given type */
int osd_get_path_count(int pathtype);

/* Get information on the existence of a file */
int osd_get_path_info(int pathtype, int pathindex, const char *filename);

/* Create a directory if it doesn't already exist */
int osd_create_directory(int pathtype, int pathindex, const char *dirname);

/* Attempt to open a file with the given name and mode using the specified path type */
osd_file *osd_fopen(int pathtype, int pathindex, const char *filename, const char *mode, osd_file_error *error);

/* Seek within a file */
int osd_fseek(osd_file *file, INT64 offset, int whence);

/* Return current file position */
UINT64 osd_ftell(osd_file *file);

/* Return 1 if we're at the end of file */
int osd_feof(osd_file *file);

/* Read bytes from a file */
UINT32 osd_fread(osd_file *file, void *buffer, UINT32 length);

/* Write bytes to a file */
UINT32 osd_fwrite(osd_file *file, const void *buffer, UINT32 length);

/* Close an open file */
void osd_fclose(osd_file *file);



/******************************************************************************

    Timing

******************************************************************************/

typedef INT64 cycles_t;

/* return the current number of cycles, or some other high-resolution timer */
cycles_t osd_cycles(void);

/* return the number of cycles per second */
cycles_t osd_cycles_per_second(void);

/* return the current number of cycles, or some other high-resolution timer.
   This call must be the fastest possible because it is called by the profiler;
   it isn't necessary to know the number of ticks per seconds. */
cycles_t osd_profiling_ticks(void);



/******************************************************************************

    Miscellaneous

******************************************************************************/

/* called to allocate/free memory that can contain executable code */
void *osd_alloc_executable(size_t size);
void osd_free_executable(void *ptr);

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,rom_load_data *romdata);

/* checks to see if a pointer is bad */
int osd_is_bad_read_ptr(const void *ptr, size_t size);

/* AdvanceMAME: Specific OSD interface */

/* return the analog value of the specified input. */
INT32 osd_get_analog_value(unsigned type, unsigned player, int* analog_type);

/* called then the game is reset */
void osd_reset(void);

/* execute the specified menu (0,1,...) */
int osd_menu(unsigned menu, int sel);

/* save the configuration */
int osd_save_config(void);
int osd_has_save_config(void);

/* filter the main exit request */
int osd_input_exit_filter(int result);

/* filter the input port state */
int osd_input_port_filter(int result, unsigned type, unsigned player, int seqtype);

/* snapshot saving */
void osd_save_snapshot(void);

/* start and stop the video/sound recording. */
void osd_record_start(void);
void osd_record_stop(void);

void osd_ui_menu(const ui_menu_item *items, int numitems, int selected);
void osd_ui_message(const char* text, int second);
void osd_ui_osd(const char *text, int percentage, int default_percentage);
void osd_ui_scroll(const char* text, int* pos);

/* customize the inputport */
void osd_config_load_default(input_port_default_entry* backup, input_port_default_entry* list);
void osd_config_load(input_port_entry* backup, input_port_entry* list);
void osd_config_save_default(input_port_default_entry* backup, input_port_default_entry* list);
void osd_config_save(input_port_entry* backup, input_port_entry* list);

/* handle the specific user interface */
int osd_handle_user_interface(mame_bitmap *bitmap, int is_menu_active);

/* osd logging */
void osd_log_va(const char* text, va_list arg);

#ifdef MESS
/* this is here to follow the current mame file hierarchy style */
#include "osd_mess.h"
#endif

#endif	/* __OSDEPEND_H__ */
