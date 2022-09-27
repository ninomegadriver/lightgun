/* paint & puzzle */
/* video is standard VGA */
/*
OK, here's a somewhat complete rundown of the PCB. There was 1 IC I didn't
get a pin count of(oops).

Main PCB
Reb B
Label:
Board Num.
90

The PCB has no silkscreen or reference designators, so the numbers I am
providing are made up.

U1 32 pin IC
27C010A
Label:
Paint N Puzzle
Ver. 1.09
Odd

U2 5 pin audio amp
LM383T

U3 40 pin IC
8926S
UM6522A

U4 28 pin IC
Mosel
MS62256L-85PC
8911 5D

U5 18 pin IC
PIC16C54-HS/P
9344 CGA

U6
P8798
L3372718E
Intel
Label:
MicroTouch
5603670 REV 1.0

U7 28 pin IC
MicroTouch Systems
c 1992
19-507 Rev 2
ICS1578N 9334

U8 28 pin IC
Mosel
MS62256L-85PC
8911 5D

U9 32 pin IC
27C010A
Label:
Paint N Puzzle
Ver. 1.09
Even

U10 64 pin IC
MC68000P12
OB26M8829

X1
16.000MHz -connected to U5

X2
ECS-120
32-1
China -connected to U7
Other side is unreadable

CN1 JAMMA
CN2 ISA? Video card slot
CN3 Touchscreen input (12 pins)
CN4 2 pins, unused

1 blue potentiometer connected to audio amp
There doesnt seem to be any dedicated sound chip, and sounds are just bleeps
really.

-----------------------------------------------
Video card (has proper silk screen and designators)
JAX-8327A

X1
40.000MHz

J1 -open
J2 -closed
J3 -open

U1/2 unpopulated but have sockets

U3 20 pin IC
KM44C256BT-8
22BY
Korea

U4 20 pin IC
KM44C256BT-8
22BY
Korea

U5 160 pin QFP
Trident
TVGA9000i
34'3 Japan

U6 28 pin IC
Quadtel
TVGA9000i BIOS Software
c 1993 Ver D3.51 LH

CN1 standard DB15 VGA connector (15KHz)
*/

#include "driver.h"
#include "machine/eeprom.h"


static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	NULL,			/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx" 	/* unlock command */
};

static NVRAM_HANDLER( pntnpuzl )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);
		}
	}
}

static UINT16 pntnpuzl_eeprom;

READ16_HANDLER( pntnpuzl_eeprom_r )
{
	/* bit 11 is EEPROM data */
	return (pntnpuzl_eeprom & 0xf4ff) | (EEPROM_read_bit()<<11) | (readinputport(3) & 0x0300);
}

WRITE16_HANDLER( pntnpuzl_eeprom_w )
{
	pntnpuzl_eeprom = data;

	/* bit 12 is data */
	/* bit 13 is clock (active high) */
	/* bit 14 is cs (active high) */

	EEPROM_write_bit(data & 0x1000);
	EEPROM_set_cs_line((data & 0x4000) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((data & 0x2000) ? ASSERT_LINE : CLEAR_LINE);
}




UINT16* pntnpuzl_3a0000ram;
UINT16* pntnpuzl_bank;
/* vid */
VIDEO_START( pntnpuzl )
{
	pntnpuzl_3a0000ram=auto_malloc(0x100000);

	return 0;
}

VIDEO_UPDATE( pntnpuzl )
{
	int x,y;
	int count;
	static int xxx=0x18f;
	static int yyy=512;
	static int sss=0xa8;
/*
    if ( code_pressed_memory(KEYCODE_Q) )
    {
        xxx--;
        printf("xxx %04x\n",xxx);
    }

    if ( code_pressed_memory(KEYCODE_W) )
    {
        xxx++;
        printf("xxx %04x\n",xxx);
    }

    if ( code_pressed_memory(KEYCODE_A) )
    {
        yyy--;
        printf("yyy %04x\n",yyy);
    }

    if ( code_pressed_memory(KEYCODE_S) )
    {
        yyy++;
        printf("yyy %04x\n",yyy);
    }

    if ( code_pressed_memory(KEYCODE_Z) )
    {
        sss--;
        printf("sss %04x\n",sss);
    }

    if ( code_pressed_memory(KEYCODE_X) )
    {
        sss++;
        printf("sss %04x\n",sss);
    }
*/


	count=sss;

	for(y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x+=2)
		{
			plot_pixel(bitmap, x,y, (pntnpuzl_3a0000ram[count]&0x1f00)>>8);
			plot_pixel(bitmap, x+1,y, (pntnpuzl_3a0000ram[count]&0x001f)>>0);
			count++;
		}
	}

	{
		int sx,sy;

		sx = readinputport(1) * 400 / 128;
		sy = (0x7f - readinputport(2)) * 240 / 128;

		draw_crosshair(bitmap,sx,sy,cliprect,0);
	}
}

