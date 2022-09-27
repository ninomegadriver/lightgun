#include "sound/discrete.h"

/*----------- defined in vidhrdw/blockade.c -----------*/

WRITE8_HANDLER( blockade_videoram_w );

VIDEO_START( blockade );
VIDEO_UPDATE( blockade );

/*----------- defined in sndhrdw/blockade.c -----------*/

extern struct Samplesinterface blockade_samples_interface;
extern struct discrete_sound_block blockade_discrete_interface[];

WRITE8_HANDLER( blockade_sound_freq_w );
WRITE8_HANDLER( blockade_env_on_w );
WRITE8_HANDLER( blockade_env_off_w );
