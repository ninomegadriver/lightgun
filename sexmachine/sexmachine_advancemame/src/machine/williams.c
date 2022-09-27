/***************************************************************************

    Williams 6809 system

***************************************************************************/

#include "driver.h"
#include "sndhrdw/williams.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "6821pia.h"
#include "machine/ticket.h"
#include "williams.h"
#include "sound/dac.h"
#include "sound/hc55516.h"


/* timers */
static mame_timer *scanline_timer;
static mame_timer *scan240_timer;
static mame_timer *scan254_timer;

/* banking addresses set by the drivers */
UINT8 *mayday_protection;

/* internal bank switching tracking */
static UINT8 blaster_bank;
static UINT8 vram_bank;

/* other stuff */
static UINT16 joust2_current_sound_data;

/* older-Williams routines */
static void williams_main_irq(int state);
static void williams_main_firq(int state);
static void williams_snd_irq(int state);
static WRITE8_HANDLER( williams_snd_cmd_w );
static WRITE8_HANDLER( playball_snd_cmd_w );

/* input port mapping */
static UINT8 port_select;
static WRITE8_HANDLER( williams_port_select_w );
static READ8_HANDLER( williams_input_port_0_3_r );
static READ8_HANDLER( williams_input_port_49way_0_5_r );
static READ8_HANDLER( williams_input_port_1_4_r );
static READ8_HANDLER( williams_49way_port_0_r );

/* newer-Williams routines */
static WRITE8_HANDLER( williams2_snd_cmd_w );

/* Lotto Fun-specific code */
static READ8_HANDLER( lottofun_input_port_0_r );

/* Turkey Shoot-specific code */
static READ8_HANDLER( tshoot_input_port_0_3_r );
static WRITE8_HANDLER( tshoot_lamp_w );
static WRITE8_HANDLER( tshoot_maxvol_w );

/* Joust 2-specific code */
static WRITE8_HANDLER( joust2_snd_cmd_w );
static WRITE8_HANDLER( joust2_pia_3_cb1_w );



/*************************************
 *
 *  Generic old-Williams PIA interfaces
 *
 *************************************/

/* Generic PIA 0, maps to input ports 0 and 1 */
const pia6821_interface williams_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CB2 */
const pia6821_interface williams_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic dual muxing PIA 0, maps to input ports 0/3 and 1/4; port select is CB2 */
const pia6821_interface williams_dual_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, williams_input_port_1_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic 49-way joystick PIA 0 for Sinistar/Blaster */
const pia6821_interface williams_49way_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_49way_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Muxing 49-way joystick PIA 0 for Blaster kit */
const pia6821_interface williams_49way_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_49way_0_5_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
const pia6821_interface williams_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, williams_snd_cmd_w, 0, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
const pia6821_interface williams_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_0_data_w, 0, 0, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/*************************************
 *
 *  Game-specific old-Williams PIA interfaces
 *
 *************************************/

/* Special PIA 0 for Lotto Fun, to handle the controls and ticket dispenser */
const pia6821_interface lottofun_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ lottofun_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, ticket_dispenser_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 2 for Sinistar, to handle the CVSD */
const pia6821_interface sinistar_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_0_data_w, 0, hc55516_0_digit_w, hc55516_0_clock_w,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};

/* Special PIA 1 for PlayBall, doesn't set the high bits on sound commands */
const pia6821_interface playball_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, playball_snd_cmd_w, 0, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* extra PIA 3 for Speed Ball */
const pia6821_interface spdball_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_3_r, input_port_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};



/*************************************
 *
 *  Generic later-Williams PIA interfaces
 *
 *************************************/

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CA2 */
const pia6821_interface williams2_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, williams_port_select_w, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
const pia6821_interface williams2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, williams2_snd_cmd_w, 0, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
const pia6821_interface williams2_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_0_data_w, pia_1_cb1_w, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/*************************************
 *
 *  Game-specific later-Williams PIA interfaces
 *
 *************************************/

/* Mystic Marathon PIA 0 */
const pia6821_interface mysticm_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ williams_main_firq, williams_main_irq
};

