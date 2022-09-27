/***************************************************************************

    Cinematronics vector hardware

    Special thanks to Neil Bradley, Zonn Moore, and Jeff Mitchell of the
    Retrocade Alliance

    Update:
    6/27/99 Jim Hernandez -- 1st Attempt at Fixing Drone Star Castle sound and
                             pitch adjustments.
    6/30/99 MLR added Rip Off, Solar Quest, Armor Attack (no samples yet)

    Bugs: Sometimes the death explosion (small explosion) does not trigger.

***************************************************************************/

#include "driver.h"
#include "cpu/ccpu/ccpu.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "cinemat.h"
#include "sound/samples.h"
#include "sound/ay8910.h"


/*************************************
 *
 *  Macros
 *
 *************************************/

#define RISING_EDGE(bit, changed, val)	(((changed) & (bit)) && ((val) & (bit)))
#define FALLING_EDGE(bit, changed, val)	(((changed) & (bit)) && !((val) & (bit)))

#define SOUNDVAL_RISING_EDGE(bit)		RISING_EDGE(bit, bits_changed, sound_val)
#define SOUNDVAL_FALLING_EDGE(bit)		FALLING_EDGE(bit, bits_changed, sound_val)

#define SHIFTREG_RISING_EDGE(bit)		RISING_EDGE(bit, (last_shift ^ current_shift), current_shift)
#define SHIFTREG_FALLING_EDGE(bit)		FALLING_EDGE(bit, (last_shift ^ current_shift), current_shift)

#define SHIFTREG2_RISING_EDGE(bit)		RISING_EDGE(bit, (last_shift2 ^ current_shift), current_shift)
#define SHIFTREG2_FALLING_EDGE(bit)		FALLING_EDGE(bit, (last_shift2 ^ current_shift), current_shift)



/*************************************
 *
 *  Global variables
 *
 *************************************/

static void (*sound_handler)(UINT8 sound_val, UINT8 bits_changed);
static UINT8 sound_control;

/* general shift register variables */
static UINT32 current_shift;
static UINT32 last_shift;
static UINT32 last_shift2;
static UINT32 current_pitch;
static UINT32 last_frame;

/* Rockola sound variables */
static UINT8 sound_fifo[16];
static UINT8 sound_fifo_in;
static UINT8 sound_fifo_out;
static UINT8 last_portb_write;



/*************************************
 *
 *  Generic sound write
 *
 *************************************/

WRITE8_HANDLER( cinemat_sound_control_w )
{
	UINT8 oldval = sound_control;

	/* form an 8-bit value with the new bit */
	sound_control = (sound_control & ~(1 << offset)) | ((data & 1) << offset);

	/* if something changed, call the sound subroutine */
	if ((sound_control != oldval) && sound_handler)
		(*sound_handler)(sound_control, sound_control ^ oldval);
}



/*************************************
 *
 *  Generic sound init
 *
 *************************************/

static void generic_init(void (*callback)(UINT8, UINT8))
{
	/* call the standard init */
	machine_reset_cinemat();

	/* set the sound handler */
	sound_handler = callback;

	/* reset sound control */
	sound_control = 0x9f;

	/* reset shift register values */
    current_shift = 0xffff;
    last_shift = 0xffff;
    last_shift2 = 0xffff;

	/* reset frame counters */
    last_frame = 0;

	/* reset Star Castle pitch */
    current_pitch = 0x20000;

    /* register for save states */
    state_save_register_global(sound_control);
    state_save_register_global(current_shift);
    state_save_register_global(last_shift);
    state_save_register_global(last_shift2);
    state_save_register_global(current_pitch);
    state_save_register_global(last_frame);
    state_save_register_global_array(sound_fifo);
    state_save_register_global(sound_fifo_in);
    state_save_register_global(sound_fifo_out);
    state_save_register_global(last_portb_write);
}



/*************************************
 *
 *  Space Wars
 *
 *************************************/

static const char *spacewar_sample_names[] =
{
	"*spacewar",
	"explode1.wav",
	"fire1.wav",
	"idle.wav",
	"thrust1.wav",
	"thrust2.wav",
	"pop.wav",
	"explode2.wav",
	"fire2.wav",
    0
};

