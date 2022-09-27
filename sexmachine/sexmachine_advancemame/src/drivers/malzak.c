/*

  Malzak

  Produced by Kitronix, cabinet artwork suggests it was
  released in 1980.

  Was sold around New Zealand in the early '80s.
  Sold about 500 sets, fairly decent for the time.

  There are more games on this hardware, as far as I am aware.

  Driver by Reip, Barry Rodewald
  SAA 5050 display code "borrowed" from the Philips 2000T/2000M driver in MESS

  Basic memory map

    0x0000 - 0x0fff | S2650 code [malzak.5, malzak.4, malzak.2 (data only)]
    0x1000 - 0x13ff | Work RAM
    0x1400 - 0x14ff | S2636 #1 video
    0x1500 - 0x15ff | S2636 #2 video
    0x1600 - 0x16ff | Playfield gfx
    0x1700 - 0x17ff | Work RAM - contains hiscore table, coin count
    0x1800 - 0x1fff | SAA 5050 video RAM

  TODO - I/O ports (0x00 for sprite->background collisions)
         sound (2x SN76477)
         playfield graphics may be banked, tiles above 0x1f are incorrect
         sprite->sprite collisions aren't quite perfect
           (you can often fly through flying missiles)

  Notes:
  - Test mode in Malzak II should be enabled by setting a POT to position
  '4' and pressing button 1 three times.  Then the POT is set to position
  '1' to go back to the game.

*/

#include "driver.h"
#include "vidhrdw/s2636.h"
#include "cpu/s2650/s2650.h"
#include "sound/sn76477.h"

#define SAA5050_VBLANK 2500

static int malzak_bank1;

extern int malzak_x;
extern int malzak_y;

extern unsigned char* saa5050_vidram;  /* Video RAM for SAA 5050 */

// in vidhrdw/malzak.c
VIDEO_START( malzak );
VIDEO_UPDATE( malzak );
WRITE8_HANDLER( playfield_w );

READ8_HANDLER( malzak_s2636_1_r )
{
	return s2636_1_ram[offset];
}

READ8_HANDLER( malzak_s2636_2_r )
{
	return s2636_2_ram[offset];
}

WRITE8_HANDLER( malzak_s2636_1_w )
{
	s2636_w(s2636_1_ram,offset,data,s2636_1_dirty);
}

WRITE8_HANDLER( malzak_s2636_2_w )
{
	s2636_w(s2636_2_ram,offset,data,s2636_2_dirty);
}


static READ8_HANDLER( saa5050_r )
{
	return saa5050_vidram[offset];
}

static WRITE8_HANDLER( saa5050_w )
{
	saa5050_vidram[offset] = data;
}

static READ8_HANDLER( fake_VRLE_r )
{
	return (s2636_1_ram[0xcb] & 0x3f) + (cpu_getvblank()*0x40);
}

static READ8_HANDLER( ram_mirror_r )
{
	return program_read_byte(0x1000+offset);
}

static WRITE8_HANDLER( ram_mirror_w )
{
	program_write_byte(0x1000+offset,data);
}

static READ8_HANDLER( bank_r )
{
	unsigned char* bank = memory_region(REGION_USER2);

	return bank[offset + (malzak_bank1 * 0x0400)];
}

static READ8_HANDLER( s2636_portA_r )
{
	// POT switch position, read from port A of the first S2636
	// Not sure of the correct values to return, but these should
	// do based on the game code.
	switch(readinputport(1))
	{
		case 0:  // Normal play
			return 0xf0;
		case 1:
			return 0x90;
		case 2:
			return 0x70;
		case 3:  // Change settings
			return 0x00;
		default:
			return 0xf0;
	}
}

