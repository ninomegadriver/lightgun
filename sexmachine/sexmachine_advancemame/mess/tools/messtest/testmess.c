/*********************************************************************

	testmess.c

	MESS testing code

*********************************************************************/

#include <time.h>
#include <ctype.h>
#include <setjmp.h>

#include "testmess.h"
#include "inputx.h"
#include "pile.h"
#include "pool.h"
#include "sound/wavwrite.h"
#include "vidhrdw/generic.h"

#ifdef WIN32
#include "windows/parallel.h"
#endif

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcpu.h"
#endif

typedef enum
{
	STATE_READY,
	STATE_INCOMMAND,
	STATE_ABORTED,
	STATE_DONE
} messtest_running_state_t;

typedef enum
{
	MESSTEST_COMMAND_END,
	MESSTEST_COMMAND_WAIT,
	MESSTEST_COMMAND_INPUT,
	MESSTEST_COMMAND_RAWINPUT,
	MESSTEST_COMMAND_SWITCH,
	MESSTEST_COMMAND_SCREENSHOT,
	MESSTEST_COMMAND_CHECKBLANK,
	MESSTEST_COMMAND_IMAGE_CREATE,
	MESSTEST_COMMAND_IMAGE_LOAD,
	MESSTEST_COMMAND_IMAGE_PRECREATE,
	MESSTEST_COMMAND_IMAGE_PRELOAD,
	MESSTEST_COMMAND_VERIFY_MEMORY,
	MESSTEST_COMMAND_VERIFY_IMAGE,
	MESSTEST_COMMAND_TRACE
} messtest_command_type_t;

struct messtest_command
{
	messtest_command_type_t command_type;
	union
	{
		double wait_time;
		struct
		{
			const char *input_chars;
			mame_time rate;
		} input_args;
		struct
		{
			int mem_region;
			offs_t start;
			offs_t end;
			const void *verify_data;
			size_t verify_data_size;
			iodevice_t device_type;
			int device_slot;
		} verify_args;
		struct
		{
			const char *filename;
			const char *format;
			iodevice_t device_type;
			int device_slot;
			const char *device_tag;
		} image_args;
		struct
		{
			const char *name;
			const char *value;
		} switch_args;
	} u;
};

struct messtest_testcase
{
	const char *name;
	const char *driver;
	double time_limit;	/* 0.0 = default */
	struct messtest_command *commands;

	/* options */
	UINT32 ram;
	unsigned int wavwrite : 1;
};

struct messtest_specific_state
{
	struct messtest_testcase testcase;
	int command_count;
	struct messtest_command current_command;
};

typedef enum
{
	MESSTEST_RESULT_SUCCESS,
	MESSTEST_RESULT_STARTFAILURE,
	MESSTEST_RESULT_RUNTIMEFAILURE
} messtest_result_t;

struct messtest_results
{
	messtest_result_t rc;
	UINT64 runtime_hash;	/* A value that is a hash taken from certain runtime parameters; used to detect different execution paths */
};



#define MESSTEST_ALWAYS_DUMP_SCREENSHOT		1


static messtest_running_state_t state;
static int had_failure;
static double wait_target;
static double final_time;
static const struct messtest_command *current_command;
static int test_flags;
static int screenshot_num;
static int format_index;
static UINT64 runtime_hash;
static void *wavptr;
static UINT32 samples_this_frame;
static int seen_first_update;

/* command list */
static mess_pile command_pile;
static memory_pool command_pool;
static int command_count;
static struct messtest_command new_command;

static struct messtest_testcase current_testcase;



