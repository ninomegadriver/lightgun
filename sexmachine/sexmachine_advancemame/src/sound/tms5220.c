/**********************************************************************************************

     TMS5220 simulator

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

***********************************************************************************************/

#include "sndintrf.h"
#include "tms5220.h"


/* Pull in the ROM tables */
#include "tms5220r.c"

/*
  Changes by R. Nabet
   * added Speech ROM support
   * modified code so that the beast only start speaking at the start of next frame, like the data
     sheet says
*/
#define USE_OBSOLETE_HACK 0


#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif


struct tms5220
{
	/* these contain data that describes the 128-bit data FIFO */
	#define FIFO_SIZE 16
	UINT8 fifo[FIFO_SIZE];
	UINT8 fifo_head;
	UINT8 fifo_tail;
	UINT8 fifo_count;
	UINT8 fifo_bits_taken;


	/* these contain global status bits */
	/*
        R Nabet : speak_external is only set when a speak external command is going on.
        tms5220_speaking is set whenever a speak or speak external command is going on.
        Note that we really need to do anything in tms5220_process and play samples only when
        tms5220_speaking is true.  Else, we can play nothing as well, which is a
        speed-up...
    */
	UINT8 tms5220_speaking;	/* Speak or Speak External command in progress */
	UINT8 speak_external;	/* Speak External command in progress */
	#if USE_OBSOLETE_HACK
	UINT8 speak_delay_frames;
	#endif
	UINT8 talk_status; 		/* tms5220 is really currently speaking */
	UINT8 first_frame;		/* we have just started speaking, and we are to parse the first frame */
	UINT8 last_frame;		/* we are doing the frame of sound */
	UINT8 buffer_low;		/* FIFO has less than 8 bytes in it */
	UINT8 buffer_empty;		/* FIFO is empty*/
	UINT8 irq_pin;			/* state of the IRQ pin (output) */

	void (*irq_func)(int state); /* called when the state of the IRQ pin changes */


	/* these contain data describing the current and previous voice frames */
	UINT16 old_energy;
	UINT16 old_pitch;
	INT32 old_k[10];

	UINT16 new_energy;
	UINT16 new_pitch;
	INT32 new_k[10];


	/* these are all used to contain the current state of the sound generation */
	UINT16 current_energy;
	UINT16 current_pitch;
	INT32 current_k[10];

	UINT16 target_energy;
	UINT16 target_pitch;
	INT32 target_k[10];

	UINT8 interp_count;		/* number of interp periods (0-7) */
	UINT8 sample_count;		/* sample number within interp (0-24) */
	UINT16 pitch_count;

	INT32 u[11];
	INT32 x[10];

	INT8 randbit;


	/* R Nabet : These have been added to emulate speech Roms */
	int (*read_callback)(int count);
	void (*load_address_callback)(int data);
	void (*read_and_branch_callback)(void);
	UINT8 schedule_dummy_read;			/* set after each load address, so that next read operation
                                              is preceded by a dummy read */

	UINT8 data_register;				/* data register, used by read command */
	UINT8 RDB_flag;					/* whether we should read data register or status register */

	/* flag for tms0285 emulation */
	/* The tms0285 is an early variant of the tms5220 used in the ti-99/4(a)
    computer.  The exact relationship of this chip with tms5200 & tms5220 is
    unknown, but it seems to use slightly different tables for LPC parameters. */
	tms5220_variant variant;
};


/* Static function prototypes */
static void process_command(struct tms5220 *tms);
static int extract_bits(struct tms5220 *tms, int count);
static int parse_frame(struct tms5220 *tms, int the_first_frame);
static void check_buffer_low(struct tms5220 *tms);
static void set_interrupt_state(struct tms5220 *tms, int state);


#define DEBUG_5220	0