static struct Samplesinterface spacewar_samples_interface =
{
	8,
	spacewar_sample_names
};

static void spacewar_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* Explosion - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x01))
		sample_start(0, (rand() & 1) ? 0 : 6, 0);

	/* Fire sound - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x02))
		sample_start(1, (rand() & 1) ? 1 : 7, 0);

	/* Player 1 thrust - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(3, 3, 1);
	if (SOUNDVAL_RISING_EDGE(0x04))
		sample_stop(3);

	/* Player 2 thrust - 0=on, 1-off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(4, 4, 1);
	if (SOUNDVAL_RISING_EDGE(0x08))
		sample_stop(4);

	/* Mute - 0=off, 1=on */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_start(2, 2, 1);	/* play idle sound */
	if (SOUNDVAL_RISING_EDGE(0x10))
	{
        int i;

		/* turn off all but the idle sound */
		for (i = 0; i < 5; i++)
			if (i != 2)
				sample_stop(i);

		/* Pop when board is shut off */
		sample_start(2, 5, 0);
	}
}

static MACHINE_RESET( spacewar )
{
	generic_init(spacewar_sound_w);
}

MACHINE_DRIVER_START( spacewar_sound )
	MDRV_MACHINE_RESET(spacewar)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(spacewar_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Barrier
 *
 *************************************/

static const char *barrier_sample_names[] =
{
	"*barrier",
	"playrdie.wav",
	"playmove.wav",
	"enemmove.wav",
    0
};

static struct Samplesinterface barrier_samples_interface =
{
	3,
	barrier_sample_names
};

static void barrier_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* Player die - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x01))
		sample_start(0, 0, 0);

	/* Player move - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(1, 1, 0);

	/* Enemy move - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(2, 2, 0);
}

static MACHINE_RESET( barrier )
{
	generic_init(barrier_sound_w);
}

MACHINE_DRIVER_START( barrier_sound )
	MDRV_MACHINE_RESET(barrier)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(barrier_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Speed Freak
 *
 *************************************/

static const char *speedfrk_sample_names[] =
{
	"*speedfrk",
	"offroad.wav",
    NULL
};

static struct Samplesinterface speedfrk_samples_interface =
{
	1,
	speedfrk_sample_names
};

static void speedfrk_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the falling edge of bit 0x08, clock the inverse of bit 0x04 into the top of the shiftreg */
	if (SOUNDVAL_FALLING_EDGE(0x08))
	{
		current_shift = ((current_shift >> 1) & 0x7fff) | ((~sound_val << 13) & 1);
		/* high 12 bits control the frequency - counts from value to $FFF, carry triggers */
		/* another counter */

		/* low 4 bits control the volume of the noise output (explosion?) */
	}

	/* off-road - 1=on, 0=off */
	if (SOUNDVAL_RISING_EDGE(0x10))
		sample_start(0, 0, 1);
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_stop(0);

    /* start LED is controlled by bit 0x02 */
    set_led_status(0, ~sound_val & 0x02);
}

static MACHINE_RESET( speedfrk )
{
	generic_init(speedfrk_sound_w);
}

MACHINE_DRIVER_START( speedfrk_sound )
	MDRV_MACHINE_RESET(speedfrk)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(speedfrk_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Star Hawk
 *
 *************************************/

static const char *starhawk_sample_names[] =
{
	"*starhawk",
	"explode.wav",
	"rlaser.wav",
	"llaser.wav",
	"k.wav",
	"master.wav",
	"kexit.wav",
    NULL
};

static struct Samplesinterface starhawk_samples_interface =
{
	5,
	starhawk_sample_names
};

static void starhawk_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x01))
		sample_start(0, 0, 0);

	/* right laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(1, 1, 0);

	/* left laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(2, 2, 0);

	/* K - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(3, 3, 1);
	if (SOUNDVAL_RISING_EDGE(0x08))
		sample_stop(3);

	/* master - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_start(4, 4, 1);
	if (SOUNDVAL_RISING_EDGE(0x10))
		sample_stop(4);

	/* K exit - 1=on, 0=off */
	if (SOUNDVAL_RISING_EDGE(0x80))
		sample_start(3, 5, 1);
	if (SOUNDVAL_FALLING_EDGE(0x80))
		sample_stop(3);
}

