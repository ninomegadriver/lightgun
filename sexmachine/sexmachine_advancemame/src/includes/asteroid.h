/*************************************************************************

    Atari Asteroids hardware

*************************************************************************/

#include "sound/discrete.h"

/*----------- defined in machine/asteroid.c -----------*/

INTERRUPT_GEN( asteroid_interrupt );
INTERRUPT_GEN( asterock_interrupt );
INTERRUPT_GEN( llander_interrupt );

READ8_HANDLER( asteroid_IN0_r );
READ8_HANDLER( asteroib_IN0_r );
READ8_HANDLER( asterock_IN0_r );
READ8_HANDLER( asteroid_IN1_r );
READ8_HANDLER( asteroid_DSW1_r );

WRITE8_HANDLER( asteroid_bank_switch_w );
WRITE8_HANDLER( astdelux_bank_switch_w );
WRITE8_HANDLER( astdelux_led_w );

MACHINE_RESET( asteroid );

READ8_HANDLER( llander_IN0_r );


/*----------- defined in sndhrdw/asteroid.c -----------*/

extern struct discrete_sound_block asteroid_discrete_interface[];
extern struct discrete_sound_block astdelux_discrete_interface[];

WRITE8_HANDLER( asteroid_explode_w );
WRITE8_HANDLER( asteroid_thump_w );
WRITE8_HANDLER( asteroid_sounds_w );
WRITE8_HANDLER( asteroid_noise_reset_w );
WRITE8_HANDLER( astdelux_sounds_w );


/*----------- defined in sndhrdw/llander.c -----------*/

extern struct discrete_sound_block llander_discrete_interface[];

WRITE8_HANDLER( llander_snd_reset_w );
WRITE8_HANDLER( llander_sounds_w );
