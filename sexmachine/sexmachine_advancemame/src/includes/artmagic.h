/*************************************************************************

    Art & Magic hardware

**************************************************************************/

/*----------- defined in vidhrdw/artmagic.c -----------*/

extern UINT16 *artmagic_vram0;
extern UINT16 *artmagic_vram1;

extern int artmagic_xor[16], artmagic_is_stoneball;

VIDEO_START( artmagic );

void artmagic_to_shiftreg(offs_t address, UINT16 *data);
void artmagic_from_shiftreg(offs_t address, UINT16 *data);

READ16_HANDLER( artmagic_blitter_r );
WRITE16_HANDLER( artmagic_blitter_w );

VIDEO_UPDATE( artmagic );
