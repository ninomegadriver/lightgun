/******************************************************************************

    Machine Hardware for Nichibutsu Mahjong series.

    Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -

******************************************************************************/
/******************************************************************************
Memo:

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "nb1413m3.h"


#define NB1413M3_DEBUG	0
#define NB1413M3_CHEAT	0


int nb1413m3_type;
int nb1413m3_sndromregion;
int nb1413m3_sndrombank1;
int nb1413m3_sndrombank2;
int nb1413m3_busyctr;
int nb1413m3_busyflag;
int nb1413m3_inputport;
unsigned char *nb1413m3_nvram;
size_t nb1413m3_nvram_size;

static int nb1413m3_74ls193_counter;
static int nb1413m3_nmi_count;			// for debug
static int nb1413m3_nmi_clock;
static int nb1413m3_nmi_enable;
static int nb1413m3_counter;
static int nb1413m3_gfxradr_l;
static int nb1413m3_gfxradr_h;
static int nb1413m3_gfxrombank;
static int nb1413m3_outcoin_enable;
static int nb1413m3_outcoin_flag;


#define NB1413M3_TIMER_BASE 20000000
void nb1413m3_timer_callback(int param)
{
	timer_set(TIME_IN_HZ(NB1413M3_TIMER_BASE / 256), 0, nb1413m3_timer_callback);

	nb1413m3_74ls193_counter++;
	nb1413m3_74ls193_counter &= 0x0f;

	if (nb1413m3_74ls193_counter == 0x0f)
	{

		if (nb1413m3_nmi_enable)
		{
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
			nb1413m3_nmi_count++;
		}

#if 1
		switch (nb1413m3_type)
		{
			case NB1413M3_TAIWANMB:
				nb1413m3_74ls193_counter = 0x05;	// 130 ???
				break;
			case NB1413M3_OMOTESND:
				nb1413m3_74ls193_counter = 0x05;	// 130 ???
				break;
			case NB1413M3_PASTELG:
				nb1413m3_74ls193_counter = 0x02;	// 96 ???
				break;
			case NB1413M3_HYHOO:
			case NB1413M3_HYHOO2:
				nb1413m3_74ls193_counter = 0x05;	// 128 ???
				break;
		}
#endif
	}

#if 0
	// nbmj1413m3_nmi_clock_w ?w??
	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8688    Z80:5.00MHz (20000000/4)
	// 7    144-145         mjsikaku, mjsikakb, otonano, mjcamera

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8891    Z80:5.00MHz (20000000/4)
	// 7    144-145         msjiken, telmahjn, mjcamerb, mmcamera

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8688    Z80:5.00MHz (20000000/4)
	// 6    130-131         kaguya, kaguya2, idhimitu

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8891    Z80:5.00MHz (20000000/4)
	// 6    130-131         hanamomo, gionbana, mgion, abunai, mjfocus, mjfocusm, peepshow, scandal, scandalm, mgmen89,
	//                      mjnanpas, mjnanpaa, mjnanpau, bananadr, mladyhtr, chinmoku, club90s, club90sa, lovehous,
	//                      maiko, mmaiko, hanaoji, pairsten

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8991    Z80:5MHz (25000000/5)
	// 6    130-131         galkoku, hyouban, galkaika, tokyogal, tokimbsj, mcontest, uchuuai

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8688    Z80:5.00MHz (20000000/4)
	// 6     81- 82         crystalg(DAC?????x??),  crystal2(DAC?????x??)
	// 6    130-131         bijokkoy(?A?j????), bijokkog(?A?j????)

	// ----------------------------------------------------------------------------------------------------------------
	//                      nbmj8688    Z80:5.00MHz (20000000/4)
	// 4    108-109         bijokkoy(?A?j????), bijokkog(?A?j????)

	// ----------------------------------------------------------------------------------------------------------------

	// nbmj1413m3_nmi_clock_w ???w??
	//*5    130-131?        hyhoo, hyhoo2   5.00MHz (????????DAC???????x???????????c)
	//*5    130-131?        taiwanmb        5.00MHz (???@??????????DAC???????x?s??)
	//*5    128-129?        omotesnd        5.00MHz
	//*2    100-101?        pastelg         2.496MHz (19968000/8) ???
#endif
}

DRIVER_INIT( nb1413m3 )
{
	;
}

MACHINE_RESET( nb1413m3 )
{
	nb1413m3_nmi_clock = 0;
	nb1413m3_nmi_enable = 0;
	nb1413m3_nmi_count = 0;
	nb1413m3_74ls193_counter = 0;
	nb1413m3_counter = 0;
	nb1413m3_sndromregion = REGION_SOUND1;
	nb1413m3_sndrombank1 = 0;
	nb1413m3_sndrombank2 = 0;
	nb1413m3_busyctr = 0;
	nb1413m3_busyflag = 1;
	nb1413m3_gfxradr_l = 0;
	nb1413m3_gfxradr_h = 0;
	nb1413m3_gfxrombank = 0;
	nb1413m3_inputport = 0xff;
	nb1413m3_outcoin_flag = 1;

	nb1413m3_74ls193_counter = 0;

	timer_set(TIME_NOW, 0, nb1413m3_timer_callback);
}

WRITE8_HANDLER( nb1413m3_nmi_clock_w )
{
	nb1413m3_nmi_clock = data;

	switch (nb1413m3_type)
	{
		case NB1413M3_APPAREL:
		case NB1413M3_CITYLOVE:
		case NB1413M3_MCITYLOV:
		case NB1413M3_SECOLOVE:
		case NB1413M3_SEIHA:
		case NB1413M3_SEIHAM:
		case NB1413M3_IEMOTO:
		case NB1413M3_IEMOTOM:
		case NB1413M3_BIJOKKOY:
		case NB1413M3_BIJOKKOG:
		case NB1413M3_RYUUHA:
		case NB1413M3_OJOUSAN:
		case NB1413M3_OJOUSANM:
		case NB1413M3_KORINAI:
		case NB1413M3_KORINAIM:
		case NB1413M3_HOUSEMNQ:
		case NB1413M3_HOUSEMN2:
		case NB1413M3_LIVEGAL:
		case NB1413M3_ORANGEC:
		case NB1413M3_ORANGECI:
		case NB1413M3_VIPCLUB:
		case NB1413M3_MMSIKAKU:
		case NB1413M3_KANATUEN:
		case NB1413M3_KYUHITO:
			nb1413m3_nmi_clock -= 1;
			break;
#if 1
		case NB1413M3_NIGHTLOV:
			nb1413m3_nmi_enable = ((data & 0x08) >> 3);
			nb1413m3_nmi_enable |= ((data & 0x01) ^ 0x01);
			nb1413m3_nmi_clock -= 1;

			nb1413m3_sndrombank1 = 1;
			break;
#endif
	}

	nb1413m3_74ls193_counter = ((nb1413m3_nmi_clock & 0xf0) >> 4);

}

INTERRUPT_GEN( nb1413m3_interrupt )
{
#if 0
	if (!cpu_getiloops())
	{
//      nb1413m3_busyflag = 1;
//      nb1413m3_busyctr = 0;
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
	if (nb1413m3_nmi_enable)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}

	#if NB1413M3_CHEAT
	#include "nbmjchet.inc"
	#endif
#else
//  nb1413m3_busyflag = 1;
//  nb1413m3_busyctr = 0;
	cpunum_set_input_line(0, 0, HOLD_LINE);

#if NB1413M3_DEBUG
	ui_popup("NMI SW:%01X CLOCK:%02X COUNT:%02X", nb1413m3_nmi_enable, nb1413m3_nmi_clock, nb1413m3_nmi_count);
	nb1413m3_nmi_count = 0;
#endif

	#if NB1413M3_CHEAT
	#include "nbmjchet.inc"
	#endif
#endif
}

NVRAM_HANDLER( nb1413m3 )
{
	if (read_or_write)
		mame_fwrite(file, nb1413m3_nvram, nb1413m3_nvram_size);
	else
	{
		if (file)
			mame_fread(file, nb1413m3_nvram, nb1413m3_nvram_size);
		else
			memset(nb1413m3_nvram, 0, nb1413m3_nvram_size);
	}
}

READ8_HANDLER( nb1413m3_sndrom_r )
{
	int rombank;

	/* get top 8 bits of the I/O port address */
	offset = (offset << 8) | (activecpu_get_reg(Z80_BC) >> 8);

	switch (nb1413m3_type)
	{
		case NB1413M3_IEMOTO:
		case NB1413M3_IEMOTOM:
		case NB1413M3_SEIHA:
		case NB1413M3_SEIHAM:
		case NB1413M3_RYUUHA:
		case NB1413M3_OJOUSAN:
		case NB1413M3_OJOUSANM:
		case NB1413M3_MJSIKAKU:
		case NB1413M3_MMSIKAKU:
		case NB1413M3_KORINAI:
		case NB1413M3_KORINAIM:
			rombank = (nb1413m3_sndrombank2 << 1) + (nb1413m3_sndrombank1 & 0x01);
			break;
		case NB1413M3_HYHOO:
		case NB1413M3_HYHOO2:
			rombank = (nb1413m3_sndrombank1 & 0x01);
			break;
		case NB1413M3_APPAREL:		// no samples
		case NB1413M3_NIGHTLOV:		// 0-1
		case NB1413M3_SECOLOVE:		// 0-1
		case NB1413M3_CITYLOVE:		// 0-1
		case NB1413M3_MCITYLOV:		// 0-1
		case NB1413M3_HOUSEMNQ:		// 0-1
		case NB1413M3_HOUSEMN2:		// 0-1
		case NB1413M3_LIVEGAL:		// 0-1
		case NB1413M3_ORANGEC:		// 0-1
		case NB1413M3_KAGUYA:		// 0-3
		case NB1413M3_KAGUYA2:		// 0-3 + 4-5 for protection
		case NB1413M3_BIJOKKOY:		// 0-7
		case NB1413M3_BIJOKKOG:		// 0-7
		case NB1413M3_OTONANO:		// 0-7
		case NB1413M3_MJCAMERA:		// 0 + 4-5 for protection
		case NB1413M3_IDHIMITU:		// 0 + 4-5 for protection
		case NB1413M3_KANATUEN:		// 0 + 6 for protection
			rombank = nb1413m3_sndrombank1;
			break;
		case NB1413M3_TAIWANMB:
		case NB1413M3_OMOTESND:
		case NB1413M3_SCANDAL:
		case NB1413M3_SCANDALM:
		case NB1413M3_MJFOCUSM:
		case NB1413M3_BANANADR:
			offset = (((offset & 0x7f00) >> 8) | ((offset & 0x0080) >> 0) | ((offset & 0x007f) << 8));
			rombank = (nb1413m3_sndrombank1 >> 1);
			break;
		case NB1413M3_MMCAMERA:
		case NB1413M3_MSJIKEN:
		case NB1413M3_HANAMOMO:
		case NB1413M3_TELMAHJN:
		case NB1413M3_GIONBANA:
		case NB1413M3_MGION:
		case NB1413M3_MGMEN89:
		case NB1413M3_MJFOCUS:
		case NB1413M3_GALKOKU:
		case NB1413M3_HYOUBAN:
		case NB1413M3_MJNANPAS:
		case NB1413M3_MLADYHTR:
		case NB1413M3_CLUB90S:
		case NB1413M3_LOVEHOUS:
		case NB1413M3_CHINMOKU:
		case NB1413M3_GALKAIKA:
		case NB1413M3_MCONTEST:
		case NB1413M3_UCHUUAI:
		case NB1413M3_TOKIMBSJ:
		case NB1413M3_TOKYOGAL:
		case NB1413M3_MAIKO:
		case NB1413M3_MMAIKO:
		case NB1413M3_HANAOJI:
		case NB1413M3_PAIRSNB:
		case NB1413M3_PAIRSTEN:
		default:
			rombank = (nb1413m3_sndrombank1 >> 1);
			break;
	}

	offset += 0x08000 * rombank;

