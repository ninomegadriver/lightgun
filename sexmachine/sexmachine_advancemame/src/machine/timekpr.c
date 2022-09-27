/*
 * STmicroelectronics TIMEKEEPER SRAM
 *
 * Supports: MK48T08, M48T02 & M48T58
 *
 */

#include <time.h>
#include "driver.h"
#include "machine/timekpr.h"

struct timekeeper_chip
{
	UINT8 control;
	UINT8 seconds;
	UINT8 minutes;
	UINT8 hours;
	UINT8 day;
	UINT8 date;
	UINT8 month;
	UINT8 year;
	UINT8 century;
	UINT8 *data;
	int type;
	int size;
	int offset_control;
	int offset_seconds;
	int offset_minutes;
	int offset_hours;
	int offset_day;
	int offset_date;
	int offset_month;
	int offset_year;
	int offset_century;
	int offset_flags;
};

struct timekeeper_chip timekeeper[ MAX_TIMEKEEPER_CHIPS ];

#define MASK_SECONDS ( 0x7f )
#define MASK_MINUTES ( 0x7f )
#define MASK_HOURS ( 0x3f )
#define MASK_DAY ( 0x07 )
#define MASK_DATE ( 0x3f )
#define MASK_MONTH ( 0x1f )
#define MASK_YEAR ( 0xff )
#define MASK_CENTURY ( 0xff )

#define CONTROL_W ( 0x80 )
#define CONTROL_R ( 0x40 )
#define CONTROL_S ( 0x20 ) /* not emulated */
#define CONTROL_CALIBRATION ( 0x1f ) /* not emulated */

#define SECONDS_ST ( 0x80 )

#define DAY_FT ( 0x40 ) /* not emulated */
#define DAY_CEB ( 0x20 ) /* M48T58 */
#define DAY_CB ( 0x10 ) /* M48T58 */

#define DATE_BLE ( 0x80 ) /* M48T58: not emulated */
#define DATE_BL ( 0x40 ) /* M48T58: not emulated */

#define FLAGS_BL ( 0x10 ) /* MK48T08: not emulated */

INLINE UINT8 make_bcd(UINT8 data)
{
	return ( ( ( data / 10 ) % 10 ) << 4 ) + ( data % 10 );
}

INLINE UINT8 from_bcd( UINT8 data )
{
	return ( ( ( data >> 4 ) & 15 ) * 10 ) + ( data & 15 );
}

static int inc_bcd( UINT8 *data, int mask, int min, int max )
{
	int bcd;
	int carry;

	bcd = ( *( data ) + 1 ) & mask;
	carry = 0;

	if( ( bcd & 0x0f ) > 9 )
	{
		bcd &= 0xf0;
		bcd += 0x10;
		if( bcd > max )
		{
			bcd = min;
			carry = 1;
		}
	}

	*( data ) = ( *( data ) & ~mask ) | ( bcd & mask );
	return carry;
}

static void counter_to_ram( UINT8 *data, int offset, int counter )
{
	if( offset >= 0 )
	{
		data[ offset ] = counter;
	}
}

static void counters_to_ram( int chip )
{
	struct timekeeper_chip *c = &timekeeper[ chip ];

	counter_to_ram( c->data, c->offset_control, c->control );
	counter_to_ram( c->data, c->offset_seconds, c->seconds );
	counter_to_ram( c->data, c->offset_minutes, c->minutes );
	counter_to_ram( c->data, c->offset_hours, c->hours );
	counter_to_ram( c->data, c->offset_day, c->day );
	counter_to_ram( c->data, c->offset_date, c->date );
	counter_to_ram( c->data, c->offset_month, c->month );
	counter_to_ram( c->data, c->offset_year, c->year );
	counter_to_ram( c->data, c->offset_century, c->century );
}

static int counter_from_ram( UINT8 *data, int offset )
{
	if( offset >= 0 )
	{
		return data[ offset ];
	}
	return 0;
}