static void dump_screenshot(int write_file)
{
	mame_file *fp;
	char buf[128];
	mame_bitmap *bitmap;
	int x, y, is_blank;
	pen_t color;
	extern mame_bitmap *scrbitmap[8];

	bitmap = artwork_get_ui_bitmap();

	if (write_file)
	{
		/* dump a screenshot */
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			(screenshot_num >= 0) ? "_%s_%d.png" : "_%s.png",
			current_testcase.name, screenshot_num);
		fp = mame_fopen(Machine->gamedrv->name, buf, FILETYPE_SCREENSHOT, 1);
		if (fp)
		{
			save_screen_snapshot_as(fp, bitmap);
			mame_fclose(fp);
			report_message(MSG_INFO, "Saved screenshot as %s", buf);
		}

		if (screenshot_num >= 0)
			screenshot_num++;
	}

	/* check to see if bitmap is blank */
	bitmap = scrbitmap[0];
	is_blank = 1;
	color = bitmap->read(bitmap, 0, 0);
	for (y = 0; is_blank && (y < bitmap->height); y++)
	{
		for (x = 0; is_blank && (x < bitmap->width); x++)
		{
			if (bitmap->read(bitmap, x, y) != color)
				is_blank = 0;
		}
	}
	if (is_blank)
	{
		had_failure = TRUE;
		report_message(MSG_FAILURE, "Screenshot is blank");
	}
}



static messtest_result_t run_test(int flags, struct messtest_results *results)
{
	int driver_num;
	messtest_result_t rc;
	clock_t begin_time;
	double real_run_time;

	/* lookup driver */
	for (driver_num = 0; drivers[driver_num]; driver_num++)
	{
		if (!mame_stricmp(current_testcase.driver, drivers[driver_num]->name))
			break;
	}

	/* cannot find driver? */
	if (!drivers[driver_num])
	{
		report_message(MSG_FAILURE, "Cannot find driver '%s'", current_testcase.driver);
		return MESSTEST_RESULT_STARTFAILURE;
	}

	/* prepare testing state */
	current_command = current_testcase.commands;
	state = STATE_READY;
	test_flags = flags;
	screenshot_num = 0;
	runtime_hash = 0;
	had_failure = FALSE;
	seen_first_update = FALSE;
	videoram = NULL;
	videoram_size = 0;
	dirtybuffer = NULL;

	/* set up options */
	memset(&options, 0, sizeof(options));
	options.skip_disclaimer = 1;
	options.skip_gameinfo = 1;
	options.skip_warnings = 1;
	options.disable_normal_ui = 1;
	options.ram = current_testcase.ram;
	options.vector_intensity = 1.5;
	options.use_artwork = 1;
	options.samplerate = 44100;
	options.mame_debug = 1;

	/* preload any needed images */
	while(current_command->command_type == MESSTEST_COMMAND_IMAGE_PRELOAD)
	{
		options.image_files[options.image_count].name = current_command->u.image_args.filename;
		options.image_files[options.image_count].device_type = current_command->u.image_args.device_type;
		options.image_files[options.image_count].device_index = -1;
		options.image_count++;
		current_command++;
	}

	/* perform the test */
	report_message(MSG_INFO, "Beginning test (driver '%s')", current_testcase.driver);
	begin_time = clock();
	run_game(driver_num);
	real_run_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

	/* what happened? */
	switch(state)
	{
		case STATE_ABORTED:
			report_message(MSG_FAILURE, "Test aborted");
			rc = MESSTEST_RESULT_RUNTIMEFAILURE;
			break;

		case STATE_DONE:
			if (had_failure)
			{
				report_message(MSG_FAILURE, "Test failed (real time %.2f; emu time %.2f [%i%%])",
					real_run_time, final_time, (int) ((final_time / real_run_time) * 100));
				rc = MESSTEST_RESULT_RUNTIMEFAILURE;
			}
			else
			{
				report_message(MSG_INFO, "Test succeeded (real time %.2f; emu time %.2f [%i%%])",
					real_run_time, final_time, (int) ((final_time / real_run_time) * 100));
				rc = MESSTEST_RESULT_SUCCESS;
			}
			break;

		default:
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Abnormal termination");
			rc = MESSTEST_RESULT_STARTFAILURE;
			break;
	}

	if (results)
	{
		results->rc = rc;
		results->runtime_hash = runtime_hash;
	}
	return rc;
}



int osd_start_audio_stream(int stereo)
{
	char buf[256];

	if (current_testcase.wavwrite)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "snap/_%s.wav", current_testcase.name);
		wavptr = wav_open(buf, Machine->sample_rate, 2);
	}
	else
	{
		wavptr = NULL;
	}
	samples_this_frame = (int) ((double)Machine->sample_rate / (double)Machine->refresh_rate);
	return samples_this_frame;
}



