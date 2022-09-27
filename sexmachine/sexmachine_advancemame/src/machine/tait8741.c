/*

Taito 8741 emulation

1.The pair chip for the PIO and serial communication between MAIN CPU and the sub CPU
2.The PIO for DIP SW and the controller reading.

*/

#include "driver.h"
#include "tait8741.h"

#define __log__ 0

/****************************************************************************

gladiatr and Great Swordsman set.

  -comminucation main and sub cpu
  -dipswitch and key handling x 2chip

  Total 4chip

  It was supposed from the schematic of gladiator.
  Now, because dump is done, change the MCU code of gladiator to the CPU emulation.

****************************************************************************/

#define CMD_IDLE 0
#define CMD_08 1
#define CMD_4a 2

typedef struct TAITO8741_status{
	unsigned char toData;    /* to host data      */
	unsigned char fromData;  /* from host data    */
	unsigned char fromCmd;   /* from host command */
	unsigned char status;    /* b0 = rd ready,b1 = wd full,b2 = cmd ?? */
	unsigned char mode;
	unsigned char phase;
	unsigned char txd[8];
	unsigned char rxd[8];
	unsigned char parallelselect;
	unsigned char txpoint;
	int connect;
	unsigned char pending4a;
	int serial_out;
	int coins;
	read8_handler portHandler;
}I8741;

static const struct TAITO8741interface *intf;
//static I8741 *taito8741;
static I8741 taito8741[MAX_TAITO8741];

/* for host data , write */
static void taito8741_hostdata_w(I8741 *st,int data)
{
	st->toData = data;
	st->status |= 0x01;
}

/* from host data , read */
static int taito8741_hostdata_r(I8741 *st)
{
	if( !(st->status & 0x02) ) return -1;
	st->status &= 0xfd;
	return st->fromData;
}

/* from host command , read */
static int taito8741_hostcmd_r(I8741 *st)
{
	if(!(st->status & 0x04)) return -1;
	st->status &= 0xfb;
	return st->fromCmd;
}


/* TAITO8741 I8741 emulation */

void taito8741_serial_rx(I8741 *st,unsigned char *data)
{
	memcpy(st->rxd,data,8);
}

/* timer callback of serial tx finish */
void taito8741_serial_tx(int num)
{
	I8741 *st = &taito8741[num];
	I8741 *sst;

	if( st->mode==TAITO8741_MASTER)
		st->serial_out = 1;

	st->txpoint = 1;
	if(st->connect >= 0 )
	{
		sst = &taito8741[st->connect];
		/* transfer data */
		taito8741_serial_rx(sst,st->txd);
#if __log__
		logerror("8741-%d Serial data TX to %d\n",num,st->connect);
#endif
		if( sst->mode==TAITO8741_SLAVE)
			sst->serial_out = 1;
	}
}

void TAITO8741_reset(int num)
{
	I8741 *st = &taito8741[num];
	st->status = 0x00;
	st->phase = 0;
	st->parallelselect = 0;
	st->txpoint = 1;
	st->pending4a = 0;
	st->serial_out = 0;
	st->coins = 0;
	memset(st->rxd,0,8);
	memset(st->txd,0,8);
}

