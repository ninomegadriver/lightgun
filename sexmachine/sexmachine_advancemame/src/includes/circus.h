#include "sound/discrete.h"

#define GAME_IS_CIRCUS		(circus_game == 1)
#define GAME_IS_ROBOTBWL	(circus_game == 2)
#define GAME_IS_CRASH		(circus_game == 3)
#define GAME_IS_RIPCORD		(circus_game == 4)

/*----------- defined in drivers/circus.c -----------*/

extern int circus_game;

/*----------- defined in sndhrdw/circus.c -----------*/

extern WRITE8_HANDLER( circus_clown_z_w );

extern struct discrete_sound_block circus_discrete_interface[];
extern struct discrete_sound_block robotbwl_discrete_interface[];
extern struct discrete_sound_block crash_discrete_interface[];
extern struct Samplesinterface circus_samples_interface;
extern struct Samplesinterface crash_samples_interface;
extern struct Samplesinterface ripcord_samples_interface;
extern struct Samplesinterface robotbwl_samples_interface;

/*----------- defined in vidhrdw/circus.c -----------*/

extern int clown_z;

extern WRITE8_HANDLER( circus_clown_x_w );
extern WRITE8_HANDLER( circus_clown_y_w );

extern WRITE8_HANDLER( circus_videoram_w );

extern VIDEO_START( circus );
extern VIDEO_UPDATE( crash );
extern VIDEO_UPDATE( circus );
extern VIDEO_UPDATE( robotbwl );
extern VIDEO_UPDATE( ripcord ); //AT
