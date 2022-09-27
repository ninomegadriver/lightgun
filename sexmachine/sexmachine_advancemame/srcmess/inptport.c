/***************************************************************************

    inptport.c

    Input port handling.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Theory of operation

    ------------
    OSD controls
    ------------

    There are three types of controls that the OSD can provide as potential
    input devices: digital controls, absolute analog controls, and relative
    analog controls.

    Digital controls have only two states: on or off. They are generally
    mapped to buttons and digital joystick directions (like a gamepad or a
    joystick hat). The OSD layer must return either 0 (off) or 1 (on) for
    these types of controls.

    Absolute analog controls are analog in the sense that they return a
    range of values depending on how much a given control is moved, but they
    are physically bounded. This means that there is a minimum and maximum
    limit to how far the control can be moved. They are generally mapped to
    analog joystick axes, lightguns, most PC steering wheels, and pedals.
    The OSD layer must determine the minimum and maximum range of each
    analog device and scale that to a value between -65536 and +65536
    representing the position of the control. -65536 generally refers to
    the topmost or leftmost position, while +65536 refers to the bottommost
    or rightmost position. Note that pedals are a special case here,
    mapping only in one direction, with a range of 0 to -65536.

    Relative analog controls are analog as well, but are not physically
    bounded. They can be moved continually in one direction without limit.
    They are generally mapped to trackballs and mice. Because they are
    unbounded, the OSD layer can only return delta values since the last
    read. Because of this, it is difficult to scale appropriately. For
    MAME's purposes, when mapping a mouse devices to a relative analog
    control, one pixel of movement should correspond to 512 units. Other
    analog control types should be scaled to return values of a similar
    magnitude. Like absolute analog controls, negative values refer to
    upward or leftward movement, while positive values refer to downward
    or rightward movement.

    -------------
    Game controls
    -------------

    Similarly, the types of controls used by arcade games fall into the same
    three categories: digital, absolute analog, and relative analog. The
    tricky part is how to map any arbitrary type of OSD control to an
    arbitrary type of game control.

    Digital controls: used for game buttons and standard 4/8-way joysticks,
    as well as many other types of game controls. Mapping an OSD digital
    control to a game's OSD control is trivial. For OSD analog controls,
    the MAME core does not directly support mapping any OSD analog devices
    to digital controls. However, the OSD layer is free to enumerate digital
    equivalents for analog devices. For example, each analog axis in the
    Windows OSD code enumerates to two digital controls, one for the
    negative direction (up/left) and one for the position direction
    (down/right). When these "digital" inputs are queried, the OSD layer
    checks the axis position against the center, adding in a dead zone,
    and returns 0 or 1 to indicate its position.

    Absolute analog controls: used for analog joysticks, lightguns, pedals,
    and wheel controls. Mapping an OSD absolute analog control to this type
    is easy. OSD relative analog controls can be mapped here as well by
    accumulating the deltas and bounding the results. OSD digital controls
    are mapped to these types of controls in pairs, one for a decrement and
    one for an increment, but apart from that, operate the same as the OSD
    relative analog controls by accumulating deltas and applying bounds.
    The speed of the digital delta is user-configurable per analog input.
    In addition, most absolute analog control types have an autocentering
    feature that is activated when using the digital increment/decrement
    sequences, which returns the control back to the center at a user-
    controllable speed if no digital sequences are pressed.

    Relative analog controls: used for trackballs and dial controls. Again,
    mapping an OSD relative analog control to this type is straightforward.
    OSD absolute analog controls can't map directly to these, but if the OSD
    layer provides a digital equivalent for each direction, it can be done.
    OSD digital controls map just like they do for absolute analog controls,
    except that the accumulated deltas are not bounded, but rather wrap.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "config.h"
#include "xmlfile.h"
#include "profiler.h"
#include <math.h>

#ifdef MESS
#include "inputx.h"
#endif


/*************************************
 *
 *  Constants
 *
 *************************************/

#define DIGITAL_JOYSTICKS_PER_PLAYER	3

/* these constants must match the order of the joystick directions in the IPT definition */
#define JOYDIR_UP			0
#define JOYDIR_DOWN			1
#define JOYDIR_LEFT			2
#define JOYDIR_RIGHT		3

#define JOYDIR_UP_BIT		(1 << JOYDIR_UP)
#define JOYDIR_DOWN_BIT		(1 << JOYDIR_DOWN)
#define JOYDIR_LEFT_BIT		(1 << JOYDIR_LEFT)
#define JOYDIR_RIGHT_BIT	(1 << JOYDIR_RIGHT)



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _analog_port_info analog_port_info;
struct _analog_port_info
{
	analog_port_info *	next;		/* linked list */
	input_port_entry *	port;		/* pointer to the input port referenced */
	INT32				accum;		/* accumulated value (including relative adjustments) */
	INT32				previous;	/* previous adjusted value */
	INT32				minimum;	/* minimum adjusted value */
	INT32				maximum;	/* maximum adjusted value */
	double				scalepos;	/* scale factor to apply to positive adjusted values */
	double				scaleneg;	/* scale factor to apply to negative adjusted values */
	double				keyscale;	/* scale factor to apply to the key delta field */
	UINT8				shift;		/* left shift to apply to the final result */
	UINT8				bits;		/* how many bits of resolution are expected? */
	UINT8				absolute;	/* is this an absolute or relative input? */
	UINT8				pedal;		/* is this a pedal input? */
	UINT8				autocenter;	/* autocenter this input? */
	UINT8				interpolate;/* should we do linear interpolation for mid-frame reads? */
	UINT8				lastdigital;/* was the last modification caused by a digital form? */
};


typedef struct _custom_port_info custom_port_info;
struct _custom_port_info
{
	custom_port_info *	next;		/* linked list */
	input_port_entry *	port;		/* pointer to the input port referenced */
	UINT8				shift;		/* left shift to apply to the final result */
};


typedef struct _input_bit_info input_bit_info;
struct _input_bit_info
{
	input_port_entry *	port;		/* port for this input */
	UINT8				impulse;	/* counter for impulse controls */
	UINT8				last;		/* were we pressed last time? */
};


typedef struct _changed_callback_info changed_callback_info;
struct _changed_callback_info
{
	changed_callback_info *next;	/* linked list */
	UINT32				mask;		/* mask we care about */
	void				(*callback)(void *, UINT32, UINT32); /* callback */
	void *				param;		/* parameter */
};


typedef struct _input_port_info input_port_info;
struct _input_port_info
{
	const char *		tag;		/* tag for this port */
	UINT32				defvalue;	/* default value of all the bits */
	UINT32				analog;		/* value from analog inputs */
	UINT32				analogmask;	/* mask of all analog inputs */
	UINT32				digital;	/* value from digital inputs */
	UINT32				vblank;		/* value of all IPT_VBLANK bits */
	UINT32				playback;	/* current playback override */
	UINT8				has_custom;	/* do we have any custom ports? */
	input_bit_info 		bit[MAX_BITS_PER_PORT]; /* info about each bit in the port */
	analog_port_info *	analoginfo;	/* pointer to linked list of analog port info */
	custom_port_info *	custominfo;	/* pointer to linked list of custom port info */
	changed_callback_info *change_notify;/* list of people to notify if things change */
	UINT32				changed_last_value;
};


typedef struct _digital_joystick_info digital_joystick_info;
struct _digital_joystick_info
{
	input_port_entry *	port[4];	/* port for up,down,left,right respectively */
	UINT8				inuse;		/* is this joystick used? */
	UINT8				current;	/* current value */
	UINT8				current4way;/* current 4-way value */
	UINT8				previous;	/* previous value */
};


struct _input_port_init_params
{
	input_port_entry *	ports;		/* base of the port array */
	int					max_ports;	/* maximum number of ports we can support */
	int					current_port;/* current port index */
};



/*************************************
 *
 *  Macros
 *
 *************************************/

#define IS_ANALOG(in)				((in)->type >= __ipt_analog_start && (in)->type <= __ipt_analog_end)
#define IS_DIGITAL_JOYSTICK(in)		((in)->type >= __ipt_digital_joystick_start && (in)->type <= __ipt_digital_joystick_end)
#define JOYSTICK_INFO_FOR_PORT(in)	(&joystick_info[(in)->player][((in)->type - __ipt_digital_joystick_start) / 4])
#define JOYSTICK_DIR_FOR_PORT(in)	(((in)->type - __ipt_digital_joystick_start) % 4)

#define APPLY_SENSITIVITY(x,s)		(((x) >= 0) ? (((INT64)(x) * (s) + 50) / 100) : ((-(INT64)(x) * (s) + 50) / -100))
#define APPLY_INVERSE_SENSITIVITY(x,s) (((x) >= 0) ? (((INT64)(x) * 100 - 50) / (s)) : ((-(INT64)(x) * 100 - 50) / -(s)))



/*************************************
 *
 *  Local variables
 *
 *************************************/

/* current value of all the ports */
static input_port_info port_info[MAX_INPUT_PORTS];

/* additiona tracking information for special types of controls */
static digital_joystick_info joystick_info[MAX_PLAYERS][DIGITAL_JOYSTICKS_PER_PLAYER];

/* memory for UI keys */
static UINT8 ui_memory[__ipt_max];

/* XML attributes for the different types */
static const char *seqtypestrings[] = { "standard", "decrement", "increment" };

/* original input_ports without modifications */
input_port_entry *input_ports_default;



/*************************************
 *
 *  Port handler tables
 *
 *************************************/

static const read8_handler port_handler8[] =
{
	input_port_0_r,			input_port_1_r,			input_port_2_r,			input_port_3_r,
	input_port_4_r,			input_port_5_r,			input_port_6_r,			input_port_7_r,
	input_port_8_r,			input_port_9_r,			input_port_10_r,		input_port_11_r,
	input_port_12_r,		input_port_13_r,		input_port_14_r,		input_port_15_r,
	input_port_16_r,		input_port_17_r,		input_port_18_r,		input_port_19_r,
	input_port_20_r,		input_port_21_r,		input_port_22_r,		input_port_23_r,
	input_port_24_r,		input_port_25_r,		input_port_26_r,		input_port_27_r,
	input_port_28_r,		input_port_29_r
};


static const read16_handler port_handler16[] =
{
	input_port_0_word_r,	input_port_1_word_r,	input_port_2_word_r,	input_port_3_word_r,
	input_port_4_word_r,	input_port_5_word_r,	input_port_6_word_r,	input_port_7_word_r,
	input_port_8_word_r,	input_port_9_word_r,	input_port_10_word_r,	input_port_11_word_r,
	input_port_12_word_r,	input_port_13_word_r,	input_port_14_word_r,	input_port_15_word_r,
	input_port_16_word_r,	input_port_17_word_r,	input_port_18_word_r,	input_port_19_word_r,
	input_port_20_word_r,	input_port_21_word_r,	input_port_22_word_r,	input_port_23_word_r,
	input_port_24_word_r,	input_port_25_word_r,	input_port_26_word_r,	input_port_27_word_r,
	input_port_28_word_r,	input_port_29_word_r
};


static const read32_handler port_handler32[] =
{
	input_port_0_dword_r,	input_port_1_dword_r,	input_port_2_dword_r,	input_port_3_dword_r,
	input_port_4_dword_r,	input_port_5_dword_r,	input_port_6_dword_r,	input_port_7_dword_r,
	input_port_8_dword_r,	input_port_9_dword_r,	input_port_10_dword_r,	input_port_11_dword_r,
	input_port_12_dword_r,	input_port_13_dword_r,	input_port_14_dword_r,	input_port_15_dword_r,
	input_port_16_dword_r,	input_port_17_dword_r,	input_port_18_dword_r,	input_port_19_dword_r,
	input_port_20_dword_r,	input_port_21_dword_r,	input_port_22_dword_r,	input_port_23_dword_r,
	input_port_24_dword_r,	input_port_25_dword_r,	input_port_26_dword_r,	input_port_27_dword_r,
	input_port_28_dword_r,	input_port_29_dword_r
};



/*************************************
 *
 *  Common shared strings
 *
 *************************************/

const char *input_port_default_strings[] =
{
	"Off",
	"On",
	"No",
	"Yes",
	"Lives",
	"Bonus Life",
	"Difficulty",
	"Demo Sounds",
	"Coinage",
	"Coin A",
	"Coin B",
	"9 Coins/1 Credit",
	"8 Coins/1 Credit",
	"7 Coins/1 Credit",
	"6 Coins/1 Credit",
	"5 Coins/1 Credit",
	"4 Coins/1 Credit",
	"3 Coins/1 Credit",
	"8 Coins/3 Credits",
	"4 Coins/2 Credits",
	"2 Coins/1 Credit",
	"5 Coins/3 Credits",
	"3 Coins/2 Credits",
	"4 Coins/3 Credits",
	"4 Coins/4 Credits",
	"3 Coins/3 Credits",
	"2 Coins/2 Credits",
	"1 Coin/1 Credit",
	"4 Coins/5 Credits",
	"3 Coins/4 Credits",
	"2 Coins/3 Credits",
	"4 Coins/7 Credits",
	"2 Coins/4 Credits",
	"1 Coin/2 Credits",
	"2 Coins/5 Credits",
	"2 Coins/6 Credits",
	"1 Coin/3 Credits",
	"2 Coins/7 Credits",
	"2 Coins/8 Credits",
	"1 Coin/4 Credits",
	"1 Coin/5 Credits",
	"1 Coin/6 Credits",
	"1 Coin/7 Credits",
	"1 Coin/8 Credits",
	"1 Coin/9 Credits",
	"Free Play",
	"Cabinet",
	"Upright",
	"Cocktail",
	"Flip Screen",
	"Service Mode",
	"Pause",
	"Test",
	"Tilt",
	"Version",
	"Region",
	"International",
	"Japan",
	"USA",
	"Europe",
	"Asia",
	"World",
	"Hispanic",
	"Language",
	"English",
	"Japanese",
	"German",
	"French",
	"Italian",
	"Spanish",
	"Very Easy",
	"Easiest",
	"Easier",
	"Easy",
	"Normal",
	"Medium",
	"Hard",
	"Harder",
	"Hardest",
	"Very Hard",
	"Very Low",
	"Low",
	"High",
	"Higher",
	"Highest",
	"Very High",
	"Players",
	"Controls",
	"Dual",
	"Single",
	"Game Time",
	"Continue Price",
	"Controller",
	"Light Gun",
	"Joystick",
	"Trackball",
	"Continues",
	"Allow Continue",
	"Level Select",
	"Infinite",
	"Stereo",
	"Mono",
	"Unused",
	"Unknown",
	"Standard",
	"Reverse",
	"Alternate",
	"None"
};



/*************************************
 *
 *  Default input ports
 *
 *************************************/