void osd_stop_audio_stream(void)
{
	if (wavptr)
	{
		wav_close(wavptr);
		wavptr = NULL;
	}
}



int osd_update_audio_stream(INT16 *buffer)
{
	if (wavptr && (Machine->sample_rate != 0))
		wav_add_data_16(wavptr, buffer, samples_this_frame * 2);
	return samples_this_frame;
}



static void find_switch(const char *switch_name, const char *switch_setting,
	int switch_type, int switch_setting_type,
	input_port_entry **in_switch, input_port_entry **in_switch_setting)
{
	input_port_entry *in;

	*in_switch = NULL;
	*in_switch_setting = NULL;

	/* find switch with the name */
	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		if (in->type == switch_type && input_port_active(in)
			&& input_port_name(in) && !mame_stricmp(input_port_name(in), switch_name))
			break;
		in++;
	}
	if (in->type == IPT_END)
		return;
	*in_switch = in;

	/* find the setting */
	in++;
	while(in->type == switch_setting_type)
	{
		if (input_port_active(in) && input_port_name(in) && !mame_stricmp(input_port_name(in), switch_setting))
			break;
		in++;
	}
	if (in->type != switch_setting_type)
		return;
	*in_switch_setting = in;
}


/* ----------------------------------------------------------------------- */

static void command_wait(void)
{
	double current_time = timer_get_time();

	if (state == STATE_READY)
	{
		/* beginning a wait command */
		wait_target = current_time + current_command->u.wait_time;
		state = STATE_INCOMMAND;
	}
	else
	{
		/* during a wait command */
		state = (current_time >= wait_target) ? STATE_READY : STATE_INCOMMAND;
	}
}



static void command_input(void)
{
	/* post a set of characters to the emulation */
	if (state == STATE_READY)
	{
		if (!inputx_can_post())
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Natural keyboard input not supported for this driver");
			return;
		}

		/* input_chars can be NULL, so we should check for that */
		if (current_command->u.input_args.input_chars)
		{
			inputx_post_utf8_rate(current_command->u.input_args.input_chars,
				current_command->u.input_args.rate);
		}
	}
	state = inputx_is_posting() ? STATE_INCOMMAND : STATE_READY;
}



static void command_rawinput(void)
{
	int parts;
	double current_time = timer_get_time();
	static const char *position;
#if 0
	int i;
	double rate = TIME_IN_SEC(1);
	const char *s;
	char buf[256];
#endif

	if (state == STATE_READY)
	{
		/* beginning of a raw input command */
		parts = 1;
		position = current_command->u.input_args.input_chars;
		wait_target = current_time;
		state = STATE_INCOMMAND;
	}
	else if (current_time > wait_target)
	{
#if 0
		do
		{
			/* process the next command */
			while(!isspace(*position))
				position++;

			/* look up the input to trigger */
			for (i = 0; input_keywords[i].name; i++)
			{
				if (!strncmp(position, input_keywords[i].name, strlen(input_keywords[i].name)))
					break;
			}

			/* go to next command */
			position = strchr(position, ',');
			if (position)
				position++;
		}
		while(position && !input_keywords[i].name);

		current_fake_input = input_keywords[i].name ? &input_keywords[i] : NULL;
		if (position)
			wait_target = current_time + rate;
		else
			state = STATE_READY;
#endif
	}
}



static void command_screenshot(void)
{
	dump_screenshot(TRUE);
}



static void command_checkblank(void)
{
	dump_screenshot(FALSE);
}



static void command_switch(void)
{
	input_port_entry *switch_name;
	input_port_entry *switch_setting;

	find_switch(current_command->u.switch_args.name, current_command->u.switch_args.value,
		IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING, &switch_name, &switch_setting);

	if (!switch_name || !switch_setting)
	{
		find_switch(current_command->u.switch_args.name, current_command->u.switch_args.value,
			IPT_CONFIG_NAME, IPT_CONFIG_SETTING, &switch_name, &switch_setting);
	}

	if (!switch_name)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Cannot find switch named '%s'", current_command->u.switch_args.name);
		return;
	}

	if (!switch_setting)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Cannot find setting '%s' on switch '%s'",
			current_command->u.switch_args.value, current_command->u.switch_args.name);
		return;
	}

	switch_name->default_value = switch_setting->default_value;
}