static MACHINE_RESET( starhawk )
{
	generic_init(starhawk_sound_w);
}

MACHINE_DRIVER_START( starhawk_sound )
	MDRV_MACHINE_RESET(starhawk)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(starhawk_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Sundance
 *
 *************************************/

static const char *sundance_sample_names[] =
{
	"*sundance",
	"bong.wav",
	"whoosh.wav",
	"explsion.wav",
	"ping1.wav",
	"ping2.wav",
	"hatch.wav",
    0
};

static struct Samplesinterface sundance_samples_interface =
{
	6,
	sundance_sample_names
};

static void sundance_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* bong - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x01))
		sample_start(0, 0, 0);

	/* whoosh - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(1, 1, 0);

	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(2, 2, 0);

	/* ping - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(3, 3, 0);

	/* ping - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_start(4, 4, 0);

	/* hatch - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x80))
		sample_start(5, 5, 0);
}

static MACHINE_RESET( sundance )
{
	generic_init(sundance_sound_w);
}

MACHINE_DRIVER_START( sundance_sound )
	MDRV_MACHINE_RESET(sundance)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(sundance_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Tail Gunner
 *
 *************************************/

static const char *tailg_sample_names[] =
{
	"*tailg",
	"sexplode.wav",
	"thrust1.wav",
	"slaser.wav",
	"shield.wav",
	"bounce.wav",
	"hypersp.wav",
    NULL
};

static struct Samplesinterface tailg_samples_interface =
{
	6,
	tailg_sample_names
};

static void tailg_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* the falling edge of bit 0x10 clocks bit 0x08 into the mux selected by bits 0x07 */
	if (SOUNDVAL_FALLING_EDGE(0x10))
	{
		/* update the shift register (actually just a simple mux) */
		current_shift = (current_shift & ~(1 << (sound_val & 7))) | (((sound_val >> 3) & 1) << (sound_val & 7));

		/* explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x01))
			sample_start(0, 0, 0);

		/* rumble - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x02))
			sample_start(1, 1, 1);
		if (SHIFTREG_RISING_EDGE(0x02))
			sample_stop(1);

		/* laser - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x04))
			sample_start(2, 2, 1);
		if (SHIFTREG_RISING_EDGE(0x04))
			sample_stop(2);

		/* shield - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x08))
			sample_start(3, 3, 1);
		if (SHIFTREG_RISING_EDGE(0x08))
			sample_stop(3);

		/* bounce - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(4, 4, 0);

		/* hyperspace - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_start(5, 5, 0);

		/* LED */
		set_led_status(0, current_shift & 0x40);

		/* remember the previous value */
		last_shift = current_shift;
	}
}

static MACHINE_RESET( tailg )
{
	generic_init(tailg_sound_w);
}

MACHINE_DRIVER_START( tailg_sound )
	MDRV_MACHINE_RESET(tailg)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(tailg_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Warrior
 *
 *************************************/

static const char *warrior_sample_names[] =
{
	"*warrior",
	"bgmhum1.wav",
	"bgmhum2.wav",
	"killed.wav",
	"fall.wav",
	"appear.wav",
    NULL
};

static struct Samplesinterface warrior_samples_interface =
{
	5,
	warrior_sample_names
};

static void warrior_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* normal level - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x01))
		sample_start(0, 0, 1);
	if (SOUNDVAL_RISING_EDGE(0x01))
		sample_stop(0);

	/* hi level - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(1, 1, 1);
	if (SOUNDVAL_RISING_EDGE(0x02))
		sample_stop(1);

	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(2, 2, 0);

	/* fall - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(3, 3, 0);

	/* appear - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_start(4, 4, 0);
}

static MACHINE_RESET( warrior )
{
	generic_init(warrior_sound_w);
}

MACHINE_DRIVER_START( warrior_sound )
	MDRV_MACHINE_RESET(warrior)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(warrior_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Armor Attack
 *
 *************************************/

static const char *armora_sample_names[] =
{
	"*armora",
    "loexp.wav",
    "jeepfire.wav",
	"hiexp.wav",
	"tankfire.wav",
	"tankeng.wav",
	"beep.wav",
	"chopper.wav",
    NULL
};

