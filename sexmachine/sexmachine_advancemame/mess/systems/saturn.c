/* Sega Saturn

Driver by David Haywood,Angelo Salese,Olivier Galibert & Mariusz Wojcieszek
SCSP driver provided by R.Belmont,based on ElSemi's SCSP sound chip emulator
CD Block driver provided by ANY,based on sthief original emulator
Many thanks to Guru,Fabien & Runik for the help given.

MESS conversion by R. Belmont

Hardware overview:
------------------
-two SH-2 CPUs,in a master/slave configuration.The master cpu is used to
boot-up and to do the most of the work,the slave one does extra work that could be
too much for a single cpu.They both shares all the existant devices;
-a M68000 CPU,used to drive sound(the SCSP chip).The program is uploaded via the
SH-2 cpus;
-a SMPC (System Manager & Peripheral Control),used to drive all the
devices on the board;
-a SCU (System Control Unit),mainly used to do DMA operations and to drive interrupts,it
also has a DSP;
-an (optional for the ST-V) SH-1 CPU,used to be the CD driver;
-An A-Bus,where the cart ROM area is located;
-A B-Bus,where the Video Hardware & the SCU sections are located;
-Two VDPs chips(named as 1 & 2),used for the video section:
 -VDP1 is used to render sprites & polygons.
 -VDP2 is for the tilemap system,there are:
 4 effective normal layers;
 2 roz layers;
 1 back layer;
 1 line layer;
 The VDP2 is capable of the following things (in order):
 -dynamic resolution (up to 704x512) & various interlacing modes;
 -mosaic process;
 -scrolling,scaling,horizontal & vertical cell scrolling & linescroll for the
  normal planes, the roz ones can also rotate;
 -versatile window system,used for various effects;
 -alpha-blending,refered as Color Calculation in the docs;
 -shadow effects;
 -global rgb brightness control,separate for every plane;

Memory map:
-----------

0x00000000, 0x0007ffff  BIOS ROM
0x00080000, 0x000fffff  Unused
0x00100000, 0x00100080  SMPC
0x00100080, 0x0017ffff  Unused
0x00180000, 0x0018ffff  Back Up Ram
0x00190000, 0x001fffff  Unused
0x00200000, 0x002fffff  Work Ram-L
0x00300000, 0x00ffffff  Unused
0x01000000, 0x01000003  MINIT
0x01000004, 0x017fffff  Unused
0x01800000, 0x01800003  SINIT
0x01800004, 0x01ffffff  Unused
0x02000000, 0x03ffffff  A-BUS CS0
0x04000000, 0x04ffffff  A-BUS CS1
0x05000000, 0x057fffff  A-BUS DUMMY
0x05800000, 0x058fffff  A-BUS CS2
0x05900000, 0x059fffff  Unused
0x05a00000, 0x05b00ee3  Sound Region
0x05b00ee4, 0x05bfffff  Unused
0x05c00000, 0x05cbffff  VDP 1
0x05cc0000, 0x05cfffff  Unused
0x05d00000, 0x05d00017  VDP 1 regs
0x05d00018, 0x05dfffff  Unused
0x05e00000, 0x05e7ffff  VDP2
0x05e80000, 0x05efffff  VDP2 Extra RAM,accessible thru the VRAMSZ register
0x05f00000, 0x05f00fff  VDP2 Color RAM
0x05f01000, 0x05f7ffff  Unused
0x05f80000  0x05f8011f  VDP2 regs
0x05f80120, 0x05fdffff  Unused
0x05fe0000, 0x05fe00cf  SCU regs
0x05fe00d0, 0x05ffffff  Unused
0x06000000, 0x060fffff  Work Ram-H
0x06100000, 0x07ffffff  Unused

*the unused locations aren't known if they are really unused or not,needs verification,most
of them seem just mirrors of the previous valid memory allocated.

ToDo / Notes:
-------------

-To enter into an Advanced Test Mode,keep pressed the Test Button (F2) on the start-up.

(Main issues)
-complete the Master/Slave communication.
-clean up the IC13 rom loading.
-Clean-ups and split the various chips(SCU,SMPC)into their respective files.
-CD block:complete it & add proper CD image support into MAME.
-the Cart-Dev mode...we need the proper ST-V Cart-Dev bios to be dumped in order to
 make this work,but probably this will never be done...
-fix some strange sound cpu memory accesses,there are various issues about this.
-finish the DSP core.
-Complete the window system in VDP2 (Still in progress).
-Add the RS232c interface (serial port),needed by fhboxers.
-(PCB owners) check if the clocks documented in the manuals are really right for ST-V.
-SCSP to master irq: see if there is a sound cpu mask bit.
-Does the cpunum_set_clock really works?Investigate.
-We need to check every game if can be completed or there are any hanging/crash/protection
 issues on them.
-Memo: Some tests done on the original & working PCB,to be implemented:
 -The AD-Stick returns 0x00 or a similar value.
 -The Ports E,F & G must return 0xff
 -The regular BIOS tests (Memory Test) changes his background color at some point to
  several gradients of red,green and blue.Current implementation remains black.I dunno
  if this is a framebuffer write or a funky transparency issue (i.e TRANSPARENT_NONE
  should instead show the back layer).
 -RBG0 rotating can be checked on the "Advanced Test" menu thru the VDP1/VDP2 check.
  It rotates clockwise IIRC.Also the ST-V logo when the game is in Multi mode rotates too.
 -The MIDI communication check fails even on a ST-V board,somebody needs to check if there
  is a MIDI port on the real PCB...

(per-game issues)
-groovef: hangs soon after loaded,caused by two memory addresses in the Work RAM-H range.
 Kludged for now to work.
-various: find idle skip if possible.
-suikoenb/shanhigw + others: why do we get 2 credits on startup with sound enabled?
-colmns97/puyosun/mausuke/cotton2/cottonbm: interrupt issues? we can't check the SCU mask
 on SMPC or controls fail
-mausuke/bakubaku/grdforce: need to sort out transparency on the colour mapped sprites
-bakubaku/colmns97/vfkids: no sound? Caused by missing irq?
-myfairld: Doesn't work with -sound enabled because of a sound ram check at relative
 addresses of $700/$710/$720/$730,so I'm not removing the NOT_WORKING flag due of that.Also
 Micronet programmers had the "great" idea to *not* use the ST-V input standards,infact
 joystick panel is mapped with input_port(10) instead of input_port(2),so for now use
 the mahjong panel instead.
-kiwames: the VDP1 sprites refresh is too slow,causing the "Draw by request" mode to
 flicker.Moved back to default ATM...
-pblbeach: Sprites are offset, because it doesn't clear vdp1 local coordinates set by bios,
 I guess that they are cleared when some vdp1 register is written (kludged for now)
-decathlt: Is currently using a strange protection DMA abus control,and it uses some sort of RLE
 compression/encryption that serves as a gfxdecode.
-vmahjong: the vdp1 textures are too dark(women).
-findlove: controls doesn't work?
-Weird design choices:
 introdon: has the working button as BUTTON2 and not BUTTON1
 batmanfr: BUTTON1 isn't used at all.
-seabass: Player sprite is corrupt/missing during movements,caused
 by incomplete framebuffer switching.
-sss: Missing backgrounds during gameplay. <- seems just too dark (night),probably
 just the positioning isn't correct...
-elandore: Polygons structures/textures aren't right in gameplay,known as protection
 for the humans structures,imperfect VDP1 emulation for the dragons.
-hanagumi: ending screens have corrupt graphics. (*untested*)
-znpwfv: missing Gouraud shading on distorted sprites and polygons
-znpwfv,twcup98: missing "two screens" mode in RBG emulation
-batmanfr: Missing sound,caused by an extra ADSP chip which is on the cart.The CPU is a
 ADSP-2181,and it's the same used by NBA Jam Extreme (ZN game).
-twcup98: missing Tecmo logo
-vfremix: hangs after second match
-sokyugrt: gameplay seems to be not smooth, timing?
-Here's the list of unmapped read/writes:
*<all games>
cpu #0 (PC=0000365C): unmapped program memory dword write to 057FFFFC = 000D0000 & FFFF0000
cpu #0 (PC=00003654): unmapped program memory dword write to 057FFFFC = 000C0000 & FFFF0000
*bakubaku:
cpu #0 (PC=0601022E): unmapped program memory dword write to 02000000 = 00000000 & FFFFFFFF
cpu #0 (PC=0601023A): unmapped program memory dword write to 02000000 = 00000000 & FFFFFFFF

*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/sh2/sh2.h"
#include "machine/stvcd.h"
#include "machine/scudsp.h"
#include "sound/scsp.h"
#include "devices/chd_cd.h"
#include <time.h>

extern UINT32* stv_vdp2_regs;
extern UINT32* stv_vdp2_vram;
extern UINT32* stv_vdp2_cram;
extern UINT32* stv_vdp1_vram;

#define USE_SLAVE 1

#ifdef MAME_DEBUG
#define LOG_CDB  1
#define LOG_SMPC 0
#define LOG_SCU  0
#define LOG_IRQ  0
#else
#define LOG_CDB  0
#define LOG_SMPC 0
#define LOG_SCU  0
#define LOG_IRQ  0
#endif

#define MASTER_CLOCK_352 57272800
#define MASTER_CLOCK_320 53748200

/**************************************************************************************/
/*to be added into a stv Header file,remember to remove all the static...*/

static UINT8 *smpc_ram;
//static void stv_dump_ram(void);