#if NB1413M3_DEBUG
	ui_popup("Sound ROM %02X:%05X [B1:%02X B2:%02X]", rombank, offset, nb1413m3_sndrombank1, nb1413m3_sndrombank2);
#endif

	if (offset < memory_region_length(nb1413m3_sndromregion))
		return memory_region(nb1413m3_sndromregion)[offset];
	else
	{
		ui_popup("read past sound ROM length (%05x[%02X])",offset, rombank);
		return 0;
	}
}

WRITE8_HANDLER( nb1413m3_sndrombank1_w )
{
	// if (data & 0x02) coin counter ?
	nb1413m3_outcoin_w(0, data);				// (data & 0x04) >> 2;
	nb1413m3_nmi_enable = ((data & 0x20) >> 5);
	nb1413m3_sndrombank1 = (((data & 0xc0) >> 5) | ((data & 0x10) >> 4));
}

WRITE8_HANDLER( nb1413m3_sndrombank2_w )
{
	nb1413m3_sndrombank2 = (data & 0x03);
}

READ8_HANDLER( nb1413m3_gfxrom_r )
{
	unsigned char *GFXROM = memory_region(REGION_GFX1);

	return GFXROM[(0x20000 * (nb1413m3_gfxrombank | ((nb1413m3_sndrombank1 & 0x02) << 3))) + ((0x0200 * nb1413m3_gfxradr_h) + (0x0002 * nb1413m3_gfxradr_l)) + (offset & 0x01)];
}

