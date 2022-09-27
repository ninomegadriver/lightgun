/* Hot Blocks */
/*
driver by David Haywood
*/

/*
HotBlock board

Tetris with naughty bits

        ||||||||||||||||
+-------++++++++++++++++-------+
|                              |
|  YM2149 TESTSW               |
|                              |
|    62256 62256   6116 6116   |
|                              |
|    24mhz  TPC1020AFN 24c04a  |
|                              |
|                     PAL      |
| P8088-1 IC4 IC5 62256 62256  |
|                              |
+------------------------------+

330ohm resistor packs for colours


--

there are a variety of test modes which can be obtained
by resetting while holding down player 2 buttons

eeprom / backup data not hooked up ( 24c04a on port4 )

most sources say this is a game by Nics but I believe Nics
to be a company from Korea, this game is quite clearly a
Spanish game, we know for a fact that NIX are from Spain
so it could be by them instead



*/

#include "driver.h"
#include "sound/ay8910.h"

static UINT8 *hotblock_ram;
static UINT8 *hotblock_pal;
static int hotblock_port0;
static int hotblock_port4;

static READ8_HANDLER( hotblock_video_read )
{
	/* right?, anything else?? */
	if(hotblock_port0 &0x20) // port 0 = a8 e8 -- palette
	{
		return hotblock_pal[offset];
	}
	else // port 0 = 88 c8
	{
		return hotblock_ram[offset];
	}
}

/* port 4 is some kind of eeprom / storage .. used to store the scores */
static READ8_HANDLER( hotblock_port4_r )
{

//  printf("port4_r\n");
	return 0x00;//return rand(); // WHAT HERE?
}


static WRITE8_HANDLER( hotblock_port4_w )
{
//  printf("port4_w: pc = %06x : data %04x\n",activecpu_get_pc(),data);
//  ui_popup("port4_w: pc = %06x : data %04x",activecpu_get_pc(),data);
	hotblock_port4=data;
}



WRITE8_HANDLER( hotblock_port0_w )
{
//  ui_popup("port4_w: pc = %06x : data %04x",activecpu_get_pc(),data);
	hotblock_port0=data;
}

WRITE8_HANDLER( hotblock_video_write )
{
	/* right?, anything else?? */
	if(hotblock_port0 &0x20) // port 0 = a8 e8 -- palette
	{
		hotblock_pal[offset]=data;
	}
	else // port 0 = 88 c8
	{
		hotblock_ram[offset]=data;
	}
}

static ADDRESS_MAP_START( hotblock_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x10000, 0x1ffff) AM_READ(hotblock_video_read) AM_WRITE(hotblock_video_write) AM_BASE(&hotblock_ram)
	AM_RANGE(0x20000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotblock_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(hotblock_port0_w)
	AM_RANGE(0x0004, 0x0004) AM_READ(hotblock_port4_r) AM_WRITE(hotblock_port4_w)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_READ( AY8910_read_port_0_r ) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END



VIDEO_START(hotblock)
{
	hotblock_pal=auto_malloc(0x10000);
	return 0;
}

VIDEO_UPDATE(hotblock)
{

	int y,x,count;
	int i;
	static int xxx=320,yyy=204;

	fillbitmap(bitmap, get_black_pen(), 0);

	for (i=0;i<256;i++)
	{
		int dat,r,g,b;
		dat=(hotblock_pal[i*2+1]<<8)|hotblock_pal[i*2];

		b = (dat>>10)&0x1f;
		g = (dat>>5)&0x1f;
		r = (dat>>0)&0x1f;
		palette_set_color(i,r*8,g*8,b*8);
	}

	count=0;
	for (y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x++)
		{
			if(hotblock_port0&0x40)plot_pixel(bitmap, x,y, hotblock_ram[count]);
			count++;
		}
	}


}


INPUT_PORTS_START(hotblock)
	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // unused?

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) // used to get test mode
INPUT_PORTS_END

static INTERRUPT_GEN( hotblocks_irq ) /* right? */
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	input_port_1_r,
};


static MACHINE_DRIVER_START( hotblock )
	/* basic machine hardware */
	MDRV_CPU_ADD(I88, 10000000)
	MDRV_CPU_PROGRAM_MAP(hotblock_map, 0)
	MDRV_CPU_IO_MAP(hotblock_io,0)
	MDRV_CPU_VBLANK_INT(hotblocks_irq,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024,1024)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(hotblock)
	MDRV_VIDEO_UPDATE(hotblock)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( hotblock )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "hotblk5.ic4", 0x000000, 0x080000, CRC(5f90f776) SHA1(5ca74714a7d264b4fafaad07dc11e57308828d30) )
	ROM_LOAD( "hotblk6.ic5", 0x080000, 0x080000, CRC(3176d231) SHA1(ac22fd0e9820c6714f51a3d8315eb5d43ef91eeb) )
ROM_END

GAME( 1993, hotblock, 0,        hotblock, hotblock, 0, ROT0,  "Nics? / Nix?", "Hot Blocks - Tetrix II", 0 )