UINT32* stv_workram_l;
UINT32* stv_workram_h;
UINT32* stv_backupram;
UINT32* stv_scu;
static UINT32* ioga;
static UINT16* scsp_regs;
static UINT16* sound_ram;

int stv_vblank,stv_hblank;
int stv_enable_slave_sh2;
/*SMPC stuff*/
static UINT8 NMI_reset;
static void system_reset(void);
static UINT8 en_68k;
/*SCU stuff*/
static int 	  timer_0;			/* Counter for Timer 0 irq*/
static int    timer_1;          /* Counter for Timer 1 irq*/
/*Maybe add these in a struct...*/
static UINT32 scu_src_0,		/* Source DMA lv 0 address*/
			  scu_src_1,		/* lv 1*/
			  scu_src_2,		/* lv 2*/
			  scu_dst_0,		/* Destination DMA lv 0 address*/
			  scu_dst_1,		/* lv 1*/
			  scu_dst_2,		/* lv 2*/
			  scu_src_add_0,	/* Source Addition for DMA lv 0*/
			  scu_src_add_1,	/* lv 1*/
			  scu_src_add_2,	/* lv 2*/
			  scu_dst_add_0,	/* Destination Addition for DMA lv 0*/
			  scu_dst_add_1,	/* lv 1*/
			  scu_dst_add_2;	/* lv 2*/
static INT32  scu_size_0,		/* Transfer DMA size lv 0*/
			  scu_size_1,		/* lv 1*/
			  scu_size_2;		/* lv 2*/

static void dma_direct_lv0(void);	/*DMA level 0 direct transfer function*/
static void dma_direct_lv1(void);   /*DMA level 1 direct transfer function*/
static void dma_direct_lv2(void);   /*DMA level 2 direct transfer function*/
static void dma_indirect_lv0(void); /*DMA level 0 indirect transfer function*/
static void dma_indirect_lv1(void); /*DMA level 1 indirect transfer function*/
static void dma_indirect_lv2(void); /*DMA level 2 indirect transfer function*/


int minit_boost,sinit_boost;
double minit_boost_timeslice, sinit_boost_timeslice;

static int scanline;


/*A-Bus IRQ checks,where they could be located these?*/
#define ABUSIRQ(_irq_,_vector_,_mask_) \
	if(!(stv_scu[40] & _mask_)) { cpunum_set_input_line_and_vector(0, _irq_, HOLD_LINE , _vector_); }
#if 0
if(stv_scu[42] & 1)//IRQ ACK
{
	ABUSIRQ(7,0x50,0x00010000);
	ABUSIRQ(7,0x51,0x00020000);
	ABUSIRQ(7,0x52,0x00040000);
	ABUSIRQ(7,0x53,0x00080000);
	ABUSIRQ(4,0x54,0x00100000);
	ABUSIRQ(4,0x55,0x00200000);
	ABUSIRQ(4,0x56,0x00400000);
	ABUSIRQ(4,0x57,0x00800000);
	ABUSIRQ(1,0x58,0x01000000);
	ABUSIRQ(1,0x59,0x02000000);
	ABUSIRQ(1,0x5a,0x04000000);
	ABUSIRQ(1,0x5b,0x08000000);
	ABUSIRQ(1,0x5c,0x10000000);
	ABUSIRQ(1,0x5d,0x20000000);
	ABUSIRQ(1,0x5e,0x40000000);
	ABUSIRQ(1,0x5f,0x80000000);
}
#endif

/**************************************************************************************/

/*

CD Block / SH-1 Handling

*/

/* SMPC
 System Manager and Peripheral Control

*/
/* SMPC Addresses

00
01 -w  Input Register 0 (IREG)
02
03 -w  Input Register 1
04
05 -w  Input Register 2
06
07 -w  Input Register 3
08
09 -w  Input Register 4
0a
0b -w  Input Register 5
0c
0d -w  Input Register 6
0e
0f
10
11
12
13
14
15
16
17
18
19
1a
1b
1c
1d
1e
1f -w  Command Register (COMREG)
20
21 r-  Output Register 0 (OREG)
22
23 r-  Output Register 1
24
25 r-  Output Register 2
26
27 r-  Output Register 3
28
29 r-  Output Register 4
2a
2b r-  Output Register 5
2c
2d r-  Output Register 6
2e
2f r-  Output Register 7
30
31 r-  Output Register 8
32
33 r-  Output Register 9
34
35 r-  Output Register 10
36
37 r-  Output Register 11
38
39 r-  Output Register 12
3a
3b r-  Output Register 13
3c
3d r-  Output Register 14
3e
3f r-  Output Register 15
40
41 r-  Output Register 16
42
43 r-  Output Register 17
44
45 r-  Output Register 18
46
47 r-  Output Register 19
48
49 r-  Output Register 20
4a
4b r-  Output Register 21
4c
4d r-  Output Register 22
4e
4f r-  Output Register 23
50
51 r-  Output Register 24
52
53 r-  Output Register 25
54
55 r-  Output Register 26
56
57 r-  Output Register 27
58
59 r-  Output Register 28
5a
5b r-  Output Register 29
5c
5d r-  Output Register 30
5e
5f r-  Output Register 31
60
61 r-  SR
62
63 rw  SF
64
65
66
67
68
69
6a
6b
6c
6d
6e
6f
70
71
72
73
74
75 rw PDR1
76
77 rw PDR2
78
79 -w DDR1
7a
7b -w DDR2
7c
7d -w IOSEL2/1
7e
7f -w EXLE2/1
*/
UINT8 IOSEL1;
UINT8 IOSEL2;
UINT8 EXLE1;
UINT8 EXLE2;
UINT8 PDR1;
UINT8 PDR2;

#define SH2_DIRECT_MODE_PORT_1 IOSEL1 = 1
#define SH2_DIRECT_MODE_PORT_2 IOSEL2 = 1
#define SMPC_CONTROL_MODE_PORT_1 IOSEL1 = 0
#define SMPC_CONTROL_MODE_PORT_2 IOSEL2 = 0

int DectoBCD(int num)
{
	int i, cnt = 0, tmp, res = 0;

	while (num > 0) 
	{
		tmp = num;
		while (tmp >= 10) tmp %= 10;
		for (i=0; i<cnt; i++)
			tmp *= 16;
		res += tmp;
		cnt++;
		num /= 10;
	}

	return res;
}

static void system_reset()
{
	/*Only backup ram and SMPC ram are retained after that this command is issued.*/
	memset(stv_scu      ,0x00,0x000100);
	memset(scsp_regs    ,0x00,0x001000);
	memset(sound_ram    ,0x00,0x080000);
	memset(stv_workram_h,0x00,0x100000);
	memset(stv_workram_l,0x00,0x100000);
	memset(stv_vdp2_regs,0x00,0x040000);
	memset(stv_vdp2_vram,0x00,0x100000);
	memset(stv_vdp2_cram,0x00,0x080000);
	//vdp1
	//A-Bus
	/*Order is surely wrong but whatever...*/
}

static UINT8 stv_SMPC_r8 (int offset)
{
	int return_data;


	return_data = smpc_ram[offset];

	if ((offset == 0x61))
		return_data = 0x00;	// must be clear for Saturn BIOS to boot

	if (offset == 0x75)//PDR1 read
		return_data = 0xff; //readinputport(0);

	if (offset == 0x77)//PDR2 read
		return_data=  0xff; // | EEPROM_read_bit());

	if (offset == 0x33) return_data = 1;	// region code (1=japan, 4=US, 12=Europe)

	if (activecpu_get_pc()==0x060020E6) return_data = 0x10;//???

	//if(LOG_SMPC) logerror ("cpu #%d (PC=%08X) SMPC: Read from Byte Offset %02x Returns %02x\n", cpu_getactivecpu(), activecpu_get_pc(), offset, return_data);


	return return_data;
}

static void stv_SMPC_w8 (int offset, UINT8 data)
{
	time_t ltime;
	struct tm *today;
	time(&ltime);
	today = localtime(&ltime);

//  if(LOG_SMPC) logerror ("8-bit SMPC Write to Offset %02x with Data %02x\n", offset, data);
	smpc_ram[offset] = data;

	if(offset == 0x75)
	{
		EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x10);
		EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);


