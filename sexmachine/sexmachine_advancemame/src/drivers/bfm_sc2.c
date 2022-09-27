/***************************************************************************

  Bellfruit scorpion2/3 driver, (under heavy construction !!!)

  07-03-2006: El Condor: Recoded to more accurately represent the hardware setup.
  18-01-2006: Cleaned up for MAME inclusion
  19-08-2005: Re-Animator

Standard scorpion2 memorymap


   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-----------------------------------------
0000-1FFF  |R/W| D D D D D D D D | RAM (8k) battery backed up
-----------+---+-----------------+-----------------------------------------
2000-20FF  | W | D D D D D D D D | Reel 1 + 2 stepper latch
-----------+---+-----------------+-----------------------------------------
2000       | R | D D D D D D D D | vfd status
-----------+---+-----------------+-----------------------------------------
2100-21FF  | W | D D D D D D D D | Reel 3 + 4 stepper latch
-----------+---+-----------------+-----------------------------------------
2200-22FF  | W | D D D D D D D D | Reel 5 + 6 stepper latch
-----------+---+-----------------+-----------------------------------------
2300-231F  | W | D D D D D D D D | output mux
-----------+---+-----------------+-----------------------------------------
2300-230B  | R | D D D D D D D D | input mux
-----------+---+-----------------+-----------------------------------------
2320       |R/W| D D D D D D D D | dimas0 ?
-----------+---+-----------------+-----------------------------------------
2321       |R/W| D D D D D D D D | dimas1 ?
-----------+---+-----------------+-----------------------------------------
2322       |R/W| D D D D D D D D | dimas2 ?
-----------+---+-----------------+-----------------------------------------
2323       |R/W| D D D D D D D D | dimas3 ?
-----------+---+-----------------+-----------------------------------------
2324       |R/W| D D D D D D D D | expansion latch
-----------+---+-----------------+-----------------------------------------
2325       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
2326       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
2327       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
2328       |R/W| D D D D D D D D | muxena
-----------+---+-----------------+-----------------------------------------
2329       | W | D D D D D D D D | Timer IRQ enable
-----------+---+-----------------+-----------------------------------------
232A       |R/W| D D D D D D D D | blkdiv ?
-----------+---+-----------------+-----------------------------------------
232B       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
232C       |R/W| D D D D D D D D | dimena ?
-----------+---+-----------------+-----------------------------------------
232D       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
232E       | R | D D D D D D D D | chip status b0 = IRQ status
-----------+---+-----------------+-----------------------------------------
232F       | W | D D D D D D D D | coin inhibits
-----------+---+-----------------+-----------------------------------------
2330       | W | D D D D D D D D | payout slide latch
-----------+---+-----------------+-----------------------------------------
2331       | W | D D D D D D D D | payout triac latch
-----------+---+-----------------+-----------------------------------------
2332       |R/W| D D D D D D D D | Watchdog timer
-----------+---+-----------------+-----------------------------------------
2333       | W | D D D D D D D D | electro mechanical meters
-----------+---+-----------------+-----------------------------------------
2334       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
2335       | ? | D D D D D D D D | ???
-----------+---+-----------------+-----------------------------------------
2336       |?/W| D D D D D D D D | dimcnt ?
-----------+---+-----------------+-----------------------------------------
2337       | W | D D D D D D D D | volume overide
-----------+---+-----------------+-----------------------------------------
2338       | W | D D D D D D D D | payout chip select
-----------+---+-----------------+-----------------------------------------
2339       | W | D D D D D D D D | clkden ?
-----------+---+-----------------+-----------------------------------------
2400       |R/W| D D D D D D D D | uart1 (MC6850 compatible) control/status register
-----------+---+-----------------+-----------------------------------------
2500       |R/W| D D D D D D D D | uart1 (MC6850 compatible) data register
-----------+---+-----------------+-----------------------------------------
2600       |R/W| D D D D D D D D | uart2 (MC6850 compatible) control/status register
-----------+---+-----------------+-----------------------------------------
2700       |R/W| D D D D D D D D | uart2 (MC6850 compatible) data register
-----------+---+-----------------+-----------------------------------------
2800       |R/W| D D D D D D D D | vfd1
-----------+---+-----------------+-----------------------------------------
2900       |R/W| D D D D D D D D | reset vfd1 + vfd2
-----------+---+-----------------+-----------------------------------------
2D00       |R/W| D D D D D D D D | ym2413 control
-----------+---+-----------------+-----------------------------------------
2D01       |R/W| D D D D D D D D | ym2413 data
-----------+---+-----------------+-----------------------------------------
2E00       |R/W| D D D D D D D D | ROM page latch
-----------+---+-----------------+-----------------------------------------
2F00       |R/W| D D D D D D D D | vfd2
-----------+---+-----------------+-----------------------------------------
3FFE       | R | D D D D D D D D | direct input1
-----------+---+-----------------+-----------------------------------------
3FFF       | R | D D D D D D D D | direct input2
-----------+---+-----------------+-----------------------------------------
2A00       | W | D D D D D D D D | NEC uPD7759 data
-----------+---+-----------------+-----------------------------------------
2B00       | W | D D D D D D D D | NEC uPD7759 reset
-----------+---+-----------------+-----------------------------------------
4000-5FFF  | R | D D D D D D D D | ROM (8k)
-----------+---+-----------------+-----------------------------------------
6000-7FFF  | R | D D D D D D D D | Paged ROM (8k)
           |   |                 |   page 0 : rom area 0x0000 - 0x1FFF
           |   |                 |   page 1 : rom area 0x2000 - 0x3FFF
           |   |                 |   page 2 : rom area 0x4000 - 0x5FFF
           |   |                 |   page 3 : rom area 0x6000 - 0x7FFF
-----------+---+-----------------+-----------------------------------------
8000-FFFF  | R | D D D D D D D D | ROM (32k)
-----------+---+-----------------+-----------------------------------------

Adder hardware:
    Games supported:
        * Quintoon (2 sets Dutch, 1 set UK)
        * Quintoon (UK) (1 set)
        * Pokio (1 set)
        * Paradice (1 set)
        * Pyramid (1 set)
        * Slots (1 set Dutch, 2 sets Belgian)
        * Golden Crown (1 Set)

    Known issues:
        * Need to find the 'missing' game numbers
        * To determine whether or not the VFD and door status are drawn on screen, change the
          comment status of the FAKE_VIDEO definition in bfm_adr2.c

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

#include "vidhrdw/bfm_adr2.h"

#include "sound/2413intf.h"
#include "sound/upd7759.h"

/* fruit machines only */
//#include "vidhrdw/bfm_dm01.h"
//#include "vidhrdw/genfmvid.h"
//#include "vidhrdw/dxfmvid.h"

#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"
#include "bfm_sc2.h"

//#define LOG_SERIAL      // log serial communication between mainboard (scorpion2) and videoboard (adder2)
#define  BFM_DEBUG //enable debug options on certain games
//#define  UART_LOG //enable UART data logging

// local prototypes ///////////////////////////////////////////////////////

static int  get_scorpion2_uart_status(void);	// retrieve status of uart on scorpion2 board

static int  read_e2ram(void);
static void e2ram_reset(void);

// global vars ////////////////////////////////////////////////////////////

static int sc2gui_update_mmtr;	// bit pattern which mechanical meter needs updating

// local vars /////////////////////////////////////////////////////////////

static UINT8 *nvram;	  // pointer to NV ram
static size_t		  nvram_size; // size of NV ram
static UINT8 key[16];	// security device on gamecard (video games only)

static UINT8 e2ram[1024]; // x24C08 e2ram

static int mmtr_latch;		  // mechanical meter latch
static int triac_latch;		  // payslide triac latch
static int vfd1_latch;		  // vfd1 latch
static int vfd2_latch;		  // vfd2 latch
static int irq_status;		  // custom chip IRQ status
static int optic_pattern;     // reel optics
static int uart1_status;	  // MC6850 status
static int uart2_status;	  // MC6850 status
static int locked;			  // hardware lock/unlock status (0=unlocked)
static int timer_enabled;
static int reel_changed;
static int coin_inhibits;
static int irq_timer_stat;
static int expansion_latch;
static int global_volume;	  // 0-31
static int volume_override;	  // 0 / 1

int adder2_data_from_sc2;	// data available for adder from sc2
int adder2_sc2data;			// data
int sc2_data_from_adder;	// data available for sc2 from adder
int sc2_adderdata;			// data

int sc2_show_door;			// flag <>0, show door state
int sc2_door_state;			// door switch strobe/data

static int reel12_latch;
static int reel34_latch;
static int reel56_latch;
static int pay_latch;

static int slide_states[6];
static int slide_pay_sensor[6];

static int has_hopper;		  // flag <>0, scorpion2 board has hopper connected

static int triac_select;

static int hopper_running;		// flag <>0, hopper is running used in some scorpion2 videogames
static int hopper_coin_sense;	// hopper coin sense state
static int timercnt;			// timer counts up every IRQ (=1000 times a second)

static int watchdog_cnt;
static int watchdog_kicked;

// user interface stuff ///////////////////////////////////////////////////

static UINT8 Lamps[256];		  // 256 multiplexed lamps
static UINT8 sc2_Inputs[64];			  // ??  multiplexed inputs,
								  // need to be hooked to buttons

static UINT8 input_override[64];  // bit pattern, bit set means this input is overriden and cannot be changed with switches

/*      INPUTS layout

     b7 b6 b5 b4 b3 b2 b1 b0

     82 81 80 04 03 02 01 00  0
     92 91 90 14 13 12 11 10  1
     A2 A1 A0 24 23 22 21 20  2
     B2 B1 B0 34 33 32 31 30  3
     -- 84 83 44 43 42 41 40  4
     -- 94 93 54 53 52 51 50  5
     -- A4 A3 64 63 62 61 60  6
     -- B4 B3 74 73 72 71 70  7

     B7 B6 B5 B4 B3 B2 B1 B0
      0  1  1  0  0  0

*/
///////////////////////////////////////////////////////////////////////////