static void command_image_preload(void)
{
	state = STATE_ABORTED;
	report_message(MSG_FAILURE, "Image preloads must be at the beginning");
}


static void command_image_loadcreate(void)
{
	mess_image *image;
	int device_type;
	int device_slot;
	const char *device_tag;
	int i, format_index = 0;
	const char *filename;
	const char *format;
	char buf[128];
	const struct IODevice *dev;
	const char *file_extensions;

	device_slot = current_command->u.image_args.device_slot;
	device_type = current_command->u.image_args.device_type;
	device_tag = current_command->u.image_args.device_tag;

	/* look up the image slot */
	if (device_tag)
		image = image_from_devtag_and_index(device_tag, device_slot);
	else
		image = image_from_devtype_and_index(device_type, device_slot);
	if (!image)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Image slot '%s %i' does not exist",
			device_typename(device_type), device_slot);
		return;
	}
	dev = image_device(image);
	file_extensions = dev->file_extensions;

	/* is an image format specified? */
	format = current_command->u.image_args.format;
	if (format)
	{
		if (current_command->command_type != MESSTEST_COMMAND_IMAGE_CREATE)
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Cannot specify format unless creating");
			return;
		}

		if (!dev->createimage_options)
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Cannot specify format for device");
			return;
		}

		for (i = 0; dev->createimage_options[i].name; i++)
		{
			if (!strcmp(format, dev->createimage_options[i].name))
				break;
		}
		if (!dev->createimage_options[i].name)
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Unknown device '%s'", format);
			return;
		}
		format_index = i;
		file_extensions = dev->createimage_options[i].extensions;
	}

	/* figure out the filename */
	filename = current_command->u.image_args.filename;
	if (!filename)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),	"%s.%s",
			current_testcase.name, file_extensions);
		make_filename_temporary(buf, sizeof(buf) / sizeof(buf[0]));
		filename = buf;
	}

	/* actually create or load the image */
	switch(current_command->command_type)
	{
		case MESSTEST_COMMAND_IMAGE_CREATE:
			if (image_create(image, filename, format_index, NULL))
			{
				state = STATE_ABORTED;
				report_message(MSG_FAILURE, "Failed to create image '%s': %s", filename, image_error(image));
				return;
			}
			break;
		
		case MESSTEST_COMMAND_IMAGE_LOAD:
			if (image_load(image, filename))
			{
				state = STATE_ABORTED;
				report_message(MSG_FAILURE, "Failed to load image '%s': %s", filename, image_error(image));
				return;
			}
			break;

		default:
			break;
	}
}



static void command_verify_memory(void)
{
	int i = 0;
	offs_t offset, offset_start, offset_end;
	const UINT8 *verify_data;
	size_t verify_data_size;
	const UINT8 *target_data;
	size_t target_data_size;
	int region;

	offset_start = current_command->u.verify_args.start;
	offset_end = current_command->u.verify_args.end;
	verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
	verify_data_size = current_command->u.verify_args.verify_data_size;

	if (offset_end == 0)
		offset_end = offset_start + verify_data_size - 1;

	region = current_command->u.verify_args.mem_region;
	if (region)
	{
		target_data = memory_region(region);
		target_data_size = memory_region_length(region);
	}
	else
	{
		target_data = mess_ram;
		target_data_size = mess_ram_size;
	}

	/* sanity check the ranges */
	if (!verify_data || (verify_data_size <= 0))
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Invalid memory region during verify");
		return;
	}
	if (offset_start > offset_end)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Invalid verify offset range (0x%x-0x%x)", offset_start, offset_end);
		return;
	}
	if (offset_end >= target_data_size)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Verify memory range out of bounds");
		return;
	}

	for (offset = offset_start; offset <= offset_end; offset++)
	{
		if (verify_data[i] != target_data[offset])
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Failed verification step (REGION_%s; 0x%x-0x%x)",
				memory_region_to_string(region), offset_start, offset_end);
			break;
		}
		i = (i + 1) % verify_data_size;
	}
}



