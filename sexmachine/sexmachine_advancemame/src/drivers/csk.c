/*****************************************************************************

Champion Poker by IGS   (documented by Mirko Buffoni)
Is this a Konami board?
---

Memory Layout (refers to CSK227IT.  Others may have different addresses)

ROM:        0000-efff
RAM:        f000-ffff

f950-f951:  DSW1-2
f952-f953:  DSW3-4
f954-f955:  DSW5-6   (6 not used, but optionally addable to board)

f9c7-f9c8:  Ports 50a0 and 5081 values are written here
f9c9-f9ca:  Ports 5082 and 5091 values are written here

             0  1  2  3  4  5  6  7
f9a7-f9ae:  xx xx xx xx xx xx xx xx     Port 50a0 bit stream
f9af-f9b6:  xx xx xx xx xx xx xx xx     Port 5081 bit stream
f9b7-f9be:  xx xx xx xx xx xx xx xx     Port 5082 bit stream
f9bf-f9c6:  xx xx xx xx xx xx xx xx     Port 5091 bit stream
            |
            xYYYYYYY ====>  Bit 7 set = Nth bit for port is set
                            Bit 6-0   = counts for how may frames Nth bit
                                        has been in that state, recently.

f065:       Double up type
                0 = None
                1 = High/Low
                2 = Red/Black

f68f:       Errorcode
                0 = Coin Error
                1 = Hopper Error
                2 = Limite Max
                3 = System Error
                4 = Punti Error
                5 = Hopper Vuoto
                6 = Limite Record

f69a:        Winning combination for current card set
                 0 = Nothing
                 1 = Coppia
                 2 = DoppiaCoppia
                 3 = Tris
                 4 = Scala
                 5 = Flush
                 6 = FullHouse
                 7 = Poker
                 8 = ScalaMinima
                 9 = Bingo
                10 = ScalaMassima
                11 = FiveJokers

f69b:       Vblank flag.  Reset during NMI.

f6a4:       MaxBet
f6a5:       MinBet to start
f070:       MinBet to play Fever

f9a3:       CoinErrorFlag
            0 = no problem
            1 = coin error
            2 = hopper error

f98a:       Hopper timeout counter.  Decremented during NMI.

f022:       ???
f023:       ???
f024:       ???
f025:       ???
f026:       ???

---

I/O Ports

Palette:    2000-27ff   (low byte)
            2800-2fff   (high byte)
VideoRAM:   7000-77ff
ColorRAM:   7800-7fff
DSW1-5:     4000-4004   (see input ports section below)
InputPorts: 50a0   (unused in this game)
            5081-5082   (Coins and Keyboard)
            5091   (Keyboard)
Expansion:  8000-ffff   (R)     Used to read from an expansion rom

Unknown:    5080        (RW)    (possibly related to ticket/hopper)
            5090-5091   (RW)    (possibly related to eprom counters)
            50b0-50b1   (W)     (possibly sound related)
            5083        (W)     (used only at reset, maybe)
            1000-10ff   (W) ???
            6000-67ff   (W) ???
            6800-6fff   (W)     Expansion video layer (used with ability)

---

Timing:

Game is synchronized with VBLANK. It uses IRQ & NMI interrupts.
During a frame, there must be 3 IRQs and 3 NMIs in order to play
to the correct speed.

---

Notes about palette:
Charset is 6 bit depth (thus 64 colors of granularity)
Colortable is made up of 2 entries of 64 bytes for each palette,
splitted, and colorinfo is stored to form the following word:

xBBBBBGGGGGRRRRR    (Bit 15 is never used)

The game uses 8 palettes, located at the following addresses:

Palette1:   54CD (low), 5509 (high), len = 60   (colorentry: 1-63)
Palette2:   5545 (low), 5581 (high), len = 60   (colorentry: 1-63)
Palette3:   55BD (low), 55F9 (high), len = 60   (colorentry: 1-63)
Palette4:   5635 (low), 5671 (high), len = 60   (colorentry: 1-63)
Palette5:   56AD (low), 56DD (high), len = 48   (colorentry: 1-47)
Palette6:   570D (low), 5749 (high), len = 60   (colorentry: 1-63)
Palette7:   5785 (low), 57C1 (high), len = 60   (colorentry: 1-63)
Palette8:   57FD (low), 582D (high), len = 48   (colorentry: 1-47)

Palette3*:  585D (low), 5899 (high), len = 60   (used alternatively with pal3)

*****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"


extern unsigned char * cpk_colorram;
extern unsigned char * cpk_videoram;
extern unsigned char * cpk_palette;
extern unsigned char * cpk_palette2;
extern unsigned char * cpk_expram;


MACHINE_RESET (cpk);
INTERRUPT_GEN( cska_interrupt );
VIDEO_UPDATE( cska );
VIDEO_START( cska );

WRITE8_HANDLER( cpk_palette_w );
WRITE8_HANDLER( cpk_palette2_w );
READ8_HANDLER( cpk_videoram_r );
WRITE8_HANDLER( cpk_videoram_w );
READ8_HANDLER( cpk_colorram_r );
WRITE8_HANDLER( cpk_colorram_w );
READ8_HANDLER( cpk_expansion_r );
WRITE8_HANDLER( cpk_expansion_w );



static size_t protection_res = 0;

static READ8_HANDLER( custom_io_r )
{
	return protection_res;
}

static WRITE8_HANDLER( custom_io_w )
{
	switch (data)
	{
		case 0x20: protection_res = 0x49; break;
		case 0x21: protection_res = 0x47; break;
		case 0x22: protection_res = 0x53; break;
		case 0x24: protection_res = 0x41; break;
		case 0x25: protection_res = 0x41; break;
		case 0x26: protection_res = 0x7f; break;
		case 0x27: protection_res = 0x41; break;
		case 0x28: protection_res = 0x41; break;
		case 0x2a: protection_res = 0x3e; break;
		case 0x2b: protection_res = 0x41; break;
		case 0x2c: protection_res = 0x49; break;
		case 0x2d: protection_res = 0xf9; break;
		case 0x2e: protection_res = 0x0a; break;
		case 0x30: protection_res = 0x26; break;
		case 0x31: protection_res = 0x49; break;
		case 0x32: protection_res = 0x49; break;
		case 0x33: protection_res = 0x49; break;
		case 0x34: protection_res = 0x32; break;
		default:
			protection_res = data;
	}
}


static ADDRESS_MAP_START( map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_REGION(REGION_CPU1, 0xf000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( csk227_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(cpk_palette_w)
	AM_RANGE(0x2800, 0x2fff) AM_WRITE(cpk_palette2_w)
	AM_RANGE(0x4000, 0x4000) AM_READ(input_port_0_r)		/* DSW1 */
	AM_RANGE(0x4001, 0x4001) AM_READ(input_port_1_r)		/* DSW2 */
	AM_RANGE(0x4002, 0x4002) AM_READ(input_port_2_r)		/* DSW3 */
	AM_RANGE(0x4003, 0x4003) AM_READ(input_port_3_r)		/* DSW4 */
	AM_RANGE(0x4004, 0x4004) AM_READ(input_port_4_r)		/* DSW5 */
	AM_RANGE(0x5081, 0x5081) AM_READ(input_port_5_r)		/* Services */
	AM_RANGE(0x5082, 0x5082) AM_READ(input_port_6_r)		/* Coing & Kbd */
	AM_RANGE(0x5091, 0x5091) AM_READ(input_port_7_r)		/* Keyboard */
	AM_RANGE(0x50a0, 0x50a0) AM_READ(input_port_8_r)		/* Not connected */
	AM_RANGE(0x6800, 0x6fff) AM_WRITE(cpk_expansion_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(cpk_videoram_r,cpk_videoram_w)
	AM_RANGE(0x7800, 0x7fff) AM_READWRITE(cpk_colorram_r,cpk_colorram_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(cpk_expansion_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( csk234_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(cpk_palette_w)
	AM_RANGE(0x2800, 0x2fff) AM_WRITE(cpk_palette2_w)
	AM_RANGE(0x4000, 0x4000) AM_READ(input_port_0_r)		/* DSW1 */
	AM_RANGE(0x4001, 0x4001) AM_READ(input_port_1_r)		/* DSW2 */
	AM_RANGE(0x4002, 0x4002) AM_READ(input_port_2_r)		/* DSW3 */
	AM_RANGE(0x4003, 0x4003) AM_READ(input_port_3_r)		/* DSW4 */
	AM_RANGE(0x4004, 0x4004) AM_READ(input_port_4_r)		/* DSW5 */
	AM_RANGE(0x5081, 0x5081) AM_READ(input_port_5_r)		/* Services */
	AM_RANGE(0x5082, 0x5082) AM_READ(input_port_6_r)		/* Coing & Kbd */
	AM_RANGE(0x5090, 0x5090) AM_WRITE(custom_io_w)
	AM_RANGE(0x5091, 0x5091) AM_READ(custom_io_r)			/* used for protection and other */
	AM_RANGE(0x50a0, 0x50a0) AM_READ(input_port_8_r)		/* Not connected */
	AM_RANGE(0x6800, 0x6fff) AM_WRITE(cpk_expansion_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(cpk_videoram_r,cpk_videoram_w)
	AM_RANGE(0x7800, 0x7fff) AM_READWRITE(cpk_colorram_r,cpk_colorram_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(cpk_expansion_r)
ADDRESS_MAP_END


/* MB: 05 Jun 99  Input ports and Dip switches are all verified! */

INPUT_PORTS_START( csk227 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Demo Music" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x06, 0x06, "Min Bet to Start" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x18, 0x18, "Max Bet" )
	PORT_DIPSETTING(    0x18, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x60, 0x60, "Min Bet to play Fever" )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Credit Limit" )
	PORT_DIPSETTING(    0x80, "100000" )
	PORT_DIPSETTING(    0x00, "Unlimited" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x07, "Coin In Rate" )
	PORT_DIPSETTING(    0x07, "1" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x04, "10" )
	PORT_DIPSETTING(    0x03, "20" )
	PORT_DIPSETTING(    0x02, "40" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x18, 0x18, "Key In Rate" )
	PORT_DIPSETTING(    0x18, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x60, 0x60, "W-UP Bonus Target" )
	PORT_DIPSETTING(    0x60, "1500" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x00, "7500" )
	PORT_DIPNAME( 0x80, 0x80, "Payout" )
	PORT_DIPSETTING(    0x80, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x03, 0x03, "W-UP Bonus Rate" )
	PORT_DIPSETTING(    0x03, "200" )
	PORT_DIPSETTING(    0x02, "300" )
	PORT_DIPSETTING(    0x01, "500" )
	PORT_DIPSETTING(    0x00, "800" )
	PORT_DIPNAME( 0x0c, 0x0c, "W-UP Chance" )
	PORT_DIPSETTING(    0x0c, "94%" )
	PORT_DIPSETTING(    0x08, "96%" )
	PORT_DIPSETTING(    0x04, "98%" )
	PORT_DIPSETTING(    0x00, "100%" )
	PORT_DIPNAME( 0x30, 0x20, "W-UP Type" )
	PORT_DIPSETTING(    0x30, DEF_STR( None ) )
	PORT_DIPSETTING(    0x20, "High-Low" )
	PORT_DIPSETTING(    0x10, "Red-Black" )		/* Bit 4 is equal for ON/OFF */
	PORT_DIPNAME( 0x40, 0x00, "Strip Girl" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, "Ability" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x0f, 0x07, "Main Game Chance" )
	PORT_DIPSETTING(    0x0f, "69%" )
	PORT_DIPSETTING(    0x0e, "72%" )
	PORT_DIPSETTING(    0x0d, "75%" )
	PORT_DIPSETTING(    0x0c, "78%" )
	PORT_DIPSETTING(    0x0b, "81%" )
	PORT_DIPSETTING(    0x0a, "83%" )
	PORT_DIPSETTING(    0x09, "85%" )
	PORT_DIPSETTING(    0x08, "87%" )
	PORT_DIPSETTING(    0x07, "89%" )
	PORT_DIPSETTING(    0x06, "91%" )
	PORT_DIPSETTING(    0x05, "93%" )
	PORT_DIPSETTING(    0x04, "95%" )
	PORT_DIPSETTING(    0x03, "97%" )
	PORT_DIPSETTING(    0x02, "99%" )
	PORT_DIPSETTING(    0x01, "101%" )
	PORT_DIPSETTING(    0x00, "103%" )
	PORT_DIPNAME( 0x10, 0x00, "Five Jokers" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Royal Flush" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Payout Select" )
	PORT_DIPSETTING(    0x80, "Ticket" )
	PORT_DIPSETTING(    0x00, "Hopper" )

	PORT_START_TAG("DSW5")
	PORT_DIPNAME( 0x07, 0x07, "Key Out Rate" )
	PORT_DIPSETTING(    0x07, "1:1" )
	PORT_DIPSETTING(    0x06, "10:1" )
	PORT_DIPSETTING(    0x05, "20:1" )
	PORT_DIPSETTING(    0x04, "50:1" )
	PORT_DIPSETTING(    0x03, "100:1" )		/* Bits 1-0 are all equivalents */
	PORT_DIPNAME( 0x08, 0x00, "Card Select" )
	PORT_DIPSETTING(    0x08, "Poker" )
	PORT_DIPSETTING(    0x00, "Tetris" )
	PORT_DIPNAME( 0x70, 0x70, "Ticket Rate" )
	PORT_DIPSETTING(    0x70, "1:1" )
	PORT_DIPSETTING(    0x60, "5:1" )
	PORT_DIPSETTING(    0x50, "10:1" )
	PORT_DIPSETTING(    0x40, "20:1" )
	PORT_DIPSETTING(    0x30, "25:1" )
	PORT_DIPSETTING(    0x20, "50:1" )
	PORT_DIPSETTING(    0x10, "100:1" )
	PORT_DIPSETTING(    0x00, "200:1" )
	PORT_DIPNAME( 0x80, 0x80, "Win Table" )
	PORT_DIPSETTING(    0x80, "Change" )
	PORT_DIPSETTING(    0x00, "Fixed" )

	PORT_START_TAG("5081")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_E) PORT_NAME("HPSW")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_4) PORT_NAME("Payout")
	PORT_SERVICE_NO_TOGGLE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_F1) PORT_NAME("Statistics")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5/Bet")

	PORT_START_TAG("5082")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 	 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 	 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE  ) PORT_CODE(KEYCODE_Q) PORT_NAME("Key In")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_SERVICE  ) PORT_CODE(KEYCODE_W) PORT_NAME("Key Down")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4/Take")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3/W-Up")

	PORT_START_TAG("5091")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1/High/Low")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2/Red/Black")
	PORT_START_TAG("50A0")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END