/* 8741 update */
static void taito8741_update(int num)
{
	I8741 *st,*sst;
	int next = num;
	int data;

	do{
		num = next;
		st = &taito8741[num];
		if( st->connect != -1 )
			 sst = &taito8741[st->connect];
		else sst = 0;
		next = -1;
		/* check pending command */
		switch(st->phase)
		{
		case CMD_08: /* serial data latch */
			if( st->serial_out)
			{
				st->status &= 0xfb; /* patch for gsword */
				st->phase = CMD_IDLE;
				next = num; /* continue this chip */
			}
			break;
		case CMD_4a: /* wait for syncronus ? */
			if(!st->pending4a)
			{
				taito8741_hostdata_w(st,0);
				st->phase = CMD_IDLE;
				next = num; /* continue this chip */
			}
			break;
		case CMD_IDLE:
			/* ----- data in port check ----- */
			data = taito8741_hostdata_r(st);
			if( data != -1 )
			{
				switch(st->mode)
				{
				case TAITO8741_MASTER:
				case TAITO8741_SLAVE:
					/* buffering transmit data */
					if( st->txpoint < 8 )
					{
//if (st->txpoint == 0 && num==1 && data&0x80) logerror("Coin Put\n");
						st->txd[st->txpoint++] = data;
					}
					break;
				case TAITO8741_PORT:
					if( data & 0xf8)
					{ /* ?? */
					}
					else
					{ /* port select */
						st->parallelselect = data & 0x07;
						taito8741_hostdata_w(st,st->portHandler ? st->portHandler(st->parallelselect) : 0);
					}
				}
			}
			/* ----- new command fetch ----- */
			data = taito8741_hostcmd_r(st);
			switch( data )
			{
			case -1: /* no command data */
				break;
			case 0x00: /* read from parallel port */
				taito8741_hostdata_w(st,st->portHandler ? st->portHandler(0) : 0 );
				break;
			case 0x01: /* read receive buffer 0 */
			case 0x02: /* read receive buffer 1 */
			case 0x03: /* read receive buffer 2 */
			case 0x04: /* read receive buffer 3 */
			case 0x05: /* read receive buffer 4 */
			case 0x06: /* read receive buffer 5 */
			case 0x07: /* read receive buffer 6 */
//if (data == 2 && num==0 && st->rxd[data-1]&0x80) logerror("Coin Get\n");
				taito8741_hostdata_w(st,st->rxd[data-1]);
				break;
			case 0x08:	/* latch received serial data */
				st->txd[0] = st->portHandler ? st->portHandler(0) : 0;
				if( sst )
				{
					timer_set (TIME_NOW,num,taito8741_serial_tx);
					st->serial_out = 0;
					st->status |= 0x04;
					st->phase = CMD_08;
				}
				break;
			case 0x0a:	/* 8741-0 : set serial comminucation mode 'MASTER' */
				//st->mode = TAITO8741_MASTER;
				break;
			case 0x0b:	/* 8741-1 : set serial comminucation mode 'SLAVE'  */
				//st->mode = TAITO8741_SLAVE;
				break;
			case 0x1f:  /* 8741-2,3 : ?? set parallelport mode ?? */
			case 0x3f:  /* 8741-2,3 : ?? set parallelport mode ?? */
			case 0xe1:  /* 8741-2,3 : ?? set parallelport mode ?? */
				st->mode = TAITO8741_PORT;
				st->parallelselect = 1; /* preset read number */
				break;
			case 0x62:  /* 8741-3   : ? */
				break;
			case 0x4a:	/* ?? syncronus with other cpu and return 00H */
				if( sst )
				{
					if(sst->pending4a)
					{
						sst->pending4a = 0; /* syncronus */
						taito8741_hostdata_w(st,0); /* return for host */
						next = st->connect;
					}
					else st->phase = CMD_4a;
				}
				break;
			case 0x80:	/* 8741-3 : return check code */
				taito8741_hostdata_w(st,0x66);
				break;
			case 0x81:	/* 8741-2 : return check code */
				taito8741_hostdata_w(st,0x48);
				break;
			case 0xf0:  /* GSWORD 8741-1 : initialize ?? */
				break;
			case 0x82:  /* GSWORD 8741-2 unknown */
				break;
			}
			break;
		}
	}while(next>=0);
}

int TAITO8741_start(const struct TAITO8741interface *taito8741intf)
{
	int i;

	intf = taito8741intf;

	//taito8741 = (I8741 *)malloc(intf->num*sizeof(I8741));
	//if( taito8741 == 0 ) return 1;

	for(i=0;i<intf->num;i++)
	{
		taito8741[i].connect     = intf->serial_connect[i];
		taito8741[i].portHandler = intf->portHandler_r[i];
		taito8741[i].mode        = intf->mode[i];
		TAITO8741_reset(i);
	}
	return 0;
}