static void command_verify_image(void)
{
	const UINT8 *verify_data;
	size_t verify_data_size;
	size_t offset, offset_start, offset_end;
	const char *filename;
	mess_image *image;
	FILE *f;
	UINT8 c;
	char filename_buf[512];

	verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
	verify_data_size = current_command->u.verify_args.verify_data_size;

	image = image_from_devtype_and_index(current_command->u.verify_args.device_type, current_command->u.verify_args.device_slot);
	filename = image_filename(image);
	if (!filename)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Failed verification: Device Not Loaded");
		return;
	}

	/* very dirty hack - we unload the image because we cannot access it
	 * because the file is locked */
	strcpy(filename_buf, filename);
	image_unload(image);
	filename = filename_buf;

	f = fopen(filename, "r");
	if (!f)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Failed verification: Cannot open image to verify");
		return;
	}

	offset_start = 0;
	offset_end = verify_data_size - 1;

	for (offset = offset_start; offset <= offset_end; offset++)
	{
		fseek(f, offset, SEEK_SET);
		c = (UINT8) fgetc(f);

		if (c != verify_data[offset])
		{
			state = STATE_ABORTED;
			report_message(MSG_FAILURE, "Failed verification step (%s; 0x%x-0x%x)",
				filename, offset_start, offset_end);
			break;
		}
	}
	fclose(f);
}



static void command_trace(void)
{
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	int cpunum;
	FILE *file;
	char filename[256];

	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		if (cpu_gettotalcpu() == 1)
			snprintf(filename, sizeof(filename) / sizeof(filename[0]), "_%s.tr", current_testcase.name);
		else
			snprintf(filename, sizeof(filename) / sizeof(filename[0]), "_%s.%d.tr", current_testcase.name, cpunum);

		file = fopen(filename, "w");
		if (file)
		{
			report_message(MSG_INFO, "Tracing CPU #%d: %s", cpunum, filename);
			debug_cpu_trace(cpunum, file, FALSE, NULL);
		}
	}
#else
	state = STATE_ABORTED;
	report_message(MSG_FAILURE, "Cannot trace; debugger not present");
#endif
}



static void command_end(void)
{
	/* at the end of our test */
	state = STATE_DONE;
	final_time = timer_get_time();
	mame_schedule_exit();
}



/* ----------------------------------------------------------------------- */

struct command_procmap_entry
{
	messtest_command_type_t command_type;
	void (*proc)(void);
};

static const struct command_procmap_entry commands[] =
{
	{ MESSTEST_COMMAND_WAIT,			command_wait },
	{ MESSTEST_COMMAND_INPUT,			command_input },
	{ MESSTEST_COMMAND_RAWINPUT,		command_rawinput },
	{ MESSTEST_COMMAND_SCREENSHOT,		command_screenshot },
	{ MESSTEST_COMMAND_CHECKBLANK,		command_checkblank },
	{ MESSTEST_COMMAND_SWITCH,			command_switch },
	{ MESSTEST_COMMAND_IMAGE_PRELOAD,	command_image_preload },
	{ MESSTEST_COMMAND_IMAGE_LOAD,		command_image_loadcreate },
	{ MESSTEST_COMMAND_IMAGE_CREATE,	command_image_loadcreate },
	{ MESSTEST_COMMAND_VERIFY_MEMORY,	command_verify_memory },
	{ MESSTEST_COMMAND_VERIFY_IMAGE,	command_verify_image },
	{ MESSTEST_COMMAND_TRACE,			command_trace },
	{ MESSTEST_COMMAND_END,				command_end }
};