static struct Samplesinterface armora_samples_interface =
{
	7,
	armora_sample_names
};

static void armora_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* bits 0-4 control the tank sound speed */

		/* lo explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(0, 0, 0);

		/* jeep fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_start(1, 1, 0);

		/* hi explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(2, 2, 0);

		/* tank fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(3, 3, 0);

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* tank sound - 0=on, 1=off */
	/* still not totally correct - should be multiple speeds based on remaining bits in shift reg */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(4, 4, 1);
	if (SOUNDVAL_RISING_EDGE(0x02))
		sample_stop(4);

	/* beep sound - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(5, 5, 1);
	if (SOUNDVAL_RISING_EDGE(0x04))
		sample_stop(5);

	/* chopper sound - 0=on, 1=off */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(6, 6, 1);
	if (SOUNDVAL_RISING_EDGE(0x08))
		sample_stop(6);
}

static MACHINE_RESET( armora )
{
	generic_init(armora_sound_w);
}

MACHINE_DRIVER_START( armora_sound )
	MDRV_MACHINE_RESET(armora)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(armora_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Ripoff
 *
 *************************************/

static const char *ripoff_sample_names[] =
{
	"*ripoff",
	"bonuslvl.wav",
	"eattack.wav",
	"shipfire.wav",
    "efire.wav",
	"explosn.wav",
	"bg1.wav",
	"bg2.wav",
	"bg3.wav",
	"bg4.wav",
	"bg5.wav",
	"bg6.wav",
	"bg7.wav",
	"bg8.wav",
    NULL
};

static struct Samplesinterface ripoff_samples_interface =
{
	6,
	ripoff_sample_names
};

static void ripoff_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x02, clock bit 0x01 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x02))
		current_shift = ((current_shift >> 1) & 0x7f) | ((sound_val << 7) & 0x80);

	/* execute on the rising edge of bit 0x04 */
	if (SOUNDVAL_RISING_EDGE(0x04))
	{
		/* background - 0=on, 1=off, selected by bits 0x38 */
		if ((((current_shift ^ last_shift) & 0x38) && !(current_shift & 0x04)) || SHIFTREG_FALLING_EDGE(0x04))
			sample_start(5, 5 + ((current_shift >> 3) & 7), 1);
		if (SHIFTREG_RISING_EDGE(0x04))
			sample_stop(5);

		/* beep - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			sample_start(0, 0, 0);

		/* motor - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x01))
			sample_start(1, 1, 1);
		if (SHIFTREG_RISING_EDGE(0x01))
			sample_stop(1);

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* torpedo - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(2, 2, 0);

	/* laser - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x10))
		sample_start(3, 3, 0);

	/* explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x80))
		sample_start(4, 4, 0);
}

static MACHINE_RESET( ripoff )
{
	generic_init(ripoff_sound_w);
}

MACHINE_DRIVER_START( ripoff_sound )
	MDRV_MACHINE_RESET(ripoff)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(ripoff_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Star Castle
 *
 *************************************/

static const char *starcas_sample_names[] =
{
	"*starcas",
	"cfire.wav",
	"shield.wav",
	"star.wav",
	"thrust.wav",
	"drone.wav",
	"lexplode.wav",
	"sexplode.wav",
	"pfire.wav",
    0
};

static struct Samplesinterface starcas_samples_interface =
{
	8,
	starcas_sample_names
};

static void starcas_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
    UINT32 target_pitch;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* fireball - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(0, 0, 0);

		/* shield hit - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(1, 1, 0);

		/* star sound - 0=off, 1=on */
		if (SHIFTREG_RISING_EDGE(0x20))
			sample_start(2, 2, 1);
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_stop(2);

		/* thrust sound - 1=off, 0=on*/
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(3, 3, 1);
		if (SHIFTREG_RISING_EDGE(0x10))
			sample_stop(3);

		/* drone - 1=off, 0=on */
		if (SHIFTREG_FALLING_EDGE(0x08))
			sample_start(4, 4, 1);
		if (SHIFTREG_RISING_EDGE(0x08))
			sample_stop(4);

		/* latch the drone pitch */
		target_pitch = (current_shift & 7) + ((current_shift & 2) << 2);
        target_pitch = 0x10000 + (target_pitch << 12);

        /* once per frame slide the pitch toward the target */
        if (cpu_getcurrentframe() > last_frame)
        {
            if (current_pitch > target_pitch)
                current_pitch -= 300;
            if (current_pitch < target_pitch)
                current_pitch += 200;
            sample_set_freq(4, current_pitch);
            last_frame = cpu_getcurrentframe();
        }

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* loud explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(5, 5, 0);

	/* soft explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(6, 6, 0);

	/* player fire - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(7, 7, 0);
}

static MACHINE_RESET( starcas )
{
	generic_init(starcas_sound_w);
}

MACHINE_DRIVER_START( starcas_sound )
	MDRV_MACHINE_RESET(starcas)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(starcas_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END



/*************************************
 *
 *  Solar Quest
 *
 *************************************/

static const char *solarq_sample_names[] =
{
	"*solarq",
    "bigexpl.wav",
	"smexpl.wav",
	"lthrust.wav",
	"slaser.wav",
	"pickup.wav",
	"nuke2.wav",
	"nuke1.wav",
    "music.wav",
    NULL
};

static struct Samplesinterface solarq_samples_interface =
{
	8,
	solarq_sample_names
};

static void solarq_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
    static float target_volume, current_volume;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7fff) | ((sound_val << 8) & 0x8000);

	/* execute on the rising edge of bit 0x02 */
	if (SOUNDVAL_RISING_EDGE(0x02))
	{
		/* only the upper 8 bits matter */
		current_shift >>= 8;

		/* loud explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(0, 0, 0);

		/* soft explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(1, 1, 0);

		/* thrust - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x20))
		{
			target_volume = 1.0;
			if (!sample_playing(2))
				sample_start(2, 2, 1);
		}
		if (SHIFTREG_RISING_EDGE(0x20))
			target_volume = 0;

		/* ramp the thrust volume */
        if (sample_playing(2) && cpu_getcurrentframe() > last_frame)
        {
            if (current_volume > target_volume)
                current_volume -= 0.078;
            if (current_volume < target_volume)
                current_volume += 0.078;
            if (current_volume > 0)
                sample_set_volume(2, current_volume);
            else
                sample_stop(2);
            last_frame = cpu_getcurrentframe();
        }

		/* fire - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(3, 3, 0);

		/* capture - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x08))
			sample_start(4, 4, 0);

		/* nuke - 1=on, 0=off */
		if (SHIFTREG_RISING_EDGE(0x04))
			sample_start(5, 5, 1);
		if (SHIFTREG_FALLING_EDGE(0x04))
			sample_stop(5);

		/* photon - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			sample_start(6, 6, 0);

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* clock music data on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		int freq, vol;

		/* start/stop the music sample on the high bit */
		if (SHIFTREG2_RISING_EDGE(0x8000))
			sample_start(7, 7, 1);
		if (SHIFTREG2_FALLING_EDGE(0x8000))
			sample_stop(7);

		/* set the frequency */
		freq = 56818.181818 / (4096 - (current_shift & 0xfff));
		sample_set_freq(7, 44100 * freq / 1050);

		/* set the volume */
		vol = (~current_shift >> 12) & 7;
		sample_set_volume(7, vol / 7.0);

		/* remember the previous value */
		last_shift2 = current_shift;
    }
}

