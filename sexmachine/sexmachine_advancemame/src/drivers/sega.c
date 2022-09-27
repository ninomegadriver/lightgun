/***************************************************************************

    Sega vector hardware

    Games supported:
        * Space Fury
        * Zektor
        * Tac/Scan
        * Eliminator
        * Star Trek

    Known bugs:
        * none at this time

****************************************************************************

    4/25/99 - Tac-Scan sound call for coins now works. (Jim Hernandez)
    2/5/98 - Added input ports support for Tac Scan. Bonus Ships now work.
             Zektor now uses it's own input port section. (Jim Hernandez)

    Sega Vector memory map (preliminary)

    Most of the info here comes from the wiretap archive at:
    http://www.spies.com/arcade/simulation/gameHardware/

     * Sega G80 Vector Simulation

    ROM Address Map
    ---------------
           Eliminator Elim4Player Space Fury  Zektor  TAC/SCAN  Star Trk
    -----+-----------+-----------+-----------+-------+---------+---------+
    0000 | 969       | 1390      | 969       | 1611  | 1711    | 1873    | CPU u25
    -----+-----------+-----------+-----------+-------+---------+---------+
    0800 | 1333      | 1347      | 960       | 1586  | 1670    | 1848    | ROM u1
    -----+-----------+-----------+-----------+-------+---------+---------+
    1000 | 1334      | 1348      | 961       | 1587  | 1671    | 1849    | ROM u2
    -----+-----------+-----------+-----------+-------+---------+---------+
    1800 | 1335      | 1349      | 962       | 1588  | 1672    | 1850    | ROM u3
    -----+-----------+-----------+-----------+-------+---------+---------+
    2000 | 1336      | 1350      | 963       | 1589  | 1673    | 1851    | ROM u4
    -----+-----------+-----------+-----------+-------+---------+---------+
    2800 | 1337      | 1351      | 964       | 1590  | 1674    | 1852    | ROM u5
    -----+-----------+-----------+-----------+-------+---------+---------+
    3000 | 1338      | 1352      | 965       | 1591  | 1675    | 1853    | ROM u6
    -----+-----------+-----------+-----------+-------+---------+---------+
    3800 | 1339      | 1353      | 966       | 1592  | 1676    | 1854    | ROM u7
    -----+-----------+-----------+-----------+-------+---------+---------+
    4000 | 1340      | 1354      | 967       | 1593  | 1677    | 1855    | ROM u8
    -----+-----------+-----------+-----------+-------+---------+---------+
    4800 | 1341      | 1355      | 968       | 1594  | 1678    | 1856    | ROM u9
    -----+-----------+-----------+-----------+-------+---------+---------+
    5000 | 1342      | 1356      |           | 1595  | 1679    | 1857    | ROM u10
    -----+-----------+-----------+-----------+-------+---------+---------+
    5800 | 1343      | 1357      |           | 1596  | 1680    | 1858    | ROM u11
    -----+-----------+-----------+-----------+-------+---------+---------+
    6000 | 1344      | 1358      |           | 1597  | 1681    | 1859    | ROM u12
    -----+-----------+-----------+-----------+-------+---------+---------+
    6800 | 1345      | 1359      |           | 1598  | 1682    | 1860    | ROM u13
    -----+-----------+-----------+-----------+-------+---------+---------+
    7000 |           | 1360      |           | 1599  | 1683    | 1861    | ROM u14
    -----+-----------+-----------+-----------+-------+---------+---------+
    7800 |                                   | 1600  | 1684    | 1862    | ROM u15
    -----+-----------+-----------+-----------+-------+---------+---------+
    8000 |                                   | 1601  | 1685    | 1863    | ROM u16
    -----+-----------+-----------+-----------+-------+---------+---------+
    8800 |                                   | 1602  | 1686    | 1864    | ROM u17
    -----+-----------+-----------+-----------+-------+---------+---------+
    9000 |                                   | 1603  | 1687    | 1865    | ROM u18
    -----+-----------+-----------+-----------+-------+---------+---------+
    9800 |                                   | 1604  | 1688    | 1866    | ROM u19
    -----+-----------+-----------+-----------+-------+---------+---------+
    A000 |                                   | 1605  | 1709    | 1867    | ROM u20
    -----+-----------+-----------+-----------+-------+---------+---------+
    A800 |                                   | 1606  | 1710    | 1868    | ROM u21
    -----+-----------+-----------+-----------+-------+---------+---------+
    B000 |                                                     | 1869    | ROM u22
    -----+-----------+-----------+-----------+-------+---------+---------+
    B800 |                                                     | 1870    | ROM u23
    -----+-----------+-----------+-----------+-------+---------+---------+

    I/O ports:
    read:

    write:

    These games all have dipswitches, but they are mapped in such a way as to make
    using them with MAME extremely difficult. I might try to implement them in the
    future.

    SWITCH MAPPINGS
    ---------------

    +------+------+------+------+------+------+------+------+
    |SW1-8 |SW1-7 |SW1-6 |SW1-5 |SW1-4 |SW1-3 |SW1-2 |SW1-1 |
    +------+------+------+------+------+------+------+------+
     F8:08 |F9:08 |FA:08 |FB:08 |F8:04 |F9:04  FA:04  FB:04    Zektor &
           |      |      |      |      |      |                Space Fury
           |      |      |      |      |      |
       1  -|------|------|------|------|------|--------------- upright
       0  -|------|------|------|------|------|--------------- cocktail
           |      |      |      |      |      |
           |  1  -|------|------|------|------|--------------- voice
           |  0  -|------|------|------|------|--------------- no voice
                  |      |      |      |      |
                  |  1   |  1  -|------|------|--------------- 5 ships
                  |  0   |  1  -|------|------|--------------- 4 ships
                  |  1   |  0  -|------|------|--------------- 3 ships
                  |  0   |  0  -|------|------|--------------- 2 ships
                                |      |      |
                                |  1   |  1  -|--------------- hardest
                                |  0   |  1  -|--------------- hard
    1 = Open                    |  1   |  0  -|--------------- medium
    0 = Closed                  |  0   |  0  -|--------------- easy

    +------+------+------+------+------+------+------+------+
    |SW2-8 |SW2-7 |SW2-6 |SW2-5 |SW2-4 |SW2-3 |SW2-2 |SW2-1 |
    +------+------+------+------+------+------+------+------+
    |F8:02 |F9:02 |FA:02 |FB:02 |F8:01 |F9:01 |FA:01 |FB:01 |
    |      |      |      |      |      |      |      |      |
    |  1   |  1   |  0   |  0   |  1   | 1    | 0    |  0   | 1 coin/ 1 play
    +------+------+------+------+------+------+------+------+

    Known problems:

    1 The games seem to run too fast. This is most noticable
      with the speech samples in Zektor - they don't match the mouth.
      Slowing down the Z80 doesn't help and in fact hurts performance.

    2 Cocktail mode isn't implemented.

    Is 1) still valid?

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/samples.h"
#include "sndhrdw/segasnd.h"
#include "vidhrdw/vector.h"
#include "includes/segar.h"
#include "sega.h"


/*************************************
 *
 *  Constants
 *
 *************************************/

#define CPU_CLOCK			8000000



/*************************************
 *
 *  Global variables
 *
 *************************************/

extern UINT8 (*sega_decrypt)(offs_t, UINT8);

static UINT8 *mainram;
static UINT8 has_usb;

static UINT8 mult_data[2];
static UINT16 mult_result;

