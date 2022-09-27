////////////////////////////////////////////////////////////////////////////
//                                                                        //
// vacfdisp.c: Vacuum Fluorescence Display                                //
// display emulation for BFM machines, amongst others                     //
//                                                                        //
// emulated displays: BFM BD1 vfd display, OKI MSC1937                    //
//                                                                        //
// 05-03-2004: Re-Animator                                                //
//                                                                        //
// TODO: - BFM sys85 seems to use a slightly different vfd to OKI MSC1937 //
//                                                                        //
//       - Implement display flashing and brightness                      //
//                                                                        //
// Any fixes for this driver should be forwarded to AGEMAME HQ            //
// (http://www.mameworld.net/agemame/)                                    //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#include "driver.h"
#include "vacfdisp.h"


static struct
{
	UINT8	type,				// type of alpha display
								// VFDTYPE_BFMBD1 or VFDTYPE_MSC1937

			changed,			// flag <>0, if vfd contents are changed
			window_start,		// display window start pos 0-15
			window_end,			// display window end   pos 0-15
			window_size,		// window  size
			pad;				// unused align byte

	INT8	pcursor_pos,		// previous cursor pos
			cursor_pos;			// current cursor pos

	UINT16  brightness;			// display brightness level 0-31 (31=MAX)

	UINT16  user_def,			// user defined character state
			user_data;			// user defined character data (16 bit)

	UINT8	scroll_active,		// flag <>0, scrolling active
			display_mode,		// display scroll   mode, 0/1/2/3
			display_blanking,	// display blanking mode, 0/1/2/3
			flash_rate,			// flash rate 0-F
			flash_control;		// flash control 0/1/2/3

	UINT8	string[18];		// text buffer
	UINT16  segments[16];		// segments

	UINT8	count,			// bit counter
			data;				// receive register

} vfds[MAX_VFDS];


// local prototypes ///////////////////////////////////////////////////////

static void ScrollLeft( int id);
static int  BD1_setdata(int id, int segdata, int data);

// local vars /////////////////////////////////////////////////////////////

//
// Bellfruit BD1 charset to ASCII conversion table
//
static const char BFM2ASCII[] =
//0123456789ABCDEF0123456789ABC DEF01 23456789ABCDEF0123456789ABCDEF
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?";

//
// OKI MSC1937 charset to ASCII conversion table
//
static const char OKI1937ASCII[]=
//0123456789ABCDEF0123456789ABC DEF01 23456789ABCDEF0123456789ABCDEF
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?"\
 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ ?\"#$%%'()*+.-./0123456789&%<=>?";

/*
   16 segment charset lookup table
        2
    ---------
   |\   |3  /|
 0 | \6 |  /7| 1
   |  \ | /  |
    -F-- --E-
   |  / | \  |
 D | /4 |A \B| 5
   |/   |   \|
    ---------  C
        9

  */