WRITE8_HANDLER( nb1413m3_gfxrombank_w )
{
	nb1413m3_gfxrombank = (((data & 0xc0) >> 4) + (data & 0x03));
}

WRITE8_HANDLER( nb1413m3_gfxradr_l_w )
{
	nb1413m3_gfxradr_l = data;
}

WRITE8_HANDLER( nb1413m3_gfxradr_h_w )
{
	nb1413m3_gfxradr_h = data;
}

WRITE8_HANDLER( nb1413m3_inputportsel_w )
{
	nb1413m3_inputport = data;
}

READ8_HANDLER( nb1413m3_inputport0_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_PASTELG:
			return ((input_port_3_r(0) & 0xfe) | (nb1413m3_busyflag & 0x01));
		case NB1413M3_TAIWANMB:
			return ((input_port_3_r(0) & 0xfc) | ((nb1413m3_outcoin_flag & 0x01) << 1) | (nb1413m3_busyflag & 0x01));
		case NB1413M3_HYHOO:
		case NB1413M3_HYHOO2:
			return ((input_port_2_r(0) & 0xfe) | (nb1413m3_busyflag & 0x01));
		default:
			return ((input_port_2_r(0) & 0xfc) | ((nb1413m3_outcoin_flag & 0x01) << 1) | (nb1413m3_busyflag & 0x01));
	}
}