static MACHINE_RESET( solarq )
{
	generic_init(solarq_sound_w);
}

MACHINE_DRIVER_START( solarq_sound )
	MDRV_MACHINE_RESET(solarq)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(solarq_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END



/*************************************
 *
 *  Boxing Bugs
 *
 *************************************/

static const char *boxingb_sample_names[] =
{
	"*boxingb",
    "softexpl.wav",
	"loudexpl.wav",
	"chirp.wav",
	"eggcrack.wav",
	"bugpusha.wav",
	"bugpushb.wav",
	"bugdie.wav",
    "beetle.wav",
    "music.wav",
    "cannon.wav",
    "bounce.wav",
    "bell.wav",
    NULL
};

static struct Samplesinterface boxingb_samples_interface =
{
	12,
	boxingb_sample_names
};

static void boxingb_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7fff) | ((sound_val << 8) & 0x8000);

	/* execute on the rising edge of bit 0x02 */
	if (SOUNDVAL_RISING_EDGE(0x02))
	{
		/* only the upper 8 bits matter */
		current_shift >>= 8;

		/* soft explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(0, 0, 0);

		/* loud explosion - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(1, 1, 0);

		/* chirping birds - 0=on, 1=off */
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_start(2, 2, 0);
		if (SHIFTREG_RISING_EDGE(0x20))
			sample_stop(2);

		/* egg cracking - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(3, 3, 0);

		/* bug pushing A - rising edge */
		if (SHIFTREG_RISING_EDGE(0x08))
			sample_start(4, 4, 0);

		/* bug pushing B - rising edge */
		if (SHIFTREG_RISING_EDGE(0x04))
			sample_start(5, 5, 0);

		/* bug dying - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x02))
			sample_start(6, 6, 0);

		/* beetle on screen - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x01))
			sample_start(7, 7, 0);

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* clock music data on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		int freq, vol;

		/* start/stop the music sample on the high bit */
		if (SHIFTREG2_RISING_EDGE(0x8000))
			sample_start(8, 8, 1);
		if (SHIFTREG2_FALLING_EDGE(0x8000))
			sample_stop(8);

		/* set the frequency */
		freq = 56818.181818 / (4096 - (current_shift & 0xfff));
		sample_set_freq(8, 44100 * freq / 1050);

		/* set the volume */
		vol = (~current_shift >> 12) & 3;
		sample_set_volume(8, vol / 3.0);

        /* cannon - falling edge */
        if (SHIFTREG2_RISING_EDGE(0x4000))
        	sample_start(9, 9, 0);

		/* remember the previous value */
		last_shift2 = current_shift;
    }

	/* bounce - rising edge */
	if (SOUNDVAL_RISING_EDGE(0x04))
		sample_start(10, 10, 0);

	/* bell - falling edge */
	if (SOUNDVAL_RISING_EDGE(0x08))
		sample_start(11, 11, 0);
}