static UINT8 spinner_select;
static UINT8 spinner_sign;
static UINT8 spinner_count;



/*************************************
 *
 *  Machine setup and config
 *
 *************************************/

static void service_switch(void *param, UINT32 oldval, UINT32 newval)
{
	/* pressing the service switch sends an NMI */
	if (newval)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}


static MACHINE_START( g80v )
{
	/* request a callback if the service switch is pressed */
	input_port_set_changed_callback(port_tag_to_index("SERVICESW"), 0x01, service_switch, NULL);

	/* register for save states */
	state_save_register_global_array(mult_data);
	state_save_register_global(mult_result);
	state_save_register_global(spinner_select);
	state_save_register_global(spinner_sign);
	state_save_register_global(spinner_count);

	return 0;
}


static MACHINE_RESET( g80v )
{
	/* if we have a Universal Sound Board, reset it here */
	if (has_usb)
		sega_usb_reset(0x10);
}



/*************************************
 *
 *  RAM writes/decryption
 *
 *************************************/

static offs_t decrypt_offset(offs_t offset)
{
	offs_t pc;

	/* if no active CPU, don't do anything */
	if (cpu_getactivecpu() == -1)
		return offset;

	/* ignore anything but accesses via opcode $32 (LD $(XXYY),A) */
	pc = activecpu_get_previouspc();
	if ((UINT16)pc == 0xffff || program_read_byte(pc) != 0x32)
		return offset;

	/* fetch the low byte of the address and munge it */
	return (offset & 0xff00) | (*sega_decrypt)(pc, program_read_byte(pc + 1));
}

static WRITE8_HANDLER( mainram_w ) { mainram[decrypt_offset(offset)] = data; }
static WRITE8_HANDLER( usb_ram_w ) { sega_usb_ram_w(decrypt_offset(offset), data); }
static WRITE8_HANDLER( vectorram_w ) { vectorram[decrypt_offset(offset)] = data; }



/*************************************
 *
 *  Input port access
 *
 *************************************/

static UINT8 sega_ports_demangle(offs_t offset)
{
	/* The input ports are odd. Neighboring lines are read via a mux chip  */
	/* one bit at a time. This means that one bank of DIP switches will be */
	/* read as two bits from each of 4 ports. For this reason, the input   */
	/* ports have been organized logically, and are demangled at runtime.  */
	/* 8 input ports each provide 4 bits of information. */
	return (((readinputportbytag("D7") >> (offset & 3)) << 7) & 0x80) |
	       (((readinputportbytag("D6") >> (offset & 3)) << 6) & 0x40) |
	       (((readinputportbytag("D5") >> (offset & 3)) << 5) & 0x20) |
	       (((readinputportbytag("D4") >> (offset & 3)) << 4) & 0x10) |
	       (((readinputportbytag("D3") >> (offset & 3)) << 3) & 0x08) |
	       (((readinputportbytag("D2") >> (offset & 3)) << 2) & 0x04) |
	       (((readinputportbytag("D1") >> (offset & 3)) << 1) & 0x02) |
	       (((readinputportbytag("D0") >> (offset & 3)) << 0) & 0x01);
}


static READ8_HANDLER( mangled_ports_r )
{
	return sega_ports_demangle(offset);
}



/*************************************
 *
 *  Spinner control emulation
 *
 *************************************/

static WRITE8_HANDLER( spinner_select_w )
{
	spinner_select = data;
}


static READ8_HANDLER( spinner_input_r )
{
	INT8 delta;

	if (spinner_select & 1)
		return readinputportbytag("IN8");

/*
 * The values returned are always increasing.  That is, regardless of whether
 * you turn the spinner left or right, the self-test should always show the
 * number as increasing. The direction is only reflected in the least
 * significant bit.
 */

	/* I'm sure this can be further simplified ;-) BW */
	delta = readinputportbytag("SPINNER");
	if (delta != 0)
	{
		spinner_sign = (delta >> 7) & 1;
		spinner_count += abs(delta);
	}
	return ~((spinner_count << 1) | spinner_sign);
}



/*************************************
 *
 *  Eliminator 4-player controls
 *
 *************************************/

static UINT32 elim4_joint_coin_r(void *param)
{
	return (readinputportbytag("COINS") & 0xf) != 0xf;
}


static READ8_HANDLER( elim4_input_r )
{
	UINT8 result = 0;

	/* bit 3 enables demux */
	if (spinner_select & 8)
	{
		/* Demux bit 0-2. Only 6 and 7 are connected */
		switch (spinner_select & 7)
		{
			case 6:
				/* player 3 & 4 controls */
				result = readinputportbytag("IN8");
				break;
			case 7:
				/* the 4 coin inputs */
				result = readinputportbytag("COINS");
				break;
		}
	}

	/* LS240 has inverting outputs */
	return (result ^ 0xff);
}



/*************************************
 *
 *  Multiplier
 *
 *************************************/

static WRITE8_HANDLER( multiply_w )
{
	mult_data[offset] = data;
	if (offset == 1)
		mult_result = mult_data[0] * mult_data[1];
}


static READ8_HANDLER( multiply_r )
{
	UINT8 result = mult_result;
	mult_result >>= 8;
	return result;
}



/*************************************
 *
 *  Misc other I/O
 *
 *************************************/

static WRITE8_HANDLER( coin_count_w )
{
	coin_counter_w(0, (data >> 7) & 1);
	coin_counter_w(1, (data >> 6) & 1);
}


