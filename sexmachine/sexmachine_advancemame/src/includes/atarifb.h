/*************************************************************************

    Atari Football hardware

*************************************************************************/

#include "sound/discrete.h"

#define GAME_IS_FOOTBALL   (atarifb_game == 1)
#define GAME_IS_FOOTBALL4  (atarifb_game == 2)
#define GAME_IS_BASEBALL   (atarifb_game == 3)
#define GAME_IS_SOCCER     (atarifb_game == 4)

/* Discrete Sound Input Nodes */
#define ATARIFB_WHISTLE_EN		NODE_01
#define ATARIFB_CROWD_DATA		NODE_02
#define ATARIFB_ATTRACT_EN		NODE_03
#define ATARIFB_NOISE_EN		NODE_04
#define ATARIFB_HIT_EN			NODE_05


/*----------- defined in drivers/atarifb.c -----------*/

extern int atarifb_game;
extern int atarifb_lamp1, atarifb_lamp2;


/*----------- defined in machine/atarifb.c -----------*/

WRITE8_HANDLER( atarifb_out1_w );
WRITE8_HANDLER( atarifb_out2_w );
WRITE8_HANDLER( atarifb_out3_w );
READ8_HANDLER( atarifb_in0_r );
READ8_HANDLER( atarifb_in2_r );
READ8_HANDLER( atarifb4_in0_r );
READ8_HANDLER( atarifb4_in2_r );


/*----------- defined in sndhrdw/atarifb.c -----------*/

extern struct discrete_sound_block atarifb_discrete_interface[];
extern struct discrete_sound_block abaseb_discrete_interface[];


/*----------- defined in vidhrdw/atarifb.c -----------*/

extern size_t atarifb_alphap1_vram_size;
extern size_t atarifb_alphap2_vram_size;
extern unsigned char *atarifb_alphap1_vram;
extern unsigned char *atarifb_alphap2_vram;
extern unsigned char *atarifb_scroll_register;

WRITE8_HANDLER( atarifb_scroll_w );
WRITE8_HANDLER( atarifb_alphap1_vram_w );
WRITE8_HANDLER( atarifb_alphap2_vram_w );

VIDEO_START( atarifb );
VIDEO_UPDATE( atarifb );