static void counters_from_ram( int chip )
{
	struct timekeeper_chip *c = &timekeeper[ chip ];

	c->control = counter_from_ram( c->data, c->offset_control );
	c->seconds = counter_from_ram( c->data, c->offset_seconds );
	c->minutes = counter_from_ram( c->data, c->offset_minutes );
	c->hours = counter_from_ram( c->data, c->offset_hours );
	c->day = counter_from_ram( c->data, c->offset_day );
	c->date = counter_from_ram( c->data, c->offset_date );
	c->month = counter_from_ram( c->data, c->offset_month );
	c->year = counter_from_ram( c->data, c->offset_year );
	c->century = counter_from_ram( c->data, c->offset_century );
}

static void timekeeper_tick( int chip )
{
	struct timekeeper_chip *c = &timekeeper[ chip ];

	int carry;

	if( ( c->seconds & SECONDS_ST ) != 0 ||
		( c->control & CONTROL_W ) != 0 )
	{
		return;
	}

	carry = inc_bcd( &c->seconds, MASK_SECONDS, 0x00, 0x59 );
	if( carry )
	{
		carry = inc_bcd( &c->minutes, MASK_MINUTES, 0x00, 0x59 );
	}
	if( carry )
	{
		carry = inc_bcd( &c->hours, MASK_HOURS, 0x00, 0x23 );
	}

	if( carry )
	{
		UINT8 month;
		UINT8 year;
		UINT8 maxdays;
		static const UINT8 daysinmonth[] = { 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31 };

		inc_bcd( &c->day, MASK_DAY, 0x01, 0x07 );

		month = from_bcd( c->month );
		year = from_bcd( c->year );

		if( month == 2 && ( year % 4 ) == 0 )
		{
			maxdays = 0x29;
		}
		else if( month >= 1 && month <= 12 )
		{
			maxdays = daysinmonth[ month - 1 ];
		}
		else
		{
			maxdays = 0x31;
		}

		carry = inc_bcd( &c->date, MASK_DATE, 0x01, maxdays );
	}
	if( carry )
	{
		carry = inc_bcd( &c->month, MASK_MONTH, 0x01, 0x12 );
	}
	if( carry )
	{
		carry = inc_bcd( &c->year, MASK_YEAR, 0x00, 0x99 );
	}
	if( carry )
	{
		carry = inc_bcd( &c->century, MASK_CENTURY, 0x00, 0x99 );
		if( c->type == TIMEKEEPER_M48T58 && ( c->day & DAY_CEB ) != 0 )
		{
			c->day ^= DAY_CB;
		}
	}

	if( ( c->control & CONTROL_R ) == 0 )
	{
		counters_to_ram( chip );
	}
}