static MACHINE_RESET( boxingb )
{
	generic_init(boxingb_sound_w);
}

MACHINE_DRIVER_START( boxingb_sound )
	MDRV_MACHINE_RESET(boxingb)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(boxingb_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  War of the Worlds (B&W)
 *
 *************************************/

static const char *wotw_sample_names[] =
{
	"*wotw",
	"cfire.wav",
	"shield.wav",
	"star.wav",
	"thrust.wav",
	"drone.wav",
	"lexplode.wav",
	"sexplode.wav",
	"pfire.wav",
    0
};

static struct Samplesinterface wotw_samples_interface =
{
	8,
	wotw_sample_names
};

static void wotw_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
    UINT32 target_pitch;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* fireball - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(0, 0, 0);

		/* shield hit - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(1, 1, 0);

		/* star sound - 0=off, 1=on */
		if (SHIFTREG_RISING_EDGE(0x20))
			sample_start(2, 2, 1);
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_stop(2);

		/* thrust sound - 1=off, 0=on*/
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(3, 3, 1);
		if (SHIFTREG_RISING_EDGE(0x10))
			sample_stop(3);

		/* drone - 1=off, 0=on */
		if (SHIFTREG_FALLING_EDGE(0x08))
			sample_start(4, 4, 1);
		if (SHIFTREG_RISING_EDGE(0x08))
			sample_stop(4);

		/* latch the drone pitch */
		target_pitch = (current_shift & 7) + ((current_shift & 2) << 2);
        target_pitch = 0x10000 + (target_pitch << 12);

        /* once per frame slide the pitch toward the target */
        if (cpu_getcurrentframe() > last_frame)
        {
            if (current_pitch > target_pitch)
                current_pitch -= 300;
            if (current_pitch < target_pitch)
                current_pitch += 200;
            sample_set_freq(4, current_pitch);
            last_frame = cpu_getcurrentframe();
        }

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* loud explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(5, 5, 0);

	/* soft explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(6, 6, 0);

	/* player fire - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(7, 7, 0);
}

static MACHINE_RESET( wotw )
{
	generic_init(wotw_sound_w);
}

MACHINE_DRIVER_START( wotw_sound )
	MDRV_MACHINE_RESET(wotw)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(wotw_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  War of the Worlds (color)
 *
 *************************************/

static const char *wotwc_sample_names[] =
{
	"*wotwc",
	"cfire.wav",
	"shield.wav",
	"star.wav",
	"thrust.wav",
	"drone.wav",
	"lexplode.wav",
	"sexplode.wav",
	"pfire.wav",
    0
};

static struct Samplesinterface wotwc_samples_interface =
{
	8,
	wotwc_sample_names
};

static void wotwc_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
    UINT32 target_pitch;

	/* on the rising edge of bit 0x10, clock bit 0x80 into the shift register */
	if (SOUNDVAL_RISING_EDGE(0x10))
		current_shift = ((current_shift >> 1) & 0x7f) | (sound_val & 0x80);

	/* execute on the rising edge of bit 0x01 */
	if (SOUNDVAL_RISING_EDGE(0x01))
	{
		/* fireball - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x80))
			sample_start(0, 0, 0);

		/* shield hit - falling edge */
		if (SHIFTREG_FALLING_EDGE(0x40))
			sample_start(1, 1, 0);

		/* star sound - 0=off, 1=on */
		if (SHIFTREG_RISING_EDGE(0x20))
			sample_start(2, 2, 1);
		if (SHIFTREG_FALLING_EDGE(0x20))
			sample_stop(2);

		/* thrust sound - 1=off, 0=on*/
		if (SHIFTREG_FALLING_EDGE(0x10))
			sample_start(3, 3, 1);
		if (SHIFTREG_RISING_EDGE(0x10))
			sample_stop(3);

		/* drone - 1=off, 0=on */
		if (SHIFTREG_FALLING_EDGE(0x08))
			sample_start(4, 4, 1);
		if (SHIFTREG_RISING_EDGE(0x08))
			sample_stop(4);

		/* latch the drone pitch */
		target_pitch = (current_shift & 7) + ((current_shift & 2) << 2);
        target_pitch = 0x10000 + (target_pitch << 12);

        /* once per frame slide the pitch toward the target */
        if (cpu_getcurrentframe() > last_frame)
        {
            if (current_pitch > target_pitch)
                current_pitch -= 300;
            if (current_pitch < target_pitch)
                current_pitch += 200;
            sample_set_freq(4, current_pitch);
            last_frame = cpu_getcurrentframe();
        }

		/* remember the previous value */
		last_shift = current_shift;
	}

	/* loud explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x02))
		sample_start(5, 5, 0);

	/* soft explosion - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x04))
		sample_start(6, 6, 0);

	/* player fire - falling edge */
	if (SOUNDVAL_FALLING_EDGE(0x08))
		sample_start(7, 7, 0);
}

