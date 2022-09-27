#ifndef OKIM6295_H
#define OKIM6295_H

/* an interface for the OKIM6295 and similar chips */

/*
  Note about the playback frequency: the external clock is internally divided,
  depending on pin 7, by 132 (high) or 165 (low). This isn't handled by the
  emulation, so you have to provide the didvided internal clock instead of the
  external clock.
*/
struct OKIM6295interface
{
	int region;		/* memory region where the sample ROM lives */
};

extern const struct OKIM6295interface okim6295_interface_region_1;
extern const struct OKIM6295interface okim6295_interface_region_2;
extern const struct OKIM6295interface okim6295_interface_region_3;
extern const struct OKIM6295interface okim6295_interface_region_4;

void OKIM6295_set_bank_base(int which, int base);
void OKIM6295_set_frequency(int which, int frequency);

READ8_HANDLER( OKIM6295_status_0_r );
READ8_HANDLER( OKIM6295_status_1_r );
READ8_HANDLER( OKIM6295_status_2_r );
READ16_HANDLER( OKIM6295_status_0_lsb_r );
READ16_HANDLER( OKIM6295_status_1_lsb_r );
READ16_HANDLER( OKIM6295_status_2_lsb_r );
READ16_HANDLER( OKIM6295_status_0_msb_r );
READ16_HANDLER( OKIM6295_status_1_msb_r );
READ16_HANDLER( OKIM6295_status_2_msb_r );
WRITE8_HANDLER( OKIM6295_data_0_w );
WRITE8_HANDLER( OKIM6295_data_1_w );
WRITE8_HANDLER( OKIM6295_data_2_w );
WRITE16_HANDLER( OKIM6295_data_0_lsb_w );
WRITE16_HANDLER( OKIM6295_data_1_lsb_w );
WRITE16_HANDLER( OKIM6295_data_2_lsb_w );
WRITE16_HANDLER( OKIM6295_data_0_msb_w );
WRITE16_HANDLER( OKIM6295_data_1_msb_w );
WRITE16_HANDLER( OKIM6295_data_2_msb_w );

/*
    To help the various custom ADPCM generators out there,
    the following routines may be used.
*/
struct adpcm_state
{
	INT32	signal;
	INT32	step;
};
void reset_adpcm(struct adpcm_state *state);
INT16 clock_adpcm(struct adpcm_state *state, UINT8 nibble);


#endif
