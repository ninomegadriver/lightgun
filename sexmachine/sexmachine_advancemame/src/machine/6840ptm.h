/**********************************************************************

    Motorola 6840 PTM interface and emulation

    This function is a simple emulation of up to 4 MC6840 PTM
    (Programmable Timer Module)

    based on ( 6522 and 6821 PTM )

**********************************************************************/

#ifndef PTM_6840
#define PTM_6840

#define PTM_6840_MAX 4		// maximum number of chips to emulate

typedef struct _ptm6840_interface ptm6840_interface;
struct _ptm6840_interface
{
	write8_handler	out1_func;	// function to call when output1 changes
	write8_handler	out2_func;	// function to call when output2 changes
	write8_handler	out3_func;	// function to call when output3 changes

	void (*irq_func)(int state);	// function called if IRQ line changes
};

void ptm6840_unconfig(void);

void ptm6840_config( int which, const ptm6840_interface *intf);
void ptm6840_reset(  int which);
int  ptm6840_read(   int which, int offset);
void ptm6840_write(  int which, int offset, int data);


void ptm6840_set_g1(int which, int state);	// set gate1  state
void ptm6840_set_c1(int which, int state);	// set clock1 state

void ptm6840_set_g2(int which, int state);	// set gate2  state
void ptm6840_set_c2(int which, int state);	// set clock2 state

void ptm6840_set_g3(int which, int state);	// set gate3  state
void ptm6840_set_c3(int which, int state);	// set clock3 state

/*-------------------------------------------------------------------------*/

READ8_HANDLER( ptm6840_0_r );
READ8_HANDLER( ptm6840_1_r );
READ8_HANDLER( ptm6840_2_r );
READ8_HANDLER( ptm6840_3_r );

READ16_HANDLER( ptm6840_0_r16l );
READ16_HANDLER( ptm6840_1_r16l );
READ16_HANDLER( ptm6840_2_r16l );
READ16_HANDLER( ptm6840_3_r16l );

READ16_HANDLER( ptm6840_0_r16u );
READ16_HANDLER( ptm6840_1_r16u );
READ16_HANDLER( ptm6840_2_r16u );
READ16_HANDLER( ptm6840_3_r16u );

WRITE8_HANDLER( ptm6840_0_w );
WRITE8_HANDLER( ptm6840_1_w );
WRITE8_HANDLER( ptm6840_2_w );
WRITE8_HANDLER( ptm6840_3_w );

WRITE16_HANDLER( ptm6840_0_w16 );
WRITE16_HANDLER( ptm6840_1_w16 );
WRITE16_HANDLER( ptm6840_2_w16 );
WRITE16_HANDLER( ptm6840_3_w16 );

#endif /* PTM_6840 */