//      if (data & 0x01)
//          if(LOG_SMPC) logerror("bit 0 active\n");
//      if (data & 0x02)
//          if(LOG_SMPC) logerror("bit 1 active\n");
//      if (data & 0x10)
			//if(LOG_SMPC) logerror("bit 4 active\n");//LOT
		//if(LOG_SMPC) logerror("SMPC: ram [0x75] = %02x\n",smpc_ram[0x75]);
		PDR1 = (data & 0x60);
	}

	if(offset == 0x77)
	{
		/*
            ACTIVE LOW
            bit 4(0x10) - Enable Sound System
        */
		//ui_popup("PDR2 = %02x",smpc_ram[0x77]);
		if(!(smpc_ram[0x77] & 0x10))
		{
			if(LOG_SMPC) logerror("SMPC: M68k on\n");
			cpunum_set_input_line(2, INPUT_LINE_RESET, PULSE_LINE);
			cpunum_set_input_line(2, INPUT_LINE_HALT, CLEAR_LINE);
			en_68k = 1;
		}
		else
		{
			if(LOG_SMPC) logerror("SMPC: M68k off\n");
			cpunum_set_input_line(2, INPUT_LINE_HALT, ASSERT_LINE);
			en_68k = 0;
		}
		//if(LOG_SMPC) logerror("SMPC: ram [0x77] = %02x\n",smpc_ram[0x77]);
		PDR2 = (data & 0x60);
	}

	if(offset == 0x7d)
	{
		if(smpc_ram[0x7d] & 1)
			SH2_DIRECT_MODE_PORT_1;
		else
			SMPC_CONTROL_MODE_PORT_1;

		if(smpc_ram[0x7d] & 2)
			SH2_DIRECT_MODE_PORT_2;
		else
			SMPC_CONTROL_MODE_PORT_2;
	}

	if(offset == 0x7f)
	{
		//enable PAD irq & VDP2 external latch for port 1/2
		EXLE1 = smpc_ram[0x7f] & 1 ? 1 : 0;
		EXLE2 = smpc_ram[0x7f] & 2 ? 1 : 0;
		if(EXLE1 || EXLE2)
			if(!(stv_scu[40] & 0x0100)) /*Pad irq*/
			{
				if(LOG_SMPC) logerror ("Interrupt: PAD irq at scanline %04x, Vector 0x48 Level 0x08\n",scanline);
				cpunum_set_input_line_and_vector(0, 8, HOLD_LINE , 0x48);
			}
	}

	if (offset == 0x1f)
	{
		switch (data)
		{
			case 0x00:
				if(LOG_SMPC) logerror ("SMPC: Master ON\n");
				smpc_ram[0x5f]=0x00;
				break;
			//in theory 0x01 is for Master OFF,but obviously is not used.
			case 0x02:
				if(LOG_SMPC) logerror ("SMPC: Slave ON\n");
				smpc_ram[0x5f]=0x02;
				#if USE_SLAVE
				stv_enable_slave_sh2 = 1;
				cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);
				cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
				#endif
				break;
			case 0x03:
				if(LOG_SMPC) logerror ("SMPC: Slave OFF\n");
				smpc_ram[0x5f]=0x03;
				stv_enable_slave_sh2 = 0;
				cpu_trigger(1000);
				cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
				break;
			case 0x06:
				if(LOG_SMPC) logerror ("SMPC: Sound ON\n");
				/* wrong? */
				smpc_ram[0x5f]=0x06;
				cpunum_set_input_line(2, INPUT_LINE_RESET, PULSE_LINE);
				cpunum_set_input_line(2, INPUT_LINE_HALT, CLEAR_LINE);
				break;
			case 0x07:
				if(LOG_SMPC) logerror ("SMPC: Sound OFF\n");
				smpc_ram[0x5f]=0x07;
				break;
			/*CD (SH-1) ON/OFF,guess that this is needed for Sports Fishing games...*/
			//case 0x08:
			//case 0x09:
			case 0x0d:
				if(LOG_SMPC) logerror ("SMPC: System Reset\n");
				smpc_ram[0x5f]=0x0d;
				cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
				system_reset();
				break;
			case 0x0e:
				if(LOG_SMPC) logerror ("SMPC: Change Clock to 352\n");
				smpc_ram[0x5f]=0x0e;
				cpunum_set_clock(0, MASTER_CLOCK_352/2);
				cpunum_set_clock(1, MASTER_CLOCK_352/2);
				cpunum_set_clock(2, MASTER_CLOCK_352/5);
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			case 0x0f:
				if(LOG_SMPC) logerror ("SMPC: Change Clock to 320\n");
				smpc_ram[0x5f]=0x0f;
				cpunum_set_clock(0, MASTER_CLOCK_320/2);
				cpunum_set_clock(1, MASTER_CLOCK_320/2);
				cpunum_set_clock(2, MASTER_CLOCK_320/5);
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			/*"Interrupt Back"*/
			case 0x10:
				if(LOG_SMPC) logerror ("SMPC: Status Acquire\n");
				smpc_ram[0x5f]=0x10;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);
			  	smpc_ram[0x23] = DectoBCD((today->tm_year + 1900)/100);
		    	smpc_ram[0x25] = DectoBCD((today->tm_year + 1900)%100);
	    		smpc_ram[0x27] = (today->tm_wday << 4) | (today->tm_mon+1);
		    	smpc_ram[0x29] = DectoBCD(today->tm_mday);
		    	smpc_ram[0x2b] = DectoBCD(today->tm_hour);
		    	smpc_ram[0x2d] = DectoBCD(today->tm_min);
		    	smpc_ram[0x2f] = DectoBCD(today->tm_sec);

				smpc_ram[0x31]=0x00;  //?

				//smpc_ram[0x33]=readinputport(7);

				smpc_ram[0x35]=0x00;
				smpc_ram[0x37]=0x00;

				smpc_ram[0x39]=0xff;
				smpc_ram[0x3b]=0xff;
				smpc_ram[0x3d]=0xff;
				smpc_ram[0x3f]=0xff;
				smpc_ram[0x41]=0xff;
				smpc_ram[0x43]=0xff;
				smpc_ram[0x45]=0xff;
				smpc_ram[0x47]=0xff;
				smpc_ram[0x49]=0xff;
				smpc_ram[0x4b]=0xff;
				smpc_ram[0x4d]=0xff;
				smpc_ram[0x4f]=0xff;
				smpc_ram[0x51]=0xff;
				smpc_ram[0x53]=0xff;
				smpc_ram[0x55]=0xff;
				smpc_ram[0x57]=0xff;
				smpc_ram[0x59]=0xff;
				smpc_ram[0x5b]=0xff;
				smpc_ram[0x5d]=0xff;

			//  /*This is for RTC,cartridge code and similar stuff...*/
			//  if(!(stv_scu[40] & 0x0080)) /*System Manager(SMPC) irq*/ /* we can't check this .. breaks controls .. probably issues elsewhere? */
				{
					if(LOG_SMPC) logerror ("Interrupt: System Manager (SMPC) at scanline %04x, Vector 0x47 Level 0x08\n",scanline);
					cpunum_set_input_line_and_vector(0, 8, HOLD_LINE , 0x47);
				}
			break;
			/* RTC write*/
			case 0x16:
				if(LOG_SMPC) logerror("SMPC: RTC write\n");
				smpc_ram[0x2f] = smpc_ram[0x0d];
				smpc_ram[0x2d] = smpc_ram[0x0b];
				smpc_ram[0x2b] = smpc_ram[0x09];
				smpc_ram[0x29] = smpc_ram[0x07];
				smpc_ram[0x27] = smpc_ram[0x05];
				smpc_ram[0x25] = smpc_ram[0x03];
				smpc_ram[0x23] = smpc_ram[0x01];
				smpc_ram[0x5f]=0x16;
			break;
			/* SMPC memory setting*/
			case 0x17:
				if(LOG_SMPC) logerror ("SMPC: memory setting\n");
				smpc_ram[0x5f]=0x17;
			break;
			case 0x18:
				if(LOG_SMPC) logerror ("SMPC: NMI request\n");
				smpc_ram[0x5f]=0x18;
				/*NMI is unconditionally requested?*/
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
				break;
			case 0x19:
				if(LOG_SMPC) logerror ("SMPC: NMI Enable\n");
				smpc_ram[0x5f]=0x19;
				NMI_reset = 0;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);
				break;
			case 0x1a:
				if(LOG_SMPC) logerror ("SMPC: NMI Disable\n");
				smpc_ram[0x5f]=0x1a;
				NMI_reset = 1;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);

				break;
			default:
				if(LOG_SMPC) logerror ("cpu #%d (PC=%08X) SMPC: undocumented Command %02x\n", cpu_getactivecpu(), activecpu_get_pc(), data);
		}

		// we've processed the command, clear status flag
		smpc_ram[0x63] = 0x00;
		/*TODO:emulate the timing of each command...*/
	}
}


static READ32_HANDLER ( stv_SMPC_r32 )
{
	int byte = 0;
	int readdata = 0;
	/* registers are all byte accesses, convert here */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; readdata = stv_SMPC_r8(offset+byte) << 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; readdata = stv_SMPC_r8(offset+byte) << 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; readdata = stv_SMPC_r8(offset+byte) << 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; readdata = stv_SMPC_r8(offset+byte) << 0;  }

	return readdata;
}


static WRITE32_HANDLER ( stv_SMPC_w32 )
{
	int byte = 0;
	int writedata = 0;
	/* registers are all byte accesses, convert here so we can use the data more easily later */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; writedata = data >> 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; writedata = data >> 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; writedata = data >> 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; writedata = data >> 0;  }

	writedata &= 0xff;

	offset += byte;

	stv_SMPC_w8(offset,writedata);
}


/*
(Preliminary) explaination about this:
VBLANK-OUT is used at the start of the vblank period.It also sets the timer zero
variable to 0.
If the Timer Compare register is zero too,the Timer 0 irq is triggered.

HBLANK-IN is used at the end of each scanline except when in VBLANK-IN/OUT periods.

The timer 0 is also incremented by one at each HBLANK and checked with the value
of the Timer Compare register;if equal,the timer 0 irq is triggered here too.
Notice that the timer 0 compare register can be more than the VBLANK maximum range,in
this case the timer 0 irq is simply never triggered.This is a known Sega Saturn/ST-V "bug".

VBLANK-IN is used at the end of the vblank period.

SCU register[36] is the timer zero compare register.
SCU register[40] is for IRQ masking.
*/

/* to do, update bios idle skips so they work better with this arrangement.. */