static unsigned short BFMcharset[]=
{         // FEDC BA98 7654 3210
	0xA626, // 1010 0110 0010 0110 @.
	0xE027, // 1110 0000 0010 0111 A.
	0x462E, // 0100 0110 0010 1110 B.
	0x2205, // 0010 0010 0000 0101 C.
	0x062E, // 0000 0110 0010 1110 D.
	0xA205, // 1010 0010 0000 0101 E.
	0xA005, // 1010 0000 0000 0101 F.
	0x6225, // 0110 0010 0010 0101 G.
	0xE023, // 1110 0000 0010 0011 H.
	0x060C, // 0000 0110 0000 1100 I.
	0x2222, // 0010 0010 0010 0010 J.
	0xA881, // 1010 1000 1000 0001 K.
	0x2201, // 0010 0010 0000 0001 L.
	0x20E3, // 0010 0000 1110 0011 M.
	0x2863, // 0010 1000 0110 0011 N.
	0x2227, // 0010 0010 0010 0111 O.
	0xE007, // 1110 0000 0000 0111 P.
	0x2A27, // 0010 1010 0010 0111 Q.
	0xE807, // 1110 1000 0000 0111 R.
	0xC225, // 1100 0010 0010 0101 S.
	0x040C, // 0000 0100 0000 1100 T.
	0x2223, // 0010 0010 0010 0011 U.
	0x2091, // 0010 0000 1001 0001 V.
	0x2833, // 0010 1000 0011 0011 W.
	0x08D0, // 0000 1000 1101 0000 X.
	0x04C0, // 0000 0100 1100 0000 Y.
	0x0294, // 0000 0010 1001 0100 Z.
	0x2205, // 0010 0010 0000 0101 [.
	0x0840, // 0000 1000 0100 0000 \.
	0x0226, // 0000 0010 0010 0110 ].
	0x0810, // 0000 1000 0001 0000 ^.
	0x0200, // 0000 0010 0000 0000 _
	0x0000, // 0000 0000 0000 0000
	0xC290, // 1100 0010 1001 0000 POUND.
	0x0009, // 0000 0000 0000 1001 ".
	0xC62A, // 1100 0110 0010 1010 #.
	0xC62D, // 1100 0110 0010 1101 $.
	0x0100, // 0000 0000 0000 0000 flash ?
	0x0000, // 0000 0000 0000 0000 not defined
	0x0040, // 0000 0000 1000 0000 '.
	0x0880, // 0000 1000 1000 0000 (.
	0x0050, // 0000 0000 0101 0000 ).
	0xCCD8, // 1100 1100 1101 1000 *.
	0xC408, // 1100 0100 0000 1000 +.
	0x1000, // 0001 0000 0000 0000 .
	0xC000, // 1100 0000 0000 0000 -.
	0x1000, // 0001 0000 0000 0000 ..
	0x0090, // 0000 0000 1001 0000 /.
	0x22B7, // 0010 0010 1011 0111 0.
	0x0408, // 0000 0100 0000 1000 1.
	0xE206, // 1110 0010 0000 0110 2.
	0x4226, // 0100 0010 0010 0110 3.
	0xC023, // 1100 0000 0010 0011 4.
	0xC225, // 1100 0010 0010 0101 5.
	0xE225, // 1110 0010 0010 0101 6.
	0x0026, // 0000 0000 0010 0110 7.
	0xE227, // 1110 0010 0010 0111 8.
	0xC227, // 1100 0010 0010 0111 9.
	0xFFFF, // 0000 0000 0000 0000 user defined, can be replaced by main program
	0x0000, // 0000 0000 0000 0000 dummy
	0x0290, // 0000 0010 1001 0000 <.
	0xC200, // 1100 0010 0000 0000 =.
	0x0A40, // 0000 1010 0100 0000 >.
	0x4406, // 0100 0100 0000 0110 ?
};

static const int poslut1937[]=
{
	14,
	13,
	12,
	11,
	10,
	9,
	8,
	7,
	6,
	5,
	4,
	3,
	2,
	1,
	0,
	15
};
///////////////////////////////////////////////////////////////////////////

void vfd_init(int id, int type )
{
	memset( &vfds[id], 0, sizeof(vfds[0]));

	vfds[id].type = type;

	vfd_reset(id);
}

///////////////////////////////////////////////////////////////////////////

void vfd_reset(int id)
{
	if ( id > 2 || id < 0 ) return;

	vfds[id].window_end  = 15;
	vfds[id].window_size = (vfds[id].window_end - vfds[id].window_start)+1;
	memset(vfds[id].string, ' ', 16);

	vfds[id].brightness = 31;
	vfds[id].count      = 0;

	vfds[id].changed |= 1;
}

///////////////////////////////////////////////////////////////////////////

UINT16 *vfd_get_segments(int id)
{
	return vfds[id].segments;
}

///////////////////////////////////////////////////////////////////////////

char  *vfd_get_string( int id)
{
	return (char *)vfds[id].string;
}
///////////////////////////////////////////////////////////////////////////

void vfd_shift_data(int id, int data)
{
	if ( id > 2 || id < 0 ) return;

	vfds[id].data <<= 1;

	if ( !data ) vfds[id].data |= 1;

	if ( ++vfds[id].count >= 8 )
	{
		if ( vfd_newdata(id, vfds[id].data) )
		{
			vfds[id].changed |= 1;
		}
	//logerror("vfd %3d -> %02X \"%s\"\n", id, vfds[id].data, vfds[id].string);

	vfds[id].count = 0;
	vfds[id].data  = 0;
  }
}