/* Turkey Shoot PIA 0 */
const pia6821_interface tshoot_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ tshoot_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, tshoot_lamp_w, williams_port_select_w, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Turkey Shoot PIA 2 */
const pia6821_interface tshoot_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_0_data_w, pia_1_cb1_w, tshoot_maxvol_w,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};

/* Joust 2 PIA 1 */
const pia6821_interface joust2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, joust2_snd_cmd_w, joust2_pia_3_cb1_w, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};



/*************************************
 *
 *  Older Williams interrupts
 *
 *************************************/

static void williams_va11_callback(int scanline)
{
	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_1_cb1_w(0, scanline & 0x20);

	/* update the screen while we're here */
	force_partial_update(scanline - 1);

	/* set a timer for the next update */
	scanline += 8;
	if (scanline >= 256) scanline = 0;
	timer_adjust(scanline_timer, cpu_getscanlinetime(scanline), scanline, 0);
}


static void williams_count240_off_callback(int param)
{
	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_1_ca1_w(0, 0);
}


static void williams_count240_callback(int param)
{
	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_1_ca1_w(0, 1);

	/* set a timer to turn it off once the scanline counter resets */
	timer_set(cpu_getscanlinetime(0), 0, williams_count240_off_callback);

	/* set a timer for next frame */
	timer_adjust(scan240_timer, cpu_getscanlinetime(240), 0, 0);
}


static void williams_main_irq(int state)
{
	/* IRQ to the main CPU */
	cpunum_set_input_line(0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void williams_main_firq(int state)
{
	/* FIRQ to the main CPU */
	cpunum_set_input_line(0, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void williams_snd_irq(int state)
{
	/* IRQ to the sound CPU */
	cpunum_set_input_line(1, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *  Older Williams initialization
 *
 *************************************/

static void williams_common_init(void)
{
	/* reset the PIAs */
	pia_reset();

	/* reset the ticket dispenser (Lotto Fun) */
	ticket_dispenser_init(70, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_HIGH);

	/* set a timer to go off every 16 scanlines, to toggle the VA11 line and update the screen */
	scanline_timer = timer_alloc(williams_va11_callback);
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);

	/* also set a timer to go off on scanline 240 */
	scan240_timer = timer_alloc(williams_count240_callback);
	timer_adjust(scan240_timer, cpu_getscanlinetime(240), 0, 0);

	state_save_register_global(vram_bank);
}


MACHINE_RESET( williams )
{
	williams_common_init();

	/* configure the memory bank */
	memory_configure_bank(1, 0, 1, williams_videoram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_CPU1) + 0x10000, 0);
}



/*************************************
 *
 *  Newer Williams interrupts
 *
 *************************************/

static void williams2_va11_callback(int scanline)
{
	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_0_cb1_w(0, scanline & 0x20);
	pia_1_ca1_w(0, scanline & 0x20);

	/* update the screen while we're here */
	force_partial_update(scanline);

	/* set a timer for the next update */
	scanline += 8;
	if (scanline >= 256) scanline = 0;
	timer_adjust(scanline_timer, cpu_getscanlinetime(scanline), scanline, 0);
}


static void williams2_endscreen_off_callback(int param)
{
	/* the /ENDSCREEN signal comes into CA1 */
	pia_0_ca1_w(0, 1);
}


static void williams2_endscreen_callback(int param)
{
	/* the /ENDSCREEN signal comes into CA1 */
	pia_0_ca1_w(0, 0);

	/* set a timer to turn it off once the scanline counter resets */
	timer_set(cpu_getscanlinetime(8), 0, williams2_endscreen_off_callback);

	/* set a timer for next frame */
	timer_adjust(scan254_timer, cpu_getscanlinetime(254), 0, 0);
}



/*************************************
 *
 *  Newer Williams initialization
 *
 *************************************/

static void williams2_postload(void)
{
	williams2_bank_select_w(0, vram_bank);
}


MACHINE_RESET( williams2 )
{
	/* reset the PIAs */
	pia_reset();

	/* configure memory banks */
	memory_configure_bank(1, 0, 1, williams_videoram, 0);
	memory_configure_bank(1, 1, 4, memory_region(REGION_CPU1) + 0x10000, 0x10000);

	/* make sure our banking is reset */
	williams2_bank_select_w(0, 0);

	/* set a timer to go off every 16 scanlines, to toggle the VA11 line and update the screen */
	scanline_timer = timer_alloc(williams2_va11_callback);
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);

	/* also set a timer to go off on scanline 254 */
	scan254_timer = timer_alloc(williams2_endscreen_callback);
	timer_adjust(scan254_timer, cpu_getscanlinetime(254), 0, 0);

	state_save_register_global(vram_bank);
	state_save_register_func_postload(williams2_postload);
}



/*************************************
 *
 *  VRAM/ROM banking
 *
 *************************************/

WRITE8_HANDLER( williams_vram_select_w )
{
	/* VRAM/ROM banking from bit 0 */
	vram_bank = data & 0x01;
	memory_set_bank(1, vram_bank);

	/* cocktail flip from bit 1 */
	williams_cocktail = data & 0x02;
}


WRITE8_HANDLER( williams2_bank_select_w )
{
	vram_bank = data & 0x07;

	/* the low two bits control the paging */
	switch (vram_bank & 0x03)
	{
		/* page 0 is video ram */
		case 0:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, MRA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, MWA8_BANK4);
			memory_set_bank(1, 0);
			memory_set_bankptr(4, &williams_videoram[0x8000]);
			break;

		/* pages 1 and 2 are ROM */
		case 1:
		case 2:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, MRA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, MWA8_BANK4);
			memory_set_bank(1, 1 + ((vram_bank & 6) >> 1));
			memory_set_bankptr(4, &williams_videoram[0x8000]);
			break;

		/* page 3 accesses palette RAM; the remaining areas are as if page 1 ROM was selected */
		case 3:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, williams2_paletteram_r);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, williams2_paletteram_w);
			memory_set_bank(1, 1 + ((vram_bank & 4) >> 1));
			break;
	}
}