void *tms5220_create(int index)
{
	struct tms5220 *tms;

	tms = malloc(sizeof(*tms));
	memset(tms, 0, sizeof(*tms));

	state_save_register_item_array("tms5220", index, tms->fifo);
	state_save_register_item("tms5220", index, tms->fifo_head);
	state_save_register_item("tms5220", index, tms->fifo_tail);
	state_save_register_item("tms5220", index, tms->fifo_count);
	state_save_register_item("tms5220", index, tms->fifo_bits_taken);

	state_save_register_item("tms5220", index, tms->tms5220_speaking);
	state_save_register_item("tms5220", index, tms->speak_external);
	state_save_register_item("tms5220", index, tms->talk_status);
	state_save_register_item("tms5220", index, tms->first_frame);
	state_save_register_item("tms5220", index, tms->last_frame);
	state_save_register_item("tms5220", index, tms->buffer_low);
	state_save_register_item("tms5220", index, tms->buffer_empty);
	state_save_register_item("tms5220", index, tms->irq_pin);

	state_save_register_item("tms5220", index, tms->old_energy);
	state_save_register_item("tms5220", index, tms->old_pitch);
	state_save_register_item_array("tms5220", index, tms->old_k);

	state_save_register_item("tms5220", index, tms->new_energy);
	state_save_register_item("tms5220", index, tms->new_pitch);
	state_save_register_item_array("tms5220", index, tms->new_k);

	state_save_register_item("tms5220", index, tms->current_energy);
	state_save_register_item("tms5220", index, tms->current_pitch);
	state_save_register_item_array("tms5220", index, tms->current_k);

	state_save_register_item("tms5220", index, tms->target_energy);
	state_save_register_item("tms5220", index, tms->target_pitch);
	state_save_register_item_array("tms5220", index, tms->target_k);

	state_save_register_item("tms5220", index, tms->interp_count);
	state_save_register_item("tms5220", index, tms->sample_count);
	state_save_register_item("tms5220", index, tms->pitch_count);

	state_save_register_item_array("tms5220", index, tms->u);
	state_save_register_item_array("tms5220", index, tms->x);

	state_save_register_item("tms5220", index, tms->randbit);

	state_save_register_item("tms5220", index, tms->schedule_dummy_read);
	state_save_register_item("tms5220", index, tms->data_register);
	state_save_register_item("tms5220", index, tms->RDB_flag);

	return tms;
}


void tms5220_destroy(void *chip)
{
	free(chip);
}

/**********************************************************************************************

     tms5220_reset -- resets the TMS5220

***********************************************************************************************/

void tms5220_reset_chip(void *chip)
{
	struct tms5220 *tms = chip;

	/* initialize the FIFO */
	/*memset(tms->fifo, 0, sizeof(tms->fifo));*/
	tms->fifo_head = tms->fifo_tail = tms->fifo_count = tms->fifo_bits_taken = 0;

	/* initialize the chip state */
	/* Note that we do not actually clear IRQ on start-up : IRQ is even raised if tms->buffer_empty or tms->buffer_low are 0 */
	tms->tms5220_speaking = tms->speak_external = tms->talk_status = tms->first_frame = tms->last_frame = tms->irq_pin = 0;
#if USE_OBSOLETE_HACK
	tms->speak_delay_frames = 0;
#endif
	if (tms->irq_func) tms->irq_func(0);
	tms->buffer_empty = tms->buffer_low = 1;

	tms->RDB_flag = FALSE;

	/* initialize the energy/pitch/k states */
	tms->old_energy = tms->new_energy = tms->current_energy = tms->target_energy = 0;
	tms->old_pitch = tms->new_pitch = tms->current_pitch = tms->target_pitch = 0;
	memset(tms->old_k, 0, sizeof(tms->old_k));
	memset(tms->new_k, 0, sizeof(tms->new_k));
	memset(tms->current_k, 0, sizeof(tms->current_k));
	memset(tms->target_k, 0, sizeof(tms->target_k));

	/* initialize the sample generators */
	tms->interp_count = tms->sample_count = tms->pitch_count = 0;
	tms->randbit = 0;
	memset(tms->u, 0, sizeof(tms->u));
	memset(tms->x, 0, sizeof(tms->x));

	if (tms->load_address_callback)
		(*tms->load_address_callback)(0);

	tms->schedule_dummy_read = TRUE;
}