/*
static READ8_HANDLER( bank2_r )
{  // probably very wrong :)
    unsigned char* bank = memory_region(REGION_USER1);

    return bank[offset + (malzak_bank1 * 0x0400)];
}
*/
static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0c00, 0x0fff) AM_READ(bank_r)
	AM_RANGE(0x1000, 0x10ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1100, 0x11ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1200, 0x12ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1300, 0x13ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x14cb, 0x14cb) AM_READ(fake_VRLE_r)
	AM_RANGE(0x1400, 0x14ff) AM_READ(malzak_s2636_1_r)
	AM_RANGE(0x1500, 0x15ff) AM_READ(malzak_s2636_2_r)
	AM_RANGE(0x1600, 0x16ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1700, 0x17ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1800, 0x1fff) AM_READ(saa5050_r)  // SAA 5050 video RAM
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_READ(ram_mirror_r)
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_READ(ram_mirror_r)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(ram_mirror_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1000, 0x10ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1100, 0x11ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1200, 0x12ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1300, 0x13ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1400, 0x14ff) AM_WRITE(malzak_s2636_1_w)
	AM_RANGE(0x1500, 0x15ff) AM_WRITE(malzak_s2636_2_w)
	AM_RANGE(0x1600, 0x16ff) AM_WRITE(playfield_w)
	AM_RANGE(0x1600, 0x16ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1700, 0x17ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1fff) AM_WRITE(saa5050_w)  // SAA 5050 video RAM
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(ram_mirror_w)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(ram_mirror_w)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(ram_mirror_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( malzak2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0bff) AM_READ(MRA8_ROM)
	AM_RANGE(0x0c00, 0x0fff) AM_READ(bank_r)
	AM_RANGE(0x1000, 0x10ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1100, 0x11ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1200, 0x12ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1300, 0x13ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x14cb, 0x14cb) AM_READ(fake_VRLE_r)
	AM_RANGE(0x14cc, 0x14cc) AM_READ(s2636_portA_r)
	AM_RANGE(0x1400, 0x14ff) AM_READ(malzak_s2636_1_r)
	AM_RANGE(0x1500, 0x15ff) AM_READ(malzak_s2636_2_r)
	AM_RANGE(0x1600, 0x16ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1700, 0x17ff) AM_READ(MRA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x1800, 0x1fff) AM_READ(saa5050_r)  // SAA 5050 video RAM
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_READ(ram_mirror_r)
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_READ(ram_mirror_r)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(ram_mirror_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( malzak2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1000, 0x10ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1100, 0x11ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1200, 0x12ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1300, 0x13ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1400, 0x14ff) AM_WRITE(malzak_s2636_1_w)
	AM_RANGE(0x1500, 0x15ff) AM_WRITE(malzak_s2636_2_w)
	AM_RANGE(0x1600, 0x16ff) AM_WRITE(playfield_w)
	AM_RANGE(0x1600, 0x16ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1700, 0x17ff) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x1800, 0x1fff) AM_WRITE(saa5050_w)  // SAA 5050 video RAM
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(ram_mirror_w)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(ram_mirror_w)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(ram_mirror_w)
ADDRESS_MAP_END

static READ8_HANDLER( s2650_data_r )
{
	ui_popup("S2650 data port read");
	return 0xff;
}

static WRITE8_HANDLER( port40_w )
{
//  Bit 0 is constantly set high during gameplay
//  Bit 4 is set high, then low, upon death
//  Bit 1 is set high on boot, and on the title screens.
//  Bits 1-3 are all set high upon death, until the game continues
//  Bit 6 is used only in Malzak II, and is set high after checking
//        the selected version
//  logerror("S2650 [0x%04x]: port 0x40 write: 0x%02x\n",cpunum_get_physical_pc_byte(0),data);
	if(data & 0x40)
		malzak_bank1 = 1;
	else
		malzak_bank1 = 0;
}

static WRITE8_HANDLER( port60_w )
{
	malzak_x = data;
//  logerror("I/O: port 0x60 write 0x%02x\n",data);
}

static WRITE8_HANDLER( portc0_w )
{
	malzak_y = data;
//  logerror("I/O: port 0xc0 write 0x%02x\n",data);
}

static READ8_HANDLER( collision_r )
{
	// High 4 bits seem to refer to the row affected.
	static int counter;

	if(++counter > 15)
		counter = 0;
//  logerror("I/O port 0x00 read\n");
	return 0xd0 + counter;
}

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(collision_r) // returns where a collision can occur.
    AM_RANGE(0x80, 0x80) AM_READ(input_port_0_r)  //controls
	AM_RANGE(S2650_DATA_PORT, S2650_DATA_PORT) AM_READ(s2650_data_r)  // read upon death
    AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x40, 0x40) AM_WRITE(port40_w)  // possibly sound codes for dual SN76477s
	AM_RANGE(0x60, 0x60) AM_WRITE(port60_w)  // possibly playfield scroll X offset
	AM_RANGE(0xa0, 0xa0) AM_WRITE(MWA8_NOP)  // echoes I/O port read from port 0x80
	AM_RANGE(0xc0, 0xc0) AM_WRITE(portc0_w)  // possibly playfield row selection for writing and/or collisions