void send_to_adder(int data)
{
	adder2_data_from_sc2 = 1;		// set flag, data from scorpion2 board available
	adder2_sc2data       = data;	// store data

	adder_acia_triggered = 1;		// set flag, acia IRQ triggered
	cpunum_set_input_line(1, M6809_IRQ_LINE, HOLD_LINE );	// trigger IRQ

	#ifdef LOG_SERIAL
	logerror("  %02X  (%c)\n",data, data );
	#endif
}

///////////////////////////////////////////////////////////////////////////

int read_from_sc2(void)
{
	int data = adder2_sc2data;

	adder2_data_from_sc2 = 0;	  // clr flag, data from scorpion2 board available

	#ifdef LOG_SERIAL
	logerror("r:%02X  (%c)\n",data, data );
	#endif

	return data;
}

///////////////////////////////////////////////////////////////////////////

int receive_from_adder(void)
{
	int data = sc2_adderdata;

	sc2_data_from_adder = 0;	  // clr flag, data from adder available

	#ifdef LOG_SERIAL
	logerror("r:  %02X(%c)\n",data, data );
	#endif

	return data;
}

///////////////////////////////////////////////////////////////////////////

void send_to_sc2(int data)
{
	sc2_data_from_adder = 1;		// set flag, data from adder available
	sc2_adderdata       = data;	// store data

	#ifdef LOG_SERIAL
	logerror("    %02X(%c)\n",data, data );
	#endif
}

///////////////////////////////////////////////////////////////////////////

int get_adder2_uart_status(void)
{
	int status = 0;

	if ( adder2_data_from_sc2 ) status |= 0x01; // receive  buffer full
	if ( !sc2_data_from_adder ) status |= 0x02; // transmit buffer empty

	return status;
}

///////////////////////////////////////////////////////////////////////////

static int get_scorpion2_uart_status(void)
{
	int status = 0;

	if ( sc2_data_from_adder  ) status |= 0x01; // receive  buffer full
	if ( !adder2_data_from_sc2) status |= 0x02; // transmit buffer empty

	return status;
}

///////////////////////////////////////////////////////////////////////////
// called if board is reset ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void on_scorpion2_reset(void)
{
	vfd1_latch        = 0;
	vfd2_latch        = 0;
	mmtr_latch        = 0;
	triac_latch       = 0;
	irq_status        = 0;
	timer_enabled     = 1;
	coin_inhibits     = 0;
	irq_timer_stat    = 0;
	expansion_latch   = 0;
	global_volume     = 0;
	volume_override   = 0;
	triac_select      = 0;
	pay_latch         = 0;

	reel12_latch      = 0;
	reel34_latch      = 0;
	reel56_latch      = 0;

	hopper_running	= 0;  // for video games
	hopper_coin_sense = 0;
	sc2gui_update_mmtr= 0xFF;

	slide_states[0] = 0;
	slide_states[1] = 0;
	slide_states[2] = 0;
	slide_states[3] = 0;
	slide_states[4] = 0;
	slide_states[5] = 0;

	watchdog_cnt     = 0;
	watchdog_kicked  = 0;


	vfd_reset(0);	// reset display1
	vfd_reset(1);	// reset display2

	e2ram_reset();

	sndti_reset(SOUND_YM2413, 0);

  // reset stepper motors /////////////////////////////////////////////////
	{
		int pattern =0, i;

		for ( i = 0; i < 6; i++)
		{
			Stepper_reset_position(i);
			if ( Stepper_optic_state(i) ) pattern |= 1<<i;
		}

		optic_pattern = pattern;

	}

	uart1_status  = 0x02; // MC6850 transmit buffer empty !!!
	uart2_status  = 0x02; // MC6850 transmit buffer empty !!!
	locked		= 0;	// hardware is NOT locked

	// make sure no inputs are overidden ////////////////////////////////////
	memset(input_override, 0, sizeof(input_override));

	// init rom bank ////////////////////////////////////////////////////////

	{
		UINT8 *rom = memory_region(REGION_CPU1);

		memory_configure_bank(1, 0, 1, &rom[0x10000], 0);
		memory_configure_bank(1, 1, 3, &rom[0x02000], 0x02000);

		memory_set_bank(1,3);
	}
}

///////////////////////////////////////////////////////////////////////////