static INTERRUPT_GEN( stv_interrupt )
{
	scanline = 261-cpu_getiloops();

	stv_hblank = 0;

	if(scanline == 0)
	{
		if(!(stv_scu[40] & 2))/*VBLANK-OUT*/
		{
			if(LOG_IRQ) logerror ("Interrupt: VBlank-OUT at scanline %04x, Vector 0x41 Level 0x0e\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xe, HOLD_LINE , 0x41);
		}
		stv_vblank = 0;
	}
	else if(scanline <= 223 && scanline >= 1)/*Correct?*/
	{
		timer_0++;
		timer_1++;
		stv_hblank = 1;
		/*TODO: check this...*/
		/*Timer 1 handling*/
		if((stv_scu[38] & 1))
		{
			if((!(stv_scu[40] & 0x10)) && (!(stv_scu[38] & 0x80)))
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 1 at scanline %04x, Vector 0x44 Level 0x0b\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xb, HOLD_LINE, 0x44 );
			}
			else if((!(stv_scu[40] & 0x10)) && (stv_scu[38] & 0x80))
			{
				if(timer_1 == (stv_scu[36] & 0x1ff))
				{
					if(LOG_IRQ) logerror ("Interrupt: Timer 1 at scanline %04x, Vector 0x44 Level 0x0b\n",scanline);
					cpunum_set_input_line_and_vector(0, 0xb, HOLD_LINE, 0x44 );
				}
			}
		}
		if(timer_0 == (stv_scu[36] & 0x1ff))
		{
			if(!(stv_scu[40] & 8))/*Timer 0*/
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 0 at scanline %04x, Vector 0x43 Level 0x0c\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xc, HOLD_LINE, 0x43 );
			}
		}

		/*TODO:use this *at the end* of the draw line.*/
		if(!(stv_scu[40] & 4))/*HBLANK-IN*/
		{
			if(LOG_IRQ) logerror ("Interrupt: HBlank-In at scanline %04x, Vector 0x42 Level 0x0d\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xd, HOLD_LINE , 0x42);
		}
	}
	else if(scanline == 224)
	{
		timer_0 = 0;
		timer_1 = 0;

		if(!(stv_scu[40] & 1))/*VBLANK-IN*/
		{
			if(LOG_IRQ) logerror ("Interrupt: VBlank IN at scanline %04x, Vector 0x40 Level 0x0f\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xf, HOLD_LINE , 0x40);
		}
		stv_vblank = 1;

		if(timer_0 == (stv_scu[36] & 0x1ff))
		{
			if(!(stv_scu[40] & 8))/*Timer 0*/
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 0 at scanline %04x, Vector 0x43 Level 0x0c\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xc, HOLD_LINE, 0x43 );
			}
		}
		if(!(stv_scu[40] & 0x2000)) /*Sprite draw end irq*/
			cpunum_set_input_line_and_vector(0, 2, HOLD_LINE , 0x4d);
	}
}

/*
I/O overview:
    PORT-A  1st player inputs
    PORT-B  2nd player inputs
    PORT-C  system input
    PORT-D  system output
    PORT-E  I/O 1
    PORT-F  I/O 2
    PORT-G  I/O 3
    PORT-AD AD-Stick inputs?(Fake for now...)
    SERIAL COM

offsets:
    0h PORT-A
    0l PORT-B
    1h PORT-C
    1l PORT-D
    2h PORT-E
    2l PORT-F (extra button layout)
    3h PORT-G
    3l
    4h PORT-SEL
    4l
    5h SERIAL COM WRITE
    5l
    6h SERIAL COM READ
    6l
    7h
    7l PORT-AD
*/
static UINT8 port_ad[] =
{
	0xcc,0xb2,0x99,0x7f,0x66,0x4c,0x33,0x19
};

static UINT8 port_sel,mux_data;

#define HI_WORD_ACCESS (mem_mask & 0x00ff0000) == 0
#define LO_WORD_ACCESS (mem_mask & 0x000000ff) == 0