ADDRESS_MAP_END

INPUT_PORTS_START( malzak )

	/* Malzak has an 8-way stick
       and only one button (firing and bomb dropping on the same button) */

	PORT_START /* I/O port 0x80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    )

    PORT_START
    // No POT switch on Malzak as far as I know

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END

INPUT_PORTS_START( malzak2 )

	/* Same as Malzak, but with additional POT switch, and
       possibly a reset button too. */

	PORT_START /* I/O port 0x80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    )

    PORT_START  // Fake DIP switch to handle the POT switch.
	PORT_DIPNAME( 0x03, 0x00, "POT switch position" )
	PORT_DIPSETTING( 0x00, "1" )  // Normal play
	PORT_DIPSETTING( 0x01, "2" )
	PORT_DIPSETTING( 0x02, "3" )
	PORT_DIPSETTING( 0x03, "4" )  // Change settings

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END


static const gfx_layout charlayout =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
//  { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*16
//  8*8
};

static const gfx_layout s2636_character10 =
{
	8,10,
	5,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
   	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	8*16
};

static const gfx_layout saa5050_charlayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

static const gfx_layout saa5050_hilayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8,
	  2*8, 3*8, 3*8, 4*8, 4*8 },
	8 * 10
};

static const gfx_layout saa5050_lolayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 5*8, 5*8, 6*8, 6*8, 7*8,
	  7*8, 8*8, 8*8, 9*8, 9*8 },
	8 * 10
};

static unsigned char saa5050_palette[8 * 3] =
{
	0x00, 0x00, 0x00,	/* black */
	0xff, 0x00, 0x00,	/* red */
	0x00, 0xff, 0x00,	/* green */
	0xff, 0xff, 0x00,	/* yellow */
	0x00, 0x00, 0xff,	/* blue */
	0xff, 0x00, 0xff,	/* magenta */
	0x00, 0xff, 0xff,	/* cyan */
	0xff, 0xff, 0xff	/* white */
};

/* bgnd, fgnd */
static	unsigned	short	saa5050_colortable[64 * 2] =
{
	0,1, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7
};

//add s2636 decodes here (i.e. from zac2650) and maybe re-arrange them
static const gfx_decode malzak_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 16 },
  	{ REGION_CPU1, 0x0000, &s2636_character10, 0, 8 },	/* s2636 #1  */
	{ REGION_CPU1, 0x0000, &s2636_character10, 0, 8 },	/* s2636 #2  */
	{ REGION_GFX2, 0x0000, &saa5050_charlayout, 0, 128},
	{ REGION_GFX2, 0x0000, &saa5050_hilayout, 0, 128},
	{ REGION_GFX2, 0x0000, &saa5050_lolayout, 0, 128},
	{ -1 } /* end of array */
};

//static int val = -1;


static PALETTE_INIT( malzak )
{
	palette_set_colors(0, saa5050_palette, sizeof(saa5050_palette) / 3);
	memcpy(colortable, saa5050_colortable, sizeof (saa5050_colortable));
}


MACHINE_RESET(malzak)
{
//  val++;
//  printf("val = %X\n",val);
}

static struct SN76477interface sn76477_intf =
{
	0,	/* N/C */		/*  4  noise_res         */
	0,	/* N/C */		/*  5  filter_res        */
	0,	/* N/C */		/*  6  filter_cap        */
	0,	/* N/C */		/*  7  decay_res         */
	0,	/* N/C */		/*  8  attack_decay_cap  */
	RES_K(100),			/* 10  attack_res        */
	RES_K(56),			/* 11  amplitude_res     */
	RES_K(10),			/* 12  feedback_res      */
	0,	/* N/C */		/* 16  vco_voltage       */
	CAP_U(0.1),			/* 17  vco_cap           */
	RES_K(8.2),			/* 18  vco_res           */
	5.0,				/* 19  pitch_voltage     */
	RES_K(120),			/* 20  slf_res           */
	CAP_U(1.0),			/* 21  slf_cap           */
	0,	/* N/C */		/* 23  oneshot_cap       */
	0	/* N/C */		/* 24  oneshot_res       */
};