extern void Scorpion2_SetSwitchState(int strobe, int data, int state)
{
	//logerror("setstate(%0x:%0x, %d) ", strobe, data, state);
	if ( strobe < 11 && data < 8 )
	{
		if ( strobe < 8 )
		{
			input_override[strobe] |= (1<<data);

			if ( state ) sc2_Inputs[strobe] |=  (1<<data);
			else		   sc2_Inputs[strobe] &= ~(1<<data);

			 //logerror("override[%02x] = %02x %02x\n", strobe, input_override[strobe], sc2_Inputs[strobe]);
		}
		else
		{
			if ( data > 2 )
			{
				input_override[strobe-8+4] |= (1<<(data+2));

				if ( state ) sc2_Inputs[strobe-8+4] |=  (1<<(data+2));
				else		 sc2_Inputs[strobe-8+4] &= ~(1<<(data+2));

				//logerror("override[%02x] = %02x %02x\n", strobe-8+4, input_override[strobe-8+4], sc2_Inputs[strobe-8+4]);
			}
			else
			{
				input_override[strobe-8] |= (1<<(data+5));

				if ( state ) sc2_Inputs[strobe-8] |=  (1 << (data+5));
				else		 sc2_Inputs[strobe-8] &= ~(1 << (data+5));

				//logerror("override[%02x] = %02x %02x\n", strobe-8, input_override[strobe-8], sc2_Inputs[strobe-8]);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////

int Scorpion2_GetSwitchState(int strobe, int data)
{
	int state = 0;

	if ( strobe < 11 && data < 8 )
	{
		if ( strobe < 8 )
		{
			state = (sc2_Inputs[strobe] & (1<<data) ) ? 1 : 0;
		}
		else
		{
			if ( data > 2 )
			{
				state = (sc2_Inputs[strobe-8+4] & (1<<(data+2)) ) ? 1 : 0;
			}
			else
			{
				state = (sc2_Inputs[strobe-8] & (1 << (data+5)) ) ? 1 : 0;
			}
		}
	}
	return state;
}

///////////////////////////////////////////////////////////////////////////

static NVRAM_HANDLER( nvram )
{
	if ( read_or_write )
	{	// writing
		mame_fwrite(file,nvram,nvram_size);
		mame_fwrite(file,e2ram,sizeof(e2ram));
	}
	else
	{ // reading
		if ( file )
		{
			mame_fread(file,nvram,nvram_size);
			mame_fread(file,e2ram,sizeof(e2ram));
		}
		else
		{
			memset(nvram,0x00,nvram_size);
			memset(e2ram,0x00,sizeof(e2ram));
			e2ram[ 0] =  1;
			e2ram[ 1] =  4;
			e2ram[ 2] = 10;
			e2ram[ 3] = 20;
			e2ram[ 4] =  0;
			e2ram[ 5] =  1;
			e2ram[ 6] =  1;
			e2ram[ 7] =  4;
			e2ram[ 8] = 10;
			e2ram[ 9] = 20;
			e2ram[10] =  0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( ram_r )
{
	return nvram[offset];	// read from RAM
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( ram_w )
{
	nvram[offset] = data;	  // write to RAM
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( watchdog_w )
{
	watchdog_kicked = 1;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1,data & 0x03);
}

///////////////////////////////////////////////////////////////////////////

static INTERRUPT_GEN( timer_irq )
{
	timercnt++;


	if ( (timercnt & 0x03) == 0 ) adder2_show_alpha_display = readinputportbytag_safe("DEBUGPORT",13) & 0x01;  // debug port

	if ( watchdog_kicked )
	{
		watchdog_cnt    = 0;
		watchdog_kicked = 0;
	}
	else
	{
		watchdog_cnt++;
		if ( watchdog_cnt > 2 )	// this is a hack, i don't know what the watchdog timeout is, 3 IRQ's works fine
		{  // reset board
			mame_schedule_soft_reset();		// reset entire machine. CPU 0 should be enough, but that doesn't seem to work !!
			on_scorpion2_reset();
			return;
		}
	}

	if ( timer_enabled )
	{
		irq_timer_stat = 0x01;
		irq_status     = 0x02;

		cpunum_set_input_line(0, M6809_IRQ_LINE, HOLD_LINE );
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel12_w )
{
	reel12_latch = data;

	if ( Stepper_update(0, data   ) ) reel_changed |= 0x01;
	if ( Stepper_update(1, data>>4) ) reel_changed |= 0x02;

	if ( Stepper_optic_state(0) ) optic_pattern |=  0x01;
	else                          optic_pattern &= ~0x01;
	if ( Stepper_optic_state(1) ) optic_pattern |=  0x02;
	else                          optic_pattern &= ~0x02;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel12_vid_w )  // in a video cabinet this is used to drive a hopper
{
	reel12_latch = data;

	if ( has_hopper )
	{
		int oldhop = hopper_running;

		if ( data & 0x01 )
		{ // hopper power
			if ( data & 0x02 )
			{
				hopper_running    = 1;
			}
			else
			{
				hopper_running    = 0;
			}
		}
		else
		{
			//hopper_coin_sense = 0;
			hopper_running    = 0;
		}

		if ( oldhop != hopper_running )
		{
			hopper_coin_sense = 0;
			oldhop = hopper_running;
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel34_w )
{
	reel34_latch = data;

	if ( Stepper_update(2, data   ) ) reel_changed |= 0x04;
	if ( Stepper_update(3, data>>4) ) reel_changed |= 0x08;

	if ( Stepper_optic_state(2) ) optic_pattern |=  0x04;
	else                          optic_pattern &= ~0x04;
	if ( Stepper_optic_state(3) ) optic_pattern |=  0x08;
	else                          optic_pattern &= ~0x08;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( reel56_w )
{
	reel56_latch = data;

	if ( Stepper_update(4, data   ) ) reel_changed |= 0x10;
	if ( Stepper_update(5, data>>4) ) reel_changed |= 0x20;

	if ( Stepper_optic_state(4) ) optic_pattern |=  0x10;
	else                          optic_pattern &= ~0x10;
	if ( Stepper_optic_state(5) ) optic_pattern |=  0x20;
	else                          optic_pattern &= ~0x20;
}

///////////////////////////////////////////////////////////////////////////
// mechanical meters //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mmtr_w )
{
	int  changed = mmtr_latch ^ data;
	long cycles  = MAME_TIME_TO_CYCLES(0, mame_timer_get_time() );

	mmtr_latch = data;

	if ( changed & 0x01 )
	{
		if ( Mechmtr_update(0, cycles, data & 0x01 ) )
		{
			sc2gui_update_mmtr |= 0x01;
			logerror("meter %d = %ld\n", 1, MechMtr_Getcount(0) );
		}
	}

	if ( changed & 0x02 )
	{
		if ( Mechmtr_update(1, cycles, data & 0x02 ) )
		{
			sc2gui_update_mmtr |= 0x02;
			logerror("meter %d = %ld\n", 2, MechMtr_Getcount(1) );
		}
	}

	if ( changed & 0x04 )
	{
		if ( Mechmtr_update(2, cycles, data & 0x04 ) )
		{
			sc2gui_update_mmtr |= 0x04;
			logerror("meter %d = %ld\n", 3, MechMtr_Getcount(2) );
		}
	}

	if ( changed & 0x08 )
	{
		if ( Mechmtr_update(3, cycles, data & 0x08 ) )
		{
			sc2gui_update_mmtr |= 0x08;
			logerror("meter %d = %ld\n", 4, MechMtr_Getcount(3) );
		}
	}


	if ( changed & 0x10 )
	{
		if ( Mechmtr_update(4, cycles, data & 0x10 ) )
		{
			sc2gui_update_mmtr |= 0x10;
			logerror("meter %d = %ld\n", 5, MechMtr_Getcount(4) );
		}
	}

	if ( changed & 0x20 )
	{
		if ( Mechmtr_update(5, cycles, data & 0x20 ) )
		{
			sc2gui_update_mmtr |= 0x20;
			logerror("meter %d = %ld\n", 6, MechMtr_Getcount(5) );
		}
	}

	if ( changed & 0x40 )
	{
		if ( Mechmtr_update(6, cycles, data & 0x40 ) )
		{
			sc2gui_update_mmtr |= 0x40;
			logerror("meter %d = %ld\n", 7, MechMtr_Getcount(6) );
		}
	}

	if ( changed & 0x80 )
	{
		if ( Mechmtr_update(7, cycles, data & 0x80 ) )
		{
			sc2gui_update_mmtr |= 0x80;
			logerror("meter %d = %ld\n", 8, MechMtr_Getcount(7) );
		}
	}

	if ( data & 0x1F ) cpunum_set_input_line(0, M6809_FIRQ_LINE, PULSE_LINE );
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( mux_output_w )
{
	int off = offset<<3;

	Lamps[ off   ] = (data & 0x01) != 0;
	Lamps[ off+1 ] = (data & 0x02) != 0;
	Lamps[ off+2 ] = (data & 0x04) != 0;
	Lamps[ off+3 ] = (data & 0x08) != 0;
	Lamps[ off+4 ] = (data & 0x10) != 0;
	Lamps[ off+5 ] = (data & 0x20) != 0;
	Lamps[ off+6 ] = (data & 0x40) != 0;
	Lamps[ off+7 ] = (data & 0x80) != 0;

	if ( offset == 0 ) Lamps_SetBrightness(0, 255, Lamps); // update all lamps afer strobe 0 has been updated
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( mux_input_r )
{
	int result = 0xFF,t1,t2;

	switch ( offset )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			t1 = input_override[offset];	  // strobe 0-3 data 0-4
			t2 = input_override[offset+8];  // strobe 8-B data 0-2

			t1 = (sc2_Inputs[offset]   & t1) | ( ( readinputport(offset+1)   & ~t1) & 0x1F);
			t2 = (sc2_Inputs[offset+8] & t2) | ( ( readinputport(offset+1+8) & ~t2) << 5);

			sc2_Inputs[offset]   = (sc2_Inputs[offset]   & ~0x1F) | t1;
			sc2_Inputs[offset+8] = (sc2_Inputs[offset+8] & ~0x60) | t2;
			result = t1 | t2;
			break;

		case 4:
		case 5:
		case 6:
		case 7:
			t1 = input_override[offset];	// strobe 4-7 data 0-4
			t2 = input_override[offset+4];// strobe 8-B data 3-4

			t1 =  (sc2_Inputs[offset]   & t1) | ( ( readinputport(offset+1)     & ~t1) & 0x1F);
			t2 =  (sc2_Inputs[offset+4] & t2) | ( ( ( readinputport(offset+1+4) & ~t2) << 2) & 0x60);

			sc2_Inputs[offset]   = (sc2_Inputs[offset]   & ~0x1F) | t1;
			sc2_Inputs[offset+4] = (sc2_Inputs[offset+4] & ~0x60) | t2;
			result =  t1 | t2;
			break;

		default:
			logerror("read strobe %d\n", offset);
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( unlock_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( dimas_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( dimcnt_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( unknown_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( volume_override_w )
{
	int old = volume_override;

	volume_override = data?1:0;

	if ( old != volume_override )
	{
		float percent = volume_override?1.0:(32-global_volume)/32.0;

		sndti_set_output_gain(SOUND_YM2413,  0, 0, percent);
		sndti_set_output_gain(SOUND_YM2413,  0, 1, percent);
		sndti_set_output_gain(SOUND_UPD7759, 0, 0, percent);
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( nec_reset_w )
{
	upd7759_start_w(0, 0);
	upd7759_reset_w(0, data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( nec_latch_w )
{
	int bank = 0;

	if ( data & 0x80 )         bank |= 0x01;
	if ( expansion_latch & 2 ) bank |= 0x02;

	//logerror("start sound %d bank %d\n", data, bank);
	upd7759_set_bank_base(0, bank*0x20000);

	upd7759_port_w(0, data&0x3F);	// setup sample
	upd7759_start_w(0, 0);
	upd7759_start_w(0, 1);			// start
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vfd_status_r )
{
	// b7 = NEC busy
	// b6 = alpha busy (also matrix board)
	// b5 - b0 = reel optics

	int result = optic_pattern;

	if ( !upd7759_busy_r(0) ) result |= 0x80;

	return result;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vfd_status_hop_r )	// on video games, hopper inputs are connected to this
{
	// b7 = NEC busy
	// b6 = alpha busy (also matrix board)
	// b5 - b0 = reel optics

	int result = 0;

	if ( has_hopper )
	{
		result |= 0x04; // hopper high level
		result |= 0x08; // hopper low  level

		result |= 0x01|0x02;

		if ( hopper_running )
		{
			result &= ~0x01;								  // set motor running input

			if ( timercnt & 0x04 ) hopper_coin_sense ^= 1;	  // toggle coin seen

			if ( hopper_coin_sense ) result &= ~0x02;		  // update coin seen input
		}
	}

	if ( !upd7759_busy_r(0) ) result |= 0x80;			  // update sound busy input

	return result;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( expansion_latch_w )
{
	int changed = expansion_latch^data;

	expansion_latch = data;

	// bit0,  1 = lamp mux disabled, 0 = lamp mux enabled
	// bit1,  ? used in Del's millions
	// bit2,  digital volume pot meter, clock line
	// bit3,  digital volume pot meter, direction line
	// bit4,  ?
	// bit5,  ?
	// bit6,  ? used in Del's millions
	// bit7   ?

	if ( changed & 0x04)
	{ // digital volume clock line changed
		if ( !(data & 0x04) )
		{ // changed from high to low,
			if ( !(data & 0x08) )
			{
				if ( global_volume < 31 ) global_volume++; //0-31 expressed as 1-32
			}
			else
			{
				if ( global_volume > 0  ) global_volume--;
			}

			{
				float percent = volume_override?1.0:(32-global_volume)/32.0;

				sndti_set_output_gain(SOUND_YM2413,  0, 0, percent);
				sndti_set_output_gain(SOUND_YM2413,  0, 1, percent);
				sndti_set_output_gain(SOUND_UPD7759, 0, 0, percent);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( expansion_latch_r )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( muxena_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( timerirq_w )
{
	timer_enabled = data & 1;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( timerirqclr_r )
{
	irq_timer_stat = 0;
	irq_status     = 0;

	return 0;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( irqstatus_r )
{
	int result = irq_status | irq_timer_stat | 0x80;	// 0x80 = ~MUXERROR

	irq_timer_stat = 0;

	return result;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( coininhib_w )
{
	int changed = coin_inhibits^data,i,p;

	coin_inhibits = data;

	p = 0x01;
	i = 0;

	while ( i < 8 && changed )
	{
		if ( changed & p )
		{ // this inhibit line has changed
			coin_lockout_w(i, (~data & p) ); // update lockouts

			//logerror("lockout %d, %s\n", i, data&p?"OFF":"ON");

			changed &= ~p;
		}

		p <<= 1;
		i++;
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( direct_input_r )
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( coin_input_r )
{
	return input_port_0_r(0);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( payout_latch_w )
{
	pay_latch = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( payout_triac_w )
{
	if ( triac_select == 0x57 )
	{
		int slide = 0;

		switch ( pay_latch )
		{
			case 0x01: slide = 1;
				break;

			case 0x02: slide = 2;
				break;

			case 0x04: slide = 3;
				break;

			case 0x08: slide = 4;
				break;

			case 0x10: slide = 5;
				break;

			case 0x20: slide = 6;
				break;
		}

		if ( slide )
		{
			if ( data == 0x4D )
			{
				if ( !slide_states[slide] )
				{
					if ( slide_pay_sensor[slide] )
					{
						int strobe = slide_pay_sensor[slide]>>4, data = slide_pay_sensor[slide]&0x0F;

						Scorpion2_SetSwitchState(strobe, data, 0);
					}
					slide_states[slide] = 1;
				}
			}
			else
			{
				if ( slide_states[slide] )
				{
					if ( slide_pay_sensor[slide] )
					{
						int strobe = slide_pay_sensor[slide]>>4, data = slide_pay_sensor[slide]&0x0F;

						Scorpion2_SetSwitchState(strobe, data, 1);
					}
					slide_states[slide] = 0;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( payout_select_w )
{
	triac_select = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vfd1_data_w )
{
	vfd1_latch = data;
	vfd_newdata(0, data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vfd2_data_w )
{
	vfd2_latch = data;
	vfd_newdata(1, data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vfd_reset_w )
{
	vfd_reset(0);	  // reset both VFD's
	vfd_reset(1);
}

///////////////////////////////////////////////////////////////////////////
// serial port ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( uart1stat_r )
{
	return uart1_status;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( uart1data_r )
{
	return 0x06; //for now
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( uart1ctrl_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( uart1data_w )
{
	#ifdef UART_LOG
	logerror("uart1:%c\n", data);
	#endif
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( uart2stat_r )
{
	return uart2_status;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( uart2data_r )
{
	return 0x06;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( uart2ctrl_w )
{
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( uart2data_w )
{
	#ifdef UART_LOG
	logerror("uart2:%c\n", data);
	#endif
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vid_uart_tx_w )
{
	send_to_adder(data);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( vid_uart_ctrl_w )
{
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vid_uart_rx_r )
{
	int data = receive_from_adder();

	return data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( vid_uart_ctrl_r )
{
	return get_scorpion2_uart_status();
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( key_r )
{
	int result = key[ offset ];

	if ( offset == 7 )
	{
		result = (result & 0xFE) | read_e2ram();
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
/*

The X24C08 is a CMOS 8,192 bit serial EEPROM,
internally organized 1024 x 8. The X24C08 features a
serial interface and software protocol allowing operation
on a simple two wire bus.

*/


static int e2reg;
static int e2state;
static int e2cnt;
static int e2data;
static int e2address;
static int e2rw;
static int e2data_pin;
static int e2dummywrite;

static int e2data_to_read;

#define SCL 0x01	//SCL pin (clock)
#define SDA	0x02	//SDA pin (data)


static void e2ram_reset(void)
{
	e2reg   = 0;
	e2state = 0;
	e2address = 0;
	e2rw    = 0;
	e2data_pin = 0;
	e2data  = (SDA|SCL);
	e2dummywrite = 0;
	e2data_to_read = 0;
}

static int recdata(int changed, int data)
{
	int res = 1;

	if ( e2cnt < 8 )
	{
		res = 0;

		if ( (changed & SCL) && (data & SCL) )
		{ // clocked in new data
			int pattern = 1 << (7-e2cnt);

			if ( data & SDA ) e2data |=  pattern;
			else              e2data &= ~pattern;

			e2data_pin = e2data_to_read & 0x80 ? 1 : 0;

			e2data_to_read <<= 1;

			logerror("  e2d pin= %d\n", e2data_pin);

			e2cnt++;
			if ( e2cnt >= 8 )
			{
				res++;
			}
		}
	}

	return res;
}

static int recAck(int changed, int data)
{
	int result = 0;

	if ( (changed & SCL) && (data & SCL) )
	{
		if ( data & SDA )
		{
			result = 1;
		}
		else
		{
			result = -1;
		}
	}
	return result;
}

//
static WRITE8_HANDLER( e2ram_w )
{ // b0 = clock b1 = data

	int changed, ack;

	data ^= (SDA|SCL);  // invert signals

	changed  = (e2reg^data) & 0x03;

	e2reg = data;

	// logerror("e2: D:%d C:%d [%d]:%d\n", data&SDA?1:0, data&SCL?1:0, e2state, e2cnt );

	if ( changed )
	{
		while ( 1 )
		{
			if ( (  (changed & SDA) && !(data & SDA))	&&  // 1->0 on SDA  AND
				( !(changed & SCL) && (data & SCL) )    // SCL=1 and not changed
				)
			{	// X24C08 Start condition (1->0 on SDA while SCL=1)
				e2dummywrite = ( e2state == 5 );

				logerror("e2ram:   c:%d d:%d Start condition dummywrite=%d\n", (data & SCL)?1:0, (data&SDA)?1:0, e2dummywrite );

				e2state = 1; // ready for commands
				e2cnt   = 0;
				e2data  = 0;
				break;
			}

			if ( (  (changed & SDA) && (data & SDA))	&&  // 0->1 on SDA  AND
				( !(changed & SCL) && (data & SCL) )     // SCL=1 and not changed
				)
			{	// X24C08 Stop condition (0->1 on SDA while SCL=1)
				logerror("e2ram:   c:%d d:%d Stop condition\n", (data & SCL)?1:0, (data&SDA)?1:0 );
				e2state = 0;
				e2data  = 0;
				break;
			}

			switch ( e2state )
			{
				case 1: // Receiving address + R/W bit

					if ( recdata(changed, data) )
					{
						e2address = (e2address & 0x00FF) | ((e2data>>1) & 0x03) << 8;
						e2cnt   = 0;
						e2rw    = e2data & 1;

						logerror("e2ram: Slave address received !!  device id=%01X device adr=%01d high order adr %0X RW=%d) %02X\n",
							e2data>>4, (e2data & 0x08)?1:0, (e2data>>1) & 0x03, e2rw , e2data );

						e2state = 2;
					}
					break;

				case 2: // Receive Acknowledge

					ack = recAck(changed,data);
					if ( ack )
					{
						e2data_pin = 0;

						if ( ack < 0 )
						{
							logerror(" ACK = 0\n");
							e2state = 0;
						}
						else
						{
							logerror(" ACK = 1\n");
							if ( e2dummywrite )
							{
								e2dummywrite = 0;

								e2data_to_read = e2ram[e2address];

								if ( e2rw & 1 ) e2state = 7; // read data
								else  		  e2state = 0; //?not sure
							}
							else
							{
								if ( e2rw & 1 ) e2state = 7; // reading
								else            e2state = 3; // writing
							}
							switch ( e2state )
							{
								case 7:
									logerror(" read address %04X\n",e2address);
									e2data_to_read = e2ram[e2address];
									break;
								case 3:
									logerror(" write, awaiting address\n");
									break;
								default:
									logerror(" ?unknow action %04X\n",e2address);
									break;
							}
						}
						e2data = 0;
					}
					break;

				case 3: // writing data, receiving address

					if ( recdata(changed, data) )
					{
						e2data_pin = 0;
						e2address = (e2address & 0xFF00) | e2data;

						logerror("  write address = %04X waiting for ACK\n", e2address);
						e2state = 4;
						e2cnt   = 0;
						e2data  = 0;
					}
					break;

				case 4: // wait ack, for write address

					ack = recAck(changed,data);
					if ( ack )
					{
						e2data_pin = 0;	// pin=0, no error !!

						if ( ack < 0 )
						{
							e2state = 0;
							logerror("  ACK = 0, cancel write\n" );
						}
						else
						{
							e2state = 5;
							logerror("  ACK = 1, awaiting data to write\n" );
						}
					}
					break;

				case 5: // receive data to write
					if ( recdata(changed, data) )
					{
						logerror("  write data = %02X received, awaiting ACK\n", e2data);
						e2cnt   = 0;
						e2state = 6;  // wait ack
					}
					break;

				case 6: // Receive Acknowlede after writing

					ack = recAck(changed,data);
					if ( ack )
					{
						if ( ack < 0 )
						{
							e2state = 0;
							logerror("  ACK=0, write canceled\n");
						}
						else
						{
							logerror("  ACK=1, writeing %02X to %04X\n", e2data, e2address);

							e2ram[e2address] = e2data;

							e2address = (e2address & ~0x000F) | ((e2address+1)&0x0F);

							e2state = 5; // write next address
						}
					}
					break;

				case 7: // receive address from read

					if ( recdata(changed, data) )
					{
						//e2data_pin = 0;

						logerror("  address read, data = %02X waiting for ACK\n", e2data );

						e2state = 8;
					}
					break;

				case 8:

					if ( recAck(changed, data) )
					{
						e2state = 7;

						e2address = (e2address & ~0x0F) | ((e2address+1)&0x0F); // lower 4 bits wrap around

						e2data_to_read = e2ram[e2address];

						logerror("  ready for next address %04X\n", e2address);

						e2cnt   = 0;
						e2data  = 0;
					}
					break;

				case 0:

					logerror("e2ram: ? c:%d d:%d\n", (data & SCL)?1:0, (data&SDA)?1:0 );
					break;
				}
			break;
		}
	}
}

static int read_e2ram(void)
{
	logerror("e2ram: r %d (%02X) \n", e2data_pin, e2data_to_read );

	return e2data_pin;
}


static const unsigned short AddressDecode[]=
{
	0x0800,0x1000,0x0001,0x0004,0x0008,0x0020,0x0080,0x0200,
	0x0100,0x0040,0x0002,0x0010,0x0400,0x2000,0x4000,0x8000,

	0
};

static const UINT8 DataDecode[]=
{
	0x02,0x08,0x20,0x40,0x10,0x04,0x01,0x80,

	0
};

static UINT8 codec_data[256];

///////////////////////////////////////////////////////////////////////////
static void decode_mainrom(int rom_region)
{
	UINT8 *tmp, *rom;

	rom = memory_region(rom_region);

	tmp = malloc(0x10000);

	if ( tmp )
	{
		int i;
		long address;

		memcpy(tmp, rom, 0x10000);

		for ( i = 0; i < 256; i++ )
		{
			UINT8 data,pattern,newdata,*tab;
			data    = i;

			tab     = (UINT8*)DataDecode;
			pattern = 0x01;
			newdata = 0;

			do
			{
				newdata |= data & pattern ? *tab : 0;
				pattern <<= 1;
			} while ( *(++tab) );

			codec_data[i] = newdata;
		}

		for ( address = 0; address < 0x10000; address++)
		{
			int	newaddress,pattern;
			unsigned short *tab;

			tab      = (unsigned short*)AddressDecode;
			pattern  = 0x0001;
			newaddress = 0;
			do
			{
				newaddress |= address & pattern ? *tab : 0;
				pattern <<= 1;
			} while ( *(++tab) );

			rom[newaddress] = codec_data[ tmp[address] ];
		}
		free(tmp);
	}
}

// machine init (called only once) ////////////////////////////////////////

static MACHINE_RESET( init )
{
	// reset the board //////////////////////////////////////////////////////

	on_scorpion2_reset();
	//BFM_dm01_reset(); No known video based game has a Matrix board
}

// memory map for scorpion2 board video addon /////////////////////////////

static ADDRESS_MAP_START( memmap_vid, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x1fff) AM_READ(ram_r)			  // 8k RAM
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(ram_w) AM_BASE(&nvram) AM_SIZE(&nvram_size)
	AM_RANGE(0x2000, 0x2000) AM_READ(vfd_status_hop_r)  //vfd_status_r) // vfd status register
	AM_RANGE(0x2000, 0x20FF) AM_WRITE(reel12_vid_w)	  //reel12_w)
	AM_RANGE(0x2100, 0x21FF) AM_WRITE(reel34_w)
	AM_RANGE(0x2200, 0x22FF) AM_WRITE(reel56_w)
	AM_RANGE(0x2300, 0x230B) AM_READ(mux_input_r)			// mux inputs
	AM_RANGE(0x2300, 0x231F) AM_WRITE(mux_output_w)		// mux outputs
	AM_RANGE(0x2320, 0x2323) AM_WRITE(dimas_w)			// ?unknown dim related
	AM_RANGE(0x2324, 0x2324) AM_READ(expansion_latch_r)	//
	AM_RANGE(0x2324, 0x2324) AM_WRITE(expansion_latch_w)	//
	AM_RANGE(0x2325, 0x2327) AM_WRITE(unknown_w)			// ?unknown
	AM_RANGE(0x2328, 0x2328) AM_WRITE(muxena_w)			// mux enable
	AM_RANGE(0x2329, 0x2329) AM_READ(timerirqclr_r)
	AM_RANGE(0x2329, 0x2329) AM_WRITE(timerirq_w)
	AM_RANGE(0x232A, 0x232D) AM_WRITE(unknown_w)			// ?unknown
	AM_RANGE(0x232E, 0x232E) AM_READ(irqstatus_r)
	AM_RANGE(0x232F, 0x232F) AM_WRITE(coininhib_w)		// coin inhibits
	AM_RANGE(0x2330, 0x2330) AM_WRITE(payout_latch_w)
	AM_RANGE(0x2331, 0x2331) AM_WRITE(payout_triac_w)
	AM_RANGE(0x2332, 0x2332) AM_WRITE(watchdog_w)		  // kick watchdog
	AM_RANGE(0x2333, 0x2333) AM_WRITE(mmtr_w)			  // mechanical meters
	AM_RANGE(0x2334, 0x2335) AM_WRITE(unknown_w)
	AM_RANGE(0x2336, 0x2336) AM_WRITE(dimcnt_w)		  // ?unknown dim related
	AM_RANGE(0x2337, 0x2337) AM_WRITE(volume_override_w)
	AM_RANGE(0x2338, 0x2338) AM_WRITE(payout_select_w)
	AM_RANGE(0x2339, 0x2339) AM_WRITE(unknown_w)		  // ?unknown
	AM_RANGE(0x2400, 0x2400) AM_READ(uart1stat_r)		  // mc6850 compatible uart
	AM_RANGE(0x2400, 0x2400) AM_WRITE(uart1ctrl_w)
	AM_RANGE(0x2500, 0x2500) AM_READ(uart1data_r)
	AM_RANGE(0x2500, 0x2500) AM_WRITE(uart1data_w)
	AM_RANGE(0x2600, 0x2600) AM_READ(uart2stat_r)		  // mc6850 compatible uart
	AM_RANGE(0x2600, 0x2600) AM_WRITE(uart2ctrl_w)
	AM_RANGE(0x2700, 0x2700) AM_READ(uart2data_r)
	AM_RANGE(0x2700, 0x2700) AM_WRITE(uart2data_w)
	AM_RANGE(0x2800, 0x2800) AM_WRITE(vfd1_data_w)	  // vfd1 data
	AM_RANGE(0x2900, 0x2900) AM_WRITE(vfd_reset_w)	  // vfd1+vfd2 reset line

	AM_RANGE(0x2A00, 0x2AFF) AM_WRITE(nec_latch_w)	  // this is where it reads?
	AM_RANGE(0x2B00, 0x2BFF) AM_WRITE(nec_reset_w)	  // upd7759 reset line
	AM_RANGE(0x2C00, 0x2C00) AM_WRITE(unlock_w)		  // custom chip unlock
	AM_RANGE(0x2D00, 0x2D00) AM_WRITE(YM2413_register_port_0_w)
	AM_RANGE(0x2D01, 0x2D01) AM_WRITE(YM2413_data_port_0_w)
	AM_RANGE(0x2F00, 0x2F00) AM_WRITE(vfd2_data_w)	  // vfd2 data
	AM_RANGE(0x2E00, 0x2E00) AM_WRITE(bankswitch_w)	  // write bank (rom page select for 0x6000 - 0x7fff )
	AM_RANGE(0x3FFF, 0x3FFF) AM_READ( coin_input_r)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_ROM)		  // 8k  fixed ROM
	AM_RANGE(0x4000, 0xFFFF) AM_WRITE(unknown_w)		  // contains unknown I/O registers
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK1)		  // 8k  paged ROM (4 pages)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)		  // 32k ROM

	AM_RANGE(0x3C00, 0x3C07) AM_READ(  key_r   )
	AM_RANGE(0x3C80, 0x3C80) AM_WRITE( e2ram_w )

	AM_RANGE(0x3e00, 0x3e00) AM_READ( vid_uart_ctrl_r)	// video uart control reg read
	AM_RANGE(0x3e00, 0x3e00) AM_WRITE(vid_uart_ctrl_w)	// video uart control reg write
	AM_RANGE(0x3e01, 0x3e01) AM_READ( vid_uart_rx_r)		// video uart receive  reg
	AM_RANGE(0x3e01, 0x3e01) AM_WRITE(vid_uart_tx_w)		// video uart transmit reg
ADDRESS_MAP_END

// input ports for pyramid ////////////////////////////////////////

	INPUT_PORTS_START( pyramid )

	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.50")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Select Stake") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)
	PORT_BIT( 0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Collect") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
 	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Coin 1 Inhibit?" ) PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin 2 Inhibit?" ) PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Coin 3 Inhibit?" ) PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin 4 Inhibit?" ) PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "DIL06" ) PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x02, "Attract mode language" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x02, "Dutch"       )
	PORT_DIPNAME( 0x0C, 0x00, "Skill Level" ) PORT_DIPLOCATION("DIL:08,10")
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x04, "Medium-Low" )
	PORT_DIPSETTING(    0x08, "Medium-High")
	PORT_DIPSETTING(    0x0C, DEF_STR( High ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL11" ) PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x00, "DIL12" ) PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL13" ) PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x04, "Attract mode" ) PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x18, 0x00, "Stake" ) PORT_DIPLOCATION("DIL:15,16")
	PORT_DIPSETTING(    0x00, "4 credits per game"  )
	PORT_DIPSETTING(    0x08, "1 credit  per round" )
	PORT_DIPSETTING(    0x10, "2 credit  per round" )
	PORT_DIPSETTING(    0x18, "4 credits per round" )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for golden crown ///////////////////////////////////

INPUT_PORTS_START( gldncrwn )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Collect") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME( "Reel 1" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME( "Reel 2" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME( "Reel 3" )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME( "Reel 4" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME( "Reel 5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME( "Reel 6" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME( "Hall Of Fame" ) PORT_CODE( KEYCODE_J )
	PORT_BIT( 0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Coin 1 Inhibit?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin 2 Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Coin 3 Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin 4 Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "Attract mode language" )PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, "Dutch")
	PORT_DIPSETTING(    0x01, DEF_STR( English ) )
	PORT_DIPNAME( 0x02, 0x00, "Max number of spins" )PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, "99")
	PORT_DIPSETTING(    0x02, "50")
	PORT_DIPNAME( 0x0C, 0x00, "Skill Level" )PORT_DIPLOCATION("DIL:08,10")
	PORT_DIPSETTING(    0x00, DEF_STR( Low ))
	PORT_DIPSETTING(    0x04, "Medium-Low"  )
	PORT_DIPSETTING(    0x08, "Medium-High" )
	PORT_DIPSETTING(    0x0C, DEF_STR( High ) )
	PORT_DIPNAME( 0x10, 0x00, "Base Pricing on:" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, "Full Game")
	PORT_DIPSETTING(    0x10, "Individual Rounds")

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x01, "Credits required:" )PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, "4 credits per game")PORT_CONDITION("STROBE10",0x10,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x01, "2 credits per game")PORT_CONDITION("STROBE10",0x10,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x00, "1 credit  per round")PORT_CONDITION("STROBE10",0x10,PORTCOND_EQUALS,0x10)
	PORT_DIPSETTING(    0x01, "4 credits per round")PORT_CONDITION("STROBE10",0x10,PORTCOND_EQUALS,0x10)

/*
          Type1 Type2
              0     0   4 credits per game
              0     1   2 credits per game
              1     0   1 credit  per round
              1     1   4 credits per round
 */

	PORT_DIPNAME( 0x02, 0x00, "Attract Mode" )PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x04, "Time bar" )PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On  ) )
	PORT_DIPNAME( 0x18, 0x00, "Time bar speed" )PORT_DIPLOCATION("DIL:15,16")
	PORT_DIPSETTING(    0x00, "1 (fast)" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "4 (slow)" )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
	INPUT_PORTS_END

// input ports for dutch quintoon /////////////////////////////////

	INPUT_PORTS_START( qntoond )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER)	  PORT_NAME("Collect") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Hand 1" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Hand 2" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Hand 3" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Hand 4" )
	PORT_BIT( 0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Hand 5" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN )

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x02, "Fl 0.25 Inhibit?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )//"Enabled")
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )//"Disabled")
	PORT_DIPNAME( 0x04, 0x04, "Fl 1.00 Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Fl 2.50 Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Fl 5.00 Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "DIL06" ) PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL07" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL08" ) PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL09" ) PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin jam alarm" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x00, "Time bar" )PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "Clear credits on reset" )PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x04, 0x00, "Attract mode" )PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "Attract mode language" )PORT_DIPLOCATION("DIL:15")
	PORT_DIPSETTING(    0x00, "Dutch"    )
	PORT_DIPSETTING(    0x08, DEF_STR( English  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL15" ) PORT_DIPLOCATION("DIL:16")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for UK quintoon ////////////////////////////////////////////

	INPUT_PORTS_START( quintoon )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("10p")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("20p")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("50p")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("?1.00")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER)   PORT_NAME("Collect/Strobe Advance") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Hand 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Hand 2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Hand 3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Hand 4")

	PORT_START_TAG("STROBE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Hand 5")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("?1") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_START1)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("?2") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("?3") PORT_CODE(KEYCODE_E)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE3")
	PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT(0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Clear credits on reset?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL03" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL04" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL05" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "Coin Lockout")PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )//Will activate coin lockout when Credit >= 1 Play
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL07" )PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL08" )PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL10" )PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Stake per Game / Jackpot" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, "20p / 6 Pounds" )
	PORT_DIPSETTING(    0x10, "50p / 20 Pounds" )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x00, "DIL12" )PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL13" )PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x1C, 0x1C, "Target percentage" )PORT_DIPLOCATION("DIL:14,15,16")
	PORT_DIPSETTING(    0x1C, "50%")
	PORT_DIPSETTING(    0x0C, "55%")
	PORT_DIPSETTING(    0x08, "60%")
	PORT_DIPSETTING(    0x18, "65%")
	PORT_DIPSETTING(    0x10, "70%")
	PORT_DIPSETTING(    0x00, "75%")
	PORT_DIPSETTING(    0x04, "80%")
	PORT_DIPSETTING(    0x14, "85%")

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for slotsnl  ///////////////////////////////////////////////

	INPUT_PORTS_START( slotsnl )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Slot 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Slot 2")
	PORT_BIT(0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON3)  PORT_NAME("Slot 3")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON4)  PORT_NAME("Slot 4")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Select") PORT_CODE( KEYCODE_W )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE3")
	PORT_BIT(0x0F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x18, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE6")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE7")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE8")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Fl 0.25 Inhibit?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Fl 1.00 Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Fl 2.50 Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Fl 5.00 Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "DIL06" ) PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL07" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL08" ) PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL10" ) PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin jam alarm" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x00, "DIL12" ) PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL13" ) PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Attract mode" )PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x18, 0x00, "Timebar speed" )PORT_DIPLOCATION("DIL:15,16")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x18, "4" )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for sltblgtk  //////////////////////////////////////////////

	INPUT_PORTS_START( sltblgtk )
	PORT_START_TAG("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Token")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("20 BFr")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("50 BFr")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Slot 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Slot 2")
	PORT_BIT(0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Slot 3")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Slot 4")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Select") PORT_CODE( KEYCODE_W )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW,  IPT_OTHER) PORT_TOGGLE PORT_NAME("Tube 1 level switch") PORT_CODE(KEYCODE_I) //Could do with being hooked up
	PORT_BIT(0x02, IP_ACTIVE_LOW,  IPT_OTHER) PORT_TOGGLE PORT_NAME("Tube 2 level switch") PORT_CODE(KEYCODE_O) //to real program
	PORT_BIT(0x0C, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW,  IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT(0x02, IP_ACTIVE_LOW,  IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x18, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE6")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE7")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE8")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "CashMeters in refill menu" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Token Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No  ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "20 Bfr Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No  ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "50 Bfr Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes  ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )

	PORT_DIPNAME( 0x0E, 0x0E, "Payout Percentage" )PORT_DIPLOCATION("DIL:07,08,10")
	PORT_DIPSETTING(    0x00, "60%")
	PORT_DIPSETTING(    0x08, "65%")
	PORT_DIPSETTING(    0x04, "70%")
	PORT_DIPSETTING(    0x0C, "75%")
	PORT_DIPSETTING(    0x02, "80%")
	PORT_DIPSETTING(    0x0A, "84%")
	PORT_DIPSETTING(    0x06, "88%")
	PORT_DIPSETTING(    0x0E, "90%")
	PORT_DIPNAME( 0x10, 0x00, "Coin jam alarm" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x01, "Timebar" )PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Clear credits" )PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Attract mode" )PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off  ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On   ) )
	PORT_DIPNAME( 0x08, 0x08, "Show hints" )PORT_DIPLOCATION("DIL:15")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Pay win to credits" )PORT_DIPLOCATION("DIL:16")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for sltblgpo  //////////////////////////////////////////////

	INPUT_PORTS_START( sltblgpo )
	PORT_START_TAG("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Bfr 20")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Bfr 50")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Slot 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Slot 2")
	PORT_BIT(0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON3)  PORT_NAME("Slot 3")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON4)  PORT_NAME("Slot 4")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE)  PORT_NAME("Select") PORT_CODE( KEYCODE_W )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)  PORT_NAME("Stake")  PORT_CODE( KEYCODE_Q )
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE3")
	PORT_BIT(0x07, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Collect") PORT_CODE(KEYCODE_C)
	PORT_BIT(0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW,  IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT(0x02, IP_ACTIVE_LOW,  IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x18, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE6")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE7")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE8")
	PORT_BIT(0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT(0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Hopper Limit" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, "300" )
	PORT_DIPSETTING(    0x02, "500" )
	PORT_DIPNAME( 0x04, 0x00, "DIL07" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )

	PORT_DIPNAME( 0x18, 0x00, "Attendant payout" )PORT_DIPLOCATION("DIL:04,05")
	PORT_DIPSETTING(    0x00, "1000 Bfr" )
	PORT_DIPSETTING(    0x08, "1250 Bfr" )
	PORT_DIPSETTING(    0x10, "1500 Bfr" )
	PORT_DIPSETTING(    0x18, "1750 Bfr" )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "Bfr 20 Inhibit?" )PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL07" )PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL08" )PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL10" )PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin jam alarm" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x00, "Clear credits on reset?" )PORT_DIPLOCATION("DIL:12")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "Attract Mode" )PORT_DIPLOCATION("DIL:13")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On  ) )
	PORT_DIPNAME( 0x1C, 0x08, "Target Percentage" )PORT_DIPLOCATION("DIL:14,15,16")
	PORT_DIPSETTING(    0x14, "80%")
	PORT_DIPSETTING(    0x04, "82%")
	PORT_DIPSETTING(    0x1C, "84%")
	PORT_DIPSETTING(    0x0C, "86%")
	PORT_DIPSETTING(    0x10, "90%")
	PORT_DIPSETTING(    0x00, "92%")
	PORT_DIPSETTING(    0x18, "94%")
	PORT_DIPSETTING(    0x08, "96%")

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for paradice ///////////////////////////////////////////////

INPUT_PORTS_START( paradice )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 ) PORT_NAME( "1 Player Start (Left)" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 ) PORT_NAME( "2 Player Start (Right)" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME( "A (Column 1)" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME( "B (Column 2)" )
	PORT_BIT( 0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME( "C (Column 3)" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER  ) PORT_NAME( "Start (Enter)" ) PORT_CODE( KEYCODE_SPACE )
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Coin 1 Inhibit?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin 2 Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Coin 3 Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin 4 Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x01, "Joker" )PORT_DIPLOCATION("DIL:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Language ) )PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x02, "Dutch"    )
	PORT_DIPNAME( 0x0C, 0x0C, "Payout level" )PORT_DIPLOCATION("DIL:08,10")
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x08, "Medium-Low"  )
	PORT_DIPSETTING(    0x04, "Medium-High" )
	PORT_DIPSETTING(    0x0C, DEF_STR( High ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Difficulty ) )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )


	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x03, 0x00, "Winlines to go" )PORT_DIPLOCATION("DIL:12,13")
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x01, "8" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x04, 0x04, "Attract mode" )PORT_DIPLOCATION("DIL:14")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x18, 0x00, "Timebar speed" )PORT_DIPLOCATION("DIL:15,16")
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

// input ports for pokio //////////////////////////////////////////////////

	INPUT_PORTS_START( pokio )
	PORT_START_TAG("COINS")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("Fl 0.25")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("Fl 1.00")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("Fl 2.50")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("Fl 5.00")
	PORT_BIT( 0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE0")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("STROBE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME( "Hand 1 Left" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME( "Hand 2 Left" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME( "Hand 3 Left" )
	PORT_BIT( 0xF8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 ) PORT_NAME( "1 Player Start (Left)" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER )  PORT_NAME( "Enter" ) PORT_CODE( KEYCODE_SPACE )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 ) PORT_NAME( "2 Player Start (Right)" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON6 )PORT_NAME( "Hand 3 Right" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 )PORT_NAME( "Hand 2 Right" )
	PORT_BIT( 0xE0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE3")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 )PORT_NAME( "Hand 1 Right" )
	PORT_BIT( 0xE8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Cashbox Door") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME("Front Door") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0xF8, IP_ACTIVE_HIGH,IPT_UNKNOWN)

	PORT_START_TAG("STROBE5")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE6")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE7")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE8")
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("STROBE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_Y)
	PORT_DIPNAME( 0x02, 0x00, "Coin 1 Inhibit?" )PORT_DIPLOCATION("DIL:02")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin 2 Inhibit?" )PORT_DIPLOCATION("DIL:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Coin 3 Inhibit?" )PORT_DIPLOCATION("DIL:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin 4 Inhibit?" )PORT_DIPLOCATION("DIL:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )

	PORT_START_TAG("STROBE10")
	PORT_DIPNAME( 0x01, 0x00, "DIL06" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL07" ) PORT_DIPLOCATION("DIL:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL08" ) PORT_DIPLOCATION("DIL:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL10" ) PORT_DIPLOCATION("DIL:10")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin Jam alarm" )PORT_DIPLOCATION("DIL:11")
	PORT_DIPSETTING(    0x10, DEF_STR( Off  ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On   ) )

	PORT_START_TAG("STROBE11")
	PORT_DIPNAME( 0x01, 0x01, "Time bar" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x04, "Attract mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x18, 0x00, "Timebar speed" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )

	PORT_START_TAG("DEBUGPORT")
#ifdef BFM_DEBUG
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_TOGGLE PORT_NAME( "Alpha display toggle" ) PORT_CODE( KEYCODE_X )
#endif
INPUT_PORTS_END

 static struct upd7759_interface upd7759_interface =
 {
	REGION_SOUND1,		/* memory region */
	0
 };

///////////////////////////////////////////////////////////////////////////
// machine driver for scorpion2 board + adder2 expansion //////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_DRIVER_START( scorpion2_vid )

  MDRV_MACHINE_RESET( adder2_init_vid )					// main scorpion2 board initialisation

  MDRV_INTERLEAVE(16)									// needed for serial communication !!

  MDRV_CPU_ADD_TAG("main", M6809, 2000000 )				// 6809 CPU at 2 Mhz

  MDRV_CPU_PROGRAM_MAP(memmap_vid,0)					// setup scorpion2 board memorymap

  MDRV_CPU_PERIODIC_INT(timer_irq, TIME_IN_HZ(1000) )	// generate 1000 IRQ's per second

  MDRV_NVRAM_HANDLER(nvram)

  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
  MDRV_SCREEN_SIZE( 400, 300)
  MDRV_VISIBLE_AREA(  0, 400-1, 0, 300-1)
  MDRV_FRAMES_PER_SECOND(50)
  MDRV_VIDEO_START( adder2)
  MDRV_VIDEO_RESET( adder2)
  MDRV_VIDEO_UPDATE(adder2)

  MDRV_PALETTE_LENGTH(16)
  MDRV_COLORTABLE_LENGTH(16)
  MDRV_PALETTE_INIT(adder2)
  MDRV_GFXDECODE(adder2_gfxdecodeinfo)

  MDRV_CPU_ADD_TAG("adder2", M6809, 2000000 )			// adder2 board 6809 CPU at 2 Mhz
  MDRV_CPU_PROGRAM_MAP(adder2_memmap,0)					// setup adder2 board memorymap
  MDRV_CPU_VBLANK_INT(adder2_vbl, 1);					// board has a VBL IRQ

  MDRV_SPEAKER_STANDARD_MONO("mono")
  MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
  MDRV_SOUND_CONFIG(upd7759_interface)
  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

  MDRV_SOUND_ADD(YM2413, 3579545)
  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_DRIVER_END

// UK quintoon initialisation ////////////////////////////////////////////////

DRIVER_INIT (quintoon)
{
	UINT8 *rom;

	decode_mainrom(REGION_CPU1);		  // decode main rom

	adder2_decode_char_roms();

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	has_hopper = 0;

	Mechmtr_init(8);	// setup mech meters

	memset(sc2_Inputs, 0, sizeof(sc2_Inputs));  // clear all inputs

	Scorpion2_SetSwitchState(3,0,1);	  // tube1 level switch
	Scorpion2_SetSwitchState(3,1,1);	  // tube2 level switch
	Scorpion2_SetSwitchState(3,2,1);	  // tube3 level switch

	Scorpion2_SetSwitchState(5,2,1);
	Scorpion2_SetSwitchState(6,4,1);

	//Scorpion2_SetSwitchState(4,1,1);

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}

// dutch pyramid intialisation //////////////////////////////////////////////

DRIVER_INIT (pyramid)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	has_hopper = 1;

	memset(sc2_Inputs, 0, sizeof(sc2_Inputs));  // clear all inputs

	Scorpion2_SetSwitchState(3,0,1);	  // tube1 level switch
	Scorpion2_SetSwitchState(3,1,1);	  // tube2 level switch
	Scorpion2_SetSwitchState(3,2,1);	  // tube3 level switch

	//Scorpion2_SetSwitchState(11,2,1);   // DIL 14 ON !!

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}

// dutch slots initialisation /////////////////////////////////////////////

DRIVER_INIT (slotsnl)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	memset(sc2_Inputs, 0x00, sizeof(sc2_Inputs));  // clear all inputs

	has_hopper = 0;

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}
// belgian slots initialisation /////////////////////////////////////////////

DRIVER_INIT (sltsbelg)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	memset(sc2_Inputs, 0x00, sizeof(sc2_Inputs));  // clear all inputs

	has_hopper = 1;

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}


// dutch quintoon /////////////////////////////////////////////////////////

DRIVER_INIT (qntoond)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	has_hopper = 0;

	memset(sc2_Inputs, 0, sizeof(sc2_Inputs));  // clear all inputs

	Scorpion2_SetSwitchState(3,0,1);	  // tube1 level switch
	Scorpion2_SetSwitchState(3,1,1);	  // tube2 level switch
	Scorpion2_SetSwitchState(3,2,1);	  // tube3 level switch

	//Scorpion2_SetSwitchState(4,0,1);    // cash box   switch
	//Scorpion2_SetSwitchState(4,1,1);    // front door switch
	//Scorpion2_SetSwitchState(11,2,1);   // DIL 14 ON !!

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}

// dutch pokio and paradice/////////////////////////////////////////////////

DRIVER_INIT (pokio)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	has_hopper = 0;

	memset(sc2_Inputs, 0, sizeof(sc2_Inputs));  // clear all inputs

	Scorpion2_SetSwitchState(3,0,1);	  // tube1 level switch
	Scorpion2_SetSwitchState(3,1,1);	  // tube2 level switch
	Scorpion2_SetSwitchState(3,2,1);	  // tube3 level switch

	//Scorpion2_SetSwitchState(4,0,1);    // cash box   switch
	//Scorpion2_SetSwitchState(4,1,1);    // front door switch

	//Scorpion2_SetSwitchState(11,2,1);   // DIL 14 ON !!

	sc2_show_door   = 1;
	sc2_door_state  = 0x41;
}

// dutch golden Crown /////////////////////////////////////////////////////

DRIVER_INIT (gldncrwn)
{
	UINT8 *pal;
	UINT8 *rom;

	pal = memory_region(REGION_PROMS);
	if ( pal )
	{
		memcpy(key, pal, 8);
	}

	decode_mainrom(REGION_CPU1);		  // decode main rom
	adder2_decode_char_roms();		  // decode GFX roms

	rom = memory_region(REGION_CPU1);
	if ( rom )
	{
		memcpy(&rom[0x10000], &rom[0x00000], 0x2000);
	}

	has_hopper = 0;

	memset(sc2_Inputs, 0, sizeof(sc2_Inputs));  // clear all inputs

	Scorpion2_SetSwitchState(3,0,1);	  // tube1 level switch
	Scorpion2_SetSwitchState(3,1,1);	  // tube2 level switch
	Scorpion2_SetSwitchState(3,2,1);	  // tube3 level switch

	//Scorpion2_SetSwitchState(4,0,1);    // cash box   switch
	//Scorpion2_SetSwitchState(4,1,1);    // front door switch
	sc2_show_door   = 0;
	sc2_door_state  = 0x41;
}

// ROM definition UK Quintoon /////////////////////////////////////////////

ROM_START( quintoon )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750203.bin",   0x0000, 0x10000,  CRC(037ef2d0) SHA1(6958624e29629a7639a80e8929b833a8b0201833))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "quinp132", 0x0000, 0x20000,  CRC(63896a7f) SHA1(81aa56874a15faa3aabdfc0fc524b2e25b751f22))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples - using Dutch version, need to check a UK Quintoon PCB
	ROM_LOAD( "95001016.snd", 0x00000, 0x20000, BAD_DUMP CRC(cf097d41) SHA1(6712f93896483360256d8baffc05977c8e532ef1))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "quinp233",0x00000, 0x20000, CRC(3d4ebecf) SHA1(b339cf16797ccf7a1ec20fcebf52b6edad9a1047))

ROM_END

// ROM definition Dutch Quintoon ///////////////////////////////////////////

ROM_START( qntoond )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750243.bin", 0x0000, 0x10000, CRC(36a8dcd1) SHA1(ab21301312fbb6609f850e1cf6bcda5a2b7f66f5))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770024.vid", 0x0000, 0x20000,  CRC(5bc7ac55) SHA1(b54e9684f750b73c357d41b88ca8c527258e2a10))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001016.snd", 0x00000, 0x20000, CRC(cf097d41) SHA1(6712f93896483360256d8baffc05977c8e532ef1))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770025.chr", 0x00000, 0x20000, CRC(f59748ea) SHA1(f0f7f914fdf72db8eb60717b95e7d027c0081339))

ROM_END

// ROM definition Dutch Quintoon alternate set /////////////////////////////

ROM_START( qntoondo )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750136.bin", 0x0000, 0x10000, CRC(839ea01d) SHA1(d7f77dbaea4e87c3d782408eb50d10f44b6df5e2))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770024.vid", 0x0000, 0x20000,  CRC(5bc7ac55) SHA1(b54e9684f750b73c357d41b88ca8c527258e2a10))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001016.snd", 0x00000, 0x20000, CRC(cf097d41) SHA1(6712f93896483360256d8baffc05977c8e532ef1))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770025.chr", 0x00000, 0x20000, CRC(f59748ea) SHA1(f0f7f914fdf72db8eb60717b95e7d027c0081339))

ROM_END

// ROM definition dutch golden crown //////////////////////////////////////

ROM_START( gldncrwn )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95752011.bin", 0x0000, 0x10000, CRC(54f7cca0) SHA1(835727d88113700a38060f880b4dfba2ded41487))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770117.vid", 0x0000, 0x20000,   CRC(598ba7cb) SHA1(ab518d7df24b0b453ec3fcddfc4db63e0391fde7))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001039.snd", 0x00000, 0x20000, CRC(6af26157) SHA1(9b3a85f5dd760c4430e38e2844928b74aadc7e75))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770118.ch1", 0x00000, 0x20000, CRC(9c9ac946) SHA1(9a571e7d00f6654242aface032c2fb186ef44aba))
	ROM_LOAD( "95770119.ch2", 0x20000, 0x20000, CRC(9e0fdb2e) SHA1(05e8257285b0009df4fcc73e93490876358a8be8))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "gcrpal.bin", 0, 8 , CRC(4edd5a1d) SHA1(d6fe38377d5f2291d33ee8ed808548871e63c4d7))
ROM_END

// ROM definition Dutch Paradice //////////////////////////////////////////

ROM_START( paradice )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750615.bin", 0x0000, 0x10000, CRC(f51192e5) SHA1(a1290e32bba698006e83fd8d6075202586232929))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770084.vid", 0x0000, 0x20000, CRC(8f27bd34) SHA1(fccf7283b5c952b74258ee6e5138c1ca89384e24))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001037.snd", 0x00000, 0x20000, CRC(82f74276) SHA1(c51c3caeb7bf514ec7a1b452c8effc4c79186062))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770085.ch1", 0x00000, 0x20000, CRC(4d1fb82f) SHA1(054f683d1d7c884911bd2d0f85aab4c59ddf9930))
	ROM_LOAD( "95770086.ch2", 0x20000, 0x20000, CRC(7b566e11) SHA1(f34c82ad75a0f88204ac4ae83a00801215c46ca9))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "pdcepal.bin", 0, 8 , CRC(64020c97) SHA1(9371841e2df950c1f2e5b5a4b52621beb6f60945))
ROM_END

// ROM definition Dutch Pokio /////////////////////////////////////////////

ROM_START( pokio )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750278.bin", 0x0000, 0x10000, CRC(5124b24d) SHA1(9bc63891a8e9283c2baa64c264a5d6d1625d44b2))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770044.vid", 0x0000, 0x20000,   CRC(46d7a6d8) SHA1(01f58e735621661b57c61491b3769ae99e92476a))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001016.snd", 0x00000, 0x20000, CRC(98aaff76) SHA1(4a59cf83daf018d93f1ff7805e06309d2f3d7252))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770045.chr", 0x00000, 0x20000, CRC(dd30da90) SHA1(b4f5a229d88613c0c7d43adf3f325c619abe38a3))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "pokiopal.bin", 0, 8 , CRC(53535184) SHA1(c5c98085e39ca3671dca72c21a8466d7d70cd341))
ROM_END

// ROM definition pyramid prototype  //////////////////////////////////////

ROM_START( pyramid )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750898.bin", 0x0000, 0x10000,  CRC(3b0df16c) SHA1(9af599fe604f86c72986aa1610d74837852e023f))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770108.vid", 0x0000, 0x20000,  CRC(216ff683) SHA1(227764771600ce88c5f36bed9878e6bb9988ae8f))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001038.snd", 0x00000, 0x20000, CRC(f885c42e) SHA1(4d79fc5ae4c58247740d78d81302bfbb43331c43))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770106.ch1", 0x00000, 0x20000, CRC(a83c27ae) SHA1(f61ca3cdf19a933bae18c1b32a5fb0a2204dde78))
	ROM_LOAD( "95770107.ch2", 0x20000, 0x20000, CRC(52e59f64) SHA1(ea4828c2cfb72cd77c92c60560b4d5ee424f7dca))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "pyrmdpal.bin", 0, 8 , CRC(1c7c37bb) SHA1(fe0276603fee8f58e4318f91645260368212b78b))
ROM_END

// ROM definition Dutch slots /////////////////////////////////////////////

ROM_START( slotsnl )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750368.bin", 0x0000, 0x10000,  CRC(3a43048c) SHA1(13728e05b334cba90ea9cc51ea00c4384baa8614))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "video.vid", 0x0000, 0x20000,  CRC(cc760208) SHA1(cc01b1e31335b26f2d0f3470d8624476b153655f))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "95001029.snd", 0x00000, 0x20000, CRC(7749c724) SHA1(a87cce0c99e392f501bba44b3936a7059d682c9c))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "charset.chr",	0x00000, 0x20000,  CRC(ef4300b6) SHA1(a1f765f38c2f146651fc685ea6195af72465f559))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "slotspal.bin", 0, 8 , CRC(ee5421f0) SHA1(21bdcbf11dda8b1a93c49ae1c706954bba53c917))
ROM_END

// ROM definition Belgian Slots (Token pay per round) Payslide ////////////

ROM_START( sltblgtk )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95750943.bin", 0x0000, 0x10000,  CRC(c9fb8153) SHA1(7c1d0660c15f05b1e0784d8322c62981fe8dc4c9))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "adder121.bin", 0x0000, 0x20000,  CRC(cedbbf28) SHA1(559ae341b55462feea771127394a54fc65266818))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "sound029.bin", 0x00000, 0x20000, CRC(7749c724) SHA1(a87cce0c99e392f501bba44b3936a7059d682c9c))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "chr122.bin",	0x00000, 0x20000,  CRC(a1e3bdf4) SHA1(f0cabe08dee028e2014cbf0fc3fe0806cdfa60c6))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "stsbtpal.bin", 0, 8 , CRC(20e13635) SHA1(5aa7e7cac8c00ebc193d63d0c6795904f42c70fa))
ROM_END

// ROM definition Belgian Slots (Cash Payout) /////////////////////////////

ROM_START( sltblgp1 )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95752008.bin", 0x0000, 0x10000,  CRC(3167d3b9) SHA1(a28563f65d55c4d47f3e7fdb41e050d8a733b9bd))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "adder142.bin", 0x0000, 0x20000,  CRC(a6f6356b) SHA1(b3d3063155ee3ea888273081f844279b6e33f7d9))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "sound033.bin", 0x00000, 0x20000, CRC(bb1dfa55) SHA1(442454fccfe03e6f4c3353551cb7459e184a099d))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "chr143.bin",	0x00000, 0x20000,  CRC(a40e91e2) SHA1(87dc76963ea961fcfbe4f3e25df9162348d39d79))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "stsbcpal.bin", 0, 8 , CRC(c63bcab6) SHA1(238841165d5b3241b0bcc5c1792e9c0be1fc0177))
ROM_END

// ROM definition Belgian Slots (Cash Payout) /////////////////////////////

ROM_START( sltblgpo )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64 + 8k for code */
	ROM_LOAD( "95770938.bin", 0x0000, 0x10000,  CRC(7e802634) SHA1(fecf86e632546649d5e647c42a248b39fc2cf982))

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "95770120.chr", 0x00000, 0x20000, CRC(ad505138) SHA1(67ccd8dc30e76283247ab5a62b22337ebaff74cd))

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "sound033.bin", 0x00000, 0x20000, CRC(bb1dfa55) SHA1(442454fccfe03e6f4c3353551cb7459e184a099d))

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASEFF )
	ROM_LOAD( "95770110.add", 0x0000, 0x20000,  CRC(64b03284) SHA1(4b1c17b75e449c9762bb949d7cde0694a3aaabeb))

	ROM_REGION( 0x10, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "stsbcpal.bin", 0, 8 , CRC(c63bcab6) SHA1(238841165d5b3241b0bcc5c1792e9c0be1fc0177))