READ32_HANDLER ( stv_io_r32 )
{
	static int i= -1;

	switch(offset)
	{
		case 0:
		switch(port_sel)
		{
			case 0x77: return 0xff000000|(readinputport(2) << 16) |0x0000ff00|(readinputport(3));
			case 0x67:
			{
				switch(mux_data)
				{
					/*Mahjong panel interface,bit wise(ACTIVE LOW)*/
					case 0xfe:	return 0xff000000 | (readinputport(7)  << 16) | 0x0000ff00 | (readinputport(12));
					case 0xfd:  return 0xff000000 | (readinputport(8)  << 16) | 0x0000ff00 | (readinputport(13));
					case 0xfb:	return 0xff000000 | (readinputport(9)  << 16) | 0x0000ff00 | (readinputport(14));
					case 0xf7:	return 0xff000000 | (readinputport(10) << 16) | 0x0000ff00 | (readinputport(15));
					case 0xef:  return 0xff000000 | (readinputport(11) << 16) | 0x0000ff00 | (readinputport(16));
					/*Joystick panel*/
					default:
					//ui_popup("%02x MUX DATA",mux_data);
				    return (readinputport(2) << 16) | (readinputport(3));
				}
			}
			//default:
			default:
			//ui_popup("%02x PORT SEL",port_sel);
			return (readinputport(2) << 16) | (readinputport(3));
		}
		case 1:
		return (readinputport(4) << 16) | (ioga[1]);
		case 2:
		switch(port_sel)
		{
			case 0x77:	return (readinputport(5) << 16) | (readinputport(6));
			case 0x67:	return 0xffffffff;/**/
			case 0x20:  return 0xffff0000 | (ioga[2] & 0xffff);
			case 0x10:  return ((ioga[2] & 0xffff) << 16) | 0xffff;
			case 0x60:  return 0xffffffff;/**/
			default:
			//ui_popup("offs: 2 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 3:
		switch(port_sel)
		{
			case 0x60:  return ((ioga[2] & 0xffff) << 16) | 0xffff;
			default:
			//ui_popup("offs: 3 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 6:
		switch(port_sel)
		{
			case 0x60:  return ioga[5];
			default:
			//ui_popup("offs: 6 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 7:
		ui_popup("Read from PORT_AD");
		i++;
		return port_ad[i & 7];
		default:
		return ioga[offset];
	}
}

WRITE32_HANDLER ( stv_io_w32 )
{
	switch(offset)
	{
		case 1:
			if(LO_WORD_ACCESS)
			{
				/*Why does the BIOS tests these as ACTIVE HIGH?A program bug?*/
				ioga[1] = (data) & 0xff;
				coin_counter_w(0,~data & 0x01);
				coin_counter_w(1,~data & 0x02);
				coin_lockout_w(0,~data & 0x04);
				coin_lockout_w(1,~data & 0x08);
				/*
                other bits reserved
                */
			}
		break;
		case 2:
			if(HI_WORD_ACCESS)
			{
				ioga[2] = data >> 16;
				mux_data = ioga[2];
			}
			else if(LO_WORD_ACCESS)
				ioga[2] = data;
		break;
		case 3:
			if(HI_WORD_ACCESS)
				ioga[3] = data;
		break;
		case 4:
			if(HI_WORD_ACCESS)
				port_sel = (data & 0xffff0000) >> 16;
		break;
		case 5:
			if(HI_WORD_ACCESS)
				ioga[5] = data;
		break;
	}
}

/*

SCU Handling

*/

/**********************************************************************************
SCU Register Table
offset,relative address
Registers are in long words.
===================================================================================
0     0000  Level 0 DMA Set Register
1     0004
2     0008
3     000c
4     0010
5     0014
6     0018
7     001c
8     0020  Level 1 DMA Set Register
9     0024
10    0028
11    002c
12    0030
13    0034
14    0038
15    003c
16    0040  Level 2 DMA Set Register
17    0044
18    0048
19    004c
20    0050
21    0054
22    0058
23    005c
24    0060  DMA Forced Stop
25    0064
26    0068
27    006c
28    0070  <Free>
29    0074
30    0078
31    007c  DMA Status Register
32    0080  DSP Program Control Port
33    0084  DSP Program RAM Data Port
34    0088  DSP Data RAM Address Port
35    008c  DSP Data RAM Data Port
36    0090  Timer 0 Compare Register
37    0094  Timer 1 Set Data Register
38    0098  Timer 1 Mode Register
39    009c  <Free>
40    00a0  Interrupt Mask Register
41    00a4  Interrupt Status Register
42    00a8  A-Bus Interrupt Acknowledge
43    00ac  <Free>
44    00b0  A-Bus Set Register
45    00b4
46    00b8  A-Bus Refresh Register
47    00bc  <Free>
48    00c0
49    00c4  SCU SDRAM Select Register
50    00c8  SCU Version Register
51    00cc  <Free>
52    00cf
===================================================================================
DMA Status Register(32-bit):
xxxx xxxx x--- xx-- xx-- xx-- xx-- xx-- UNUSED
---- ---- -x-- ---- ---- ---- ---- ---- DMA DSP-Bus access
---- ---- --x- ---- ---- ---- ---- ---- DMA B-Bus access
---- ---- ---x ---- ---- ---- ---- ---- DMA A-Bus access
---- ---- ---- --x- ---- ---- ---- ---- DMA lv 1 interrupt
---- ---- ---- ---x ---- ---- ---- ---- DMA lv 0 interrupt
---- ---- ---- ---- --x- ---- ---- ---- DMA lv 2 in stand-by
---- ---- ---- ---- ---x ---- ---- ---- DMA lv 2 in operation
---- ---- ---- ---- ---- --x- ---- ---- DMA lv 1 in stand-by
---- ---- ---- ---- ---- ---x ---- ---- DMA lv 1 in operation
---- ---- ---- ---- ---- ---- --x- ---- DMA lv 0 in stand-by
---- ---- ---- ---- ---- ---- ---x ---- DMA lv 0 in operation
---- ---- ---- ---- ---- ---- ---- --x- DSP side DMA in stand-by
---- ---- ---- ---- ---- ---- ---- ---x DSP side DMA in operation

**********************************************************************************/
/*
DMA TODO:
-Verify if there are any kind of bugs,do clean-ups,use better comments
 and macroize for better reading...
-Add timings(but how fast are each DMA?).
-Add level priority & DMA status register.
-Add DMA start factor conditions that are different than 7.
-Add byte data type transfer.
-Set boundaries.
*/

#define DIRECT_MODE(_lv_)			(!(stv_scu[5+(_lv_*8)] & 0x01000000))
#define INDIRECT_MODE(_lv_)			  (stv_scu[5+(_lv_*8)] & 0x01000000)
#define DRUP(_lv_)					  (stv_scu[5+(_lv_*8)] & 0x00010000)
#define DWUP(_lv_)                    (stv_scu[5+(_lv_*8)] & 0x00000100)

#define DMA_STATUS				(stv_scu[31])
/*These macros sets the various DMA status flags.*/
#define D0MV_1	if(!(DMA_STATUS & 0x10))    DMA_STATUS^=0x10
#define D1MV_1	if(!(DMA_STATUS & 0x100))   DMA_STATUS^=0x100
#define D2MV_1	if(!(DMA_STATUS & 0x1000))  DMA_STATUS^=0x1000
#define D0MV_0	if(DMA_STATUS & 0x10) 	    DMA_STATUS^=0x10
#define D1MV_0	if(DMA_STATUS & 0x100) 	    DMA_STATUS^=0x100
#define D2MV_0	if(DMA_STATUS & 0x1000)     DMA_STATUS^=0x1000

UINT32 scu_index_0,scu_index_1,scu_index_2;
static UINT8 scsp_to_main_irq;

static UINT32 scu_add_tmp;

/*For area checking*/
#define ABUS(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x02000000) && ((scu_##_lv_ & 0x07ffffff) <= 0x04ffffff)
#define BBUS(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x05a00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05ffffff)
#define VDP1_REGS(_lv_)  ((scu_##_lv_ & 0x07ffffff) >= 0x05d00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05dfffff)
#define VDP2(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x05e00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05fdffff)
#define WORK_RAM_L(_lv_) ((scu_##_lv_ & 0x07ffffff) >= 0x00200000) && ((scu_##_lv_ & 0x07ffffff) <= 0x002fffff)
#define WORK_RAM_H(_lv_) ((scu_##_lv_ & 0x07ffffff) >= 0x06000000) && ((scu_##_lv_ & 0x07ffffff) <= 0x060fffff)
#define SOUND_RAM(_lv_)  ((scu_##_lv_ & 0x07ffffff) >= 0x05a00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05afffff)

READ32_HANDLER( stv_scu_r32 )
{
	/*TODO: write only registers must return 0...*/
	//ui_popup("%02x",DMA_STATUS);
	//if (offset == 23)
	//{
		//Super Major League reads here???
	//}

	// Saturn BIOS needs this (need to investigate further)
	if (offset == 32)
	{
		return 0x00100000;
	}

	if (offset == 31)
	{
		if(LOG_SCU) logerror("(PC=%08x) DMA status reg read\n",activecpu_get_pc());
		return stv_scu[offset];
	}
	else if ( offset == 35 )
	{
        if(LOG_SCU) logerror( "DSP mem read at %08X\n", stv_scu[34]);
        return dsp_ram_addr_r();
    }
    else if( offset == 41)
    {
		logerror("(PC=%08x) IRQ status reg read\n",activecpu_get_pc());
		/*TODO:for now we're activating everything here,but we need to return the proper active irqs*/
		return 0xffffffff;
	}
	else if( offset == 50 )
	{
		logerror("(PC=%08x) SCU version reg read\n",activecpu_get_pc());
		return 0x00000000;/*SCU Version 0*/
	}
    else
    {
    	if(LOG_SCU) logerror("(PC=%08x) SCU reg read at %d = %08x\n",activecpu_get_pc(),offset,stv_scu[offset]);
    	return stv_scu[offset];
   	}
}

WRITE32_HANDLER( stv_scu_w32 )
{
	COMBINE_DATA(&stv_scu[offset]);

	switch(offset)
	{
		/*LV 0 DMA*/
		case 0:	scu_src_0  = ((stv_scu[0] & 0x07ffffff) >> 0); break;
		case 1:	scu_dst_0  = ((stv_scu[1] & 0x07ffffff) >> 0); break;
		case 2: scu_size_0 = ((stv_scu[2] & 0x000fffff) >> 0); break;
		case 3:
			/*Read address add value for DMA lv 0*/
			if(stv_scu[3] & 0x100)
				scu_src_add_0 = 4;
			else
				scu_src_add_0 = 1;

			/*Write address add value for DMA lv 0*/
			switch(stv_scu[3] & 7)
			{
				case 0: scu_dst_add_0 = 2;   break;
				case 1: scu_dst_add_0 = 4;   break;
				case 2: scu_dst_add_0 = 8;   break;
				case 3: scu_dst_add_0 = 16;  break;
				case 4: scu_dst_add_0 = 32;  break;
				case 5: scu_dst_add_0 = 64;  break;
				case 6: scu_dst_add_0 = 128; break;
				case 7: scu_dst_add_0 = 256; break;
			}
			break;
		case 4:
/*
-stv_scu[4] bit 0 is DMA starting bit.
    Used when the start factor is 7.Toggle after execution.
-stv_scu[4] bit 8 is DMA Enable bit.
    This is an execution mask flag.
-stv_scu[5] bit 0,bit 1 and bit 2 is DMA starting factor.
    It must be 7 for this specific condition.
-stv_scu[5] bit 24 is Indirect Mode/Direct Mode (0/1).
*/
		if(stv_scu[4] & 1 && ((stv_scu[5] & 7) == 7) && stv_scu[4] & 0x100)
		{
			if(DIRECT_MODE(0)) { dma_direct_lv0(); }
			else			   { dma_indirect_lv0(); }

			stv_scu[4]^=1;//disable starting bit.

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
				#if 0
				if((scu_dst_0 & 0x7ffffff) != 0x05a00000)
				{
					if(!(stv_scu[40] & 0x1000))
					{
						cpunum_set_input_line_and_vector(0, 3, HOLD_LINE, 0x4c);
						logerror("SCU: Illegal DMA interrupt\n");
					}
				}
				#endif
			}
		}
		break;
		case 5:
		if(INDIRECT_MODE(0))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 0 set\n");
			if(!DWUP(0)) scu_index_0 = scu_dst_0;
		}

		/*Start factor enable bits,bit 2,bit 1 and bit 0*/
		if((stv_scu[5] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 0 = %d\n",stv_scu[5] & 7);
		break;
		/*LV 1 DMA*/
		case 8:	 scu_src_1  = ((stv_scu[8] &  0x07ffffff) >> 0);  break;
		case 9:	 scu_dst_1  = ((stv_scu[9] &  0x07ffffff) >> 0);  break;
		case 10: scu_size_1 = ((stv_scu[10] & 0x00001fff) >> 0);  break;
		case 11:
		/*Read address add value for DMA lv 1*/
		if(stv_scu[11] & 0x100)
			scu_src_add_1 = 4;
		else
			scu_src_add_1 = 1;

		/*Write address add value for DMA lv 1*/
		switch(stv_scu[11] & 7)
		{
			case 0: scu_dst_add_1 = 2;   break;
			case 1: scu_dst_add_1 = 4;   break;
			case 2: scu_dst_add_1 = 8;   break;
			case 3: scu_dst_add_1 = 16;  break;
			case 4: scu_dst_add_1 = 32;  break;
			case 5: scu_dst_add_1 = 64;  break;
			case 6: scu_dst_add_1 = 128; break;
			case 7: scu_dst_add_1 = 256; break;
		}
		break;
		case 12:
		if(stv_scu[12] & 1 && ((stv_scu[13] & 7) == 7) && stv_scu[12] & 0x100)
		{
			if(DIRECT_MODE(1)) { dma_direct_lv1(); }
			else			   { dma_indirect_lv1(); }

			stv_scu[12]^=1;

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
			}
		}
		break;
		case 13:
		if(INDIRECT_MODE(1))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 1 set\n");
			if(!DWUP(1)) scu_index_1 = scu_dst_1;
		}

		if((stv_scu[13] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 1 = %d\n",stv_scu[13] & 7);
		break;
		/*LV 2 DMA*/
		case 16: scu_src_2  = ((stv_scu[16] & 0x07ffffff) >> 0);  break;
		case 17: scu_dst_2  = ((stv_scu[17] & 0x07ffffff) >> 0);  break;
		case 18: scu_size_2 = ((stv_scu[18] & 0x00001fff) >> 0);  break;
		case 19:
		/*Read address add value for DMA lv 2*/
		if(stv_scu[19] & 0x100)
			scu_src_add_2 = 4;
		else
			scu_src_add_2 = 1;

		/*Write address add value for DMA lv 2*/
		switch(stv_scu[19] & 7)
		{
			case 0: scu_dst_add_2 = 2;   break;
			case 1: scu_dst_add_2 = 4;   break;
			case 2: scu_dst_add_2 = 8;   break;
			case 3: scu_dst_add_2 = 16;  break;
			case 4: scu_dst_add_2 = 32;  break;
			case 5: scu_dst_add_2 = 64;  break;
			case 6: scu_dst_add_2 = 128; break;
			case 7: scu_dst_add_2 = 256; break;
		}
		break;
		case 20:
		if(stv_scu[20] & 1 && ((stv_scu[21] & 7) == 7) && stv_scu[20] & 0x100)
		{
			if(DIRECT_MODE(2)) { dma_direct_lv2(); }
			else			   { dma_indirect_lv2(); }

			stv_scu[20]^=1;

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
			}
		}
		break;
		case 21:
		if(INDIRECT_MODE(2))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 2 set\n");
			if(!DWUP(2)) scu_index_2 = scu_dst_2;
		}

		if((stv_scu[21] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 2 = %d\n",stv_scu[21] & 7);
		break;
		case 24:
		if(LOG_SCU) logerror("DMA Forced Stop Register set = %02x\n",stv_scu[24]);
		break;
		case 31: if(LOG_SCU) logerror("Warning: DMA status WRITE! Offset %02x(%d)\n",offset*4,offset); break;
		/*DSP section*/
		/*Use functions so it is easier to work out*/
		case 32:
		dsp_prg_ctrl(data);
		if(LOG_SCU) logerror("SCU DSP: Program Control Port Access %08x\n",data);
		break;
		case 33:
		dsp_prg_data(data);
		if(LOG_SCU) logerror("SCU DSP: Program RAM Data Port Access %08x\n",data);
		break;
		case 34:
		dsp_ram_addr_ctrl(data);
		if(LOG_SCU) logerror("SCU DSP: Data RAM Address Port Access %08x\n",data);
		break;
		case 35:
		dsp_ram_addr_w(data);
		if(LOG_SCU) logerror("SCU DSP: Data RAM Data Port Access %08x\n",data);
		break;
		case 36: if(LOG_SCU) logerror("timer 0 compare data = %03x\n",stv_scu[36]);break;
		case 37: if(LOG_SCU) logerror("timer 1 set data = %08x\n",stv_scu[37]); break;
		case 38: if(LOG_SCU) logerror("timer 1 mode data = %08x\n",stv_scu[38]); break;
		case 40:
		/*An interrupt is masked when his specific bit is 1.*/
		/*Are bit 16-bit 31 for External A-Bus irq mask like the status register?*/
		/*What is 0x00000080 setting?Is it valid?*/

		/*Take out the common settings to keep logging quiet.*/
		if(stv_scu[40] != 0xfffffffe &&
		   stv_scu[40] != 0xfffffffc &&
		   stv_scu[40] != 0xffffffff)
		{
			if(LOG_SCU) logerror("cpu #%d (PC=%08X) IRQ mask reg set %08x = %d%d%d%d|%d%d%d%d|%d%d%d%d|%d%d%d%d\n",
			cpu_getactivecpu(), activecpu_get_pc(),
			stv_scu[offset],
			stv_scu[offset] & 0x8000 ? 1 : 0, /*A-Bus irq*/
			stv_scu[offset] & 0x4000 ? 1 : 0, /*<reserved>*/
			stv_scu[offset] & 0x2000 ? 1 : 0, /*Sprite draw end irq(VDP1)*/
			stv_scu[offset] & 0x1000 ? 1 : 0, /*Illegal DMA irq*/
			stv_scu[offset] & 0x0800 ? 1 : 0, /*Lv 0 DMA end irq*/
			stv_scu[offset] & 0x0400 ? 1 : 0, /*Lv 1 DMA end irq*/
			stv_scu[offset] & 0x0200 ? 1 : 0, /*Lv 2 DMA end irq*/
			stv_scu[offset] & 0x0100 ? 1 : 0, /*PAD irq*/
			stv_scu[offset] & 0x0080 ? 1 : 0, /*System Manager(SMPC) irq*/
			stv_scu[offset] & 0x0040 ? 1 : 0, /*Snd req*/
			stv_scu[offset] & 0x0020 ? 1 : 0, /*DSP irq end*/
			stv_scu[offset] & 0x0010 ? 1 : 0, /*Timer 1 irq*/
			stv_scu[offset] & 0x0008 ? 1 : 0, /*Timer 0 irq*/
			stv_scu[offset] & 0x0004 ? 1 : 0, /*HBlank-IN*/
			stv_scu[offset] & 0x0002 ? 1 : 0, /*VBlank-OUT*/
			stv_scu[offset] & 0x0001 ? 1 : 0);/*VBlank-IN*/
		}
		break;
		case 41:
		/*This is r/w by introdon...*/
		if(LOG_SCU) logerror("IRQ status reg set:%08x\n",stv_scu[41]);
		break;
		case 42: if(LOG_SCU) logerror("A-Bus IRQ ACK %08x\n",stv_scu[42]); break;
		case 49: if(LOG_SCU) logerror("SCU SDRAM set: %02x\n",stv_scu[49]); break;
		default: if(LOG_SCU) logerror("Warning: unused SCU reg set %d = %08x\n",offset,data);
	}
}

static void dma_direct_lv0()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 0 transfer START\n"
			             "Start %08x End %08x Size %04x\n",scu_src_0,scu_dst_0,scu_size_0);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_0,scu_dst_add_0);

	D0MV_1;

	if(scu_size_0 == 0) scu_size_0 = 0x00100000;

	scsp_to_main_irq = 0;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_0))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_0 != scu_src_add_0) && (ABUS(src_0)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = scu_src_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these three*/
	if(ABUS(dst_0))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_0))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP2(src_0))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(VDP1_REGS(dst_0))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = scu_src_add_0 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(DRUP(0))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_src_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_0) && (scu_dst_add_0 != 4))
	{
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_0;
	if(!(DRUP(0))) tmp_src = scu_src_0;
	if(!(DWUP(0))) tmp_dst = scu_dst_0;

	for (; scu_size_0 > 0; scu_size_0-=scu_dst_add_0)
	{
		if(scu_dst_add_0 == 2)
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
		else if(scu_dst_add_0 == 8)
		{
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+2,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+4,program_read_word(scu_src_0+2));
			program_write_word(scu_dst_0+6,program_read_word(scu_src_0+2));
		}
		else
		{
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+2,program_read_word(scu_src_0+2));
		}

		scu_dst_0+=scu_dst_add_0;
		scu_src_0+=scu_src_add_0;
	}

	scu_size_0 = tmp_size;
	if(!(DRUP(0))) scu_src_0 = tmp_src;
	if(!(DWUP(0))) scu_dst_0 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x800))/*Lv 0 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 5, HOLD_LINE , 0x4b);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_0 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_0 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D0MV_0;
}

static void dma_direct_lv1()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 1 transfer START\n"
			 "Start %08x End %08x Size %04x\n",scu_src_1,scu_dst_1,scu_size_1);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_1,scu_dst_add_1);

	D1MV_1;

	if(scu_size_1 == 0) scu_size_1 = 0x00002000;

	scsp_to_main_irq = 0;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_1))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_1 != scu_src_add_1) && (ABUS(src_1)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_dst_add_1 = scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these ones*/
	if(ABUS(dst_1))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_1))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP1_REGS(dst_1))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_dst_add_1 = scu_src_add_1 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(VDP2(src_1))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(DRUP(1))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_1) && (scu_dst_add_1 != 4))
	{
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_1;
	if(!(DRUP(1))) tmp_src = scu_src_1;
	if(!(DWUP(1))) tmp_dst = scu_dst_1;

	for (; scu_size_1 > 0; scu_size_1-=scu_dst_add_1)
	{
		if(scu_dst_add_1 == 2)
			program_write_word(scu_dst_1,program_read_word(scu_src_1));
		else
		{
			program_write_word(scu_dst_1,program_read_word(scu_src_1));
			program_write_word(scu_dst_1+2,program_read_word(scu_src_1+2));
		}

		scu_dst_1+=scu_dst_add_1;
		scu_src_1+=scu_src_add_1;
	}

	scu_size_1 = tmp_size;
	if(!(DRUP(1))) scu_src_1 = tmp_src;
	if(!(DWUP(1))) scu_dst_1 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x400))/*Lv 1 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x4a);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_1 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_1 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D1MV_0;
}

static void dma_direct_lv2()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 2 transfer START\n"
			 "Start %08x End %08x Size %04x\n",scu_src_2,scu_dst_2,scu_size_2);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_2,scu_dst_add_2);

	D2MV_1;

	scsp_to_main_irq = 0;

	if(scu_size_2 == 0) scu_size_2 = 0x00002000;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_2))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_2 != scu_src_add_2) && (ABUS(src_2)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_dst_add_2 = scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these ones*/
	if(ABUS(dst_2))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_2))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP1_REGS(dst_2))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_dst_add_2 = scu_src_add_2 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(VDP2(src_2))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(DRUP(2))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_2) && (scu_dst_add_2 != 4))
	{
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_2;
	if(!(DRUP(2))) tmp_src = scu_src_2;
	if(!(DWUP(2))) tmp_dst = scu_dst_2;

	for (; scu_size_2 > 0; scu_size_2-=scu_dst_add_2)
	{
		if(scu_dst_add_2 == 2)
			program_write_word(scu_dst_2,program_read_word(scu_src_2));
		else
		{
			program_write_word(scu_dst_2,program_read_word(scu_src_2));
			program_write_word(scu_dst_2+2,program_read_word(scu_src_2+2));
		}

		scu_dst_2+=scu_dst_add_2;
		scu_src_2+=scu_src_add_2;
	}

	scu_size_2 = tmp_size;
	if(!(DRUP(2))) scu_src_2 = tmp_src;
	if(!(DWUP(2))) scu_dst_2 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x200))/*Lv 2 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x49);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_2 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_2 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D2MV_0;
}

static void dma_indirect_lv0()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D0MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_0 == 0) { scu_index_0 = scu_dst_0; }

	do{
		tmp_src = scu_index_0;

		/*Thanks for Runik of Saturnin for pointing this out...*/
		scu_size_0 = program_read_dword(scu_index_0);
		scu_src_0 =  program_read_dword(scu_index_0+8);
		scu_dst_0 =  program_read_dword(scu_index_0+4);

		/*Indirect Mode end factor*/
		if(scu_src_0 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_0))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 0 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_0,scu_dst_0,scu_size_0);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_0,scu_dst_add_0);

		//guess,but I believe it's right.
		scu_src_0 &=0x07ffffff;
		scu_dst_0 &=0x07ffffff;
		scu_size_0 &=0xfffff;

		for (; scu_size_0 > 0; scu_size_0-=scu_dst_add_0)
		{
			if(scu_dst_add_0 == 2)
				program_write_word(scu_dst_0,program_read_word(scu_src_0));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_0,program_read_word(scu_src_0));
				program_write_word(scu_dst_0+2,program_read_word(scu_src_0+2));
			}
			scu_dst_0+=scu_dst_add_0;
			scu_src_0+=scu_src_add_0;
		}

		//if(DRUP(0))   program_write_dword(tmp_src+8,scu_src_0|job_done ? 0x80000000 : 0);
		//if(DWUP(0)) program_write_dword(tmp_src+4,scu_dst_0);

		scu_index_0 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x800))/*Lv 0 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 5, HOLD_LINE , 0x4b);

	D0MV_0;
}

static void dma_indirect_lv1()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D1MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_1 == 0) { scu_index_1 = scu_dst_1; }

	do{
		tmp_src = scu_index_1;

		scu_size_1 = program_read_dword(scu_index_1);
		scu_src_1 =  program_read_dword(scu_index_1+8);
		scu_dst_1 =  program_read_dword(scu_index_1+4);

		/*Indirect Mode end factor*/
		if(scu_src_1 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_1))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 1 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_1,scu_dst_1,scu_size_1);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_1,scu_dst_add_1);

		//guess,but I believe it's right.
		scu_src_1 &=0x07ffffff;
		scu_dst_1 &=0x07ffffff;
		scu_size_1 &=0xffff;


		for (; scu_size_1 > 0; scu_size_1-=scu_dst_add_1)
		{

			if(scu_dst_add_1 == 2)
				program_write_word(scu_dst_1,program_read_word(scu_src_1));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_1,program_read_word(scu_src_1));
				program_write_word(scu_dst_1+2,program_read_word(scu_src_1+2));
			}
			scu_dst_1+=scu_dst_add_1;
			scu_src_1+=scu_src_add_1;
		}

		//if(DRUP(1))   program_write_dword(tmp_src+8,scu_src_1|job_done ? 0x80000000 : 0);
		//if(DWUP(1)) program_write_dword(tmp_src+4,scu_dst_1);

		scu_index_1 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x400))/*Lv 1 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x4a);

	D1MV_0;
}

static void dma_indirect_lv2()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D2MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_2 == 0) { scu_index_2 = scu_dst_2; }

	do{
		tmp_src = scu_index_2;

		scu_size_2 = program_read_dword(scu_index_2);
		scu_src_2 =  program_read_dword(scu_index_2+8);
		scu_dst_2 =  program_read_dword(scu_index_2+4);

		/*Indirect Mode end factor*/
		if(scu_src_2 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_2))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 2 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_2,scu_dst_2,scu_size_2);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_2,scu_dst_add_2);

		//guess,but I believe it's right.
		scu_src_2 &=0x07ffffff;
		scu_dst_2 &=0x07ffffff;
		scu_size_2 &=0xffff;

		for (; scu_size_2 > 0; scu_size_2-=scu_dst_add_2)
		{
			if(scu_dst_add_2 == 2)
				program_write_word(scu_dst_2,program_read_word(scu_src_2));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_2,program_read_word(scu_src_2));
				program_write_word(scu_dst_2+2,program_read_word(scu_src_2+2));
			}

			scu_dst_2+=scu_dst_add_2;
			scu_src_2+=scu_src_add_2;
		}

		//if(DRUP(2))   program_write_dword(tmp_src+8,scu_src_2|job_done ? 0x80000000 : 0);
		//if(DWUP(2)) program_write_dword(tmp_src+4,scu_dst_2);

		scu_index_2 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x200))/*Lv 2 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x49);

	D2MV_0;
}


/**************************************************************************************/