/*************************************
 *
 *  Sound commands
 *
 *************************************/

static void williams_deferred_snd_cmd_w(int param)
{
	pia_2_portb_w(0, param);
	pia_2_cb1_w(0, (param == 0xff) ? 0 : 1);
}

WRITE8_HANDLER( williams_snd_cmd_w )
{
	/* the high two bits are set externally, and should be 1 */
	timer_set(TIME_NOW, data | 0xc0, williams_deferred_snd_cmd_w);
}

WRITE8_HANDLER( playball_snd_cmd_w )
{
	timer_set(TIME_NOW, data, williams_deferred_snd_cmd_w);
}


static void williams2_deferred_snd_cmd_w(int param)
{
	pia_2_porta_w(0, param);
}

static WRITE8_HANDLER( williams2_snd_cmd_w )
{
	timer_set(TIME_NOW, data, williams2_deferred_snd_cmd_w);
}



/*************************************
 *
 *  General input port handlers
 *
 *************************************/

WRITE8_HANDLER( williams_port_select_w )
{
	port_select = data;
}


READ8_HANDLER( williams_input_port_0_3_r )
{
	return readinputport(port_select ? 3 : 0);
}


READ8_HANDLER( williams_input_port_1_4_r )
{
	return readinputport(port_select ? 4 : 1);
}


/*
 *  Williams 49-way joystick
 *
 *  The joystick works on a 7x7 grid system:
 *
 *      + + + | + + +
 *      + + + | + + +
 *      + + + | + + +
 *      ------+------
 *      + + + | + + +
 *      + + + | + + +
 *      + + + | + + +
 *
 *  Each axis has 7 positions, reported as follows
 *  in 4 bits/axis:
 *
 *      0000 = left/up full
 *      0100 = left/up 2/3
 *      0110 = left/up 1/3
 *      0111 = center
 *      1011 = right/down 1/3
 *      1001 = right/down 2/3
 *      1000 = right/down full
 */

