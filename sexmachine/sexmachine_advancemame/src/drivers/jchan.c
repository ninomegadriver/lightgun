/*
    Jackie Chan The Kung-Fu Master
    Jackie Chan in Fists of Fire
    (c) Kaneko 1995

WIP Driver by Sebastien Volpe, based on "this does nothing" by Haze ;)

 started: May 12 2004

Checks done by main code:
- as part of EEPROM data
jchan  : "95/05/24 Jackie ChanVer 1.20"
jchan2 : "95/11/28 Jackie ChanVer 2.31"
- as (one of) MCU protection cmd
jchan  : "1995/05/24 The kung-Fu Master Jackie Chan   "
jchan2 : "1995/10/24 Fists Of Fire"

STILL TO DO:
  - video registers meaning:
    have a look at subcpu routines $11,$12 (different registers init according flipscreen dsw)

  - sound: can be guessed from DSW and what main68k does with 'DemoSounds On/Off'
    subcpu routines $05,$06,$07 + elsewhere  (search for $10600c in dasm)
    status/data register is probably $10600c + 2/6 (hooked up but commented out for now)

  - player controls not really understood - they end up @ $2000b1.b,$2000b5.b, see $21e2a

  - sub68k IT not hooked up (except the one triggered by main68k)
    NOTE: triggering any IT at VBL makes self-test HANG!

DONE:
  - almost figured out Main<->Sub 68k communication (read below)
  - started MCU simulation, following the existing implementation of kaneko16.c
  - located (at least main) sprite RAM and format
  - inputs (coin, controls, dsw) - controls not understood! :/

 turns out to be quite similar to other kaneko16 games (main 68k + mcu),
 mcu communication triggering is identical to bloodwar & gtmr,
 except it has a sub 68k that seems to drive a 2nd video chip (and sound)?

 main2sub comunication is done within $400000-$403fff (mainsub_shared_ram):
 - $403C02(W) : ]
 - $403C04(W) : ] main68k sets parameters before calling subcpu routine, when required
 - $403C06(W) : ]
 - $403FFE(W) : main68k writes cmd.w, which triggers IT4 on sub68k
 - $400002(W) : subcpu busy status (0=busy, 0xffff=cmd done) - main68k often clears it first and wait 0xffff
 - $400004(W) : subcpu result, read by maincpu, when required
 - $400000(W) : subcpu writes cmd.w, which triggers IT3 on main68k (*)

(*) this happens @ $5b4(cmd 3),$5f2(cmd 1) (diagnostic routine), IT4(cmd 7), and inside subcpu routines $18,$19

99.99% of main->sub communication is done the following:

        clr.w   $400002.l           ; clear (shared ram) sub-cpu busy status word
        move.w  #cmd, $403ffe.l     ; call subcpu
wait    tst.w   $400002.l           ; read (shared ram) sub-cpu busy status word
        beq     wait                ; active wait-loop


********

    there are error-msgs for (palette/sprite/bg) RAM + sub*(palette/sprite/bg) RAM, suggesting each 68k holds palette/sprite/bg ???

    main spriteram is located @ $500000-$5005ff

    sprite format is similar as what's described in vidhrdw\kaneko16.c:
    -> each field is coded on a long, lo-word is zeroes except code
    .L  attributes
    .L  code.w|color.w ???
    .L  x << 6
    .L  y << 6

*/

/*
 probably IT 1,2 triggered at a time for maincpu and IT 1,2,3 for subcpu ?

main 68k interrupts (others are rte)
lev 1 : 0x64 : 0000 1ed8 - counter, dsw, controls, + more to find
lev 2 : 0x68 : 0000 1f26 -
lev 3 : 0x6c : 0000 1f68 - comm. with sub68k, triggered when sub writes cmd @ $400000

sub 68k interrupts (others are rte)
lev 1 : 0x64 : 0000 103a -
lev 2 : 0x68 : 0000 1066 -
lev 3 : 0x6c : 0000 10a4 -
lev 4 : 0x70 : 0000 10d0 - comm. with main68k, triggered when main writes cmd @ $403ffe

*/

/*

Developers note:

This document is primarily intended for person attempting to emulate this game.

Complete dump of...

Kung Fu Master - Jackie Chan (C) Kaneko 1995

KANEKO PCB NUMBER: JC01X00047

CPU: TMP68HC000N-16 x 2
SND: YAMAHA YMZ280B
OSC: 16.0000MHZ, 33.3333MHZ, 28.6364MHZ
DIPS:1 DIP LABELLED SW2, 8 POSITION
     Location for SW1 on PCB, but empty.

Eproms

Location    Rom Type    PCB Label
U164        27C2001     SPA-7A
U165        27C2001     SPA-7B
U13     27C1001     27C1001A
U56     27C2001     23C8001E
U67     27C040      27C4001
U68     27C040      27C4001
U69     27C040      27C4001
U70     27C040      27C4001
U86     27C040      27C4001
U87     27C040      27C4001


there are 12 mask roms (42 pin) labelled....

Rom Label           Label on PCB        Location
JC-100-00  9511 D       SPA-0           U179
JC-101-00  9511 D       SPA-1           U180
JC-102-00  9511 D       SPA-2           U181
JC-103-00  9511 D       SPA-3           U182
JC-104-00  T39 9510K7092    SPA-4           U183
JC-105-00  T40 9510K7094    SPA-5           U184
JC-108-00  T65 9517K7012    SPA-6           U185
JC-106-00  T41 9510K7091    SPB-0           U171
JC-107-00  T42 9510K7096    SPB-1           U172
JC-200-00  W10 9510K7055    BG-0            U177
JC-300-00  T43 9510K7098    23C16000        U84
JC-301-00  W11 9510K7059    23C16000        U85

there are other positions for mask roms, but they are empty. the locations are labelled SPB-2, SPB-3 and SPA-7.
perhaps this pcb is used for other Kaneko games?

The mask roms have been dumped in 4 meg banks with a 40 to 42 pin adapter.
Since this is my first 42 pin dump, I'm not 100% sure my dumping method is correct.
i intended to put them together as parts A + B + C + D.= MASKROM
i have left the parts separate incase i got the quarters mixed up somehow
so i leave it up to you to put them back together. if they are mixed up, i dumped them all the same way,
so when you figure out the correct order, just rename them the same way when putting them back together.
I would appreciate some feedback on whether these are dumped correctly.

i have the original manual for this game aswell, which has the dips and other info. i've already made it available to Mamedev.
if you need it, just email me and i'll send it to you.

I can see a large space on the PCB near the JAMMA edge connector. Written on the PCB is MC1091. I would assume this is some sort of
protection module, which unfortunately someone has ripped off the pcb. Yep, that's right, this PCB is not working.

Other chips
located next to U13 is a small 8 pin chip..... AMTEL AT93C46
LATTICE pLSI 2032-80LJ (x 2, square, socketed)
FUJITSU CG24143 4181 9449 Z01 (x 2, square SMD)
FUJITSU CG24173 6186 9447 Z01 (x 2, square SMD)
KANEKO VIEW2-CHIP 1633F1208 (square, SMD)
KANEKO BABY004 9511EX009 VT-171 (square, SMD)
KANEKO TBS0P01 452 9430HK001 (square, SMD)


Ram i can see...
SONY CXK58257ASP-10L (x 8)
NEC D42101C-3 (x 4)
SHARP LH5497D-20 (x 2)
SANYO LC3564SM-85 (x 12, SMD)
NEC 42S4260-70 (x 4, SMD)

there are 9 PALS on the pcb (not dumped)

*/