///////////////////////////////////////////////////////////////////////////

int vfd_newdata(int id, int data)
{
	int change = 0;
	int cursor;

	switch ( vfds[id].type )
	{
		case VFDTYPE_BFMBD1:

		if ( vfds[id].user_def )
		{
			vfds[id].user_def--;

			vfds[id].user_data <<= 8;
			vfds[id].user_data |= data;

			if ( vfds[id].user_def )
			{
				return 0;
			}

		data = '@';
		change = BD1_setdata(id, vfds[id].user_def, data);
		}
		else
		{
		}

		switch ( data & 0xF0 )
		{
			case 0x80:	// 0x80 - 0x8F Set display blanking

			vfds[id].display_blanking = data & 0x0F;
			change = 1;
			break;

			case 0x90:	// 0x90 - 0x9F Set cursor pos

			vfds[id].cursor_pos = data & 0x0F;

			vfds[id].scroll_active = 0;
			if ( vfds[id].display_mode == 2 )
			{
				if ( vfds[id].cursor_pos >= vfds[id].window_end) vfds[id].scroll_active = 1;
			}
			break;

			case 0xA0:	// 0xA0 - 0xAF Set display mode

			vfds[id].display_mode = data &0x03;
			break;

			case 0xB0:	// 0xB0 - 0xBF Clear display area

			switch ( data & 0x03 )
			{
				case 0x00:	// clr nothing
				break;

				case 0x01:	// clr inside window

				if ( vfds[id].window_size > 0 )
					{
						memset( vfds[id].string+vfds[id].window_start, ' ',vfds[id].window_size );
					}
					break;

				case 0x02:	// clr outside window

				if ( vfds[id].window_size > 0 )
					{
						if ( vfds[id].window_start > 0 )
						{
							memset( vfds[id].string, ' ', vfds[id].window_start);
							for (cursor = 0; cursor < vfds[id].window_start; cursor++)
							{
								vfds[id].segments[cursor] = 0x0000;
							}
						}

						if (vfds[id].window_end < 15 )
						{
							memset( vfds[id].string+vfds[id].window_end, ' ', 15-vfds[id].window_end);
							for (cursor = vfds[id].window_end; cursor < 15-vfds[id].window_end; cursor++)
							{
								vfds[id].segments[cursor] = 0x0000;
							}

						}
					}
				case 0x03:	// clr entire display

				memset(vfds[id].string, ' ' , 16);
				for (cursor = 0; cursor < 16; cursor++)
				{
				vfds[id].segments[cursor] = 0x0000;
				}
				break;
			}
			change = 1;
			break;

			case 0xC0:	// 0xC0 - 0xCF Set flash rate

			vfds[id].flash_rate = data & 0x0F;
			break;

			case 0xD0:	// 0xD0 - 0xDF Set Flash control

			vfds[id].flash_control = data & 0x03;
			break;

			case 0xE0:	// 0xE0 - 0xEF Set window start pos

			vfds[id].window_start = data &0x0F;
			vfds[id].window_size  = (vfds[id].window_end - vfds[id].window_start)+1;
			break;

			case 0xF0:	// 0xF0 - 0xFF Set window end pos

			vfds[id].window_end   = data &0x0F;
			vfds[id].window_size  = (vfds[id].window_end - vfds[id].window_start)+1;

			vfds[id].scroll_active = 0;
			if ( vfds[id].display_mode == 2 )
			{
				if ( vfds[id].cursor_pos >= vfds[id].window_end)
				{
					vfds[id].scroll_active = 1;
					vfds[id].cursor_pos    = vfds[id].window_end;
				}
			}
			break;

			default:	// normal character

			change = BD1_setdata(id, BFMcharset[data & 0x3F], data);
			break;
		}
		break;

		case VFDTYPE_MSC1937:

		if ( data & 0x80 )
		{ // Control data received
			if ( (data & 0xF0) == 0xA0 ) // 1010 xxxx
			{ // 1010 xxxx Buffer Pointer control
				vfds[id].cursor_pos = poslut1937[data & 0x0F];
				logerror("CUR%d\n", vfds[id].cursor_pos);
			}
			else if ( (data & 0xF0) == 0xC0 ) // 1100 xxxx
				{ // 1100 xxxx Set number of digits
					data &= 0x07;

					if ( data == 0 ) vfds[id].window_size = 16;
					else             vfds[id].window_size = data+8;
					vfds[id].window_start = 0;
					vfds[id].window_end   = vfds[id].window_size-1;
				}
			else if ( (data & 0xE0) == 0xE0 ) // 111x xxxx
				{ // 111x xxxx Set duty cycle ( brightness )
					vfds[id].brightness = (data & 0xF)*2;
					change = 1;
				}
			else if ( (data & 0xE0) == 0x80 ) // 100x ---
				{ // 100x xxxx Test mode
				}
		}
		else
		{ // Display data
			data &= 0x3F;
			change = 1;

			switch ( data )
			{
				case 0x2C: // ;
				case 0x2E: //

				vfds[id].segments[vfds[id].pcursor_pos] |= (1<<12);
				break;
				default :
				vfds[id].pcursor_pos = vfds[id].cursor_pos;
				vfds[id].string[ vfds[id].cursor_pos ] = OKI1937ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = BFMcharset[data & 0x3F];

				vfds[id].cursor_pos++;
				if (  vfds[id].cursor_pos > vfds[id].window_end )
					{
						vfds[id].cursor_pos = 0;
					}
			}
		}
		break;
	}

	return change;
}