/**********************************************************************************************

     tms5220_set_irq -- sets the interrupt handler

***********************************************************************************************/

void tms5220_set_irq(void *chip, void (*func)(int))
{
	struct tms5220 *tms = chip;
    tms->irq_func = func;
}


/**********************************************************************************************

     tms5220_set_read -- sets the speech ROM read handler

***********************************************************************************************/

void tms5220_set_read(void *chip, int (*func)(int))
{
	struct tms5220 *tms = chip;
	tms->read_callback = func;
}


/**********************************************************************************************

     tms5220_set_load_address -- sets the speech ROM load address handler

***********************************************************************************************/

void tms5220_set_load_address(void *chip, void (*func)(int))
{
	struct tms5220 *tms = chip;
	tms->load_address_callback = func;
}


/**********************************************************************************************

     tms5220_set_read_and_branch -- sets the speech ROM read and branch handler

***********************************************************************************************/

void tms5220_set_read_and_branch(void *chip, void (*func)(void))
{
	struct tms5220 *tms = chip;
	tms->read_and_branch_callback = func;
}


/**********************************************************************************************

     tms5220_set_variant -- sets the tms5220 core to emulate its buggy forerunner, the tms0285

***********************************************************************************************/

void tms5220_set_variant(void *chip, tms5220_variant new_variant)
{
	struct tms5220 *tms = chip;
	tms->variant = new_variant;
}


/**********************************************************************************************

     tms5220_data_write -- handle a write to the TMS5220

***********************************************************************************************/

void tms5220_data_write(void *chip, int data)
{
	struct tms5220 *tms = chip;

    /* add this byte to the FIFO */
    if (tms->fifo_count < FIFO_SIZE)
    {
        tms->fifo[tms->fifo_tail] = data;
        tms->fifo_tail = (tms->fifo_tail + 1) % FIFO_SIZE;
        tms->fifo_count++;

		/* if we were speaking, then we're no longer empty */
		if (tms->speak_external)
			tms->buffer_empty = 0;

        if (DEBUG_5220) logerror("Added byte to FIFO (size=%2d)\n", tms->fifo_count);
    }
    else
    {
        if (DEBUG_5220) logerror("Ran out of room in the FIFO!\n");
    }

    /* update the buffer low state */
    check_buffer_low(tms);

	if (! tms->speak_external)
		/* R Nabet : we parse commands at once.  It is necessary for such commands as read. */
		process_command (tms/*data*/);
}


/**********************************************************************************************

     tms5220_status_read -- read status or data from the TMS5220

      From the data sheet:
        bit 0 = TS - Talk Status is active (high) when the VSP is processing speech data.
                Talk Status goes active at the initiation of a Speak command or after nine
                bytes of data are loaded into the FIFO following a Speak External command. It
                goes inactive (low) when the stop code (Energy=1111) is processed, or
                immediately by a buffer empty condition or a reset command.
        bit 1 = BL - Buffer Low is active (high) when the FIFO buffer is more than half empty.
                Buffer Low is set when the "Last-In" byte is shifted down past the half-full
                boundary of the stack. Buffer Low is cleared when data is loaded to the stack
                so that the "Last-In" byte lies above the half-full boundary and becomes the
                ninth data byte of the stack.
        bit 2 = BE - Buffer Empty is active (high) when the FIFO buffer has run out of data
                while executing a Speak External command. Buffer Empty is set when the last bit
                of the "Last-In" byte is shifted out to the Synthesis Section. This causes
                Talk Status to be cleared. Speed is terminated at some abnormal point and the
                Speak External command execution is terminated.

***********************************************************************************************/

int tms5220_status_read(void *chip)
{
	struct tms5220 *tms = chip;

	if (tms->RDB_flag)
	{	/* if last command was read, return data register */
		tms->RDB_flag = FALSE;
		return(tms->data_register);
	}
	else
	{	/* read status */

		/* clear the interrupt pin */
		set_interrupt_state(tms, 0);

		if (DEBUG_5220) logerror("Status read: TS=%d BL=%d BE=%d\n", tms->talk_status, tms->buffer_low, tms->buffer_empty);

		return (tms->talk_status << 7) | (tms->buffer_low << 6) | (tms->buffer_empty << 5);
	}
}