static MACHINE_DRIVER_START( malzak )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", S2650, 3800000/4)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(SAA5050_VBLANK)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(240, 240)
	MDRV_VISIBLE_AREA(0, 239, 0, 239)
	MDRV_GFXDECODE(malzak_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(malzak)
	MDRV_COLORTABLE_LENGTH(128)

//  MDRV_MACHINE_RESET(malzak)

	MDRV_VIDEO_START(malzak)
	MDRV_VIDEO_UPDATE(malzak)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( malzak2 )
	MDRV_IMPORT_FROM( malzak )

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( malzak2_readmem, malzak2_writemem )

	MDRV_NVRAM_HANDLER( generic_0fill )
MACHINE_DRIVER_END

ROM_START( malzak )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "malzak.5",     0x0000, 0x0800, CRC(75355c98) SHA1(7036ed5d9ee38585b1a6bc204d410d5fb5ddd81f) )
	ROM_CONTINUE( 0x2000, 0x0800 )
	ROM_LOAD( "malzak.4",     0x0800, 0x0400, CRC(744c81e3) SHA1(c08d6df3cf2808a5f99d8247fc19a59be88121a9) )
	ROM_CONTINUE( 0x4000, 0x0c00 )
	// Terrain data
	ROM_LOAD( "malzak.3",     0x4400, 0x0800, CRC(b947229e) SHA1(37b88b5aa91a483fcfe60a9bdd67a66f6378c487) )

	// Screen data
	ROM_REGION(0x0800, REGION_USER2, 0)
	ROM_LOAD( "malzak.2",     0x0000, 0x0800, CRC(2a12ad67) SHA1(f89a50b62311a170004c061abd8dedc3ebd84748) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "malzak.1",     0x0000, 0x0800, CRC(74d5ff7b) SHA1(cae326370dc83b86542f9d070e2dc91b1b833356) )

	ROM_REGION(0x01000, REGION_GFX2,0) // SAA5050 internal character ROM
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e) SHA1(4e1c59dc484505de1dc0b1ba7e5f70a54b0d4ccc) )

ROM_END

ROM_START( malzak2 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "malz1a.bin",   0x000000, 0x000800, CRC(5c3cb14c) SHA1(2d3b5703cb9a47e34aa593f0e8d42d4e67c167d9) )
	ROM_CONTINUE( 0x2000, 0x0800 )

	ROM_LOAD( "malz2b.bin",     0x0800, 0x0400, CRC(2af8aace) SHA1(7aaf03d4848c2cce72b2b3729661e7826834ad44) )
	ROM_CONTINUE( 0x4000, 0x0400 )
	ROM_CONTINUE( 0x2800, 0x0400 )
	ROM_CONTINUE( 0x6000, 0x0400 )
	// Terrain data
	ROM_LOAD( "malz3c.bin",     0x4400, 0x0800, CRC(54d6a02e) SHA1(80c550d74da770689fe451cb0ee8e550a63b1b96) )

	// Screen data
	ROM_REGION(0x0800, REGION_USER2, 0)
	ROM_LOAD( "malz4d.bin",     0x0000, 0x0800, CRC(5c6ca415) SHA1(e7571519ac7911507d2c1cf975a7663f41321cb9) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "malzak.1",     0x0000, 0x0800, CRC(74d5ff7b) SHA1(cae326370dc83b86542f9d070e2dc91b1b833356) )

	ROM_REGION(0x01000, REGION_GFX2,0) // SAA5050 internal character ROM
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e) SHA1(4e1c59dc484505de1dc0b1ba7e5f70a54b0d4ccc) )

ROM_END


GAME( 19??, malzak,   0,       malzak,  malzak,  0,        ROT0, "Kitronix", "Malzak", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAME( 19??, malzak2, malzak,   malzak2, malzak2, 0,        ROT0, "Kitronix", "Malzak II", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