WRITE32_HANDLER( stv_sh2_soundram_w )
{
	COMBINE_DATA(sound_ram+offset*2+1);
	data >>= 16;
	mem_mask >>= 16;
	COMBINE_DATA(sound_ram+offset*2);
}

READ32_HANDLER( stv_sh2_soundram_r )
{
	return (sound_ram[offset*2]<<16)|sound_ram[offset*2+1];
}

static READ32_HANDLER( stv_scsp_regs_r32 )
{
	offset <<= 1;
	return (SCSP_0_r(offset+1, 0xffff) | (SCSP_0_r(offset, 0xffff)<<16));
}

static WRITE32_HANDLER( stv_scsp_regs_w32 )
{
	offset <<= 1;
	SCSP_0_w(offset, data>>16, mem_mask >> 16);
	SCSP_0_w(offset+1, data, mem_mask);
}

/* communication,SLAVE CPU acquires data from the MASTER CPU and triggers an irq.  *
 * Enter into Radiant Silver Gun specific menu for a test...                       */
static WRITE32_HANDLER( minit_w )
{
	logerror("cpu #%d (PC=%08X) MINIT write = %08x\n",cpu_getactivecpu(), activecpu_get_pc(),data);
	cpu_boost_interleave(minit_boost_timeslice, TIME_IN_USEC(minit_boost));
	cpu_trigger(1000);
	cpunum_set_info_int(1, CPUINFO_INT_SH2_FRT_INPUT, PULSE_LINE);
}