/* read status port */
static int I8741_status_r(int num)
{
	I8741 *st = &taito8741[num];
	taito8741_update(num);
#if __log__
	logerror("8741-%d ST Read %02x PC=%04x\n",num,st->status,activecpu_get_pc());
#endif
	return st->status;
}

/* read data port */
static int I8741_data_r(int num)
{
	I8741 *st = &taito8741[num];
	int ret = st->toData;
	st->status &= 0xfe;
#if __log__
	logerror("8741-%d DATA Read %02x PC=%04x\n",num,ret,activecpu_get_pc());
#endif
	/* update chip */
	taito8741_update(num);

	switch( st->mode )
	{
	case TAITO8741_PORT: /* parallel data */
		taito8741_hostdata_w(st,st->portHandler ? st->portHandler(st->parallelselect) : 0);
		break;
	}
	return ret;
}

/* Write data port */
static void I8741_data_w(int num, int data)
{
	I8741 *st = &taito8741[num];
#if __log__
	logerror("8741-%d DATA Write %02x PC=%04x\n",num,data,activecpu_get_pc());
#endif
	st->fromData = data;
	st->status |= 0x02;
	/* update chip */
	taito8741_update(num);
}

/* Write command port */
static void I8741_command_w(int num, int data)
{
	I8741 *st = &taito8741[num];
#if __log__
	logerror("8741-%d CMD Write %02x PC=%04x\n",num,data,activecpu_get_pc());
#endif
	st->fromCmd = data;
	st->status |= 0x04;
	/* update chip */
	taito8741_update(num);
}

/* Write port handler */
WRITE8_HANDLER( TAITO8741_0_w )
{
	if(offset&1) I8741_command_w(0,data);
	else         I8741_data_w(0,data);
}
WRITE8_HANDLER( TAITO8741_1_w )
{
	if(offset&1) I8741_command_w(1,data);
	else         I8741_data_w(1,data);
}
WRITE8_HANDLER( TAITO8741_2_w )
{
	if(offset&1) I8741_command_w(2,data);
	else         I8741_data_w(2,data);
}
WRITE8_HANDLER( TAITO8741_3_w )
{
	if(offset&1) I8741_command_w(3,data);
	else         I8741_data_w(3,data);
}

/* Read port handler */
READ8_HANDLER( TAITO8741_0_r )
{
	if(offset&1) return I8741_status_r(0);
	return I8741_data_r(0);
}
READ8_HANDLER( TAITO8741_1_r )
{
	if(offset&1) return I8741_status_r(1);
	return I8741_data_r(1);
}
READ8_HANDLER( TAITO8741_2_r )
{
	if(offset&1) return I8741_status_r(2);
	return I8741_data_r(2);
}
READ8_HANDLER( TAITO8741_3_r )
{
	if(offset&1) return I8741_status_r(3);
	return I8741_data_r(3);
}
/****************************************************************************

joshi Vollyball set.

  Only the chip of the communication between MAIN and the SUB.
  For the I/O, there may be I8741 of the addition.

  MCU code is not dumped.
  There is some HACK operation because the emulation is imperfect.

****************************************************************************/

int josvolly_nmi_enable;

typedef struct josvolly_8741_struct {
	UINT8 cmd;
	UINT8 sts;
	UINT8 txd;
	UINT8 outport;
	UINT8 rxd;
	UINT8 connect;

	UINT8 rst;

	read8_handler initReadPort;
}JV8741;

static JV8741 i8741[4];

