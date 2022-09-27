/*************************************************************************

    Atari Pool Shark hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */

/*----------- defined in sndhrdw/poolshrk.c -----------*/

WRITE8_HANDLER( poolshrk_scratch_sound_w );
WRITE8_HANDLER( poolshrk_score_sound_w );
WRITE8_HANDLER( poolshrk_click_sound_w );
WRITE8_HANDLER( poolshrk_bump_sound_w );

extern struct discrete_sound_block poolshrk_discrete_interface[];


/*----------- defined in vidhrdw/poolshrk.c -----------*/

VIDEO_START( poolshrk );
VIDEO_UPDATE( poolshrk );

extern UINT8* poolshrk_playfield_ram;
extern UINT8* poolshrk_hpos_ram;
extern UINT8* poolshrk_vpos_ram;