static WRITE32_HANDLER( sinit_w )
{
	logerror("cpu #%d (PC=%08X) SINIT write = %08x\n",cpu_getactivecpu(), activecpu_get_pc(),data);
	cpu_boost_interleave(sinit_boost_timeslice, TIME_IN_USEC(sinit_boost));
	cpunum_set_info_int(0, CPUINFO_INT_SH2_FRT_INPUT, PULSE_LINE);
}

extern WRITE32_HANDLER ( stv_vdp2_vram_w );
extern READ32_HANDLER ( stv_vdp2_vram_r );

extern WRITE32_HANDLER ( stv_vdp2_cram_w );
extern READ32_HANDLER ( stv_vdp2_cram_r );

extern WRITE32_HANDLER ( stv_vdp2_regs_w );
extern READ32_HANDLER ( stv_vdp2_regs_r );

extern VIDEO_START ( stv_vdp2 );
extern VIDEO_UPDATE( stv_vdp2 );

extern READ32_HANDLER( stv_vdp1_regs_r );
extern WRITE32_HANDLER( stv_vdp1_regs_w );
extern READ32_HANDLER ( stv_vdp1_vram_r );
extern WRITE32_HANDLER ( stv_vdp1_vram_w );

extern WRITE32_HANDLER ( stv_vdp1_framebuffer0_w );
extern READ32_HANDLER ( stv_vdp1_framebuffer0_r );

extern WRITE32_HANDLER ( stv_vdp1_framebuffer1_w );
extern READ32_HANDLER ( stv_vdp1_framebuffer1_r );