void josvolly_8741_reset(void)
{
	int i;

	josvolly_nmi_enable = 0;

	for(i=0;i<4;i++)
	{
		i8741[i].cmd = 0;
		i8741[i].sts = 0; // 0xf0; /* init flag */
		i8741[i].txd = 0;
		i8741[i].outport = 0xff;
		i8741[i].rxd = 0;

		i8741[i].rst = 1;

	}
	i8741[0].connect = 1;
	i8741[1].connect = 0;

	i8741[0].initReadPort = input_port_3_r;  /* DSW1 */
	i8741[1].initReadPort = input_port_4_r;  /* DSW2 */
	i8741[2].initReadPort = input_port_3_r;  /* DUMMY */
	i8741[3].initReadPort = input_port_4_r;  /* DUMMY */
}

/* transmit data finish callback */
static void josvolly_8741_tx(int num)
{
	JV8741 *src = &i8741[num];
	JV8741 *dst = &i8741[src->connect];

	dst->rxd = src->txd;

	src->sts &= ~0x02; /* TX full ? */
	dst->sts |=  0x01; /* RX ready  ? */
}

static void josvolly_8741_do(int num)
{
	if( (i8741[num].sts & 0x02) )
	{
		/* transmit data */
		timer_set (TIME_IN_USEC(1),num,josvolly_8741_tx);
	}
}

static void josvolly_8741_w(int num,int offset,int data,int log)
{
	JV8741 *mcu = &i8741[num];

	if(offset==1)
	{
#if __log__
if(log)
		logerror("PC=%04X 8741[%d] CW %02X\n",activecpu_get_pc(),num,data);
#endif

		/* read pointer */
		mcu->cmd = data;
		/* CMD */
		switch(data)
		{
		case 0:
			mcu->txd = data ^ 0x40;
			mcu->sts |= 0x02;
			break;
		case 1:
			mcu->txd = data ^ 0x40;
			mcu->sts |= 0x02;
#if 1
			/* ?? */
			mcu->rxd = 0;  /* SBSTS ( DIAG ) , killed */
			mcu->sts |= 0x01; /* RD ready */
#endif
			break;
		case 2:
#if 1
			mcu->rxd = input_port_4_r(0);  /* DSW2 */
			mcu->sts |= 0x01; /* RD ready */
#endif
			break;
		case 3: /* normal mode ? */
			break;

		case 0xf0: /* clear main sts ? */
			mcu->txd = data ^ 0x40;
			mcu->sts |= 0x02;
			break;
		}
	}
	else
	{
		/* data */
#if __log__
if(log)
		logerror("PC=%04X 8741[%d] DW %02X\n",activecpu_get_pc(),num,data);
#endif

		mcu->txd  = data^0x40; /* parity reverce ? */
		mcu->sts  |= 0x02;     /* TXD busy         */
#if 1
		/* interrupt ? */
		if(num==0)
		{
			if(josvolly_nmi_enable)
			{
				cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
				josvolly_nmi_enable = 0;
			}
		}
#endif
	}
	josvolly_8741_do(num);
}

static INT8 josvolly_8741_r(int num,int offset,int log)
{
	JV8741 *mcu = &i8741[num];
	int ret;

	if(offset==1)
	{
		if(mcu->rst)
			mcu->rxd = (mcu->initReadPort)(0); /* port in */
		ret = mcu->sts;
#if __log__
if(log)
		logerror("PC=%04X 8741[%d]       SR %02X\n",activecpu_get_pc(),num,ret);
#endif
	}
	else
	{
		/* clear status port */
		mcu->sts &= ~0x01; /* RD ready */
		ret = mcu->rxd;
#if __log__
if(log)
		logerror("PC=%04X 8741[%d]       DR %02X\n",activecpu_get_pc(),num,ret);
#endif
		mcu->rst = 0;
	}
	return ret;
}

WRITE8_HANDLER( josvolly_8741_0_w ){ josvolly_8741_w(0,offset,data,1); }
READ8_HANDLER( josvolly_8741_0_r ) { return josvolly_8741_r(0,offset,1); }
WRITE8_HANDLER( josvolly_8741_1_w ) { josvolly_8741_w(1,offset,data,1); }
READ8_HANDLER( josvolly_8741_1_r ) { return josvolly_8741_r(1,offset,1); }