ROM_END

//     year, name,     parent,    machine,       input,     init,       monitor, company,    fullname
GAME ( 1993, qntoondo, qntoond,	  scorpion2_vid, qntoond,   qntoond,    0,       "BFM/ELAM", "Quintoon (Dutch, Game Card 95-750-136)",0)
GAME(  1993, quintoon, 0,   	  scorpion2_vid, quintoon,  quintoon,   0,       "BFM",      "Quintoon (UK, Game Card 95-750-203)", GAME_IMPERFECT_SOUND  ) //Current samples need verification
GAME ( 1993, qntoond,  0,		  scorpion2_vid, qntoond,   qntoond,    0,       "BFM/ELAM", "Quintoon (Dutch, Game Card 95-750-243)",0)
GAME ( 1994, pokio,    0,		  scorpion2_vid, pokio,     pokio,      0,       "BFM/ELAM", "Pokio (Dutch, Game Card 95-750-278)",0)
GAME ( 1995, slotsnl,  0,		  scorpion2_vid, slotsnl,   slotsnl,    0,       "BFM/ELAM", "Slots (Dutch, Game Card 95-750-368)",0)
GAME ( 1995, paradice, 0,		  scorpion2_vid, paradice,  pokio,      0,       "BFM/ELAM", "Paradice (Dutch, Game Card 95-750-615)",0)
GAME ( 1996, pyramid,  0,		  scorpion2_vid, pyramid,   pyramid,    0,       "BFM/ELAM", "Pyramid (Dutch, Game Card 95-750-898)",0)

GAME ( 1996, sltblgtk, 0,		  scorpion2_vid, sltblgtk,  sltsbelg,   0,       "BFM/ELAM", "Slots (Belgian Token, Game Card 95-750-943)",0)
GAME ( 1996, sltblgpo, 0, 		  scorpion2_vid, sltblgpo,  sltsbelg,   0,       "BFM/ELAM", "Slots (Belgian Cash, Game Card 95-750-938)",0)
GAME ( 1996, sltblgp1, sltblgpo,  scorpion2_vid, sltblgpo,  sltsbelg,   0,       "BFM/ELAM", "Slots (Belgian Cash, Game Card 95-752-008)",0)
GAME ( 1997, gldncrwn, 0,		  scorpion2_vid, gldncrwn,  gldncrwn,   0,       "BFM/ELAM", "Golden Crown (Dutch, Game Card 95-752-011)",0)