/**********************************************************************************************

     tms5220_ready_read -- returns the ready state of the TMS5220

***********************************************************************************************/

int tms5220_ready_read(void *chip)
{
	struct tms5220 *tms = chip;
    return (tms->fifo_count < FIFO_SIZE-1);
}


/**********************************************************************************************

     tms5220_ready_read -- returns the number of cycles until ready is asserted

***********************************************************************************************/

int tms5220_cycles_to_ready(void *chip)
{
	struct tms5220 *tms = chip;
	int answer;


	if (tms5220_ready_read(tms))
		answer = 0;
	else
	{
		int val;

		answer = 200-tms->sample_count+8;

		/* total number of bits available in current byte is (8 - tms->fifo_bits_taken) */
		/* if more than 4 are available, we need to check the energy */
		if (tms->fifo_bits_taken < 4)
		{
			/* read energy */
			val = (tms->fifo[tms->fifo_head] >> tms->fifo_bits_taken) & 0xf;
			if (val == 0)
				/* 0 -> silence frame: we will only read 4 bits, and we will
                therefore need to read another frame before the FIFO is not
                full any more */
				answer += 200;
			/* 15 -> stop frame, we will only read 4 bits, but the FIFO will
            we cleared */
			/* otherwise, we need to parse the repeat flag (1 bit) and the
            pitch (6 bits), so everything will be OK. */
		}
	}

	return answer;
}


/**********************************************************************************************

     tms5220_int_read -- returns the interrupt state of the TMS5220

***********************************************************************************************/

int tms5220_int_read(void *chip)
{
	struct tms5220 *tms = chip;
    return tms->irq_pin;
}



/**********************************************************************************************

     tms5220_process -- fill the buffer with a specific number of samples

***********************************************************************************************/