READ8_HANDLER( williams_49way_port_0_r )
{
	static const UINT8 translate49[7] = { 0x0, 0x4, 0x6, 0x7, 0xb, 0x9, 0x8 };
	return (translate49[readinputportbytag("49WAYX") >> 4] << 4) | translate49[readinputportbytag("49WAYY") >> 4];
}


READ8_HANDLER( williams_input_port_49way_0_5_r )
{
	if (port_select)
		return williams_49way_port_0_r(0);
	else
		return readinputport(5);
}



/*************************************
 *
 *  CMOS access
 *
 *************************************/

WRITE8_HANDLER( williams_cmos_w )
{
	/* only 4 bits are valid */
	generic_nvram[offset] = data | 0xf0;
}


WRITE8_HANDLER( bubbles_cmos_w )
{
	/* bubbles has additional CMOS for a full 8 bits */
	generic_nvram[offset] = data;
}



/*************************************
 *
 *  Watchdog
 *
 *************************************/

WRITE8_HANDLER( williams_watchdog_reset_w )
{
	/* yes, the data bits are checked for this specific value */
	if (data == 0x39)
		watchdog_reset_w(0,0);
}


WRITE8_HANDLER( williams2_watchdog_reset_w )
{
	/* yes, the data bits are checked for this specific value */
	if ((data & 0x3f) == 0x14)
		watchdog_reset_w(0,0);
}



/*************************************
 *
 *  Diagnostic controls
 *
 *************************************/

WRITE8_HANDLER( williams2_7segment_w )
{
	int n;
	char dot;

	switch (data & 0x7F)
	{
		case 0x40:	n = 0; break;
		case 0x79:	n = 1; break;
		case 0x24:	n = 2; break;
		case 0x30:	n = 3; break;
		case 0x19:	n = 4; break;
		case 0x12:	n = 5; break;
		case 0x02:	n = 6; break;
		case 0x03:	n = 6; break;
		case 0x78:	n = 7; break;
		case 0x00:	n = 8; break;
		case 0x18:	n = 9; break;
		case 0x10:	n = 9; break;
		default:	n =-1; break;
	}

	if ((data & 0x80) == 0x00)
		dot = '.';
	else
		dot = ' ';

	if (n == -1)
		logerror("[ %c]\n", dot);
	else
		logerror("[%d%c]\n", n, dot);
}



/*************************************
 *
 *  Defender-specific routines
 *
 *************************************/

static void defender_postload(void)
{
	defender_bank_select_w(0, vram_bank);
}


MACHINE_RESET( defender )
{
	williams_common_init();

	/* configure the banking and make sure it is reset to 0 */
	memory_configure_bank(1, 0, 9, &memory_region(REGION_CPU1)[0x10000], 0x1000);
	defender_bank_select_w(0, 0);

	state_save_register_func_postload(defender_postload);
}


WRITE8_HANDLER( defender_video_control_w )
{
	williams_cocktail = data & 0x01;
}


WRITE8_HANDLER( defender_bank_select_w )
{
	vram_bank = data & 0x0f;

	/* set bank address */
	switch (data)
	{
		/* page 0 is I/O space */
		case 0:
			defender_install_io_space();
			break;

		/* pages 1-9 map to ROM banks */
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, MRA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, MWA8_UNMAP);
			memory_set_bank(1, vram_bank - 1);
			break;

		/* pages A-F are not connected */
		default:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, MRA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, MWA8_NOP);
			break;
	}
}



/*************************************
 *
 *  Mayday-specific routines
 *
 *************************************/

READ8_HANDLER( mayday_protection_r )
{
	/* Mayday does some kind of protection check that is not currently understood  */
	/* However, the results of that protection check are stored at $a190 and $a191 */
	/* These are compared against $a193 and $a194, respectively. Thus, to prevent  */
	/* the protection from resetting the machine, we just return $a193 for $a190,  */
	/* and $a194 for $a191. */
	return mayday_protection[offset + 3];
}



/*************************************
 *
 *  Sinistar-specific routines
 *
 *************************************/