void osd_update_video_and_audio(mame_display *display)
{
	int i;
	double time_limit;
	double current_time;
	int cpunum;

	/* if the visible area has changed, update it */
	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
	{
		ui_set_visible_area(display->game_visible_area.min_x, display->game_visible_area.min_y,
			display->game_visible_area.max_x, display->game_visible_area.max_y);
	}

	/* is this the first update?  if so, eat it */
	if (!seen_first_update)
	{
		seen_first_update = TRUE;
		return;
	}

	/* if we have already aborted or completed, our work is done */
	if ((state == STATE_ABORTED) || (state == STATE_DONE))
		return;

	/* have we hit the time limit? */
	current_time = timer_get_time();
	time_limit = (current_testcase.time_limit != 0.0) ? current_testcase.time_limit
		: TIME_IN_SEC(600);
	if (current_time > time_limit)
	{
		state = STATE_ABORTED;
		report_message(MSG_FAILURE, "Time limit of %.2f seconds exceeded", time_limit);
		return;
	}

	/* update the runtime hash */
	if (0)
	{
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			runtime_hash *= 57;
			runtime_hash ^= cpunum_get_reg(cpunum, REG_PC);	/* TODO - Add more registers? */
		}
	}

	for (i = 0; i < sizeof(commands) / sizeof(commands[i]); i++)
	{
		if (current_command->command_type == commands[i].command_type)
		{
			commands[i].proc();
			break;
		}
	}

	/* if we are ready for the next command, advance to it */
	if (state == STATE_READY)
	{
		/* if we are at the end, and we are dumping screenshots, and we didn't
		 * just dump a screenshot, dump one now
		 */
		if ((test_flags & MESSTEST_ALWAYS_DUMP_SCREENSHOT) &&
			(current_command[0].command_type != MESSTEST_COMMAND_SCREENSHOT) &&
			(current_command[1].command_type == MESSTEST_COMMAND_END))
		{
			dump_screenshot(TRUE);
		}

		current_command++;
	}
}



#if 0
/* still need to work out some kinks here */
const struct KeyboardInfo *osd_get_key_list(void)
{
	int i;

	if (!ki)
	{
		ki = auto_malloc((__code_key_last - __code_key_first + 1) * sizeof(struct KeyboardInfo));

		for (i = __code_key_first; i <= __code_key_last; i++)
		{
			ki[i - __code_key_first].name = "Dummy";
			ki[i - __code_key_first].code = i;
			ki[i - __code_key_first].standardcode = i;
		}
	}
	return ki;
}

const struct JoystickInfo *osd_get_joy_list(void)
{
	int i;

	if (!ji)
	{
		ji = auto_malloc((__code_joy_last - __code_joy_first + 1) * sizeof(struct JoystickInfo));

		for (i = __code_joy_first; i <= __code_joy_last; i++)
		{
			ji[i - __code_joy_first].name = "Dummy";
			ji[i - __code_joy_first].code = i;
			ji[i - __code_joy_first].standardcode = i;
		}
	}
	return ji;
}

static int is_input_pressed(int inputcode)
{
	return 0;
}

int osd_is_joy_pressed(int joycode)
{
	return is_input_pressed(joycode);
}

int osd_is_key_pressed(int keycode)
{
	return is_input_pressed(keycode);
}
#endif


char *rompath_extra;



static int append_command(void)
{
	if (pile_write(&command_pile, &new_command, sizeof(new_command)))
		return FALSE;
	current_testcase.commands = (struct messtest_command *) pile_getptr(&command_pile);
	command_count++;
	return TRUE;
}