/*  this one is not gurus dump

524288  DeflatX  70733  87%  03-08-2000  22:26  b1aadc5a --wa  Jm00x3-U68   // 68k code (Main)
524288  DeflatX 156800  71%  03-08-2000  22:25  c0adb141 --wa  Jm01x3-U67   // 68k code (Main)
524288  DeflatX  38310  93%  03-08-2000  22:27  d2e3f913 --wa  Jm11x3-U69   // 68k code (Main)
524288  DeflatX  54963  90%  03-08-2000  22:28  ee08fee1 --wa  Jm10x3-U70   // 68k code (Main)

524288  DeflatX  25611  96%  03-08-2000  22:23  d15d2b8e --wa  Jsp1x3-U86   // 68k code (2nd)
524288  DeflatX  18389  97%  03-08-2000  22:24  ebec50b1 --wa  Jsp0x3-U87   // 68k code (2nd)

// SPA-x
2097152  DeflatX 1203535  43%  03-08-2000  21:06  c38c5f84 --wa  Jc-100-00
2097152  DeflatX 1555500  26%  03-08-2000  21:07  cc47d68a --wa  Jc-101-00
2097152  DeflatX 1446459  32%  03-08-2000  22:12  e08f1dee --wa  Jc-102-00
2097152  DeflatX 1569324  26%  03-08-2000  22:14  ce0c81d8 --wa  Jc-103-00
2097152  DeflatX 1535427  27%  03-08-2000  22:16  6b2a2e93 --wa  Jc-104-00
2097152  DeflatX 236167   89%  03-08-2000  22:19  73cad1f0 --wa  Jc-105-00
2097152  DeflatX 1138206  46%  03-08-2000  22:21  67dd1131 --wa  Jc-108-00

// SPB-x
2097152  DeflatX 1596989  24%  03-08-2000  20:59  bc65661b --wa  Jc-106-00
2097152  DeflatX 1571950  26%  03-08-2000  21:02  92a86e8b --wa  Jc-107-00

// BG
1048576  DeflatX 307676  71%  03-08-2000  21:03  1f30c24e --wa  Jc-200-00

// AUDIO
2097152  DeflatX 1882895  11%  03-08-2000  20:57  13d5b1eb --wa  Jc-300-00
1048576  DeflatX 858475   19%  03-08-2000  22:42  9c5b3077 --wa  Jc-301-00

// AUDIO2 ?
 262144  DeflatX 218552  17%  03-08-2000  22:31  bcf25c2a --wa  Jcw0x0-U56

// MCU DATA?
 131072  DeflatX 123787   6%  03-08-2000  22:28  2a41da9c --wa  Jcd0x1-U13

// UNKNOWNS
 262144  DeflatX 160330  39%  03-08-2000  22:29  9a012cbc --wa  Jcs0x3-U164
 262144  DeflatX 160491  39%  03-08-2000  22:30  57ae7c8d --wa  Jcs1x3-U165

 */

#include "driver.h"
#include "sound/ymz280b.h"

extern UINT32* skns_spc_regs;


/***************************************************************************

                            MCU Code Simulation
                (follows the implementation of kaneko16.c)

Provided we found a working PCB, trojan code will help:
- to get this game working (but there are many #4 sub-commands!)
- to have more patterns and eventualy defeat the MCU 'encryption'

This will benefit galpani3 and other kaneko16 games with TOYBOX MCU.

***************************************************************************/
static UINT16 *mcu_ram, jchan_mcu_com[4];