static WRITE8_HANDLER( unknown_w )
{
	/* writing an 0x04 here enables interrupts */
	/* some games write 0x00/0x01 here as well */
	if (data != 0x00 && data != 0x01 && data != 0x04)
		printf("%04X:unknown_w = %02X\n", activecpu_get_pc(), data);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc800, 0xcfff) AM_READWRITE(MRA8_RAM, mainram_w) AM_BASE(&mainram)
	AM_RANGE(0xe000, 0xefff) AM_READWRITE(MRA8_RAM, vectorram_w) AM_BASE(&vectorram) AM_SIZE(&vectorram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( main_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xbd, 0xbe) AM_WRITE(multiply_w)
	AM_RANGE(0xbe, 0xbe) AM_READ(multiply_r)
	AM_RANGE(0xbf, 0xbf) AM_WRITE(unknown_w)

	AM_RANGE(0xf9, 0xf9) AM_WRITE(coin_count_w)
	AM_RANGE(0xf8, 0xfb) AM_READ(mangled_ports_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Generic Port definitions
 *
 *************************************/

INPUT_PORTS_START( segaxy_generic )
	PORT_START_TAG("D7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("D6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("D5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("D4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))

	PORT_START_TAG("D2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown )) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))

	PORT_START_TAG("D1")
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR ( Coin_A )) PORT_DIPLOCATION("SW2:4,3,2,1")
	PORT_DIPSETTING(	0x00, DEF_STR ( 4C_1C ))
	PORT_DIPSETTING(	0x01, DEF_STR ( 3C_1C ))
	PORT_DIPSETTING(	0x09, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(	0x0a, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(	0x02, DEF_STR ( 2C_1C ))
	PORT_DIPSETTING(	0x03, DEF_STR ( 1C_1C ))
	PORT_DIPSETTING(	0x0b, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(	0x0c, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(	0x0d, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(	0x04, DEF_STR ( 1C_2C ))
	PORT_DIPSETTING(	0x0f, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(	0x0e, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(	0x05, DEF_STR ( 1C_3C ))
	PORT_DIPSETTING(	0x06, DEF_STR ( 1C_4C ))
	PORT_DIPSETTING(	0x07, DEF_STR ( 1C_5C ))
	PORT_DIPSETTING(	0x08, DEF_STR ( 1C_6C ))

	PORT_START_TAG("D0")
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR ( Coin_B )) PORT_DIPLOCATION("SW2:8,7,6,5")
	PORT_DIPSETTING(	0x00, DEF_STR ( 4C_1C ))
	PORT_DIPSETTING(	0x01, DEF_STR ( 3C_1C ))
	PORT_DIPSETTING(	0x09, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(	0x0a, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(	0x02, DEF_STR ( 2C_1C ))
	PORT_DIPSETTING(	0x03, DEF_STR ( 1C_1C ))
	PORT_DIPSETTING(	0x0b, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(	0x0c, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(	0x0d, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(	0x04, DEF_STR ( 1C_2C ))
	PORT_DIPSETTING(	0x0f, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(	0x0e, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(	0x05, DEF_STR ( 1C_3C ))
	PORT_DIPSETTING(	0x06, DEF_STR ( 1C_4C ))
	PORT_DIPSETTING(	0x07, DEF_STR ( 1C_5C ))
	PORT_DIPSETTING(	0x08, DEF_STR ( 1C_6C ))

	PORT_START_TAG("SERVICESW")
	PORT_SERVICE_NO_TOGGLE( 0x01, IP_ACTIVE_HIGH )
INPUT_PORTS_END



/*************************************
 *
 *  Specific Port definitions
 *
 *************************************/

INPUT_PORTS_START( elim2 )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D6")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

	PORT_MODIFY("D5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_MODIFY("D4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives )) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, DEF_STR( Very_Hard ))
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x0c, DEF_STR( None ))
	PORT_DIPSETTING(	0x08, "10000" )
	PORT_DIPSETTING(	0x04, "20000" )
	PORT_DIPSETTING(	0x00, "30000" )

	PORT_START_TAG("IN8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( elim2c )
	PORT_INCLUDE( elim2 )

	PORT_MODIFY("D6")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("D5")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

	PORT_MODIFY("D4")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_MODIFY("IN8")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( elim4 )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM(elim4_joint_coin_r, 0)	/* combination of all four coin inputs */

	PORT_MODIFY("D6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_MODIFY("D5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)

	PORT_MODIFY("D4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives )) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, DEF_STR( Very_Hard ))
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x0c, DEF_STR( None ))
	PORT_DIPSETTING(	0x08, "10000" )
	PORT_DIPSETTING(	0x04, "20000" )
	PORT_DIPSETTING(	0x00, "30000" )

	PORT_MODIFY("D1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown )) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown )) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown )) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown )) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))

	PORT_MODIFY("D0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown )) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x01, DEF_STR( On ))
	PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coin_A )) PORT_DIPLOCATION("SW2:8,7,6")
	PORT_DIPSETTING(	0x0e, DEF_STR( 8C_1C ))
	PORT_DIPSETTING(	0x0c, DEF_STR( 7C_1C ))
	PORT_DIPSETTING(	0x0a, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(	0x08, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(	0x06, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0x04, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ))

	PORT_START_TAG("IN8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)

	PORT_START_TAG("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_IMPULSE(1)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( spacfury )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_MODIFY("D4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds )) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives )) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, DEF_STR( Very_Hard ))
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x00, "10000" )
	PORT_DIPSETTING(	0x04, "20000" )
	PORT_DIPSETTING(	0x08, "30000" )
	PORT_DIPSETTING(	0x0c, "40000" )

	PORT_START_TAG("IN8")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( zektor )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds )) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives )) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, DEF_STR( Very_Hard ))
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x00, DEF_STR( None ))
	PORT_DIPSETTING(	0x0c, "10000" )
	PORT_DIPSETTING(	0x08, "20000" )
	PORT_DIPSETTING(	0x04, "30000" )

	PORT_START_TAG("IN8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("SPINNER")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(3) PORT_RESET
INPUT_PORTS_END


INPUT_PORTS_START( tacscan )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds )) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0c, 0x0c, "Number of Ships" ) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "4" )
	PORT_DIPSETTING(	0x08, "6" )
	PORT_DIPSETTING(	0x0c, "8" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, DEF_STR( Very_Hard ))
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x00, DEF_STR( None ))
	PORT_DIPSETTING(	0x0c, "10000" )
	PORT_DIPSETTING(	0x08, "20000" )
	PORT_DIPSETTING(	0x04, "30000" )

	PORT_START_TAG("IN8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("SPINNER")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET
INPUT_PORTS_END


INPUT_PORTS_START( startrek )
	PORT_INCLUDE( segaxy_generic )

	PORT_MODIFY("D3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet )) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ))
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds )) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ))
	PORT_DIPSETTING(	0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0c, 0x0c, "Photon Torpedoes" ) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x04, "2" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x0c, "4" )

	PORT_MODIFY("D2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty )) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(	0x00, DEF_STR( Easy ))
	PORT_DIPSETTING(	0x01, DEF_STR( Normal ))
	PORT_DIPSETTING(	0x02, DEF_STR( Hard ))
	PORT_DIPSETTING(	0x03, "Tournament" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life )) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(	0x00, "10000" )
	PORT_DIPSETTING(	0x04, "20000" )
	PORT_DIPSETTING(	0x08, "30000" )
	PORT_DIPSETTING(	0x0c, "40000" )

	PORT_START_TAG("IN8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("SPINNER")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET
INPUT_PORTS_END



/*************************************
 *
 *  Eliminator sound interfaces
 *
 *************************************/

static const char *elim_sample_names[] =
{
	"*elim2",
	"elim1.wav",
	"elim2.wav",
	"elim3.wav",
	"elim4.wav",
	"elim5.wav",
	"elim6.wav",
	"elim7.wav",
	"elim8.wav",
	"elim9.wav",
	"elim10.wav",
	"elim11.wav",
	"elim12.wav",
	0	/* end of array */
};

static struct Samplesinterface elim2_samples_interface =
{
	8,	/* 8 channels */
	elim_sample_names
};



/*************************************
 *
 *  Space Fury sound interfaces
 *
 *************************************/

static const char *spacfury_sample_names[] =
{
	"*spacfury",
	/* Sound samples */
	"sfury1.wav",
	"sfury2.wav",
	"sfury3.wav",
	"sfury4.wav",
	"sfury5.wav",
	"sfury6.wav",
	"sfury7.wav",
	"sfury8.wav",
	"sfury9.wav",
	"sfury10.wav",
	0	/* end of array */
};


static struct Samplesinterface spacfury_samples_interface =
{
	8,	/* 8 channels */
	spacfury_sample_names
};


/*************************************
 *
 *  Zektor sound interfaces
 *
 *************************************/

static const char *zektor_sample_names[] =
{
	"*zektor",
	"elim1.wav",  /*  0 fireball */
	"elim2.wav",  /*  1 bounce */
	"elim3.wav",  /*  2 Skitter */
	"elim4.wav",  /*  3 Eliminator */
	"elim5.wav",  /*  4 Electron */
	"elim6.wav",  /*  5 fire */
	"elim7.wav",  /*  6 thrust */
	"elim8.wav",  /*  7 Electron */
	"elim9.wav",  /*  8 small explosion */
	"elim10.wav", /*  9 med explosion */
	"elim11.wav", /* 10 big explosion */
	0
};


static struct Samplesinterface zektor_samples_interface =
{
	8,
	zektor_sample_names
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( g80v_base )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, CPU_CLOCK/2)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(main_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_START(g80v)
	MDRV_MACHINE_RESET(g80v)
	MDRV_FRAMES_PER_SECOND(40)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(512, 1536, 640-32, 1408+32)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(sega)
	MDRV_VIDEO_UPDATE(sega)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( elim2 )
	MDRV_IMPORT_FROM(g80v_base)

	/* custom sound board */
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(elim2_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacfury )
	MDRV_IMPORT_FROM(g80v_base)

	/* custom sound board */
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(spacfury_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	/* speech board */
	MDRV_IMPORT_FROM(sega_speech_board)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( zektor )
	MDRV_IMPORT_FROM(g80v_base)

	/* custom sound board */
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(zektor_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(AY8910, CPU_CLOCK/2/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.33)

	/* speech board */
	MDRV_IMPORT_FROM(sega_speech_board)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tacscan )
	MDRV_IMPORT_FROM(g80v_base)

	/* universal sound board */
	MDRV_IMPORT_FROM(sega_universal_sound_board)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( startrek )
	MDRV_IMPORT_FROM(g80v_base)

	/* speech board */
	MDRV_IMPORT_FROM(sega_speech_board)

	/* universal sound board */
	MDRV_IMPORT_FROM(sega_universal_sound_board)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( elim2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_u25.969",  0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "1333",         0x0800, 0x0800, CRC(fd2a2916) SHA1(431d340c0c9257d66f5851a591861bcefb600cec) )
	ROM_LOAD( "1334",         0x1000, 0x0800, CRC(79eb5548) SHA1(d951de5c0ab94fdb6e58207ee9a147674dd74220) )
	ROM_LOAD( "1335",         0x1800, 0x0800, CRC(3944972e) SHA1(59c84cf23898adb7e434c5802dbb821c79099890) )
	ROM_LOAD( "1336",         0x2000, 0x0800, CRC(852f7b4d) SHA1(6db45b9d11374f4cadf185aec81f33c0040bc001) )
	ROM_LOAD( "1337",         0x2800, 0x0800, CRC(cf932b08) SHA1(f0b61ca8266fd3de7522244c9b1587eecd24a4f1) )
	ROM_LOAD( "1338",         0x3000, 0x0800, CRC(99a3f3c9) SHA1(aa7d4805c70311ebe24ff70fcc32c0e2a7c4488a) )
	ROM_LOAD( "1339",         0x3800, 0x0800, CRC(d35f0fa3) SHA1(752f14b298604a9b91e94cd6d5d291ef33f27ec0) )
	ROM_LOAD( "1340",         0x4000, 0x0800, CRC(8fd4da21) SHA1(f30627dd1fbcc12bb587742a9072bbf38ba48401) )
	ROM_LOAD( "1341",         0x4800, 0x0800, CRC(629c9a28) SHA1(cb7df14ea1bb2d7997bfae1ca70b47763c73298a) )
	ROM_LOAD( "1342",         0x5000, 0x0800, CRC(643df651) SHA1(80c5da44b5d2a7d97c7ba0067f773eb645a9d432) )
	ROM_LOAD( "1343",         0x5800, 0x0800, CRC(d29d70d2) SHA1(ee2cd752b99ebd522eccf5e71d02c31479acfdf5) )
	ROM_LOAD( "1344",         0x6000, 0x0800, CRC(c5e153a3) SHA1(7e805573aeed01e3d4ed477870800dd7ecad7a1b) )
	ROM_LOAD( "1345",         0x6800, 0x0800, CRC(40597a92) SHA1(ee1ae2b424c38b40d2cbeda4aba3328e6d3f9c81) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END

ROM_START( elim2a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_u25.969",  0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "1158",         0x0800, 0x0800, CRC(a40ac3a5) SHA1(9cf707e3439def17390ae16b49552fb1996a6335) )
	ROM_LOAD( "1159",         0x1000, 0x0800, CRC(ff100604) SHA1(1636337c702473b5a567832a622b0c09bd1e2aba) )
	ROM_LOAD( "1160a",        0x1800, 0x0800, CRC(ebfe33bd) SHA1(226da36becd278d34030f564fef61851819d2324) )
	ROM_LOAD( "1161a",        0x2000, 0x0800, CRC(03d41db3) SHA1(da9e618314c01b56b9d66abd14f1e5bf928fff54) )
	ROM_LOAD( "1162a",        0x2800, 0x0800, CRC(f2c7ece3) SHA1(86a9099ce97439cd849dc32ed2c164a1be14e4e7) )
	ROM_LOAD( "1163a",        0x3000, 0x0800, CRC(1fc58b00) SHA1(732c57781cd45cd301b2337b6879ff811d9692f3) )
	ROM_LOAD( "1164a",        0x3800, 0x0800, CRC(f37480d1) SHA1(3d7fac05d60083ddcd51c0190078c89a39f79a91) )
	ROM_LOAD( "1165a",        0x4000, 0x0800, CRC(328819f8) SHA1(ed5e3488399b4481e69f623404a28515524af60a) )
	ROM_LOAD( "1166a",        0x4800, 0x0800, CRC(1b8e8380) SHA1(d56ccc4fac9c8149ebef4939ba401372d69bf022) )
	ROM_LOAD( "1167a",        0x5000, 0x0800, CRC(16aa3156) SHA1(652a547ff1cb4ede507418b392e28f30a3cc179c) )
	ROM_LOAD( "1168a",        0x5800, 0x0800, CRC(3c7c893a) SHA1(73d2835833a6d40f6a9b0a87364af48a449d9674) )
	ROM_LOAD( "1169a",        0x6000, 0x0800, CRC(5cee23b1) SHA1(66f6fc6163148608296e3d25abb194559a2f5179) )
	ROM_LOAD( "1170a",        0x6800, 0x0800, CRC(8cdacd35) SHA1(f24f8a74cb4b8452ddbd42e61d3b0366bbee7f98) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END

ROM_START( elim2c )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "969t.u25.bin", 0x0000, 0x0800, CRC(896a615c) SHA1(542386196eca9fd822e36508e173201ee8a962ed) )
	ROM_LOAD( "1200.u1.bin",  0x0800, 0x0800, CRC(590beb6a) SHA1(307c33cbc0b90f290aac302366e3ce4f70e5265e) )
	ROM_LOAD( "1201.u2.bin",  0x1000, 0x0800, CRC(fed32b30) SHA1(51fba99d3bf543318ebe70ee1aa91e3171767d6f) )
	ROM_LOAD( "1202.u3.bin",  0x1800, 0x0800, CRC(0a2068d0) SHA1(90acf1e78f5c3266d1fbc31470ad4d6a8cb43fe8) )
	ROM_LOAD( "1203.u4.bin",  0x2000, 0x0800, CRC(1f593aa2) SHA1(aaad927174fa806d2c602b5672b1396eb9ec50fa) )
	ROM_LOAD( "1204.u5.bin",  0x2800, 0x0800, CRC(046f1030) SHA1(632ac37b84007f169ce72877d8089538413ba20b) )
	ROM_LOAD( "1205.u6.bin",  0x3000, 0x0800, CRC(8d10b870) SHA1(cc91a06c6b0e1697c399700bc351384360ecd5a3) )
	ROM_LOAD( "1206.u7.bin",  0x3800, 0x0800, CRC(7f6c5afa) SHA1(0e684c654cfe2365e7d21e7bccb25f1ddb883770) )
	ROM_LOAD( "1207.u8.bin",  0x4000, 0x0800, CRC(6cc74d62) SHA1(3392e5cd177885be7391a2699164f39302554d26) )
	ROM_LOAD( "1208.u9.bin",  0x4800, 0x0800, CRC(cc37a631) SHA1(084ecc6b0179fe4f984131d057d5de5382443911) )
	ROM_LOAD( "1209.u10.bin", 0x5000, 0x0800, CRC(844922f8) SHA1(0ad201fce2eaa7dde77d8694d226aad8b9a46ea7) )
	ROM_LOAD( "1210.u11.bin", 0x5800, 0x0800, CRC(7b289783) SHA1(5326ca94b5197ef99db4ea3b28051090f0d7a9ce) )
	ROM_LOAD( "1211.u12.bin", 0x6000, 0x0800, CRC(17349db7) SHA1(8e7ee1fbf153a36a13f3252ca4c588df531b56ec) )
	ROM_LOAD( "1212.u13.bin", 0x6800, 0x0800, CRC(152cf376) SHA1(56c3141598b8bac81e85b1fc7052fdd19cd95609) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END

ROM_START( elim4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1390_cpu.u25", 0x0000, 0x0800, CRC(97010c3e) SHA1(b07db05abf48461b633bbabe359a973a5bc6da13) )
	ROM_LOAD( "1347",         0x0800, 0x0800, CRC(657d7320) SHA1(ef8a637d94dfa8b9dfa600269d914d635e597a9c) )
	ROM_LOAD( "1348",         0x1000, 0x0800, CRC(b15fe578) SHA1(d53773a5f7ec3c130d4ff75a5348a9f37c82c7c8) )
	ROM_LOAD( "1349",         0x1800, 0x0800, CRC(0702b586) SHA1(9847172872419c475d474ff09612c38b867e15af) )
	ROM_LOAD( "1350",         0x2000, 0x0800, CRC(4168dd3b) SHA1(1f26877c63cd7983dfa9a869e0442e8a213f382f) )
	ROM_LOAD( "1351",         0x2800, 0x0800, CRC(c950f24c) SHA1(497a9aa7b9d040a4ff7b3f938093edec2218120d) )
	ROM_LOAD( "1352",         0x3000, 0x0800, CRC(dc8c91cc) SHA1(c99224c7e57dfce9440771f78ce90ea576feed2a) )
	ROM_LOAD( "1353",         0x3800, 0x0800, CRC(11eda631) SHA1(8ba926268762d491d28d5629d5a310b1accca47d) )
	ROM_LOAD( "1354",         0x4000, 0x0800, CRC(b9dd6e7a) SHA1(ab6780f0abe99a5ef76746d45384e80399c6d611) )
	ROM_LOAD( "1355",         0x4800, 0x0800, CRC(c92c7237) SHA1(18aad6618df51a1980775a3aaa4447205453a8af) )
	ROM_LOAD( "1356",         0x5000, 0x0800, CRC(889b98e3) SHA1(23661149e7ffbdbc2c95920d13e9b8b24f86cd9a) )
	ROM_LOAD( "1357",         0x5800, 0x0800, CRC(d79248a5) SHA1(e58062d5c4e5f6fe8d70dd9b55d46a57137c9a64) )
	ROM_LOAD( "1358",         0x6000, 0x0800, CRC(c5dabc77) SHA1(2dc59e627f40fefefc206f2e9d070a62606e44fc) )
	ROM_LOAD( "1359",         0x6800, 0x0800, CRC(24c8e5d8) SHA1(d0ae6e1dfd05d170c49837760369f04df4eaa14f) )
	ROM_LOAD( "1360",         0x7000, 0x0800, CRC(96d48238) SHA1(76a7b49081cd2d0dd1976077aa66b6d5ae5b2b43) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END

ROM_START( elim4p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1390.u25.bin", 0x0000, 0x0800, CRC(97010c3e) SHA1(b07db05abf48461b633bbabe359a973a5bc6da13) )
	ROM_LOAD( "sw1.bin",		 0x0800, 0x0800, CRC(5350b8eb) SHA1(def9192971d1943e45cea1845b1d8c8e2a01bc38) )
	ROM_LOAD( "sw2.bin",		 0x1000, 0x0800, CRC(44f45465) SHA1(e3139878602864509803dabc0f9c278e4b856431) )
	ROM_LOAD( "sw3.bin",		 0x1800, 0x0800, CRC(5b692c3c) SHA1(6cd1361e9f063af1f175baed466cc2667b776a52) )
	ROM_LOAD( "sw4.bin",		 0x2000, 0x0800, CRC(0b78dd00) SHA1(f48c4bdd5fc2e818107b036aa6eddebf46a0e964) )
	ROM_LOAD( "sw5.bin",		 0x2800, 0x0800, CRC(8b3795f1) SHA1(1bcd12791e45dd14c7541e6fe3798a8159b6c11b) )
	ROM_LOAD( "sw6.bin",		 0x3000, 0x0800, CRC(4304b503) SHA1(2bc7a702d43092818ecb713fa0bac476c272e3a0) )
	ROM_LOAD( "sw7.bin",		 0x3800, 0x0800, CRC(3cb4a604) SHA1(868c3c1bead99c2e6857d1c2eef02d84e0e87f29) )
	ROM_LOAD( "sw8.bin",		 0x4000, 0x0800, CRC(bdc55223) SHA1(47ca7485c9e2878cbcb92d93a022f7d74a6d13df) )
	ROM_LOAD( "sw9.bin",		 0x4800, 0x0800, CRC(f6ca1bf1) SHA1(e4dc6bd6486dff2d0e8a93e5c7649093107cde46) )
	ROM_LOAD( "swa.bin",		 0x5000, 0x0800, CRC(12373f7f) SHA1(685c1202345ae8ef53fa61b7254ce04efd94a12b) )
	ROM_LOAD( "swb.bin",		 0x5800, 0x0800, CRC(d1effc6b) SHA1(b72cd14642f26ac50fbe6199d121b0278588ca22) )
	ROM_LOAD( "swc.bin",		 0x6000, 0x0800, CRC(bf361ab3) SHA1(23e3396dc937c0a19d0d312d1de3443b43807d91) )
	ROM_LOAD( "swd.bin",		 0x6800, 0x0800, CRC(ae2c88e5) SHA1(b0833051f543529502e05fb183effa9f817757fb) )
	ROM_LOAD( "swe.bin",		 0x7000, 0x0800, CRC(ec4cc343) SHA1(00e107eaf530ce6bec2afffd7d7bedd7763cfb17) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END


ROM_START( spacfury ) /* Revision C */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "969c.u25", 0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "960c.u1",  0x0800, 0x0800, CRC(d071ab7e) SHA1(c7d2429e4fa77988d7ac62bc68f876ffb7467838) )
	ROM_LOAD( "961c.u2",  0x1000, 0x0800, CRC(aebc7b97) SHA1(d0a0328ed34de9bd2c83da4ddc2d017e2b5a8bdc) )
	ROM_LOAD( "962c.u3",  0x1800, 0x0800, CRC(dbbba35e) SHA1(0400d1ba09199d19f5b8aa5bb1a85ed27930822d) )
	ROM_LOAD( "963c.u4",  0x2000, 0x0800, CRC(d9e9eadc) SHA1(1ad228d65dca48d084bbac358af80882886e7a40) )
	ROM_LOAD( "964c.u5",  0x2800, 0x0800, CRC(7ed947b6) SHA1(c0fd7ed74a87cc422a42e2a4f9eb947f5d5d9fed) )
	ROM_LOAD( "965c.u6",  0x3000, 0x0800, CRC(d2443a22) SHA1(45e5d43eae89e25370bb8e8db2b664642a238eb9) )
	ROM_LOAD( "966c.u7",  0x3800, 0x0800, CRC(1985ccfc) SHA1(8c5931519b976c82a94d17279cc919b4baad5bb7) )
	ROM_LOAD( "967c.u8",  0x4000, 0x0800, CRC(330f0751) SHA1(07ae52fdbfa2cc326f88dc76c3dc8e145b592863) )
	ROM_LOAD( "968c.u9",  0x4800, 0x0800, CRC(8366eadb) SHA1(8e4cb30a730237da2e933370faf5eaa1a41cacbf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808c.u7",  0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD( "970c.u6",  0x0800, 0x1000, CRC(979d8535) SHA1(1ed097e563319ca6d2b7df9875ce7ee921eae468) )
	ROM_LOAD( "971c.u5",  0x1800, 0x1000, CRC(022dbd32) SHA1(4e0504b5ccc28094078912673c49571cf83804ab) )
	ROM_LOAD( "972c.u4",  0x2800, 0x1000, CRC(fad9346d) SHA1(784e5ab0fb00235cfd733c502baf23960923504f) )

	ROM_REGION( 0x0440, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
	ROM_LOAD ("6331.u30", 0x0420, 0x0020, CRC(adcb81d0) SHA1(74b0efc7e8362b0c98e54a6107981cff656d87e1) )	/* speech board addressing */
ROM_END

ROM_START( spacfura ) /* Revision A */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "969a.u25", 0x0000, 0x0800, CRC(896a615c) SHA1(542386196eca9fd822e36508e173201ee8a962ed) )
	ROM_LOAD( "960a.u1",  0x0800, 0x0800, CRC(e1ea7964) SHA1(9c84c525973fcf1437b062d98195272723249d02) )
	ROM_LOAD( "961a.u2",  0x1000, 0x0800, CRC(cdb04233) SHA1(6f8d2fe6d46d04ebe94b7943006d63b24c88ed5a) )
	ROM_LOAD( "962a.u3",  0x1800, 0x0800, CRC(5f03e632) SHA1(c6e8d72ba13ab05ec01a78502948a73c21e0fd69) )
	ROM_LOAD( "963a.u4",  0x2000, 0x0800, CRC(45a77b44) SHA1(91f4822b89ec9c16c67c781a11fabfa4b9914660) )
	ROM_LOAD( "964a.u5",  0x2800, 0x0800, CRC(ba008f8b) SHA1(24f5bef240ae2bcfd5b1d95f51b3599f79518b56) )
	ROM_LOAD( "965a.u6",  0x3000, 0x0800, CRC(78677d31) SHA1(ed5810aa4bddbfe36a6ff9992dd0cb58cce66836) )
	ROM_LOAD( "966a.u7",  0x3800, 0x0800, CRC(a8a51105) SHA1(f5e0fa662552f50fa6905f579d4c678b790ffa96) )
	ROM_LOAD( "967a.u8",  0x4000, 0x0800, CRC(d60f667d) SHA1(821271ec1918e22ed29a5b1f4b0182765ef5ba10) )
	ROM_LOAD( "968a.u9",  0x4800, 0x0800, CRC(aea85b6a) SHA1(8778ff0be34cd4fd5b8f6f76c64bfca68d4d240e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808a.u7",  0x0000, 0x0800, CRC(5988c767) SHA1(3b91a8cd46aa7e714028cc40f700fea32287afb1) )
	ROM_LOAD( "970.u6",   0x0800, 0x1000, CRC(f3b47b36) SHA1(6ae0b627349664140a7f70799645b368e452d69c) )
	ROM_LOAD( "971.u5",   0x1800, 0x1000, CRC(e72bbe88) SHA1(efadf8aa448c289cf4d0cf1831255b9ac60820f2) )
	ROM_LOAD( "972.u4",   0x2800, 0x1000, CRC(8b3da539) SHA1(3a0c4af96a2116fc668a340534582776b2018663) )

	ROM_REGION( 0x0440, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
	ROM_LOAD ("6331.u30", 0x0420, 0x0020, CRC(adcb81d0) SHA1(74b0efc7e8362b0c98e54a6107981cff656d87e1) )	/* speech board addressing */
ROM_END


ROM_START( zektor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1611.cpu", 0x0000, 0x0800, CRC(6245aa23) SHA1(815f3c7edad9c290b719a60964085e90e7268112) )
	ROM_LOAD( "1586.rom", 0x0800, 0x0800, CRC(efeb4fb5) SHA1(b337179c01870c953b8d38c20263802e9a7936d3) )
	ROM_LOAD( "1587.rom", 0x1000, 0x0800, CRC(daa6c25c) SHA1(061e390775b6dd24f85d51951267bca4339a3845) )
	ROM_LOAD( "1588.rom", 0x1800, 0x0800, CRC(62b67dde) SHA1(831bad0f5a601d6859f69c70d0962c970d92db0e) )
	ROM_LOAD( "1589.rom", 0x2000, 0x0800, CRC(c2db0ba4) SHA1(658773f2b56ea805d7d678e300f9bbc896fbf176) )
	ROM_LOAD( "1590.rom", 0x2800, 0x0800, CRC(4d948414) SHA1(f60d295b0f8f798126dbfdc197943d8511238390) )
	ROM_LOAD( "1591.rom", 0x3000, 0x0800, CRC(b0556a6c) SHA1(84b481cc60dc3df3a1cf18b1ece4c70bcc7bb5a1) )
	ROM_LOAD( "1592.rom", 0x3800, 0x0800, CRC(750ecadf) SHA1(83ddd482230fbf6cf78a054fb4abd5bc8aec3ec8) )
	ROM_LOAD( "1593.rom", 0x4000, 0x0800, CRC(34f8850f) SHA1(d93594e529aca8d847c9f1e9055f1840f6069fb2) )
	ROM_LOAD( "1594.rom", 0x4800, 0x0800, CRC(52b22ab2) SHA1(c8f822a1a54081cfc88149c97b4dc19aa745a8d5) )
	ROM_LOAD( "1595.rom", 0x5000, 0x0800, CRC(a704d142) SHA1(95c1249a8efd1a69972ffd7a4da76a0bca5095d9) )
	ROM_LOAD( "1596.rom", 0x5800, 0x0800, CRC(6975e33d) SHA1(3f12037edd6f1b803b5f864789f4b88958ac9578) )
	ROM_LOAD( "1597.rom", 0x6000, 0x0800, CRC(d48ab5c2) SHA1(3f4faf4b131b120b30cd4e73ff34d5cd7ef6c47a) )
	ROM_LOAD( "1598.rom", 0x6800, 0x0800, CRC(ab54a94c) SHA1(9dd57b4b6e46d46922933128d9786df011c6133d) )
	ROM_LOAD( "1599.rom", 0x7000, 0x0800, CRC(c9d4f3a5) SHA1(8516914b49fad85222cbdd9a43609834f5d0f13d) )
	ROM_LOAD( "1600.rom", 0x7800, 0x0800, CRC(893b7dbc) SHA1(136135f0be2e8dddfa0d21a5f4119ee4685c4866) )
	ROM_LOAD( "1601.rom", 0x8000, 0x0800, CRC(867bdf4f) SHA1(5974d32d878206abd113f74ba20fa5276cf21a6f) )
	ROM_LOAD( "1602.rom", 0x8800, 0x0800, CRC(bd447623) SHA1(b8d255aeb32096891379330c5b8adf1d151d70c2) )
	ROM_LOAD( "1603.rom", 0x9000, 0x0800, CRC(9f8f10e8) SHA1(ffe9d872d9011b3233cb06d966852319f9e4cd01) )
	ROM_LOAD( "1604.rom", 0x9800, 0x0800, CRC(ad2f0f6c) SHA1(494a224905b1dac58b3b50f65a8be986b68b06f2) )
	ROM_LOAD( "1605.rom", 0xa000, 0x0800, CRC(e27d7144) SHA1(5b82fda797d86e11882d1f9738a59092c5e3e7d8) )
	ROM_LOAD( "1606.rom", 0xa800, 0x0800, CRC(7965f636) SHA1(5c8720beedab4979a813ce7f0e8961c863973ff7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "1607.spc", 0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD( "1608.spc", 0x0800, 0x1000, CRC(637e2b13) SHA1(8a470f9a8a722f7ced340c4d32b4cf6f05b3e848) )
	ROM_LOAD( "1609.spc", 0x1800, 0x1000, CRC(675ee8e5) SHA1(e314482028b8925ad02e833a1d22224533d0a683) )
	ROM_LOAD( "1610.spc", 0x2800, 0x1000, CRC(2915c7bd) SHA1(3ed98747b5237aa1b3bab6866292370dc2c7655a) )

	ROM_REGION( 0x0440, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
	ROM_LOAD ("6331.u30", 0x0420, 0x0020, CRC(adcb81d0) SHA1(74b0efc7e8362b0c98e54a6107981cff656d87e1) )	/* speech board addressing */
ROM_END


ROM_START( tacscan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1711a",    0x0000, 0x0800, CRC(0da13158) SHA1(256c5441a4841441501c9b7bcf09e0e99e8dd671) )
	ROM_LOAD( "1670c",    0x0800, 0x0800, CRC(98de6fd5) SHA1(f22c215d7558e00366fec5092abb51c670468f8c) )
	ROM_LOAD( "1671a",    0x1000, 0x0800, CRC(dc400074) SHA1(70093ef56e0784173a06da1ac781bb9d8c4e7fc5) )
	ROM_LOAD( "1672a",    0x1800, 0x0800, CRC(2caf6f7e) SHA1(200119260f78bb1c5389707b3ceedfbc1ae43549) )
	ROM_LOAD( "1673a",    0x2000, 0x0800, CRC(1495ce3d) SHA1(3189f8061961d90a52339c855c06e81f4537fb2b) )
	ROM_LOAD( "1674a",    0x2800, 0x0800, CRC(ab7fc5d9) SHA1(b2d9241d83d175ead4da36d7311a41a5f972e06a) )
	ROM_LOAD( "1675a",    0x3000, 0x0800, CRC(cf5e5016) SHA1(78a3f1e4a905515330d4737ac38576ac6e0d8611) )
	ROM_LOAD( "1676a",    0x3800, 0x0800, CRC(b61a3ab3) SHA1(0f4ef5c7fe299ad20fa4637260282a733f1cf461) )
	ROM_LOAD( "1677a",    0x4000, 0x0800, CRC(bc0273b1) SHA1(8e8d8830f17b9fa6d45d98108ca02d90c29de574) )
	ROM_LOAD( "1678b",    0x4800, 0x0800, CRC(7894da98) SHA1(2de7c121ad847e51a10cb1b81aec84cc44a3d04c) )
	ROM_LOAD( "1679a",    0x5000, 0x0800, CRC(db865654) SHA1(db4d5675b53ff2bbaf70090fd064e98862f4ad33) )
	ROM_LOAD( "1680a",    0x5800, 0x0800, CRC(2c2454de) SHA1(74101806439c9faeba88ffe573fa4f93ffa0ba3c) )
	ROM_LOAD( "1681a",    0x6000, 0x0800, CRC(77028885) SHA1(bc981620ebbfbe4e32b3b4d00504475634454c57) )
	ROM_LOAD( "1682a",    0x6800, 0x0800, CRC(babe5cf1) SHA1(26219b7a26f818fee2fe579ec6fb0b16c6bf056f) )
	ROM_LOAD( "1683a",    0x7000, 0x0800, CRC(1b98b618) SHA1(19854cb2741ba37c11ae6d429fa6c17ff930f5e5) )
	ROM_LOAD( "1684a",    0x7800, 0x0800, CRC(cb3ded3b) SHA1(f1e886f4f71b0f6f2c11fb8b4921c3452fc9b2c0) )
	ROM_LOAD( "1685a",    0x8000, 0x0800, CRC(43016a79) SHA1(ee22c1fe0c8df90d9215175104f8a796c3d2aed3) )
	ROM_LOAD( "1686a",    0x8800, 0x0800, CRC(a4397772) SHA1(cadc95b869f5bf5dba7f03dfe5ae64a50899cced) )
	ROM_LOAD( "1687a",    0x9000, 0x0800, CRC(002f3bc4) SHA1(7f3795a05d5651c90cdcd4d00c46d05178b433ea) )
	ROM_LOAD( "1688a",    0x9800, 0x0800, CRC(0326d87a) SHA1(3a5ea4526db417b9e00b24b019c1c6016773c9e7) )
	ROM_LOAD( "1709a",    0xa000, 0x0800, CRC(f35ed1ec) SHA1(dce95a862af0c6b67fb76b99fee0523d53b7551c) )
	ROM_LOAD( "1710a",    0xa800, 0x0800, CRC(6203be22) SHA1(89731c7c88d0125a11368d707f566eb53c783266) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
ROM_END


ROM_START( startrek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu1873",  0x0000, 0x0800, CRC(be46f5d9) SHA1(fadf13042d31b0dacf02a3166545c946f6fd3f33) )
	ROM_LOAD( "1848",     0x0800, 0x0800, CRC(65e3baf3) SHA1(0c081ed6c8be0bb5eb3d5769ac1f0b8fe4735d11) )
	ROM_LOAD( "1849",     0x1000, 0x0800, CRC(8169fd3d) SHA1(439d4b857083ae40df7d7f53c36ec13b05d86a86) )
	ROM_LOAD( "1850",     0x1800, 0x0800, CRC(78fd68dc) SHA1(fb56567458807d9becaacac11091931af9889620) )
	ROM_LOAD( "1851",     0x2000, 0x0800, CRC(3f55ab86) SHA1(f75ce0c56e22e8758dd1f5ce9ac00f5f41b13465) )
	ROM_LOAD( "1852",     0x2800, 0x0800, CRC(2542ecfb) SHA1(7cacee44670768e9fae1024f172b867193d2ea4a) )
	ROM_LOAD( "1853",     0x3000, 0x0800, CRC(75c2526a) SHA1(6e86b30fcdbe7622ab873092e7a7a46d8bad790f) )
	ROM_LOAD( "1854",     0x3800, 0x0800, CRC(096d75d0) SHA1(26e90c296b00239a6cde4ec5e80cccd7bb36bcbd) )
	ROM_LOAD( "1855",     0x4000, 0x0800, CRC(bc7b9a12) SHA1(6dc60e380dc5790cd345b06c064ea7d69570aadb) )
	ROM_LOAD( "1856",     0x4800, 0x0800, CRC(ed9fe2fb) SHA1(5d56e8499cb4f54c5e76a9231c53d95777777e05) )
	ROM_LOAD( "1857",     0x5000, 0x0800, CRC(28699d45) SHA1(c133eb4fc13987e634d3789bfeaf9e03196f8fd3) )
	ROM_LOAD( "1858",     0x5800, 0x0800, CRC(3a7593cb) SHA1(7504f960507579d043b7ee20fb8fd2610399ff4b) )
	ROM_LOAD( "1859",     0x6000, 0x0800, CRC(5b11886b) SHA1(b0fb6e912953822242501943f7214e4af6ab7891) )
	ROM_LOAD( "1860",     0x6800, 0x0800, CRC(62eb96e6) SHA1(51d1f5e48e3e21147584ace61b8832ad892cb6e2) )
	ROM_LOAD( "1861",     0x7000, 0x0800, CRC(99852d1d) SHA1(eaea6a99f0a7f0292db3ea19649b5c1be45b9507) )
	ROM_LOAD( "1862",     0x7800, 0x0800, CRC(76ce27b2) SHA1(8fa8d73aa4dcf3709ecd057bad3278fac605988c) )
	ROM_LOAD( "1863",     0x8000, 0x0800, CRC(dd92d187) SHA1(5a11cdc91bb7b36ea98503892847d8dbcedfe95a) )
	ROM_LOAD( "1864",     0x8800, 0x0800, CRC(e37d3a1e) SHA1(15d949989431dcf1e0406f1e3745f3ee91012ff5) )
	ROM_LOAD( "1865",     0x9000, 0x0800, CRC(b2ec8125) SHA1(70982c614471614f6b490ae2d65faec0eff2ac37) )
	ROM_LOAD( "1866",     0x9800, 0x0800, CRC(6f188354) SHA1(e99946467090b68559c2b54ad2e85204b71a459f) )
	ROM_LOAD( "1867",     0xa000, 0x0800, CRC(b0a3eae8) SHA1(51a0855753dc2d4fe1a05bd54fa958beeab35299) )
	ROM_LOAD( "1868",     0xa800, 0x0800, CRC(8b4e2e07) SHA1(11f7de6327abf88012854417224b38a2352a9dc7) )
	ROM_LOAD( "1869",     0xb000, 0x0800, CRC(e5663070) SHA1(735944c2b924964f72f3bb3d251a35ea2aef3d15) )
	ROM_LOAD( "1870",     0xb800, 0x0800, CRC(4340616d) SHA1(e93686a29377933332523425532d102e30211111) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD ("1670",     0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD ("1871",     0x0800, 0x1000, CRC(03713920) SHA1(25a0158cab9983248e91133f96d1849c9e9bcbd2) )
	ROM_LOAD ("1872",     0x1800, 0x1000, CRC(ebb5c3a9) SHA1(533b6f0499b311f561cf7aba14a7f48ca7c47321) )

	ROM_REGION( 0x0440, REGION_PROMS, 0 )
	ROM_LOAD ("s-c.u39",  0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* sine table */
	ROM_LOAD ("pr-82.u15",0x0400, 0x0020, CRC(c609b79e) SHA1(49dbcbb607079a182d7eb396c0da097166ea91c9) )	/* CPU board addressing */
	ROM_LOAD ("6331.u30", 0x0420, 0x0020, CRC(adcb81d0) SHA1(74b0efc7e8362b0c98e54a6107981cff656d87e1) )	/* speech board addressing */
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

DRIVER_INIT( elim2 )
{
	/* configure security */
	sega_security(70);

	/* configure sound */
	has_usb = FALSE;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3e, 0x3e, 0, 0, elim1_sh_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, elim2_sh_w);

	/* configure inputs */
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, port_tag_to_handler8("IN8"));
}


DRIVER_INIT( elim4 )
{
	/* configure security */
	sega_security(76);

	/* configure sound */
	has_usb = FALSE;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3e, 0x3e, 0, 0, elim1_sh_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, elim2_sh_w);

	/* configure inputs */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, spinner_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, elim4_input_r);
}


DRIVER_INIT( spacfury )
{
	/* configure security */
	sega_security(64);

	/* configure sound */
	has_usb = FALSE;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x38, 0x38, 0, 0, sega_speech_data_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b, 0x3b, 0, 0, sega_speech_control_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3e, 0x3e, 0, 0, spacfury1_sh_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, spacfury2_sh_w);

	/* configure inputs */
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, port_tag_to_handler8("IN8"));
}


DRIVER_INIT( zektor )
{
	/* configure security */
	sega_security(82);

	/* configure sound */
	has_usb = FALSE;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x38, 0x38, 0, 0, sega_speech_data_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b, 0x3b, 0, 0, sega_speech_control_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3c, 0x3c, 0, 0, AY8910_control_port_0_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d, 0x3d, 0, 0, AY8910_write_port_0_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3e, 0x3e, 0, 0, zektor1_sh_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, zektor2_sh_w);

	/* configure inputs */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, spinner_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, spinner_input_r);
}


DRIVER_INIT( tacscan )
{
	/* configure security */
	sega_security(76);

	/* configure sound */
	has_usb = TRUE;
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, sega_usb_status_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, sega_usb_data_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, sega_usb_ram_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, usb_ram_w);

	/* configure inputs */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, spinner_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, spinner_input_r);
}


DRIVER_INIT( startrek )
{
	/* configure security */
	sega_security(64);

	/* configure sound */
	has_usb = TRUE;
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x38, 0x38, 0, 0, sega_speech_data_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b, 0x3b, 0, 0, sega_speech_control_w);

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, sega_usb_status_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3f, 0x3f, 0, 0, sega_usb_data_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, sega_usb_ram_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, usb_ram_w);

	/* configure inputs */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, spinner_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xfc, 0xfc, 0, 0, spinner_input_r);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1981, elim2,	  0,        elim2,    elim2,    elim2,    ORIENTATION_FLIP_Y,   "Gremlin", "Eliminator (2 Players, set 1)", 0 )
GAME( 1981, elim2a,   elim2,    elim2,    elim2,    elim2,    ORIENTATION_FLIP_Y,   "Gremlin", "Eliminator (2 Players, set 2)", 0 )
GAME( 1981, elim2c,	  elim2,	elim2,	  elim2c,	elim2,	  ORIENTATION_FLIP_Y,   "Gremlin", "Eliminator (2 Players, cocktail)", 0 )
GAME( 1981, elim4,	  elim2,    elim2,    elim4,    elim4,    ORIENTATION_FLIP_Y,   "Gremlin", "Eliminator (4 Players)", 0 )
GAME( 1981, elim4p,	  elim2,	elim2,	  elim4,	elim4,	  ORIENTATION_FLIP_Y,   "Gremlin", "Eliminator (4 Players, prototype)", 0 )
GAME( 1981, spacfury, 0,        spacfury, spacfury, spacfury, ORIENTATION_FLIP_Y,   "Sega", "Space Fury (revision C)", 0 )
GAME( 1981, spacfura, spacfury, spacfury, spacfury, spacfury, ORIENTATION_FLIP_Y,   "Sega", "Space Fury (revision A)", 0 )
GAME( 1982, zektor,   0,        zektor,   zektor,   zektor,   ORIENTATION_FLIP_Y,   "Sega", "Zektor (revision B)", 0 )
GAME( 1982, tacscan,  0,        tacscan,  tacscan,  tacscan,  ORIENTATION_FLIP_X ^ ROT270, "Sega", "Tac/Scan", GAME_IMPERFECT_SOUND )
GAME( 1982, startrek, 0,        startrek, startrek, startrek, ORIENTATION_FLIP_Y,   "Sega", "Star Trek", GAME_IMPERFECT_SOUND )