READ8_HANDLER( nb1413m3_inputport1_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_PASTELG:
		case NB1413M3_THREEDS:
		case NB1413M3_TAIWANMB:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case 0x01:	return readinputport(4);
				case 0x02:	return readinputport(5);
				case 0x04:	return readinputport(6);
				case 0x08:	return readinputport(7);
				case 0x10:	return readinputport(8);
				default:	return (readinputport(4) & readinputport(5) & readinputport(6) & readinputport(7) & readinputport(8));
			}
			break;
		case NB1413M3_HYHOO:
		case NB1413M3_HYHOO2:
			switch ((nb1413m3_inputport ^ 0xff) & 0x07)
			{
				case 0x01:	return readinputport(3);
				case 0x02:	return readinputport(4);
				case 0x04:	return 0xff;
				default:	return 0xff;
			}
			break;
		case NB1413M3_MSJIKEN:
		case NB1413M3_TELMAHJN:
			if (readinputport(0) & 0x80)
			{
				switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
				{
					case 0x01:	return readinputport(3);
					case 0x02:	return readinputport(4);
					case 0x04:	return readinputport(5);
					case 0x08:	return readinputport(6);
					case 0x10:	return readinputport(7);
					default:	return (readinputport(3) & readinputport(4) & readinputport(5) & readinputport(6) & readinputport(7));
				}
			}
			else return readinputport(14);
			break;
		case NB1413M3_PAIRSNB:
		case NB1413M3_PAIRSTEN:
			return readinputport(3);
			break;
		default:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case 0x01:	return readinputport(3);
				case 0x02:	return readinputport(4);
				case 0x04:	return readinputport(5);
				case 0x08:	return readinputport(6);
				case 0x10:	return readinputport(7);
				default:	return (readinputport(3) & readinputport(4) & readinputport(5) & readinputport(6) & readinputport(7));
			}
			break;
	}
}