void tms5220_process(void *chip, INT16 *buffer, unsigned int size)
{
	struct tms5220 *tms = chip;
    int buf_count=0;
    int i, interp_period;

tryagain:

    /* if we're not speaking, parse commands */
	/*while (!tms->speak_external && tms->fifo_count > 0)
        process_command(tms);*/

    /* if we're empty and still not speaking, fill with nothingness */
	if ((!tms->tms5220_speaking) && (!tms->last_frame))
        goto empty;

    /* if we're to speak, but haven't started, wait for the 9th byte */
	if (!tms->talk_status && tms->speak_external)
    {
        if (tms->fifo_count < 9)
           goto empty;

        tms->talk_status = 1;
		tms->first_frame = 1;	/* will cause the first frame to be parsed */
		tms->buffer_empty = 0;
	}

#if 0
	/* we are to speak, yet we fill with 0s until start of next frame */
	if (tms->first_frame)
	{
		while ((size > 0) && ((tms->sample_count != 0) || (tms->interp_count != 0)))
		{
			tms->sample_count = (tms->sample_count + 1) % 200;
			tms->interp_count = (tms->interp_count + 1) % 25;
			buffer[buf_count] = 0x00;	/* should be (-1 << 8) ??? (cf note in data sheet, p 10, table 4) */
			buf_count++;
			size--;
		}
	}
#endif

#if USE_OBSOLETE_HACK
    /* apply some delay before we actually consume data; Victory requires this */
    if (tms->speak_delay_frames)
    {
    	if (size <= tms->speak_delay_frames)
    	{
    		tms->speak_delay_frames -= size;
    		size = 0;
    	}
    	else
    	{
    		size -= tms->speak_delay_frames;
    		tms->speak_delay_frames = 0;
    	}
    }
#endif

    /* loop until the buffer is full or we've stopped speaking */
	while ((size > 0) && tms->talk_status)
    {
        int current_val;

        /* if we're ready for a new frame */
        if ((tms->interp_count == 0) && (tms->sample_count == 0))
        {
            /* Parse a new frame */
			if (!parse_frame(tms, tms->first_frame))
				break;
			tms->first_frame = 0;

            /* Set old target as new start of frame */
            tms->current_energy = tms->old_energy;
            tms->current_pitch = tms->old_pitch;
            for (i = 0; i < 10; i++)
                tms->current_k[i] = tms->old_k[i];

            /* is this a zero energy frame? */
            if (tms->current_energy == 0)
            {
                /*printf("processing frame: zero energy\n");*/
                tms->target_energy = 0;
                tms->target_pitch = tms->current_pitch;
                for (i = 0; i < 10; i++)
                    tms->target_k[i] = tms->current_k[i];
            }

            /* is this a stop frame? */
            else if (tms->current_energy == (energytable[15] >> 6))
            {
                /*printf("processing frame: stop frame\n");*/
                tms->current_energy = energytable[0] >> 6;
                tms->target_energy = tms->current_energy;
				/*tms->interp_count = tms->sample_count =*/ tms->pitch_count = 0;
				tms->last_frame = 0;
				if (tms->tms5220_speaking)
					/* new speech command in progress */
					tms->first_frame = 1;
				else
				{
					/* really stop speaking */
					tms->talk_status = 0;

					/* generate an interrupt if necessary */
					set_interrupt_state(tms, 1);
				}

                /* try to fetch commands again */
                goto tryagain;
            }
            else
            {
                /* is this the ramp down frame? */
                if (tms->new_energy == (energytable[15] >> 6))
                {
                    /*printf("processing frame: ramp down\n");*/
                    tms->target_energy = 0;
                    tms->target_pitch = tms->current_pitch;
                    for (i = 0; i < 10; i++)
                        tms->target_k[i] = tms->current_k[i];
                }
                /* Reset the step size */
                else
                {
                    /*printf("processing frame: Normal\n");*/
                    /*printf("*** Energy = %d\n",tms->current_energy);*/
                    /*printf("proc: %d %d\n",last_fbuf_head,fbuf_head);*/

                    tms->target_energy = tms->new_energy;
                    tms->target_pitch = tms->new_pitch;

                    for (i = 0; i < 4; i++)
                        tms->target_k[i] = tms->new_k[i];
                    if (tms->current_pitch == 0)
                        for (i = 4; i < 10; i++)
                        {
                            tms->target_k[i] = tms->current_k[i] = 0;
                        }
                    else
                        for (i = 4; i < 10; i++)
                            tms->target_k[i] = tms->new_k[i];
                }
            }
        }
        else if (tms->interp_count == 0)
        {
            /* Update values based on step values */
            /*printf("\n");*/

            interp_period = tms->sample_count / 25;
            tms->current_energy += (tms->target_energy - tms->current_energy) / interp_coeff[interp_period];
            if (tms->old_pitch != 0)
                tms->current_pitch += (tms->target_pitch - tms->current_pitch) / interp_coeff[interp_period];

            /*printf("*** Energy = %d\n",tms->current_energy);*/

            for (i = 0; i < 10; i++)
            {
                tms->current_k[i] += (tms->target_k[i] - tms->current_k[i]) / interp_coeff[interp_period];
            }
        }

        if (tms->old_energy == 0)
        {
            /* generate silent samples here */
            current_val = 0x00;
        }
        else if (tms->old_pitch == 0)
        {
            /* generate unvoiced samples here */
            tms->randbit = (rand() % 2) * 2 - 1;
            current_val = (tms->randbit * tms->current_energy) / 4;
        }
        else
        {
            /* generate voiced samples here */
            if (tms->pitch_count < sizeof(chirptable))
                current_val = (chirptable[tms->pitch_count] * tms->current_energy) / 256;
            else
                current_val = 0x00;
        }

        /* Lattice filter here */

        tms->u[10] = current_val;

        for (i = 9; i >= 0; i--)
        {
            tms->u[i] = tms->u[i+1] - ((tms->current_k[i] * tms->x[i]) / 32768);
        }
        for (i = 9; i >= 1; i--)
        {
            tms->x[i] = tms->x[i-1] + ((tms->current_k[i-1] * tms->u[i-1]) / 32768);
        }

        tms->x[0] = tms->u[0];

        /* clipping, just like the chip */

        if (tms->u[0] > 511)
            buffer[buf_count] = 127<<8;
        else if (tms->u[0] < -512)
            buffer[buf_count] = -128<<8;
        else
            buffer[buf_count] = tms->u[0] << 6;

        /* Update all counts */

        size--;
        tms->sample_count = (tms->sample_count + 1) % 200;

        if (tms->current_pitch != 0)
            tms->pitch_count = (tms->pitch_count + 1) % tms->current_pitch;
        else
            tms->pitch_count = 0;

        tms->interp_count = (tms->interp_count + 1) % 25;
        buf_count++;
    }

empty:

    while (size > 0)
    {
		tms->sample_count = (tms->sample_count + 1) % 200;
		tms->interp_count = (tms->interp_count + 1) % 25;
		buffer[buf_count] = 0x00;	/* should be (-1 << 8) ??? (cf note in data sheet, p 10, table 4) */
        buf_count++;
        size--;
    }
}