WRITE8_HANDLER( sinistar_vram_select_w )
{
	/* low two bits are standard */
	williams_vram_select_w(offset, data);

	/* window enable from bit 2 (clips to 0x7400) */
	williams_blitter_window_enable = data & 0x04;
}



/*************************************
 *
 *  Blaster-specific routines
 *
 *************************************/

MACHINE_RESET( blaster )
{
	williams_common_init();

	/* banking is different for blaster */
	memory_configure_bank(1, 0, 1, williams_videoram, 0);
	memory_configure_bank(1, 1, 16, memory_region(REGION_CPU1) + 0x18000, 0x4000);

	memory_configure_bank(2, 0, 1, williams_videoram + 0x4000, 0);
	memory_configure_bank(2, 1, 16, memory_region(REGION_CPU1) + 0x10000, 0x0000);

	state_save_register_global(blaster_bank);
}


INLINE void update_blaster_banking(void)
{
	memory_set_bank(1, vram_bank * (blaster_bank + 1));
	memory_set_bank(2, vram_bank * (blaster_bank + 1));
}


WRITE8_HANDLER( blaster_vram_select_w )
{
	/* VRAM/ROM banking from bit 0 */
	vram_bank = data & 0x01;
	update_blaster_banking();

	/* cocktail flip from bit 1 */
	williams_cocktail = data & 0x02;

	/* window enable from bit 2 (clips to 0x9700) */
	williams_blitter_window_enable = data & 0x04;
}


WRITE8_HANDLER( blaster_bank_select_w )
{
	blaster_bank = data & 15;
	update_blaster_banking();
}



/*************************************
 *
 *  Lotto Fun-specific routines
 *
 *************************************/

static READ8_HANDLER( lottofun_input_port_0_r )
{
	/* merge in the ticket dispenser status */
	return input_port_0_r(offset) | ticket_dispenser_r(offset);
}



/*************************************
 *
 *  Turkey Shoot-specific routines
 *
 *************************************/

static READ8_HANDLER( tshoot_input_port_0_3_r )
{
	/* merge in the gun inputs with the standard data */
	int data = williams_input_port_0_3_r(offset);
	int gun = (data & 0x3f) ^ ((data & 0x3f) >> 1);
	return (data & 0xc0) | gun;
}


static WRITE8_HANDLER( tshoot_maxvol_w )
{
	/* something to do with the sound volume */
	logerror("tshoot maxvol = %d (pc:%x)\n", data, activecpu_get_pc());
}


static WRITE8_HANDLER( tshoot_lamp_w )
{
	/* set the grenade lamp */
	set_led_status(0,data & 0x04);

	/* set the gun lamp */
	set_led_status(1,data & 0x08);

#if 0
	/* gun coil */
	if (data & 0x10)
		printf("[gun coil] ");
	else
		printf("           ");

	/* feather coil */
	if (data & 0x20)
		printf("[feather coil] ");
	else
		printf("               ");

	printf("\n");
#endif
}



/*************************************
 *
 *  Joust 2-specific routines
 *
 *************************************/

MACHINE_START( joust2 )
{
	williams_cvsd_init(3);
	return 0;
}


MACHINE_RESET( joust2 )
{
	/* standard init */
	machine_reset_williams2();
	pia_set_input_ca1(3, 1);
	state_save_register_global(joust2_current_sound_data);
}


static void joust2_deferred_snd_cmd_w(int param)
{
	pia_2_porta_w(0, param & 0xff);
}


static WRITE8_HANDLER( joust2_pia_3_cb1_w )
{
	joust2_current_sound_data = (joust2_current_sound_data & ~0x100) | ((data << 8) & 0x100);
	pia_3_cb1_w(offset, data);
}


static WRITE8_HANDLER( joust2_snd_cmd_w )
{
	joust2_current_sound_data = (joust2_current_sound_data & ~0xff) | (data & 0xff);
	williams_cvsd_data_w(joust2_current_sound_data);
	timer_set(TIME_NOW, joust2_current_sound_data, joust2_deferred_snd_cmd_w);
}