READ8_HANDLER( nb1413m3_inputport2_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_PASTELG:
		case NB1413M3_THREEDS:
		case NB1413M3_TAIWANMB:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case 0x01:	return readinputport(9);
				case 0x02:	return readinputport(10);
				case 0x04:	return readinputport(11);
				case 0x08:	return readinputport(12);
				case 0x10:	return readinputport(13);
				default:	return (readinputport(9) & readinputport(10) & readinputport(11) & readinputport(12) & readinputport(13));
			}
			break;
		case NB1413M3_HYHOO:
		case NB1413M3_HYHOO2:
			switch ((nb1413m3_inputport ^ 0xff) & 0x07)
			{
				case 0x01:	return 0xff;
				case 0x02:	return 0xff;
				case 0x04:	return readinputport(5);
				default:	return 0xff;
			}
			break;
		case NB1413M3_MSJIKEN:
		case NB1413M3_TELMAHJN:
			if (readinputport(0) & 0x80)
			{
				switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
				{
					case 0x01:	return readinputport(8);
					case 0x02:	return readinputport(9);
					case 0x04:	return readinputport(10);
					case 0x08:	return readinputport(11);
					case 0x10:	return readinputport(12);
					default:	return (readinputport(8) & readinputport(9) & readinputport(10) & readinputport(11) & readinputport(12));
				}
			}
			else return readinputport(13);
			break;
		case NB1413M3_PAIRSNB:
		case NB1413M3_PAIRSTEN:
			return readinputport(4);
			break;
		default:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case 0x01:	return readinputport(8);
				case 0x02:	return readinputport(9);
				case 0x04:	return readinputport(10);
				case 0x08:	return readinputport(11);
				case 0x10:	return readinputport(12);
				default:	return (readinputport(8) & readinputport(9) & readinputport(10) & readinputport(11) & readinputport(12));
			}
			break;
	}
}

READ8_HANDLER( nb1413m3_inputport3_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_TAIWANMB:
		case NB1413M3_IEMOTOM:
		case NB1413M3_OJOUSANM:
		case NB1413M3_SEIHAM:
		case NB1413M3_RYUUHA:
		case NB1413M3_KORINAIM:
		case NB1413M3_HYOUBAN:
		case NB1413M3_TOKIMBSJ:
		case NB1413M3_MJFOCUSM:
		case NB1413M3_SCANDALM:
		case NB1413M3_BANANADR:
		case NB1413M3_FINALBNY:
		case NB1413M3_MMSIKAKU:
			return ((nb1413m3_outcoin_flag & 0x01) << 1);
			break;
		case NB1413M3_LOVEHOUS:
		case NB1413M3_MAIKO:
		case NB1413M3_MMAIKO:
		case NB1413M3_HANAOJI:
			return ((input_port_13_r(0) & 0xfd) | ((nb1413m3_outcoin_flag & 0x01) << 1));
			break;
		default:
			return 0xff;
			break;
	}
}

READ8_HANDLER( nb1413m3_dipsw1_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_KANATUEN:
		case NB1413M3_KYUHITO:
			return readinputport(1);
			break;
		case NB1413M3_TAIWANMB:
			return ((readinputport(0) & 0xf0) | ((readinputport(1) & 0xf0) >> 4));
			break;
		case NB1413M3_OTONANO:
		case NB1413M3_MJCAMERA:
		case NB1413M3_IDHIMITU:
		case NB1413M3_KAGUYA2:
			return (((readinputport(0) & 0x0f) << 4) | (readinputport(1) & 0x0f));
			break;
		case NB1413M3_SCANDAL:
		case NB1413M3_SCANDALM:
		case NB1413M3_MJFOCUSM:
		case NB1413M3_GALKOKU:
		case NB1413M3_HYOUBAN:
		case NB1413M3_GALKAIKA:
		case NB1413M3_MCONTEST:
		case NB1413M3_UCHUUAI:
		case NB1413M3_TOKIMBSJ:
		case NB1413M3_TOKYOGAL:
			return ((readinputport(0) & 0x0f) | ((readinputport(1) & 0x0f) << 4));
			break;
		case NB1413M3_TRIPLEW1:
		case NB1413M3_NTOPSTAR:
		case NB1413M3_PSTADIUM:
		case NB1413M3_TRIPLEW2:
		case NB1413M3_VANILLA:
		case NB1413M3_FINALBNY:
		case NB1413M3_MJLSTORY:
		case NB1413M3_QMHAYAKU:
		case NB1413M3_MJGOTTUB:
			return (((readinputport(1) & 0x01) >> 0) | ((readinputport(1) & 0x04) >> 1) |
			        ((readinputport(1) & 0x10) >> 2) | ((readinputport(1) & 0x40) >> 3) |
			        ((readinputport(0) & 0x01) << 4) | ((readinputport(0) & 0x04) << 3) |
			        ((readinputport(0) & 0x10) << 2) | ((readinputport(0) & 0x40) << 1));
			break;
		default:
			return readinputport(0);
			break;
	}
}