/**********************************************************************************************

     process_command -- extract a byte from the FIFO and interpret it as a command

***********************************************************************************************/

static void process_command(struct tms5220 *tms)
{
    unsigned char cmd;

    /* if there are stray bits, ignore them */
	if (tms->fifo_bits_taken)
	{
		tms->fifo_bits_taken = 0;
        tms->fifo_count--;
        tms->fifo_head = (tms->fifo_head + 1) % FIFO_SIZE;
    }

    /* grab a full byte from the FIFO */
    if (tms->fifo_count > 0)
    {
		cmd = tms->fifo[tms->fifo_head];
		tms->fifo_count--;
		tms->fifo_head = (tms->fifo_head + 1) % FIFO_SIZE;

		/* parse the command */
		switch (cmd & 0x70)
		{
		case 0x10 : /* read byte */
			if (tms->schedule_dummy_read)
			{
				tms->schedule_dummy_read = FALSE;
				if (tms->read_callback)
					(*tms->read_callback)(1);
			}
			if (tms->read_callback)
				tms->data_register = (*tms->read_callback)(8);	/* read one byte from speech ROM... */
			tms->RDB_flag = TRUE;
			break;

		case 0x30 : /* read and branch */
			if (DEBUG_5220) logerror("read and branch command received\n");
			tms->RDB_flag = FALSE;
			if (tms->read_and_branch_callback)
				(*tms->read_and_branch_callback)();
			break;

		case 0x40 : /* load address */
			/* tms5220 data sheet says that if we load only one 4-bit nibble, it won't work.
              This code does not care about this. */
			if (tms->load_address_callback)
				(*tms->load_address_callback)(cmd & 0x0f);
			tms->schedule_dummy_read = TRUE;
			break;

		case 0x50 : /* speak */
			if (tms->schedule_dummy_read)
			{
				tms->schedule_dummy_read = FALSE;
				if (tms->read_callback)
					(*tms->read_callback)(1);
			}
			tms->tms5220_speaking = 1;
			tms->speak_external = 0;
			if (! tms->last_frame)
			{
				tms->first_frame = 1;
			}
			tms->talk_status = 1;  /* start immediately */
			break;

		case 0x60 : /* speak external */
			tms->tms5220_speaking = tms->speak_external = 1;
#if USE_OBSOLETE_HACK
            tms->speak_delay_frames = 10;
#endif

			tms->RDB_flag = FALSE;

            /* according to the datasheet, this will cause an interrupt due to a BE condition */
            if (!tms->buffer_empty)
            {
                tms->buffer_empty = 1;
                set_interrupt_state(tms, 1);
            }

			tms->talk_status = 0;	/* wait to have 8 bytes in buffer before starting */
			break;

		case 0x70 : /* reset */
			if (tms->schedule_dummy_read)
			{
				tms->schedule_dummy_read = FALSE;
				if (tms->read_callback)
					(*tms->read_callback)(1);
			}
			tms5220_reset_chip(tms);
			break;
        }
    }

    /* update the buffer low state */
    check_buffer_low(tms);
}