void jchan_mcu_run(void)
{
	UINT16 mcu_command = mcu_ram[0x0010/2];		/* command nb */
	UINT16 mcu_offset  = mcu_ram[0x0012/2] / 2;	/* offset in shared RAM where MCU will write */
	UINT16 mcu_subcmd  = mcu_ram[0x0014/2];		/* sub-command parameter, happens only for command #4 */

	logerror("CPU #0 (PC=%06X) : MCU executed command: %04X %04X %04X ",activecpu_get_pc(),mcu_command,mcu_offset*2,mcu_subcmd);

/*
    the only MCU commands found in program code are:
    - 0x04: protection: provide data (see below) and code
    - 0x03: read DSW
    - 0x02: load game settings \ stored in ATMEL AT93C46 chip,
    - 0x42: save game settings / 128 bytes serial EEPROM
*/

	switch (mcu_command >> 8)
	{
		case 0x04: /* Protection: during self-test for mcu_subcmd = 0x3d, 0x3e, 0x3f */
		{
			switch(mcu_subcmd)
			{
				case 0x3d: /* $f3c ($f34-$f66) */
					/* MCU writes the string "USMM0713-TB1994 " to shared ram */
					mcu_ram[mcu_offset + 0] = 0x5553; mcu_ram[mcu_offset + 1] = 0x4D4D;
					mcu_ram[mcu_offset + 2] = 0x3037; mcu_ram[mcu_offset + 3] = 0x3133;
					mcu_ram[mcu_offset + 4] = 0x2D54; mcu_ram[mcu_offset + 5] = 0x4231;
					mcu_ram[mcu_offset + 6] = 0x3939; mcu_ram[mcu_offset + 7] = 0x3420;
					break;
				case 0x3e:
					if ( strcmp(Machine->gamedrv->name, "jchan") == 0 ) /* $f72 ($f6a-$fc6) */
					{
						/* MCU writes the string "1995/05/24 The kung-Fu Master Jackie Chan   " to shared ram */
						mcu_ram[mcu_offset +  0] = 0x3139; mcu_ram[mcu_offset +  1] = 0x3935;
						mcu_ram[mcu_offset +  2] = 0x2F30; mcu_ram[mcu_offset +  3] = 0x352F;
						mcu_ram[mcu_offset +  4] = 0x3234; mcu_ram[mcu_offset +  5] = 0x2054;
						mcu_ram[mcu_offset +  6] = 0x6865; mcu_ram[mcu_offset +  7] = 0x206B;
						mcu_ram[mcu_offset +  8] = 0x756E; mcu_ram[mcu_offset +  9] = 0x672D;
						mcu_ram[mcu_offset + 10] = 0x4675; mcu_ram[mcu_offset + 11] = 0x204D;
						mcu_ram[mcu_offset + 12] = 0x6173; mcu_ram[mcu_offset + 13] = 0x7465;
						mcu_ram[mcu_offset + 14] = 0x7220; mcu_ram[mcu_offset + 15] = 0x4A61;
						mcu_ram[mcu_offset + 16] = 0x636B; mcu_ram[mcu_offset + 17] = 0x6965;
						mcu_ram[mcu_offset + 18] = 0x2043; mcu_ram[mcu_offset + 19] = 0x6861;
						mcu_ram[mcu_offset + 20] = 0x6E20; mcu_ram[mcu_offset + 21] = 0x2020;
					}
					else if ( strcmp(Machine->gamedrv->name, "jchan2") == 0 )
					{
						/* MCU writes the string "1995/10/24 Fists Of Fire" to shared ram */
						mcu_ram[mcu_offset +  0] = 0x3139; mcu_ram[mcu_offset +  1] = 0x3935;
						mcu_ram[mcu_offset +  2] = 0x2F31; mcu_ram[mcu_offset +  3] = 0x302F;
						mcu_ram[mcu_offset +  4] = 0x3234; mcu_ram[mcu_offset +  5] = 0x2046;
						mcu_ram[mcu_offset +  6] = 0x6973; mcu_ram[mcu_offset +  7] = 0x7473;
						mcu_ram[mcu_offset +  8] = 0x204F; mcu_ram[mcu_offset +  9] = 0x6620;
						mcu_ram[mcu_offset + 10] = 0x4669; mcu_ram[mcu_offset + 11] = 0x7265;
						mcu_ram[mcu_offset + 12] = 0x6173; mcu_ram[mcu_offset + 13] = 0x7465;
					}
					break;
				case 0x3f: /* $fd2 ($fca-$101a) */
					/* MCU writes the string "(C)1995 KANEKO // TEAM JACKIE CHAN  " to shared ram */
					mcu_ram[mcu_offset +  0] = 0x2843; mcu_ram[mcu_offset +  1] = 0x2931;
					mcu_ram[mcu_offset +  2] = 0x3939; mcu_ram[mcu_offset +  3] = 0x3520;
					mcu_ram[mcu_offset +  4] = 0x4B41; mcu_ram[mcu_offset +  5] = 0x4E45;
					mcu_ram[mcu_offset +  6] = 0x4B4F; mcu_ram[mcu_offset +  7] = 0x202F;
					mcu_ram[mcu_offset +  8] = 0x2F20; mcu_ram[mcu_offset +  9] = 0x5445;
					mcu_ram[mcu_offset + 10] = 0x414D; mcu_ram[mcu_offset + 11] = 0x204A;
					mcu_ram[mcu_offset + 12] = 0x4143; mcu_ram[mcu_offset + 13] = 0x4B49;
					mcu_ram[mcu_offset + 14] = 0x4520; mcu_ram[mcu_offset + 15] = 0x4348;
					mcu_ram[mcu_offset + 16] = 0x414E; mcu_ram[mcu_offset + 17] = 0x2020;
					break;

				case 0x1b: /* $12dbe, $12dd4 */
				case 0x2d: /* $209f4 */
				default:   /* $1b3e0, $1ca40, $20a12, $20b0e, $2140a, $21436, $214cc, $21552 (dynamic) */
					mcu_ram[mcu_offset] = 0x4e75; // faked with an RTS for now
					logerror("- UNKNOWN PARAMETER %02X", mcu_subcmd);
			}
			logerror("\n");
		}
		break;

		case 0x03: 	// DSW
		{
			mcu_ram[mcu_offset] = readinputport(3);
			logerror("PC=%06X : MCU executed command: %04X %04X (read DSW)\n",activecpu_get_pc(),mcu_command,mcu_offset*2);
		}
		break;

		case 0x02: /* load game settings from 93C46 EEPROM ($1090-$10dc) */
		{
/*
Current feeling of devs is that this EEPROM might also play a role in the protection scheme,
but I (SV) feel that it is very unlikely because of the following, which has been verified:
if the checksum test fails at most 3 times, then the initial settings, stored in main68k ROM,
are loaded in RAM then saved with cmd 0x42 (see code @ $5196 & $50d4)
*/
#if 0
			int	i;

			/* MCU writes 128 bytes to shared ram: last byte is the byte-sum */
			/* first 32 bytes (header): 0x8BE08E71.L, then the string:       */
			/* "95/05/24 Jackie ChanVer 1.20" for jchan (the one below)      */
			/* "95/11/28 Jackie ChanVer 2.31" for jchan2                     */
			mcu_ram[mcu_offset +  0] = 0x8BE0; mcu_ram[mcu_offset +  1] = 0x8E71;
			mcu_ram[mcu_offset +  2] = 0x3935; mcu_ram[mcu_offset +  3] = 0x2F30;
			mcu_ram[mcu_offset +  4] = 0x352F; mcu_ram[mcu_offset +  5] = 0x3234;
			mcu_ram[mcu_offset +  6] = 0x204A; mcu_ram[mcu_offset +  7] = 0x6163;
			mcu_ram[mcu_offset +  8] = 0x6B69; mcu_ram[mcu_offset +  9] = 0x6520;
			mcu_ram[mcu_offset + 10] = 0x4368; mcu_ram[mcu_offset + 11] = 0x616E;
			mcu_ram[mcu_offset + 12] = 0x5665; mcu_ram[mcu_offset + 13] = 0x7220;
			mcu_ram[mcu_offset + 14] = 0x312E; mcu_ram[mcu_offset + 15] = 0x3230;
			/* next 12 bytes - initial NVRAM settings */
			mcu_ram[mcu_offset + 16] = 0x0001; mcu_ram[mcu_offset + 17] = 0x0101;
			mcu_ram[mcu_offset + 18] = 0x0100; mcu_ram[mcu_offset + 19] = 0x0310;
			mcu_ram[mcu_offset + 20] = 0x1028; mcu_ram[mcu_offset + 21] = 0x0201;
			/* rest is zeroes */
			for (i=22;i<63;i++)
				mcu_ram[mcu_offset + i] = 0;
			/* and sum is $62.b */
			mcu_ram[mcu_offset + 63] = 0x0062;
#endif
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
			{
				mame_fread(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
			logerror("(load NVRAM settings)\n");
		}
		break;

		case 0x42: /* save game settings to 93C46 EEPROM ($50d4) */
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
			logerror("(save NVRAM settings)\n");
		}
		break;

		default:
			logerror("- UNKNOWN COMMAND!!!\n");
	}
}

#define JCHAN_MCU_COM_W(_n_) \
WRITE16_HANDLER( jchan_mcu_com##_n_##_w ) \
{ \
	COMBINE_DATA(&jchan_mcu_com[_n_]); \
	if (jchan_mcu_com[0] != 0xFFFF)	return; \
	if (jchan_mcu_com[1] != 0xFFFF)	return; \
	if (jchan_mcu_com[2] != 0xFFFF)	return; \
	if (jchan_mcu_com[3] != 0xFFFF)	return; \
\
	memset(jchan_mcu_com, 0, 4 * sizeof( UINT16 ) ); \
	jchan_mcu_run(); \
}

JCHAN_MCU_COM_W(0)
JCHAN_MCU_COM_W(1)
JCHAN_MCU_COM_W(2)
JCHAN_MCU_COM_W(3)

READ16_HANDLER( jchan_mcu_status_r )
{
	logerror("cpu #%d (PC=%06X): read mcu status\n", cpu_getactivecpu(), activecpu_get_previouspc());
	return 0;
}

/***************************************************************************

 vidhrdw

***************************************************************************/

static UINT16 *jchan_spriteram;

INTERRUPT_GEN( jchan_vblank )
{
	if (!cpu_getiloops())
		cpunum_set_input_line(0, 1, HOLD_LINE);
	else
		cpunum_set_input_line(0, 2, HOLD_LINE);
}



VIDEO_START(jchan)
{
	/* so we can use suprnova.c */
	buffered_spriteram32 = auto_malloc ( 0x4000 );
	spriteram_size = 0x4000;
	skns_spc_regs = auto_malloc (0x40);

	return 0;
}

extern void skns_drawsprites( mame_bitmap *bitmap, const rectangle *cliprect );


VIDEO_UPDATE(jchan)
{
	fillbitmap(bitmap, get_black_pen(), cliprect);

	skns_drawsprites (bitmap,cliprect);
}

/***************************************************************************

 controls - working like this but not really understood :/

***************************************************************************/
/*
    controls handling routine is $21e2a, part of IT 1
    player 1/2 controls are read from $f00000/$f00002 resp.
    $f00006 is read and impacts controls 'decoding'
    $f00000 is the only location also written
*/
static UINT16 *jchan_ctrl;

WRITE16_HANDLER( jchan_ctrl_w )
{
// Player 1 buttons C/D for are ON
// Coin 1 affects Button C and sometimes(!) makes Player 2 buttons C/D both ON definitively
// Coin 2 affects Button D and sometimes(!) makes Player 2 buttons C/D both ON definitively
	jchan_ctrl[6/2] = data;

// both players C/D buttons don't work
	jchan_ctrl[6/2] = -1;
}

READ16_HANDLER ( jchan_ctrl_r )
{
	switch(offset)
	{
		case 0/2: return readinputport(0); // Player 1 controls
		case 2/2: return readinputport(1); // Player 2 controls
		default: logerror("jchan_ctrl_r unknown!"); break;
	}
	return jchan_ctrl[offset];
}

/***************************************************************************

 memory maps

***************************************************************************/

static UINT16 *mainsub_shared_ram;

#define main2sub_cmd      mainsub_shared_ram[0x03ffe/2]
#define main2sub_status   mainsub_shared_ram[0x00002/2]
#define main2sub_result   mainsub_shared_ram[0x00004/2]
#define main2sub_param(n) mainsub_shared_ram[(0x03c02 + (n))/2]
#define sub2main_cmd      mainsub_shared_ram[0x00000/2]

WRITE16_HANDLER( main2sub_cmd_w )
{
	COMBINE_DATA(&main2sub_cmd);
	logerror("cpu #%d (PC=%06X): write cmd %04x to subcpu\n", cpu_getactivecpu(), activecpu_get_previouspc(), main2sub_cmd);
	cpunum_set_input_line(1, 4, HOLD_LINE);
}
READ16_HANDLER ( main2sub_status_r )
{
	return main2sub_status;
}
WRITE16_HANDLER( main2sub_status_w )
{
	COMBINE_DATA(&main2sub_status);
	logerror("cpu #%d (PC=%06X): write status (%04x)\n", cpu_getactivecpu(), activecpu_get_previouspc(), main2sub_status);
}
READ16_HANDLER ( main2sub_result_r )
{
	logerror("cpu #%d (PC=%06X): read subcpu result (%04x)\n", cpu_getactivecpu(), activecpu_get_previouspc(), main2sub_result);
	return main2sub_result;
}
WRITE16_HANDLER( main2sub_unknown )
{
#define mainsub_unknown (0x400100+offset/2)
	COMBINE_DATA(&mainsub_shared_ram[offset]);
	logerror("cpu #%d (PC=%06X): write unknown (%06X):%04x to subcpu\n", cpu_getactivecpu(), activecpu_get_previouspc(), mainsub_unknown, main2sub_param(offset));
}

WRITE16_HANDLER( main2sub_param_w )
{
	COMBINE_DATA(&main2sub_param(offset));
	logerror("cpu #%d (PC=%06X): write param(%d):%04x to subcpu\n", cpu_getactivecpu(), activecpu_get_previouspc(), offset, main2sub_param(offset));
}

WRITE16_HANDLER( sub2main_cmd_w )
{
	COMBINE_DATA(&sub2main_cmd);
	logerror("cpu #%d (PC=%06X): write cmd %04x to maincpu\n", cpu_getactivecpu(), activecpu_get_previouspc(), sub2main_cmd);
	cpunum_set_input_line(0, 3, HOLD_LINE);
}
READ16_HANDLER ( sub2main_cmd_r )
{
	logerror("cpu #%d (PC=%06X): read cmd %04x from subcpu\n", cpu_getactivecpu(), activecpu_get_previouspc(), sub2main_cmd);
	return sub2main_cmd;
}

WRITE16_HANDLER( jchan_suprnova_sprite32_w )
{
//  UINT32 dat32;

	COMBINE_DATA(&jchan_spriteram[offset]);

	/* put in buffered_spriteram32 for suprnova.c */
	offset>>=1;
	buffered_spriteram32[offset]=(jchan_spriteram[offset*2+1]<<16) | (jchan_spriteram[offset*2]);
}

UINT16* jchan_sprregs;

WRITE16_HANDLER( jchan_suprnova_sprite32regs_w )
{
//  UINT32 dat32;

	COMBINE_DATA(&jchan_sprregs[offset]);

	/* put in skns_spc_regs for suprnova.c */
	offset>>=1;
	skns_spc_regs[offset]=(jchan_sprregs[offset*2+1]<<16) | (jchan_sprregs[offset*2]);
}

static ADDRESS_MAP_START( jchan_main, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM // Work RAM - [A] grid tested, cleared ($9d6-$a54)

	AM_RANGE(0x300000, 0x30ffff) AM_RAM AM_BASE(&mcu_ram)	// MCU [G] grid tested, cleared ($a5a-$ad8)
	AM_RANGE(0x330000, 0x330001) AM_WRITE(jchan_mcu_com0_w)	// _[ these 2 are set to 0xFFFF
	AM_RANGE(0x340000, 0x340001) AM_WRITE(jchan_mcu_com1_w)	//  [ to trigger mcu to run cmd ?
	AM_RANGE(0x350000, 0x350001) AM_WRITE(jchan_mcu_com2_w)	// _[ these 2 are set to 0xFFFF
	AM_RANGE(0x360000, 0x360001) AM_WRITE(jchan_mcu_com3_w)	//  [ for mcu to return its status ?
	AM_RANGE(0x370000, 0x370001) AM_READ(jchan_mcu_status_r)

	AM_RANGE(0x400000, 0x400001) AM_READ(sub2main_cmd_r) // some subcpu cmd writes @ $400000, IT3 of main reads this location
	AM_RANGE(0x400002, 0x400003) AM_READWRITE(main2sub_status_r, main2sub_status_w)
	AM_RANGE(0x400004, 0x400005) AM_READ(main2sub_result_r)
//  AM_RANGE(0x400006, 0x400007) // ???
	AM_RANGE(0x400100, 0x400123) AM_WRITE(main2sub_unknown)
	AM_RANGE(0x403c02, 0x403c09) AM_WRITE(main2sub_param_w) // probably much more, see subcpu routine $7 @ $14aa
	AM_RANGE(0x403ffe, 0x403fff) AM_WRITE(main2sub_cmd_w)

	/* 1st sprite layer */
//  AM_RANGE(0x500000, 0x5005ff) AM_RAM //     grid tested ($924-$97c), cleared ($982-$9a4) until $503fff
//  AM_RANGE(0x500600, 0x503fff) AM_RAM // [B] grid tested, cleared ($b68-$be6)
	AM_RANGE(0x500000, 0x503fff) AM_RAM AM_WRITE(jchan_suprnova_sprite32_w) AM_BASE(&jchan_spriteram)
	AM_RANGE(0x600000, 0x60003f) AM_RAM AM_WRITE(jchan_suprnova_sprite32regs_w) AM_BASE(&jchan_sprregs)

	/* (0x700000, 0x707fff) = palette zone - but there seems to be 'sub-zones' used differently */
//  AM_RANGE(0x700000, 0x707fff) AM_RAM //     grid tested, cleared ($dbc-$e3a), $2000 bytes (8Kb) copied from $aae40 ($e40-$e56)
//  AM_RANGE(0x708000, 0x70ffff) AM_RAM // [E] grid tested, cleared ($d1c-$d9a), $8000 bytes (32Kb) copied from $a2e40 ($da0-$db6)
	AM_RANGE(0x700000, 0x707fff) AM_RAM // palette for tilemaps?
	AM_RANGE(0x708000, 0x70ffff) AM_RAM AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16) // palette for sprites?
//  AM_RANGE(0x700000, 0x70ffff) AM_RAM AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16) // palette for sprites?

	AM_RANGE(0xf00000, 0xf00003) AM_READWRITE(jchan_ctrl_r, jchan_ctrl_w) AM_BASE(&jchan_ctrl)
	AM_RANGE(0xf00004, 0xf00005) AM_READ(input_port_2_word_r) // DSW2
	AM_RANGE(0xf00006, 0xf00007) AM_RAM // ???

	AM_RANGE(0xf80000, 0xf80001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// watchdog
ADDRESS_MAP_END


static ADDRESS_MAP_START( jchan_sub, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM // Work RAM - grid tested, cleared ($612-$6dc)

	/* (0x400000, 0x403fff) = mainsub_shared_ram */
	AM_RANGE(0x400000, 0x400001) AM_WRITE(sub2main_cmd_w) AM_BASE(&mainsub_shared_ram)
	AM_RANGE(0x400002, 0x403fff) AM_RAM

	/* VIEW2 Tilemap - [D] grid tested, cleared ($1d84), also cleared at startup ($810-$826) */
	AM_RANGE(0x500000, 0x500fff) AM_RAM // AM_READWRITE(MRA16_RAM, kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1) // Layers 0
	AM_RANGE(0x501000, 0x501fff) AM_RAM // AM_READWRITE(MRA16_RAM, kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0) //
	AM_RANGE(0x502000, 0x502fff) AM_RAM // AM_RAM AM_BASE(&kaneko16_vscroll_1)                                  //
	AM_RANGE(0x503000, 0x503fff) AM_RAM // AM_RAM AM_BASE(&kaneko16_vscroll_0)                                  //
	AM_RANGE(0x600000, 0x60001f) AM_RAM // AM_READWRITE(MRA16_RAM, kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)   // Layers 0 Regs

	/* 2nd sprite layer? - [C] grid tested, cleared ($1e2a), also cleared at startup ($7dc-$80a) */
	AM_RANGE(0x700000, 0x703fff) AM_RAM // AM_BASE(&jchan_spriteram) AM_WRITE(jchan_suprnova_sprite32_w)
	AM_RANGE(0x780000, 0x78003f) AM_RAM // AM_BASE(&jchan_sprregs) AM_WRITE(jchan_suprnova_sprite32regs_w)

	AM_RANGE(0x800000, 0x800001) AM_WRITE(YMZ280B_register_0_lsb_w) // sound
	AM_RANGE(0x800002, 0x800003) AM_WRITE(YMZ280B_data_0_lsb_w) 	//

	AM_RANGE(0xa00000, 0xa00001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// watchdog
ADDRESS_MAP_END

/* video registers ?!? according values, can be seen as 2 functionnal blocks: 780000-780014 & 780018-780034
cpu #1 (PC=00000702): unmapped program memory word write to 00780000 = 0040 & FFFF
cpu #1 (PC=00000708): unmapped program memory word write to 00780004 = 0000 & FFFF
cpu #1 (PC=00000716): unmapped program memory word write to 00780008 = 0000 & FFFF
cpu #1 (PC=0000071C): unmapped program memory word write to 0078000C = 0200 & FFFF
cpu #1 (PC=00000722): unmapped program memory word write to 00780010 = 0000 & FFFF
cpu #1 (PC=00000728): unmapped program memory word write to 00780014 = 0000 & FFFF

cpu #1 (PC=0000072E): unmapped program memory word write to 00780018 = 0000 & FFFF
cpu #1 (PC=00000734): unmapped program memory word write to 0078001C = FC00 & FFFF
cpu #1 (PC=0000073A): unmapped program memory word write to 00780020 = 0000 & FFFF
cpu #1 (PC=00000740): unmapped program memory word write to 00780024 = FC00 & FFFF
cpu #1 (PC=00000746): unmapped program memory word write to 00780028 = 0000 & FFFF
cpu #1 (PC=0000074C): unmapped program memory word write to 0078002C = FC00 & FFFF
cpu #1 (PC=00000752): unmapped program memory word write to 00780030 = 0000 & FFFF
cpu #1 (PC=00000758): unmapped program memory word write to 00780034 = FC00 & FFFF

--- the previous ones are suprnova sprite registers, similar to 600000 of main cpu!!!
--- the next ones are probable tilemap registers...

cpu #1 (PC=00000760): unmapped program memory word write to 0060000E = 0000 & FFFF
cpu #1 (PC=00000768): unmapped program memory word write to 0060000A = 0002 & FFFF
cpu #1 (PC=00000770): unmapped program memory word write to 00600008 = 0C00 & FF00   (2)
cpu #1 (PC=00000780): unmapped program memory word write to 00600008 = 000C & 00FF (1)
cpu #1 (PC=00000790): unmapped program memory word write to 00600010 = 0000 & FFFF
cpu #1 (PC=00000796): unmapped program memory word write to 00600012 = 0000 & FFFF
cpu #1 (PC=0000079E): unmapped program memory word write to 00600000 = 7740 & FFFF (1)
cpu #1 (PC=000007A6): unmapped program memory word write to 00600002 = 0140 & FFFF (1)
cpu #1 (PC=000007AE): unmapped program memory word write to 00600004 = 77C0 & FFFF   (2)
cpu #1 (PC=000007B6): unmapped program memory word write to 00600006 = 0140 & FFFF   (2)

(1) seems to be grouped - see subcpu routine $09 @ $16e2
(2) seems to be grouped - see subcpu routine $0A @ $172c
*/


/* gfx decode , this one seems ok */
static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 8*32+4, 8*32+0, 8*32+12, 8*32+8, 8*32+20, 8*32+16, 8*32+28, 8*32+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 16*32,17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

// we don't decode the sprites, they are non-tile based and RLE encoded!, see suprnova.c */

static const gfx_decode gfxdecodeinfo[] =
{
//  { REGION_GFX1, 0, &char2layout,   0, 512  },
//  { REGION_GFX2, 0, &char2layout,   0, 512  },
	{ REGION_GFX3, 0, &tilelayout,   16384, 16384  },
	{ -1 } /* end of array */
};


/* input ports */

INPUT_PORTS_START( jchan )

/* TO BE VERIFIED: Player 1 & 2 - see subroutine $21e2a of main68k IT1 */
/* TO BE VERIFIED: dips assignements according infos by BrianT at http://www.crazykong.com - seems ok */

	PORT_START_TAG("IN0")	// Player Controls - $f00000.w (-> $2000b1.b)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")	// Player Controls - $f00002.w (-> $2000b5.b)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")	// Coins - f00004.b
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_SERVICE_NO_TOGGLE( 0x1000, IP_ACTIVE_LOW	)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT		)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1	)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN	)

	PORT_START_TAG("IN3")	// DSW provided by the MCU - $200098.b <- $300200
	PORT_DIPNAME( 0x0100, 0x0100, "Test Mode" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Stereo ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Mono ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Stereo ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Blood Mode" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "Bloody" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )	/* impacts $200008.l many times */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )	/* impacts $200116.l once! -> impacts reading of controls @ $21e2a */
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/* sound stuff */

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0	// irq ?
};


/* machine driver */

static MACHINE_DRIVER_START( jchan )

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(jchan_main,0)
	MDRV_CPU_VBLANK_INT(jchan_vblank, 2)

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(jchan_sub,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(jchan)
	MDRV_VIDEO_UPDATE(jchan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 28636400 / 2)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/* rom loading */

ROM_START( jchan )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "jm01x3.u67", 0x000001, 0x080000, CRC(c0adb141) SHA1(de265e1da06e723492e0c2465cd04e25ce1c237f) )
	ROM_LOAD16_BYTE( "jm00x3.u68", 0x000000, 0x080000, CRC(b1aadc5a) SHA1(0a93693088c0a4b8a79159fb0ebac47d5556d800) )
	ROM_LOAD16_BYTE( "jm11x3.u69", 0x100001, 0x080000, CRC(d2e3f913) SHA1(db2d790fba5351660a9525f545ab1b23dfe319b0) )
	ROM_LOAD16_BYTE( "jm10x3.u70", 0x100000, 0x080000, CRC(ee08fee1) SHA1(5514bd8c625bc7cf8dd5da2f76b760716609b925) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "jsp1x3.u86", 0x000001, 0x080000, CRC(d15d2b8e) SHA1(e253f2d64fee6627f68833b441f41ea6bbb3ab07) ) // 1xxxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "jsp0x3.u87", 0x000000, 0x080000, CRC(ebec50b1) SHA1(57d7bd728349c2b9d662bcf20a3be92902cb3ffb) ) // 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* SPA GFX */
	ROM_LOAD( "jc-100-00.179", 0x0000000, 0x0400000, CRC(578d928c) SHA1(1cfe04f9b02c04f95a85d6fe7c4306a535ff969f) ) // SPA0 kaneko logo
	ROM_LOAD( "jc-101-00.180", 0x0400000, 0x0400000, CRC(7f5e1aca) SHA1(66ed3deedfd55d88e7dcd017b9c2ce523ccb421a) ) // SPA1
	ROM_LOAD( "jc-102-00.181", 0x0800000, 0x0400000, CRC(72caaa68) SHA1(f6b98aa949768a306ac9bc5f9c05a1c1a3fb6c3f) ) // SPA2
	ROM_LOAD( "jc-103-00.182", 0x0c00000, 0x0400000, CRC(4e9e9fc9) SHA1(bf799cdee930b7f71aea4d55c3dd6a760f7478bb) ) // SPA3 title logo? + char select
	ROM_LOAD( "jc-104-00.183", 0x1000000, 0x0200000, CRC(6b2a2e93) SHA1(e34010e39043b67493bcb23a04828ab7cda8ba4d) ) // SPA4
	ROM_LOAD( "jc-105-00.184", 0x1200000, 0x0200000, CRC(73cad1f0) SHA1(5dbe4e318948e4f74bfc2d0d59455d43ba030c0d) ) // SPA5 11xxxxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD( "jc-108-00.185", 0x1400000, 0x0200000, CRC(67dd1131) SHA1(96f334378ae0267bdb3dc528635d8d03564bd859) ) // SPA6 text
	ROM_LOAD16_BYTE( "jcs0x3.164", 0x1600000, 0x040000, CRC(9a012cbc) SHA1(b3e7390220c90d55dccfb96397f0af73925e36f9) ) // SPA-7A female portraits
	ROM_LOAD16_BYTE( "jcs1x3.165", 0x1600001, 0x040000, CRC(57ae7c8d) SHA1(4086f638c2aabcee84e838243f0fd15cec5c040d) ) // SPA-7B female portraits

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* SPB GFX (we haven't used yet, not sure where they map, 2nd sprite layer maybe?) */
	ROM_LOAD( "jc-106-00.171", 0x000000, 0x200000, CRC(bc65661b) SHA1(da28b8fcd7c7a0de427a54be2cf41a1d6a295164) ) // SPB0
	ROM_LOAD( "jc-107-00.172", 0x200000, 0x200000, CRC(92a86e8b) SHA1(c37eddbc9d84239deb543504e27b5bdaf2528f79) ) // SPB1

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG GFX */
	ROM_LOAD( "jc-200.00", 0x000000, 0x100000, CRC(1f30c24e) SHA1(0c413fc67c3ec020e6786e7157d82aa242c8d2ad) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Audio 1? */
	ROM_LOAD( "jc-300-00.84", 0x000000, 0x200000, CRC(13d5b1eb) SHA1(b047594d0f1a71d89b8f072879ccba480f54a483) )
	ROM_LOAD( "jc-301-00.85", 0x200000, 0x100000, CRC(9c5b3077) SHA1(db9a31e1c65d9f12d0f2fb316ced48a02aae089d) )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 ) /* Audio 2? */
	ROM_LOAD( "jcw0x0.u56", 0x000000, 0x040000, CRC(bcf25c2a) SHA1(b57a563ab5c05b05d133eed3d099c4de997f37e4) )

	ROM_REGION( 0x020000, REGION_USER1, 0 ) /* MCU Code? */
	ROM_LOAD( "jcd0x1.u13", 0x000000, 0x020000, CRC(2a41da9c) SHA1(7b1ba0efc0544e276196b9605df1881fde871708) )
ROM_END

/* the program code of gurus set is the same, the graphic roms were split up, and might be bad but they should be the same since
the code was, decoding the gfx on the second set they appear to be partly bad. */

/* some kind of semi-sequel? mask roms not dumped but might be the same */
ROM_START( jchan2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "j2p1x1.u67", 0x000001, 0x080000, CRC(5448c4bc) SHA1(447835275d5454f86a51879490a6b22b06a23e81) )
	ROM_LOAD16_BYTE( "j2p1x2.u68", 0x000000, 0x080000, CRC(52104ab9) SHA1(d6647e628662bdb832270540ece18b265b7ce62d) )
	ROM_LOAD16_BYTE( "j2p1x3.u69", 0x100001, 0x080000, CRC(8763ebca) SHA1(daf6af42a34802ef9aa996e340e218779bad695f) )
	ROM_LOAD16_BYTE( "j2p1x4.u70", 0x100000, 0x080000, CRC(0f8e5e69) SHA1(1f71042458f76b7d99382db6412fb6c362cd3ded) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "j2p1x5.u86", 0x000001, 0x080000, CRC(dc897725) SHA1(d3e94bac96497deb2f79996c2d4a349f6da5b1d6) ) // 1xxxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "j2p1x6.u87", 0x000000, 0x080000, CRC(594224f9) SHA1(bc546a98c5f3c5b08f521c54a4b0e9e2cdf83ced) ) // 1xxxxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* SPA GFX */
	#if 1 // NOT verified as being the same yet
	ROM_LOAD( "jc-100-00.179", 0x0000000, 0x0400000, CRC(578d928c) SHA1(1cfe04f9b02c04f95a85d6fe7c4306a535ff969f) ) // SPA0 kaneko logo
	ROM_LOAD( "jc-101-00.180", 0x0400000, 0x0400000, CRC(7f5e1aca) SHA1(66ed3deedfd55d88e7dcd017b9c2ce523ccb421a) ) // SPA1
	ROM_LOAD( "jc-102-00.181", 0x0800000, 0x0400000, CRC(72caaa68) SHA1(f6b98aa949768a306ac9bc5f9c05a1c1a3fb6c3f) ) // SPA2
	ROM_LOAD( "jc-103-00.182", 0x0c00000, 0x0400000, CRC(4e9e9fc9) SHA1(bf799cdee930b7f71aea4d55c3dd6a760f7478bb) ) // SPA3 title logo? + char select
	ROM_LOAD( "jc-104-00.183", 0x1000000, 0x0200000, CRC(6b2a2e93) SHA1(e34010e39043b67493bcb23a04828ab7cda8ba4d) ) // SPA4
	ROM_LOAD( "jc-105-00.184", 0x1200000, 0x0200000, CRC(73cad1f0) SHA1(5dbe4e318948e4f74bfc2d0d59455d43ba030c0d) ) // SPA5 11xxxxxxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD( "jc-108-00.185", 0x1400000, 0x0200000, CRC(67dd1131) SHA1(96f334378ae0267bdb3dc528635d8d03564bd859) ) // SPA6 text
	#endif
	ROM_LOAD16_BYTE( "j2g1x1.164", 0x1600000, 0x080000, CRC(66a7ea6a) SHA1(605cbc1eb50fb0decbea790f2a11e999d5fde762) ) // SPA-7A female portraits
	ROM_LOAD16_BYTE( "j2g1x2.165", 0x1600001, 0x080000, CRC(660e770c) SHA1(1e385a6ee83559b269d2179e6c247238c0f3c850) ) // SPA-7B female portraits

	#if 1 // NOT verified as being the same yet
	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* SPB GFX (we haven't used yet, not sure where they map, 2nd sprite layer maybe?) */
	ROM_LOAD( "jc-106-00.171", 0x000000, 0x200000, CRC(bc65661b) SHA1(da28b8fcd7c7a0de427a54be2cf41a1d6a295164) ) // SPB0
	ROM_LOAD( "jc-107-00.172", 0x200000, 0x200000, CRC(92a86e8b) SHA1(c37eddbc9d84239deb543504e27b5bdaf2528f79) ) // SPB1

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG GFX */
	ROM_LOAD( "jc-200.00", 0x000000, 0x100000, CRC(1f30c24e) SHA1(0c413fc67c3ec020e6786e7157d82aa242c8d2ad) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Audio 1? */
	ROM_LOAD( "jc-300-00.84", 0x000000, 0x200000, CRC(13d5b1eb) SHA1(b047594d0f1a71d89b8f072879ccba480f54a483) )
	ROM_LOAD( "jc-301-00.85", 0x200000, 0x100000, CRC(9c5b3077) SHA1(db9a31e1c65d9f12d0f2fb316ced48a02aae089d) )
	#endif

	ROM_REGION( 0x040000, REGION_SOUND2, 0 ) /* Audio 2? */
	ROM_LOAD( "j2m1x1.u56", 0x000000, 0x040000, CRC(baf6e25e) SHA1(6b02f3eb1eafcd43022a9f60f98573d02277adfe) )

	ROM_REGION( 0x020000, REGION_USER1, 0 ) /* MCU Code? */
	ROM_LOAD( "j2d1x1.u13", 0x000000, 0x020000, CRC(b2b7fc90) SHA1(1b90c13bb41a313c4ed791a15d56073a7c29928b) )
ROM_END

DRIVER_INIT( jchan )
{
	memset(jchan_mcu_com, 0, 4 * sizeof( UINT16) );
}


/* game drivers */
GAME( 1995, jchan,      0, jchan, jchan, jchan, ROT0, "Kaneko", "Jackie Chan - The Kung-Fu Master", GAME_NOT_WORKING )
GAME( 1995, jchan2, jchan, jchan, jchan, jchan, ROT0, "Kaneko", "Jackie Chan in Fists of Fire", GAME_NOT_WORKING )