INPUT_PORTS_START( csk234 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Demo Music" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x06, 0x06, "Min Bet to Start" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x18, 0x10, "Max Bet" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPNAME( 0x60, 0x60, "Min Bet to play Fever" )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Credit Limit" )
	PORT_DIPSETTING(    0x80, "5000" )
	PORT_DIPSETTING(    0x00, "10000" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x07, "Coin In Rate" )
	PORT_DIPSETTING(    0x07, "1" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x04, "10" )
	PORT_DIPSETTING(    0x03, "20" )
	PORT_DIPSETTING(    0x02, "40" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x18, 0x18, "Key In Rate" )
	PORT_DIPSETTING(    0x18, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x08, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x60, 0x60, "Key Out Rate" )
	PORT_DIPSETTING(    0x60, "1:1" )
	PORT_DIPSETTING(    0x40, "10:1" )
	PORT_DIPSETTING(    0x20, "100:1" )
	PORT_DIPSETTING(    0x00, "100:1" )
	PORT_DIPNAME( 0x80, 0x80, "Payout" )
	PORT_DIPSETTING(    0x80, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "W-UP Bonus Target" )
	PORT_DIPSETTING(    0x01, "3000" )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPNAME( 0x02, 0x02, "W-UP Bonus Rate" )
	PORT_DIPSETTING(    0x02, "300" )
	PORT_DIPSETTING(    0x00, "500" )
	PORT_DIPNAME( 0x0c, 0x0c, "W-UP Chance" )
	PORT_DIPSETTING(    0x0c, "94%" )
	PORT_DIPSETTING(    0x08, "96%" )
	PORT_DIPSETTING(    0x04, "98%" )
	PORT_DIPSETTING(    0x00, "100%" )
	PORT_DIPNAME( 0x30, 0x20, "W-UP Type" )
	PORT_DIPSETTING(    0x30, DEF_STR( None ) )
	PORT_DIPSETTING(    0x20, "High-Low" )
	PORT_DIPSETTING(    0x10, "Red-Black" )		/* Bit 4 is equal for ON/OFF */
	PORT_DIPNAME( 0x40, 0x40, "Card Select" )
	PORT_DIPSETTING(    0x40, "Poker" )
	PORT_DIPSETTING(    0x00, "Symbols" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x0f, 0x07, "Main Game Chance" )
	PORT_DIPSETTING(    0x0f, "69%" )
	PORT_DIPSETTING(    0x0e, "72%" )
	PORT_DIPSETTING(    0x0d, "75%" )
	PORT_DIPSETTING(    0x0c, "78%" )
	PORT_DIPSETTING(    0x0b, "81%" )
	PORT_DIPSETTING(    0x0a, "83%" )
	PORT_DIPSETTING(    0x09, "85%" )
	PORT_DIPSETTING(    0x08, "87%" )
	PORT_DIPSETTING(    0x07, "89%" )
	PORT_DIPSETTING(    0x06, "91%" )
	PORT_DIPSETTING(    0x05, "93%" )
	PORT_DIPSETTING(    0x04, "95%" )
	PORT_DIPSETTING(    0x03, "97%" )
	PORT_DIPSETTING(    0x02, "99%" )
	PORT_DIPSETTING(    0x01, "101%" )
	PORT_DIPSETTING(    0x00, "103%" )
	PORT_DIPNAME( 0x10, 0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Anytime Key-in" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0xC0, IP_ACTIVE_LOW, IPT_UNUSED )			/* Joker and Royal Flush are always enabled */

	PORT_START_TAG("DSW5")
	PORT_DIPNAME( 0x01, 0x00, "Hopper" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "Payout Select" )
	PORT_DIPSETTING(    0x02, "Hopper" )
	PORT_DIPSETTING(    0x00, "Ticket" )
	PORT_DIPNAME( 0x0c, 0x0c, "Ticket Rate" )
	PORT_DIPSETTING(    0x0c, "10:1" )
	PORT_DIPSETTING(    0x08, "20:1" )
	PORT_DIPSETTING(    0x04, "50:1" )
	PORT_DIPSETTING(    0x00, "100:1" )
	PORT_DIPNAME( 0x10, 0x00, "Ability" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("5081")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_E) PORT_NAME("HPSW")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_4) PORT_NAME("Payout")
	PORT_SERVICE_NO_TOGGLE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_F1) PORT_NAME("Statistics")

	PORT_START_TAG("5082")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_Q) PORT_NAME("Key In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_W) PORT_NAME("Key Down")
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("5091")	/* Custom IO */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("50A0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1/High/Low")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5/Bet")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4/Take")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3/W-Up")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2/Red/Black")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	8192,   /* 8192 characters */
	6,      /* 6 bits per pixel */
	{ 8, 0, 0x20000*8+8, 0x20000*8+0, 0x40000*8+8, 0x40000*8+0 }, /* the bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8   /* every char takes 32 consecutive bytes */
};

