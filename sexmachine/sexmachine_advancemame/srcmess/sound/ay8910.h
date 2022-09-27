#ifndef AY8910_H
#define AY8910_H

#define ALL_8910_CHANNELS -1

struct AY8910interface
{
	read8_handler portAread;
	read8_handler portBread;
	write8_handler portAwrite;
	write8_handler portBwrite;
};

void AY8910_set_volume(int chip,int channel,int volume);


READ8_HANDLER( AY8910_read_port_0_r );
READ8_HANDLER( AY8910_read_port_1_r );
READ8_HANDLER( AY8910_read_port_2_r );
READ8_HANDLER( AY8910_read_port_3_r );
READ8_HANDLER( AY8910_read_port_4_r );
READ16_HANDLER( AY8910_read_port_0_lsb_r );
READ16_HANDLER( AY8910_read_port_1_lsb_r );
READ16_HANDLER( AY8910_read_port_2_lsb_r );
READ16_HANDLER( AY8910_read_port_3_lsb_r );
READ16_HANDLER( AY8910_read_port_4_lsb_r );
READ16_HANDLER( AY8910_read_port_0_msb_r );
READ16_HANDLER( AY8910_read_port_1_msb_r );
READ16_HANDLER( AY8910_read_port_2_msb_r );
READ16_HANDLER( AY8910_read_port_3_msb_r );
READ16_HANDLER( AY8910_read_port_4_msb_r );

WRITE8_HANDLER( AY8910_control_port_0_w );
WRITE8_HANDLER( AY8910_control_port_1_w );
WRITE8_HANDLER( AY8910_control_port_2_w );
WRITE8_HANDLER( AY8910_control_port_3_w );
WRITE8_HANDLER( AY8910_control_port_4_w );
WRITE16_HANDLER( AY8910_control_port_0_lsb_w );
WRITE16_HANDLER( AY8910_control_port_1_lsb_w );
WRITE16_HANDLER( AY8910_control_port_2_lsb_w );
WRITE16_HANDLER( AY8910_control_port_3_lsb_w );
WRITE16_HANDLER( AY8910_control_port_4_lsb_w );
WRITE16_HANDLER( AY8910_control_port_0_msb_w );
WRITE16_HANDLER( AY8910_control_port_1_msb_w );
WRITE16_HANDLER( AY8910_control_port_2_msb_w );
WRITE16_HANDLER( AY8910_control_port_3_msb_w );
WRITE16_HANDLER( AY8910_control_port_4_msb_w );

WRITE8_HANDLER( AY8910_write_port_0_w );
WRITE8_HANDLER( AY8910_write_port_1_w );
WRITE8_HANDLER( AY8910_write_port_2_w );
WRITE8_HANDLER( AY8910_write_port_3_w );
WRITE8_HANDLER( AY8910_write_port_4_w );
WRITE16_HANDLER( AY8910_write_port_0_lsb_w );
WRITE16_HANDLER( AY8910_write_port_1_lsb_w );
WRITE16_HANDLER( AY8910_write_port_2_lsb_w );
WRITE16_HANDLER( AY8910_write_port_3_lsb_w );
WRITE16_HANDLER( AY8910_write_port_4_lsb_w );
WRITE16_HANDLER( AY8910_write_port_0_msb_w );
WRITE16_HANDLER( AY8910_write_port_1_msb_w );
WRITE16_HANDLER( AY8910_write_port_2_msb_w );
WRITE16_HANDLER( AY8910_write_port_3_msb_w );
WRITE16_HANDLER( AY8910_write_port_4_msb_w );

/*********** An interface for SSG of YM2203 ***********/

void *ay8910_start_ym(int chip_type, int sndindex, int clock, int streams,
		read8_handler portAread, read8_handler portBread,
		write8_handler portAwrite, write8_handler portBwrite);

void ay8910_stop_ym(void *chip);
void ay8910_reset_ym(void *chip);
void ay8910_set_clock_ym(void *chip, int clock);
void ay8910_write_ym(void *chip, int addr, int data);
int ay8910_read_ym(void *chip);

#endif