WRITE16_HANDLER( pntnpuzl_palette_w )
{
	static int indx,sub,rgb[3];

	if (ACCESSING_MSB)
	{
		indx = data >> 8;
		sub = 0;
	}
	if (ACCESSING_LSB)
	{
		rgb[sub++] = data & 0xff;
		if (sub == 3)
		{
			palette_set_color(indx++,rgb[0]<<2,rgb[1]<<2,rgb[2]<<2);
			sub = 0;
			if (indx == 256) indx = 0;
		}
	}
}



READ16_HANDLER ( pntnpuzl_random_r )
{
	return mame_rand();
}

READ16_HANDLER( pntnpuzl_vid_r )
{
//  logerror("read_videoram: pc = %06x : offset %04x reg %04x\n",activecpu_get_pc(),offset*2, pntnpuzl_bank[0]);
	return pntnpuzl_3a0000ram[offset+ (pntnpuzl_bank[0]&0x0001)*0x8000 ];
}

WRITE16_HANDLER( pntnpuzl_vid_w )
{
//  logerror("write_to_videoram: pc = %06x : offset %04x data %04x reg %04x\n",activecpu_get_pc(),offset*2, data, pntnpuzl_bank[0]);
	COMBINE_DATA(&pntnpuzl_3a0000ram[offset+ (pntnpuzl_bank[0]&0x0001)*0x8000 ]);
}

READ16_HANDLER( pntnpuzl_vblank_r )
{
	return (readinputport(0) & 1) << 11;
}



/*
reading works this way:
280016 = 00
28001A = 08
wait for bit 3 of 28001A to be 1 (after a timeout, fail)
280010 = 3d
280012 = 00
280016 = 04
read 280014 (throw away result)
wait for bit 2 of 28001A to be 1
read data from 280014

during startup it expects this series:
write                                     read
01 52 0d <pause> 01 50 4e 38 31 0d   ---> 80 0c
01 4d 51 0d                          ---> 80 0c
01 46 54 0d                          ---> 80 0c
01 46 4e 30 38 0d                    ---> 80 0c
01 53 45 32 0d                       ---> 80 0c
01 03 46 31 38 0d                    ---> 80 0c
*/
static UINT16 pntpzl_200000, serial, serial_out,read_count;

WRITE16_HANDLER( pntnpuzl_200000_w )
{
// logerror("200000: %04x\n",data);
	// bit 12: set to 1 when going to serial output to 280018
	if ((pntpzl_200000 & 0x1000) && !(data & 0x1000))
	{
		serial_out = (serial>>1) & 0xff;
		read_count = 0;
		logerror("serial out: %02x\n",serial_out);
	}

	pntpzl_200000 = data;
}

WRITE16_HANDLER( pntnpuzl_280018_w )
{
// logerror("%04x: 280018: %04x\n",activecpu_get_pc(),data);
	serial >>= 1;
	if (data & 0x2000)
		serial |= 0x400;
}

READ16_HANDLER( pntnpuzl_280014_r )
{
	static int startup[3] = { 0x80, 0x0c, 0x00 };
	int res;

	if (serial_out == 0x11)
	{
		static int touchscr[5];
		if (readinputport(0) & 0x10)
		{
			touchscr[0] = 0x1b;
			touchscr[2] = BITSWAP8(readinputport(1),0,1,2,3,4,5,6,7);
			touchscr[4] = BITSWAP8(readinputport(2),0,1,2,3,4,5,6,7);
		}
		else
			touchscr[0] = 0;

		if (read_count >= 10) read_count = 0;
		res = touchscr[read_count/2];
		read_count++;
	}
	else
	{
		if (read_count >= 6) read_count = 0;
		res = startup[read_count/2];
		read_count++;
	}
	logerror("read 280014: %02x\n",res);
	return res << 8;
}

