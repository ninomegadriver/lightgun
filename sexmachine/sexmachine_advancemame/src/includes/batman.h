/*************************************************************************

    Atari Batman hardware

*************************************************************************/

/*----------- defined in vidhrdw/batman.c -----------*/

extern UINT8 batman_alpha_tile_bank;

VIDEO_START( batman );
VIDEO_UPDATE( batman );

void batman_scanline_update(int scanline);