static READ32_HANDLER( stv_sh2_random_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( saturn_unk_r )
{
	return 0;
}

static ADDRESS_MAP_START( saturn_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM   // bios
	AM_RANGE(0x00100000, 0x0010007f) AM_READWRITE(stv_SMPC_r32, stv_SMPC_w32)
	AM_RANGE(0x00200000, 0x002fffff) AM_RAM AM_MIRROR(0x100000) AM_SHARE(2) AM_BASE(&stv_workram_l)
	AM_RANGE(0x01000000, 0x01000003) AM_WRITE(minit_w)
	AM_RANGE(0x01406f40, 0x01406f43) AM_WRITE(minit_w) // prikura seems to write here ..
//  AM_RANGE(0x01000000, 0x01000003) AM_WRITE(minit_w) AM_MIRROR(0x00080000)
	AM_RANGE(0x01800000, 0x01800003) AM_WRITE(sinit_w)
	AM_RANGE(0x02000000, 0x04ffffef) AM_READNOP AM_WRITENOP	// cartridge space
	AM_RANGE(0x04fffff0, 0x04ffffff) AM_READNOP AM_WRITENOP //WRITE(a_bus_ctrl_r,a_bus_ctrl_w)
	AM_RANGE(0x05800000, 0x0589ffff) AM_READWRITE(stvcd_r, stvcd_w)
	/* Sound */
	AM_RANGE(0x05a00000, 0x05a7ffff) AM_READWRITE(stv_sh2_soundram_r, stv_sh2_soundram_w)
	//AM_RANGE(0x05a80000, 0x05afffff) AM_READ(stv_sh2_random_r)
	AM_RANGE(0x05b00000, 0x05b00fff) AM_READWRITE(stv_scsp_regs_r32, stv_scsp_regs_w32)
	/* VDP1 */
	/*0x05c00000-0x05c7ffff VRAM*/
	/*0x05c80000-0x05c9ffff Frame Buffer 0*/
	/*0x05ca0000-0x05cbffff Frame Buffer 1*/
	/*0x05d00000-0x05d7ffff VDP1 Regs */
	AM_RANGE(0x05c00000, 0x05c7ffff) AM_READWRITE(stv_vdp1_vram_r, stv_vdp1_vram_w)
	AM_RANGE(0x05c80000, 0x05cbffff) AM_READWRITE(stv_vdp1_framebuffer0_r, stv_vdp1_framebuffer0_w)
	AM_RANGE(0x05d00000, 0x05d0001f) AM_READWRITE(stv_vdp1_regs_r, stv_vdp1_regs_w)
	AM_RANGE(0x05e00000, 0x05efffff) AM_READWRITE(stv_vdp2_vram_r, stv_vdp2_vram_w)
	AM_RANGE(0x05f00000, 0x05f7ffff) AM_READWRITE(stv_vdp2_cram_r, stv_vdp2_cram_w)
	AM_RANGE(0x05f80000, 0x05fbffff) AM_READWRITE(stv_vdp2_regs_r, stv_vdp2_regs_w)
	AM_RANGE(0x05fe0000, 0x05fe00cf) AM_READWRITE(stv_scu_r32, stv_scu_w32)
	AM_RANGE(0x06000000, 0x060fffff) AM_RAM AM_MIRROR(0x01f00000) AM_SHARE(3) AM_BASE(&stv_workram_h)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_RAM AM_REGION(REGION_CPU3, 0) AM_BASE(&sound_ram)
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(SCSP_0_r, SCSP_0_w)
ADDRESS_MAP_END

#define STV_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(_n_)

INPUT_PORTS_START( saturn )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
/*
    PORT_START
    STV_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

    PORT_START
    STV_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	/*This *might* be unused...*/
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Extra button layout,used by Power Instinct 3 & Suikoenbu*/
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/*We don't need these,AFAIK the country code doesn't work either...*/
	#if 0
	PORT_START							//7
	PORT_DIPNAME( 0x0f, 0x01, "Country" )
	PORT_DIPSETTING(    0x01, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x02, "Asia Ntsc" )
	PORT_DIPSETTING(    0x04, DEF_STR( USA ) )
	PORT_DIPSETTING(    0x08, "Sud America Ntsc" )
	PORT_DIPSETTING(    0x06, "Korea" )
	PORT_DIPSETTING(    0x0a, "Asia Pal" )
	PORT_DIPSETTING(    0x0c, "Europe/Other Pal" )
	PORT_DIPSETTING(    0x0d, "Sud America Pal" )

	PORT_START	/* Pad data 1a */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("B") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("C") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("A") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Start") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Up") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Down") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Left") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Right") PORT_CODE(KEYCODE_L)

	PORT_START	/* Pad data 1b */
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("L trig") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Z") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Y") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("X") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("R trig") PORT_CODE(KEYCODE_S)
	#endif
INPUT_PORTS_END

DRIVER_INIT ( saturn )
{
	time_t ltime;
	struct tm *today;
	time(&ltime);
	today = localtime(&ltime);

	/* amount of time to boost interleave for on MINIT / SINIT, needed for communication to work */
	minit_boost = 400;
	sinit_boost = 400;
	minit_boost_timeslice = 0;
	sinit_boost_timeslice = 0;

	smpc_ram = auto_malloc (0x80);
	stv_scu = auto_malloc (0x100);
	scsp_regs = auto_malloc (0x1000);

  	smpc_ram[0x23] = DectoBCD((today->tm_year + 1900)/100);
    smpc_ram[0x25] = DectoBCD((today->tm_year + 1900)%100);
    smpc_ram[0x27] = (today->tm_wday << 4) | (today->tm_mon+1);
    smpc_ram[0x29] = DectoBCD(today->tm_mday);
    smpc_ram[0x2b] = DectoBCD(today->tm_hour);
    smpc_ram[0x2d] = DectoBCD(today->tm_min);
    smpc_ram[0x2f] = DectoBCD(today->tm_sec);
    smpc_ram[0x31] = 0x00; //CTG1=0 CTG0=0 (correct??)
//  smpc_ram[0x33] = readinputport(7);
 	smpc_ram[0x5f] = 0x10;
}

MACHINE_RESET( saturn )
{
	// don't let the slave cpu and the 68k go anywhere
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	stv_enable_slave_sh2 = 0;
	cpunum_set_input_line(2, INPUT_LINE_HALT, ASSERT_LINE);

	timer_0 = 0;
	timer_1 = 0;
	en_68k = 0;
	NMI_reset = 1;
	smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);

	cpunum_set_clock(0, MASTER_CLOCK_320/2);
	cpunum_set_clock(1, MASTER_CLOCK_320/2);
	cpunum_set_clock(2, MASTER_CLOCK_320/5);

	stvcd_reset();
}

static const gfx_layout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
	  32*8+0, 32*8+4, 32*8+8, 32*8+12, 32*8+16, 32*8+20, 32*8+24, 32*8+28,

	  },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  32*16, 32*17,32*18, 32*19,32*20,32*21,32*22,32*23

	  },
	32*32
};

static const gfx_layout tiles8x8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static const gfx_layout tiles16x16x8_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56,
	64*8+0, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8

	},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	64*16, 64*17, 64*18, 64*19, 64*20, 64*21, 64*22, 64*23
	},
	128*16
};




static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x4_layout,   0x00, (0x80*(2+1))  },
	{ REGION_GFX1, 0, &tiles16x16x4_layout, 0x00, (0x80*(2+1))  },
	{ REGION_GFX1, 0, &tiles8x8x8_layout,   0x00, (0x08*(2+1))  },
	{ REGION_GFX1, 0, &tiles16x16x8_layout, 0x00, (0x08*(2+1))  },

	/* vdp1 .. pointless for drawing but can help us debug */
	{ REGION_GFX2, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ REGION_GFX2, 0, &tiles16x16x4_layout, 0x00, 0x100  },
	{ REGION_GFX2, 0, &tiles8x8x8_layout,   0x00, 0x20  },
	{ REGION_GFX2, 0, &tiles16x16x8_layout, 0x00, 0x20  },

	{ -1 } /* end of array */
};

struct sh2_config sh2_conf_master = { 0 };
struct sh2_config sh2_conf_slave  = { 1 };

static int scsp_last_line = 0;

static void scsp_irq(int irq)
{
	// don't bother the 68k if it's off
	if (!en_68k)
	{
		return;
	}

	if (irq)
	{
		scsp_last_line = irq;
		cpunum_set_input_line(2, irq, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(2, scsp_last_line, CLEAR_LINE);
	}
}

static struct SCSPinterface scsp_interface =
{
	REGION_CPU3,
	0,
	scsp_irq
};

static MACHINE_DRIVER_START( saturn )

	/* basic machine hardware */
	MDRV_CPU_ADD(SH2, MASTER_CLOCK_352/2) // 28.6364 MHz
	MDRV_CPU_PROGRAM_MAP(saturn_mem, 0)
	MDRV_CPU_VBLANK_INT(stv_interrupt,264)/*264 lines,224 display lines*/
	MDRV_CPU_CONFIG(sh2_conf_master)

	MDRV_CPU_ADD(SH2, MASTER_CLOCK_352/2) // 28.6364 MHz
	MDRV_CPU_PROGRAM_MAP(saturn_mem, 0)
	MDRV_CPU_CONFIG(sh2_conf_slave)

	MDRV_CPU_ADD(M68000, MASTER_CLOCK_352/5) //11.46 MHz
	MDRV_CPU_PROGRAM_MAP(sound_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(192);	// guess, needed to force video update after V-Blank OUT interrupt

	MDRV_MACHINE_RESET(saturn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT )
	MDRV_SCREEN_SIZE(1024, 1024)
	MDRV_VISIBLE_AREA(0*8, 703, 0*8, 512) // we need to use a resolution as high as the max size it can change to
	MDRV_PALETTE_LENGTH(2048+(2048*2))//standard palette + extra memory for rgb brightness.
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(stv_vdp2)
	MDRV_VIDEO_UPDATE(stv_vdp2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START(saturn)
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD("sega_101.bin", 0x00000000, 0x00080000, CRC(224b752c))
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */
	ROM_COPY( REGION_CPU1,0,0,0x080000)
	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* VDP2 GFX */
	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* VDP1 GFX */
ROM_END

ROM_START(saturnus)
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD("sega_us.bin", 0x00000000, 0x00080000, CRC(f90f0089))
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */
	ROM_COPY( REGION_CPU1,0,0,0x080000)
	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* VDP2 GFX */
	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* VDP1 GFX */
ROM_END

ROM_START(saturneu)
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD("sega_eu.bin", 0x00000000, 0x00080000, CRC(4afcf0fa))
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */
	ROM_COPY( REGION_CPU1,0,0,0x080000)
	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* VDP2 GFX */
	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* VDP1 GFX */
ROM_END

static void saturn_chdcd_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* CHD CD-ROM */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default: cdrom_device_getinfo(devclass, state, info);
	}
}

SYSTEM_CONFIG_START( saturn )
	CONFIG_DEVICE(saturn_chdcd_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY	FULLNAME */
CONS(1994, saturn,	0,	0,	saturn,	saturn,	saturn,	saturn,	"Sega",	"Saturn (Japan v1.01)", GAME_NOT_WORKING)
CONS(1994, saturnus,	0,	0,	saturn,	saturn,	saturn,	saturn,	"Sega",	"Saturn (US)", GAME_NOT_WORKING)
CONS(1994, saturneu,	0,	0,	saturn,	saturn,	saturn,	saturn,	"Sega",	"Saturn (Europe)", GAME_NOT_WORKING)