#define INPUT_PORT_DIGITAL_DEF(player_,group_,type_,name_,seq_) \
	{ IPT_##type_, group_, (player_ == 0) ? player_ : (player_) - 1, (player_ == 0) ? #type_ : ("P" #player_ "_" #type_), name_, seq_, SEQ_DEF_0, SEQ_DEF_0 },

#define INPUT_PORT_ANALOG_DEF(player_,group_,type_,name_,seq_,decseq_,incseq_) \
	{ IPT_##type_, group_, (player_ == 0) ? player_ : (player_) - 1, (player_ == 0) ? #type_ : ("P" #player_ "_" #type_), name_, seq_, incseq_, decseq_ },

static const input_port_default_entry default_ports_builtin[] =
{
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICK_UP,		"P1 Up",				SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICK_DOWN,      "P1 Down",				SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICK_LEFT,      "P1 Left",    			SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICK_RIGHT,     "P1 Right",   			SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKRIGHT_UP,   "P1 Right/Up",			SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_1_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKRIGHT_DOWN, "P1 Right/Down",		SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_1_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKRIGHT_LEFT, "P1 Right/Left",		SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_1_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKRIGHT_RIGHT,"P1 Right/Right",		SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_1_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKLEFT_UP,    "P1 Left/Up", 			SEQ_DEF_3(KEYCODE_E, CODE_OR, JOYCODE_1_UP) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKLEFT_DOWN,  "P1 Left/Down",			SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKLEFT_LEFT,  "P1 Left/Left", 		SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_1_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	JOYSTICKLEFT_RIGHT, "P1 Left/Right",		SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON1,			"P1 Button 1",			SEQ_DEF_5(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1, CODE_OR, MOUSECODE_1_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON2,			"P1 Button 2",			SEQ_DEF_5(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2, CODE_OR, MOUSECODE_1_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON3,			"P1 Button 3",			SEQ_DEF_5(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3, CODE_OR, MOUSECODE_1_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON4,			"P1 Button 4",			SEQ_DEF_3(KEYCODE_LSHIFT, CODE_OR, JOYCODE_1_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON5,			"P1 Button 5",			SEQ_DEF_3(KEYCODE_Z, CODE_OR, JOYCODE_1_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON6,			"P1 Button 6",			SEQ_DEF_3(KEYCODE_X, CODE_OR, JOYCODE_1_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON7,			"P1 Button 7",			SEQ_DEF_3(KEYCODE_C, CODE_OR, JOYCODE_1_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON8,			"P1 Button 8",			SEQ_DEF_3(KEYCODE_V, CODE_OR, JOYCODE_1_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON9,			"P1 Button 9",			SEQ_DEF_3(KEYCODE_B, CODE_OR, JOYCODE_1_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	BUTTON10,			"P1 Button 10",			SEQ_DEF_3(KEYCODE_N, CODE_OR, JOYCODE_1_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1, START,				"P1 Start",				SEQ_DEF_3(KEYCODE_1, CODE_OR, JOYCODE_1_START) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1, SELECT,				"P1 Select",			SEQ_DEF_3(KEYCODE_5, CODE_OR, JOYCODE_1_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_A,          "P1 Mahjong A",			SEQ_DEF_1(KEYCODE_A) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_B,          "P1 Mahjong B",			SEQ_DEF_1(KEYCODE_B) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_C,          "P1 Mahjong C",			SEQ_DEF_1(KEYCODE_C) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_D,          "P1 Mahjong D",			SEQ_DEF_1(KEYCODE_D) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_E,          "P1 Mahjong E",			SEQ_DEF_1(KEYCODE_E) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_F,          "P1 Mahjong F",			SEQ_DEF_1(KEYCODE_F) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_G,          "P1 Mahjong G",			SEQ_DEF_1(KEYCODE_G) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_H,          "P1 Mahjong H",			SEQ_DEF_1(KEYCODE_H) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_I,          "P1 Mahjong I",			SEQ_DEF_1(KEYCODE_I) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_J,          "P1 Mahjong J",			SEQ_DEF_1(KEYCODE_J) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_K,          "P1 Mahjong K",			SEQ_DEF_1(KEYCODE_K) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_L,          "P1 Mahjong L",			SEQ_DEF_1(KEYCODE_L) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_M,          "P1 Mahjong M",			SEQ_DEF_1(KEYCODE_M) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_N,          "P1 Mahjong N",			SEQ_DEF_1(KEYCODE_N) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_O,          "P1 Mahjong O",			SEQ_DEF_1(KEYCODE_O) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_P,          "P1 Mahjong P",			SEQ_DEF_1(KEYCODE_COLON) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_Q,          "P1 Mahjong Q",			SEQ_DEF_1(KEYCODE_Q) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_KAN,        "P1 Mahjong Kan",		SEQ_DEF_1(KEYCODE_LCONTROL) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_PON,        "P1 Mahjong Pon",		SEQ_DEF_1(KEYCODE_LALT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_CHI,        "P1 Mahjong Chi",		SEQ_DEF_1(KEYCODE_SPACE) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_REACH,      "P1 Mahjong Reach",		SEQ_DEF_1(KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_RON,        "P1 Mahjong Ron",		SEQ_DEF_1(KEYCODE_Z) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_BET,        "P1 Mahjong Bet",		SEQ_DEF_1(KEYCODE_2) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_LAST_CHANCE,"P1 Mahjong Last Chance",SEQ_DEF_1(KEYCODE_RALT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_SCORE,      "P1 Mahjong Score",		SEQ_DEF_1(KEYCODE_RCONTROL) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_DOUBLE_UP,  "P1 Mahjong Double Up",	SEQ_DEF_1(KEYCODE_RSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_FLIP_FLOP,  "P1 Mahjong Flip Flop",	SEQ_DEF_1(KEYCODE_Y) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_BIG,        "P1 Mahjong Big",       SEQ_DEF_1(KEYCODE_ENTER) )
	INPUT_PORT_DIGITAL_DEF( 1, IPG_PLAYER1,	MAHJONG_SMALL,      "P1 Mahjong Small",     SEQ_DEF_1(KEYCODE_BACKSPACE) )

	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICK_UP,		"P2 Up",				SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICK_DOWN,      "P2 Down",    			SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICK_LEFT,      "P2 Left",    			SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICK_RIGHT,     "P2 Right",   			SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKRIGHT_UP,   "P2 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKRIGHT_DOWN, "P2 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKRIGHT_LEFT, "P2 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKRIGHT_RIGHT,"P2 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKLEFT_UP,    "P2 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKLEFT_DOWN,  "P2 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKLEFT_LEFT,  "P2 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	JOYSTICKLEFT_RIGHT, "P2 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON1,			"P2 Button 1",			SEQ_DEF_3(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON2,			"P2 Button 2",			SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON3,			"P2 Button 3",			SEQ_DEF_3(KEYCODE_Q, CODE_OR, JOYCODE_2_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON4,			"P2 Button 4",			SEQ_DEF_3(KEYCODE_W, CODE_OR, JOYCODE_2_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON5,			"P2 Button 5",			SEQ_DEF_1(JOYCODE_2_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON6,			"P2 Button 6",			SEQ_DEF_1(JOYCODE_2_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON7,			"P2 Button 7",			SEQ_DEF_1(JOYCODE_2_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON8,			"P2 Button 8",			SEQ_DEF_1(JOYCODE_2_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON9,			"P2 Button 9",			SEQ_DEF_1(JOYCODE_2_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	BUTTON10,			"P2 Button 10",			SEQ_DEF_1(JOYCODE_2_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2, START,				"P2 Start",				SEQ_DEF_3(KEYCODE_2, CODE_OR, JOYCODE_2_START) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2, SELECT,				"P2 Select",			SEQ_DEF_3(KEYCODE_6, CODE_OR, JOYCODE_2_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_A,          "P2 Mahjong A",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_B,          "P2 Mahjong B",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_C,          "P2 Mahjong C",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_D,          "P2 Mahjong D",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_E,          "P2 Mahjong E",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_F,          "P2 Mahjong F",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_G,          "P2 Mahjong G",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_H,          "P2 Mahjong H",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_I,          "P2 Mahjong I",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_J,          "P2 Mahjong J",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_K,          "P2 Mahjong K",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_L,          "P2 Mahjong L",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_M,          "P2 Mahjong M",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_N,          "P2 Mahjong N",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_O,          "P2 Mahjong O",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_P,          "P2 Mahjong P",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_Q,          "P2 Mahjong Q",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_KAN,        "P2 Mahjong Kan",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_PON,        "P2 Mahjong Pon",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_CHI,        "P2 Mahjong Chi",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_REACH,      "P2 Mahjong Reach",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_RON,        "P2 Mahjong Ron",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_BET,        "P2 Mahjong Bet",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_LAST_CHANCE,"P2 Mahjong Last Chance",SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_SCORE,      "P2 Mahjong Score",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_DOUBLE_UP,  "P2 Mahjong Double Up",	SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_FLIP_FLOP,  "P2 Mahjong Flip Flop",	SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_BIG,        "P2 Mahjong Big",       SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 2, IPG_PLAYER2,	MAHJONG_SMALL,      "P2 Mahjong Small",     SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICK_UP,		"P3 Up",				SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICK_DOWN,      "P3 Down",    			SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICK_LEFT,      "P3 Left",    			SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICK_RIGHT,     "P3 Right",   			SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKRIGHT_UP,   "P3 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKRIGHT_DOWN, "P3 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKRIGHT_LEFT, "P3 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKRIGHT_RIGHT,"P3 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKLEFT_UP,    "P3 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKLEFT_DOWN,  "P3 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKLEFT_LEFT,  "P3 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	JOYSTICKLEFT_RIGHT, "P3 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON1,			"P3 Button 1",			SEQ_DEF_3(KEYCODE_RCONTROL, CODE_OR, JOYCODE_3_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON2,			"P3 Button 2",			SEQ_DEF_3(KEYCODE_RSHIFT, CODE_OR, JOYCODE_3_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON3,			"P3 Button 3",			SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_3_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON4,			"P3 Button 4",			SEQ_DEF_1(JOYCODE_3_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON5,			"P3 Button 5",			SEQ_DEF_1(JOYCODE_3_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON6,			"P3 Button 6",			SEQ_DEF_1(JOYCODE_3_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON7,			"P3 Button 7",			SEQ_DEF_1(JOYCODE_3_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON8,			"P3 Button 8",			SEQ_DEF_1(JOYCODE_3_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON9,			"P3 Button 9",			SEQ_DEF_1(JOYCODE_3_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3,	BUTTON10,			"P3 Button 10",			SEQ_DEF_1(JOYCODE_3_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3, START,				"P3 Start",				SEQ_DEF_3(KEYCODE_3, CODE_OR, JOYCODE_3_START) )
	INPUT_PORT_DIGITAL_DEF( 3, IPG_PLAYER3, SELECT,				"P3 Select",			SEQ_DEF_3(KEYCODE_7, CODE_OR, JOYCODE_3_SELECT) )

	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICK_UP,		"P4 Up",				SEQ_DEF_3(KEYCODE_8_PAD, CODE_OR, JOYCODE_4_UP) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICK_DOWN,      "P4 Down",    			SEQ_DEF_3(KEYCODE_2_PAD, CODE_OR, JOYCODE_4_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICK_LEFT,      "P4 Left",    			SEQ_DEF_3(KEYCODE_4_PAD, CODE_OR, JOYCODE_4_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICK_RIGHT,     "P4 Right",   			SEQ_DEF_3(KEYCODE_6_PAD, CODE_OR, JOYCODE_4_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKRIGHT_UP,   "P4 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKRIGHT_DOWN, "P4 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKRIGHT_LEFT, "P4 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKRIGHT_RIGHT,"P4 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKLEFT_UP,    "P4 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKLEFT_DOWN,  "P4 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKLEFT_LEFT,  "P4 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	JOYSTICKLEFT_RIGHT, "P4 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON1,			"P4 Button 1",			SEQ_DEF_3(KEYCODE_0_PAD, CODE_OR, JOYCODE_4_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON2,			"P4 Button 2",			SEQ_DEF_3(KEYCODE_DEL_PAD, CODE_OR, JOYCODE_4_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON3,			"P4 Button 3",			SEQ_DEF_3(KEYCODE_ENTER_PAD, CODE_OR, JOYCODE_4_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON4,			"P4 Button 4",			SEQ_DEF_1(JOYCODE_4_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON5,			"P4 Button 5",			SEQ_DEF_1(JOYCODE_4_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON6,			"P4 Button 6",			SEQ_DEF_1(JOYCODE_4_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON7,			"P4 Button 7",			SEQ_DEF_1(JOYCODE_4_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON8,			"P4 Button 8",			SEQ_DEF_1(JOYCODE_4_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON9,			"P4 Button 9",			SEQ_DEF_1(JOYCODE_4_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4,	BUTTON10,			"P4 Button 10",			SEQ_DEF_1(JOYCODE_4_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4, START,				"P4 Start",				SEQ_DEF_3(KEYCODE_4, CODE_OR, JOYCODE_4_START) )
	INPUT_PORT_DIGITAL_DEF( 4, IPG_PLAYER4, SELECT,				"P4 Select",			SEQ_DEF_3(KEYCODE_8, CODE_OR, JOYCODE_4_SELECT) )

	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICK_UP,		"P5 Up",				SEQ_DEF_1(JOYCODE_5_UP) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICK_DOWN,      "P5 Down",    			SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICK_LEFT,      "P5 Left",    			SEQ_DEF_1(JOYCODE_5_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICK_RIGHT,     "P5 Right",   			SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKRIGHT_UP,   "P5 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKRIGHT_DOWN, "P5 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKRIGHT_LEFT, "P5 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKRIGHT_RIGHT,"P5 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKLEFT_UP,    "P5 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKLEFT_DOWN,  "P5 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKLEFT_LEFT,  "P5 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	JOYSTICKLEFT_RIGHT, "P5 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON1,			"P5 Button 1",			SEQ_DEF_1(JOYCODE_5_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON2,			"P5 Button 2",			SEQ_DEF_1(JOYCODE_5_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON3,			"P5 Button 3",			SEQ_DEF_1(JOYCODE_5_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON4,			"P5 Button 4",			SEQ_DEF_1(JOYCODE_5_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON5,			"P5 Button 5",			SEQ_DEF_1(JOYCODE_5_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON6,			"P5 Button 6",			SEQ_DEF_1(JOYCODE_5_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON7,			"P5 Button 7",			SEQ_DEF_1(JOYCODE_5_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON8,			"P5 Button 8",			SEQ_DEF_1(JOYCODE_5_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON9,			"P5 Button 9",			SEQ_DEF_1(JOYCODE_5_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5,	BUTTON10,			"P5 Button 10",			SEQ_DEF_1(JOYCODE_5_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5, START,				"P5 Start",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 5, IPG_PLAYER5, SELECT,				"P5 Select",			SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICK_UP,		"P6 Up",				SEQ_DEF_1(JOYCODE_6_UP) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICK_DOWN,      "P6 Down",    			SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICK_LEFT,      "P6 Left",    			SEQ_DEF_1(JOYCODE_6_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICK_RIGHT,     "P6 Right",   			SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKRIGHT_UP,   "P6 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKRIGHT_DOWN, "P6 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKRIGHT_LEFT, "P6 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKRIGHT_RIGHT,"P6 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKLEFT_UP,    "P6 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKLEFT_DOWN,  "P6 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKLEFT_LEFT,  "P6 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	JOYSTICKLEFT_RIGHT, "P6 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON1,			"P6 Button 1",			SEQ_DEF_1(JOYCODE_6_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON2,			"P6 Button 2",			SEQ_DEF_1(JOYCODE_6_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON3,			"P6 Button 3",			SEQ_DEF_1(JOYCODE_6_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON4,			"P6 Button 4",			SEQ_DEF_1(JOYCODE_6_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON5,			"P6 Button 5",			SEQ_DEF_1(JOYCODE_6_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON6,			"P6 Button 6",			SEQ_DEF_1(JOYCODE_6_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON7,			"P6 Button 7",			SEQ_DEF_1(JOYCODE_6_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON8,			"P6 Button 8",			SEQ_DEF_1(JOYCODE_6_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON9,			"P6 Button 9",			SEQ_DEF_1(JOYCODE_6_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6,	BUTTON10,			"P6 Button 10",			SEQ_DEF_1(JOYCODE_6_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6, START,				"P6 Start",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 6, IPG_PLAYER6, SELECT,				"P6 Select",			SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICK_UP,		"P7 Up",				SEQ_DEF_1(JOYCODE_7_UP) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICK_DOWN,      "P7 Down",    			SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICK_LEFT,      "P7 Left",    			SEQ_DEF_1(JOYCODE_7_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICK_RIGHT,     "P7 Right",   			SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKRIGHT_UP,   "P7 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKRIGHT_DOWN, "P7 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKRIGHT_LEFT, "P7 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKRIGHT_RIGHT,"P7 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKLEFT_UP,    "P7 Left/Up", 			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKLEFT_DOWN,  "P7 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKLEFT_LEFT,  "P7 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	JOYSTICKLEFT_RIGHT, "P7 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON1,			"P7 Button 1",			SEQ_DEF_1(JOYCODE_7_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON2,			"P7 Button 2",			SEQ_DEF_1(JOYCODE_7_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON3,			"P7 Button 3",			SEQ_DEF_1(JOYCODE_7_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON4,			"P7 Button 4",			SEQ_DEF_1(JOYCODE_7_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON5,			"P7 Button 5",			SEQ_DEF_1(JOYCODE_7_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON6,			"P7 Button 6",			SEQ_DEF_1(JOYCODE_7_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON7,			"P7 Button 7",			SEQ_DEF_1(JOYCODE_7_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON8,			"P7 Button 8",			SEQ_DEF_1(JOYCODE_7_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON9,			"P7 Button 9",			SEQ_DEF_1(JOYCODE_7_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7,	BUTTON10,			"P7 Button 10",			SEQ_DEF_1(JOYCODE_7_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7, START,				"P7 Start",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 7, IPG_PLAYER7, SELECT,				"P7 Select",			SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICK_UP,		"P8 Up",				SEQ_DEF_1(JOYCODE_8_UP) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICK_DOWN,      "P8 Down",				SEQ_DEF_1(JOYCODE_8_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICK_LEFT,      "P8 Left",				SEQ_DEF_1(JOYCODE_8_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICK_RIGHT,     "P8 Right",				SEQ_DEF_1(JOYCODE_8_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKRIGHT_UP,   "P8 Right/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKRIGHT_DOWN, "P8 Right/Down",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKRIGHT_LEFT, "P8 Right/Left",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKRIGHT_RIGHT,"P8 Right/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKLEFT_UP,    "P8 Left/Up",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKLEFT_DOWN,  "P8 Left/Down",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKLEFT_LEFT,  "P8 Left/Left",			SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	JOYSTICKLEFT_RIGHT, "P8 Left/Right",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON1,			"P8 Button 1",			SEQ_DEF_1(JOYCODE_8_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON2,			"P8 Button 2",			SEQ_DEF_1(JOYCODE_8_BUTTON2) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON3,			"P8 Button 3",			SEQ_DEF_1(JOYCODE_8_BUTTON3) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON4,			"P8 Button 4",			SEQ_DEF_1(JOYCODE_8_BUTTON4) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON5,			"P8 Button 5",			SEQ_DEF_1(JOYCODE_8_BUTTON5) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON6,			"P8 Button 6",			SEQ_DEF_1(JOYCODE_8_BUTTON6) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON7,			"P8 Button 7",			SEQ_DEF_1(JOYCODE_8_BUTTON7) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON8,			"P8 Button 8",			SEQ_DEF_1(JOYCODE_8_BUTTON8) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON9,			"P8 Button 9",			SEQ_DEF_1(JOYCODE_8_BUTTON9) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8,	BUTTON10,			"P8 Button 10",			SEQ_DEF_1(JOYCODE_8_BUTTON10) )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8, START,				"P8 Start",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 8, IPG_PLAYER8, SELECT,				"P8 Select",			SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START1,				"1 Player Start",		SEQ_DEF_3(KEYCODE_1, CODE_OR, JOYCODE_1_START) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START2,				"2 Players Start",		SEQ_DEF_3(KEYCODE_2, CODE_OR, JOYCODE_2_START) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START3,				"3 Players Start",		SEQ_DEF_3(KEYCODE_3, CODE_OR, JOYCODE_3_START) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START4,				"4 Players Start",		SEQ_DEF_3(KEYCODE_4, CODE_OR, JOYCODE_4_START) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START5,				"5 Players Start",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START6,				"6 Players Start",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START7,				"7 Players Start",		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   START8,				"8 Players Start",		SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN1,				"Coin 1",				SEQ_DEF_3(KEYCODE_5, CODE_OR, JOYCODE_1_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN2,				"Coin 2",				SEQ_DEF_3(KEYCODE_6, CODE_OR, JOYCODE_2_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN3,				"Coin 3",				SEQ_DEF_3(KEYCODE_7, CODE_OR, JOYCODE_3_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN4,				"Coin 4",				SEQ_DEF_3(KEYCODE_8, CODE_OR, JOYCODE_4_SELECT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN5,				"Coin 5",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN6,				"Coin 6",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN7,				"Coin 7",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   COIN8,				"Coin 8",				SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   BILL1,				"Bill 1",				SEQ_DEF_1(KEYCODE_BACKSPACE) )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   SERVICE1,			"Service 1",    		SEQ_DEF_1(KEYCODE_9) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   SERVICE2,			"Service 2",    		SEQ_DEF_1(KEYCODE_0) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   SERVICE3,			"Service 3",     		SEQ_DEF_1(KEYCODE_MINUS) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   SERVICE4,			"Service 4",     		SEQ_DEF_1(KEYCODE_EQUALS) )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   SERVICE,			"Service",	     		SEQ_DEF_1(KEYCODE_F2) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   TILT, 				"Tilt",			   		SEQ_DEF_1(KEYCODE_T) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   INTERLOCK,			"Door Interlock",  		SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   VOLUME_DOWN,		"Volume Down",     		SEQ_DEF_1(KEYCODE_MINUS) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   VOLUME_UP,			"Volume Up",     		SEQ_DEF_1(KEYCODE_EQUALS) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	PEDAL,				"P1 Pedal 1",     		SEQ_DEF_3(JOYCODE_1_ANALOG_PEDAL1, CODE_OR, JOYCODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	PEDAL,				"P2 Pedal 1", 			SEQ_DEF_3(JOYCODE_2_ANALOG_PEDAL1, CODE_OR, JOYCODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	PEDAL,				"P3 Pedal 1",			SEQ_DEF_3(JOYCODE_3_ANALOG_PEDAL1, CODE_OR, JOYCODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_RCONTROL, CODE_OR, JOYCODE_3_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	PEDAL,				"P4 Pedal 1",			SEQ_DEF_3(JOYCODE_4_ANALOG_PEDAL1, CODE_OR, JOYCODE_4_ANALOG_Y), SEQ_DEF_3(KEYCODE_0_PAD, CODE_OR, JOYCODE_4_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	PEDAL,				"P5 Pedal 1",			SEQ_DEF_3(JOYCODE_5_ANALOG_PEDAL1, CODE_OR, JOYCODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	PEDAL,				"P6 Pedal 1",			SEQ_DEF_3(JOYCODE_6_ANALOG_PEDAL1, CODE_OR, JOYCODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	PEDAL,				"P7 Pedal 1",			SEQ_DEF_3(JOYCODE_7_ANALOG_PEDAL1, CODE_OR, JOYCODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_BUTTON1), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	PEDAL,				"P8 Pedal 1",			SEQ_DEF_3(JOYCODE_8_ANALOG_PEDAL1, CODE_OR, JOYCODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_BUTTON1), SEQ_DEF_0 )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	PEDAL2,				"P1 Pedal 2",			SEQ_DEF_1(JOYCODE_1_ANALOG_PEDAL2), SEQ_DEF_3(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	PEDAL2,				"P2 Pedal 2",			SEQ_DEF_1(JOYCODE_2_ANALOG_PEDAL2), SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	PEDAL2,				"P3 Pedal 2",			SEQ_DEF_1(JOYCODE_3_ANALOG_PEDAL2), SEQ_DEF_3(KEYCODE_RSHIFT, CODE_OR, JOYCODE_3_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	PEDAL2,				"P4 Pedal 2",			SEQ_DEF_1(JOYCODE_4_ANALOG_PEDAL2), SEQ_DEF_3(KEYCODE_DEL_PAD, CODE_OR, JOYCODE_4_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	PEDAL2,				"P5 Pedal 2",			SEQ_DEF_1(JOYCODE_5_ANALOG_PEDAL2), SEQ_DEF_1(JOYCODE_5_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	PEDAL2,				"P6 Pedal 2",			SEQ_DEF_1(JOYCODE_6_ANALOG_PEDAL2), SEQ_DEF_1(JOYCODE_6_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	PEDAL2,				"P7 Pedal 2",			SEQ_DEF_1(JOYCODE_7_ANALOG_PEDAL2), SEQ_DEF_1(JOYCODE_7_BUTTON2), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	PEDAL2,				"P8 Pedal 2",			SEQ_DEF_1(JOYCODE_8_ANALOG_PEDAL2), SEQ_DEF_1(JOYCODE_8_BUTTON2), SEQ_DEF_0 )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	PEDAL3,				"P1 Pedal 3",			SEQ_DEF_1(JOYCODE_1_ANALOG_PEDAL3), SEQ_DEF_3(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	PEDAL3,				"P2 Pedal 3",			SEQ_DEF_1(JOYCODE_2_ANALOG_PEDAL3), SEQ_DEF_3(KEYCODE_Q, CODE_OR, JOYCODE_2_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	PEDAL3,				"P3 Pedal 3",			SEQ_DEF_1(JOYCODE_3_ANALOG_PEDAL3), SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_3_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	PEDAL3,				"P4 Pedal 3",			SEQ_DEF_1(JOYCODE_4_ANALOG_PEDAL3), SEQ_DEF_3(KEYCODE_ENTER_PAD, CODE_OR, JOYCODE_4_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	PEDAL3,				"P5 Pedal 3",			SEQ_DEF_1(JOYCODE_5_ANALOG_PEDAL3), SEQ_DEF_1(JOYCODE_5_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	PEDAL3,				"P6 Pedal 3",			SEQ_DEF_1(JOYCODE_6_ANALOG_PEDAL3), SEQ_DEF_1(JOYCODE_6_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	PEDAL3,				"P7 Pedal 3",			SEQ_DEF_1(JOYCODE_7_ANALOG_PEDAL3), SEQ_DEF_1(JOYCODE_7_BUTTON3), SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	PEDAL3,				"P8 Pedal 3",			SEQ_DEF_1(JOYCODE_8_ANALOG_PEDAL3), SEQ_DEF_1(JOYCODE_8_BUTTON3), SEQ_DEF_0 )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	PADDLE,				"Paddle",   	    	SEQ_DEF_3(MOUSECODE_1_ANALOG_X, CODE_OR, JOYCODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	PADDLE,				"Paddle 2",      		SEQ_DEF_3(MOUSECODE_2_ANALOG_X, CODE_OR, JOYCODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	PADDLE,				"Paddle 3",      		SEQ_DEF_3(MOUSECODE_3_ANALOG_X, CODE_OR, JOYCODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	PADDLE,				"Paddle 4",      		SEQ_DEF_3(MOUSECODE_4_ANALOG_X, CODE_OR, JOYCODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	PADDLE,				"Paddle 5",      		SEQ_DEF_3(MOUSECODE_5_ANALOG_X, CODE_OR, JOYCODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	PADDLE,				"Paddle 6",      		SEQ_DEF_3(MOUSECODE_6_ANALOG_X, CODE_OR, JOYCODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	PADDLE,				"Paddle 7",      		SEQ_DEF_3(MOUSECODE_7_ANALOG_X, CODE_OR, JOYCODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	PADDLE,				"Paddle 8",      		SEQ_DEF_3(MOUSECODE_8_ANALOG_X, CODE_OR, JOYCODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	PADDLE_V,			"Paddle V",				SEQ_DEF_3(MOUSECODE_1_ANALOG_Y, CODE_OR, JOYCODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	PADDLE_V,			"Paddle V 2",			SEQ_DEF_3(MOUSECODE_2_ANALOG_Y, CODE_OR, JOYCODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	PADDLE_V,			"Paddle V 3",			SEQ_DEF_3(MOUSECODE_3_ANALOG_Y, CODE_OR, JOYCODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	PADDLE_V,			"Paddle V 4",			SEQ_DEF_3(MOUSECODE_4_ANALOG_Y, CODE_OR, JOYCODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	PADDLE_V,			"Paddle V 5",			SEQ_DEF_3(MOUSECODE_5_ANALOG_Y, CODE_OR, JOYCODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	PADDLE_V,			"Paddle V 6",			SEQ_DEF_3(MOUSECODE_6_ANALOG_Y, CODE_OR, JOYCODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	PADDLE_V,			"Paddle V 7",			SEQ_DEF_3(MOUSECODE_7_ANALOG_Y, CODE_OR, JOYCODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	PADDLE_V,			"Paddle V 8",			SEQ_DEF_3(MOUSECODE_8_ANALOG_Y, CODE_OR, JOYCODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	DIAL,				"Dial",					SEQ_DEF_3(MOUSECODE_1_ANALOG_X, CODE_OR, JOYCODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	DIAL,				"Dial 2",				SEQ_DEF_3(MOUSECODE_2_ANALOG_X, CODE_OR, JOYCODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	DIAL,				"Dial 3",				SEQ_DEF_3(MOUSECODE_3_ANALOG_X, CODE_OR, JOYCODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	DIAL,				"Dial 4",				SEQ_DEF_3(MOUSECODE_4_ANALOG_X, CODE_OR, JOYCODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	DIAL,				"Dial 5",				SEQ_DEF_3(MOUSECODE_5_ANALOG_X, CODE_OR, JOYCODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	DIAL,				"Dial 6",				SEQ_DEF_3(MOUSECODE_6_ANALOG_X, CODE_OR, JOYCODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	DIAL,				"Dial 7",				SEQ_DEF_3(MOUSECODE_7_ANALOG_X, CODE_OR, JOYCODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	DIAL,				"Dial 8",				SEQ_DEF_3(MOUSECODE_8_ANALOG_X, CODE_OR, JOYCODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	DIAL_V,				"Dial V",				SEQ_DEF_3(MOUSECODE_1_ANALOG_Y, CODE_OR, JOYCODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	DIAL_V,				"Dial V 2",				SEQ_DEF_3(MOUSECODE_2_ANALOG_Y, CODE_OR, JOYCODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	DIAL_V,				"Dial V 3",				SEQ_DEF_3(MOUSECODE_3_ANALOG_Y, CODE_OR, JOYCODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	DIAL_V,				"Dial V 4",				SEQ_DEF_3(MOUSECODE_4_ANALOG_Y, CODE_OR, JOYCODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	DIAL_V,				"Dial V 5",				SEQ_DEF_3(MOUSECODE_5_ANALOG_Y, CODE_OR, JOYCODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	DIAL_V,				"Dial V 6",				SEQ_DEF_3(MOUSECODE_6_ANALOG_Y, CODE_OR, JOYCODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	DIAL_V,				"Dial V 7",				SEQ_DEF_3(MOUSECODE_7_ANALOG_Y, CODE_OR, JOYCODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	DIAL_V,				"Dial V 8",				SEQ_DEF_3(MOUSECODE_8_ANALOG_Y, CODE_OR, JOYCODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	TRACKBALL_X,		"Track X",				SEQ_DEF_3(MOUSECODE_1_ANALOG_X, CODE_OR, JOYCODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	TRACKBALL_X,		"Track X 2",			SEQ_DEF_3(MOUSECODE_2_ANALOG_X, CODE_OR, JOYCODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	TRACKBALL_X,		"Track X 3",			SEQ_DEF_3(MOUSECODE_3_ANALOG_X, CODE_OR, JOYCODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	TRACKBALL_X,		"Track X 4",			SEQ_DEF_3(MOUSECODE_4_ANALOG_X, CODE_OR, JOYCODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	TRACKBALL_X,		"Track X 5",			SEQ_DEF_3(MOUSECODE_5_ANALOG_X, CODE_OR, JOYCODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	TRACKBALL_X,		"Track X 6",			SEQ_DEF_3(MOUSECODE_6_ANALOG_X, CODE_OR, JOYCODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	TRACKBALL_X,		"Track X 7",			SEQ_DEF_3(MOUSECODE_7_ANALOG_X, CODE_OR, JOYCODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	TRACKBALL_X,		"Track X 8",			SEQ_DEF_3(MOUSECODE_8_ANALOG_X, CODE_OR, JOYCODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	TRACKBALL_Y,		"Track Y",				SEQ_DEF_3(MOUSECODE_1_ANALOG_Y, CODE_OR, JOYCODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	TRACKBALL_Y,		"Track Y 2",			SEQ_DEF_3(MOUSECODE_2_ANALOG_Y, CODE_OR, JOYCODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	TRACKBALL_Y,		"Track Y 3",			SEQ_DEF_3(MOUSECODE_3_ANALOG_Y, CODE_OR, JOYCODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	TRACKBALL_Y,		"Track Y 4",			SEQ_DEF_3(MOUSECODE_4_ANALOG_Y, CODE_OR, JOYCODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	TRACKBALL_Y,		"Track Y 5",			SEQ_DEF_3(MOUSECODE_5_ANALOG_Y, CODE_OR, JOYCODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	TRACKBALL_Y,		"Track Y 6",			SEQ_DEF_3(MOUSECODE_6_ANALOG_Y, CODE_OR, JOYCODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	TRACKBALL_Y,		"Track Y 7",			SEQ_DEF_3(MOUSECODE_7_ANALOG_Y, CODE_OR, JOYCODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	TRACKBALL_Y,		"Track Y 8",			SEQ_DEF_3(MOUSECODE_8_ANALOG_Y, CODE_OR, JOYCODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	AD_STICK_X,			"AD Stick X",			SEQ_DEF_3(JOYCODE_1_ANALOG_X, CODE_OR, MOUSECODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	AD_STICK_X,			"AD Stick X 2",			SEQ_DEF_3(JOYCODE_2_ANALOG_X, CODE_OR, MOUSECODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	AD_STICK_X,			"AD Stick X 3",			SEQ_DEF_3(JOYCODE_3_ANALOG_X, CODE_OR, MOUSECODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	AD_STICK_X,			"AD Stick X 4",			SEQ_DEF_3(JOYCODE_4_ANALOG_X, CODE_OR, MOUSECODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	AD_STICK_X,			"AD Stick X 5",			SEQ_DEF_3(JOYCODE_5_ANALOG_X, CODE_OR, MOUSECODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	AD_STICK_X,			"AD Stick X 6",			SEQ_DEF_3(JOYCODE_6_ANALOG_X, CODE_OR, MOUSECODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	AD_STICK_X,			"AD Stick X 7",			SEQ_DEF_3(JOYCODE_7_ANALOG_X, CODE_OR, MOUSECODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	AD_STICK_X,			"AD Stick X 8",			SEQ_DEF_3(JOYCODE_8_ANALOG_X, CODE_OR, MOUSECODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	AD_STICK_Y,			"AD Stick Y",			SEQ_DEF_3(JOYCODE_1_ANALOG_Y, CODE_OR, MOUSECODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	AD_STICK_Y,			"AD Stick Y 2",			SEQ_DEF_3(JOYCODE_2_ANALOG_Y, CODE_OR, MOUSECODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	AD_STICK_Y,			"AD Stick Y 3",			SEQ_DEF_3(JOYCODE_3_ANALOG_Y, CODE_OR, MOUSECODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	AD_STICK_Y,			"AD Stick Y 4",			SEQ_DEF_3(JOYCODE_4_ANALOG_Y, CODE_OR, MOUSECODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	AD_STICK_Y,			"AD Stick Y 5",			SEQ_DEF_3(JOYCODE_5_ANALOG_Y, CODE_OR, MOUSECODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	AD_STICK_Y,			"AD Stick Y 6",			SEQ_DEF_3(JOYCODE_6_ANALOG_Y, CODE_OR, MOUSECODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	AD_STICK_Y,			"AD Stick Y 7",			SEQ_DEF_3(JOYCODE_7_ANALOG_Y, CODE_OR, MOUSECODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	AD_STICK_Y,			"AD Stick Y 8",			SEQ_DEF_3(JOYCODE_8_ANALOG_Y, CODE_OR, MOUSECODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	AD_STICK_Z,			"AD Stick Z",			SEQ_DEF_1(JOYCODE_1_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	AD_STICK_Z,			"AD Stick Z 2",			SEQ_DEF_1(JOYCODE_2_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	AD_STICK_Z,			"AD Stick Z 3",			SEQ_DEF_1(JOYCODE_3_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	AD_STICK_Z,			"AD Stick Z 4",			SEQ_DEF_1(JOYCODE_4_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	AD_STICK_Z,			"AD Stick Z 5",			SEQ_DEF_1(JOYCODE_5_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	AD_STICK_Z,			"AD Stick Z 6",			SEQ_DEF_1(JOYCODE_6_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	AD_STICK_Z,			"AD Stick Z 7",			SEQ_DEF_1(JOYCODE_7_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	AD_STICK_Z,			"AD Stick Z 8",			SEQ_DEF_1(JOYCODE_8_ANALOG_Z), SEQ_DEF_0, SEQ_DEF_0 )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	LIGHTGUN_X,			"Lightgun X",			SEQ_DEF_5(GUNCODE_1_ANALOG_X, CODE_OR, MOUSECODE_1_ANALOG_X, CODE_OR, JOYCODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	LIGHTGUN_X,			"Lightgun X 2",			SEQ_DEF_5(GUNCODE_2_ANALOG_X, CODE_OR, MOUSECODE_2_ANALOG_X, CODE_OR, JOYCODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	LIGHTGUN_X,			"Lightgun X 3",			SEQ_DEF_5(GUNCODE_3_ANALOG_X, CODE_OR, MOUSECODE_3_ANALOG_X, CODE_OR, JOYCODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	LIGHTGUN_X,			"Lightgun X 4",			SEQ_DEF_5(GUNCODE_4_ANALOG_X, CODE_OR, MOUSECODE_4_ANALOG_X, CODE_OR, JOYCODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	LIGHTGUN_X,			"Lightgun X 5",			SEQ_DEF_5(GUNCODE_5_ANALOG_X, CODE_OR, MOUSECODE_5_ANALOG_X, CODE_OR, JOYCODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	LIGHTGUN_X,			"Lightgun X 6",			SEQ_DEF_5(GUNCODE_6_ANALOG_X, CODE_OR, MOUSECODE_6_ANALOG_X, CODE_OR, JOYCODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	LIGHTGUN_X,			"Lightgun X 7",			SEQ_DEF_5(GUNCODE_7_ANALOG_X, CODE_OR, MOUSECODE_7_ANALOG_X, CODE_OR, JOYCODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	LIGHTGUN_X,			"Lightgun X 8",			SEQ_DEF_5(GUNCODE_8_ANALOG_X, CODE_OR, MOUSECODE_8_ANALOG_X, CODE_OR, JOYCODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	LIGHTGUN_Y,			"Lightgun Y",			SEQ_DEF_5(GUNCODE_1_ANALOG_Y, CODE_OR, MOUSECODE_1_ANALOG_Y, CODE_OR, JOYCODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	LIGHTGUN_Y,			"Lightgun Y 2",			SEQ_DEF_5(GUNCODE_2_ANALOG_Y, CODE_OR, MOUSECODE_2_ANALOG_Y, CODE_OR, JOYCODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	LIGHTGUN_Y,			"Lightgun Y 3",			SEQ_DEF_5(GUNCODE_3_ANALOG_Y, CODE_OR, MOUSECODE_3_ANALOG_Y, CODE_OR, JOYCODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	LIGHTGUN_Y,			"Lightgun Y 4",			SEQ_DEF_5(GUNCODE_4_ANALOG_Y, CODE_OR, MOUSECODE_4_ANALOG_Y, CODE_OR, JOYCODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	LIGHTGUN_Y,			"Lightgun Y 5",			SEQ_DEF_5(GUNCODE_5_ANALOG_Y, CODE_OR, MOUSECODE_5_ANALOG_Y, CODE_OR, JOYCODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	LIGHTGUN_Y,			"Lightgun Y 6",			SEQ_DEF_5(GUNCODE_6_ANALOG_Y, CODE_OR, MOUSECODE_6_ANALOG_Y, CODE_OR, JOYCODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	LIGHTGUN_Y,			"Lightgun Y 7",			SEQ_DEF_5(GUNCODE_7_ANALOG_Y, CODE_OR, MOUSECODE_7_ANALOG_Y, CODE_OR, JOYCODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	LIGHTGUN_Y,			"Lightgun Y 8",			SEQ_DEF_5(GUNCODE_8_ANALOG_Y, CODE_OR, MOUSECODE_8_ANALOG_Y, CODE_OR, JOYCODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	MOUSE_X,			"Mouse X",				SEQ_DEF_1(MOUSECODE_1_ANALOG_X), SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	MOUSE_X,			"Mouse X 2",			SEQ_DEF_1(MOUSECODE_2_ANALOG_X), SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	MOUSE_X,			"Mouse X 3",			SEQ_DEF_1(MOUSECODE_3_ANALOG_X), SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	MOUSE_X,			"Mouse X 4",			SEQ_DEF_1(MOUSECODE_4_ANALOG_X), SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	MOUSE_X,			"Mouse X 5",			SEQ_DEF_1(MOUSECODE_5_ANALOG_X), SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	MOUSE_X,			"Mouse X 6",			SEQ_DEF_1(MOUSECODE_6_ANALOG_X), SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	MOUSE_X,			"Mouse X 7",			SEQ_DEF_1(MOUSECODE_7_ANALOG_X), SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	MOUSE_X,			"Mouse X 8",			SEQ_DEF_1(MOUSECODE_8_ANALOG_X), SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) )

	INPUT_PORT_ANALOG_DEF ( 1, IPG_PLAYER1,	MOUSE_Y,			"Mouse Y",				SEQ_DEF_1(MOUSECODE_1_ANALOG_Y), SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 2, IPG_PLAYER2,	MOUSE_Y,			"Mouse Y 2",			SEQ_DEF_1(MOUSECODE_2_ANALOG_Y), SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 3, IPG_PLAYER3,	MOUSE_Y,			"Mouse Y 3",			SEQ_DEF_1(MOUSECODE_3_ANALOG_Y), SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 4, IPG_PLAYER4,	MOUSE_Y,			"Mouse Y 4",			SEQ_DEF_1(MOUSECODE_4_ANALOG_Y), SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 5, IPG_PLAYER5,	MOUSE_Y,			"Mouse Y 5",			SEQ_DEF_1(MOUSECODE_5_ANALOG_Y), SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 6, IPG_PLAYER6,	MOUSE_Y,			"Mouse Y 6",			SEQ_DEF_1(MOUSECODE_6_ANALOG_Y), SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 7, IPG_PLAYER7,	MOUSE_Y,			"Mouse Y 7",			SEQ_DEF_1(MOUSECODE_7_ANALOG_Y), SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) )
	INPUT_PORT_ANALOG_DEF ( 8, IPG_PLAYER8,	MOUSE_Y,			"Mouse Y 8",			SEQ_DEF_1(MOUSECODE_8_ANALOG_Y), SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_OTHER,   KEYBOARD, 			"Keyboard",		   		SEQ_DEF_0 )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_ON_SCREEN_DISPLAY,"On Screen Display",	SEQ_DEF_1(KEYCODE_TILDE) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_DEBUG_BREAK,		"Break in Debugger",	SEQ_DEF_1(KEYCODE_TILDE) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_CONFIGURE,		"Config Menu",			SEQ_DEF_1(KEYCODE_TAB) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_PAUSE,			"Pause",				SEQ_DEF_1(KEYCODE_P) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_RESET_MACHINE,	"Reset Game",			SEQ_DEF_2(KEYCODE_F3, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SOFT_RESET,		"Soft Reset",			SEQ_DEF_3(KEYCODE_F3, CODE_NOT, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SHOW_GFX,		"Show Gfx",				SEQ_DEF_1(KEYCODE_F4) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_FRAMESKIP_DEC,	"Frameskip Dec",		SEQ_DEF_1(KEYCODE_F8) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_FRAMESKIP_INC,	"Frameskip Inc",		SEQ_DEF_1(KEYCODE_F9) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_THROTTLE,		"Throttle",				SEQ_DEF_1(KEYCODE_F10) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SHOW_FPS,		"Show FPS",				SEQ_DEF_5(KEYCODE_F11, CODE_NOT, KEYCODE_LCONTROL, CODE_NOT, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SNAPSHOT,		"Save Snapshot",		SEQ_DEF_3(KEYCODE_F12, CODE_NOT, KEYCODE_LSHIFT) )
#if 0 /* AdvanceMAME has its record code */
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_RECORD_MOVIE,	"Record Movie",			SEQ_DEF_2(KEYCODE_F12, KEYCODE_LSHIFT) )
#endif
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_TOGGLE_CHEAT,	"Toggle Cheat",			SEQ_DEF_1(KEYCODE_F6) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_UP,				"UI Up",				SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_DOWN,			"UI Down",				SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_LEFT,			"UI Left",				SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_RIGHT,			"UI Right",				SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_HOME,			"UI Home",				SEQ_DEF_1(KEYCODE_HOME) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_END,				"UI End",				SEQ_DEF_1(KEYCODE_END) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SELECT,			"UI Select",			SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_1_BUTTON1) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_CANCEL,			"UI Cancel",			SEQ_DEF_1(KEYCODE_ESC) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_CLEAR,			"UI Clear",				SEQ_DEF_1(KEYCODE_DEL) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_PAN_UP,			"Pan Up",				SEQ_DEF_3(KEYCODE_PGUP, CODE_NOT, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_PAN_DOWN,		"Pan Down",				SEQ_DEF_3(KEYCODE_PGDN, CODE_NOT, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_PAN_LEFT,		"Pan Left",				SEQ_DEF_2(KEYCODE_PGUP, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_PAN_RIGHT,		"Pan Right",			SEQ_DEF_2(KEYCODE_PGDN, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SHOW_PROFILER,	"Show Profiler",		SEQ_DEF_2(KEYCODE_F11, KEYCODE_LSHIFT) )
#ifdef MESS
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_TOGGLE_UI,		"UI Toggle",			SEQ_DEF_3(KEYCODE_SCRLOCK, CODE_OR, KEYCODE_NUMLOCK) )
#endif
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_TOGGLE_DEBUG,    "Toggle Debugger",		SEQ_DEF_1(KEYCODE_F5) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SAVE_STATE,      "Save State",			SEQ_DEF_2(KEYCODE_F7, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_LOAD_STATE,      "Load State",			SEQ_DEF_3(KEYCODE_F7, CODE_NOT, KEYCODE_LSHIFT) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_ADD_CHEAT,		"Add Cheat",			SEQ_DEF_1(KEYCODE_A) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_DELETE_CHEAT,	"Delete Cheat",			SEQ_DEF_1(KEYCODE_D) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_SAVE_CHEAT,		"Save Cheat",			SEQ_DEF_1(KEYCODE_S) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_WATCH_VALUE,		"Watch Value",			SEQ_DEF_1(KEYCODE_W) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_EDIT_CHEAT,		"Edit Cheat",			SEQ_DEF_1(KEYCODE_E) )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      UI_TOGGLE_CROSSHAIR,"Toggle Crosshair",		SEQ_DEF_1(KEYCODE_F1) )

	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_1,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_2,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_3,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_4,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_5,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_6,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_7,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_8,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_9,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_10,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_11,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_12,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_13,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_14,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_15,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_UI,      OSD_16,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, UNKNOWN,			NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, SPECIAL,			NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, OTHER,				NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, DIPSWITCH_NAME,		NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, CONFIG_NAME,		NULL,					SEQ_DEF_0 )
	INPUT_PORT_DIGITAL_DEF( 0, IPG_INVALID, END,				NULL,					SEQ_DEF_0 )
};


static input_port_default_entry default_ports[ARRAY_LENGTH(default_ports_builtin)];
static input_port_default_entry default_ports_backup[ARRAY_LENGTH(default_ports_builtin)];
static const int input_port_count = ARRAY_LENGTH(default_ports_builtin);
static int default_ports_lookup[__ipt_max][MAX_PLAYERS];

/* AdvanceMAME: Custom settings load */
int input_config_load_settings()
{
	/* AdvanceMAME: Load user defined customization */
	osd_config_load_default(default_ports_backup, default_ports);

	/* AdvanceMAME: Load user defined customization */
	osd_config_load(input_ports_default, Machine->input_ports);

	/* AdvanceMAME: Report no setting loaded, always show the disclaimer */
	return 0;
}

/* AdvanceMAME: Custom settings save */
void input_config_save_settings(void)
{
	/* AdvanceMAME: Save user defined customization */
	osd_config_save_default(default_ports_backup, default_ports);

	/* AdvanceMAME: Save user defined customization */
	osd_config_save(input_ports_default, Machine->input_ports);
}


/*************************************
 *
 *  Function prototypes
 *
 *************************************/

static void input_port_load(int config_type, xml_data_node *parentnode);
static void input_port_save(int config_type, xml_data_node *parentnode);
static void update_digital_joysticks(void);
static void update_analog_port(int port);
static void interpolate_analog_port(int port);



/*************************************
 *
 *  Input port initialize
 *
 *************************************/

int input_port_init(void (*construct_ipt)(input_port_init_params *))
{
	int ipnum, player;

	/* start with the raw defaults and ask the OSD to customize them in the backup array */
	memcpy(default_ports_backup, default_ports_builtin, sizeof(default_ports_backup));
	osd_customize_inputport_list(default_ports_backup);

	/* propogate these changes forward to the final input list */
	memcpy(default_ports, default_ports_backup, sizeof(default_ports));

	/* make a lookup table mapping type/player to the default port list entry */
	for (ipnum = 0; ipnum < __ipt_max; ipnum++)
		for (player = 0; player < MAX_PLAYERS; player++)
			default_ports_lookup[ipnum][player] = -1;
	for (ipnum = 0; default_ports[ipnum].type != IPT_END; ipnum++)
		default_ports_lookup[default_ports[ipnum].type][default_ports[ipnum].player] = ipnum;

	/* reset the port info */
	memset(port_info, 0, sizeof(port_info));

	/* if we have inputs, process them now */
	if (construct_ipt)
	{
		input_port_entry *port;
		int portnum;

		/* allocate input ports */
		Machine->input_ports = input_port_allocate(construct_ipt, NULL);
		if (!Machine->input_ports)
			return 1;

		/* allocate default input ports */
		input_ports_default = input_port_allocate(construct_ipt, NULL);
		if (!input_ports_default)
			return 1;

		/* identify all the tagged ports up front so the memory system can access them */
		portnum = 0;
		for (port = Machine->input_ports; port->type != IPT_END; port++)
			if (port->type == IPT_PORT)
				port_info[portnum++].tag = port->start.tag;

		/* look up all the tags referenced in conditions */
		for (port = Machine->input_ports; port->type != IPT_END; port++)
			if (port->condition.tag)
			{
				int tag = port_tag_to_index(port->condition.tag);
				if (tag == -1)
					fatalerror("Conditional port references invalid tag '%s'", port->condition.tag);
				port->condition.portnum = tag;
			}
	}

	/* register callbacks for when we load configurations */
	config_register("input", input_port_load, input_port_save);

	return 0;
}



/*************************************
 *
 *  Input port initialization
 *
 *************************************/

static void input_port_postload(void)
{
	input_port_entry *port;
	int portnum, bitnum;
	UINT32 mask;

	/* reset the pointers */
	memset(&joystick_info, 0, sizeof(joystick_info));

	/* loop over the ports and identify all the analog inputs */
	portnum = -1;
	bitnum = 0;
	for (port = Machine->input_ports; port->type != IPT_END; port++)
	{
		/* if this is IPT_PORT, increment the port number */
		if (port->type == IPT_PORT)
		{
			portnum++;
			bitnum = 0;
		}

		/* if this is not a DIP setting or config setting, add it to the list */
		else if (port->type != IPT_DIPSWITCH_SETTING && port->type != IPT_CONFIG_SETTING)
		{
			/* fatal error if we didn't hit an IPT_PORT */
			if (portnum < 0)
				fatalerror("Error in InputPort definition: expecting PORT_START");

			/* fatal error if too many bits */
			if (bitnum >= MAX_BITS_PER_PORT)
				fatalerror("Error in InputPort definition: too many bits for a port (%d max)", MAX_BITS_PER_PORT);

			/* fill in the bit info */
			port_info[portnum].bit[bitnum].port = port;
			port_info[portnum].bit[bitnum].impulse = 0;
			port_info[portnum].bit[bitnum++].last = 0;

			/* if this is a custom input, add it to the list */
			if (port->custom != NULL)
			{
				custom_port_info *info;

				/* allocate memory */
				info = auto_malloc(sizeof(*info));
				memset(info, 0, sizeof(*info));

				/* fill in the data */
				info->port = port;
				for (mask = port->mask; !(mask & 1); mask >>= 1)
					info->shift++;

				/* hook in the list */
				info->next = port_info[portnum].custominfo;
				port_info[portnum].custominfo = info;
			}

			/* if this is an analog port, create an info struct for it */
			else if (IS_ANALOG(port))
			{
				analog_port_info *info;

				/* allocate memory */
				info = auto_malloc(sizeof(*info));
				memset(info, 0, sizeof(*info));

				/* fill in the data */
				info->port = port;
				for (mask = port->mask; !(mask & 1); mask >>= 1)
					info->shift++;
				for ( ; mask & 1; mask >>= 1)
					info->bits++;

				/* workaround: if bits < 8, clamp to 8; we will mask them at the end */
				if (info->bits < 8)
					info->bits = 8;

				/* based on the port type determine if we need absolute or relative coordinates */
				info->minimum = ANALOG_VALUE_MIN;
				info->maximum = ANALOG_VALUE_MAX;
				info->interpolate = 1;
				switch (port->type)
				{
					/* pedals are absolute and get interpolation, but only range in the negative values */
					case IPT_PEDAL:
					case IPT_PEDAL2:
					case IPT_PEDAL3:
						info->maximum = 0;
						info->pedal = 1;
						/* fall through... */

					/* pedals, paddles and analog joysticks are absolute and autocenter */
					case IPT_PADDLE:
					case IPT_PADDLE_V:
					case IPT_AD_STICK_X:
					case IPT_AD_STICK_Y:
					case IPT_AD_STICK_Z:
						info->absolute = 1;
						info->autocenter = 1;
						break;

					/* lightguns are absolute as well, but don't autocenter and don't interpolate their values */
					case IPT_LIGHTGUN_X:
					case IPT_LIGHTGUN_Y:
						info->absolute = 1;
						info->interpolate = 0;
						break;

					/* dials, mice and trackballs are relative devices */
					/* these have fixed "min" and "max" values based on how many bits are in the port */
					/* in addition, we set the wrap around min/max values to 512 * the min/max values */
					/* this takes into account the mapping that one mouse unit ~= 512 analog units */
					case IPT_DIAL:
					case IPT_DIAL_V:
					case IPT_MOUSE_X:
					case IPT_MOUSE_Y:
					case IPT_TRACKBALL_X:
					case IPT_TRACKBALL_Y:
						info->absolute = 0;
						port->analog.min = 0;
						port->analog.max = (1 << info->bits) - 1;
						info->minimum = 0;
						info->maximum = port->analog.max * 512;
						break;

					default:
						fatalerror("Unknown analog port type -- don't know if it is absolute or not");
						break;
				}

				/* extremes can be signed or unsigned */
				if (info->absolute)
				{
					if (port->analog.max > port->analog.min)
					{
						info->scalepos = (double)(port->analog.max - port->default_value) / (double)(ANALOG_VALUE_MAX - 0);
						info->scaleneg = (double)(port->default_value - port->analog.min) / (double)(0 - ANALOG_VALUE_MIN);
					}
					else
					{
						info->scalepos = (double)((INT32)((port->analog.max - port->default_value) << (32 - info->bits)) >> (32 - info->bits)) / (double)(ANALOG_VALUE_MAX - 0);
						info->scaleneg = (double)((INT32)((port->default_value - port->analog.min) << (32 - info->bits)) >> (32 - info->bits)) / (double)(0 - ANALOG_VALUE_MIN);
					}
				}

				/* relative controls all map directly with a 512x scale factor */
				else
					info->scalepos = info->scaleneg = 1.0 / 512.0;

				/* compute scale for keypresses */
				info->keyscale = 1.0 / (0.5 * (info->scalepos + info->scaleneg));

				/* hook in the list */
				info->next = port_info[portnum].analoginfo;
				port_info[portnum].analoginfo = info;
			}

			/* if this is a digital joystick port, update info on it */
			else if (IS_DIGITAL_JOYSTICK(port))
			{
				digital_joystick_info *info = JOYSTICK_INFO_FOR_PORT(port);
				info->port[JOYSTICK_DIR_FOR_PORT(port)] = port;
				info->inuse = 1;
			}
		}
	}

	/* run an initial update */
	input_port_vblank_start();
}



/*************************************
 *
 *  Identifies which port types are
 *  saved/loaded
 *
 *************************************/

static int save_this_port_type(int type)
{
	switch (type)
	{
		case IPT_UNUSED:
		case IPT_END:
		case IPT_PORT:
		case IPT_DIPSWITCH_SETTING:
		case IPT_CONFIG_SETTING:
		case IPT_CATEGORY_SETTING:
		case IPT_VBLANK:
		case IPT_UNKNOWN:
			return 0;
	}
	return 1;
}



/*************************************
 *
 *  Input port configuration read
 *
 *************************************/

INLINE input_code get_default_code(int config_type, int type)
{
	switch (type)
	{
		case IPT_DIPSWITCH_NAME:
		case IPT_CATEGORY_NAME:
			return CODE_NONE;

		default:
			if (config_type != CONFIG_TYPE_GAME)
				return CODE_NONE;
			else
				return CODE_DEFAULT;
	}
	return CODE_NONE;
}


INLINE int string_to_seq_index(const char *string)
{
	int seqindex;

	for (seqindex = 0; seqindex < ARRAY_LENGTH(seqtypestrings); seqindex++)
		if (!mame_stricmp(string, seqtypestrings[seqindex]))
			return seqindex;

	return -1;
}


static void apply_remaps(input_code *remaptable)
{
	int remapnum, portnum, seqnum;

	/* loop over the remapping table, operating only if something was specified */
	for (remapnum = 0; remapnum < __code_max; remapnum++)
		if (remaptable[remapnum] != remapnum)

			/* loop over all default ports, remapping the requested keys */
			for (portnum = 0; default_ports[portnum].type != IPT_END; portnum++)
			{
				/* remap anything in the default sequences */
				for (seqnum = 0; seqnum < SEQ_MAX; seqnum++)
					if (default_ports[portnum].defaultseq.code[seqnum] == remapnum)
						default_ports[portnum].defaultseq.code[seqnum] = remaptable[remapnum];
				for (seqnum = 0; seqnum < SEQ_MAX; seqnum++)
					if (default_ports[portnum].defaultdecseq.code[seqnum] == remapnum)
						default_ports[portnum].defaultdecseq.code[seqnum] = remaptable[remapnum];
				for (seqnum = 0; seqnum < SEQ_MAX; seqnum++)
					if (default_ports[portnum].defaultincseq.code[seqnum] == remapnum)
						default_ports[portnum].defaultincseq.code[seqnum] = remaptable[remapnum];
			}
}



static int apply_config_to_default(xml_data_node *portnode, int type, int player, input_seq *newseq)
{
	int portnum;

	/* find a matching port in the list */
	for (portnum = 0; portnum < ARRAY_LENGTH(default_ports_backup); portnum++)
	{
		input_port_default_entry *updateport = &default_ports[portnum];
		if (updateport->type == type && updateport->player == player)
		{
			/* copy the sequence(s) that were specified */
			if (seq_get_1(&newseq[0]) != __code_max)
				seq_copy(&updateport->defaultseq, &newseq[0]);
			if (seq_get_1(&newseq[1]) != __code_max)
				seq_copy(&updateport->defaultdecseq, &newseq[1]);
			if (seq_get_1(&newseq[2]) != __code_max)
				seq_copy(&updateport->defaultincseq, &newseq[2]);
			return 1;
		}
	}
	return 0;
}


static int apply_config_to_current(xml_data_node *portnode, int type, int player, input_seq *newseq)
{
	input_port_entry *updateport;
	int mask, defvalue, index;

	/* read the mask, index, and defvalue attributes */
	index = xml_get_attribute_int(portnode, "index", 0);
	mask = xml_get_attribute_int(portnode, "mask", 0);
	defvalue = xml_get_attribute_int(portnode, "defvalue", 0);

	/* find the indexed port; we scan the array to make sure we don't read past the end */
	for (updateport = input_ports_default; updateport->type != IPT_END; updateport++)
		if (index-- == 0)
			break;

	/* verify that it matches */
	if (updateport->type == type && updateport->player == player &&
		updateport->mask == mask && (updateport->default_value & mask) == (defvalue & mask))
	{
		const char *revstring;

		/* point to the real port */
		updateport = Machine->input_ports + (updateport - input_ports_default);

		/* fill in the data from the attributes */
		updateport->default_value = xml_get_attribute_int(portnode, "value", updateport->default_value);
		updateport->analog.delta = xml_get_attribute_int(portnode, "keydelta", updateport->analog.delta);
		updateport->analog.centerdelta = xml_get_attribute_int(portnode, "centerdelta", updateport->analog.centerdelta);
		updateport->analog.sensitivity = xml_get_attribute_int(portnode, "sensitivity", updateport->analog.sensitivity);
		revstring = xml_get_attribute_string(portnode, "reverse", NULL);
		if (revstring)
			updateport->analog.reverse = (strcmp(revstring, "yes") == 0);

		/* copy the sequence(s) that were specified */
		if (seq_get_1(&newseq[0]) != __code_max)
			seq_copy(&updateport->seq, &newseq[0]);
		if (seq_get_1(&newseq[1]) != __code_max)
			seq_copy(&updateport->analog.decseq, &newseq[1]);
		if (seq_get_1(&newseq[2]) != __code_max)
			seq_copy(&updateport->analog.incseq, &newseq[2]);
		return 1;
	}
	return 0;
}


static void input_port_load(int config_type, xml_data_node *parentnode)
{
	xml_data_node *portnode;
	int seqnum;

	/* in the completion phase, we finish the initialization with the final ports */
	if (config_type == CONFIG_TYPE_FINAL)
		input_port_postload();

	/* early exit if no data to parse */
	if (!parentnode)
		return;

	/* iterate over all the remap nodes for controller configs only */
	if (config_type == CONFIG_TYPE_CONTROLLER)
	{
		xml_data_node *remapnode;
		input_code remap[__code_max];
		int remapnum;

		/* reset the remap table to default */
		for (remapnum = 0; remapnum < __code_max; remapnum++)
			remap[remapnum] = remapnum;

		/* build up the remap table */
		for (remapnode = xml_get_sibling(parentnode->child, "remap"); remapnode; remapnode = xml_get_sibling(remapnode->next, "remap"))
		{
			input_code origcode = token_to_code(xml_get_attribute_string(remapnode, "origcode", ""));
			input_code newcode = token_to_code(xml_get_attribute_string(remapnode, "newcode", ""));
			if (origcode != CODE_NONE && origcode < __code_max && newcode != CODE_NONE)
				remap[origcode] = newcode;
		}

		/* apply it */
		apply_remaps(remap);
	}

	/* iterate over all the port nodes */
	for (portnode = xml_get_sibling(parentnode->child, "port"); portnode; portnode = xml_get_sibling(portnode->next, "port"))
	{
		input_seq newseq[3], tempseq;
		xml_data_node *seqnode;
		int type, player;

		/* get the basic port info from the attributes */
		type = token_to_port_type(xml_get_attribute_string(portnode, "type", ""), &player);

		/* initialize sequences to invalid defaults */
		for (seqnum = 0; seqnum < 3; seqnum++)
			seq_set_1(&newseq[seqnum], __code_max);

		/* loop over new sequences */
		for (seqnode = xml_get_sibling(portnode->child, "newseq"); seqnode; seqnode = xml_get_sibling(seqnode->next, "newseq"))
		{
			/* with a valid type, parse out the new sequence */
			seqnum = string_to_seq_index(xml_get_attribute_string(seqnode, "type", ""));
			if (seqnum != -1)
				if (seqnode->value && string_to_seq(seqnode->value, &tempseq) != 0)
					seq_copy(&newseq[seqnum], &tempseq);
		}

		/* if we're loading default ports, apply to the default_ports */
		if (config_type != CONFIG_TYPE_GAME)
			apply_config_to_default(portnode, type, player, newseq);
		else
			apply_config_to_current(portnode, type, player, newseq);
	}

	/* after applying the controller config, push that back into the backup, since that is */
	/* what we will diff against */
	if (config_type == CONFIG_TYPE_CONTROLLER)
		memcpy(default_ports_backup, default_ports, sizeof(default_ports_backup));
}



/*************************************
 *
 *  Input port configuration write
 *
 *************************************/

static void add_sequence(xml_data_node *parentnode, int type, int porttype, const input_seq *seq)
{
	xml_data_node *seqnode;
	char seqbuffer[256];

	/* get the string for the sequence */
	if (seq_get_1(seq) == CODE_NONE)
		strcpy(seqbuffer, "NONE");
	else
		seq_to_string(seq, seqbuffer, sizeof(seqbuffer));

	/* add the new node */
	seqnode = xml_add_child(parentnode, "newseq", seqbuffer);
	if (seqnode)
		xml_set_attribute(seqnode, "type", seqtypestrings[type]);
}


static void save_default_inputs(xml_data_node *parentnode)
{
	int portnum;

	/* iterate over ports */
	for (portnum = 0; portnum < ARRAY_LENGTH(default_ports_backup); portnum++)
	{
		input_port_default_entry *defport = &default_ports_backup[portnum];
		input_port_default_entry *curport = &default_ports[portnum];

		/* only save if something has changed and this port is a type we save */
		if (save_this_port_type(defport->type) &&
			(seq_cmp(&defport->defaultseq, &curport->defaultseq) != 0 ||
			 seq_cmp(&defport->defaultdecseq, &curport->defaultdecseq) != 0 ||
			 seq_cmp(&defport->defaultincseq, &curport->defaultincseq) != 0))
		{
			/* add a new port node */
			xml_data_node *portnode = xml_add_child(parentnode, "port", NULL);
			if (portnode)
			{
				/* add the port information and attributes */
				xml_set_attribute(portnode, "type", port_type_to_token(defport->type, defport->player));

				/* add only the sequences that have changed from the defaults */
				if (seq_cmp(&defport->defaultseq, &curport->defaultseq) != 0)
					add_sequence(portnode, 0, defport->type, &curport->defaultseq);
				if (seq_cmp(&defport->defaultdecseq, &curport->defaultdecseq) != 0)
					add_sequence(portnode, 1, defport->type, &curport->defaultdecseq);
				if (seq_cmp(&defport->defaultincseq, &curport->defaultincseq) != 0)
					add_sequence(portnode, 2, defport->type, &curport->defaultincseq);
			}
		}
	}
}


static void save_game_inputs(xml_data_node *parentnode)
{
	int portnum;

	/* iterate over ports */
	for (portnum = 0; input_ports_default[portnum].type != IPT_END; portnum++)
	{
		input_port_entry *defport = &input_ports_default[portnum];
		input_port_entry *curport = &Machine->input_ports[portnum];

		/* only save if something has changed and this port is a type we save */
		if (save_this_port_type(defport->type) &&
			((defport->default_value & defport->mask) != (curport->default_value & defport->mask) ||
			 defport->analog.delta != curport->analog.delta ||
			 defport->analog.centerdelta != curport->analog.centerdelta ||
			 defport->analog.sensitivity != curport->analog.sensitivity ||
			 defport->analog.reverse != curport->analog.reverse ||
			 seq_cmp(&defport->seq, &curport->seq) != 0 ||
			 seq_cmp(&defport->analog.decseq, &curport->analog.decseq) != 0 ||
			 seq_cmp(&defport->analog.incseq, &curport->analog.incseq) != 0))
		{
			/* add a new port node */
			xml_data_node *portnode = xml_add_child(parentnode, "port", NULL);
			if (portnode)
			{
				/* add the port information and attributes */
				xml_set_attribute(portnode, "type", port_type_to_token(defport->type, defport->player));
				xml_set_attribute_int(portnode, "mask", defport->mask);
				xml_set_attribute_int(portnode, "index", portnum);
				xml_set_attribute_int(portnode, "defvalue", defport->default_value & defport->mask);

				/* if the value has changed, add it as well */
				if (defport->default_value != curport->default_value)
					xml_set_attribute_int(portnode, "value", curport->default_value & defport->mask);

				/* add analog-specific attributes if they have changed */
				if (port_type_is_analog(defport->type))
				{
					if (defport->analog.delta != curport->analog.delta)
						xml_set_attribute_int(portnode, "keydelta", curport->analog.delta);
					if (defport->analog.centerdelta != curport->analog.centerdelta)
						xml_set_attribute_int(portnode, "centerdelta", curport->analog.centerdelta);
					if (defport->analog.sensitivity != curport->analog.sensitivity)
						xml_set_attribute_int(portnode, "sensitivity", curport->analog.sensitivity);
					if (defport->analog.reverse != curport->analog.reverse)
						xml_set_attribute(portnode, "reverse", curport->analog.reverse ? "yes" : "no");
				}

				/* add only the sequences that have changed from the defaults */
				if (seq_cmp(&defport->seq, &curport->seq) != 0)
					add_sequence(portnode, 0, defport->type, &curport->seq);
				if (seq_cmp(&defport->analog.decseq, &curport->analog.decseq) != 0)
					add_sequence(portnode, 1, defport->type, &curport->analog.decseq);
				if (seq_cmp(&defport->analog.incseq, &curport->analog.incseq) != 0)
					add_sequence(portnode, 2, defport->type, &curport->analog.incseq);
			}
		}
	}
}


static void input_port_save(int config_type, xml_data_node *parentnode)
{
	if (parentnode)
	{
		/* default ports save differently */
		if (config_type == CONFIG_TYPE_DEFAULT)
			save_default_inputs(parentnode);
		else
			save_game_inputs(parentnode);
	}
}



/*************************************
 *
 *  Input port construction
 *
 *************************************/

input_port_entry *input_port_initialize(input_port_init_params *iip, UINT32 type, const char *tag, UINT32 mask)
{
	/* this function is used within an INPUT_PORT callback to set up a single port */
	input_port_entry *port;
	input_code code;

	/* are we modifying an existing port? */
	if (tag != NULL)
	{
		int portnum, deleting;

		/* find the matching port */
		for (portnum = 0; portnum < iip->current_port; portnum++)
			if (iip->ports[portnum].type == IPT_PORT && iip->ports[portnum].start.tag != NULL && !strcmp(iip->ports[portnum].start.tag, tag))
				break;
		if (portnum >= iip->current_port)
			fatalerror("Could not find port to modify: '%s'", tag);

		/* nuke any matching masks */
		for (portnum++, deleting = 0; portnum < iip->current_port && iip->ports[portnum].type != IPT_PORT; portnum++)
		{
			deleting = (iip->ports[portnum].mask & mask) || (deleting && iip->ports[portnum].mask == 0);
			if (deleting)
			{
				iip->current_port--;
				memmove(&iip->ports[portnum], &iip->ports[portnum + 1], (iip->current_port - portnum) * sizeof(iip->ports[0]));
				portnum--;
			}
		}

		/* allocate space for a new port at the end of this entry */
		if (iip->current_port >= iip->max_ports)
			fatalerror("Too many input ports");
		if (portnum < iip->current_port)
		{
			memmove(&iip->ports[portnum + 1], &iip->ports[portnum], (iip->current_port - portnum) * sizeof(iip->ports[0]));
			iip->current_port++;
			port = &iip->ports[portnum];
		}
		else
			port = &iip->ports[iip->current_port++];
	}

	/* otherwise, just allocate a new one from the end */
	else
	{
		if (iip->current_port >= iip->max_ports)
			fatalerror("Too many input ports");
		port = &iip->ports[iip->current_port++];
	}

	/* set up defaults */
	memset(port, 0, sizeof(*port));
	port->name = IP_NAME_DEFAULT;
	port->type = type;

	/* sets up default port codes */
	switch (port->type)
	{
		case IPT_DIPSWITCH_NAME:
		case IPT_DIPSWITCH_SETTING:
		case IPT_CONFIG_NAME:
		case IPT_CONFIG_SETTING:
			code = CODE_NONE;
			break;

		default:
			code = CODE_DEFAULT;
			break;
	}

	/* set the default codes */
	seq_set_1(&port->seq, code);
	seq_set_1(&port->analog.incseq, code);
	seq_set_1(&port->analog.decseq, code);
	return port;
}


input_port_entry *input_port_allocate(void (*construct_ipt)(input_port_init_params *param), input_port_entry *memory)
{
	input_port_init_params iip;

	/* set up the port parameter structure */
 	iip.max_ports = MAX_INPUT_PORTS * MAX_BITS_PER_PORT;
 	iip.current_port = 0;

	/* allocate memory for the input ports */
	if (!memory)
		iip.ports = (input_port_entry *)auto_malloc(iip.max_ports * sizeof(*iip.ports));
	else
		iip.ports = memory;
	memset(iip.ports, 0, iip.max_ports * sizeof(*iip.ports));

	/* construct the ports */
 	construct_ipt(&iip);

	/* append final IPT_END */
	input_port_initialize(&iip, IPT_END, NULL, 0);

#ifdef MESS
	/* process MESS specific extensions to the port */
	inputx_handle_mess_extensions(iip.ports);
#endif

	return iip.ports;
}


void input_port_parse_diplocation(input_port_entry *in, const char *location)
{
	char *curname = NULL, tempbuf[100];
	const char *entry;
	int index, val, bits;
	UINT32 temp;

	/* if nothing present, bail */
	if (!location)
		return;
	memset(in->diploc, 0, sizeof(in->diploc));

	/* parse the string */
	for (index = 0, entry = location; *entry && index < ARRAY_LENGTH(in->diploc); index++)
	{
		const char *comma, *colon, *number;

		/* find the end of this entry */
		comma = strchr(entry, ',');
		if (comma == NULL)
			comma = entry + strlen(entry);

		/* extract it to tempbuf */
		strncpy(tempbuf, entry, comma - entry);
		tempbuf[comma - entry] = 0;

		/* first extract the switch name if present */
		number = tempbuf;
		colon = strchr(tempbuf, ':');
		if (colon != NULL)
		{
			curname = auto_malloc(colon - tempbuf + 1);
			strncpy(curname, tempbuf, colon - tempbuf);
			tempbuf[colon - tempbuf] = 0;
			number = colon + 1;
		}

		/* if we don't have a name by now, we're screwed */
		if (curname == NULL)
			fatalerror("Switch location '%s' missing switch name!", location);

		/* now scan the switch number */
		if (sscanf(number, "%d", &val) != 1)
			fatalerror("Switch location '%s' has invalid format!", location);

		/* fill the entry and bump the index */
		in->diploc[index].swname = curname;
		in->diploc[index].swnum = val;

		/* advance to the next item */
		entry = comma;
		if (*entry)
			entry++;
	}

	/* then verify the number of bits in the mask matches */
	for (bits = 0, temp = in->mask; temp && bits < 32; bits++)
		temp &= temp - 1;
	if (bits != index)
		fatalerror("Switch location '%s' does not describe enough bits for mask %X\n", location, in->mask);
}



/*************************************
 *
 *  List access
 *
 *************************************/

input_port_default_entry *get_input_port_list(void)
{
	return default_ports;
}


const input_port_default_entry *get_input_port_list_defaults(void)
{
	return default_ports_backup;
}



/*************************************
 *
 *  Input port tokens
 *
 *************************************/

const char *port_type_to_token(int type, int player)
{
	static char tempbuf[32];
	int defindex;

	/* look up the port and return the token */
	defindex = default_ports_lookup[type][player];
	if (defindex != -1)
		return default_ports[defindex].token;

	/* if that fails, carry on */
	sprintf(tempbuf, "TYPE_OTHER(%d,%d)", type, player);
	return tempbuf;
}


int token_to_port_type(const char *string, int *player)
{
	int ipnum;

	/* check for our failsafe case first */
	if (sscanf(string, "TYPE_OTHER(%d,%d)", &ipnum, player) == 2)
		return ipnum;

	/* find the token in the list */
	for (ipnum = 0; ipnum < input_port_count; ipnum++)
		if (default_ports[ipnum].token != NULL && !strcmp(default_ports[ipnum].token, string))
		{
			*player = default_ports[ipnum].player;
			return default_ports[ipnum].type;
		}

	/* if we fail, return IPT_UNKNOWN */
	*player = 0;
	return IPT_UNKNOWN;
}



/*************************************
 *
 *  Input port getters
 *
 *************************************/

int input_port_active(const input_port_entry *port)
{
	return (input_port_name(port) != NULL && !port->unused);
}


int port_type_is_analog(int type)
{
	return (type >= __ipt_analog_start && type <= __ipt_analog_end);
}


int port_type_in_use(int type)
{
	input_port_entry *port;
	for (port = Machine->input_ports; port->type != IPT_END; port++)
		if (port->type == type)
			return 1;
	return 0;
}


int port_type_to_group(int type, int player)
{
	int defindex = default_ports_lookup[type][player];
	if (defindex != -1)
		return default_ports[defindex].group;
	return IPG_INVALID;
}


int port_tag_to_index(const char *tag)
{
	int port;

	/* find the matching tag */
	for (port = 0; port < MAX_INPUT_PORTS; port++)
		if (port_info[port].tag != NULL && !strcmp(port_info[port].tag, tag))
			return port;
	return -1;
}


read8_handler port_tag_to_handler8(const char *tag)
{
	int port = port_tag_to_index(tag);
	return (port == -1) ? MRA8_NOP : port_handler8[port];
}


read16_handler port_tag_to_handler16(const char *tag)
{
	int port = port_tag_to_index(tag);
	return (port == -1) ? MRA16_NOP : port_handler16[port];
}


read32_handler port_tag_to_handler32(const char *tag)
{
	int port = port_tag_to_index(tag);
	return (port == -1) ? MRA32_NOP : port_handler32[port];
}


const char *input_port_name(const input_port_entry *port)
{
	int defindex;

	/* if we have a non-default name, use that */
	if (port->name != IP_NAME_DEFAULT)
		return port->name;

	/* if the port exists, return the default name */
	defindex = default_ports_lookup[port->type][port->player];
	if (defindex != -1)
		return default_ports[defindex].name;

	/* should never get here */
	return NULL;
}


input_seq *input_port_seq(input_port_entry *port, int seqtype)
{
	static input_seq ip_none = SEQ_DEF_1(CODE_NONE);
	input_seq *portseq;

	/* if port is disabled, return no key */
	if (port->unused)
		return &ip_none;

	/* handle the various seq types */
	switch (seqtype)
	{
		case SEQ_TYPE_STANDARD:
			portseq = &port->seq;
			break;

		case SEQ_TYPE_INCREMENT:
			if (!IS_ANALOG(port))
				return &ip_none;
			portseq = &port->analog.incseq;
			break;

		case SEQ_TYPE_DECREMENT:
			if (!IS_ANALOG(port))
				return &ip_none;
			portseq = &port->analog.decseq;
			break;

		default:
			return &ip_none;
	}

	/* does this override the default? if so, return it directly */
	if (seq_get_1(portseq) != CODE_DEFAULT)
		return portseq;

	/* otherwise find the default setting */
	return input_port_default_seq(port->type, port->player, seqtype);
}


input_seq *input_port_default_seq(int type, int player, int seqtype)
{
	static input_seq ip_none = SEQ_DEF_1(CODE_NONE);

	/* find the default setting */
	int defindex = default_ports_lookup[type][player];
	if (defindex != -1)
	{
		switch (seqtype)
		{
			case SEQ_TYPE_STANDARD:
				return &default_ports[defindex].defaultseq;
			case SEQ_TYPE_INCREMENT:
				return &default_ports[defindex].defaultincseq;
			case SEQ_TYPE_DECREMENT:
				return &default_ports[defindex].defaultdecseq;
		}
	}
	return &ip_none;
}


int input_port_condition(const input_port_entry *in)
{
	switch (in->condition.condition)
	{
		case PORTCOND_EQUALS:
			return ((readinputport(in->condition.portnum) & in->condition.mask) == in->condition.value);
		case PORTCOND_NOTEQUALS:
			return ((readinputport(in->condition.portnum) & in->condition.mask) != in->condition.value);
	}
	return 1;
}



/*************************************
 *
 *  Key sequence handlers
 *
 *************************************/

int input_port_type_pressed(int type, int player)
{
	int defindex = default_ports_lookup[type][player];
	if (defindex != -1) {
		int pressed = seq_pressed(&default_ports[defindex].defaultseq);

		/* AdvanceMAME: Filter all the input ports */
		pressed = osd_input_port_filter(pressed, type, player, SEQ_TYPE_STANDARD);

		return pressed;
	}

	return 0;
}


int input_ui_pressed(int code)
{
	int pressed;

profiler_mark(PROFILER_INPUT);

	/* get the status of this key (assumed to be only in the defaults) */
	pressed = seq_pressed(input_port_default_seq(code, 0, SEQ_TYPE_STANDARD));

	/* AdvanceMAME: Filter all the input ports */
	pressed = osd_input_port_filter(pressed, code, 0, SEQ_TYPE_STANDARD);

	/* if pressed, handle it specially */
	if (pressed)
	{
		/* if this is the first press, leave pressed = 1 */
		if (ui_memory[code] == 0)
			ui_memory[code] = 1;

		/* otherwise, reset pressed = 0 */
		else
			pressed = 0;
	}

	/* if we're not pressed, reset the memory field */
	else
		ui_memory[code] = 0;

profiler_mark(PROFILER_END);

	return pressed;
}


int input_ui_pressed_repeat(int code, int speed)
{
	static int counter;
	static int keydelay;
	int pressed;

profiler_mark(PROFILER_INPUT);

	/* get the status of this key (assumed to be only in the defaults) */
	pressed = seq_pressed(input_port_default_seq(code, 0, SEQ_TYPE_STANDARD));

	/* AdvanceMAME: Filter all the input ports */
	pressed = osd_input_port_filter(pressed, code, 0, SEQ_TYPE_STANDARD);

	/* if so, handle it specially */
	if (pressed)
	{
		/* if this is the first press, set a 3x delay and leave pressed = 1 */
		if (ui_memory[code] == 0)
		{
			ui_memory[code] = 1;
			keydelay = 3;
			counter = 0;
		}

		/* if this is an autorepeat case, set a 1x delay and leave pressed = 1 */
		else if (++counter > keydelay * speed * Machine->refresh_rate / 60)
		{
			keydelay = 1;
			counter = 0;
		}

		/* otherwise, reset pressed = 0 */
		else
			pressed = 0;
	}

	/* if we're not pressed, reset the memory field */
	else
		ui_memory[code] = 0;

profiler_mark(PROFILER_END);

	return pressed;
}



/*************************************
 *
 *  Playback/record helper
 *
 *************************************/

static void update_playback_record(int portnum, UINT32 portvalue)
{
	/* handle playback */
	if (Machine->playback_file != NULL)
	{
		UINT32 result;

		/* a successful read goes into the playback field which overrides everything else */
		if (mame_fread(Machine->playback_file, &result, sizeof(result)) == sizeof(result))
			portvalue = port_info[portnum].playback = BIG_ENDIANIZE_INT32(result);

		/* a failure causes us to close the playback file and stop playback */
		else
		{
			mame_fclose(Machine->playback_file);
			Machine->playback_file = NULL;
		}
	}

	/* handle recording */
	if (Machine->record_file != NULL)
	{
		UINT32 result = BIG_ENDIANIZE_INT32(portvalue);

		/* a successful write just works */
		if (mame_fwrite(Machine->record_file, &result, sizeof(result)) == sizeof(result))
			;

		/* a failure causes us to close the record file and stop recording */
		else
		{
			mame_fclose(Machine->record_file);
			Machine->record_file = NULL;
		}
	}
}



/*************************************
 *
 *  Update default ports
 *
 *************************************/

void input_port_update_defaults(void)
{
	int portnum;

	/* loop over all input ports */
	for (portnum = 0; portnum < MAX_INPUT_PORTS; portnum++)
	{
		input_port_info *portinfo = &port_info[portnum];
		input_bit_info *info;
		int bitnum;

		/* first compute the default value for the entire port */
		portinfo->defvalue = 0;
		for (bitnum = 0, info = &portinfo->bit[0]; bitnum < MAX_BITS_PER_PORT && info->port; bitnum++, info++)
			if (input_port_condition(info->port))
				portinfo->defvalue = (portinfo->defvalue & ~info->port->mask) | (info->port->default_value & info->port->mask);
	}
}



/*************************************
 *
 *  VBLANK start routine
 *
 *************************************/

void input_port_vblank_start(void)
{
	int ui_visible = ui_is_setup_active() || ui_is_onscrd_active();
	int portnum, bitnum;

	/* AdvanceMAME: Never disable the input port. */
	ui_visible = 0;

profiler_mark(PROFILER_INPUT);

	/* update the digital joysticks first */
	update_digital_joysticks();

	/* compute default values for all the ports */
	input_port_update_defaults();

	/* loop over all input ports */
	for (portnum = 0; portnum < MAX_INPUT_PORTS; portnum++)
	{
		input_port_info *portinfo = &port_info[portnum];
		input_bit_info *info;

		/* compute the VBLANK mask */
		portinfo->vblank = 0;
		for (bitnum = 0, info = &portinfo->bit[0]; bitnum < MAX_BITS_PER_PORT && info->port; bitnum++, info++)
			if (info->port->type == IPT_VBLANK)
			{
				portinfo->vblank ^= info->port->mask;
				if (Machine->drv->vblank_duration == 0)
					logerror("Warning: you are using IPT_VBLANK with vblank_duration = 0. You need to increase vblank_duration for IPT_VBLANK to work.\n");
			}

		/* now loop back and modify based on the inputs */
		portinfo->digital = 0;
		for (bitnum = 0, info = &portinfo->bit[0]; bitnum < MAX_BITS_PER_PORT && info->port; bitnum++, info++)
			if (input_port_condition(info->port))
			{
				input_port_entry *port = info->port;

				/* handle non-analog types, but only when the UI isn't visible */
				if (port->type != IPT_VBLANK && !IS_ANALOG(port) && !ui_visible)
				{
					/* if the sequence for this port is currently pressed.... */
					int pressed = seq_pressed(input_port_seq(port, SEQ_TYPE_STANDARD));

					/* AdvanceMAME: Filter all the input ports */
					pressed = osd_input_port_filter(pressed, port->type, port->player, SEQ_TYPE_STANDARD);

					if (pressed)
					{
#ifdef MESS
						/* (MESS-specific) check for disabled keyboard */
						if (port->type == IPT_KEYBOARD && osd_keyboard_disabled())
							continue;
#endif
						/* skip locked-out coin inputs */
						if (port->type >= IPT_COIN1 && port->type <= IPT_COIN8 && coinlockedout[port->type - IPT_COIN1])
							continue;

						/* if this is a downward press and we're an impulse control, reset the count */
						if (port->impulse)
						{
							if (info->last == 0)
								info->impulse = port->impulse;
						}

						/* if this is a downward press and we're a toggle control, toggle the value */
						else if (port->toggle)
						{
							if (info->last == 0)
							{
								port->default_value ^= port->mask;
								portinfo->digital ^= port->mask;
							}
						}

						/* if this is a digital joystick type, apply either standard or 4-way rules */
						else if (IS_DIGITAL_JOYSTICK(port))
						{
							digital_joystick_info *joyinfo = JOYSTICK_INFO_FOR_PORT(port);
							UINT8 mask = port->four_way ? joyinfo->current4way : joyinfo->current;
							if ((mask >> JOYSTICK_DIR_FOR_PORT(port)) & 1)
								portinfo->digital ^= port->mask;
						}

						/* otherwise, just set it raw */
						else
							portinfo->digital ^= port->mask;

						/* track the last value */
						info->last = 1;
					}
					else
						info->last = 0;

					/* handle the impulse countdown */
					if (port->impulse && info->impulse > 0)
					{
						info->impulse--;
						info->last = 1;
						portinfo->digital ^= port->mask;
					}
				}

				/* note that analog ports are handled instantaneously at port read time */
			}
	}

#ifdef MESS
	/* less MESS to MESSy things */
	inputx_update();
#endif

	/* call changed handlers */
	for (portnum = 0; portnum < MAX_INPUT_PORTS; portnum++)
		if (port_info[portnum].change_notify != NULL)
		{
			changed_callback_info *cbinfo;
			UINT32 newvalue = readinputport(portnum);
			UINT32 oldvalue = port_info[portnum].changed_last_value;
			UINT32 delta = newvalue ^ oldvalue;

			/* call all the callbacks whose mask matches the requested mask */
			for (cbinfo = port_info[portnum].change_notify; cbinfo; cbinfo = cbinfo->next)
				if (delta & cbinfo->mask)
					(*cbinfo->callback)(cbinfo->param, oldvalue & cbinfo->mask, newvalue & cbinfo->mask);

			port_info[portnum].changed_last_value = newvalue;
		}

	/* handle playback/record */
	for (portnum = 0; portnum < MAX_INPUT_PORTS; portnum++)
	{
		/* analog ports automatically update on every read */
		if (port_info[portnum].analoginfo)
			readinputport(portnum);

		/* non-analog ports must be manually updated */
		else
			update_playback_record(portnum, readinputport(portnum));
	}

profiler_mark(PROFILER_END);
}



/*************************************
 *
 *  VBLANK end routine
 *
 *************************************/

void input_port_vblank_end(void)
{
	int ui_visible = ui_is_setup_active() || ui_is_onscrd_active();
	int port;

profiler_mark(PROFILER_INPUT);

	/* update all analog ports if the UI isn't visible */
	if (!ui_visible)
		for (port = 0; port < MAX_INPUT_PORTS; port++)
			update_analog_port(port);

profiler_mark(PROFILER_END);
}



/*************************************
 *
 *  Digital joystick updating
 *
 *************************************/

static void update_digital_joysticks(void)
{
	int player, joyindex;

	/* loop over all the joysticks */
	for (player = 0; player < MAX_PLAYERS; player++)
		for (joyindex = 0; joyindex < DIGITAL_JOYSTICKS_PER_PLAYER; joyindex++)
		{
			digital_joystick_info *info = &joystick_info[player][joyindex];
			if (info->inuse)
			{
				info->previous = info->current;
				info->current = 0;

				/* read all the associated ports */
				/* AdvanceMAME: Filter all the input ports */
				if (info->port[JOYDIR_UP] != NULL && osd_input_port_filter(seq_pressed(input_port_seq(info->port[JOYDIR_UP], SEQ_TYPE_STANDARD)), info->port[JOYDIR_UP]->type, info->port[JOYDIR_UP]->player, SEQ_TYPE_STANDARD))
					info->current |= JOYDIR_UP_BIT;
				if (info->port[JOYDIR_DOWN] != NULL && osd_input_port_filter(seq_pressed(input_port_seq(info->port[JOYDIR_DOWN], SEQ_TYPE_STANDARD)), info->port[JOYDIR_DOWN]->type, info->port[JOYDIR_DOWN]->player, SEQ_TYPE_STANDARD))
					info->current |= JOYDIR_DOWN_BIT;
				if (info->port[JOYDIR_LEFT] != NULL && osd_input_port_filter(seq_pressed(input_port_seq(info->port[JOYDIR_LEFT], SEQ_TYPE_STANDARD)), info->port[JOYDIR_LEFT]->type, info->port[JOYDIR_LEFT]->player, SEQ_TYPE_STANDARD))
					info->current |= JOYDIR_LEFT_BIT;
				if (info->port[JOYDIR_RIGHT] != NULL && osd_input_port_filter(seq_pressed(input_port_seq(info->port[JOYDIR_RIGHT], SEQ_TYPE_STANDARD)), info->port[JOYDIR_RIGHT]->type, info->port[JOYDIR_RIGHT]->player, SEQ_TYPE_STANDARD))
					info->current |= JOYDIR_RIGHT_BIT;

				/* lock out opposing directions (left + right or up + down) */
				if ((info->current & (JOYDIR_UP_BIT | JOYDIR_DOWN_BIT)) == (JOYDIR_UP_BIT | JOYDIR_DOWN_BIT))
					info->current &= ~(JOYDIR_UP_BIT | JOYDIR_DOWN_BIT);
				if ((info->current & (JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT)) == (JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT))
					info->current &= ~(JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT);

				/* only update 4-way case if joystick has moved */
				if (info->current != info->previous)
				{
					info->current4way = info->current;

					/*
                        If joystick is pointing at a diagonal, acknowledge that the player moved
                        the joystick by favoring a direction change.  This minimizes frustration
                        when using a keyboard for input, and maximizes responsiveness.

                        For example, if you are holding "left" then switch to "up" (where both left
                        and up are briefly pressed at the same time), we'll transition immediately
                        to "up."

                        Zero any switches that didn't change from the previous to current state.
                     */
					if ((info->current4way & (JOYDIR_UP_BIT | JOYDIR_DOWN_BIT)) &&
						(info->current4way & (JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT)))
					{
						info->current4way ^= info->current4way & info->previous;
					}

					/*
                        If we are still pointing at a diagonal, we are in an indeterminant state.

                        This could happen if the player moved the joystick from the idle position directly
                        to a diagonal, or from one diagonal directly to an extreme diagonal.

                        The chances of this happening with a keyboard are slim, but we still need to
                        constrain this case.

                        For now, just resolve randomly.
                     */
					if ((info->current4way & (JOYDIR_UP_BIT | JOYDIR_DOWN_BIT)) &&
						(info->current4way & (JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT)))
					{
						if (rand() & 1)
							info->current4way &= ~(JOYDIR_LEFT_BIT | JOYDIR_RIGHT_BIT);
						else
							info->current4way &= ~(JOYDIR_UP_BIT | JOYDIR_DOWN_BIT);
					}
				}
			}
		}
}



/*************************************
 *
 *  Analog minimum/maximum clamping
 *
 *************************************/

INLINE INT32 apply_analog_min_max(const analog_port_info *info, INT32 value)
{
	const input_port_entry *port = info->port;
	INT32 adjmax, adjmin;

	/* take the analog minimum and maximum values and apply the inverse of the */
	/* sensitivity so that we can clamp against them before applying sensitivity */
	adjmin = APPLY_INVERSE_SENSITIVITY(info->minimum, port->analog.sensitivity);
	adjmax = APPLY_INVERSE_SENSITIVITY(info->maximum, port->analog.sensitivity);

	/* for absolute devices, clamp to the bounds absolutely */
	if (info->absolute)
	{
		if (value > adjmax)
			value = adjmax;
		else if (value < adjmin)
			value = adjmin;
	}

	/* for relative devices, wrap around when we go past the edge */
	else
	{
		while (value > adjmax)
			value -= (adjmax - adjmin);
		while (value < adjmin)
			value += (adjmax - adjmin);
	}
	return value;
}



/*************************************
 *
 *  Analog port updating
 *
 *************************************/

static void update_analog_port(int portnum)
{
	analog_port_info *info;

	/* loop over all analog ports in this port number */
	for (info = port_info[portnum].analoginfo; info != NULL; info = info->next)
	{
		input_port_entry *port = info->port;
		INT32 delta = 0, rawvalue;
		int analog_type, keypressed = 0;

		/* clamp the previous value to the min/max range and remember it */
		info->previous = info->accum = apply_analog_min_max(info, info->accum);

		/* AdvanceMAME: Get the analog input for different players from the osd core. */
		rawvalue = osd_get_analog_value(port->type, port->player, &analog_type);

		/* if we got it from a relative device, use that as the starting delta */
		/* also note that the last input was not a digital one */
		if (analog_type == ANALOG_TYPE_RELATIVE && rawvalue != 0)
		{
			delta = rawvalue;
			info->lastdigital = 0;
		}

		/* if the decrement code sequence is pressed, add the key delta to */
		/* the accumulated delta; also note that the last input was a digital one */
		/* AdvanceMAME: Filter all the input ports */
		if (osd_input_port_filter(seq_pressed(input_port_seq(info->port, SEQ_TYPE_DECREMENT)), info->port->type, info->port->player, SEQ_TYPE_DECREMENT))
		{
			delta -= (INT32)(port->analog.delta * info->keyscale);
			keypressed = info->lastdigital = 1;
		}

		/* same for the increment code sequence */
		/* AdvanceMAME: Filter all the input ports */
		if (osd_input_port_filter(seq_pressed(input_port_seq(info->port, SEQ_TYPE_INCREMENT)), info->port->type, info->port->player, SEQ_TYPE_INCREMENT))
		{
			delta += (INT32)(port->analog.delta * info->keyscale);
			keypressed = info->lastdigital = 1;
		}

		/* if resetting is requested, clear the accumulated position to 0 before */
		/* applying the deltas so that we only return this frame's delta */
		/* note that centering only works for relative controls */
		if (port->analog.reset && !info->absolute)
			info->accum = 0;

		/* apply the delta to the accumulated value */
		info->accum += delta;

		/* if we got an absolute input, it overrides everything else */
		if (analog_type == ANALOG_TYPE_ABSOLUTE)
		{
			/* allow both the analog joystick and the keyboard to control the input port */
			if (rawvalue != 0 || !info->lastdigital) {
				/* apply the inverse of the sensitivity to the raw value so that */
				/* it will still cover the full min->max range requested after */
				/* we apply the sensitivity adjustment */
				info->accum = APPLY_INVERSE_SENSITIVITY(rawvalue, port->analog.sensitivity);
				info->lastdigital = 0;
			}
		}

		/* if our last movement was due to a digital input, and if this control */
		/* type autocenters, and if neither the increment nor the decrement seq */
		/* was pressed, apply autocentering */
		if (info->lastdigital && info->autocenter && !keypressed)
		{
			/* autocenter from positive values */
			if (info->accum >= 0)
			{
				info->accum -= (INT32)(port->analog.centerdelta * info->keyscale);
				if (info->accum < 0)
					info->accum = 0;
			}

			/* autocenter from negative values */
			else
			{
				info->accum += (INT32)(port->analog.centerdelta * info->keyscale);
				if (info->accum > 0)
					info->accum = 0;
			}
		}
	}
}



/*************************************
 *
 *  Analog port interpolation
 *
 *************************************/

static void interpolate_analog_port(int port)
{
	analog_port_info *info;

profiler_mark(PROFILER_INPUT);

	/* set the default mask and value */
	port_info[port].analogmask = 0;
	port_info[port].analog = 0;

	/* loop over all analog ports in this port number */
	for (info = port_info[port].analoginfo; info != NULL; info = info->next)
		if (input_port_condition(info->port))
		{
			INT32 current;
			INT32 value;

			/* interpolate or not */
			if (info->interpolate && !info->port->analog.reset)
				current = info->previous + cpu_scalebyfcount(info->accum - info->previous);
			else
				current = info->accum;

			/* apply the min/max and then the sensitivity */
			current = apply_analog_min_max(info, current);
			current = APPLY_SENSITIVITY(current, info->port->analog.sensitivity);

			/* apply special reversal rules for pedals */
			if (info->pedal)
				current = !info->port->analog.reverse ? (info->maximum - current) : (-info->minimum + current);

			/* apply standard reversal rules for everything else */
			else if (info->port->analog.reverse)
				current = -current;

			/* map differently for positive and negative values */
			if (current >= 0)
				value = (INT32)(current * info->scalepos);
			else
				value = (INT32)(current * info->scaleneg);
			value += info->port->default_value;

			/* insert into the port */
			port_info[port].analogmask |= info->port->mask;
			port_info[port].analog = (port_info[port].analog & ~info->port->mask) | ((value << info->shift) & info->port->mask);
		}

profiler_mark(PROFILER_END);
}



/*************************************
 *
 *  Input port reading
 *
 *************************************/

UINT32 readinputport(int port)
{
	input_port_info *portinfo = &port_info[port];
	custom_port_info *custom;
	UINT32 result;

	/* interpolate analog values */
	interpolate_analog_port(port);

	/* update custom values */
	for (custom = portinfo->custominfo; custom; custom = custom->next)
		if (input_port_condition(custom->port))
		{
			/* replace the bits with bits from the custom routine */
			input_port_entry *port = custom->port;
			portinfo->digital &= ~port->mask;
			portinfo->digital |= ((*port->custom)(port->custom_param) << custom->shift) & port->mask;
		}

	/* compute the current result: default value XOR the digital, merged with the analog */
	result = ((portinfo->defvalue ^ portinfo->digital) & ~portinfo->analogmask) | portinfo->analog;

	/* if we have analog data, update the recording state */
	if (port_info[port].analoginfo)
		update_playback_record(port, result);

	/* if we're playing back, use the recorded value for inputs instead */
	if (Machine->playback_file != NULL)
		result = portinfo->playback;

	/* handle VBLANK bits after inputs */
	if (portinfo->vblank)
	{
		/* reset the VBLANK bits to their default value, regardless of inputs */
		result = (result & ~portinfo->vblank) | (portinfo->defvalue & portinfo->vblank);

		/* toggle VBLANK if we're in a VBLANK state */
		if (cpu_getvblank())
			result ^= portinfo->vblank;
	}
	return result;
}


UINT32 readinputportbytag(const char *tag)
{
	int port = port_tag_to_index(tag);
	if (port != -1)
		return readinputport(port);

	/* otherwise fail horribly */
	fatalerror("Unable to locate input port '%s'", tag);
	return -1;
}


UINT32 readinputportbytag_safe(const char *tag, UINT32 defvalue)
{
	int port = port_tag_to_index(tag);
	if (port != -1)
		return readinputport(port);
	return defvalue;
}



/*************************************
 *
 *  Input port writing
 *
 *************************************/

void input_port_set_digital_value(int portnum, UINT32 value, UINT32 mask)
{
	input_port_info *portinfo = &port_info[portnum];
	portinfo->digital &= ~mask;
	portinfo->digital |= value;
}



/*************************************
 *
 *  Input port callbacks
 *
 *************************************/

void input_port_set_changed_callback(int port, UINT32 mask, void (*callback)(void *, UINT32, UINT32), void *param)
{
	input_port_info *portinfo = &port_info[port];
	changed_callback_info *cbinfo;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call input_port_set_changed_callback() at init time!");
	assert_always((port >= 0) && (port < MAX_INPUT_PORTS), "Invalid port number passed to input_port_set_changed_callback()!");

	cbinfo = auto_malloc(sizeof(*cbinfo));
	cbinfo->next = portinfo->change_notify;
	cbinfo->mask = mask;
	cbinfo->callback = callback;
	cbinfo->param = param;

	portinfo->change_notify = cbinfo;
}