/**********************************************************************************************

     extract_bits -- extract a specific number of bits from the FIFO

***********************************************************************************************/

static int extract_bits(struct tms5220 *tms, int count)
{
    int val = 0;

	if (tms->speak_external)
	{
		/* extract from FIFO */
    	while (count--)
    	{
        	val = (val << 1) | ((tms->fifo[tms->fifo_head] >> tms->fifo_bits_taken) & 1);
        	tms->fifo_bits_taken++;
        	if (tms->fifo_bits_taken >= 8)
        	{
        	    tms->fifo_count--;
        	    tms->fifo_head = (tms->fifo_head + 1) % FIFO_SIZE;
        	    tms->fifo_bits_taken = 0;
        	}
    	}
    }
	else
	{
		/* extract from speech ROM */
		if (tms->read_callback)
			val = (* tms->read_callback)(count);
	}

    return val;
}



/**********************************************************************************************

     parse_frame -- parse a new frame's worth of data; returns 0 if not enough bits in buffer

***********************************************************************************************/

static int parse_frame(struct tms5220 *tms, int the_first_frame)
{
	int bits = 0;	/* number of bits in FIFO (speak external only) */
	int indx, i, rep_flag;

	if (! the_first_frame)
	{
    /* remember previous frame */
    tms->old_energy = tms->new_energy;
    tms->old_pitch = tms->new_pitch;
    for (i = 0; i < 10; i++)
        tms->old_k[i] = tms->new_k[i];
	}

    /* clear out the new frame */
    tms->new_energy = 0;
    tms->new_pitch = 0;
    for (i = 0; i < 10; i++)
        tms->new_k[i] = 0;

    /* if the previous frame was a stop frame, don't do anything */
	if ((! the_first_frame) && (tms->old_energy == (energytable[15] >> 6)))
		/*return 1;*/
	{
		tms->buffer_empty = 1;
		return 1;
	}

	if (tms->speak_external)
    	/* count the total number of bits available */
		bits = tms->fifo_count * 8 - tms->fifo_bits_taken;

    /* attempt to extract the energy index */
	if (tms->speak_external)
	{
    bits -= 4;
    if (bits < 0)
        goto ranout;
	}
    indx = extract_bits(tms, 4);
    tms->new_energy = energytable[indx] >> 6;

	/* if the index is 0 or 15, we're done */
	if (indx == 0 || indx == 15)
	{
		if (DEBUG_5220) logerror("  (4-bit energy=%d frame)\n",tms->new_energy);

		/* clear tms->fifo if stop frame encountered */
		if (indx == 15)
		{
			tms->fifo_head = tms->fifo_tail = tms->fifo_count = tms->fifo_bits_taken = 0;
			tms->speak_external = tms->tms5220_speaking = 0;
			tms->last_frame = 1;
		}
		goto done;
	}

    /* attempt to extract the repeat flag */
	if (tms->speak_external)
	{
    bits -= 1;
    if (bits < 0)
        goto ranout;
	}
    rep_flag = extract_bits(tms, 1);

    /* attempt to extract the pitch */
	if (tms->speak_external)
	{
    bits -= 6;
    if (bits < 0)
        goto ranout;
	}
    indx = extract_bits(tms, 6);
    tms->new_pitch = pitchtable[indx] / 256;

    /* if this is a repeat frame, just copy the k's */
    if (rep_flag)
    {
        for (i = 0; i < 10; i++)
            tms->new_k[i] = tms->old_k[i];

        if (DEBUG_5220) logerror("  (11-bit energy=%d pitch=%d rep=%d frame)\n", tms->new_energy, tms->new_pitch, rep_flag);
        goto done;
    }

    /* if the pitch index was zero, we need 4 k's */
    if (indx == 0)
    {
        /* attempt to extract 4 K's */
		if (tms->speak_external)
		{
        bits -= 18;
        if (bits < 0)
            goto ranout;
		}
        tms->new_k[0] = k1table[extract_bits(tms, 5)];
        tms->new_k[1] = k2table[extract_bits(tms, 5)];
        tms->new_k[2] = k3table[extract_bits(tms, 4)];
		if (tms->variant == variant_tms0285)
			tms->new_k[3] = k3table[extract_bits(tms, 4)];	/* ??? */
		else
			tms->new_k[3] = k4table[extract_bits(tms, 4)];

        if (DEBUG_5220) logerror("  (29-bit energy=%d pitch=%d rep=%d 4K frame)\n", tms->new_energy, tms->new_pitch, rep_flag);
        goto done;
    }

    /* else we need 10 K's */
	if (tms->speak_external)
	{
    bits -= 39;
    if (bits < 0)
        goto ranout;
	}

    tms->new_k[0] = k1table[extract_bits(tms, 5)];
    tms->new_k[1] = k2table[extract_bits(tms, 5)];
    tms->new_k[2] = k3table[extract_bits(tms, 4)];
	if (tms->variant == variant_tms0285)
		tms->new_k[3] = k3table[extract_bits(tms, 4)];	/* ??? */
	else
		tms->new_k[3] = k4table[extract_bits(tms, 4)];
    tms->new_k[4] = k5table[extract_bits(tms, 4)];
    tms->new_k[5] = k6table[extract_bits(tms, 4)];
    tms->new_k[6] = k7table[extract_bits(tms, 4)];
    tms->new_k[7] = k8table[extract_bits(tms, 3)];
    tms->new_k[8] = k9table[extract_bits(tms, 3)];
    tms->new_k[9] = k10table[extract_bits(tms, 3)];

    if (DEBUG_5220) logerror("  (50-bit energy=%d pitch=%d rep=%d 10K frame)\n", tms->new_energy, tms->new_pitch, rep_flag);

done:
	if (DEBUG_5220)
	{
		if (tms->speak_external)
			logerror("Parsed a frame successfully in FIFO - %d bits remaining\n", bits);
		else
			logerror("Parsed a frame successfully in ROM\n");
	}

	if (the_first_frame)
	{
		/* if this is the first frame, no previous frame to take as a starting point */
		tms->old_energy = tms->new_energy;
		tms->old_pitch = tms->new_pitch;
		for (i = 0; i < 10; i++)
			tms->old_k[i] = tms->new_k[i];
    }

    /* update the tms->buffer_low status */
    check_buffer_low(tms);
    return 1;

ranout:

    if (DEBUG_5220) logerror("Ran out of bits on a parse!\n");

    /* this is an error condition; mark the buffer empty and turn off speaking */
    tms->buffer_empty = 1;
	tms->talk_status = tms->speak_external = tms->tms5220_speaking = the_first_frame = tms->last_frame = 0;
    tms->fifo_count = tms->fifo_head = tms->fifo_tail = 0;

	tms->RDB_flag = FALSE;

    /* generate an interrupt if necessary */
    set_interrupt_state(tms, 1);
    return 0;
}



/**********************************************************************************************

     check_buffer_low -- check to see if the buffer low flag should be on or off

***********************************************************************************************/

static void check_buffer_low(struct tms5220 *tms)
{
    /* did we just become low? */
    if (tms->fifo_count <= 8)
    {
        /* generate an interrupt if necessary */
        if (!tms->buffer_low)
            set_interrupt_state(tms, 1);
        tms->buffer_low = 1;

        if (DEBUG_5220) logerror("Buffer low set\n");
    }

    /* did we just become full? */
    else
    {
        tms->buffer_low = 0;

        if (DEBUG_5220) logerror("Buffer low cleared\n");
    }
}



/**********************************************************************************************

     set_interrupt_state -- generate an interrupt

***********************************************************************************************/

static void set_interrupt_state(struct tms5220 *tms, int state)
{
    if (tms->irq_func && state != tms->irq_pin)
    	tms->irq_func(state);
    tms->irq_pin = state;
}