static const gfx_layout charlayout2 =
{
	8,32,   /* 8*32 characters */
	256,    /* 256 characters */
	6,      /* 6 bits per pixel */
	{ 8, 0, 0x10000*8+8, 0x10000*8+0, 0x20000*8+8, 0x20000*8+0 }, /* the bitplanes are packed in one byte */
	{
		0, 1, 2, 3, 4, 5, 6, 7,
	},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	4*16*8   /* every char takes 32 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,   0, 16 },
	{ REGION_GFX2, 0x04000, &charlayout2,  0, 16 },
	{ REGION_GFX2, 0x08000, &charlayout2,  0, 16 },
	{ REGION_GFX2, 0x0c000, &charlayout2,  0, 16 },
	{ REGION_GFX2, 0x00000, &charlayout2,  0, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( csk227it )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80, 3579545)//(4000000?)
	MDRV_CPU_PROGRAM_MAP(map,0)
	MDRV_CPU_IO_MAP(csk227_map,0)
	MDRV_CPU_VBLANK_INT(cska_interrupt,6)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(cpk)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_COLORTABLE_LENGTH(2048)

	MDRV_VIDEO_START(cska)
	MDRV_VIDEO_UPDATE(cska)

	/* sound hardware */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( csk234it )

	MDRV_IMPORT_FROM(csk227it)
	MDRV_CPU_MODIFY("main")
	/* basic machine hardware */
	MDRV_CPU_IO_MAP(csk234_map,0)
	MACHINE_DRIVER_END

/*  ROM Regions definition
 */

ROM_START( csk227it )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "v227i.bin",   0x0000, 0x10000, CRC(df1ebf49) SHA1(829c7575d3d3780557405b3a61859901df6dbe4f) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6.227",  0x00000, 0x20000, CRC(e9aad93b) SHA1(72116759cd8ddd9828f534e8f8a3f9f96ad2e002) )
	ROM_LOAD( "5.227",  0x20000, 0x20000, CRC(e4c4c8da) SHA1(0442b0de68f3b69e613506348e00c3cf9139edcf) )
	ROM_LOAD( "4.227",  0x40000, 0x20000, CRC(afb365dd) SHA1(930a4cd516258e703a75afc25ef6b2655b8b696a) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",  0x00000, 0x10000, CRC(fcb115ac) SHA1(a9f2b9762413840669cd44f8e54b47a7c4350d11) )	/* extension charset, used for ability game */
	ROM_LOAD( "2.bin",  0x10000, 0x10000, CRC(848343a3) SHA1(b12f9bc2feb470d2fa8b085621fa60c0895109d4) )
	ROM_LOAD( "1.bin",  0x20000, 0x10000, CRC(921ad5de) SHA1(b06ab2e63b31361dcb0367110f47bf2453ecdca6) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* expansion rom - contains backgrounds and pictures charmaps */
	ROM_LOAD( "7.227",   0x0000, 0x10000, CRC(a10786ad) SHA1(82f5f81808ca70d67a2710cc66fbbf78588b33b5) )
ROM_END

ROM_START( csk234it )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "v234it.bin",   0x0000, 0x10000, CRC(344b7059)  SHA1(990cb84e35c0c50d3be9fbb76a11395114dc6c9b) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6.234",  0x00000, 0x20000, CRC(23b855a4) SHA1(8217bac61ad09483d8789113cf394d0e525ab28a) )
	ROM_LOAD( "5.234",  0x20000, 0x20000, CRC(189039d7) SHA1(146fd1ddb23ceaa4192e0382b0ab82f5cfbdabfe) )
	ROM_LOAD( "4.234",  0x40000, 0x20000, CRC(c82b0ffc) SHA1(5ebd7da76d402b7111cbe9012cfa3b8a8ff1a86e) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",  0x00000, 0x10000, CRC(fcb115ac) SHA1(a9f2b9762413840669cd44f8e54b47a7c4350d11) )	/* extension charset, used for ability game */
	ROM_LOAD( "2.bin",  0x10000, 0x10000, CRC(848343a3) SHA1(b12f9bc2feb470d2fa8b085621fa60c0895109d4) )
	ROM_LOAD( "1.bin",  0x20000, 0x10000, CRC(921ad5de) SHA1(b06ab2e63b31361dcb0367110f47bf2453ecdca6) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* expansion rom - contains backgrounds and pictures charmaps */
	ROM_LOAD( "7.234",   0x0000, 0x10000, CRC(ae6dd4ad) SHA1(4772d5c150d64d1ef3b68e16214f594eea0b3c1b) )
ROM_END

/*  Decode a simple PAL encryption
 */

static void init_cska(void)
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);


	for (A = 0;A < 0x10000;A++)
	{
		if ((A & 0x0020) == 0x0000) rom[A] ^= 0x01;
		if ((A & 0x0020) == 0x0020) rom[A] ^= 0x21;
		if ((A & 0x0282) == 0x0282) rom[A] ^= 0x01;
		if ((A & 0x0028) == 0x0028) rom[A] ^= 0x20;
		if ((A & 0x0940) == 0x0940) rom[A] ^= 0x02;
	}
}

GAME( 198?, csk227it, 0,        csk227it, csk227, cska, ROT0, "IGS", "Champion Skill (with Ability)", GAME_NO_SOUND )               /* SU 062 */
GAME( 198?, csk234it, csk227it, csk234it, csk234, cska, ROT0, "IGS", "Champion Skill (Ability, Poker & Symbols)", GAME_NO_SOUND )   /* SU 062 */