static void command_end_handler(const void *buffer, size_t size)
{
	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_wait(xml_data_node *node)
{
	xml_attribute_node *attr_node;

	attr_node = xml_get_attribute(node, "time");
	if (!attr_node)
	{
		error_missingattribute("time");
		return;
	}

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_WAIT;
	new_command.u.wait_time = mame_time_to_double(parse_time(attr_node->value));

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_input(xml_data_node *node)
{
	/* <input> - inputs natural keyboard data into a system */
	xml_attribute_node *attr_node;
	mame_time rate;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_INPUT;
	attr_node = xml_get_attribute(node, "rate");
	rate = attr_node ? parse_time(attr_node->value) : make_mame_time(0, 0);
	new_command.u.input_args.rate = rate;
	new_command.u.input_args.input_chars = node->value;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_rawinput(xml_data_node *node)
{
	/* <rawinput> - inputs raw data into a system */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_RAWINPUT;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_switch(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	const char *s1;
	const char *s2;

	/* <switch> - switches a DIP switch/config setting */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_SWITCH;

	/* 'name' attribute */
	attr_node = xml_get_attribute(node, "name");
	if (!attr_node)
	{
		error_missingattribute("name");
		return;
	}
	s1 = attr_node->value;

	/* 'value' attribute */
	attr_node = xml_get_attribute(node, "value");
	if (!attr_node)
	{
		error_missingattribute("value");
		return;
	}
	s2 = attr_node->value;

	new_command.u.switch_args.name = s1;
	new_command.u.switch_args.value = s2;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_screenshot(xml_data_node *node)
{
	/* <screenshot> - dumps a screenshot */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_SCREENSHOT;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_checkblank(xml_data_node *node)
{
	/* <checkblank> - checks to see if the screen is blank */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_CHECKBLANK;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_image(xml_data_node *node, messtest_command_type_t command)
{
	xml_attribute_node *attr_node;
	const char *s2;
	int preload, device_type;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = command;

	/* 'preload' attribute */
	attr_node = xml_get_attribute(node, "preload");
	preload = attr_node ? atoi(attr_node->value) : 0;
	if (preload)
		new_command.command_type += 2;

	/* 'filename' attribute */
	attr_node = xml_get_attribute(node, "filename");
	new_command.u.image_args.filename = attr_node ? attr_node->value : NULL;

	/* 'type' attribute */
	attr_node = xml_get_attribute(node, "type");
	if (!attr_node)
	{
		error_missingattribute("type");
		return;
	}
	s2 = attr_node->value;

	device_type = device_typeid(s2);
	if (device_type < 0)
	{
		error_baddevicetype(s2);
		return;
	}
	new_command.u.image_args.device_type = device_type;
	
	/* 'slot' attribute */
	attr_node = xml_get_attribute(node, "slot");
	new_command.u.image_args.device_slot = attr_node ? atoi(attr_node->value) : 0;

	/* 'format' attribute */
	format_index = 0;
	attr_node = xml_get_attribute(node, "format");
	new_command.u.image_args.format = attr_node ? attr_node->value : NULL;

	/* 'tag' attribute */
	attr_node = xml_get_attribute(node, "tag");
	new_command.u.image_args.device_tag = attr_node ? attr_node->value : NULL;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_imagecreate(xml_data_node *node)
{
	/* <imagecreate> - creates an image */
	node_image(node, MESSTEST_COMMAND_IMAGE_CREATE);
}



static void node_imageload(xml_data_node *node)
{
	/* <imageload> - loads an image */
	node_image(node, MESSTEST_COMMAND_IMAGE_LOAD);
}



static void node_memverify(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	const char *s1;
	const char *s2;
	const char *s3;
	int region;
	void *new_buffer;
	mess_pile pile;

	/* <memverify> - verifies that a range of memory contains specific data */
	attr_node = xml_get_attribute(node, "start");
	s1 = attr_node ? attr_node->value : NULL;
	if (!s1)
	{
		error_missingattribute("start");
		return;
	}

	attr_node = xml_get_attribute(node, "end");
	s2 = attr_node ? attr_node->value : NULL;
	if (!s2)
		s2 = "0";

	attr_node = xml_get_attribute(node, "region");
	s3 = attr_node ? attr_node->value : NULL;

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_VERIFY_MEMORY;
	new_command.u.verify_args.start = parse_offset(s1);
	new_command.u.verify_args.end = parse_offset(s2);

	if (s3)
	{
		region = memory_region_from_string(s3);
		if (region == REGION_INVALID)
			error_invalidmemregion(s3);
		new_command.u.verify_args.mem_region = region;
	}

	pile_init(&pile);
	messtest_get_data(node, &pile);
	new_buffer = pool_malloc(&command_pool, pile_size(&pile));
	memcpy(new_buffer, pile_getptr(&pile), pile_size(&pile));
	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = pile_size(&pile);
	pile_delete(&pile);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_imageverify(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	const char *s;
	iodevice_t device_type;
	void *new_buffer;
	mess_pile pile;

	/* <imageverify> - verifies that an image contains specific data */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_VERIFY_IMAGE;

	/* 'type' attribute */
	attr_node = xml_get_attribute(node, "type");
	s = attr_node ? attr_node->value : NULL;
	if (!s)
	{
		error_missingattribute("type");
		return;
	}

	device_type = device_typeid(s);
	if (device_type < 0)
	{
		error_baddevicetype(s);
		return;
	}

	new_command.u.verify_args.device_type = device_type;

	pile_init(&pile);
	messtest_get_data(node, &pile);
	new_buffer = pool_malloc(&command_pool, pile_size(&pile));
	memcpy(new_buffer, pile_getptr(&pile), pile_size(&pile));
	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = pile_size(&pile);
	pile_delete(&pile);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void verify_end_handler(const void *buffer, size_t size)
{
	void *new_buffer;
	new_buffer = pool_malloc(&command_pool, size);
	memcpy(new_buffer, buffer, size);

	new_command.u.verify_args.verify_data = new_buffer;
	new_command.u.verify_args.verify_data_size = size;
	command_end_handler(NULL, 0);

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



static void node_trace(xml_data_node *node)
{
	/* <trace> - emit a trace file */
	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_TRACE;

	if (!append_command())
	{
		error_outofmemory();
		return;
	}
}



void node_testmess(xml_data_node *node)
{
	xml_data_node *child_node;
	xml_attribute_node *attr_node;
	int result;

	pile_init(&command_pile);
	pool_init(&command_pool);

	memset(&new_command, 0, sizeof(new_command));
	command_count = 0;

	/* 'driver' attribute */
	attr_node = xml_get_attribute(node, "driver");
	if (!attr_node)
	{
		error_missingattribute("driver");
		return;
	}
	current_testcase.driver = attr_node->value;

	/* 'name' attribute */
	attr_node = xml_get_attribute(node, "name");
	current_testcase.name = attr_node ? attr_node->value : current_testcase.driver;

	/* 'ramsize' attribute */
	attr_node = xml_get_attribute(node, "ramsize");
	current_testcase.ram = attr_node ? ram_parse_string(attr_node->value) : 0;

	/* 'wavwrite' attribute */
	attr_node = xml_get_attribute(node, "wavwrite");
	current_testcase.wavwrite = attr_node && (atoi(attr_node->value) != 0);

	/* report the beginning of the test case */
	report_testcase_begin(current_testcase.name);
	current_testcase.commands = NULL;

	for (child_node = node->child; child_node; child_node = child_node->next)
	{
		if (!strcmp(child_node->name, "wait"))
			node_wait(child_node);
		else if (!strcmp(child_node->name, "input"))
			node_input(child_node);
		else if (!strcmp(child_node->name, "rawinput"))
			node_rawinput(child_node);
		else if (!strcmp(child_node->name, "switch"))
			node_switch(child_node);
		else if (!strcmp(child_node->name, "screenshot"))
			node_screenshot(child_node);
		else if (!strcmp(child_node->name, "checkblank"))
			node_checkblank(child_node);
		else if (!strcmp(child_node->name, "imagecreate"))
			node_imagecreate(child_node);
		else if (!strcmp(child_node->name, "imageload"))
			node_imageload(child_node);
		else if (!strcmp(child_node->name, "memverify"))
			node_memverify(child_node);
		else if (!strcmp(child_node->name, "imageverify"))
			node_imageverify(child_node);
		else if (!strcmp(child_node->name, "trace"))
			node_trace(child_node);
	}

	memset(&new_command, 0, sizeof(new_command));
	new_command.command_type = MESSTEST_COMMAND_END;
	if (!append_command())
	{
		error_outofmemory();
		return;
	}

	result = run_test(0, NULL);
	report_testcase_ran(result);

	pile_delete(&command_pile);
	pool_exit(&command_pool);
}