READ8_HANDLER( nb1413m3_dipsw2_r )
{
	switch (nb1413m3_type)
	{
		case NB1413M3_KANATUEN:
		case NB1413M3_KYUHITO:
			return readinputport(0);
			break;
		case NB1413M3_TAIWANMB:
			return (((readinputport(0) & 0x0f) << 4) | (readinputport(1) & 0x0f));
			break;
		case NB1413M3_OTONANO:
		case NB1413M3_MJCAMERA:
		case NB1413M3_IDHIMITU:
		case NB1413M3_KAGUYA2:
			return ((readinputport(0) & 0xf0) | ((readinputport(1) & 0xf0) >> 4));
			break;
		case NB1413M3_SCANDAL:
		case NB1413M3_SCANDALM:
		case NB1413M3_MJFOCUSM:
		case NB1413M3_GALKOKU:
		case NB1413M3_HYOUBAN:
		case NB1413M3_GALKAIKA:
		case NB1413M3_MCONTEST:
		case NB1413M3_UCHUUAI:
		case NB1413M3_TOKIMBSJ:
		case NB1413M3_TOKYOGAL:
			return (((readinputport(0) & 0xf0) >> 4) | (readinputport(1) & 0xf0));
			break;
		case NB1413M3_TRIPLEW1:
		case NB1413M3_NTOPSTAR:
		case NB1413M3_PSTADIUM:
		case NB1413M3_TRIPLEW2:
		case NB1413M3_VANILLA:
		case NB1413M3_FINALBNY:
		case NB1413M3_MJLSTORY:
		case NB1413M3_QMHAYAKU:
		case NB1413M3_MJGOTTUB:
			return (((readinputport(1) & 0x02) >> 1) | ((readinputport(1) & 0x08) >> 2) |
			        ((readinputport(1) & 0x20) >> 3) | ((readinputport(1) & 0x80) >> 4) |
			        ((readinputport(0) & 0x02) << 3) | ((readinputport(0) & 0x08) << 2) |
			        ((readinputport(0) & 0x20) << 1) | ((readinputport(0) & 0x80) << 0));
			break;
		default:
			return readinputport(1);
			break;
	}
}

READ8_HANDLER( nb1413m3_dipsw3_l_r )
{
	return ((readinputport(2) & 0xf0) >> 4);
}

READ8_HANDLER( nb1413m3_dipsw3_h_r )
{
	return ((readinputport(2) & 0x0f) >> 0);
}

WRITE8_HANDLER( nb1413m3_outcoin_w )
{
	static int counter = 0;

	nb1413m3_outcoin_enable = (data & 0x04) >> 2;

	switch (nb1413m3_type)
	{
		case NB1413M3_TAIWANMB:
		case NB1413M3_IEMOTOM:
		case NB1413M3_OJOUSANM:
		case NB1413M3_SEIHAM:
		case NB1413M3_RYUUHA:
		case NB1413M3_KORINAIM:
		case NB1413M3_MMSIKAKU:
		case NB1413M3_HYOUBAN:
		case NB1413M3_TOKIMBSJ:
		case NB1413M3_MJFOCUSM:
		case NB1413M3_SCANDALM:
		case NB1413M3_BANANADR:
		case NB1413M3_MGION:
		case NB1413M3_HANAOJI:
		case NB1413M3_FINALBNY:
		case NB1413M3_LOVEHOUS:
		case NB1413M3_MMAIKO:
			if (nb1413m3_outcoin_enable)
			{
				if (counter++ == 2)
				{
					nb1413m3_outcoin_flag ^= 1;
					counter = 0;
				}
			}
			break;
		default:
			break;
	}

	set_led_status(2, nb1413m3_outcoin_flag);		// out coin
}

WRITE8_HANDLER( nb1413m3_vcrctrl_w )
{
	if (data & 0x08)
	{
		ui_popup(" ** VCR CONTROL ** ");
		set_led_status(2, 1);
	}
	else
	{
		set_led_status(2, 0);
	}
}