void timekeeper_init( int chip, int type, UINT8 *data )
{
	time_t currenttime;
	mame_timer *timer;
	mame_time duration;
	struct tm *mytime;
	struct timekeeper_chip *c;

	if( chip >= MAX_TIMEKEEPER_CHIPS )
	{
		logerror( "timekeeper_init( %d ) invalid chip\n", chip );
		return;
	}
	c = &timekeeper[ chip ];

	c->type = type;

	switch( c->type )
	{
	case TIMEKEEPER_M48T02:
		c->offset_control = 0x7f8;
		c->offset_seconds = 0x7f9;
		c->offset_minutes = 0x7fa;
		c->offset_hours = 0x7fb;
		c->offset_day = 0x7fc;
		c->offset_date = 0x7fd;
		c->offset_month = 0x7fe;
		c->offset_year = 0x7ff;
		c->offset_century = -1;
		c->offset_flags = -1;
		c->size = 0x800;
		break;
	case TIMEKEEPER_M48T58:
		c->offset_control = 0x1ff8;
		c->offset_seconds = 0x1ff9;
		c->offset_minutes = 0x1ffa;
		c->offset_hours = 0x1ffb;
		c->offset_day = 0x1ffc;
		c->offset_date = 0x1ffd;
		c->offset_month = 0x1ffe;
		c->offset_year = 0x1fff;
		c->offset_century = -1;
		c->offset_flags = -1;
		c->size = 0x2000;
		break;
	case TIMEKEEPER_MK48T08:
		c->offset_control = 0x1ff8;
		c->offset_seconds = 0x1ff9;
		c->offset_minutes = 0x1ffa;
		c->offset_hours = 0x1ffb;
		c->offset_day = 0x1ffc;
		c->offset_date = 0x1ffd;
		c->offset_month = 0x1ffe;
		c->offset_year = 0x1fff;
		c->offset_century = 0x1ff1;
		c->offset_flags = 0x1ff0;
		c->size = 0x2000;
		break;
	}

	if( data == NULL )
	{
		data = auto_malloc( c->size );
		memset( data, 0xff, c->size );
	}
	c->data = data;

	time(&currenttime);
	mytime = localtime(&currenttime);

	c->control = 0;
	c->seconds = make_bcd( mytime->tm_sec );
	c->minutes = make_bcd( mytime->tm_min );
	c->hours = make_bcd( mytime->tm_hour );
	c->day = make_bcd( mytime->tm_wday + 1 );
	c->date = make_bcd( mytime->tm_mday );
	c->month = make_bcd( mytime->tm_mon + 1 );
	c->year = make_bcd( ( 1900 + mytime->tm_year ) % 100 );
	c->century = make_bcd( ( 1900 + mytime->tm_year ) / 100 );

	state_save_register_item( "timekeeper", chip, c->control );
	state_save_register_item( "timekeeper", chip, c->seconds );
	state_save_register_item( "timekeeper", chip, c->minutes );
	state_save_register_item( "timekeeper", chip, c->hours );
	state_save_register_item( "timekeeper", chip, c->day );
	state_save_register_item( "timekeeper", chip, c->date );
	state_save_register_item( "timekeeper", chip, c->month );
	state_save_register_item( "timekeeper", chip, c->year );
	state_save_register_item( "timekeeper", chip, c->century );
	state_save_register_item_pointer( "timekeeper", chip, c->data, c->size );

	timer = mame_timer_alloc( timekeeper_tick );
	duration = make_mame_time( 1, 0 );
	mame_timer_adjust( timer, duration, chip, duration );
}

static void timekeeper_nvram( int chip, mame_file *file, int read_or_write )
{
	struct timekeeper_chip *c;
	if( chip >= MAX_TIMEKEEPER_CHIPS )
	{
		logerror( "timekeeper_nvram( %d ) invalid chip\n", chip );
		return;
	}
	c = &timekeeper[ chip ];

	if( read_or_write )
	{
		mame_fwrite( file, c->data, c->size );
	}
	else
	{
		if( file )
		{
			mame_fread( file, c->data, c->size );
		}
		counters_to_ram( chip );
	}
}

NVRAM_HANDLER( timekeeper_0 ) { timekeeper_nvram( 0, file, read_or_write ); }

static UINT8 timekeeper_read( UINT32 chip, offs_t offset )
{
	UINT8 data;
	struct timekeeper_chip *c;
	if( chip >= MAX_TIMEKEEPER_CHIPS )
	{
		logerror( "%08x: timekeeper_read( %d, %04x ) chip out of range\n", activecpu_get_pc(), chip, offset );
		return 0;
	}
	c = &timekeeper[ chip ];

	data = c->data[ offset ];
//  logerror( "%08x: timekeeper_read( %d, %04x ) %02x\n", activecpu_get_pc(), chip, offset, data );
	return data;
}

static void timekeeper_write( UINT32 chip, offs_t offset, UINT8 data )
{
	struct timekeeper_chip *c;
	if( chip >= MAX_TIMEKEEPER_CHIPS )
	{
		logerror( "%08x: timekeeper_write( %d, %04x, %02x ) chip out of range\n", activecpu_get_pc(), chip, offset, data );
		return;
	}
	c = &timekeeper[ chip ];

	if( offset == c->offset_control )
	{
		if( ( c->control & CONTROL_W ) != 0 &&
			( data & CONTROL_W ) == 0 )
		{
			counters_from_ram( chip );
		}
		c->control = data;
	}
	else if( c->type == TIMEKEEPER_M48T58 && offset == c->offset_day )
	{
		c->day = ( c->day & ~DAY_CEB ) | ( data & DAY_CEB );
	}
	else if( c->type == TIMEKEEPER_M48T58 && offset == c->offset_date )
	{
		data &= ~DATE_BL;
	}
	else if( c->type == TIMEKEEPER_MK48T08 && offset == c->offset_flags )
	{
		data &= ~FLAGS_BL;
	}

//  logerror( "%08x: timekeeper_write( %d, %04x, %02x )\n", activecpu_get_pc(), chip, offset, data );
	c->data[ offset ] = data;
}

