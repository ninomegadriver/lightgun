/*************************************************************************

    Driver for Midway MCR games

**************************************************************************/

#include "cpu/z80/z80daisy.h"

/* constants */
#define MAIN_OSC_MCR_I		19968000


/*----------- defined in drivers/mcr.c -----------*/

WRITE8_HANDLER( mcr_ipu_sio_transmit );



/*----------- defined in machine/mcr.c -----------*/

extern double mcr68_timing_factor;

extern struct z80_irq_daisy_chain mcr_daisy_chain[];
extern struct z80_irq_daisy_chain mcr_ipu_daisy_chain[];
extern UINT8 mcr_cocktail_flip;

extern gfx_layout mcr_bg_layout;
extern gfx_layout mcr_sprite_layout;

extern UINT32 mcr_cpu_board;
extern UINT32 mcr_sprite_board;
extern UINT32 mcr_ssio_board;

MACHINE_START( mcr );
MACHINE_RESET( mcr );
MACHINE_START( nflfoot );
MACHINE_START( mcr68 );
MACHINE_RESET( mcr68 );
MACHINE_START( zwackery );
MACHINE_RESET( zwackery );

INTERRUPT_GEN( mcr_interrupt );
INTERRUPT_GEN( mcr_ipu_interrupt );
INTERRUPT_GEN( mcr68_interrupt );

WRITE8_HANDLER( mcr_control_port_w );
WRITE8_HANDLER( mcrmono_control_port_w );
WRITE8_HANDLER( mcr_scroll_value_w );

WRITE16_HANDLER( mcr68_6840_upper_w );
WRITE16_HANDLER( mcr68_6840_lower_w );
READ16_HANDLER( mcr68_6840_upper_r );
READ16_HANDLER( mcr68_6840_lower_r );

READ8_HANDLER( mcr_ipu_pio_0_r );
READ8_HANDLER( mcr_ipu_pio_1_r );
READ8_HANDLER( mcr_ipu_sio_r );
WRITE8_HANDLER( mcr_ipu_pio_0_w );
WRITE8_HANDLER( mcr_ipu_pio_1_w );
WRITE8_HANDLER( mcr_ipu_sio_w );
WRITE8_HANDLER( mcr_ipu_laserdisk_w );
READ8_HANDLER( mcr_ipu_watchdog_r );
WRITE8_HANDLER( mcr_ipu_watchdog_w );


/*----------- defined in vidhrdw/mcr.c -----------*/

extern INT8 mcr12_sprite_xoffs;
extern INT8 mcr12_sprite_xoffs_flip;

VIDEO_START( mcr );

WRITE8_HANDLER( mcr_91490_paletteram_w );

WRITE8_HANDLER( mcr_90009_videoram_w );
WRITE8_HANDLER( mcr_90010_videoram_w );
READ8_HANDLER( twotiger_videoram_r );
WRITE8_HANDLER( twotiger_videoram_w );
WRITE8_HANDLER( mcr_91490_videoram_w );

VIDEO_UPDATE( mcr );


/*----------- defined in vidhrdw/mcr3.c -----------*/

extern UINT8 spyhunt_sprite_color_mask;
extern INT16 spyhunt_scrollx, spyhunt_scrolly;
extern INT16 spyhunt_scroll_offset;

extern UINT8 *spyhunt_alpharam;

WRITE8_HANDLER( mcr3_paletteram_w );
WRITE8_HANDLER( mcr3_videoram_w );
WRITE8_HANDLER( spyhunt_videoram_w );
WRITE8_HANDLER( spyhunt_alpharam_w );

void mcr3_update_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int color_mask, int code_xor, int dx, int dy);

VIDEO_START( mcr3 );
VIDEO_START( mcrmono );
VIDEO_START( spyhunt );

PALETTE_INIT( spyhunt );

VIDEO_UPDATE( mcr3 );
VIDEO_UPDATE( spyhunt );


/*----------- defined in vidhrdw/mcr68.c -----------*/

extern UINT8 mcr68_sprite_clip;
extern INT8 mcr68_sprite_xoffset;

WRITE16_HANDLER( mcr68_paletteram_w );
WRITE16_HANDLER( mcr68_videoram_w );

VIDEO_START( mcr68 );
VIDEO_UPDATE( mcr68 );

WRITE16_HANDLER( zwackery_paletteram_w );
WRITE16_HANDLER( zwackery_videoram_w );
WRITE16_HANDLER( zwackery_spriteram_w );

VIDEO_START( zwackery );
VIDEO_UPDATE( zwackery );