///////////////////////////////////////////////////////////////////////////

static void ScrollLeft(int id)
{
	int i = vfds[id].window_start;

	while ( i < vfds[id].window_end )
	{
		vfds[id].string[ i ] = vfds[id].string[ i+1 ];
		vfds[id].segments[i] = vfds[id].segments[i+1];
		i++;
	}
}

///////////////////////////////////////////////////////////////////////////

static int BD1_setdata(int id, int segdata, int data)
{
	int change = 0, move = 0;

	switch ( data )
	{
		case 0x25:	// flash
		move++;
		break;

		case 0x26:  // undefined
		break;

		case 0x2C:  // semicolon
		case 0x2E:  // decimal point

		vfds[id].segments[vfds[id].pcursor_pos] |= (1<<12);
		change++;
		break;

		case 0x3B:	// dummy char
		move++;
		break;

		case 0x3A:
		vfds[id].user_def = 2;
		break;

		default:
		move   = 1;
		change = 1;
	}

	if ( move )
	{
		int mode = vfds[id].display_mode;

		vfds[id].pcursor_pos = vfds[id].cursor_pos;

		if ( vfds[id].window_size <= 0 || (vfds[id].window_size > 16))
			{ // no window selected default to rotate mode
	  			if ( mode == 2 )      mode = 0;
				else if ( mode == 3 ) mode = 1;
				//mode &= -2;
	}

	switch ( mode )
	{
		case 0: // rotate left

		vfds[id].cursor_pos &= 0x0F;

		if ( change )
		{
			vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
			vfds[id].segments[vfds[id].cursor_pos] = segdata;
		}
		vfds[id].cursor_pos++;
		if ( vfds[id].cursor_pos >= 16 ) vfds[id].cursor_pos = 0;
		break;


		case 1: // Rotate right

		vfds[id].cursor_pos &= 0x0F;

		if ( change )
		{
			vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
			vfds[id].segments[vfds[id].cursor_pos] = segdata;
		}
		vfds[id].cursor_pos--;
		if ( vfds[id].cursor_pos < 0  ) vfds[id].cursor_pos = 15;
		break;

		case 2: // Scroll left

		if ( vfds[id].cursor_pos < vfds[id].window_end )
		{
			vfds[id].scroll_active = 0;
			if ( change )
			{
				vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = segdata;
			}
			if ( move ) vfds[id].cursor_pos++;
		}
		else
		{
			if ( move )
			{
				if   ( vfds[id].scroll_active ) ScrollLeft(id);
				else                            vfds[id].scroll_active = 1;
			}

			if ( change )
			{
				vfds[id].string[vfds[id].window_end]   = BFM2ASCII[data];
				vfds[id].segments[vfds[id].cursor_pos] = segdata;
		  	}
			else
			{
				vfds[id].string[vfds[id].window_end]   = ' ';
				vfds[id].segments[vfds[id].cursor_pos] = 0;
			}
		}
		break;

		case 3: // Scroll right

		if ( vfds[id].cursor_pos > vfds[id].window_start )
			{
				if ( change )
				{
					vfds[id].string[vfds[id].cursor_pos]   = BFM2ASCII[data];
					vfds[id].segments[vfds[id].cursor_pos] = segdata;
				}
				vfds[id].cursor_pos--;
			}
			else
			{
				int i = vfds[id].window_end;

				while ( i > vfds[id].window_start )
				{
					vfds[id].string[ i ] = vfds[id].string[ i-1 ];
					vfds[id].segments[i] = vfds[id].segments[i-1];
					i--;
				}

				if ( change )
				{
					vfds[id].string[vfds[id].window_start]   = BFM2ASCII[data];
					vfds[id].segments[vfds[id].window_start] = segdata;
				}
			}
			break;
		}
	}
	return change;
}