static MACHINE_RESET( wotwc )
{
	generic_init(wotwc_sound_w);
}

MACHINE_DRIVER_START( wotwc_sound )
	MDRV_MACHINE_RESET(wotwc)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(wotwc_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



/*************************************
 *
 *  Demon
 *
 *************************************/

static void synced_sound_w(int param)
{
	sound_fifo[sound_fifo_in] = param;
	sound_fifo_in = (sound_fifo_in + 1) % 16;
}


static void demon_sound_w(UINT8 sound_val, UINT8 bits_changed)
{
	/* all inputs are inverted */
	sound_val = ~sound_val;

	/* watch for a 0->1 edge on bit 4 ("shift in") to clock in the new data */
	if ((bits_changed & 0x10) && (sound_val & 0x10))
		timer_set(TIME_NOW, sound_val & 0x0f, synced_sound_w);
}


static READ8_HANDLER( sound_porta_r )
{
	/* bits 0-3 are the sound data; bit 4 is the data ready */
	return sound_fifo[sound_fifo_out] | ((sound_fifo_in != sound_fifo_out) << 4);
}


static READ8_HANDLER( sound_portb_r )
{
	return last_portb_write;
}


static WRITE8_HANDLER( sound_portb_w )
{
	/* watch for a 0->1 edge on bit 0 ("shift out") to advance the data pointer */
	if ((data & 1) != (last_portb_write & 1) && (data & 1) != 0)
		sound_fifo_out = (sound_fifo_out + 1) % 16;

	/* watch for a 0->1 edge of bit 1 ("hard reset") to reset the FIFO */
	if ((data & 2) != (last_portb_write & 2) && (data & 2) != 0)
		sound_fifo_in = sound_fifo_out = 0;

	/* bit 2 controls the global mute */
	if ((data & 4) != (last_portb_write & 4))
		sound_global_enable(!(data & 4));

	/* remember the last value written */
	last_portb_write = data;
}


static WRITE8_HANDLER( sound_output_w )
{
	logerror("sound_output = %02X\n", data);
}


static struct AY8910interface demon_ay8910_interface_1 =
{
	sound_porta_r,
	sound_portb_r,
	0,
	sound_portb_w
};

static struct AY8910interface demon_ay8910_interface_3 =
{
	0,
	0,
	0,
	sound_output_w
};


static void ctc_interrupt(int state)
{
	cpunum_set_input_line(1, 0, state);
}


static z80ctc_interface demon_z80ctc_interface =
{
	0,               /* clock (filled in from the CPU clock) */
	0,               /* timer disables */
	ctc_interrupt,   /* interrupt handler */
	0,               /* ZC/TO0 callback */
	0,               /* ZC/TO1 callback */
	0                /* ZC/TO2 callback */
};


static MACHINE_RESET( demon_sound )
{
	/* generic init */
	generic_init(demon_sound_w);

	/* initialize the CTC interface */
	demon_z80ctc_interface.baseclock = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init(0, &demon_z80ctc_interface);

	/* reset the FIFO */
	sound_fifo_in = sound_fifo_out = 0;
	last_portb_write = 0xff;

	/* turn off channel A on AY8910 #0 because it is used as a low-pass filter */
	AY8910_set_volume(0, 0, 0);
}


ADDRESS_MAP_START( demon_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x3000, 0x33ff) AM_RAM
	AM_RANGE(0x4001, 0x4001) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x4002, 0x4002) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4003, 0x4003) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x5001, 0x5001) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x5002, 0x5002) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x6001, 0x6001) AM_READ(AY8910_read_port_2_r)
	AM_RANGE(0x6002, 0x6002) AM_WRITE(AY8910_write_port_2_w)
	AM_RANGE(0x6003, 0x6003) AM_WRITE(AY8910_control_port_2_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(MWA8_NOP)  /* watchdog? */
ADDRESS_MAP_END


ADDRESS_MAP_START( demon_sound_ports, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_WRITE(z80ctc_0_w)
	AM_RANGE(0x1c, 0x1f) AM_WRITE(z80ctc_0_w)
ADDRESS_MAP_END


static struct z80_irq_daisy_chain daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0,0,0,0,-1 }
};