READ16_HANDLER( pntnpuzl_28001a_r )
{
	return 0x4c00;
}



static ADDRESS_MAP_START( pntnpuzl_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(MRA16_NOP) //|
	AM_RANGE(0x100000, 0x100001) AM_READ(MRA16_NOP)	//| irq lines clear
	AM_RANGE(0x180000, 0x180001) AM_READ(MRA16_NOP) //|
	AM_RANGE(0x200000, 0x200001) AM_WRITE(pntnpuzl_200000_w)
	AM_RANGE(0x280000, 0x280001) AM_READ(pntnpuzl_eeprom_r)
	AM_RANGE(0x280002, 0x280003) AM_READ(input_port_4_word_r)
	AM_RANGE(0x280000, 0x280001) AM_WRITE(pntnpuzl_eeprom_w)
	AM_RANGE(0x280008, 0x280009) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x28000a, 0x28000b) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280010, 0x280011) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280012, 0x280013) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280014, 0x280015) AM_READ(pntnpuzl_280014_r)
	AM_RANGE(0x280016, 0x280017) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280018, 0x280019) AM_WRITE(pntnpuzl_280018_w)
	AM_RANGE(0x28001a, 0x28001b) AM_READ(pntnpuzl_28001a_r)
	AM_RANGE(0x28001a, 0x28001b) AM_WRITE(MWA16_NOP)

	/* standard VGA */
	AM_RANGE(0x3a0000, 0x3affff) AM_READWRITE(pntnpuzl_vid_r, pntnpuzl_vid_w)
	AM_RANGE(0x3c03c4, 0x3c03c5) AM_RAM AM_BASE(&pntnpuzl_bank)//??
	AM_RANGE(0x3c03c8, 0x3c03c9) AM_WRITE(pntnpuzl_palette_w)
	AM_RANGE(0x3c03da, 0x3c03db) AM_READ(pntnpuzl_vblank_r)

	AM_RANGE(0x400000, 0x407fff) AM_RAM
ADDRESS_MAP_END


INTERRUPT_GEN( pntnpuzl_irq )
{
	if (readinputport(0) & 0x02)	/* coin */
		cpunum_set_input_line(0, 1, PULSE_LINE);
	else if (readinputport(0) & 0x04)	/* service */
		cpunum_set_input_line(0, 2, PULSE_LINE);
	else if (readinputport(0) & 0x08)	/* coin */
		cpunum_set_input_line(0, 4, PULSE_LINE);
}

INPUT_PORTS_START( pntnpuzl )
	PORT_START	/* fake inputs */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_HIGH ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	/* game uses a touch screen */
	PORT_START
	PORT_BIT( 0x7f, 0x40, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0x7f) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)

	PORT_START
	PORT_BIT( 0x7f, 0x40, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0x7f) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_A)

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_D)
INPUT_PORTS_END



static MACHINE_DRIVER_START( pntnpuzl )
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)//??
	MDRV_CPU_PROGRAM_MAP(pntnpuzl_map,0)
	MDRV_CPU_VBLANK_INT(pntnpuzl_irq,1)	// irq1 = coin irq2 = service irq4 = coin

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(pntnpuzl)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 50*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(pntnpuzl)
	MDRV_VIDEO_UPDATE(pntnpuzl)
MACHINE_DRIVER_END

ROM_START( pntnpuzl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "pntnpuzl.u2", 0x00001, 0x40000, CRC(dfda3f73) SHA1(cca8ccdd501a26cba07365b1238d7b434559bbc6) )
	ROM_LOAD16_BYTE( "pntnpuzl.u3", 0x00000, 0x40000, CRC(4173f250) SHA1(516fe6f91b925f71c36b97532608b82e63bda436) )
ROM_END


static DRIVER_INIT(pip)
{
//  UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
//  rom[0x2696/2] = 0x4e71;
//  rom[0x26a0/2] = 0x4e71;
}

GAME( 199?, pntnpuzl,    0, pntnpuzl,    pntnpuzl,    pip, ROT90,  "Century?", "Paint & Puzzle",GAME_NO_SOUND|GAME_NOT_WORKING )