void draw_16seg(mame_bitmap *bitmap,int x,int y,int vfd, int col_on, int col_off )
{
	int cursor;
	int i;

	for (cursor = 0; cursor < 16; cursor++)
	{
		plot_box(bitmap, x+(7*cursor), y+1, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x0001) ? col_on : col_off);//0
		plot_box(bitmap, x+4+(7*cursor), y+1, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x0002) ? col_on : col_off);//1
		plot_box(bitmap, x+(7*cursor), y, 4, 1, (vfd_get_segments(vfd)[cursor] & 0x0004) ? col_on : col_off);//2
		plot_box(bitmap, x+2+(7*cursor), y+1, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x0008) ? col_on : col_off);//3
		plot_box(bitmap, x+4+(7*cursor), y+10.5, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x0020) ? col_on : col_off);//5

		for (i = 0; i < 11; i++)
		{
			plot_box(bitmap, x+0+(i/5)+(7*cursor), y+20-(i*0.95), 1, 1, (vfd_get_segments(vfd)[cursor] & 0x0010) ? col_on : col_off);//4
			plot_box(bitmap, x+2-(i/5)+(7*cursor), y+10.5-(i*0.95), 1, 1, (vfd_get_segments(vfd)[cursor] & 0x0040) ? col_on : col_off);//6
			plot_box(bitmap, x+2+(i/5)+(7*cursor), y+10.5-(i*0.95), 1, 1, (vfd_get_segments(vfd)[cursor] & 0x0080) ? col_on : col_off);//7
			plot_box(bitmap, x+4-(i/5)+(7*cursor), y+20-(i*0.95), 1, 1, (vfd_get_segments(vfd)[cursor] & 0x0800) ? col_on : col_off);//11
		}

		plot_box(bitmap, x+(7*cursor), y+20, 4, 1, (vfd_get_segments(vfd)[cursor] & 0x0200) ? col_on : col_off);//8
		plot_box(bitmap, x+2+(7*cursor), y+11.5, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x0400) ? col_on : col_off);//10
		plot_box(bitmap, x+5+(7*cursor), y+18, 1, 3, (vfd_get_segments(vfd)[cursor] & 0x1000) ? col_on : col_off);//12
		plot_box(bitmap, x+6+(7*cursor), y+19, 1, 3, (vfd_get_segments(vfd)[cursor] & 0x1000) ? col_on : col_off);//12
		plot_box(bitmap, x+7+(7*cursor), y+22, 1, 1, (vfd_get_segments(vfd)[cursor] & 0x1000) ? col_on : col_off);//12
		plot_box(bitmap, x+(7*cursor), y+10.5, 1, 9.5, (vfd_get_segments(vfd)[cursor] & 0x2000) ? col_on : col_off);//13

		plot_box(bitmap, x+2+(7*cursor), y+10.5, 2, 1, (vfd_get_segments(vfd)[cursor] & 0x4000) ? col_on : col_off);//14
		plot_box(bitmap, x+(7*cursor), y+10.5, 2, 1, (vfd_get_segments(vfd)[cursor] & 0x8000) ? col_on : col_off);//15

		if (vfd_get_segments(vfd)[cursor] & 0x0100)
				{
					//activate flashing (unimplemented)
				}
			else
				{
					//deactivate flashing (unimplemented)
				}

	}
}