MACHINE_DRIVER_START( demon_sound )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */
	MDRV_CPU_CONFIG(daisy_chain)
	MDRV_CPU_PROGRAM_MAP(demon_sound_map,0)
	MDRV_CPU_IO_MAP(demon_sound_ports,0)

	MDRV_MACHINE_RESET(demon_sound)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 3579545)
	MDRV_SOUND_CONFIG(demon_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 3579545)
	MDRV_SOUND_CONFIG(demon_ay8910_interface_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END



/*************************************
 *
 *  QB3
 *
 *************************************/

static WRITE8_HANDLER( qb3_sound_w )
{
	UINT16 rega = cpunum_get_reg(0, CCPU_A);
	demon_sound_w(0x00 | (~rega & 0x0f), 0x10);
}


static MACHINE_RESET( qb3_sound )
{
	machine_reset_demon_sound();
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x04, 0x04, 0, 0, qb3_sound_w);

	/* this patch prevents the sound ROM from eating itself when command $0A is sent */
	/* on a cube rotate */
	memory_region(REGION_CPU2)[0x11dc] = 0x09;
}


MACHINE_DRIVER_START( qb3_sound )
	MDRV_IMPORT_FROM(demon_sound)
	MDRV_MACHINE_RESET(qb3_sound)
MACHINE_DRIVER_END