/* 16bit memory handlers */

static UINT16 timekeeper_msb16_read( UINT32 chip, offs_t offset, UINT16 mem_mask )
{
	UINT16 data = 0;
	if( ACCESSING_MSB16 )
	{
		data |= timekeeper_read( chip, offset ) << 8;
	}
	return data;
}

static void timekeeper_msb16_write( UINT32 chip, offs_t offset, UINT16 data, UINT16 mem_mask )
{
	if( ACCESSING_MSB16 )
	{
		timekeeper_write( chip, offset, data >> 8 );
	}
}

READ16_HANDLER( timekeeper_0_msb16_r ) { return timekeeper_msb16_read( 0, offset, mem_mask ); }
WRITE16_HANDLER( timekeeper_0_msb16_w ) { timekeeper_msb16_write( 0, offset, data, mem_mask ); }

/* 32bit memory handlers */

static UINT32 timekeeper_32be_read( UINT32 chip, offs_t offset, UINT32 mem_mask )
{
	UINT32 data = 0;
	if( ACCESSING_MSB32 )
	{
		data |= timekeeper_read( chip, ( offset * 4 ) + 0 ) << 24;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		data |= timekeeper_read( chip, ( offset * 4 ) + 1 ) << 16;
	}
	if( ( mem_mask & 0x0000ff00 ) == 0 )
	{
		data |= timekeeper_read( chip, ( offset * 4 ) + 2 ) << 8;
	}
	if( ACCESSING_LSB32 )
	{
		data |= timekeeper_read( chip, ( offset * 4 ) + 3 ) << 0;
	}
	return data;
}

static void timekeeper_32be_write( UINT32 chip, offs_t offset, UINT32 data, UINT32 mem_mask )
{
	if( ACCESSING_MSB32 )
	{
		timekeeper_write( chip, ( offset * 4 ) + 0, data >> 24 );
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		timekeeper_write( chip, ( offset * 4 ) + 1, data >> 16 );
	}
	if( ( mem_mask & 0x0000ff00 ) == 0 )
	{
		timekeeper_write( chip, ( offset * 4 ) + 2, data >> 8 );
	}
	if( ACCESSING_LSB32 )
	{
		timekeeper_write( chip, ( offset * 4 ) + 3, data >> 0 );
	}
}

READ32_HANDLER( timekeeper_0_32be_r ) { return timekeeper_32be_read( 0, offset, mem_mask ); }
WRITE32_HANDLER( timekeeper_0_32be_w ) { timekeeper_32be_write( 0, offset, data, mem_mask ); }

static UINT32 timekeeper_32le_lsb16_read( UINT32 chip, offs_t offset, UINT32 mem_mask )
{
	UINT32 data = 0;
	if( ACCESSING_LSB32 )
	{
		data |= timekeeper_read( chip, ( offset * 2 ) + 0 ) << 0;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		data |= timekeeper_read( chip, ( offset * 2 ) + 1 ) << 16;
	}
	return data;
}

static void timekeeper_32le_lsb16_write( UINT32 chip, offs_t offset, UINT32 data, UINT32 mem_mask )
{
	if( ACCESSING_LSB32 )
	{
		timekeeper_write( chip, ( offset * 2 ) + 0, data >> 0 );
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		timekeeper_write( chip, ( offset * 2 ) + 1, data >> 16 );
	}
}

READ32_HANDLER( timekeeper_0_32le_lsb16_r ) { return timekeeper_32le_lsb16_read( 0, offset, mem_mask ); }
WRITE32_HANDLER( timekeeper_0_32le_lsb16_w ) { timekeeper_32le_lsb16_write( 0, offset, data, mem_mask ); }
