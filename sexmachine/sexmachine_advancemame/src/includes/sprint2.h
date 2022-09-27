/*************************************************************************

    Atari Sprint hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define SPRINT2_SKIDSND1_EN        NODE_01
#define SPRINT2_SKIDSND2_EN        NODE_02
#define SPRINT2_MOTORSND1_DATA     NODE_03
#define SPRINT2_MOTORSND2_DATA     NODE_04
#define SPRINT2_CRASHSND_DATA      NODE_05
#define SPRINT2_ATTRACT_EN         NODE_06
#define SPRINT2_NOISE_RESET        NODE_07

#define DOMINOS_FREQ_DATA          SPRINT2_MOTORSND1_DATA
#define DOMINOS_AMP_DATA           SPRINT2_CRASHSND_DATA
#define DOMINOS_TUMBLE_EN          SPRINT2_SKIDSND1_EN
#define DOMINOS_ATTRACT_EN         SPRINT2_ATTRACT_EN


/*----------- defined in sndhrdw/sprint2.c -----------*/

extern struct discrete_sound_block sprint2_discrete_interface[];
extern struct discrete_sound_block sprint1_discrete_interface[];
extern struct discrete_sound_block dominos_discrete_interface[];


/*----------- defined in vidhrdw/sprint2.c -----------*/

extern READ8_HANDLER( sprint2_collision1_r );
extern READ8_HANDLER( sprint2_collision2_r );

extern WRITE8_HANDLER( sprint2_collision_reset1_w );
extern WRITE8_HANDLER( sprint2_collision_reset2_w );
extern WRITE8_HANDLER( sprint2_video_ram_w );

extern VIDEO_UPDATE( sprint2 );
extern VIDEO_START( sprint2 );
extern VIDEO_EOF( sprint2 );

extern UINT8* sprint2_video_ram;
