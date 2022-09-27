/*************************************************************************

    Atari tank8 hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define TANK8_CRASH_EN			NODE_01
#define TANK8_BUGLE_EN			NODE_02
#define TANK8_MOTOR1_EN			NODE_03
#define TANK8_MOTOR2_EN			NODE_04
#define TANK8_MOTOR3_EN			NODE_05
#define TANK8_MOTOR4_EN			NODE_06
#define TANK8_MOTOR5_EN			NODE_07
#define TANK8_MOTOR6_EN			NODE_08
#define TANK8_MOTOR7_EN			NODE_09
#define TANK8_MOTOR8_EN			NODE_10
#define TANK8_EXPLOSION_EN		NODE_11
#define TANK8_ATTRACT_EN		NODE_12
#define TANK8_BUGLE_DATA1		NODE_13
#define TANK8_BUGLE_DATA2		NODE_14


/*----------- defined in sndhrdw/tank8.c -----------*/

extern struct discrete_sound_block tank8_discrete_interface[];


/*----------- defined in drivers/tank8.c -----------*/

extern void tank8_collision_callback(int index);

/*----------- defined in vidhrdw/tank8.c -----------*/

VIDEO_EOF( tank8 );
VIDEO_START( tank8 );
VIDEO_UPDATE( tank8 );

WRITE8_HANDLER( tank8_video_ram_w );

extern UINT8* tank8_video_ram;
extern UINT8* tank8_pos_h_ram;
extern UINT8* tank8_pos_v_ram;
extern UINT8* tank8_pos_d_ram;

