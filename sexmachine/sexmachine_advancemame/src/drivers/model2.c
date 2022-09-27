/*
    Sega Model 2: i960KB + (5x TGP) or (2x SHARC) or (2x TGPx4)
    System 24 tilemaps
    Custom Sega/Lockheed-Martin rasterization hardware
    (68000 + YM3834 + 2x MultiPCM) or (68000 + SCSP)

    Hardware and protection reverse-engineering and general assistance by ElSemi.
    MAME driver by R. Belmont, Olivier Galibert, and ElSemi.

    OK (controls may be wrong/missing/incomplete)
    --
    daytona/daytonat/daytonam
    desert
    vcop
    vf2
    vcop2
    zerogun
    gunblade
    indy500
    bel
    hotd
    topskatr
    von
    fvipers
    schamp
    stcc
    srallyc
    skytargt
    dynamcop
    dynabb
    lastbrnj/lastbrnx
    pltkids/pltkidsa
    skisuprg

    almost OK
    ---------
    sgt24h: hangs on network test.  You can set it to non-linked in test but it still hangs on the network test.
    overrev: bad network test like sgt24h.
    vstriker: shows some attract mode, then hangs
    manxtt: no escape from "active motion slider" tutorial (needs analog inputs)
    doaa: shows parade of 'ANIME' errors


    TODO
    ----
    Controls are pretty basic right now
    Sound doesn't work properly in all games
    System 24 tilemaps need more advanced linescroll support (see fvipers, daytona)
    DSP cores + hookups
*/

#include "driver.h"
#include "machine/eeprom.h"
#include "vidhrdw/segaic24.h"
#include "cpu/i960/i960.h"
#include "cpu/m68000/m68k.h"
#include "cpu/sharc/sharc.h"
#include "sound/scsp.h"
#include "sound/multipcm.h"
#include "sound/2612intf.h"

UINT32 *model2_bufferram, *model2_colorxlat, *model2_workram, *model2_backup1, *model2_backup2;
static UINT32 model2_intreq;
static UINT32 model2_intena;
static UINT32 model2_coproctl, model2_coprocnt, model2_geoctl, model2_geocnt;

static UINT32 model2_timervals[4], model2_timerorig[4];
static int      model2_timerrun[4];
static void    *model2_timers[4];
static int model2_ctrlmode;



#define COPRO_FIFOIN_SIZE	16
static int copro_fifoin_rpos, copro_fifoin_wpos;
static UINT32 copro_fifoin_data[COPRO_FIFOIN_SIZE];
static int copro_fifoin_num = 0;
static UINT32 copro_fifoin_pop(void)
{
	UINT32 r;
	if (copro_fifoin_wpos == copro_fifoin_rpos)
	{
		//fatalerror("Copro FIFOIN underflow (at %08X)", activecpu_get_pc());
		return 0;
	}
	r = copro_fifoin_data[copro_fifoin_rpos++];

	if (copro_fifoin_rpos == COPRO_FIFOIN_SIZE)
	{
		copro_fifoin_rpos = 0;
	}

	copro_fifoin_num--;
	if (copro_fifoin_num == 0)
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG0, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG0, CLEAR_LINE);
	}

	return r;
}

static void copro_fifoin_push(UINT32 data)
{
	copro_fifoin_data[copro_fifoin_wpos++] = data;
	if (copro_fifoin_wpos == COPRO_FIFOIN_SIZE)
	{
		copro_fifoin_wpos = 0;
	}
	if (copro_fifoin_wpos == copro_fifoin_rpos)
	{
		//fatalerror("Copro FIFOIN overflow (at %08X)", activecpu_get_pc());
		return;
	}

	copro_fifoin_num++;
	// clear FIFO empty flag on SHARC
	cpunum_set_input_line(2, SHARC_INPUT_FLAG0, CLEAR_LINE);
}


#define COPRO_FIFOOUT_SIZE	16
static int copro_fifoout_rpos, copro_fifoout_wpos;
static UINT32 copro_fifoout_data[COPRO_FIFOOUT_SIZE];
static int copro_fifoout_num = 0;
static int copro_fifoout_empty = 1;
static UINT32 copro_fifoout_pop(void)
{
	UINT32 r;
	if (copro_fifoout_wpos == copro_fifoout_rpos)
	{
		//fatalerror("Copro FIFOOUT underflow (at %08X)", activecpu_get_pc());
		return 0;
	}
	r = copro_fifoout_data[copro_fifoout_rpos++];

	if (copro_fifoout_rpos == COPRO_FIFOOUT_SIZE)
	{
		copro_fifoout_rpos = 0;
	}
	copro_fifoout_num--;
	if (copro_fifoout_num == 0)
	{
		copro_fifoout_empty = 1;
	}
	else
	{
		copro_fifoout_empty = 0;
	}

	// set SHARC flag 1: 0 if space available, 1 if FIFO full
	if (copro_fifoout_num == COPRO_FIFOOUT_SIZE)
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
	}

	return r;
}

static void copro_fifoout_push(UINT32 data)
{
	copro_fifoout_data[copro_fifoout_wpos++] = data;
	if (copro_fifoout_wpos == COPRO_FIFOOUT_SIZE)
	{
		copro_fifoout_wpos = 0;
	}
	if (copro_fifoout_wpos == copro_fifoout_rpos)
	{
		//fatalerror("Copro FIFOOUT overflow (at %08X)", activecpu_get_pc());
		return;
	}
	copro_fifoout_num++;
	// clear FIFO empty flag on i960
	copro_fifoout_empty = 0;

	// set SHARC flag 1: 0 if space available, 1 if FIFO full
	if (copro_fifoout_num == COPRO_FIFOOUT_SIZE)
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
	}
}



static NVRAM_HANDLER( model2 )
{
	nvram_handler_93C46(file, read_or_write);

	if (read_or_write)
	{
		mame_fwrite(file, model2_backup1, 0x3fff);
		if (model2_backup2)
			mame_fwrite(file, model2_backup2, 0xff);
	}
	else
	{
		if (file)
		{
			mame_fread(file, model2_backup1, 0x3fff);
			if (model2_backup2)
				mame_fread(file, model2_backup2, 0xff);
		}
	}
}

/* Timers - these count down at 25 MHz and pull IRQ2 when they hit 0 */
static READ32_HANDLER( timers_r )
{
	i960_noburst();

	// if timer is running, calculate current value
	if (model2_timerrun[offset])
	{
		double cur;

		// get elapsed time
		cur = timer_timeelapsed(model2_timers[offset]);

		// convert to units of 25 MHz
		cur /= (1.0 / 25000000.0);

		// subtract units from starting value
		model2_timervals[offset] = model2_timerorig[offset] - cur;
	}

	return model2_timervals[offset];
}

static WRITE32_HANDLER( timers_w )
{
	double time;

	i960_noburst();
	COMBINE_DATA(&model2_timervals[offset]);

	model2_timerorig[offset] = model2_timervals[offset];
	time = 25000000.0 / (double)model2_timerorig[offset];
	timer_adjust(model2_timers[offset], TIME_IN_HZ(time), 0, 0);
	model2_timerrun[offset] = 1;
}

static void model2_timer_exp(int tnum, int bit)
{
	timer_adjust(model2_timers[tnum], TIME_NEVER, 0, 0);

	model2_intreq |= (1<<bit);
	if (model2_intena & (1<<bit))
	{
		cpunum_set_input_line(0, I960_IRQ2, ASSERT_LINE);
	}

	model2_timervals[tnum] = 0;
	model2_timerrun[tnum] = 0;
}

static void model2_timer_0_cb(int num) { model2_timer_exp(0, 2); }
static void model2_timer_1_cb(int num) { model2_timer_exp(1, 3); }
static void model2_timer_2_cb(int num) { model2_timer_exp(2, 4); }
static void model2_timer_3_cb(int num) { model2_timer_exp(3, 5); }

static MACHINE_RESET(model2o)
{
	model2_intreq = 0;
	model2_intena = 0;
	model2_coproctl = 0;
	model2_coprocnt = 0;
	model2_geoctl = 0;
	model2_geocnt = 0;
	model2_ctrlmode = 0;

	model2_timervals[0] = 0xfffff;
	model2_timervals[1] = 0xfffff;
	model2_timervals[2] = 0xfffff;
	model2_timervals[3] = 0xfffff;

	model2_timerrun[0] = model2_timerrun[1] = model2_timerrun[2] = model2_timerrun[3] = 0;

	model2_timers[0] = timer_alloc(model2_timer_0_cb);
	model2_timers[1] = timer_alloc(model2_timer_1_cb);
	model2_timers[2] = timer_alloc(model2_timer_2_cb);
	model2_timers[3] = timer_alloc(model2_timer_3_cb);

	timer_adjust(model2_timers[0], TIME_NEVER, 0, 0);
	timer_adjust(model2_timers[1], TIME_NEVER, 0, 0);
	timer_adjust(model2_timers[2], TIME_NEVER, 0, 0);
	timer_adjust(model2_timers[3], TIME_NEVER, 0, 0);
}

static MACHINE_RESET(model2)
{
	machine_reset_model2o();

	memory_set_bankptr(4, memory_region(REGION_SOUND1) + 0x200000);
	memory_set_bankptr(5, memory_region(REGION_SOUND1) + 0x600000);

	// copy the 68k vector table into RAM
	memcpy(memory_region(REGION_CPU2), memory_region(REGION_CPU2)+0x80000, 16);
}

static MACHINE_RESET(model2b)
{
	machine_reset_model2();

	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);

	// set FIFOIN empty flag on SHARC
	cpunum_set_input_line(2, SHARC_INPUT_FLAG0, ASSERT_LINE);
	// clear FIFOOUT buffer full flag on SHARC
	cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);

	// set FIFO empty flag on i960
	copro_fifoout_empty = 1;
}

static VIDEO_START(model2)
{
	if(sys24_tile_vh_start(0x3fff))
		return 1;
	return 0;
}

static VIDEO_UPDATE(model2)
{
	sys24_tile_update();
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);

	sys24_tile_draw(bitmap, cliprect, 7, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 6, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 5, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 4, 0, 0);

	sys24_tile_draw(bitmap, cliprect, 3, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 2, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 1, 0, 0);
	sys24_tile_draw(bitmap, cliprect, 0, 0, 0);
}

static void chcolor(pen_t color, UINT16 data)
{
	int r,g,b;

	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

static WRITE32_HANDLER(pal32_w)
{
	COMBINE_DATA(paletteram32+offset);
	if(ACCESSING_LSW32)
		chcolor(offset*2, paletteram32[offset]);
	if(ACCESSING_MSW32)
		chcolor(offset*2+1, paletteram32[offset]>>16);
}

static WRITE32_HANDLER(ctrl0_w)
{
	if(!(mem_mask & 0x000000ff)) {
		model2_ctrlmode = data & 0x01;
		EEPROM_write_bit(data & 0x20);
		EEPROM_set_clock_line((data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_set_cs_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static READ32_HANDLER(ctrl0_r)
{
	UINT32 ret = readinputport(0);
	ret <<= 16;
	if(model2_ctrlmode==0)
	{
		return ret;
	}
	else
	{
		ret &= ~0x00300000;
		return ret | 0x00100000 | (EEPROM_read_bit() << 21);
	}
}
static READ32_HANDLER(ctrl1_r)
{
	return readinputport(1) | readinputport(2)<<16;
}

static READ32_HANDLER(ctrl10_r)
{
	return readinputport(0) | readinputport(1)<<16;
}

static READ32_HANDLER(ctrl14_r)
{
	return readinputport(2);
}

static READ32_HANDLER(analog_r)
{
	if (offset)
		return readinputport(5);

	return readinputport(3) | readinputport(4)<<16;
}

static READ32_HANDLER(fifoctl_r)
{
	// #### 1 if fifo empty, zerogun needs | 0x04 set
	return copro_fifoout_empty | 0x04;
}

static READ32_HANDLER(videoctl_r)
{
	return (cpu_getcurrentframe() & 1)<<2;
}

static READ32_HANDLER(geo_prg_r)
{
	return 0xffffffff;
}

static WRITE32_HANDLER(geo_prg_w)
{
	if (model2_geoctl & 0x80000000)
	{
		logerror("geo_prg_w: %08X:   %08X\n", model2_geocnt, data);
		model2_geocnt++;
	}
}

static READ32_HANDLER(copro_prg_r)
{
	if ((strcmp(Machine->gamedrv->name, "manxtt" ) == 0) || (strcmp(Machine->gamedrv->name, "srallyc" ) == 0))
	{
		return 8;
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( copro_ctl1_w )
{
	// did hi bit change?
	if ((data ^ model2_coproctl) == 0x80000000)
	{
		if (data & 0x80000000)
		{
			logerror("Start copro upload\n");
			model2_coprocnt = 0;
		}
		else
		{
			logerror("Boot copro, %d dwords\n", model2_coprocnt);
		}
	}

	model2_coproctl = data;
}

static WRITE32_HANDLER( geo_ctl1_w )
{
	// did hi bit change?
	if ((data ^ model2_geoctl) == 0x80000000)
	{
		if (data & 0x80000000)
		{
			logerror("Start geo upload\n");
			model2_geocnt = 0;
		}
		else
		{
			logerror("Boot geo, %d dwords\n", model2_geocnt);
		}
	}

	model2_geoctl = data;
}

static WRITE32_HANDLER(copro_prg_w)
{
	if (model2_coproctl & 0x80000000)
	{
		logerror("copro_prg_w: %08X:   %08X\n", model2_coprocnt, data);
		model2_coprocnt++;
	}
}



static WRITE32_HANDLER( copro_sharc_ctl1_w )
{
	// did hi bit change?
	if ((data ^ model2_coproctl) == 0x80000000)
	{
		if (data & 0x80000000)
		{
			logerror("Start copro upload\n");
			model2_coprocnt = 0;
		}
		else
		{
			logerror("Boot copro, %d dwords\n", model2_coprocnt);
			cpunum_set_input_line(2, INPUT_LINE_RESET, CLEAR_LINE);
			//cpu_spinuntil_time(TIME_IN_USEC(1000));       // Give the SHARC enough time to boot itself
		}
	}

	model2_coproctl = data;
}

static WRITE32_HANDLER( geo_sharc_ctl1_w )
{
	// did hi bit change?
	if ((data ^ model2_geoctl) == 0x80000000)
	{
		if (data & 0x80000000)
		{
			logerror("Start geo upload\n");
			model2_geocnt = 0;
		}
		else
		{
			logerror("Boot geo, %d dwords\n", model2_geocnt);
			cpunum_set_input_line(3, INPUT_LINE_RESET, CLEAR_LINE);
		}
	}

	model2_geoctl = data;
}

static READ32_HANDLER(copro_sharc_fifo_r)
{
	if ((strcmp(Machine->gamedrv->name, "manxtt" ) == 0) || (strcmp(Machine->gamedrv->name, "srallyc" ) == 0))
	{
		return 8;
	}
	else
	{
		logerror("copro_fifo_r: %08X, %08X\n", offset, mem_mask);
		return copro_fifoout_pop();
	}
}

static UINT16 copro_program_word[3];
static WRITE32_HANDLER(copro_sharc_fifo_w)
{
	if (model2_coproctl & 0x80000000)
	{
		copro_program_word[model2_coprocnt % 3] = data & 0xffff;
		if ((model2_coprocnt % 3) == 2)
		{
			UINT64 value = ((UINT64)copro_program_word[2] << 32) |
						   ((UINT64)copro_program_word[1] << 16) |
						   ((UINT64)copro_program_word[0]);
			cpuintrf_push_context(2);
			sharc_write_program_ram(0x20000 + (model2_coprocnt/3), value);
			cpuintrf_pop_context();
		}

		model2_coprocnt++;
	}
	else
	{
		logerror("copro_fifo_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
		if (data != 0)
		{
			copro_fifoin_push(data);
		}
	}
}

static UINT16 geo_program_word[3];
static WRITE32_HANDLER(geo_sharc_prg_w)
{
	if (model2_geoctl & 0x80000000)
	{
		geo_program_word[model2_geocnt % 3] = data & 0xffff;
		if ((model2_geocnt % 3) == 2)
		{
			UINT64 value = ((UINT64)geo_program_word[2] << 32) |
						   ((UINT64)geo_program_word[1] << 16) |
						   ((UINT64)geo_program_word[0]);
			cpuintrf_push_context(3);
			sharc_write_program_ram(0x20000 + (model2_geocnt/3), value);
			cpuintrf_pop_context();
		}

		model2_geocnt++;
	}
}

static WRITE32_HANDLER(copro_function_port_w)
{
	logerror("copro_function_port_w: %08X, %08X, %08X\n", data, offset, mem_mask);
}




static READ32_HANDLER(hotd_unk_r)
{
	return 0x000c0000;
}

static READ32_HANDLER(sonic_unk_r)
{
	return 0x001a0000;
}

static READ32_HANDLER(daytona_unk_r)
{
	return 0x00400000;
}

static READ32_HANDLER(desert_unk_r)
{
	// vcop needs bit 3 clear (infinite loop otherwise)
	// desert needs other bits set (not sure which specifically)
	// daytona needs the MSW to return ff
	return 0x00ff00f7;
}

static READ32_HANDLER(model2_irq_r)
{
	i960_noburst();

	if (offset)
	{
		return model2_intena;
	}

	return model2_intreq;
}

static WRITE32_HANDLER(model2_irq_w)
{
	i960_noburst();

	if (offset)
	{
		COMBINE_DATA(&model2_intena);
		return;
	}

	model2_intreq &= data;
}

static int to_68k;

static int snd_68k_ready_r(void)
{
	int sr = cpunum_get_reg(1, M68K_REG_SR);

	if ((sr & 0x0700) > 0x0100)
	{
		cpu_spinuntil_time(TIME_IN_USEC(40));
		return 0;	// not ready yet, interrupts disabled
	}

	return 0xff;
}

static void snd_latch_to_68k_w(int data)
{
	while (!snd_68k_ready_r())
	{
		cpu_spinuntil_time(TIME_IN_USEC(40));
	}

	to_68k = data;

	cpunum_set_input_line(1, 2, HOLD_LINE);

	// give the 68k time to notice
	cpu_spinuntil_time(TIME_IN_USEC(40));
}

static READ32_HANDLER( model2_serial_r )
{
	if ((offset == 0) && (mem_mask == 0x0000ffff))
	{
		return 0x00070000;	// TxRdy RxRdy (zeroguna also needs bit 4 set)
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( model2o_serial_w )
{
	if (mem_mask == 0xffff0000)
	{
		snd_latch_to_68k_w(data&0xff);
	}
}

static WRITE32_HANDLER( model2_serial_w )
{
	if (mem_mask == 0xffff0000)
	{
		SCSP_MidiIn(0, data&0xff, 0);

		// give the 68k time to notice
		cpu_spinuntil_time(TIME_IN_USEC(40));
	}
}

/* Protection handling */

static unsigned char ZGUNProt[] =
{
	0x7F,0x4E,0x1B,0x1E,0xA8,0x48,0xF5,0x49,0x31,0x32,0x4A,0x09,0x89,0x29,0xC0,0x41,
	0x3A,0x49,0x85,0x24,0xA0,0x4D,0x21,0x31,0xEA,0xC3,0x3F,0xAF,0x0E,0x4B,0x25,0x02,
	0xFB,0x0F,0x44,0x55,0x2E,0x82,0x55,0xC3,0xCB,0x91,0x52,0x7E,0x72,0x53,0xF2,0xAA,
	0x39,0x19,0xB1,0x42,0x33,0x63,0x13,0xFA,0x39,0x9C,0xE0,0x53,0x93,0x8B,0x14,0x91,
	0x9D,0x1C,0xFE,0x52,0x59,0xD4,0x2A,0x6A,0xA3,0xC5,0xA0,0xCA,0x92,0x5A,0x58,0xAC,
	0x95,0x4A,0x19,0x89,0x65,0xD3,0xA8,0x4A,0xE3,0xCE,0x8D,0x89,0xC5,0x48,0x95,0xE4,
	0x94,0xD5,0x73,0x09,0xE4,0x3D,0x2D,0x92,0xC9,0xA7,0xA3,0x53,0x42,0x82,0x55,0x67,
	0xE4,0x66,0xD0,0x4A,0x7D,0x4A,0x13,0xDE,0xD7,0x9F,0x38,0xAA,0x00,0x56,0x85,0x0A
};
static unsigned char DCOPKey1326[]=
{
	0x43,0x66,0x54,0x11,0x99,0xfe,0xcc,0x8e,0xdd,0x87,0x11,0x89,0x22,0xdf,0x44,0x09
};
static int protstate, protpos;
static unsigned char protram[256];

static READ32_HANDLER( model2_prot_r )
{
	static int a = 0;
	UINT32 retval = 0;

	if (offset == 0x10000/4)
	{
		// status: bit 0 = 1 for busy, 0 for ready
		return 0;	// we're always ready
	}
	else if (offset == 0x1000e/4)
	{
		retval = protram[protstate+1] | protram[protstate]<<8;
		retval <<= 16;
		protstate+=2;
	}
	else if (offset == 0x7ff8/4)
	{
		retval = protram[protstate+1] | protram[protstate]<<8;
		protstate+=2;
	}
	else if (offset == 0x400c/4)
	{
		a = !a;
		if (a)
			return 0xffff;
		else
			return 0xfff0;
	}
	else logerror("Unhandled Protection READ @ %x mask %x (PC=%x)\n", offset, mem_mask, activecpu_get_pc());

	return retval;
}

static WRITE32_HANDLER( model2_prot_w )
{
	if (mem_mask == 0xffff)
	{
		data >>= 16;
	}

	if (offset == 0x10008/4)
	{
		protpos = data;
	}
	else if (offset == 0x1000c/4)
	{
		switch (data)
		{
			// dynamcop
			case 0x7700:
				strcpy((char *)protram+2, "UCHIDA MOMOKA   ");
				break;

			// dynamcop
			case 0x1326:
				protstate = 0;
				memcpy(protram+2, DCOPKey1326, sizeof(DCOPKey1326));
				break;

			// zerogun
			case 0xA1BC:
			case 0xAD23:
			case 0x13CD:
			case 0x4D53:
			case 0x234D:
			case 0x113D:
			case 0x1049:
			case 0x993D:
			case 0x983C:
			case 0x935:
			case 0x9845:
			case 0x556D:
			case 0x98CC:
			case 0x3422:
			case 0x10:
				protstate = 0;
				memcpy(protram+2, ZGUNProt+((2*protpos)/12)*8, sizeof(ZGUNProt));
				break;

			// pltkids
			case 0x7140:
				protstate = 0;
				strcpy((char *)protram+2, "98-PILOT  ");
				break;

			default:
				protstate = 0;
				break;
		}
	}
	else if (offset == 0x7ff2/4)
	{
		if (data == 0)
		{
			protstate = 0;
			strcpy((char *)protram, "  TECMO LTD.  DEAD OR ALIVE  1996.10.22  VER. 1.00");
		}
	}
	else logerror("Unhandled Protection WRITE %x @ %x mask %x (PC=%x)\n", data, offset, mem_mask, activecpu_get_pc());

}

/* Daytona "To The MAXX" PIC protection simulation */

static int model2_maxxstate = 0;

static READ32_HANDLER( maxx_r )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	if (offset <= 0x1f/4)
	{
		// special
		if (mem_mask == 0x0000ffff)
		{
			// 16-bit protection reads
			model2_maxxstate++;
			model2_maxxstate &= 0xf;
			if (!model2_maxxstate)
			{
				return 0x00070000;
			}
			else
			{
				if (model2_maxxstate & 0x2)
				{
					return 0;
				}
				else
				{
					return 0x00040000;
				}
			}
		}
		else if (mem_mask == 0)
		{
			// 32-bit read
			if (offset == 0x22/4)
			{
				return 0x00ff0000;
			}
		}
	}

	return ROM[offset + (0x040000/4)];
}

/* Network board emulation */

static UINT32 model2_netram[0x8000/4];

static int zflagi, zflag, sysres;

static READ32_HANDLER( network_r )
{
	if ((mem_mask == 0) || (mem_mask == 0xffff0000) || (mem_mask == 0x0000ffff))
	{
		return 0xffffffff;
	}

	if (offset < 0x4000/4)
	{
		return model2_netram[offset];
	}

	if (mem_mask == 0xff00ffff)
	{
		return sysres<<16;
	}
	else if (mem_mask == 0xffffff00)
	{
		return zflagi;
	}

	return 0xffffffff;
}

static WRITE32_HANDLER( network_w )
{
	if ((mem_mask == 0) || (mem_mask == 0xffff0000) || (mem_mask == 0x0000ffff))
	{
		COMBINE_DATA(&model2_netram[offset+0x4000/4]);
		return;
	}

	if (offset < 0x4000/4)
	{
		COMBINE_DATA(&model2_netram[offset]);
		return;
	}

	if (mem_mask == 0xff00ffff)
	{
		sysres = data>>16;
	}
	else if (mem_mask == 0xffffff00)
	{
		zflagi = data;
		zflag = 0;
		if (data & 0x01) zflag |= 0x80;
		if (data & 0x80) zflag |= 0x01;
	}
}

/* common map for all Model 2 versions */
static ADDRESS_MAP_START( model2_base_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM AM_WRITENOP

	AM_RANGE(0x00500000, 0x005fffff) AM_RAM AM_BASE(&model2_workram)

	AM_RANGE(0x00800010, 0x00800013) AM_WRITENOP
	AM_RANGE(0x008000b0, 0x008000b3) AM_WRITENOP
	AM_RANGE(0x00804004, 0x0080400f) AM_WRITENOP	// quiet psikyo games

	AM_RANGE(0x00900000, 0x0097ffff) AM_RAM AM_BASE(&model2_bufferram)


	AM_RANGE(0x00980004, 0x00980007) AM_READ(fifoctl_r)
	AM_RANGE(0x0098000c, 0x0098000f) AM_READ(videoctl_r)

	AM_RANGE(0x00e80000, 0x00e80007) AM_READWRITE(model2_irq_r, model2_irq_w)

	AM_RANGE(0x00f00000, 0x00f0000f) AM_READWRITE(timers_r, timers_w)

	AM_RANGE(0x01000000, 0x0100ffff) AM_READWRITE(sys24_tile32_r, sys24_tile32_w) AM_MIRROR(0x100000)
	AM_RANGE(0x01020000, 0x01020003) AM_WRITENOP AM_MIRROR(0x100000)		// Unknown, always 0
	AM_RANGE(0x01040000, 0x01040003) AM_WRITENOP AM_MIRROR(0x100000)		// Horizontal synchronization register
	AM_RANGE(0x01060000, 0x01060003) AM_WRITENOP AM_MIRROR(0x100000)		// Vertical synchronization register
	AM_RANGE(0x01070000, 0x01070003) AM_WRITENOP AM_MIRROR(0x100000)		// Video synchronization switch
	AM_RANGE(0x01080000, 0x010fffff) AM_READWRITE(sys24_char32_r, sys24_char32_w) AM_MIRROR(0x100000)
	AM_RANGE(0x01100000, 0x0110ffff) AM_READWRITE(sys24_tile32_r, sys24_tile32_w) AM_MIRROR(0x10000)
	AM_RANGE(0x01180000, 0x011fffff) AM_READWRITE(sys24_char32_r, sys24_char32_w) AM_MIRROR(0x100000)

	AM_RANGE(0x01800000, 0x01803fff) AM_READWRITE(MRA32_RAM, pal32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x01810000, 0x0181bfff) AM_RAM AM_BASE(&model2_colorxlat)
	AM_RANGE(0x01a10000, 0x01a1ffff) AM_READWRITE(network_r, network_w)
	AM_RANGE(0x01d00000, 0x01d03fff) AM_RAM AM_BASE( &model2_backup1 ) // Backup sram
	AM_RANGE(0x02000000, 0x03ffffff) AM_ROM AM_REGION(REGION_USER1, 0)

	// "extra" data
	AM_RANGE(0x06000000, 0x06ffffff) AM_ROM AM_REGION(REGION_USER1, 0x1000000)

	AM_RANGE(0x11000000, 0x111fffff) AM_RAM	// texture RAM 0 (2b/2c)
	AM_RANGE(0x11200000, 0x113fffff) AM_RAM	// texture RAM 1 (2b/2c)
	AM_RANGE(0x11400000, 0x1140ffff) AM_RAM	// polygon "luma" RAM (2b/2c)

	AM_RANGE(0x11600000, 0x1167ffff) AM_RAM	AM_SHARE(1) // framebuffer (last bronx)
	AM_RANGE(0x11680000, 0x116fffff) AM_RAM	AM_SHARE(1) // FB mirror

	AM_RANGE(0x12000000, 0x12ffffff) AM_WRITENOP	// texture RAM for model 2/2a?
ADDRESS_MAP_END

/* original Model 2 overrides */
static ADDRESS_MAP_START( model2o_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00200000, 0x0021ffff) AM_RAM
	AM_RANGE(0x00220000, 0x0023ffff) AM_ROM AM_REGION(REGION_CPU1, 0x20000)

	AM_RANGE(0x00804000, 0x00804003) AM_READWRITE(geo_prg_r, geo_prg_w)
	AM_RANGE(0x00884000, 0x00884003) AM_READWRITE(copro_prg_r, copro_prg_w)

	AM_RANGE(0x00980000, 0x00980003) AM_WRITE( copro_ctl1_w )
	AM_RANGE(0x00980008, 0x0098000b) AM_WRITE( geo_ctl1_w )
	AM_RANGE(0x009c0000, 0x009cffff) AM_READWRITE( model2_serial_r, model2o_serial_w )

	AM_RANGE(0x01c00000, 0x01c00007) AM_READ(analog_r)
	AM_RANGE(0x01c00010, 0x01c00013) AM_READ(ctrl10_r)
	AM_RANGE(0x01c00014, 0x01c00017) AM_READ(ctrl14_r)
	AM_RANGE(0x01c0001c, 0x01c0001f) AM_READ( desert_unk_r )
	AM_RANGE(0x01c00040, 0x01c00043) AM_READ( daytona_unk_r )
	AM_RANGE(0x01c00200, 0x01c002ff) AM_RAM AM_BASE( &model2_backup2 )
	AM_RANGE(0x01c80000, 0x01c80003) AM_READWRITE( model2_serial_r, model2o_serial_w )
ADDRESS_MAP_END

/* 2A-CRX overrides */
static ADDRESS_MAP_START( model2a_crx_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00200000, 0x0023ffff) AM_RAM

	AM_RANGE(0x00804000, 0x00804003) AM_READWRITE(geo_prg_r, geo_prg_w)
	AM_RANGE(0x00884000, 0x00884003) AM_READWRITE(copro_prg_r, copro_prg_w)

	AM_RANGE(0x00980000, 0x00980003) AM_WRITE( copro_ctl1_w )
	AM_RANGE(0x00980008, 0x0098000b) AM_WRITE( geo_ctl1_w )
	AM_RANGE(0x009c0000, 0x009cffff) AM_READWRITE( model2_serial_r, model2_serial_w )

	AM_RANGE(0x01c00000, 0x01c00003) AM_READWRITE(ctrl0_r, ctrl0_w)
	AM_RANGE(0x01c00004, 0x01c00007) AM_READ(ctrl1_r)
	AM_RANGE(0x01c00010, 0x01c00013) AM_READ(ctrl10_r)
	AM_RANGE(0x01c00014, 0x01c00017) AM_READ(ctrl14_r)
	AM_RANGE(0x01c00018, 0x01c0001b) AM_READ( hotd_unk_r )
	AM_RANGE(0x01c0001c, 0x01c0001f) AM_READ( sonic_unk_r )
	AM_RANGE(0x01c00200, 0x01c002ff) AM_RAM AM_BASE( &model2_backup2 )
	AM_RANGE(0x01c80000, 0x01c80003) AM_READWRITE( model2_serial_r, model2_serial_w )
ADDRESS_MAP_END

/* 2B-CRX overrides */
static ADDRESS_MAP_START( model2b_crx_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00200000, 0x0023ffff) AM_RAM

	AM_RANGE(0x00804000, 0x00804003) AM_READWRITE(geo_prg_r, geo_sharc_prg_w)
	AM_RANGE(0x00880000, 0x00883fff) AM_WRITE(copro_function_port_w)
	AM_RANGE(0x00884000, 0x00887fff) AM_READWRITE(copro_sharc_fifo_r, copro_sharc_fifo_w)

	AM_RANGE(0x00980000, 0x00980003) AM_WRITE( copro_sharc_ctl1_w )
	AM_RANGE(0x00980008, 0x0098000b) AM_WRITE( geo_sharc_ctl1_w )
	AM_RANGE(0x009c0000, 0x009cffff) AM_READWRITE( model2_serial_r, model2_serial_w )

	AM_RANGE(0x01c00000, 0x01c00003) AM_READWRITE(ctrl0_r, ctrl0_w)
	AM_RANGE(0x01c00004, 0x01c00007) AM_READ(ctrl1_r)
	AM_RANGE(0x01c00010, 0x01c00013) AM_READ(ctrl10_r)
	AM_RANGE(0x01c00014, 0x01c00017) AM_READ(ctrl14_r)
	AM_RANGE(0x01c00018, 0x01c0001b) AM_READ( hotd_unk_r )
	AM_RANGE(0x01c0001c, 0x01c0001f) AM_READ( sonic_unk_r )
	AM_RANGE(0x01c80000, 0x01c80003) AM_READWRITE( model2_serial_r, model2_serial_w )
ADDRESS_MAP_END

/* 2C-CRX overrides */
static ADDRESS_MAP_START( model2c_crx_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00200000, 0x0023ffff) AM_RAM

	AM_RANGE(0x00804000, 0x00804003) AM_READWRITE(geo_prg_r, geo_prg_w)
	AM_RANGE(0x00884000, 0x00884003) AM_READWRITE(copro_prg_r, copro_prg_w)

	AM_RANGE(0x00980000, 0x00980003) AM_WRITE( copro_ctl1_w )
	AM_RANGE(0x00980008, 0x0098000b) AM_WRITE( geo_ctl1_w )
	AM_RANGE(0x009c0000, 0x009cffff) AM_READWRITE( model2_serial_r, model2_serial_w )

	AM_RANGE(0x01c00000, 0x01c00003) AM_READWRITE(ctrl0_r, ctrl0_w)
	AM_RANGE(0x01c00004, 0x01c00007) AM_READ(ctrl1_r)
	AM_RANGE(0x01c00010, 0x01c00013) AM_READ(ctrl10_r)
	AM_RANGE(0x01c00014, 0x01c00017) AM_READ(ctrl14_r)
	AM_RANGE(0x01c00018, 0x01c0001b) AM_READ( hotd_unk_r )
	AM_RANGE(0x01c0001c, 0x01c0001f) AM_READ( sonic_unk_r )
	AM_RANGE(0x01c80000, 0x01c80003) AM_READWRITE( model2_serial_r, model2_serial_w )
ADDRESS_MAP_END

/* Input definitions */

#define MODEL2_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(_n_)

INPUT_PORTS_START( model2 )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	PORT_START
	MODEL2_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	MODEL2_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
INPUT_PORTS_END

INPUT_PORTS_START( desert )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) // VR 1 (Blue)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) // VR 2 (Green)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) // VR 3 (Red)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) // shift
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)  // machine gun
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)  // cannon
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	MODEL2_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// steer
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accel
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( daytona )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) // VR 1 (Red)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) // VR 2 (Blue)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) // VR 3 (Yellow)

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1) // VR 4 (Green)
	PORT_BIT(0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) // shift 3
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // shift 4
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)

	PORT_START
	MODEL2_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	// steer
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accel
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( bel )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	PORT_START
	MODEL2_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	MODEL2_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
INPUT_PORTS_END

static INTERRUPT_GEN(model2_interrupt)
{
	switch (cpu_getiloops())
	{
		case 0:
			model2_intreq |= (1<<10);
			if (model2_intena & (1<<10))
			{
				cpunum_set_input_line(0, I960_IRQ3, ASSERT_LINE);
			}
			break;
		case 1:
			model2_intreq |= (1<<0);
			if (model2_intena & (1<<0))
			{
				cpunum_set_input_line(0, I960_IRQ0, ASSERT_LINE);
			}
			break;
	}
}

static INTERRUPT_GEN(model2c_interrupt)
{
	switch (cpu_getiloops())
	{
		case 0:
			model2_intreq |= (1<<10);
			if (model2_intena & (1<<10))
				cpunum_set_input_line(0, I960_IRQ3, ASSERT_LINE);
			break;
		case 1:
			model2_intreq |= (1<<2);
			if (model2_intena & (1<<2))
				cpunum_set_input_line(0, I960_IRQ2, ASSERT_LINE);

			break;
		case 2:
			model2_intreq |= (1<<0);
			if (model2_intena & (1<<0))
				cpunum_set_input_line(0, I960_IRQ0, ASSERT_LINE);
			break;
	}
}

/* Model 1 sound board emulation */

static READ16_HANDLER( m1_snd_68k_latch_r )
{
	return to_68k;
}

static READ16_HANDLER( m1_snd_v60_ready_r )
{
	return 1;
}

static READ16_HANDLER( m1_snd_mpcm0_r )
{
	return MultiPCM_reg_0_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm0_w )
{
	MultiPCM_reg_0_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm0_bnk_w )
{
	multipcm_set_bank(0, 0x100000 * (data & 0xf), 0x100000 * (data & 0xf));
}

static READ16_HANDLER( m1_snd_mpcm1_r )
{
	return MultiPCM_reg_1_r(0);
}

static WRITE16_HANDLER( m1_snd_mpcm1_w )
{
	MultiPCM_reg_1_w(offset, data);
}

static WRITE16_HANDLER( m1_snd_mpcm1_bnk_w )
{
	multipcm_set_bank(1, 0x100000 * (data & 0xf), 0x100000 * (data & 0xf));
}

static READ16_HANDLER( m1_snd_ym_r )
{
	return YM3438_status_port_0_A_r(0);
}

static WRITE16_HANDLER( m1_snd_ym_w )
{
	switch (offset)
	{
		case 0:
			YM3438_control_port_0_A_w(0, data);
			break;

		case 1:
			YM3438_data_port_0_A_w(0, data);
			break;

		case 2:
			YM3438_control_port_0_B_w(0, data);
			break;

		case 3:
			YM3438_data_port_0_B_w(0, data);
			break;
	}
}

static WRITE16_HANDLER( m1_snd_68k_latch1_w )
{
}

static WRITE16_HANDLER( m1_snd_68k_latch2_w )
{
}

static ADDRESS_MAP_START( model1_snd, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x0bffff) AM_ROM AM_REGION(REGION_CPU2, 0x20000)	// mirror of second program ROM
	AM_RANGE(0xc20000, 0xc20001) AM_READWRITE( m1_snd_68k_latch_r, m1_snd_68k_latch1_w )
	AM_RANGE(0xc20002, 0xc20003) AM_READWRITE( m1_snd_v60_ready_r, m1_snd_68k_latch2_w )
	AM_RANGE(0xc40000, 0xc40007) AM_READWRITE( m1_snd_mpcm0_r, m1_snd_mpcm0_w )
	AM_RANGE(0xc40012, 0xc40013) AM_WRITENOP
	AM_RANGE(0xc50000, 0xc50001) AM_WRITE( m1_snd_mpcm0_bnk_w )
	AM_RANGE(0xc60000, 0xc60007) AM_READWRITE( m1_snd_mpcm1_r, m1_snd_mpcm1_w )
	AM_RANGE(0xc70000, 0xc70001) AM_WRITE( m1_snd_mpcm1_bnk_w )
	AM_RANGE(0xd00000, 0xd00007) AM_READWRITE( m1_snd_ym_r, m1_snd_ym_w )
	AM_RANGE(0xf00000, 0xf0ffff) AM_RAM
ADDRESS_MAP_END

/* Model 2 sound board emulation */

static WRITE16_HANDLER( model2snd_ctrl )
{
	// handle sample banking
	if (memory_region_length(REGION_SOUND1) > 0x800000)
	{
		if (data & 0x20)
		{
	  		memory_set_bankptr(4, memory_region(REGION_SOUND1) + 0x200000);
			memory_set_bankptr(5, memory_region(REGION_SOUND1) + 0x600000);
		}
		else
		{
			memory_set_bankptr(4, memory_region(REGION_SOUND1) + 0x800000);
			memory_set_bankptr(5, memory_region(REGION_SOUND1) + 0xa00000);
		}
	}
}

static ADDRESS_MAP_START( model2_snd, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_RAM AM_REGION(REGION_CPU2, 0)
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(SCSP_0_r, SCSP_0_w)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(model2snd_ctrl)
	AM_RANGE(0x600000, 0x67ffff) AM_ROM AM_REGION(REGION_CPU2, 0x80000)
	AM_RANGE(0x800000, 0x9fffff) AM_ROM AM_REGION(REGION_SOUND1, 0)
	AM_RANGE(0xa00000, 0xdfffff) AM_READ(MRA16_BANK4)
	AM_RANGE(0xe00000, 0xffffff) AM_READ(MRA16_BANK5)
ADDRESS_MAP_END

static int scsp_last_line = 0;

static void scsp_irq(int irq)
{
	if (irq)
	{
		scsp_last_line = irq;
		cpunum_set_input_line(1, irq, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(1, scsp_last_line, CLEAR_LINE);
	}
}

static struct SCSPinterface scsp_interface =
{
	REGION_CPU2,
	0,
	scsp_irq
};

static struct MultiPCM_interface m1_multipcm_interface_1 =
{
	REGION_SOUND1
};

static struct MultiPCM_interface m1_multipcm_interface_2 =
{
	REGION_SOUND2
};




static READ32_HANDLER(copro_sharc_input_fifo_r)
{
	return copro_fifoin_pop();
}

static WRITE32_HANDLER(copro_sharc_output_fifo_w)
{
	copro_fifoout_push(data);
}

static ADDRESS_MAP_START( copro_sharc_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x0bfffff) AM_READ(copro_sharc_input_fifo_r)
	AM_RANGE(0x0c00000, 0x13fffff) AM_WRITE(copro_sharc_output_fifo_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( geo_sharc_map, ADDRESS_SPACE_DATA, 32 )
ADDRESS_MAP_END





/* original Model 2 */
static MACHINE_DRIVER_START( model2o )
	MDRV_CPU_ADD(I960, 25000000)
	MDRV_CPU_PROGRAM_MAP(model2_base_mem, model2o_mem)
 	MDRV_CPU_VBLANK_INT(model2_interrupt,2)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(model1_snd, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model2o)
	MDRV_NVRAM_HANDLER( model2 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model2)
	MDRV_VIDEO_UPDATE(model2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM3438, 8000000)
	MDRV_SOUND_ROUTE(0, "left", 0.60)
	MDRV_SOUND_ROUTE(1, "right", 0.60)

	MDRV_SOUND_ADD(MULTIPCM, 8000000)
	MDRV_SOUND_CONFIG(m1_multipcm_interface_1)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(MULTIPCM, 8000000)
	MDRV_SOUND_CONFIG(m1_multipcm_interface_2)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/* 2A-CRX */
static MACHINE_DRIVER_START( model2a )
	MDRV_CPU_ADD(I960, 25000000)
	MDRV_CPU_PROGRAM_MAP(model2_base_mem, model2a_crx_mem)
 	MDRV_CPU_VBLANK_INT(model2_interrupt,2)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(model2_snd, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model2)
	MDRV_NVRAM_HANDLER( model2 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model2)
	MDRV_VIDEO_UPDATE(model2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(0, "right", 1.0)
MACHINE_DRIVER_END


static sharc_config sharc_cfg =
{
	BOOT_MODE_HOST
};

/* 2B-CRX */
static MACHINE_DRIVER_START( model2b )
	MDRV_CPU_ADD(I960, 25000000)
	MDRV_CPU_PROGRAM_MAP(model2_base_mem, model2b_crx_mem)
 	MDRV_CPU_VBLANK_INT(model2_interrupt,2)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(model2_snd, 0)

	MDRV_CPU_ADD(ADSP21062, 40000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(copro_sharc_map, 0)

	MDRV_CPU_ADD(ADSP21062, 40000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(geo_sharc_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model2b)
	MDRV_NVRAM_HANDLER( model2 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model2)
	MDRV_VIDEO_UPDATE(model2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(0, "right", 1.0)
MACHINE_DRIVER_END

/* 2C-CRX */
static MACHINE_DRIVER_START( model2c )
	MDRV_CPU_ADD(I960, 25000000)
	MDRV_CPU_PROGRAM_MAP(model2_base_mem, model2c_crx_mem)
 	MDRV_CPU_VBLANK_INT(model2c_interrupt,3)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(model2_snd, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(model2)
	MDRV_NVRAM_HANDLER( model2 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)

	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(model2)
	MDRV_VIDEO_UPDATE(model2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(0, "right", 1.0)
MACHINE_DRIVER_END

/* ROM definitions */

/*
(info from 2a)

The smt ROMs are located on the CPU board and are labelled....
OPR-14742A \
OPR-14743A /  Linked to 315-5674
OPR-14744    \
OPR-14745    /  Linked to 315-5679B
OPR-14746    \
OPR-14747    /  Linked to 315-5679B

*/

#define MODEL2_CPU_BOARD \
	ROM_REGION( 0xc0000, REGION_USER5, 0 ) \
	ROM_LOAD("opr14742a.45",  0x000000,  0x20000, CRC(90c6b117) SHA1(f46429fffcee17d056f56d5fe035a33f1fd6c27e) ) \
	ROM_LOAD("opr14743a.46",  0x020000,  0x20000, CRC(ae7f446b) SHA1(5b9f1fc47caf21e061e930c0d72804e4ec8c7bca) ) \
	ROM_LOAD("opr14744.58",   0x040000,  0x20000, CRC(730ea9e0) SHA1(651f1db4089a400d073b19ada299b4b08b08f372) ) \
	ROM_LOAD("opr14745.59",   0x060000,  0x20000, CRC(4c934d96) SHA1(e3349ece0e47f684d61ad11bfea4a90602287350) ) \
	ROM_LOAD("opr14746.62",   0x080000,  0x20000, CRC(2a266cbd) SHA1(34e047a93459406c22acf4c25089d1a4955f94ca) ) \
	ROM_LOAD("opr14747.63",   0x0a0000,  0x20000, CRC(a4ad5e19) SHA1(7d7ec300eeb9a8de1590011e37108688c092f329) ) \

/*
These are smt ROMs found on Sega Model 2A Video board
They are linked to a QFP208 IC labelled 315-5645
*/

#define MODEL2A_VID_BOARD \
	ROM_REGION( 0x180000, REGION_USER6, 0 ) \
	ROM_LOAD("mpr16310.15",   0x000000,  0x80000, CRC(c078a780) SHA1(0ad5b49774172743e2708b7ca4c061acfe10957a) ) \
	ROM_LOAD("mpr16311.16",   0x080000,  0x80000, CRC(452a492b) SHA1(88c2f6c2dbfd0c1b39a7bf15c74455fb68c7274e) ) \
	ROM_LOAD("mpr16312.14",   0x100000,  0x80000, CRC(a25fef5b) SHA1(c6a37856b97f5bc4996cb6b66209f47af392cc38) ) \



ROM_START( zeroguna )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "epr20437",     0x000000, 0x080000, CRC(fad30cc0) SHA1(5c6222e07594b4be59b5095f7cc0a164d5895306) )
        ROM_LOAD32_WORD( "epr20438",     0x000002, 0x080000, CRC(ca364408) SHA1(4672ebdd7d9ccab5e107fda9d322b70583246c7a) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20296.11",   0x000000, 0x400000, CRC(072d8a5e) SHA1(7f69c90dd3c3e6e522d1065b3c4b09434cb4e634))
	ROM_LOAD32_WORD("mp20297.12",   0x000002, 0x400000, CRC(ba6a825b) SHA1(670a86c3a1a78550c760cc66c0a6181928fb9054))
	ROM_LOAD32_WORD("mp20294.9",    0x800000, 0x400000, CRC(a0bd1474) SHA1(c0c032adac69bd545e3aab481878b08f3c3edab8))
	ROM_LOAD32_WORD("mp20295.10",   0x800002, 0x400000, CRC(c548cced) SHA1(d34f2fc9b4481c75a6824aa4bdd3f1884188d35b))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20298.17",   0x000000, 0x400000, CRC(8ab782fc) SHA1(595f6fc2e9c58ce9763d51798ceead8d470f0a33))
	ROM_LOAD32_WORD("mp20299.21",   0x000002, 0x400000, CRC(90e20cdb) SHA1(730d58286fb7e91aa4128dc208b0f60eb3becc78))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20301.27",   0x000000, 0x200000, CRC(52010fb2) SHA1(8dce67c6f9e48d749c64b11d4569df413dc40e07))
	ROM_LOAD32_WORD("mp20300.25",   0x000002, 0x200000, CRC(6f042792) SHA1(75db68e57ec3fbc7af377342eef81f26fae4e1c4))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20302.31",   0x080000,  0x80000, CRC(44ff50d2) SHA1(6ffec81042fd5708e8a5df47b63f9809f93bf0f8))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20303.32",   0x000000, 0x200000, CRC(c040973f) SHA1(57a496c5dcc1a3931b6e41bf8d41e45d6dac0c31))
	ROM_LOAD("mp20304.33",   0x200000, 0x200000, CRC(6decfe83) SHA1(d73adafceff2f1776c93e53bd5677d67f1c2c08f))

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( zerogun )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep20439.15",   0x000000, 0x080000, CRC(10125381) SHA1(1e178e6bd2b1312cd6290f1be4b386f520465836))
	ROM_LOAD32_WORD("ep20440.16",   0x000002, 0x080000, CRC(ce872747) SHA1(82bf138a42c659b675b14e41d526b1628fb46ae3))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20296.11",   0x000000, 0x400000, CRC(072d8a5e) SHA1(7f69c90dd3c3e6e522d1065b3c4b09434cb4e634))
	ROM_LOAD32_WORD("mp20297.12",   0x000002, 0x400000, CRC(ba6a825b) SHA1(670a86c3a1a78550c760cc66c0a6181928fb9054))
	ROM_LOAD32_WORD("mp20294.9",    0x800000, 0x400000, CRC(a0bd1474) SHA1(c0c032adac69bd545e3aab481878b08f3c3edab8))
	ROM_LOAD32_WORD("mp20295.10",   0x800002, 0x400000, CRC(c548cced) SHA1(d34f2fc9b4481c75a6824aa4bdd3f1884188d35b))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20298.17",   0x000000, 0x400000, CRC(8ab782fc) SHA1(595f6fc2e9c58ce9763d51798ceead8d470f0a33))
	ROM_LOAD32_WORD("mp20299.21",   0x000002, 0x400000, CRC(90e20cdb) SHA1(730d58286fb7e91aa4128dc208b0f60eb3becc78))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20301.27",   0x000000, 0x200000, CRC(52010fb2) SHA1(8dce67c6f9e48d749c64b11d4569df413dc40e07))
	ROM_LOAD32_WORD("mp20300.25",   0x000002, 0x200000, CRC(6f042792) SHA1(75db68e57ec3fbc7af377342eef81f26fae4e1c4))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20302.31",   0x080000,  0x80000, CRC(44ff50d2) SHA1(6ffec81042fd5708e8a5df47b63f9809f93bf0f8))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20303.32",   0x000000, 0x200000, CRC(c040973f) SHA1(57a496c5dcc1a3931b6e41bf8d41e45d6dac0c31))
	ROM_LOAD("mp20304.33",   0x200000, 0x200000, CRC(6decfe83) SHA1(d73adafceff2f1776c93e53bd5677d67f1c2c08f))
ROM_END

ROM_START( zerogunj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep20290.15",   0x000000, 0x080000, CRC(9ce3ad21) SHA1(812ab45cc9e2920e74e58937d1826774f3f54183))
	ROM_LOAD32_WORD("ep20291.16",   0x000002, 0x080000, CRC(7267a03d) SHA1(a7216914ee7535fa1856cb19bc05c89948a93c89))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20296.11",   0x000000, 0x400000, CRC(072d8a5e) SHA1(7f69c90dd3c3e6e522d1065b3c4b09434cb4e634))
	ROM_LOAD32_WORD("mp20297.12",   0x000002, 0x400000, CRC(ba6a825b) SHA1(670a86c3a1a78550c760cc66c0a6181928fb9054))
	ROM_LOAD32_WORD("mp20294.9",    0x800000, 0x400000, CRC(a0bd1474) SHA1(c0c032adac69bd545e3aab481878b08f3c3edab8))
	ROM_LOAD32_WORD("mp20295.10",   0x800002, 0x400000, CRC(c548cced) SHA1(d34f2fc9b4481c75a6824aa4bdd3f1884188d35b))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20298.17",   0x000000, 0x400000, CRC(8ab782fc) SHA1(595f6fc2e9c58ce9763d51798ceead8d470f0a33))
	ROM_LOAD32_WORD("mp20299.21",   0x000002, 0x400000, CRC(90e20cdb) SHA1(730d58286fb7e91aa4128dc208b0f60eb3becc78))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20301.27",   0x000000, 0x200000, CRC(52010fb2) SHA1(8dce67c6f9e48d749c64b11d4569df413dc40e07))
	ROM_LOAD32_WORD("mp20300.25",   0x000002, 0x200000, CRC(6f042792) SHA1(75db68e57ec3fbc7af377342eef81f26fae4e1c4))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20302.31",   0x080000,  0x80000, CRC(44ff50d2) SHA1(6ffec81042fd5708e8a5df47b63f9809f93bf0f8))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20303.32",   0x000000, 0x200000, CRC(c040973f) SHA1(57a496c5dcc1a3931b6e41bf8d41e45d6dac0c31))
	ROM_LOAD("mp20304.33",   0x200000, 0x200000, CRC(6decfe83) SHA1(d73adafceff2f1776c93e53bd5677d67f1c2c08f))
ROM_END

ROM_START( gunblade )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18988a.15",  0x000000, 0x080000, CRC(f63f1ad2) SHA1(fcfb0a4691cd7d66168c421e4e1694ecaea56ab2))
	ROM_LOAD32_WORD("ep18989a.16",  0x000002, 0x080000, CRC(c1c84d65) SHA1(92bffbf1250c53499c37a53f9e2a054fc7bf256f))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18974.11",   0x000000, 0x400000, CRC(e29ecaff) SHA1(dcdfe9f59158cec2f02b213ee13f5e40cdb92e55))
	ROM_LOAD32_WORD("mp18975.12",   0x000002, 0x400000, CRC(d8187582) SHA1(34a0b32eeed1a9f41bca8b9261851881b2ba79f2))
	ROM_LOAD32_WORD("mp18976.9",    0x800000, 0x400000, CRC(c95c15eb) SHA1(892063e91b2ed20e0600d4b188da1e9f45a19692))
	ROM_LOAD32_WORD("mp18977.10",   0x800002, 0x400000, CRC(db8f5b6f) SHA1(c11d2c9e1e215aa7b2ebb777639c8cd651901f52))

	ROM_REGION( 0x800000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp18986.29",   0x000000, 0x400000, CRC(04820f7b) SHA1(5eb6682399b358d77658d82e612b02b724e3f3e1))
	ROM_LOAD32_WORD("mp18987.30",   0x000002, 0x400000, CRC(2419367f) SHA1(0a04a1049d2da486dc9dbb97b383bd24259b78c8))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18980.17",   0x000000, 0x400000, CRC(22345534) SHA1(7b8bdcfe88953ce1b2d75af2ce4712ab6507e2cf))
	ROM_LOAD32_WORD("mp18981.21",   0x000002, 0x400000, CRC(2544a33d) SHA1(a76193f70adb6abeba02328b290af5cca47d4e25))
	ROM_LOAD32_WORD("mp18982.18",   0x800000, 0x400000, CRC(d0a92b2a) SHA1(95404baed88cc95b75ff9b9084d09622961d3e57))
	ROM_LOAD32_WORD("mp18983.22",   0x800002, 0x400000, CRC(1b4af982) SHA1(550f8248699b9267da7d2e64002be56972381714))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18985.27",   0x000000, 0x400000, CRC(ad6166e3) SHA1(2c487fb743730cacf92dbea952b1efada0f073df))
	ROM_LOAD32_WORD("mp18984.25",   0x000002, 0x400000, CRC(756f6f37) SHA1(095964de773f515d64d65dbc8f8ef9bae97e5ba9))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18990.31",   0x080000,  0x80000, CRC(02b1b0d1) SHA1(759b4683dc7149e04f41ddac7bd395e8d07ea858))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18978.32",   0x000000, 0x400000, CRC(0f78b3e3) SHA1(6c2cd6236cb001bb8d487a9b1e9907519dc43daa))
	ROM_LOAD("mp18979.34",   0x400000, 0x400000, CRC(f13ea36f) SHA1(a8165116b5e07e031ff960201dd8c9a441544961))
ROM_END

ROM_START( vf2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "18385.epr",    0x000000, 0x020000, CRC(78ed2d41) SHA1(471c19389ceeec6138107dd81863320bd4825327) )
        ROM_LOAD32_WORD( "18386.epr",    0x000002, 0x020000, CRC(3418f428) SHA1(0f51e389e13efc172a26471331a60c459ad43c38) )
        ROM_LOAD32_WORD( "18387.epr",    0x040000, 0x020000, CRC(124a8453) SHA1(26fb787451824fc6060724e37fe0ba6bb66796cb) )
        ROM_LOAD32_WORD( "18388.epr",    0x040002, 0x020000, CRC(8d347980) SHA1(da79e51ad501b9560c4ed7cf1ec768daad93efe0) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mpr17560.10",    0x000000, 0x200000, CRC(d1389864) SHA1(88e9a8b6b0f58c96957015179e7ff10f837040e6) )
        ROM_LOAD32_WORD( "mpr17561.11",    0x000002, 0x200000, CRC(b98d0101) SHA1(e154877380b9250d8119dd4c14ba306c7b337dcd) )
        ROM_LOAD32_WORD( "mpr17558.8",     0x400000, 0x200000, CRC(4b15f5a6) SHA1(9a34724958fef9b49eae39c6ea136e0cf532154b) )
        ROM_LOAD32_WORD( "mpr17559.9",     0x400002, 0x200000, CRC(d3264de6) SHA1(2f094ff0b95bf1cd5c283414634ea9597204d374) )
        ROM_LOAD32_WORD( "mpr17566.6",     0x800000, 0x200000, CRC(fb41ef98) SHA1(ad4d1ba5e5b39b2d87105ae80750284867aa4ed3) )
        ROM_LOAD32_WORD( "mpr17567.7",     0x800002, 0x200000, CRC(c3396922) SHA1(7e0700ded530e4eb58e9a68cdb92791284c91431) )
        ROM_LOAD32_WORD( "mpr17564.4",     0xc00000, 0x200000, CRC(d8062489) SHA1(57666b6937f79bb65c43ed02b04a454882d01e61) )
        ROM_LOAD32_WORD( "mpr17565.5",     0xc00002, 0x200000, CRC(0517c6e9) SHA1(d9ba93998286713758385033119416714674c8d8) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mpr17554.16",     0x000000, 0x200000, CRC(27896d82) SHA1(c0624e58de2e427465daaa10dbb02ea2a1fd0f1b) )
        ROM_LOAD32_WORD( "mpr17548.20",     0x000002, 0x200000, CRC(c95facc2) SHA1(09d19abe5d75a335df7510df8abb2d4425159cdf) )
        ROM_LOAD32_WORD( "mpr17555.17",     0x400000, 0x200000, CRC(4df2810b) SHA1(720c4628d7783f0323b5723b441e13741556241e) )
        ROM_LOAD32_WORD( "mpr17549.21",     0x400002, 0x200000, CRC(e0bce0e6) SHA1(0570604dc2007288795a3125ffd480bc4b3b0802) )
        ROM_LOAD32_WORD( "mpr17556.18",     0x800000, 0x200000, CRC(41a47616) SHA1(55b909d2bc2079d0dfed5036c78c9e09bce09843) )
        ROM_LOAD32_WORD( "mpr17550.22",     0x800002, 0x200000, CRC(c36ff3f5) SHA1(f14fdf275905a90a0d4cc534d90b0302f26676d8) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mpr17553.25",     0x000000, 0x200000, CRC(5da1c5d3) SHA1(c627b25a1f61a9fe9182e2199f70f6e485503c7b) )
        ROM_LOAD32_WORD( "mpr17552.24",     0x000002, 0x200000, CRC(e91e7427) SHA1(0ac1111f2ecb4f924b5119eaaac8fa7bc87ab9d1) )
        ROM_LOAD32_WORD( "mpr17547.27",     0x400000, 0x200000, CRC(be940431) SHA1(5c1196a6454a4fead79a930979f2e69639ec2bb9) )
        ROM_LOAD32_WORD( "mpr17546.26",     0x400002, 0x200000, CRC(042a194b) SHA1(c6d8524dc0a879394f1234b7bb04836081bb3830) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "mpr17574.30",     0x080000, 0x080000, CRC(4d4c3a55) SHA1(b6c0c3f0473bd7fc3ef4f5146110dfcc899a5af9) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr17573.31",     0x000000, 0x200000, CRC(e43557fe) SHA1(4c61a135819862df02347c118dc4d88a0adac273) )
        ROM_LOAD( "mpr17572.32",     0x200000, 0x200000, CRC(4febecc8) SHA1(9683ea9bedfc5cd7b4a28e9a68792c0dc549d911) )
        ROM_LOAD( "mpr17571.36",     0x400000, 0x200000, CRC(d22dc8ca) SHA1(3bba7780fa4ec23e40127a5b0104fba7d5e30b5f) )
        ROM_LOAD( "mpr17570.37",     0x600000, 0x200000, CRC(bccd324b) SHA1(4c7ebdea08b2dedf621f121785ed1c40ebae4236) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( vf2b )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "mpr17568.12",   0x000000, 0x020000, CRC(5d966bbf) SHA1(01d46313148ce509fa5641fb07a3f840c00886ac) )
        ROM_LOAD32_WORD( "mpr17569.13",   0x000002, 0x020000, CRC(0b8c1ccc) SHA1(ba2e0ac8b31955fed237ba9a5eda9fa14d1db11f) )
        ROM_LOAD32_WORD( "mpr17562.14",   0x040000, 0x020000, CRC(b778d4eb) SHA1(a7162d9c39d601ac92310c8cf2ae388647a5295a) )
        ROM_LOAD32_WORD( "mpr17563.15",   0x040002, 0x020000, CRC(a05c15f6) SHA1(b9b1f3c68c53a86dfa3cbc85fcb9150546c13f23) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mpr17560.10",    0x000000, 0x200000, CRC(d1389864) SHA1(88e9a8b6b0f58c96957015179e7ff10f837040e6) )
        ROM_LOAD32_WORD( "mpr17561.11",    0x000002, 0x200000, CRC(b98d0101) SHA1(e154877380b9250d8119dd4c14ba306c7b337dcd) )
        ROM_LOAD32_WORD( "mpr17558.8",     0x400000, 0x200000, CRC(4b15f5a6) SHA1(9a34724958fef9b49eae39c6ea136e0cf532154b) )
        ROM_LOAD32_WORD( "mpr17559.9",     0x400002, 0x200000, CRC(d3264de6) SHA1(2f094ff0b95bf1cd5c283414634ea9597204d374) )
        ROM_LOAD32_WORD( "mpr17566.6",     0x800000, 0x200000, CRC(fb41ef98) SHA1(ad4d1ba5e5b39b2d87105ae80750284867aa4ed3) )
        ROM_LOAD32_WORD( "mpr17567.7",     0x800002, 0x200000, CRC(c3396922) SHA1(7e0700ded530e4eb58e9a68cdb92791284c91431) )
        ROM_LOAD32_WORD( "mpr17564.4",     0xc00000, 0x200000, CRC(d8062489) SHA1(57666b6937f79bb65c43ed02b04a454882d01e61) )
        ROM_LOAD32_WORD( "mpr17565.5",     0xc00002, 0x200000, CRC(0517c6e9) SHA1(d9ba93998286713758385033119416714674c8d8) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mpr17554.16",     0x000000, 0x200000, CRC(27896d82) SHA1(c0624e58de2e427465daaa10dbb02ea2a1fd0f1b) )
        ROM_LOAD32_WORD( "mpr17548.20",     0x000002, 0x200000, CRC(c95facc2) SHA1(09d19abe5d75a335df7510df8abb2d4425159cdf) )
        ROM_LOAD32_WORD( "mpr17555.17",     0x400000, 0x200000, CRC(4df2810b) SHA1(720c4628d7783f0323b5723b441e13741556241e) )
        ROM_LOAD32_WORD( "mpr17549.21",     0x400002, 0x200000, CRC(e0bce0e6) SHA1(0570604dc2007288795a3125ffd480bc4b3b0802) )
        ROM_LOAD32_WORD( "mpr17556.18",     0x800000, 0x200000, CRC(41a47616) SHA1(55b909d2bc2079d0dfed5036c78c9e09bce09843) )
        ROM_LOAD32_WORD( "mpr17550.22",     0x800002, 0x200000, CRC(c36ff3f5) SHA1(f14fdf275905a90a0d4cc534d90b0302f26676d8) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mpr17553.25",     0x000000, 0x200000, CRC(5da1c5d3) SHA1(c627b25a1f61a9fe9182e2199f70f6e485503c7b) )
        ROM_LOAD32_WORD( "mpr17552.24",     0x000002, 0x200000, CRC(e91e7427) SHA1(0ac1111f2ecb4f924b5119eaaac8fa7bc87ab9d1) )
        ROM_LOAD32_WORD( "mpr17547.27",     0x400000, 0x200000, CRC(be940431) SHA1(5c1196a6454a4fead79a930979f2e69639ec2bb9) )
        ROM_LOAD32_WORD( "mpr17546.26",     0x400002, 0x200000, CRC(042a194b) SHA1(c6d8524dc0a879394f1234b7bb04836081bb3830) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "mpr17574.30",     0x080000, 0x080000, CRC(4d4c3a55) SHA1(b6c0c3f0473bd7fc3ef4f5146110dfcc899a5af9) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr17573.31",     0x000000, 0x200000, CRC(e43557fe) SHA1(4c61a135819862df02347c118dc4d88a0adac273) )
        ROM_LOAD( "mpr17572.32",     0x200000, 0x200000, CRC(4febecc8) SHA1(9683ea9bedfc5cd7b4a28e9a68792c0dc549d911) )
        ROM_LOAD( "mpr17571.36",     0x400000, 0x200000, CRC(d22dc8ca) SHA1(3bba7780fa4ec23e40127a5b0104fba7d5e30b5f) )
        ROM_LOAD( "mpr17570.37",     0x600000, 0x200000, CRC(bccd324b) SHA1(4c7ebdea08b2dedf621f121785ed1c40ebae4236) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( vf2o )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "epr17568.12",   0x000000, 0x020000, CRC(cf5d53d1) SHA1(4ed907bbfc1a47e51c9cc11f55645752574adaef) )
        ROM_LOAD32_WORD( "epr17569.13",   0x000002, 0x020000, CRC(0fb32808) SHA1(95efb3eeaf95fb5f79ddae4ef20e2211b07f8d30) )
        ROM_LOAD32_WORD( "epr17562.14",   0x040000, 0x020000, CRC(b893bcef) SHA1(2f862a7099aa757ee1f2ad8245eb4f8f4fdfb7bc) )
        ROM_LOAD32_WORD( "epr17563.15",   0x040002, 0x020000, CRC(3b55f5a8) SHA1(b1ca3d4d3568c1652dcd8e546ffff23a4a21a699) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mpr17560.10",    0x000000, 0x200000, CRC(d1389864) SHA1(88e9a8b6b0f58c96957015179e7ff10f837040e6) )
        ROM_LOAD32_WORD( "mpr17561.11",    0x000002, 0x200000, CRC(b98d0101) SHA1(e154877380b9250d8119dd4c14ba306c7b337dcd) )
        ROM_LOAD32_WORD( "mpr17558.8",     0x400000, 0x200000, CRC(4b15f5a6) SHA1(9a34724958fef9b49eae39c6ea136e0cf532154b) )
        ROM_LOAD32_WORD( "mpr17559.9",     0x400002, 0x200000, CRC(d3264de6) SHA1(2f094ff0b95bf1cd5c283414634ea9597204d374) )
        ROM_LOAD32_WORD( "mpr17566.6",     0x800000, 0x200000, CRC(fb41ef98) SHA1(ad4d1ba5e5b39b2d87105ae80750284867aa4ed3) )
        ROM_LOAD32_WORD( "mpr17567.7",     0x800002, 0x200000, CRC(c3396922) SHA1(7e0700ded530e4eb58e9a68cdb92791284c91431) )
        ROM_LOAD32_WORD( "mpr17564.4",     0xc00000, 0x200000, CRC(d8062489) SHA1(57666b6937f79bb65c43ed02b04a454882d01e61) )
        ROM_LOAD32_WORD( "mpr17565.5",     0xc00002, 0x200000, CRC(0517c6e9) SHA1(d9ba93998286713758385033119416714674c8d8) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mpr17554.16",     0x000000, 0x200000, CRC(27896d82) SHA1(c0624e58de2e427465daaa10dbb02ea2a1fd0f1b) )
        ROM_LOAD32_WORD( "mpr17548.20",     0x000002, 0x200000, CRC(c95facc2) SHA1(09d19abe5d75a335df7510df8abb2d4425159cdf) )
        ROM_LOAD32_WORD( "mpr17555.17",     0x400000, 0x200000, CRC(4df2810b) SHA1(720c4628d7783f0323b5723b441e13741556241e) )
        ROM_LOAD32_WORD( "mpr17549.21",     0x400002, 0x200000, CRC(e0bce0e6) SHA1(0570604dc2007288795a3125ffd480bc4b3b0802) )
        ROM_LOAD32_WORD( "mpr17556.18",     0x800000, 0x200000, CRC(41a47616) SHA1(55b909d2bc2079d0dfed5036c78c9e09bce09843) )
        ROM_LOAD32_WORD( "mpr17550.22",     0x800002, 0x200000, CRC(c36ff3f5) SHA1(f14fdf275905a90a0d4cc534d90b0302f26676d8) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mpr17553.25",     0x000000, 0x200000, CRC(5da1c5d3) SHA1(c627b25a1f61a9fe9182e2199f70f6e485503c7b) )
        ROM_LOAD32_WORD( "mpr17552.24",     0x000002, 0x200000, CRC(e91e7427) SHA1(0ac1111f2ecb4f924b5119eaaac8fa7bc87ab9d1) )
        ROM_LOAD32_WORD( "mpr17547.27",     0x400000, 0x200000, CRC(be940431) SHA1(5c1196a6454a4fead79a930979f2e69639ec2bb9) )
        ROM_LOAD32_WORD( "mpr17546.26",     0x400002, 0x200000, CRC(042a194b) SHA1(c6d8524dc0a879394f1234b7bb04836081bb3830) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "mpr17574.30",     0x080000, 0x080000, CRC(4d4c3a55) SHA1(b6c0c3f0473bd7fc3ef4f5146110dfcc899a5af9) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr17573.31",     0x000000, 0x200000, CRC(e43557fe) SHA1(4c61a135819862df02347c118dc4d88a0adac273) )
        ROM_LOAD( "mpr17572.32",     0x200000, 0x200000, CRC(4febecc8) SHA1(9683ea9bedfc5cd7b4a28e9a68792c0dc549d911) )
        ROM_LOAD( "mpr17571.36",     0x400000, 0x200000, CRC(d22dc8ca) SHA1(3bba7780fa4ec23e40127a5b0104fba7d5e30b5f) )
        ROM_LOAD( "mpr17570.37",     0x600000, 0x200000, CRC(bccd324b) SHA1(4c7ebdea08b2dedf621f121785ed1c40ebae4236) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( srallyc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "17888.12",    0x000000, 0x080000, CRC(3d6808aa) SHA1(33abf9cdcee9583dc600c94e1e29ce260e8c5d32) )
        ROM_LOAD32_WORD( "17889.13",    0x000002, 0x080000, CRC(f43c7802) SHA1(4b1efb3d5644fed1753da1750bf5c300d3a15d2c) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "17746.bin",    0x000000, 0x200000, CRC(8fe311f4) SHA1(f4ada8e5c906fc384bed1b96f09cdf313f89e825) )
        ROM_LOAD32_WORD( "17747.bin",    0x000002, 0x200000, CRC(543593fd) SHA1(5ba63a77e9fc70569af21d50b3171bc8ff4522b8) )
        ROM_LOAD32_WORD( "17744.bin",    0x400000, 0x200000, CRC(71fed098) SHA1(1d187cad375121a45348d640edd3cc7dce658d28) )
        ROM_LOAD32_WORD( "17745.bin",    0x400002, 0x200000, CRC(8ecca705) SHA1(ed2b3298aad6f4e52dc672a0168183e457564b43) )
        ROM_LOAD32_WORD( "17884.bin",    0x800000, 0x200000, CRC(4cfc95e1) SHA1(81d927b8c4f9d0c4c5e29d676b30f30f83751fdc) )
        ROM_LOAD32_WORD( "17885.bin",    0x800002, 0x200000, CRC(a08d2467) SHA1(9449ac8f8f9ce8d8e536b05a91e46841fed7f2d0) )

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
        ROM_LOAD32_WORD( "17754.bin",    0x000000, 0x200000, CRC(81a84f67) SHA1(c0a9b690523a529e4015e9af10dc3fb2a1726f08) )
        ROM_LOAD32_WORD( "17755.bin",    0x000002, 0x200000, CRC(2a6e7da4) SHA1(e60803ae951489fe47d66731d15c32249ca547b4) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "17748.bin",    0x000000, 0x200000, CRC(3148a2b2) SHA1(283cc49bfb6c6381a7ead9273fd097dca5b981b6) )
        ROM_LOAD32_WORD( "17749.bin",    0x000002, 0x200000, CRC(0838d184) SHA1(704175c8b29e4c989afcb7be42e7e0e096740eaf) )
        ROM_LOAD32_WORD( "17750.bin",    0x400000, 0x200000, CRC(232aec29) SHA1(4d470e71df61298282c356814e2d151fda323fb6) )
        ROM_LOAD32_WORD( "17751.bin",    0x400002, 0x200000, CRC(ed87ac62) SHA1(601542149d33ca52a47536b4b0af47bf1fd87eb2) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "17752.bin",    0x000000, 0x200000, CRC(d6aa86ce) SHA1(1d342f87d1af1e5438d1ae818b1b14268e765897) )
        ROM_LOAD32_WORD( "17753.bin",    0x000002, 0x200000, CRC(6db0eb36) SHA1(dd5fd3c9592360d3e95623ac2491e6faabe9dbcb) )

	ROM_REGION( 0x20000, REGION_CPU5, 0) // Communication program
        ROM_LOAD( "16726.bin",    0x000000, 0x020000, CRC(c179b8c7) SHA1(86d3e65c77fb53b1d380b629348f4ab5b3d39228) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "17890.bin",    0x080000, 0x040000, CRC(5bac3fa1) SHA1(3635333d36463b6fab25560ed918e05138f964dc) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "17756.bin",    0x000000, 0x200000, CRC(7725f111) SHA1(1f1ee3f19a6bcf57bc5a1c7dd64ee83f8b81f084) )
        ROM_LOAD( "17757.bin",    0x000000, 0x200000, CRC(1616e649) SHA1(1d3a0e441d150ada0535a9d50e2f69dd4b99c584) )
        ROM_LOAD( "17886.bin",    0x000000, 0x200000, CRC(54a72923) SHA1(103c4838b27378c834c08d29d6fb6ba95e7f9d03) )
        ROM_LOAD( "17887.bin",    0x000000, 0x200000, CRC(38c31fdd) SHA1(a85f05160b060d9d4a431aaa73cfc03f24214fb9) )

	ROM_REGION( 0x010000, REGION_USER4, 0 ) // Unknown (on "amp" board)
        ROM_LOAD( "17891.bin",    0x000000, 0x010000, CRC(9a33b437) SHA1(3e8f210aa5159e78f640126cb5ce7f05f22560f2) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( manxtt )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "18822.bin",    0x000000, 0x020000, CRC(c7b3e45a) SHA1(d3a6910bf6efc138e0e40332219b90dea7d6ea56) )
        ROM_LOAD32_WORD( "18823.bin",    0x000002, 0x020000, CRC(6b0c1dfb) SHA1(6da5c071e3ce842a99f928f473d4ccf7165785ac) )
        ROM_LOAD32_WORD( "18824.bin",    0x040000, 0x020000, CRC(352bb817) SHA1(389cbf951ba606acb9ab7bff5cda85d9166e64ff) )
        ROM_LOAD32_WORD( "18825.bin",    0x040002, 0x020000, CRC(f88b036c) SHA1(f6196e8da5e6579fe3fa5c24ab9538964c98e267) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "18751.bin",    0x000000, 0x200000, CRC(773ad43d) SHA1(4d1601dc08a08b724e33e7cd90a4f22e18cfed9c) )
        ROM_LOAD32_WORD( "18752.bin",    0x000002, 0x200000, CRC(4da3719e) SHA1(24007e4ae3ba1a06321328d14e2bd6002fa1936e) )
        ROM_LOAD32_WORD( "18749.bin",    0x400000, 0x200000, CRC(c3fe0eea) SHA1(ada21405a136935ac4da1a3535c25fccf903f2d1) )
        ROM_LOAD32_WORD( "18750.bin",    0x400002, 0x200000, CRC(40b55494) SHA1(d98ae5518c5d31b155b1a7c4f7d9d67f44d7beae) )
        ROM_LOAD32_WORD( "18747.bin",    0x800000, 0x200000, CRC(f2ecac6f) SHA1(879d7b277e238d1d8713f2b604bc3db407aa249d) )
        ROM_LOAD32_WORD( "18748.bin",    0x800002, 0x200000, CRC(7815d634) SHA1(bfeff77630759c88b2191ea7e1ff78e0d70cbdf5) )
        ROM_LOAD32_WORD( "18862.bin",    0xc00000, 0x080000, CRC(9adc3a30) SHA1(029db946338f8e0eccace8590082cc96bdf13e31) )
        ROM_RELOAD     (                 0xd00000, 0x080000 )
        ROM_RELOAD     (                 0xe00000, 0x080000 )
        ROM_RELOAD     (                 0xf00000, 0x080000 )
        ROM_LOAD32_WORD( "18863.bin",    0xc00002, 0x080000, CRC(603742e9) SHA1(f78a5f7e582d313880c734158bb0fa68b256a58a) )
        ROM_RELOAD     (                 0xd00002, 0x080000 )
        ROM_RELOAD     (                 0xe00002, 0x080000 )
        ROM_RELOAD     (                 0xf00002, 0x080000 )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD( "18753.bin",    0x000000, 0x200000, CRC(33ddaa0d) SHA1(26f643d6b9cecf08bd249290a670a0edea1b5be4) )
        ROM_LOAD32_WORD( "18756.bin",    0x000002, 0x200000, CRC(e1678c59) SHA1(d55828c2ed9714c4e07455a8eed7fc39967a9124) )
        ROM_LOAD32_WORD( "18754.bin",    0x400000, 0x200000, CRC(09aabde5) SHA1(e50646efb2ca59792833ce91398c4efa861ad6d1) )
        ROM_LOAD32_WORD( "18757.bin",    0x400004, 0x200000, CRC(25fc92e9) SHA1(226c4c7289b3b6009c1ffea4a171e3fb4e31a67c) )
        ROM_LOAD32_WORD( "18755.bin",    0x000000, 0x200000, CRC(fd1fa53c) SHA1(d8dabbd6efcc2752879cce3df2cb99b65e043eca) )
        ROM_LOAD32_WORD( "18758.bin",    0x800002, 0x200000, CRC(1b5473d0) SHA1(658e33503f6990f4d9a954c63efad5f53d15f3a4) )

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
        ROM_LOAD32_WORD( "18761.bin",    0x000000, 0x200000, CRC(4e39ec05) SHA1(50696cd320f1a6492e0c193713acbce085d959cd) )
        ROM_LOAD32_WORD( "18762.bin",    0x000002, 0x200000, CRC(4ab165d8) SHA1(7ff42a4c7236fec76f94f2d0c5537e503bcc98e5) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "18759.bin",    0x000000, 0x200000, CRC(89d7927d) SHA1(6908de9cbb71b36fc7dd35aace6bfd61408bb30b) )
        ROM_LOAD32_WORD( "18760.bin",    0x000002, 0x200000, CRC(4e3a4a89) SHA1(bba6cd2a15b3f963388a3a87880da86b10f6e0a2) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "epr18826.030", 0x080000, 0x040000, CRC(ed9fe4c1) SHA1(c3dd8a1324a4dc9b012bd9bf21d1f48578870f72) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr18827.031", 0x000000, 0x200000, CRC(58d78ca1) SHA1(95275ed8315c044bfde2f23c10416f22627b34df) )
        ROM_LOAD( "mpr18764.032", 0x200000, 0x200000, CRC(0dc6a860) SHA1(cb2ada0f8a592940de11ee781ad4beb5095c3b37) )
        ROM_LOAD( "mpr18765.036", 0x400000, 0x200000, CRC(ca4a803c) SHA1(70b59da8f2532a02e980caba5bb86ec13a4d7ab5) )
        ROM_LOAD( "mpr18766.037", 0x600000, 0x200000, CRC(10f3c932) SHA1(5f6da9825b70c1ea83a421e35038042fb17862ec) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( skytargt )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "ep18406.12",   0x000000, 0x080000, CRC(fde9c00a) SHA1(01cd519daaf6138d9df4940bf8bb5923a1f163df) )
        ROM_LOAD32_WORD( "ep18407.13",   0x000002, 0x080000, CRC(35f8b529) SHA1(faf6dcf8f345c1e7968823f2dba60afcd88f37c2) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mp18415.10",   0x000000, 0x400000, CRC(d7a1bbd7) SHA1(3061cc68755ca36255f325135aa44659afc3498c) )
        ROM_LOAD32_WORD( "mp18416.11",   0x000002, 0x400000, CRC(b77c9243) SHA1(6ffeef418364df9e08398c7564142cbf5750beb2) )
        ROM_LOAD32_WORD( "mp18417.8",    0x800000, 0x400000, CRC(a0d03f63) SHA1(88b97a76f0a85a3977915808eee4d64b69734e88) )
        ROM_LOAD32_WORD( "mp18418.9",    0x800002, 0x400000, CRC(c7a6f97f) SHA1(cf7c6887519e53d7fa321a2ad888b1673e16565b) )
        ROM_LOAD32_WORD( "ep18404.6",    0x1000000, 0x080000, CRC(f1407ec4) SHA1(d6805faea657ea0f998fb2470d7d24aa78a02bd4) )
	ROM_RELOAD     (               0x1100000, 0x080000)
	ROM_RELOAD     (               0x1200000, 0x080000)
	ROM_RELOAD     (               0x1300000, 0x080000)
	ROM_RELOAD     (               0x1400000, 0x080000)
	ROM_RELOAD     (               0x1500000, 0x080000)
	ROM_RELOAD     (               0x1600000, 0x080000)
	ROM_RELOAD     (               0x1700000, 0x080000)
	ROM_RELOAD     (               0x1800000, 0x080000)
	ROM_RELOAD     (               0x1900000, 0x080000)
	ROM_RELOAD     (               0x1a00000, 0x080000)
	ROM_RELOAD     (               0x1b00000, 0x080000)
	ROM_RELOAD     (               0x1c00000, 0x080000)
	ROM_RELOAD     (               0x1d00000, 0x080000)
	ROM_RELOAD     (               0x1e00000, 0x080000)
	ROM_RELOAD     (               0x1f00000, 0x080000)
        ROM_LOAD32_WORD( "ep18405.7",    0x1000002, 0x080000, CRC(00b40f9e) SHA1(21b6b390d8635349ba76899acea176954a24985e) )
	ROM_RELOAD     (               0x1100002, 0x080000)
	ROM_RELOAD     (               0x1200002, 0x080000)
	ROM_RELOAD     (               0x1300002, 0x080000)
	ROM_RELOAD     (               0x1400002, 0x080000)
	ROM_RELOAD     (               0x1500002, 0x080000)
	ROM_RELOAD     (               0x1600002, 0x080000)
	ROM_RELOAD     (               0x1700002, 0x080000)
	ROM_RELOAD     (               0x1800002, 0x080000)
	ROM_RELOAD     (               0x1900002, 0x080000)
	ROM_RELOAD     (               0x1a00002, 0x080000)
	ROM_RELOAD     (               0x1b00002, 0x080000)
	ROM_RELOAD     (               0x1c00002, 0x080000)
	ROM_RELOAD     (               0x1d00002, 0x080000)
	ROM_RELOAD     (               0x1e00002, 0x080000)
	ROM_RELOAD     (               0x1f00002, 0x080000)

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
        ROM_LOAD32_WORD( "mp18420.28",   0x000000, 0x200000, CRC(92b87817) SHA1(b6949b745d0bedeecd6d0240f8911cb345c16d8d) )
        ROM_LOAD32_WORD( "mp18419.29",   0x000002, 0x200000, CRC(74542d87) SHA1(37230e96dd526fb47fcbde5778e5466d8955a969) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mp18413.16",   0x000000, 0x400000, CRC(1c4d416c) SHA1(2bd6eae4ab5751d485be105a06776fccd3c48d21) )
        ROM_LOAD32_WORD( "mp18414.17",   0x000002, 0x400000, CRC(858885ba) SHA1(1729f6ff689a462a3d6e303ebc2dac323145a67c) )
        ROM_LOAD32_WORD( "mp18409.20",   0x800000, 0x400000, CRC(666037ef) SHA1(6f622a82fd5ffd7a4692b5bf51b76755053a674b) )
        ROM_LOAD32_WORD( "mp18410.21",   0x800002, 0x400000, CRC(b821a695) SHA1(139cbba0ceffa83c0f9925258944ec8a414b3040) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mp18411.24",   0x000000, 0x400000, CRC(9c2dc40c) SHA1(842a647a70ef29a8c775e88c0bcbc63782496bba) )
        ROM_LOAD32_WORD( "mp18412.25",   0x000002, 0x400000, CRC(4db52f8b) SHA1(66796f6c20e680a87e8939a70692680b1dd0b324) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "ep18408.30",   0x080000, 0x080000, CRC(6deb9657) SHA1(30e1894432a0765c64b93dd5ca7ca17ef58ac6c0) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mp18424.31",   0x000000, 0x200000, CRC(590a4338) SHA1(826f167d7a4f5d30466b2f75f0123187c29c2d69) )
        ROM_LOAD( "mp18423.32",   0x200000, 0x200000, CRC(c356d765) SHA1(ae69c9d4e333579d826178d2863156dc784aedef) )
        ROM_LOAD( "mp18422.36",   0x400000, 0x200000, CRC(b4f3cea6) SHA1(49669be09e10dfae7fddce0fc4e415466cb29566) )
        ROM_LOAD( "mp18421.37",   0x600000, 0x200000, CRC(00522390) SHA1(5dbbf2ba008adad36929fcecb7c2c1e5ffd12618) )
ROM_END

ROM_START( vcop2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "18524.12",    0x000000, 0x080000, CRC(1858988b) SHA1(2979f8470cc31e6c5c32c6fec1a87dbd29b52309) )
        ROM_LOAD32_WORD( "18525.13",    0x000002, 0x080000, CRC(0c13df3f) SHA1(6b4188f04aad80b89f1826e8ca47cff763980410) )
        ROM_LOAD32_WORD( "18518.14",    0x100000, 0x080000, CRC(7842951b) SHA1(bed4ec9a5e59807d17e5e602bdaf3c68fcba08b6) )
        ROM_LOAD32_WORD( "18519.15",    0x100002, 0x080000, CRC(31a30edc) SHA1(caf3c2676508a2ed032d3657ac640a257f04bdd4) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "18516.10",   0x000000, 0x200000, CRC(a3928ff0) SHA1(5a9695fb5eda394a1111a05ee5fb9cce29970e91) )
        ROM_LOAD32_WORD( "18517.11",   0x000002, 0x200000, CRC(4bd73da4) SHA1(a4434bce019729e2148a95e3a6dea38de7f789c1) )
        ROM_LOAD32_WORD( "18514.8",    0x400000, 0x200000, CRC(791283c5) SHA1(006fb22eefdd9205ede9a74fe53cbffe8c8fd45b) )
        ROM_LOAD32_WORD( "18515.9",    0x400002, 0x200000, CRC(6ba1ffec) SHA1(70f493aa4eb93edce8dd5b7b532d1f50f81069ce) )
        ROM_LOAD32_WORD( "18522.6",    0x800000, 0x200000, CRC(61d18536) SHA1(cc467cb26a8fccc48837d000fe9e1c41b0c0f4f9) )
        ROM_LOAD32_WORD( "18523.7",    0x800002, 0x200000, CRC(61d08dc4) SHA1(40d8231d184582c0fc01ad874371aaec7dfcc337) )
        ROM_LOAD32_WORD( "18520.4",    0xc00000, 0x080000, CRC(1d4ec5e8) SHA1(44c4b5560d150909342e4182496f136c8c5e2edb) )
        ROM_RELOAD     (               0xd00000, 0x080000 )
        ROM_RELOAD     (               0xe00000, 0x080000 )
        ROM_RELOAD     (               0xf00000, 0x080000 )
        ROM_LOAD32_WORD( "18521.5",    0xc00002, 0x080000, CRC(b8b3781c) SHA1(11956fe912c34d6a86a6b91d55987f6bead73473) )
        ROM_RELOAD     (               0xd00002, 0x080000 )
        ROM_RELOAD     (               0xe00002, 0x080000 )
        ROM_RELOAD     (               0xf00002, 0x080000 )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "18513.16",     0x000000, 0x200000, CRC(777a3633) SHA1(edc2798c4d88975ce67b54fc0db008e7d24db6ef) )
        ROM_LOAD32_WORD( "18510.20",     0x000002, 0x200000, CRC(083f5186) SHA1(0f9d5c6e18bf3ae5d6bc195f513668286aa23478) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "18511.24",     0x000000, 0x200000, CRC(cae77a4f) SHA1(f21474486f0dc4092cbad4566deea8a952862ab7) )
        ROM_LOAD32_WORD( "18512.25",     0x000002, 0x200000, CRC(d9bc7e71) SHA1(774eba886083b0dad9a47519c5801e44346312cf) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "18530.30",    0x080000, 0x080000, CRC(ac9c8357) SHA1(ad297c7fecaa9b877f0dd31e859983816947e437) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "18529.31",     0x000000, 0x200000, CRC(f76715b1) SHA1(258418c1cb37338a694e48f3b48fadfae5f40239) )
        ROM_LOAD( "18528.32",     0x200000, 0x200000, CRC(287a2f9a) SHA1(78ba93ab90322152efc37f7130073b0dc516ef5d) )
        ROM_LOAD( "18527.36",     0x400000, 0x200000, CRC(e6a49314) SHA1(26563f425f2f0906ae9278fe5de02955653d49fe) )
        ROM_LOAD( "18526.37",     0x600000, 0x200000, CRC(6516d9b5) SHA1(8f13cb02c76f7b7cd11f3c3772ff13302d55e9c3) )

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( dynamcop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep20930.12",   0x000000, 0x080000, CRC(b8fc8ff7) SHA1(53b0f9dc8494effa077170ddced2d95f43a5f134))
	ROM_LOAD32_WORD("ep20931.13",   0x000002, 0x080000, CRC(89d13f88) SHA1(5e266b5e153a0d9a57360cfd1af81e3a58a2fb7d))
	ROM_LOAD32_WORD("ep20932.14",   0x100000, 0x080000, CRC(618a68bf) SHA1(3022283dded4d08d790d034b6d543c0397b5bf5a))
	ROM_LOAD32_WORD("ep20933.15",   0x100002, 0x080000, CRC(13abe49c) SHA1(a741a0205c1b3664ab4d09d6d991a768269a79ea))

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20797.10",   0x000000, 0x400000, CRC(87bab1e4) SHA1(af2b2d82364621a1528d6ed59fbfbf15dc93ee72))
	ROM_LOAD32_WORD("mp20798.11",   0x000002, 0x400000, CRC(40dd752b) SHA1(8c2e210ac7c7b133ba9befc79a07c4ca6b4e3f18))
	ROM_LOAD32_WORD("mp20795.8",    0x800000, 0x400000, CRC(0ef85e12) SHA1(97c657edd98cde6f0780a04a7711e5b370087a60))
	ROM_LOAD32_WORD("mp20796.9",    0x800002, 0x400000, CRC(870139cb) SHA1(24fda2cd458cf7a3db485564c02ac61d30cbdf5e))
	ROM_LOAD32_WORD("mp20793.6",   0x1000000, 0x400000, CRC(42ea08f8) SHA1(e70b55709067628ea0bf3f5190a300100b61eed1))
	ROM_LOAD32_WORD("mp20794.7",   0x1000002, 0x400000, CRC(8e5cd1db) SHA1(d90e86d38bda12f2d0f99e23a42928f05bde3ea8))
	ROM_LOAD32_WORD("mp20791.4",   0x1800000, 0x400000, CRC(4883d0df) SHA1(b98af63e81f6c1b2766d7e96acbd1821bba000d4))
	ROM_LOAD32_WORD("mp20792.5",   0x1800002, 0x400000, CRC(47becfa2) SHA1(a333885872a64b322f3cb464a70352d73654b1b3))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20799.16",   0x000000, 0x400000, CRC(424571bf) SHA1(18a4e8d0e968fff3b645b59a0023b0ef38d51924))
	ROM_LOAD32_WORD("mp20803.20",   0x000002, 0x400000, CRC(61a8ad52) SHA1(0215b5de6d10f0852ac0ca4e10475e10243e39c7))
	ROM_LOAD32_WORD("mp20800.17",   0x800000, 0x400000, CRC(3c2ee808) SHA1(dc0c470c6b410ab991ef0e09ce1cc0f63c8a204d))
	ROM_LOAD32_WORD("mp20804.21",   0x800002, 0x400000, CRC(03b35cb8) SHA1(7bd2ae89f9cc7c0570dbaffe5f54aea2dfa1b39e))
	ROM_LOAD32_WORD("mp20801.18",  0x1000000, 0x400000, CRC(c6914173) SHA1(d0861366c4123c833a325df5345f951386a94d1a))
	ROM_LOAD32_WORD("mp20805.22",  0x1000002, 0x400000, CRC(f6605ede) SHA1(7c95bfe2e95bae3d59c3c9efe1f40b5bc292ad44))
	ROM_LOAD32_WORD("mp20802.19",  0x1800000, 0x400000, CRC(d11b5267) SHA1(b90909849fbe0f62d5ec7c38608c84e7fa845ebf))
	ROM_LOAD32_WORD("mp20806.23",  0x1800002, 0x400000, CRC(0c942073) SHA1(5f32a56857e2213b110c32deea184dba882e34b8))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20809.25",  0x0000000, 0x400000, CRC(3b7b4622) SHA1(c6f1a1fd2684f352d3846b7f859b0405fa2d667a))
	ROM_LOAD32_WORD("mp20807.24",  0x0000002, 0x400000, CRC(1241e0f2) SHA1(3f7fa1d7d3d398bc8d5295bc1df6fe11405d20d9))
	ROM_LOAD32_WORD("mp20810.27",  0x0800000, 0x400000, CRC(838a10a7) SHA1(a658f1864829058b1d419e7c001e47cd0ab06a20))
	ROM_LOAD32_WORD("mp20808.26",  0x0800002, 0x400000, CRC(706bd495) SHA1(f857b303afda6301b19d97dfe5c313126261716e))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20811.30",   0x080000,  0x80000, CRC(a154b83e) SHA1(2640c6b6966f4a888329e583b6b713bd0e779b6b))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20812.31",   0x000000, 0x200000, CRC(695b6088) SHA1(09682d18144e60d740a9d7a3e19db6f76fa581f1))
	ROM_LOAD("mp20813.32",   0x200000, 0x200000, CRC(1908679c) SHA1(32913385f09da2e43af0c4a4612b955527bfe759))
	ROM_LOAD("mp20814.36",   0x400000, 0x200000, CRC(e8ebc74c) SHA1(731ce721bb9e148f3a9f7fbe569522567a681c4e))
	ROM_LOAD("mp20815.37",   0x600000, 0x200000, CRC(1b5aaae4) SHA1(32b4bf6c096fdccdd5d8f1ddb6c27d3389a52234))

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( dyndeka2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("epr-20922.12",  0x000000, 0x080000, CRC(0a8b5604) SHA1(4076998fc600c1df3bb5ef48d42681c01e651495))
	ROM_LOAD32_WORD("epr-20923.13",  0x000002, 0x080000, CRC(83be73d4) SHA1(1404a9c79cd2bae13f60e5e008307417324c3666))
	ROM_LOAD32_WORD("epr-20924.14",  0x100000, 0x080000, CRC(618a68bf) SHA1(3022283dded4d08d790d034b6d543c0397b5bf5a)) // == ep20932.14
	ROM_LOAD32_WORD("epr-20925.15",  0x100002, 0x080000, CRC(13abe49c) SHA1(a741a0205c1b3664ab4d09d6d991a768269a79ea)) // == ep20933.15

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20797.10",   0x000000, 0x400000, CRC(87bab1e4) SHA1(af2b2d82364621a1528d6ed59fbfbf15dc93ee72))
	ROM_LOAD32_WORD("mp20798.11",   0x000002, 0x400000, CRC(40dd752b) SHA1(8c2e210ac7c7b133ba9befc79a07c4ca6b4e3f18))
	ROM_LOAD32_WORD("mp20795.8",    0x800000, 0x400000, CRC(0ef85e12) SHA1(97c657edd98cde6f0780a04a7711e5b370087a60))
	ROM_LOAD32_WORD("mp20796.9",    0x800002, 0x400000, CRC(870139cb) SHA1(24fda2cd458cf7a3db485564c02ac61d30cbdf5e))
	ROM_LOAD32_WORD("mp20793.6",   0x1000000, 0x400000, CRC(42ea08f8) SHA1(e70b55709067628ea0bf3f5190a300100b61eed1))
	ROM_LOAD32_WORD("mp20794.7",   0x1000002, 0x400000, CRC(8e5cd1db) SHA1(d90e86d38bda12f2d0f99e23a42928f05bde3ea8))
	ROM_LOAD32_WORD("mp20791.4",   0x1800000, 0x400000, CRC(4883d0df) SHA1(b98af63e81f6c1b2766d7e96acbd1821bba000d4))
	ROM_LOAD32_WORD("mp20792.5",   0x1800002, 0x400000, CRC(47becfa2) SHA1(a333885872a64b322f3cb464a70352d73654b1b3))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20799.16",   0x000000, 0x400000, CRC(424571bf) SHA1(18a4e8d0e968fff3b645b59a0023b0ef38d51924))
	ROM_LOAD32_WORD("mp20803.20",   0x000002, 0x400000, CRC(61a8ad52) SHA1(0215b5de6d10f0852ac0ca4e10475e10243e39c7))
	ROM_LOAD32_WORD("mp20800.17",   0x800000, 0x400000, CRC(3c2ee808) SHA1(dc0c470c6b410ab991ef0e09ce1cc0f63c8a204d))
	ROM_LOAD32_WORD("mp20804.21",   0x800002, 0x400000, CRC(03b35cb8) SHA1(7bd2ae89f9cc7c0570dbaffe5f54aea2dfa1b39e))
	ROM_LOAD32_WORD("mp20801.18",  0x1000000, 0x400000, CRC(c6914173) SHA1(d0861366c4123c833a325df5345f951386a94d1a))
	ROM_LOAD32_WORD("mp20805.22",  0x1000002, 0x400000, CRC(f6605ede) SHA1(7c95bfe2e95bae3d59c3c9efe1f40b5bc292ad44))
	ROM_LOAD32_WORD("mp20802.19",  0x1800000, 0x400000, CRC(d11b5267) SHA1(b90909849fbe0f62d5ec7c38608c84e7fa845ebf))
	ROM_LOAD32_WORD("mp20806.23",  0x1800002, 0x400000, CRC(0c942073) SHA1(5f32a56857e2213b110c32deea184dba882e34b8))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20809.25",  0x0000000, 0x400000, CRC(3b7b4622) SHA1(c6f1a1fd2684f352d3846b7f859b0405fa2d667a))
	ROM_LOAD32_WORD("mp20807.24",  0x0000002, 0x400000, CRC(1241e0f2) SHA1(3f7fa1d7d3d398bc8d5295bc1df6fe11405d20d9))
	ROM_LOAD32_WORD("mp20810.27",  0x0800000, 0x400000, CRC(838a10a7) SHA1(a658f1864829058b1d419e7c001e47cd0ab06a20))
	ROM_LOAD32_WORD("mp20808.26",  0x0800002, 0x400000, CRC(706bd495) SHA1(f857b303afda6301b19d97dfe5c313126261716e))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20811.30",   0x080000,  0x80000, CRC(a154b83e) SHA1(2640c6b6966f4a888329e583b6b713bd0e779b6b))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20812.31",   0x000000, 0x200000, CRC(695b6088) SHA1(09682d18144e60d740a9d7a3e19db6f76fa581f1))
	ROM_LOAD("mp20813.32",   0x200000, 0x200000, CRC(1908679c) SHA1(32913385f09da2e43af0c4a4612b955527bfe759))
	ROM_LOAD("mp20814.36",   0x400000, 0x200000, CRC(e8ebc74c) SHA1(731ce721bb9e148f3a9f7fbe569522567a681c4e))
	ROM_LOAD("mp20815.37",   0x600000, 0x200000, CRC(1b5aaae4) SHA1(32b4bf6c096fdccdd5d8f1ddb6c27d3389a52234))

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( dynmcopb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("progrom0.15",  0x000000, 0x080000, CRC(29b142f2) SHA1(b81d1ee7203b2f5fb6db4ff4185f4071e99aaedf))
	ROM_LOAD32_WORD("progrom1.16",  0x000002, 0x080000, CRC(c495912e) SHA1(1a45296a5554923cb52b38586e40ceda2517f1bf))
	ROM_LOAD32_WORD("ep20932.14",   0x100000, 0x080000, CRC(618a68bf) SHA1(3022283dded4d08d790d034b6d543c0397b5bf5a))
	ROM_LOAD32_WORD("ep20933.15",   0x100002, 0x080000, CRC(13abe49c) SHA1(a741a0205c1b3664ab4d09d6d991a768269a79ea))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20797.10",   0x000000, 0x400000, CRC(87bab1e4) SHA1(af2b2d82364621a1528d6ed59fbfbf15dc93ee72))
	ROM_LOAD32_WORD("mp20798.11",   0x000002, 0x400000, CRC(40dd752b) SHA1(8c2e210ac7c7b133ba9befc79a07c4ca6b4e3f18))
	ROM_LOAD32_WORD("mp20795.8",    0x800000, 0x400000, CRC(0ef85e12) SHA1(97c657edd98cde6f0780a04a7711e5b370087a60))
	ROM_LOAD32_WORD("mp20796.9",    0x800002, 0x400000, CRC(870139cb) SHA1(24fda2cd458cf7a3db485564c02ac61d30cbdf5e))
	ROM_LOAD32_WORD("mp20793.6",   0x1000000, 0x400000, CRC(42ea08f8) SHA1(e70b55709067628ea0bf3f5190a300100b61eed1))
	ROM_LOAD32_WORD("mp20794.7",   0x1000002, 0x400000, CRC(8e5cd1db) SHA1(d90e86d38bda12f2d0f99e23a42928f05bde3ea8))
	ROM_LOAD32_WORD("mp20791.4",   0x1800000, 0x400000, CRC(4883d0df) SHA1(b98af63e81f6c1b2766d7e96acbd1821bba000d4))
	ROM_LOAD32_WORD("mp20792.5",   0x1800002, 0x400000, CRC(47becfa2) SHA1(a333885872a64b322f3cb464a70352d73654b1b3))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20799.16",   0x000000, 0x400000, CRC(424571bf) SHA1(18a4e8d0e968fff3b645b59a0023b0ef38d51924))
	ROM_LOAD32_WORD("mp20803.20",   0x000002, 0x400000, CRC(61a8ad52) SHA1(0215b5de6d10f0852ac0ca4e10475e10243e39c7))
	ROM_LOAD32_WORD("mp20800.17",   0x800000, 0x400000, CRC(3c2ee808) SHA1(dc0c470c6b410ab991ef0e09ce1cc0f63c8a204d))
	ROM_LOAD32_WORD("mp20804.21",   0x800002, 0x400000, CRC(03b35cb8) SHA1(7bd2ae89f9cc7c0570dbaffe5f54aea2dfa1b39e))
	ROM_LOAD32_WORD("mp20801.18",  0x1000000, 0x400000, CRC(c6914173) SHA1(d0861366c4123c833a325df5345f951386a94d1a))
	ROM_LOAD32_WORD("mp20805.22",  0x1000002, 0x400000, CRC(f6605ede) SHA1(7c95bfe2e95bae3d59c3c9efe1f40b5bc292ad44))
	ROM_LOAD32_WORD("mp20802.19",  0x1800000, 0x400000, CRC(d11b5267) SHA1(b90909849fbe0f62d5ec7c38608c84e7fa845ebf))
	ROM_LOAD32_WORD("mp20806.23",  0x1800002, 0x400000, CRC(0c942073) SHA1(5f32a56857e2213b110c32deea184dba882e34b8))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20809.25",  0x0000000, 0x400000, CRC(3b7b4622) SHA1(c6f1a1fd2684f352d3846b7f859b0405fa2d667a))
	ROM_LOAD32_WORD("mp20807.24",  0x0000002, 0x400000, CRC(1241e0f2) SHA1(3f7fa1d7d3d398bc8d5295bc1df6fe11405d20d9))
	ROM_LOAD32_WORD("mp20810.27",  0x0800000, 0x400000, CRC(838a10a7) SHA1(a658f1864829058b1d419e7c001e47cd0ab06a20))
	ROM_LOAD32_WORD("mp20808.26",  0x0800002, 0x400000, CRC(706bd495) SHA1(f857b303afda6301b19d97dfe5c313126261716e))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep20811.30",   0x080000,  0x80000, CRC(a154b83e) SHA1(2640c6b6966f4a888329e583b6b713bd0e779b6b))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20812.31",   0x000000, 0x200000, CRC(695b6088) SHA1(09682d18144e60d740a9d7a3e19db6f76fa581f1))
	ROM_LOAD("mp20813.32",   0x200000, 0x200000, CRC(1908679c) SHA1(32913385f09da2e43af0c4a4612b955527bfe759))
	ROM_LOAD("mp20814.36",   0x400000, 0x200000, CRC(e8ebc74c) SHA1(731ce721bb9e148f3a9f7fbe569522567a681c4e))
	ROM_LOAD("mp20815.37",   0x600000, 0x200000, CRC(1b5aaae4) SHA1(32b4bf6c096fdccdd5d8f1ddb6c27d3389a52234))
ROM_END

ROM_START( schamp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19001.15",   0x000000, 0x080000, CRC(9b088511) SHA1(20718d985d14f4d2b1b8e982bfbebddd73cdb972))
	ROM_LOAD32_WORD("ep19002.16",   0x000002, 0x080000, CRC(46f510da) SHA1(edcbf61122db568ccaa4c3106f507087c1740c9b))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19007.11",   0x000000, 0x400000, CRC(8b8ff751) SHA1(5343a9a2502052e3587424c984bd48caa7564849))
	ROM_LOAD32_WORD("mp19008.12",   0x000002, 0x400000, CRC(a94654f5) SHA1(39ad2e9431543ea6cbc0307bc39933cf64956a74))
	ROM_LOAD32_WORD("mp19005.9",    0x800000, 0x400000, CRC(98cd1127) SHA1(300c9cdef199f31255bacb95399e9c75be73f817))
	ROM_LOAD32_WORD("mp19006.10",   0x800002, 0x400000, CRC(e79f0a26) SHA1(37a4ff13cfccfda587ca59a9ef08b5914d2c28d4))
	ROM_LOAD32_WORD("ep19003.7",   0x1000000, 0x080000, CRC(63bae5c5) SHA1(cbd55b7b7376ac2f67befaf4c43eef3727ba7b7f))
	ROM_RELOAD     (               0x1100000, 0x080000)
	ROM_RELOAD     (               0x1200000, 0x080000)
	ROM_RELOAD     (               0x1300000, 0x080000)
	ROM_RELOAD     (               0x1400000, 0x080000)
	ROM_RELOAD     (               0x1500000, 0x080000)
	ROM_RELOAD     (               0x1600000, 0x080000)
	ROM_RELOAD     (               0x1700000, 0x080000)
	ROM_RELOAD     (               0x1800000, 0x080000)
	ROM_RELOAD     (               0x1900000, 0x080000)
	ROM_RELOAD     (               0x1a00000, 0x080000)
	ROM_RELOAD     (               0x1b00000, 0x080000)
	ROM_RELOAD     (               0x1c00000, 0x080000)
	ROM_RELOAD     (               0x1d00000, 0x080000)
	ROM_RELOAD     (               0x1e00000, 0x080000)
	ROM_RELOAD     (               0x1f00000, 0x080000)
	ROM_LOAD32_WORD("ep19004.8",   0x1000002, 0x080000, CRC(c10c9f39) SHA1(cf806501dbfa48d16cb7ed5f39a6146f734ba455))
	ROM_RELOAD     (               0x1100002, 0x080000)
	ROM_RELOAD     (               0x1200002, 0x080000)
	ROM_RELOAD     (               0x1300002, 0x080000)
	ROM_RELOAD     (               0x1400002, 0x080000)
	ROM_RELOAD     (               0x1500002, 0x080000)
	ROM_RELOAD     (               0x1600002, 0x080000)
	ROM_RELOAD     (               0x1700002, 0x080000)
	ROM_RELOAD     (               0x1800002, 0x080000)
	ROM_RELOAD     (               0x1900002, 0x080000)
	ROM_RELOAD     (               0x1a00002, 0x080000)
	ROM_RELOAD     (               0x1b00002, 0x080000)
	ROM_RELOAD     (               0x1c00002, 0x080000)
	ROM_RELOAD     (               0x1d00002, 0x080000)
	ROM_RELOAD     (               0x1e00002, 0x080000)
	ROM_RELOAD     (               0x1f00002, 0x080000)

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp19015.29",   0x000000, 0x200000, CRC(c74d99e3) SHA1(9914be9925b86af6af670745b5eba3a9e4f24af9))
	ROM_LOAD32_WORD("mp19016.30",   0x000002, 0x200000, CRC(746ae931) SHA1(a6f0f589ad174a34493ee24dc0cb509ead3aed70))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19009.17",   0x000000, 0x400000, CRC(fd410350) SHA1(5af3a90c87ec8a90a8fc58ae469ef23ec6e6213c))
	ROM_LOAD32_WORD("mp19012.21",   0x000002, 0x400000, CRC(9bb7b5b6) SHA1(8e13a0bb34e187a340b38d76ab15ff6fe4bae764))
	ROM_LOAD32_WORD("mp19010.18",   0x800000, 0x400000, CRC(6fd94187) SHA1(e3318ef0eb0168998e139e527339c7c667c17fb1))
	ROM_LOAD32_WORD("mp19013.22",   0x800002, 0x400000, CRC(9e232fe5) SHA1(a6c4b2b3bf8efc6f6263f73d6f4cacf9785010c1))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19019.27",   0x000000, 0x400000, CRC(59121896) SHA1(c29bedb41b14d63c6067ae12ad009deaafca2aa4))
	ROM_LOAD32_WORD("mp19017.25",   0x000002, 0x400000, CRC(7b298379) SHA1(52fad61412040c90c7dd300c0fd7aa5b8d5af441))
	ROM_LOAD32_WORD("mp19020.28",   0x800000, 0x400000, CRC(9540dba0) SHA1(7b9a75caa8c5b12ba54c6f4f746d80b165ee97ab))
	ROM_LOAD32_WORD("mp19018.26",   0x800002, 0x400000, CRC(3b7e7a12) SHA1(9c707a7c2cffc5eff19f9919ddfae7300842fd19))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19021.31",   0x080000,  0x80000, CRC(0b9f7583) SHA1(21290389cd8bd9e52ed438152cc6cb5793f809d3))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19022.32",   0x000000, 0x200000, CRC(4381869b) SHA1(43a21609b49926a227558d4938088526acf1fe42))
	ROM_LOAD("mp19023.33",   0x200000, 0x200000, CRC(07c67f88) SHA1(696dc85e066fb27c7618e52e0acd0d00451e4589))
	ROM_LOAD("mp19024.34",   0x400000, 0x200000, CRC(15ff76d3) SHA1(b431bd85c973aa0a4d6032ac98fb057139f142a2))
	ROM_LOAD("mp19025.35",   0x600000, 0x200000, CRC(6ad8fb70) SHA1(b666d31f9be26eb0cdcb71041a3c3c08d5aa41e1))
ROM_END

ROM_START( stcc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19272a.15",  0x000000, 0x080000, CRC(20cedd05) SHA1(e465967c784de18caaaac77e164796e9779f576a))
	ROM_LOAD32_WORD("ep19273a.16",  0x000002, 0x080000, CRC(1b0ab4d6) SHA1(142bcd53fa6632fcc866bbda817aa83470111ef1))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19257.11",   0x000000, 0x400000, CRC(ac28ee24) SHA1(31d360dc435336942f70365d0491a2ccfc24c4c0))
	ROM_LOAD32_WORD("mp19258.12",   0x000002, 0x400000, CRC(f5ba7d78) SHA1(9c8304a1f856d1ded869ed2b86de52129510f019))
	ROM_LOAD32_WORD("ep19270.9",    0x800000, 0x080000, CRC(7bd1d04e) SHA1(0490f3abc97af16e05f0dc9623e8fc635b1d4262))
	ROM_LOAD32_WORD("ep19271.10",   0x800002, 0x080000, CRC(d2d74f85) SHA1(49e7a1e6478122b4f0e679d7b336fb34044b503b))
	ROM_COPY(REGION_USER1, 0x800000,0x900000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xa00000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xb00000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xc00000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xd00000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xe00000,0x100000)
	ROM_COPY(REGION_USER1, 0x800000,0xf00000,0x100000)

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp19255.29",   0x000000, 0x200000, CRC(d78bf030) SHA1(e6b3d8422613d22db50cf6c251f9a21356d96653))
	ROM_LOAD32_WORD("mp19256.30",   0x000002, 0x200000, CRC(cb2b2d9e) SHA1(86b2b8bb6074352f72eb81e616093a1ba6f5163f))

	ROM_REGION( 0x900000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19251.17",   0x000000, 0x400000, CRC(e06ff7ba) SHA1(6d472d03cd3caeb66be929c74ae63c32d305a3db))
	ROM_LOAD32_WORD("mp19252.21",   0x000002, 0x400000, CRC(68509993) SHA1(654d5cdf44e7e1e788b26593f418ce76a5c1165a))
	ROM_LOAD32_WORD("ep19266.18",   0x800000, 0x080000, CRC(41464ee2) SHA1(afbbc0328bd36c34c69f0f54404dfd6a64036417))
	ROM_LOAD32_WORD("ep19267.22",   0x800002, 0x080000, CRC(780f994d) SHA1(f134482ed0fcfc7b3eea39947da47081301a111a))

	ROM_REGION( 0x900000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19254.27",   0x000000, 0x200000, CRC(1ec49c02) SHA1(a9bdbab7b4b265c9118cf27fd45ca94f4516d5c6))
	ROM_RELOAD     (                0x400000, 0x200000 )
	ROM_LOAD32_WORD("mp19253.25",   0x000002, 0x200000, CRC(41ba79fb) SHA1(f4d8a4f8278eec6d528bd947b91ebeb5223559d5))
	ROM_RELOAD     (                0x400002, 0x200000 )
	ROM_LOAD32_WORD("ep19269.28",   0x800000, 0x080000, CRC(01881121) SHA1(fe711709e70b3743b2a0318b823d859f233d3ff8))
	ROM_LOAD32_WORD("ep19268.26",   0x800002, 0x080000, CRC(bc4e081c) SHA1(b89d39ed19a146d1e94e52682f67d2cd23d8df7f))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19274.31",   0x080000,  0x20000, CRC(2dcc08ae) SHA1(bad26e2c994f2d4db5d9be0e34cf21a8bf5aa7e9))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // DSB program
	ROM_LOAD16_WORD_SWAP("ep19275.2s",   0x000000,  0x20000, CRC(ee809d3f) SHA1(347080858fbfe9955002f382603a1b86a52d26d5))

	ROM_REGION( 0x20000, REGION_CPU5, 0) // Communication program
	ROM_LOAD16_WORD_SWAP("ep18643a.7",   0x000000,  0x20000, CRC(b5e048ec) SHA1(8182e05a2ffebd590a936c1359c81e60caa79c2a))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD16_WORD_SWAP("mp19259.32",   0x000000, 0x400000, CRC(4d55dbfc) SHA1(6e57e6e6e785b0f14bb5e221a44d518dbde7ad65))
	ROM_LOAD16_WORD_SWAP("mp19261.34",   0x400000, 0x400000, CRC(b88878ff) SHA1(4bebcfba68b0cc2fa0bcacfaaf2d2e8af3625c5d))

	ROM_REGION( 0x800000, REGION_SOUND2, 0 ) // MPEG audio data
	ROM_LOAD("mp19262.57s",  0x000000, 0x200000, CRC(922aed7a) SHA1(8d6872bdd46eaf2076c10d18c10af8ccbd3b10e8))
	ROM_LOAD("mp19263.58s",  0x200000, 0x200000, CRC(a256f4cd) SHA1(a17b49050f1ecf1970477b12201cc3b58b31d89c))
	ROM_LOAD("mp19264.59s",  0x400000, 0x200000, CRC(b6c51d0f) SHA1(9e0969a1e49ec1462f69cd0f0f9ce630d66174ce))
	ROM_LOAD("mp19265.60s",  0x600000, 0x200000, CRC(7d98700a) SHA1(bedd37314ecab424b5b27030e1e7dc1b596303f3))
ROM_END

ROM_START( skisuprg )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "epr19489.15",  0x000000, 0x080000, CRC(1df948a7) SHA1(a38faeb97c65b379ad05f7311b55217118c8d2be) )
        ROM_LOAD32_WORD( "epr19490.16",  0x000002, 0x080000, CRC(e6fc24d3) SHA1(1ac9172cf0b4d6a3488483ffa490a4ca5d410927) )
        ROM_LOAD32_WORD( "epr19551.13",  0x100000, 0x080000, CRC(3ee8f0d5) SHA1(23f45858559776a70b3b57f4cb2840f44e6a6531) )
        ROM_LOAD32_WORD( "epr19552.14",  0x100002, 0x080000, CRC(baa2e49a) SHA1(b234f3b65e8fabfb6ec7ca62dd9a1d2935b2e95a) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mpr19494.11",  0x000000, 0x400000, CRC(f19cdb5c) SHA1(bdbb7d9e91a7742ff5a908b6244adbed291e5e7f) )
        ROM_LOAD32_WORD( "mpr19495.12",  0x000002, 0x400000, CRC(d42e5ef2) SHA1(21ca5e7e543595a4691aacdbcdd2af21d464d939) )
        ROM_LOAD32_WORD( "mpr19492.9",   0x800000, 0x400000, CRC(4805318f) SHA1(dbd1359817933313c6d74d3a1450682e8ce5857a) )
        ROM_LOAD32_WORD( "mpr19493.10",  0x800002, 0x400000, CRC(39daa909) SHA1(e29e50c7fc39bd4945f993ceaa100358054efc5a) )

	ROM_REGION( 0x800000, REGION_CPU3, 0 ) // TGPx4 program
        ROM_LOAD32_WORD( "mpr19502.29",  0x000000, 0x400000, CRC(2212d8d6) SHA1(3b8a4da2dc00a1eac41b48cbdc322ea1c31b8b29) )
        ROM_LOAD32_WORD( "mpr19503.30",  0x000002, 0x400000, CRC(3c9cfc73) SHA1(2213485a00cef0bcef11b67f00027c4159c5e2f5) )

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mpr19496.17",  0x000000, 0x400000, CRC(0e9aef4e) SHA1(d4b511b90c0a6e27d6097cb25ff005f68d5fa83c) )
        ROM_LOAD32_WORD( "mpr19497.21",  0x000002, 0x400000, CRC(5397efe9) SHA1(4b20bab36462f9506fa2601c2545051ca49de7f5) )
        ROM_LOAD32_WORD( "mpr19498.18",  0x800000, 0x400000, CRC(32e5ae60) SHA1(b8a1cc117875c3919a78eedb60a06926288d9b95) )
        ROM_LOAD32_WORD( "mpr19499.22",  0x800002, 0x400000, CRC(2b9f5b48) SHA1(40f3f2844244c3f1c8792aa262872243ad20fd69) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mpr19501.27",  0x000000, 0x400000, CRC(66d7b02e) SHA1(cede0dc5c8d9fbfa8de01fe864b3cc101abf67d7) )
        ROM_LOAD32_WORD( "mpr19500.25",  0x000002, 0x400000, CRC(905f5798) SHA1(31f104e3022b5bc7ed7c667eb801a57949a06c93) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP( "epr19491.31",  0x000000, 0x080000, CRC(1c9b15fd) SHA1(045244a4eebc45f149aecf47f090cede1813477b) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr19504.32",  0x000000, 0x400000, CRC(9419ec08) SHA1(d05d9ceb7fd09fa8991c0df4d1c57eb621460e30) )
        ROM_LOAD( "mpr19505.34",  0x400000, 0x400000, CRC(eba7f41d) SHA1(f6e521bedf298808a768f6fdcb0b60b320a66d04) )
ROM_END

ROM_START( hotd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19696.15",   0x000000, 0x080000, CRC(03da5623) SHA1(be0bd34a9216375c7204445f084f6c74c4d3b0c8))
	ROM_LOAD32_WORD("ep19697.16",   0x000002, 0x080000, CRC(a9722d87) SHA1(0b14f9a81272f79a5b294bc024711042c5fb2637))
	ROM_LOAD32_WORD("ep19694.13",   0x100000, 0x080000, CRC(e85ca1a3) SHA1(3d688be98f78fe40c2af1e91df6decd500400ae9))
	ROM_LOAD32_WORD("ep19695.14",   0x100002, 0x080000, CRC(cd52b461) SHA1(bc96ab2a4ba7f30c0b89814acc8931c8bf800a82))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19704.11",  0x0000000, 0x400000, CRC(aa80dbb0) SHA1(24e63f4392847f288971469cd10448536eb435d4))
	ROM_LOAD32_WORD("mp19705.12",  0x0000002, 0x400000, CRC(f906843b) SHA1(bee4f43b3ad15d93a2f9f07b873c9cf5d228e2f9))
	ROM_LOAD32_WORD("mp19702.9",   0x0800000, 0x400000, CRC(fc8aa3b7) SHA1(b64afb17d9c97277d8c4f20811f14f65a61cbb56))
	ROM_LOAD32_WORD("mp19703.10",  0x0800002, 0x400000, CRC(208d993d) SHA1(e5c45ea5621f99661a87ffe88e24764d2bbcb51e))
	ROM_LOAD32_WORD("mp19700.7",   0x1000000, 0x400000, CRC(0558cfd3) SHA1(94440839d3325176c2d03f39a78949d0ef040bba))
	ROM_LOAD32_WORD("mp19701.8",   0x1000002, 0x400000, CRC(224a8929) SHA1(933770546d46abca400e7f524eff2ae89241e56d))
	ROM_LOAD32_WORD("ep19698.5",   0x1800000, 0x080000, CRC(e7a7b6ea) SHA1(77cb53f8730fdb55080b70910ab8c750d79acb02))
	ROM_RELOAD     (               0x1900000, 0x080000)
	ROM_RELOAD     (               0x1a00000, 0x080000)
	ROM_RELOAD     (               0x1b00000, 0x080000)
	ROM_RELOAD     (               0x1c00000, 0x080000)
	ROM_RELOAD     (               0x1d00000, 0x080000)
	ROM_RELOAD     (               0x1e00000, 0x080000)
	ROM_RELOAD     (               0x1f00000, 0x080000)
	ROM_LOAD32_WORD("ep19699.6",   0x1800002, 0x080000, CRC(8160b3d9) SHA1(9dab483c60624dddba8085e94a4325739592ec17))
	ROM_RELOAD     (               0x1900002, 0x080000)
	ROM_RELOAD     (               0x1a00002, 0x080000)
	ROM_RELOAD     (               0x1b00002, 0x080000)
	ROM_RELOAD     (               0x1c00002, 0x080000)
	ROM_RELOAD     (               0x1d00002, 0x080000)
	ROM_RELOAD     (               0x1e00002, 0x080000)
	ROM_RELOAD     (               0x1f00002, 0x080000)

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("ep19707.29",   0x000000, 0x080000, CRC(384fd133) SHA1(6d060378d0f801b04d12e7ee874f2fa0572992d9))
	ROM_LOAD32_WORD("ep19706.30",   0x000002, 0x080000, CRC(1277531c) SHA1(08d3e733ba9989fcd32290634171c73f26ab6e2b))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19715.17",  0x0000000, 0x400000, CRC(3ff7dda7) SHA1(0a61b091bb0bc659f0cbca8ad401d0925a1dc2ea))
	ROM_LOAD32_WORD("mp19711.21",  0x0000002, 0x400000, CRC(080d13f1) SHA1(4167428a2a903aea2c14631ccf924afb81338b89))
	ROM_LOAD32_WORD("mp19714.18",  0x0800000, 0x400000, CRC(3e55ab49) SHA1(70b4c1627db80e6734112c02265495e2b4a53278))
	ROM_LOAD32_WORD("mp19710.22",  0x0800002, 0x400000, CRC(80df1036) SHA1(3cc59bb4910aa5382e95762f63325c06b763bd23))
	ROM_LOAD32_WORD("mp19713.19",  0x1000000, 0x400000, CRC(4d092cd3) SHA1(b6d0be283c25235249186751c7f025a7c38d2f36))
	ROM_LOAD32_WORD("mp19709.23",  0x1000002, 0x400000, CRC(d08937bf) SHA1(c92571e35960f27dc8b0b059f12167026d0666d1))
	ROM_LOAD32_WORD("mp19712.20",  0x1800000, 0x400000, CRC(41577943) SHA1(25a0d921c8662043c5860dc7a226d4895ff9fff6))
	ROM_LOAD32_WORD("mp19708.24",  0x1800002, 0x400000, CRC(5cb790f2) SHA1(d3cae450186bc62fd746b14d6a05cb397efcfe40))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19718.27",  0x0000000, 0x400000, CRC(a9de5924) SHA1(3ebac2aeb1467939337c9a5c87ad9c293560dae2))
	ROM_LOAD32_WORD("mp19716.25",  0x0000002, 0x400000, CRC(45c7dcce) SHA1(f602cabd879c69afee544848feafb9fb9f5d51e2))
	ROM_LOAD32_WORD("mp19719.28",  0x0800000, 0x400000, CRC(838f8343) SHA1(fe6622b5917f9a99c097fd60d9446ac6b481fa75))
	ROM_LOAD32_WORD("mp19717.26",  0x0800002, 0x400000, CRC(393e440b) SHA1(927ac9cad22f87b339cc86043678470ff139ce1f))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19720.31",   0x080000,  0x80000, CRC(b367d21d) SHA1(1edaed489a3518ddad85728e416319f940ea02bb))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19721.32",   0x000000, 0x400000, CRC(f5d8fa9a) SHA1(6836973a687c59dd80f8e6c30d33155e306be199))
	ROM_LOAD("mp19722.34",   0x400000, 0x400000, CRC(a56fa539) SHA1(405a892bc368ba862ba71bda7525b421d6973c0e))
ROM_END

ROM_START( lastbrnj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19046a.15",  0x000000, 0x080000, CRC(75be7b7a) SHA1(e57320ac3abac54b7b5278596979746ed1856188))
	ROM_LOAD32_WORD("ep19047a.16",  0x000002, 0x080000, CRC(1f5541e2) SHA1(87214f285a7bf67fbd824f2190cb9b2daf408193))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19050.11",   0x000000, 0x400000, CRC(e6af2b61) SHA1(abdf7aa4c594f0916d4335c70fdd67dc6b1f4630))
	ROM_LOAD32_WORD("mp19051.12",   0x000002, 0x400000, CRC(14b88961) SHA1(bec22f657c6d939c095b99ca9c6eb44b9683fd72))
	ROM_LOAD32_WORD("mp19048.9",    0x800000, 0x400000, CRC(02180215) SHA1(cc5f8e61fee07aa4fc5bfe2d011088ee523c77c2))
	ROM_LOAD32_WORD("mp19049.10",   0x800002, 0x400000, CRC(db7eecd6) SHA1(5955885ad2bfd69d7a2c4e1d1df907aca41fbdd0))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19052.17",   0x000000, 0x400000, CRC(d7f27216) SHA1(b393af96522306dc2e055aea1e837979f41940d4))
	ROM_LOAD32_WORD("mp19053.21",   0x000002, 0x400000, CRC(1f328465) SHA1(950a92209b7c24f66db62c31627a1f1d52721f1e))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19055.27",   0x000000, 0x200000, CRC(85a57d49) SHA1(99c49fe135dc46fa861337b5bac654ae8478778a))
	ROM_LOAD32_WORD("mp19054.25",   0x000002, 0x200000, CRC(05366277) SHA1(f618e2b9b26a1f7eccebfc8f8e17ef8ad9029be8))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("mp19056.31",   0x080000,  0x80000, CRC(22a22918) SHA1(baa039cd86650b6cd81f295916c4d256e60cb29c))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19057.32",  0x0000000, 0x400000, CRC(64809438) SHA1(aa008f83e1eff0daafe01944248ebae6054cee9f))
	ROM_LOAD("mp19058.34",  0x0400000, 0x400000, CRC(e237c11c) SHA1(7c89cba757bd58747ed0d633b2fe7ef559fcd15e))
ROM_END

ROM_START( lastbrnx )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19061a.15",  0x000000, 0x080000, CRC(c0aebab2) SHA1(fa63081b0aa6f02c3d197485865ee38e9c78b43d))
	ROM_LOAD32_WORD("ep19062a.16",  0x000002, 0x080000, CRC(cdf597e8) SHA1(a85ca36a537ba21d11ef3cfdf914c2c93ac5e68f))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19050.11",   0x000000, 0x400000, CRC(e6af2b61) SHA1(abdf7aa4c594f0916d4335c70fdd67dc6b1f4630))
	ROM_LOAD32_WORD("mp19051.12",   0x000002, 0x400000, CRC(14b88961) SHA1(bec22f657c6d939c095b99ca9c6eb44b9683fd72))
	ROM_LOAD32_WORD("mp19048.9",    0x800000, 0x400000, CRC(02180215) SHA1(cc5f8e61fee07aa4fc5bfe2d011088ee523c77c2))
	ROM_LOAD32_WORD("mp19049.10",   0x800002, 0x400000, CRC(db7eecd6) SHA1(5955885ad2bfd69d7a2c4e1d1df907aca41fbdd0))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19052.17",   0x000000, 0x400000, CRC(d7f27216) SHA1(b393af96522306dc2e055aea1e837979f41940d4))
	ROM_LOAD32_WORD("mp19053.21",   0x000002, 0x400000, CRC(1f328465) SHA1(950a92209b7c24f66db62c31627a1f1d52721f1e))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19055.27",   0x000000, 0x200000, CRC(85a57d49) SHA1(99c49fe135dc46fa861337b5bac654ae8478778a))
	ROM_LOAD32_WORD("mp19054.25",   0x000002, 0x200000, CRC(05366277) SHA1(f618e2b9b26a1f7eccebfc8f8e17ef8ad9029be8))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("mp19056.31",   0x080000,  0x80000, CRC(22a22918) SHA1(baa039cd86650b6cd81f295916c4d256e60cb29c))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19057.32",  0x0000000, 0x400000, CRC(64809438) SHA1(aa008f83e1eff0daafe01944248ebae6054cee9f))
	ROM_LOAD("mp19058.34",  0x0400000, 0x400000, CRC(e237c11c) SHA1(7c89cba757bd58747ed0d633b2fe7ef559fcd15e))
ROM_END

ROM_START( pltkidsa )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep21281.pr0",  0x000000, 0x080000, CRC(293ead5d) SHA1(5a6295e543d7e68387de0ca4d88e930a0d8ed25c))
	ROM_LOAD32_WORD("ep21282.pr1",  0x000002, 0x080000, CRC(ed0e7b9e) SHA1(15f3fab6ac2dd40f32bda55503378ab14f998707))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp21262.da0",  0x000000, 0x400000, CRC(aa71353e) SHA1(6eb5e8284734f01beec1dbbee049b6b7672e2504))
	ROM_LOAD32_WORD("mp21263.da1",  0x000002, 0x400000, CRC(d55d4509) SHA1(641db6ec3e9266f8265a4b541bcd8c2f7d164cc3))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp21264.tp0",  0x000000, 0x400000, CRC(6b35204d) SHA1(3a07701b140eb3088fad29c8b2d9c1e1e7ef9471))
	ROM_LOAD32_WORD("mp21268.tp1",  0x000002, 0x400000, CRC(16ce2147) SHA1(39cba6b4f1130a3da7e2d226c948425eec34090e))
	ROM_LOAD32_WORD("mp21265.tp2",  0x800000, 0x400000, CRC(f061e639) SHA1(a89b7a84192fcc1e9e0fe9adf7446f7b275d5a03))
	ROM_LOAD32_WORD("mp21269.tp3",  0x800002, 0x400000, CRC(8c06255e) SHA1(9a8c302528e590be1b56ed301da30abf21f0be2e))
	ROM_LOAD32_WORD("mp21266.tp4", 0x1000000, 0x400000, CRC(f9c32021) SHA1(b21f8bf281bf2cfcdc7e5eb798cd633e905ab8b8))
	ROM_LOAD32_WORD("mp21270.tp5", 0x1000002, 0x400000, CRC(b61f81c3) SHA1(7733f44e791974070df139958eb97e0585ee50f8))
	ROM_LOAD32_WORD("mp21267.tp6", 0x1800000, 0x400000, CRC(c42cc938) SHA1(6153f52add63295122e1215dd07d648d030a7306))
	ROM_LOAD32_WORD("mp21271.tp7", 0x1800002, 0x400000, CRC(a5325c75) SHA1(d52836760475c7d9fbb4e5b8147ac416ffd1fcd9))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp21274.tx1", 0x0000000, 0x400000, CRC(f045e3d1) SHA1(548909d2da22ed98594e0ab6ecffebec4fca2f93))
	ROM_LOAD32_WORD("mp21272.tx0", 0x0000002, 0x400000, CRC(dd605c21) SHA1(8363a082a666ceeb84df84929ff3fbaff49af821))
	ROM_LOAD32_WORD("mp21275.tx3", 0x0800000, 0x400000, CRC(c4870b7c) SHA1(feb8a34acb620a36ed5aea92d22622a76d7e1b29))
	ROM_LOAD32_WORD("mp21273.tx2", 0x0800002, 0x400000, CRC(722ec8a2) SHA1(1a1dc92488cde6284a96acce80e47a9cceccde76))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep21276.sd0", 0x080000, 0x080000, CRC(8f415bc3) SHA1(4e8e1ccbe025deca42fcf2582f3da46fa34780b7))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp21277.sd1", 0x0000000, 0x200000, CRC(bfba0ff6) SHA1(11081b3eabc33a42ecfc0b2b535ce16510496144))
	ROM_LOAD("mp21278.sd2", 0x0200000, 0x200000, CRC(27e18e08) SHA1(254c0ad4d6bd572ff0efc3ea80489e73716a31a7))
	ROM_LOAD("mp21279.sd3", 0x0400000, 0x200000, CRC(3a8dcf68) SHA1(312496b45b699051c8b4dd0e5d94e73fe5f3ad8d))
	ROM_LOAD("mp21280.sd4", 0x0600000, 0x200000, CRC(aa548124) SHA1(a94adfe16b5c3236746451c181ccd3e1c27432f4))

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( pltkids )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep21285a.15",  0x000000, 0x080000, CRC(bdde5b41) SHA1(14c3f5031f85c6756c00bc67765a967ebaf7eb7f) )
	ROM_LOAD32_WORD("ep21286a.16",  0x000002, 0x080000, CRC(c8092e0e) SHA1(01030621efa9c97eb43f4a5e3e029ec99a2363c5) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp21262.da0",  0x000000, 0x400000, CRC(aa71353e) SHA1(6eb5e8284734f01beec1dbbee049b6b7672e2504))
	ROM_LOAD32_WORD("mp21263.da1",  0x000002, 0x400000, CRC(d55d4509) SHA1(641db6ec3e9266f8265a4b541bcd8c2f7d164cc3))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp21264.tp0",  0x000000, 0x400000, CRC(6b35204d) SHA1(3a07701b140eb3088fad29c8b2d9c1e1e7ef9471))
	ROM_LOAD32_WORD("mp21268.tp1",  0x000002, 0x400000, CRC(16ce2147) SHA1(39cba6b4f1130a3da7e2d226c948425eec34090e))
	ROM_LOAD32_WORD("mp21265.tp2",  0x800000, 0x400000, CRC(f061e639) SHA1(a89b7a84192fcc1e9e0fe9adf7446f7b275d5a03))
	ROM_LOAD32_WORD("mp21269.tp3",  0x800002, 0x400000, CRC(8c06255e) SHA1(9a8c302528e590be1b56ed301da30abf21f0be2e))
	ROM_LOAD32_WORD("mp21266.tp4", 0x1000000, 0x400000, CRC(f9c32021) SHA1(b21f8bf281bf2cfcdc7e5eb798cd633e905ab8b8))
	ROM_LOAD32_WORD("mp21270.tp5", 0x1000002, 0x400000, CRC(b61f81c3) SHA1(7733f44e791974070df139958eb97e0585ee50f8))
	ROM_LOAD32_WORD("mp21267.tp6", 0x1800000, 0x400000, CRC(c42cc938) SHA1(6153f52add63295122e1215dd07d648d030a7306))
	ROM_LOAD32_WORD("mp21271.tp7", 0x1800002, 0x400000, CRC(a5325c75) SHA1(d52836760475c7d9fbb4e5b8147ac416ffd1fcd9))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp21274.tx1", 0x0000000, 0x400000, CRC(f045e3d1) SHA1(548909d2da22ed98594e0ab6ecffebec4fca2f93))
	ROM_LOAD32_WORD("mp21272.tx0", 0x0000002, 0x400000, CRC(dd605c21) SHA1(8363a082a666ceeb84df84929ff3fbaff49af821))
	ROM_LOAD32_WORD("mp21275.tx3", 0x0800000, 0x400000, CRC(c4870b7c) SHA1(feb8a34acb620a36ed5aea92d22622a76d7e1b29))
	ROM_LOAD32_WORD("mp21273.tx2", 0x0800002, 0x400000, CRC(722ec8a2) SHA1(1a1dc92488cde6284a96acce80e47a9cceccde76))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep21276.sd0", 0x080000, 0x080000, CRC(8f415bc3) SHA1(4e8e1ccbe025deca42fcf2582f3da46fa34780b7))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp21277.sd1", 0x0000000, 0x200000, CRC(bfba0ff6) SHA1(11081b3eabc33a42ecfc0b2b535ce16510496144))
	ROM_LOAD("mp21278.sd2", 0x0200000, 0x200000, CRC(27e18e08) SHA1(254c0ad4d6bd572ff0efc3ea80489e73716a31a7))
	ROM_LOAD("mp21279.sd3", 0x0400000, 0x200000, CRC(3a8dcf68) SHA1(312496b45b699051c8b4dd0e5d94e73fe5f3ad8d))
	ROM_LOAD("mp21280.sd4", 0x0600000, 0x200000, CRC(aa548124) SHA1(a94adfe16b5c3236746451c181ccd3e1c27432f4))
ROM_END

ROM_START( indy500 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18254a.15",  0x000000, 0x080000, CRC(ad0f1fc5) SHA1(0bff35fc1d892aaffbf1a3965bf3109c54839f4b))
	ROM_LOAD32_WORD("ep18255a.16",  0x000002, 0x080000, CRC(784daab8) SHA1(299e87f8ec7bdefa6f94f4ab65e29e91f290611e))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18237.11",   0x000000, 0x400000, CRC(37e4255a) SHA1(3ee69a5b9364048dfab242773d97f3af430845b7))
	ROM_LOAD32_WORD("mp18238.12",   0x000002, 0x400000, CRC(bf837bac) SHA1(6624417b65f15f20427bc42c27283f10342c76b5))
	ROM_LOAD32_WORD("mp18239.9",    0x800000, 0x400000, CRC(9a2db86e) SHA1(0b81f6037657af7d96ed5e9bfef407d87cbcc294))
	ROM_LOAD32_WORD("mp18240.10",   0x800002, 0x400000, CRC(ab46a35f) SHA1(67da857db7155a858a1fa575b6c50f4be3c9ab7c))
	ROM_LOAD32_WORD("ep18389.7",   0x1000000, 0x080000, CRC(d22ea019) SHA1(ef10bb0ffcb1bbcf4672bb5f705a27679a793764))
	ROM_RELOAD     (               0x1100000, 0x080000)
	ROM_RELOAD     (               0x1200000, 0x080000)
	ROM_RELOAD     (               0x1300000, 0x080000)
	ROM_RELOAD     (               0x1400000, 0x080000)
	ROM_RELOAD     (               0x1500000, 0x080000)
	ROM_RELOAD     (               0x1600000, 0x080000)
	ROM_RELOAD     (               0x1700000, 0x080000)
	ROM_RELOAD     (               0x1800000, 0x080000)
	ROM_RELOAD     (               0x1900000, 0x080000)
	ROM_RELOAD     (               0x1a00000, 0x080000)
	ROM_RELOAD     (               0x1b00000, 0x080000)
	ROM_RELOAD     (               0x1c00000, 0x080000)
	ROM_RELOAD     (               0x1d00000, 0x080000)
	ROM_RELOAD     (               0x1e00000, 0x080000)
	ROM_RELOAD     (               0x1f00000, 0x080000)
	ROM_LOAD32_WORD("ep18390.8",   0x1000002, 0x080000, CRC(38e796e5) SHA1(b23cfe45c363d616a65decd57aeb8ae61d5370e9))
	ROM_RELOAD     (               0x1100002, 0x080000)
	ROM_RELOAD     (               0x1200002, 0x080000)
	ROM_RELOAD     (               0x1300002, 0x080000)
	ROM_RELOAD     (               0x1400002, 0x080000)
	ROM_RELOAD     (               0x1500002, 0x080000)
	ROM_RELOAD     (               0x1600002, 0x080000)
	ROM_RELOAD     (               0x1700002, 0x080000)
	ROM_RELOAD     (               0x1800002, 0x080000)
	ROM_RELOAD     (               0x1900002, 0x080000)
	ROM_RELOAD     (               0x1a00002, 0x080000)
	ROM_RELOAD     (               0x1b00002, 0x080000)
	ROM_RELOAD     (               0x1c00002, 0x080000)
	ROM_RELOAD     (               0x1d00002, 0x080000)
	ROM_RELOAD     (               0x1e00002, 0x080000)
	ROM_RELOAD     (               0x1f00002, 0x080000)

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("ep18249.29",   0x000000, 0x080000, CRC(a399f023) SHA1(8b453313c16d935701ed7dbf71c1607c40aede63))
	ROM_LOAD32_WORD("ep18250.30",   0x000002, 0x080000, CRC(7479ad52) SHA1(d453e25709cd5970cd21bdc8b4785bc8eb5a50d7))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18233.17",   0x000000, 0x400000, CRC(48a024d3) SHA1(501c6ab969713187025331942f922cb0e8efa69a))
	ROM_LOAD32_WORD("mp18234.21",   0x000002, 0x400000, CRC(1178bfc8) SHA1(4a9982fdce08f9d375371763dd5287e8485c24b1))
	ROM_LOAD32_WORD("mp18235.18",   0x800000, 0x400000, CRC(e7d70d59) SHA1(6081739c15a634d5cc7680a4fc7decead93540ed))
	ROM_LOAD32_WORD("mp18236.22",   0x800002, 0x400000, CRC(6ca29e0e) SHA1(5de8b569d2a91047836f4a251c21db82fd7841c9))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18232.27",   0x000000, 0x400000, CRC(f962347d) SHA1(79f07ee6b821724294ca9e7a079cb33249102508))
	ROM_LOAD32_WORD("mp18231.25",   0x000002, 0x400000, CRC(673d5338) SHA1(ce592857496ccc0a51efb377cf7cccc000b4296b))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18391.31",   0x080000,  0x40000, CRC(79579b72) SHA1(36fed8a9eeb34968b2852ea8fc9198427f0d27c6))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18241.32",  0x0000000, 0x200000, CRC(3a380ae1) SHA1(114113325e9e5262af8750c05089f24818943cde))
	ROM_LOAD("mp18242.33",  0x0200000, 0x200000, CRC(1cc3deae) SHA1(5c9cb8ce43a909b25b4e734c6a4ffd786f4dde31))
	ROM_LOAD("mp18243.34",  0x0400000, 0x200000, CRC(a00a0053) SHA1(9c24fbcd0318c7e195dd153d6ba05e8c1e052968))
	ROM_LOAD("mp18244.35",  0x0600000, 0x200000, CRC(bfa75beb) SHA1(fec89260d887e90ee9c2803e2eaf937cf9bfa10b))
ROM_END

ROM_START( bel )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep20225.15",   0x000000, 0x020000, CRC(4abc6b59) SHA1(cc6da75aafcbbc86720435182a66e8de065c8e99))
	ROM_LOAD32_WORD("ep20226.16",   0x000002, 0x020000, CRC(43e05b3a) SHA1(204b3cc6bbfdc92b4871c45fe4abff4ab4a66317))
	ROM_LOAD32_WORD("ep20223.13",   0x040000, 0x020000, CRC(61b1be98) SHA1(03c308c58a72bf3b78f41d5a9c0adaa7aad631c2))
	ROM_LOAD32_WORD("ep20224.14",   0x040002, 0x020000, CRC(eb2d7dbf) SHA1(f3b126e2fcef1cf673b239696ed8018241b1170e))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp20233.11",   0x000000, 0x200000, CRC(3e079a3b) SHA1(a3f15cd68a514cf80f0a40dbbb08e8b0489a0e4b))
	ROM_LOAD32_WORD("mp20234.12",   0x000002, 0x200000, CRC(58bde826) SHA1(386d0d07738f579cb23e4168aceb26f56bcca1c1))
	ROM_LOAD32_WORD("mp20231.9",    0x400000, 0x200000, CRC(b3393e93) SHA1(aa52ae307aa37faaaf86c326642af1946c5f4056))
	ROM_LOAD32_WORD("mp20232.10",   0x400002, 0x200000, CRC(da4a2e11) SHA1(f9138813f6d1ca2126f5de10d8d69dcbb533aa0e))
	ROM_LOAD32_WORD("mp20229.7",    0x800000, 0x200000, CRC(cdec7bf4) SHA1(510b6d41f1d32a9929379ba76037db137164cd43))
	ROM_LOAD32_WORD("mp20230.8",    0x800002, 0x200000, CRC(a166fa87) SHA1(d4f6d4fba7f43b21f0bf9d948ec93b372425bf7c))
	ROM_LOAD32_WORD("mp20227.5",    0xc00000, 0x200000, CRC(1277686e) SHA1(fff27006659458300001425261b944e690f1d494))
	ROM_LOAD32_WORD("mp20228.6",    0xc00002, 0x200000, CRC(49cb5568) SHA1(ee3273302830f3499c7d4e548b629c51e0369e8a))

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp20236.29",   0x000000, 0x200000, CRC(8de9a3c2) SHA1(e7fde1fd509531e1002ff813163067dc0d134536))
	ROM_LOAD32_WORD("mp20235.30",   0x000002, 0x200000, CRC(78fa11ef) SHA1(a60deabb662e9c09f5d6342dc1a1c6045744d93f))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp20244.17",  0x0000000, 0x200000, CRC(9d2a8660) SHA1(59302e7119c9ff779ce0c871713fe3688c29cccb))
	ROM_LOAD32_WORD("mp20240.21",  0x0000002, 0x200000, CRC(51615908) SHA1(c70252b0b6f17aa0cd9b5264d4166df8ab7d1784))
	ROM_LOAD32_WORD("mp20243.18",  0x0400000, 0x200000, CRC(48671f7c) SHA1(b0bdc7f42450c8d9cebbcf43cf858f7399e378e4))
	ROM_LOAD32_WORD("mp20239.22",  0x0400002, 0x200000, CRC(6cd8d8a5) SHA1(1c634fbbcbafb1c3825117682901a3264599b246))
	ROM_LOAD32_WORD("mp20242.19",  0x0800000, 0x200000, CRC(e7f86ac7) SHA1(7b7724127b27834eaaa228050ceb779d8a027882))
	ROM_LOAD32_WORD("mp20238.23",  0x0800002, 0x200000, CRC(0a480c7c) SHA1(239d2c9c49cb8ddc0d6aa956a497b494217f38d7))
	ROM_LOAD32_WORD("mp20241.20",  0x0c00000, 0x200000, CRC(51974b98) SHA1(7d6ab9c0ccec77676222611bf200d2e067e20520))
	ROM_LOAD32_WORD("mp20237.24",  0x0c00002, 0x200000, CRC(89b5d8b6) SHA1(6e0a0323d6a804f1f1e4404694cc1ea7dfbf2d95))

	ROM_REGION( 0xc00000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp20247.27",   0x000000, 0x200000, CRC(00b0417d) SHA1(5e9d38509c1e5273079a342a64ca2c956cd47e6d))
	ROM_LOAD32_WORD("mp20245.25",   0x000002, 0x200000, CRC(36490a08) SHA1(a462e094c9a9ec4743e4bf2c4ce23357257a2a54))
	ROM_LOAD32_WORD("mp20248.28",   0x800000, 0x200000, CRC(0ace6bef) SHA1(a231aeb7b984f5b927144f0eec4ef2282429494f))
	ROM_LOAD32_WORD("mp20246.26",   0x800002, 0x200000, CRC(250d6ca1) SHA1(cd1d4bc0fcf89e47884b87863a09bb263bce72cc))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("mp20249.31",  0x080000, 0x020000, CRC(dc24f13d) SHA1(66ab8e843319d07663ef13f3d2299c6c7414071f))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp20250.32",  0x0000000, 0x200000, CRC(91b735d3) SHA1(b0e7e493fb20ebf30c17378199e49d529ffb3f20))
	ROM_LOAD("mp20251.33",  0x0200000, 0x200000, CRC(703a947b) SHA1(95b8d3dc29e87e6537b288d8e946728e0b345dd0))
	ROM_LOAD("mp20252.34",  0x0400000, 0x200000, CRC(8f48f375) SHA1(9e511e89e99c77f06a5fba033ca8f9b98bd86f91))
	ROM_LOAD("mp20253.35",  0x0600000, 0x200000, CRC(ca6aa17c) SHA1(f6df2483ca75573449ba36638dbbed4be7843a44))
ROM_END

ROM_START( overrev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "20124a.15", 0x000000, 0x080000, CRC(74beb8d7) SHA1(c65c641138ecd7312c4930702d1498b8a346175a) )
        ROM_LOAD32_WORD( "20125a.16", 0x000002, 0x080000, CRC(def64456) SHA1(cedb64d2d99a73301ef45c2f5f860a9b87faf6a7) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "19996.11",        0x000000, 0x400000, CRC(21928a00) SHA1(6b439fd2b113b64df9378ef8180a17aa6fa975c5) )
        ROM_LOAD32_WORD( "19997.12",        0x000002, 0x400000, CRC(2a169cab) SHA1(dbf9af938afd0599d345c42c1df242e575c14de9) )
        ROM_LOAD32_WORD( "19994.9",         0x800000, 0x400000, CRC(e691fbd5) SHA1(b99c2f3f2a682966d792917dfcb8ed8e53bc0b7a) )
        ROM_LOAD32_WORD( "19995.10",        0x800002, 0x400000, CRC(82a7828e) SHA1(4336a12a07a67f94091b4a9b491bab02c375dd15) )

	ROM_REGION( 0x800000, REGION_CPU3, ROMREGION_ERASE00 ) // TGPx4 program (COPRO sockets)

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models (TGP sockets)
        ROM_LOAD32_WORD( "19998.17",        0x000000, 0x200000, CRC(6a834574) SHA1(8be19bf42dbb157d6acde62a2018ef4c0d41aab4) )
        ROM_LOAD32_WORD( "19999.21",        0x000002, 0x200000, CRC(ff590a2d) SHA1(ad29e4270b4a2f82189fbab83358eb1200f43777) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures (TEXTURE sockets)
        ROM_LOAD32_WORD( "20001.27",        0x000000, 0x200000, CRC(6ca236aa) SHA1(b3cb89fadb42afed13be4f229d7158dee487978a) )
        ROM_LOAD32_WORD( "20000.25",        0x000002, 0x200000, CRC(894d8ded) SHA1(9bf7c754a29eef47fa49b5567980601895127306) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "20002.31",   0x000000, 0x080000, CRC(7efb069e) SHA1(30b1bbaf348d6a6b9ee2fdf82a0749baa025e0bf) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "20003.32", 0x000000, 0x200000, BAD_DUMP CRC(538215a6) SHA1(c68bba03026b774a70bab045b41fa226bcfbc1d6) ) /* Dumped at half size, should be 32MBit */
        ROM_LOAD( "20004.34", 0x400000, 0x200000, BAD_DUMP CRC(197c0b8d) SHA1(fdd948fa853d75b1d06539afd7945532041d2826) ) /* Dumped at half size, should be 32MBit */
ROM_END

ROM_START( topskatr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19755a.15",  0x000000, 0x080000, CRC(b80633b9) SHA1(5396da414beeb918e6f38f25a43dd76345a0c8ed))
	ROM_LOAD32_WORD("ep19756a.16",  0x000002, 0x080000, CRC(472046a2) SHA1(06d0f609257ba476e6bd3b956e0850e7167429ce))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19735.11",   0x000000, 0x400000, CRC(8e509266) SHA1(49afc91467f08befaf34e743cbe823de3e3c9d85))
	ROM_LOAD32_WORD("mp19736.12",   0x000002, 0x400000, CRC(094e0a0d) SHA1(de2c739f71e51166263446b9f6a566866ab8bee8))
	ROM_LOAD32_WORD("mp19737.9",    0x800000, 0x400000, CRC(281a7dde) SHA1(71d5ba434328a81969bfdc71ac1160c5ff3ae9d3))
	ROM_LOAD32_WORD("mp19738.10",   0x800002, 0x400000, CRC(f688327e) SHA1(68c9db242ef7e8f98979e968a09e4b093bc5d470))

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp19743.29",   0x000000, 0x200000, CRC(d41a41bf) SHA1(a5f6b24e6526d0d2ef9c526c273c018d1e0fed59))
	ROM_LOAD32_WORD("mp19744.30",   0x000002, 0x200000, CRC(84f203bf) SHA1(4952b764e6bf6cd735018738c5eff08781ee2315))

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19741.17",   0x000000, 0x200000, CRC(111a6e29) SHA1(8664059f157626e4bbdcf8357e3d30b37d3c25b8))
	ROM_LOAD32_WORD("mp19742.21",   0x000002, 0x200000, CRC(28510aff) SHA1(3e68aec090f36a60b3b70bc90f09e2f9ce088718))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19740.27",   0x000000, 0x400000, CRC(b20f508b) SHA1(c90fa3b42d87291ea459ccc137f3a2f3eb7efec0))
	ROM_LOAD32_WORD("mp19739.25",   0x000002, 0x400000, CRC(8120cfd8) SHA1(a82744bff5dcdfae296c7c3e8c3fbfda26324e85))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("mp19759.31",   0x080000,  0x80000, CRC(573530f2) SHA1(7b205085965d6694f8e75e29c4028f7cb6f631ab))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // DSB program
	ROM_LOAD16_WORD_SWAP("mp19760.2s",   0x000000,  0x20000, CRC(2e41ca15) SHA1(a302209bfe0f1491dff2da64b32cfaa13c3d3304))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD16_WORD_SWAP("mp19745.32",   0x000000, 0x400000, CRC(7082a0af) SHA1(415f9d0793a697cb1719bbd96370f4a741866527))
	ROM_LOAD16_WORD_SWAP("mp19746.34",   0x400000, 0x400000, CRC(657b5977) SHA1(ca76f211d68b6b55678a4d7949bfd2ddef1b1710))

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 ) // MPEG audio data
	ROM_LOAD("mp19747.18s",  0x000000, 0x400000, CRC(6e895aaa) SHA1(4c67c1e1d58a3034bbd711252a78689db9f235bb))
	ROM_LOAD("mp19748.20s",  0x400000, 0x400000, CRC(fcd74de3) SHA1(fd4da4cf40c4342c6263cf22eee5968292a4d2c0))
	ROM_LOAD("mp19749.22s",  0x800000, 0x400000, CRC(842ca1eb) SHA1(6ee6b2eb2ea400bdb9c0a9b4a126b4b86886e813))
	ROM_LOAD("mp19750.24s",  0xc00000, 0x400000, CRC(cd95d0bf) SHA1(40e2a2980c89049c339fefd48bf7aac79962cd2e))
ROM_END

ROM_START( doaa )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("e19310aa.12",  0x000000, 0x080000, CRC(06486f7a) SHA1(b3e14103570e5f45aed16e1c158e469bc85002ae))
	ROM_LOAD32_WORD("e19311aa.13",  0x000002, 0x080000, CRC(1be62912) SHA1(dcc2df8e28e1a107867f74248e6ffcac83afe7c0))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19318.11",   0x000000, 0x400000, CRC(ab431bfe) SHA1(45b5ccf67c91014daf6bf3c4bd8ec372b246e404))
	ROM_LOAD32_WORD("mp19319.12",   0x000002, 0x400000, CRC(c5cb694d) SHA1(448b45d30cc7a71395a49a2c5789989fd7b7b4e7))
	ROM_LOAD32_WORD("mp19316.9",    0x800000, 0x400000, CRC(2d2d1b1a) SHA1(77ce5d8aa98bdbc97ae08a452f584b30d8885cfc))
	ROM_LOAD32_WORD("mp19317.10",   0x800002, 0x400000, CRC(96b17bcf) SHA1(3aa9d2f8afad74b5626ce2cf2d7a86aef8cac80b))
	ROM_LOAD32_WORD("mp19314.7",   0x1000000, 0x400000, CRC(a8d963fb) SHA1(6a1680d6380321279b0d701e4b47d4ae712f3b72))
	ROM_LOAD32_WORD("mp19315.8",   0x1000002, 0x400000, CRC(90ae5682) SHA1(ec56df14f0847daf9bd0435f785a8946c94d2988))
	ROM_LOAD32_WORD("mp19312.5",   0x1800000, 0x200000, CRC(1dcedb10) SHA1(a60fb9e7c0731004d0f0ff28c4cde272b21dd658))
	ROM_RELOAD     (               0x1c00000, 0x200000 )
	ROM_LOAD32_WORD("mp19313.6",   0x1800002, 0x200000, CRC(8c63055e) SHA1(9f375b3f4a8884163ffcf364989499f2cd21e18b))
	ROM_RELOAD     (               0x1c00002, 0x200000 )

	ROM_REGION( 0x1800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19322.17",  0x0000000, 0x400000, CRC(d0e6ecf0) SHA1(1b87f6337b4286fd738856da899462e7baa92601))
	ROM_LOAD32_WORD("mp19325.21",  0x0000002, 0x400000, CRC(7cbe432d) SHA1(8b31e292160b88df9c77b36096914d09ab8b6086))
	ROM_LOAD32_WORD("mp19323.18",  0x0800000, 0x400000, CRC(453d3f4a) SHA1(8c0530824bb8ecb007021ee6e93412597bb0ecd6))
	ROM_LOAD32_WORD("mp19326.22",  0x0800002, 0x400000, CRC(b976da02) SHA1(a154eb128604aac9e35438d8811971133eab94a1))
	ROM_LOAD32_WORD("mp19324.19",  0x1000000, 0x400000, CRC(d972201f) SHA1(1857ffc58697997ee22436586c398eb0c3daba6c))
	ROM_LOAD32_WORD("mp19327.23",  0x1000002, 0x400000, CRC(6a75634c) SHA1(8ed74c7afd95fc7a4df0f01a47479b6f44e3073c))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19321.27",   0x000000, 0x400000, CRC(9c49e845) SHA1(344839640d9814263fa5ed00c2043cd6f18d5cb2))
	ROM_LOAD32_WORD("mp19320.25",   0x000002, 0x400000, CRC(190c017f) SHA1(4c3250b9abe39fc5c8fd0fcdb5fb7ea131434516))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19328.30",   0x080000,  0x80000, CRC(400bdbfb) SHA1(54db969fa54cf3c502d77aa6a6aaeef5d7db9f04))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19329.31",   0x000000, 0x200000, CRC(8fd2708a) SHA1(7a341b15afa489aa95af70cb34ac3934b1a7d887))
	ROM_LOAD("mp19330.32",   0x200000, 0x200000, CRC(0c69787d) SHA1(dc5870cd93da2babe5fc9c03b252fc6ea6e45721))
	ROM_LOAD("mp19331.33",   0x400000, 0x200000, CRC(c18ea0b8) SHA1(0f42458829ae85fffcedd42cd9f728a7a3d75f1c))
	ROM_LOAD("mp19332.34",   0x600000, 0x200000, CRC(2877f96f) SHA1(00e5677da30527b862e238f10762a5cbfbabde2b))

	MODEL2_CPU_BOARD
	MODEL2A_VID_BOARD
ROM_END

ROM_START( doa )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19379b.15",   0x000000, 0x080000, CRC(8a10a944) SHA1(c675a344f74d0118907fb5292495883c0c30c719))
	ROM_LOAD32_WORD("ep19380b.16",   0x000002, 0x080000, CRC(766c1ec8) SHA1(49250886f66db9fd37d88bc22c8f22046f74f043))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19318.11",   0x000000, 0x400000, CRC(ab431bfe) SHA1(45b5ccf67c91014daf6bf3c4bd8ec372b246e404))
	ROM_LOAD32_WORD("mp19319.12",   0x000002, 0x400000, CRC(c5cb694d) SHA1(448b45d30cc7a71395a49a2c5789989fd7b7b4e7))
	ROM_LOAD32_WORD("mp19316.9",    0x800000, 0x400000, CRC(2d2d1b1a) SHA1(77ce5d8aa98bdbc97ae08a452f584b30d8885cfc))
	ROM_LOAD32_WORD("mp19317.10",   0x800002, 0x400000, CRC(96b17bcf) SHA1(3aa9d2f8afad74b5626ce2cf2d7a86aef8cac80b))
	ROM_LOAD32_WORD("mp19314.7",   0x1000000, 0x400000, CRC(a8d963fb) SHA1(6a1680d6380321279b0d701e4b47d4ae712f3b72))
	ROM_LOAD32_WORD("mp19315.8",   0x1000002, 0x400000, CRC(90ae5682) SHA1(ec56df14f0847daf9bd0435f785a8946c94d2988))
	ROM_LOAD32_WORD("mp19312.5",   0x1800000, 0x200000, CRC(1dcedb10) SHA1(a60fb9e7c0731004d0f0ff28c4cde272b21dd658))
	ROM_RELOAD     (               0x1c00000, 0x200000 )
	ROM_LOAD32_WORD("mp19313.6",   0x1800002, 0x200000, CRC(8c63055e) SHA1(9f375b3f4a8884163ffcf364989499f2cd21e18b))
	ROM_RELOAD     (               0x1c00002, 0x200000 )

	ROM_REGION( 0x1800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19322.17",  0x0000000, 0x400000, CRC(d0e6ecf0) SHA1(1b87f6337b4286fd738856da899462e7baa92601))
	ROM_LOAD32_WORD("mp19325.21",  0x0000002, 0x400000, CRC(7cbe432d) SHA1(8b31e292160b88df9c77b36096914d09ab8b6086))
	ROM_LOAD32_WORD("mp19323.18",  0x0800000, 0x400000, CRC(453d3f4a) SHA1(8c0530824bb8ecb007021ee6e93412597bb0ecd6))
	ROM_LOAD32_WORD("mp19326.22",  0x0800002, 0x400000, CRC(b976da02) SHA1(a154eb128604aac9e35438d8811971133eab94a1))
	ROM_LOAD32_WORD("mp19324.19",  0x1000000, 0x400000, CRC(d972201f) SHA1(1857ffc58697997ee22436586c398eb0c3daba6c))
	ROM_LOAD32_WORD("mp19327.23",  0x1000002, 0x400000, CRC(6a75634c) SHA1(8ed74c7afd95fc7a4df0f01a47479b6f44e3073c))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19321.27",   0x000000, 0x400000, CRC(9c49e845) SHA1(344839640d9814263fa5ed00c2043cd6f18d5cb2))
	ROM_LOAD32_WORD("mp19320.25",   0x000002, 0x400000, CRC(190c017f) SHA1(4c3250b9abe39fc5c8fd0fcdb5fb7ea131434516))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19328.30",   0x080000,  0x80000, CRC(400bdbfb) SHA1(54db969fa54cf3c502d77aa6a6aaeef5d7db9f04))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19329.31",   0x000000, 0x200000, CRC(8fd2708a) SHA1(7a341b15afa489aa95af70cb34ac3934b1a7d887))
	ROM_LOAD("mp19330.32",   0x200000, 0x200000, CRC(0c69787d) SHA1(dc5870cd93da2babe5fc9c03b252fc6ea6e45721))
	ROM_LOAD("mp19331.33",   0x400000, 0x200000, CRC(c18ea0b8) SHA1(0f42458829ae85fffcedd42cd9f728a7a3d75f1c))
	ROM_LOAD("mp19332.34",   0x600000, 0x200000, CRC(2877f96f) SHA1(00e5677da30527b862e238f10762a5cbfbabde2b))
ROM_END

ROM_START( sgt24h )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19155.15",   0x000000, 0x080000, CRC(593952fd) SHA1(1fc4afc6e3910cc8adb0688542e61a9efb442e56))
	ROM_LOAD32_WORD("ep19156.16",   0x000002, 0x080000, CRC(a91fc4ee) SHA1(a37611da0295f7d7e5d2411c3f9b73140d311f74))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19146.11",   0x000000, 0x400000, CRC(d66b5b0f) SHA1(c2a5b83c9041d8f46dfac4a3ff8cfdefb96d02b3))
	ROM_LOAD32_WORD("mp19147.12",   0x000002, 0x400000, CRC(d5558f48) SHA1(c9f40328d6974b7767fa6ba719d0d2b7a173c210))
	ROM_LOAD32_WORD("mp19148.9",    0x800000, 0x400000, CRC(a14c86db) SHA1(66cd8672c00e4e2572de7c5648de595674ffa8f8))
	ROM_LOAD32_WORD("mp19149.10",   0x800002, 0x400000, CRC(94ef5849) SHA1(3e1748dc5e61c93eedbf0ca6b1946a30be722403))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19150.17",   0x000000, 0x400000, CRC(e0ad870e) SHA1(3429d9f9434d75ddb5fa05d4b493828adfe826a4))
	ROM_LOAD32_WORD("mp19151.21",   0x000002, 0x400000, CRC(e2a1b125) SHA1(cc5c2d9ab8a01f52e66969464f53ae3cefca6a09))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19153.27",   0x000000, 0x200000, CRC(136adfd0) SHA1(70ce4e609c8b003ff04518044c18d29089e6a353))
	ROM_LOAD32_WORD("mp19152.25",   0x000002, 0x200000, CRC(363769a2) SHA1(51b2f11a01fb72e151025771f8a8496993e605c2))

	ROM_REGION( 0x20000, REGION_CPU5, 0) // Communication program
	ROM_LOAD16_WORD_SWAP("ep18643a.7",   0x000000,  0x20000, CRC(b5e048ec) SHA1(8182e05a2ffebd590a936c1359c81e60caa79c2a))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19157.31",   0x080000,  0x80000, CRC(8ffea0cf) SHA1(439e784081329db2fe03419681150f3216f4ccff))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19154.32",   0x000000, 0x400000, CRC(7cd9e679) SHA1(b9812c4f3042f95febc96bcdd46e3b0724ad4b4f))
ROM_END

ROM_START( von )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18664b.15",  0x000000, 0x080000, CRC(27d0172c) SHA1(f3bcae9898c7d656eccb4d2546c9bb93daaefbb7))
	ROM_LOAD32_WORD("ep18665b.16",  0x000002, 0x080000, CRC(2f0142ee) SHA1(73f2a19a519ced8e0a1ab5cf69a4bf9d9841e288))
	ROM_LOAD32_WORD("ep18666.13",   0x100000, 0x080000, CRC(66edb432) SHA1(b67131b0158a58138380734dd5b9394b70010026))
	ROM_LOAD32_WORD("ep18667.14",   0x100002, 0x080000, CRC(b593d31f) SHA1(1e9f23f4052ab1b0275307cc80e51352f13bc319))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18648.11",   0x000000, 0x400000, CRC(2edbe489) SHA1(ded2e4b295be08970d13c387818c570c3afe8109))
	ROM_LOAD32_WORD("mp18649.12",   0x000002, 0x400000, CRC(e68c5aa6) SHA1(cdee1ba9247eda4282442d0522f8de7d7c86e1e6))
	ROM_LOAD32_WORD("mp18650.9",    0x800000, 0x400000, CRC(89a855b9) SHA1(5096db1da1f7e175000e89fca2a1dd3fd53030ea))
	ROM_LOAD32_WORD("mp18651.10",   0x800002, 0x400000, CRC(f4c23107) SHA1(f65984614111b12dd414db80751efe64fcf5ef16))

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp18662.29",   0x000000, 0x200000, CRC(a33d3335) SHA1(991bbe9dcbef8bfa96682e9d142623fc9b7c0879))
	ROM_LOAD32_WORD("mp18663.30",   0x000002, 0x200000, CRC(ea74a641) SHA1(a684e13c0afe2ef3f3108ae9b73389121368fc4e))

	ROM_REGION( 0x2000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18654.17",   0x000000, 0x400000, CRC(6a0caf29) SHA1(9f009f44e62ae0f9dec7a34a163bc186d1c4cbbd))
	ROM_LOAD32_WORD("mp18655.21",   0x000002, 0x400000, CRC(a4293e78) SHA1(af512c994bedbdaf3a5eeed607e771dcd87810fc))
	ROM_LOAD32_WORD("mp18656.18",   0x800000, 0x400000, CRC(b4f51e76) SHA1(eb71ada331576f2a7219d238ea07a61bcbf6381a))
	ROM_LOAD32_WORD("mp18657.22",   0x800002, 0x400000, CRC(a9be4674) SHA1(a918c2a3de78a08104480097edfb9d6aeaeda873))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18660.27",   0x000000, 0x200000, CRC(e53663e5) SHA1(0a4908be654bad4f00d7d58f0e42f631996911c9))
	ROM_LOAD32_WORD("mp18658.25",   0x000002, 0x200000, CRC(3d0fcd01) SHA1(c8626c879bfcf7abd095cac5dc03a04ae8629423))
	ROM_LOAD32_WORD("mp18661.28",   0x800000, 0x200000, CRC(52b50410) SHA1(64ea7b2f86745954e0b8a15d71203444705240a2))
	ROM_LOAD32_WORD("mp18659.26",   0x800002, 0x200000, CRC(27aa8ae2) SHA1(e9b756e5b4b1c19e52e47af03c773fee544be420))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // Communication program
	ROM_LOAD16_WORD_SWAP("ep18643a.7",   0x000000,  0x20000, CRC(b5e048ec) SHA1(8182e05a2ffebd590a936c1359c81e60caa79c2a))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18670.31",   0x080000,  0x80000, CRC(3e715f76) SHA1(4fd997e379a8cdb94ec3b1986b3ab443fc6fa12a))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18652.32",   0x000000, 0x400000, CRC(037eee53) SHA1(e592f9e97abe0a7bc9009d8327b93da9bc43749c))
	ROM_LOAD("mp18653.34",   0x400000, 0x400000, CRC(9ec3e7bf) SHA1(197bc8adc823e93128c1cebf69361a7c7297f808))
ROM_END

ROM_START( vonusa )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("vo-prog0.usa", 0x000000, 0x080000, CRC(6499cc59) SHA1(8289be295f021acbf0c903513ba97ae7de50dedb))
	ROM_LOAD32_WORD("vo-prog1.usa", 0x000002, 0x080000, CRC(0053b10f) SHA1(b89cc814b02b4ab5e37c75ee1a9cf57b88b63053))
	ROM_LOAD32_WORD("ep18666.13",   0x100000, 0x080000, CRC(66edb432) SHA1(b67131b0158a58138380734dd5b9394b70010026))
	ROM_LOAD32_WORD("ep18667.14",   0x100002, 0x080000, CRC(b593d31f) SHA1(1e9f23f4052ab1b0275307cc80e51352f13bc319))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18648.11",   0x000000, 0x400000, CRC(2edbe489) SHA1(ded2e4b295be08970d13c387818c570c3afe8109))
	ROM_LOAD32_WORD("mp18649.12",   0x000002, 0x400000, CRC(e68c5aa6) SHA1(cdee1ba9247eda4282442d0522f8de7d7c86e1e6))
	ROM_LOAD32_WORD("mp18650.9",    0x800000, 0x400000, CRC(89a855b9) SHA1(5096db1da1f7e175000e89fca2a1dd3fd53030ea))
	ROM_LOAD32_WORD("mp18651.10",   0x800002, 0x400000, CRC(f4c23107) SHA1(f65984614111b12dd414db80751efe64fcf5ef16))

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp18662.29",   0x000000, 0x200000, CRC(a33d3335) SHA1(991bbe9dcbef8bfa96682e9d142623fc9b7c0879))
	ROM_LOAD32_WORD("mp18663.30",   0x000002, 0x200000, CRC(ea74a641) SHA1(a684e13c0afe2ef3f3108ae9b73389121368fc4e))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18654.17",   0x000000, 0x400000, CRC(6a0caf29) SHA1(9f009f44e62ae0f9dec7a34a163bc186d1c4cbbd))
	ROM_LOAD32_WORD("mp18655.21",   0x000002, 0x400000, CRC(a4293e78) SHA1(af512c994bedbdaf3a5eeed607e771dcd87810fc))
	ROM_LOAD32_WORD("mp18656.18",   0x800000, 0x400000, CRC(b4f51e76) SHA1(eb71ada331576f2a7219d238ea07a61bcbf6381a))
	ROM_LOAD32_WORD("mp18657.22",   0x800002, 0x400000, CRC(a9be4674) SHA1(a918c2a3de78a08104480097edfb9d6aeaeda873))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18660.27",   0x000000, 0x200000, CRC(e53663e5) SHA1(0a4908be654bad4f00d7d58f0e42f631996911c9))
	ROM_LOAD32_WORD("mp18658.25",   0x000002, 0x200000, CRC(3d0fcd01) SHA1(c8626c879bfcf7abd095cac5dc03a04ae8629423))
	ROM_LOAD32_WORD("mp18661.28",   0x800000, 0x200000, CRC(52b50410) SHA1(64ea7b2f86745954e0b8a15d71203444705240a2))
	ROM_LOAD32_WORD("mp18659.26",   0x800002, 0x200000, CRC(27aa8ae2) SHA1(e9b756e5b4b1c19e52e47af03c773fee544be420))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // Communication program
	ROM_LOAD16_WORD_SWAP("ep18643a.7",   0x000000,  0x20000, CRC(b5e048ec) SHA1(8182e05a2ffebd590a936c1359c81e60caa79c2a))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18670.31",   0x080000,  0x80000, CRC(3e715f76) SHA1(4fd997e379a8cdb94ec3b1986b3ab443fc6fa12a))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18652.32",   0x000000, 0x400000, CRC(037eee53) SHA1(e592f9e97abe0a7bc9009d8327b93da9bc43749c))
	ROM_LOAD("mp18653.34",   0x400000, 0x400000, CRC(9ec3e7bf) SHA1(197bc8adc823e93128c1cebf69361a7c7297f808))
ROM_END

ROM_START( vstriker )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18068.15",   0x000000, 0x020000, CRC(74a47795) SHA1(3ba34bd467e11e768eda95ff345f5993fb9d6bca))
	ROM_LOAD32_WORD("ep18069.16",   0x000002, 0x020000, CRC(f6c3fcbf) SHA1(84bf16fc2a441cb724f4bc635a4c4209c240cfbf))
	ROM_LOAD32_WORD("ep18066.13",   0x040000, 0x020000, CRC(e774229e) SHA1(0ff20aa3e030df869767bb9614565acc9f3fe3b1))
	ROM_LOAD32_WORD("ep18067.14",   0x040002, 0x020000, CRC(7dfd950c) SHA1(d5eff8aff37fb0ef3c7f9d8bfca8460213b0f0a7))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18055.11",   0x000000, 0x200000, CRC(5aba9fc0) SHA1(40d45af7e58fa48b6afa85071c2bd1d4b5b5ffa5))
	ROM_LOAD32_WORD("mp18056.12",   0x000002, 0x200000, CRC(017f0c55) SHA1(744e5a02abd82fbeb875c5cd30c5543570140cff))
	ROM_LOAD32_WORD("mp18053.9",    0x400000, 0x200000, CRC(46c770c8) SHA1(000e9edfed49cc3dcc136f80e044dcd2b42378ce))
	ROM_LOAD32_WORD("mp18054.10",   0x400002, 0x200000, CRC(437af66e) SHA1(c5afa62100a93e160aa96b327a260cc7fee51fdc))
	ROM_LOAD32_WORD("ep18070.7",    0x800000, 0x080000, CRC(f52e4db5) SHA1(731452284c45329701258ee9fb8b7df6514fbba1))
	ROM_LOAD32_WORD("ep18071.8",    0x800002, 0x080000, CRC(1be63a7d) SHA1(c678f1f42de86cc968c3f823994d36c74b2e55fd))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18057.17",   0x000000, 0x200000, CRC(890d8806) SHA1(fe73e4ea310e13b172e49d39c7eafba8f9052e67))
	ROM_LOAD32_WORD("mp18059.21",   0x000002, 0x200000, CRC(c5cdf534) SHA1(fd127d33bc5a78b81aaa7d5886beca2192a62867))
	ROM_LOAD32_WORD("mp18058.18",   0x400000, 0x200000, CRC(d4cbdf7c) SHA1(fe783c5bc94c2581fd990f0f0a705bdc5c05a386))
	ROM_LOAD32_WORD("mp18060.22",   0x400002, 0x200000, CRC(93d5c95f) SHA1(bca83f024d85c97ca59fae8d9097fc510ec0fc7f))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18062.27",   0x000000, 0x200000, CRC(126e7de3) SHA1(0810364934dee8d5035cef623d01dfbacc64bf2b))
	ROM_LOAD32_WORD("mp18061.25",   0x000002, 0x200000, CRC(c37f1c67) SHA1(c917046c2d98af17c59ceb0ea4f89d215cc0ead8))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18072.31",   0x080000,  0x20000, CRC(73eabb58) SHA1(4f6d70d6e0d7b469c5f2527efb08f208f4aa017e))

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18063.32",   0x000000, 0x200000, CRC(b74d7c8a) SHA1(da0bc8b3822b01087b6f9de0446cab1eb6617e8e))
	ROM_LOAD("mp18064.33",   0x200000, 0x200000, CRC(783b9910) SHA1(108b23bb57e3133c555083aa4f9bc573ac6e3152))
	ROM_LOAD("mp18065.34",   0x400000, 0x200000, CRC(046b55fe) SHA1(2db7eabf4318881a67b10dba24f6f0cd68940ace))
ROM_END

ROM_START( vstrikra )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18068a.15",  0x000000, 0x020000, CRC(afc69b54) SHA1(2127bde1de3cd6663c31cf2126847815234e09a4))
	ROM_LOAD32_WORD("ep18069a.16",  0x000002, 0x020000, CRC(0243250c) SHA1(3cbeac09d503a19c5950cf70e3b329f791acfa13))
	ROM_LOAD32_WORD("ep18066a.13",  0x040000, 0x020000, CRC(e658b33a) SHA1(33266e6372e73f670688f58e51081ec5a7deec11))
	ROM_LOAD32_WORD("ep18067a.14",  0x040002, 0x020000, CRC(49e94047) SHA1(56c8d1a365985886dffeddf24d692ce6b377760a))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18055.11",   0x000000, 0x200000, CRC(5aba9fc0) SHA1(40d45af7e58fa48b6afa85071c2bd1d4b5b5ffa5))
	ROM_LOAD32_WORD("mp18056.12",   0x000002, 0x200000, CRC(017f0c55) SHA1(744e5a02abd82fbeb875c5cd30c5543570140cff))
	ROM_LOAD32_WORD("mp18053.9",    0x400000, 0x200000, CRC(46c770c8) SHA1(000e9edfed49cc3dcc136f80e044dcd2b42378ce))
	ROM_LOAD32_WORD("mp18054.10",   0x400002, 0x200000, CRC(437af66e) SHA1(c5afa62100a93e160aa96b327a260cc7fee51fdc))
	ROM_LOAD32_WORD("ep18070a.7",   0x800000, 0x080000, CRC(1961e2fc) SHA1(12ead9b782e092346b7cd5a7343b302f546fe066))
	ROM_LOAD32_WORD("ep18071a.8",   0x800002, 0x080000, CRC(b2492dca) SHA1(3b35522ab8e1fdfa327245fef797e3d7c0cceb85))

	ROM_REGION( 0x800000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18057.17",   0x000000, 0x200000, CRC(890d8806) SHA1(fe73e4ea310e13b172e49d39c7eafba8f9052e67))
	ROM_LOAD32_WORD("mp18059.21",   0x000002, 0x200000, CRC(c5cdf534) SHA1(fd127d33bc5a78b81aaa7d5886beca2192a62867))
	ROM_LOAD32_WORD("mp18058.18",   0x400000, 0x200000, CRC(d4cbdf7c) SHA1(fe783c5bc94c2581fd990f0f0a705bdc5c05a386))
	ROM_LOAD32_WORD("mp18060.22",   0x400002, 0x200000, CRC(93d5c95f) SHA1(bca83f024d85c97ca59fae8d9097fc510ec0fc7f))

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18062.27",   0x000000, 0x200000, CRC(126e7de3) SHA1(0810364934dee8d5035cef623d01dfbacc64bf2b))
	ROM_LOAD32_WORD("mp18061.25",   0x000002, 0x200000, CRC(c37f1c67) SHA1(c917046c2d98af17c59ceb0ea4f89d215cc0ead8))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18072.31",   0x080000,  0x20000, CRC(73eabb58) SHA1(4f6d70d6e0d7b469c5f2527efb08f208f4aa017e))

	ROM_REGION( 0x600000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18063.32",   0x000000, 0x200000, CRC(b74d7c8a) SHA1(da0bc8b3822b01087b6f9de0446cab1eb6617e8e))
	ROM_LOAD("mp18064.33",   0x200000, 0x200000, CRC(783b9910) SHA1(108b23bb57e3133c555083aa4f9bc573ac6e3152))
	ROM_LOAD("mp18065.34",   0x400000, 0x200000, CRC(046b55fe) SHA1(2db7eabf4318881a67b10dba24f6f0cd68940ace))
ROM_END

ROM_START( dynabb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep19833a.15",  0x000000, 0x080000, CRC(d99ed1b2) SHA1(b04613d564c04c35feafccad56ed85810d894185))
	ROM_LOAD32_WORD("ep19834a.16",  0x000002, 0x080000, CRC(24192bb1) SHA1(c535ab4b38ffd42f03eed6a5a1706e867eaccd67))
	ROM_LOAD32_WORD("ep19831a.13",  0x100000, 0x080000, CRC(0527ea40) SHA1(8e80e2627aafe395d8ced4a97ba50cd9a781fb45))
	ROM_LOAD32_WORD("ep19832a.14",  0x100002, 0x080000, CRC(2f380a40) SHA1(d770dfd70aa14dcc716aa47e6cbf26f32649f294))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp19841.11",   0x000000, 0x400000, CRC(989309af) SHA1(d527f46865d00a91d5b38a93dc38baf62f372cb1))
	ROM_LOAD32_WORD("mp19842.12",   0x000002, 0x400000, CRC(eec54070) SHA1(29ed4a005b52f6e16492998183ec4e5f7475022b))
	ROM_LOAD32_WORD("mp19839.9",    0x800000, 0x400000, CRC(d5a74cf4) SHA1(ddea9cfc0a14461448acae2eed2092829ef3b418))
	ROM_LOAD32_WORD("mp19840.10",   0x800002, 0x400000, CRC(45704e95) SHA1(2a325ee39f9d719399040ed2a41123bcf0c6f385))
	ROM_LOAD32_WORD("mp19837.7",   0x1000000, 0x400000, CRC(c02187d9) SHA1(1da108a2ec00e3fc472b1a819655aff8c679051d))
	ROM_LOAD32_WORD("mp19838.8",   0x1000002, 0x400000, CRC(546b61cd) SHA1(0cc0edd0a9c288143168d63a7d48d0fbfa64d8bf))
	ROM_LOAD32_WORD("mp19835.5",   0x1800000, 0x400000, CRC(a3b0a37c) SHA1(dcde1946008ab86c7fca212ec57c1cc468f30c58))
	ROM_LOAD32_WORD("mp19836.6",   0x1800002, 0x400000, CRC(d70a32aa) SHA1(fd56bb284eb66e6c078b386a0db1c2b10dc1dd4a))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp19843.17",   0x000000, 0x400000, CRC(019bc583) SHA1(8889a9438d8f3ea50058372ad03ebd4653f23313))
	ROM_LOAD32_WORD("mp19845.21",   0x000002, 0x400000, CRC(2d23e73a) SHA1(63e5859518172f88a5ba98b69309d4162c233cf0))
	ROM_LOAD32_WORD("mp19844.18",   0x800000, 0x400000, CRC(150198d6) SHA1(3ea5c3e41eb95e715860619f771bc580c91b095f))
	ROM_LOAD32_WORD("mp19846.22",   0x800002, 0x400000, CRC(fe53cd17) SHA1(58eab07976972917c345a8d3a50ff1e96e5fa798))

	ROM_REGION( 0x800000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp19848.27",  0x0000000, 0x400000, CRC(4c0526b7) SHA1(e8db7125be8a052e41a00c69cc08ca0d75b3b96f))
	ROM_LOAD32_WORD("mp19847.25",  0x0000002, 0x400000, CRC(fe55edbd) SHA1(b0b6135b23349d7d6ae007002d8df83748cab7b1))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep19849.31",   0x080000,  0x80000, CRC(b0d5bff0) SHA1(1fb824adaf3ed330a8039be726a87eb85c00abd7))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp19880.32",   0x000000, 0x200000, CRC(e1fd27bf) SHA1(a7189ad398138a91f96b192cb7c112c0301dcda4))
	ROM_LOAD("mp19850.33",   0x200000, 0x200000, CRC(dc644077) SHA1(8765bdb1d471dbeea065a97ae131f2d8f78aa13d))
	ROM_LOAD("mp19851.34",   0x400000, 0x200000, CRC(cfda4efd) SHA1(14d55f127da6673c538c2ef9be34a4e02ca449f3))
	ROM_LOAD("mp19853.35",   0x600000, 0x200000, CRC(cfc64857) SHA1(cf51fafb3d45bf799b9ccb407bee862e15c95981))
ROM_END

ROM_START( fvipers )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("ep18606d.15",  0x000000, 0x020000, CRC(7334de7d) SHA1(d10355198a3f62b503701f44dc49bfe018c787d1))
	ROM_LOAD32_WORD("ep18607d.16",  0x000002, 0x020000, CRC(700d2ade) SHA1(656e25a6389f04f7fb9099f0b41fb03fa645a2f0))
	ROM_LOAD32_WORD("ep18604d.13",  0x040000, 0x020000, CRC(704fdfcf) SHA1(52b6ae90231d40a3ece133debaeb210fc36c6fcb))
	ROM_LOAD32_WORD("ep18605d.14",  0x040002, 0x020000, CRC(7dddf81f) SHA1(3e0da0eaf1f98dbbd4ca5f78c04052b347b234b2))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mp18614.11",   0x000000, 0x400000, CRC(0ebc899f) SHA1(49c80b11b207cba4ec10fbb7cc140f3a5b039e82))
	ROM_LOAD32_WORD("mp18615.12",   0x000002, 0x400000, CRC(018abdb7) SHA1(59e5b6378404e10ace4f3675428d61d3ae9d1963))
	ROM_LOAD32_WORD("mp18612.9",    0x800000, 0x400000, CRC(1f174cd1) SHA1(89b56dd2f350edd093dc06f4cc258652c26b1d45))
	ROM_LOAD32_WORD("mp18613.10",   0x800002, 0x400000, CRC(f057cdf2) SHA1(e16d5de2a00670aba4fbe0dc88ccf317de9842be))
	ROM_LOAD32_WORD("ep18610d.7",  0x1000000, 0x080000, CRC(a1871703) SHA1(8d7b362a8fd9d63f5cea2f3fab97e5fe3fa30d87))
	ROM_RELOAD     (               0x1100000, 0x080000)
	ROM_RELOAD     (               0x1200000, 0x080000)
	ROM_RELOAD     (               0x1300000, 0x080000)
	ROM_RELOAD     (               0x1400000, 0x080000)
	ROM_RELOAD     (               0x1500000, 0x080000)
	ROM_RELOAD     (               0x1600000, 0x080000)
	ROM_RELOAD     (               0x1700000, 0x080000)
	ROM_LOAD32_WORD("ep18611d.8",  0x1000002, 0x080000, CRC(39a75fee) SHA1(c962805f03e2503dd1671ba3e906c6e306a92e48))
	ROM_RELOAD     (               0x1100002, 0x080000)
	ROM_RELOAD     (               0x1200002, 0x080000)
	ROM_RELOAD     (               0x1300002, 0x080000)
	ROM_RELOAD     (               0x1400002, 0x080000)
	ROM_RELOAD     (               0x1500002, 0x080000)
	ROM_RELOAD     (               0x1600002, 0x080000)
	ROM_RELOAD     (               0x1700002, 0x080000)
	ROM_LOAD32_WORD("ep18608d.5",  0x1800000, 0x080000, CRC(5bc11881) SHA1(97ce5faf9719cb02dd3a15d47245cc4634f08fcb))
	ROM_RELOAD     (               0x1900000, 0x080000)
	ROM_RELOAD     (               0x1a00000, 0x080000)
	ROM_RELOAD     (               0x1b00000, 0x080000)
	ROM_RELOAD     (               0x1c00000, 0x080000)
	ROM_RELOAD     (               0x1d00000, 0x080000)
	ROM_RELOAD     (               0x1e00000, 0x080000)
	ROM_RELOAD     (               0x1f00000, 0x080000)
	ROM_LOAD32_WORD("ep18609d.6",  0x1800002, 0x080000, CRC(cd426035) SHA1(94c85a656c86bc4880db6bff2ef795ec30f62f39))
	ROM_RELOAD     (               0x1900002, 0x080000)
	ROM_RELOAD     (               0x1a00002, 0x080000)
	ROM_RELOAD     (               0x1b00002, 0x080000)
	ROM_RELOAD     (               0x1c00002, 0x080000)
	ROM_RELOAD     (               0x1d00002, 0x080000)
	ROM_RELOAD     (               0x1e00002, 0x080000)
	ROM_RELOAD     (               0x1f00002, 0x080000)

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGPx4 program
	ROM_LOAD32_WORD("mp18622.29",   0x000000, 0x200000, CRC(c74d99e3) SHA1(9914be9925b86af6af670745b5eba3a9e4f24af9))
	ROM_LOAD32_WORD("mp18623.30",   0x000002, 0x200000, CRC(746ae931) SHA1(a6f0f589ad174a34493ee24dc0cb509ead3aed70))

	ROM_REGION( 0xc00000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mp18616.17",   0x000000, 0x200000, CRC(15a239be) SHA1(1a33c48f99eed20da4b622219d21ec5995acc9aa))
	ROM_LOAD32_WORD("mp18619.21",   0x000002, 0x200000, CRC(9d5e8e2b) SHA1(f79ae0a7b966ddb0948b464d233845d4f362a2e7))
	ROM_LOAD32_WORD("mp18617.18",   0x400000, 0x200000, CRC(a62cab7d) SHA1(f20a545148f2a1d6f4f1c897f1ed82ad17429dce))
	ROM_LOAD32_WORD("mp18620.22",   0x400002, 0x200000, CRC(4d432afd) SHA1(30a1ef1e309a163b2d8756810fc33debf069141c))
	ROM_LOAD32_WORD("mp18618.19",   0x800000, 0x200000, CRC(adab589f) SHA1(67818ec4185da17f1549fb3a125cade267a46a48))
	ROM_LOAD32_WORD("mp18621.23",   0x800002, 0x200000, CRC(f5eeaa95) SHA1(38d7019afcef6dbe292354d717fd49da511cbc2b))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mp18626.27",   0x000000, 0x200000, CRC(9df0a961) SHA1(d8fb4bbbdc00303330047be380a79da7838d4fd5))
	ROM_LOAD32_WORD("mp18624.25",   0x000002, 0x200000, CRC(1d74433e) SHA1(5b6d2d17609ae741546d99d40f575bb24d62b5d3))
	ROM_LOAD32_WORD("mp18627.28",   0x800000, 0x200000, CRC(946175a0) SHA1(8b6e5e1342f98c9c6f2f7d61e843275d244f331a))
	ROM_LOAD32_WORD("mp18625.26",   0x800002, 0x200000, CRC(182fd572) SHA1(b09a682eff7e835ff8c33aaece12f3727a91dd5e))

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("ep18628.31",   0x080000,  0x80000, CRC(aa7dd79f) SHA1(d8bd1485273652d7c2a303bbdcdf607d3b530283))

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mp18629.32",   0x000000, 0x200000, CRC(5d0006cc) SHA1(f6d2552ffc5473836aafb06735b62f65ef8f5ef5))
	ROM_LOAD("mp18630.33",   0x200000, 0x200000, CRC(9d405615) SHA1(7e7ffbb4ec080a0815c6ca49b9d8efe1f676203b))
	ROM_LOAD("mp18631.34",   0x400000, 0x200000, CRC(9dae5b45) SHA1(055ac989eafb81749326520d0be264f7a984c627))
	ROM_LOAD("mp18632.35",   0x600000, 0x200000, CRC(39da6805) SHA1(9e9523b7c2bc50f869d062f80955da1281951299))
ROM_END

/* original Model 2 w/Model 1 sound board */

ROM_START( daytona )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("epr16722a.12",   0x000000, 0x020000, CRC(48b94318) SHA1(a476a9a3531beef760c88c9634ed4a7d270e8ee7))
	ROM_LOAD32_WORD("epr16723a.13",   0x000002, 0x020000, CRC(8af8b32d) SHA1(2039ec1f8da524176fcf85473c10a8b6e49e139a))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mpr16528.10",    0x000000, 0x200000, CRC(9ce591f6) SHA1(e22fc8a70b533f7a6191f5952c581fb8f9627906))
	ROM_LOAD32_WORD("mpr16529.11",    0x000002, 0x200000, CRC(f7095eaf) SHA1(da3c922f950dd730ea348eae12aa1cb69cee9a58))
	ROM_LOAD32_WORD("mpr16808.8",    0x400000, 0x200000, CRC(44f1f5a0) SHA1(343866a6e2187a8ebc17f6727080f9f2f9ac9200))
	ROM_LOAD32_WORD("mpr16809.9",    0x400002, 0x200000, CRC(37a2dd12) SHA1(8192d8698d6bd52ee11cc28917aff5840c447627))
	ROM_LOAD32_WORD("epr16724a.6",   0x800000, 0x080000, CRC(469f10fd) SHA1(7fad3b8d03960e5e1f7a6cb36509238977e00fcc))
	ROM_RELOAD     (                0x900000, 0x080000 )
	ROM_RELOAD     (                0xa00000, 0x080000 )
	ROM_RELOAD     (                0xb00000, 0x080000 )
	ROM_RELOAD     (                0xc00000, 0x080000 )
	ROM_RELOAD     (                0xd00000, 0x080000 )
	ROM_RELOAD     (                0xe00000, 0x080000 )
	ROM_RELOAD     (                0xf00000, 0x080000 )
	ROM_LOAD32_WORD("epr16725a.7",   0x800002, 0x080000, CRC(ba0df8db) SHA1(d0c5581c56500b5266cab8e8151db24fcbdea0d7))
	ROM_RELOAD     (                0x900002, 0x080000 )
	ROM_RELOAD     (                0xa00002, 0x080000 )
	ROM_RELOAD     (                0xb00002, 0x080000 )
	ROM_RELOAD     (                0xc00002, 0x080000 )
	ROM_RELOAD     (                0xd00002, 0x080000 )
	ROM_RELOAD     (                0xe00002, 0x080000 )
	ROM_RELOAD     (                0xf00002, 0x080000 )

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
	ROM_LOAD32_WORD("mpr16537.28",    0x000000, 0x100000, CRC(44ce6834) SHA1(61b7ee8f9667683bb723bb930945e550ba84daa4))
	ROM_LOAD32_WORD("mpr16536.29",    0x000002, 0x100000, CRC(adea6e68) SHA1(fd9065ea1a19791e813ec163525c072a2e985e61))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mpr16523.16",    0x000000, 0x200000, CRC(2f484d42) SHA1(0b83a3fc92b7d913a14cfb01d688c63555c17c41))
	ROM_LOAD32_WORD("mpr16518.20",    0x000002, 0x200000, CRC(df683bf7) SHA1(16afe5029591f3536b5b75d9cf50a34d0ea72c3d))
	ROM_LOAD32_WORD("mpr16524.17",    0x400000, 0x100000, CRC(fd5ce5e7) SHA1(b33098977b6fc980e3f21096f96745199a2155a9))
	ROM_LOAD32_WORD("mpr16519.21",    0x400002, 0x100000, CRC(5348d519) SHA1(b0fa271e6d350ac5112a258bb9c0fa569e05c739))
	ROM_LOAD32_WORD("mpr16525.18",    0x600000, 0x100000, CRC(04d56b09) SHA1(62d28ba02cdcc5de89b558ea32ac92aca95d1ff8))
	ROM_LOAD32_WORD("mpr16520.22",    0x600002, 0x100000, CRC(937f390b) SHA1(e8fe943a2824101d0a37dada5103daa1009a531d))
	ROM_LOAD32_WORD("mpr16772.19",    0x800000, 0x200000, CRC(770ed912) SHA1(1789f35dd403f73f8be18495a0fe4ad1e6841417))
	ROM_LOAD32_WORD("mpr16771.23",    0x800002, 0x200000, CRC(a2205124) SHA1(257a3675e4ef6adbf61285a5daa5954223c28cb2))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mpr16521.24",    0x000000, 0x200000, CRC(af1934fb) SHA1(a6a21a23cd34d0de6d3e6a5c3c2687f905d0dc2a))
	ROM_LOAD32_WORD("mpr16522.25",    0x000002, 0x200000, CRC(55d39a57) SHA1(abf7b0fc0f111f90da42463d600db9fa32e95efe))
	ROM_LOAD32_WORD("mpr16769.26",    0x400000, 0x200000, CRC(e57429e9) SHA1(8c712ab09e61ef510741a55f29b3c4e497471372))
	ROM_LOAD32_WORD("mpr16770.27",    0x400002, 0x200000, CRC(f9fa7bfb) SHA1(8aa933b74d4e05dc49987238705e50b00e5dae73))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // Communication program
        ROM_LOAD( "16726.bin",    0x000000, 0x020000, CRC(c179b8c7) SHA1(86d3e65c77fb53b1d380b629348f4ab5b3d39228) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("epr16720.7",    0x000000,  0x20000, CRC(8e73cffd) SHA1(9933ccc0757e8c86e0adb938d1c89210b26841ea))
	ROM_LOAD16_WORD_SWAP("epr16721.8",    0x020000,  0x20000, CRC(1bb3b7b7) SHA1(ee2fd1480e535fc37e9932e6fe4e31344559fc87))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mpr16491.32",    0x000000, 0x200000, CRC(89920903) SHA1(06d1d55470ae99f8de0f8c88c694f34c4eb13668))
	ROM_LOAD("mpr16492.33",    0x200000, 0x200000, CRC(459e701b) SHA1(2054f69cecad677eb00c6a3051f5b5d90885e19b))

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) // Samples
	ROM_LOAD("mpr16493.4",    0x000000, 0x200000, CRC(9990db15) SHA1(ea9a8b45a07dccaae62be7cf095532ce7596a70c))
	ROM_LOAD("mpr16494.5",    0x200000, 0x200000, CRC(600e1d6c) SHA1(d4e246fc57a16ff562bbcbccf6a739b706f58696))

	MODEL2_CPU_BOARD /* Model 2 CPU board extra roms */

	ROM_REGION( 0x10000, REGION_USER7, 0 ) // Unknown ROM
	ROM_LOAD("14869c.25",   0x000000, 0x010000, CRC(24b68e64) SHA1(c19d044d4c2fe551474492aa51922587394dd371))
ROM_END

ROM_START( daytonat )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "turbo1",      0x000000, 0x080000, CRC(0b3d5d4e) SHA1(1660959cb383e22f0d6204547c30cf5fe9272b03) )
        ROM_LOAD32_WORD( "turbo2",      0x000002, 0x080000, CRC(f7d4e866) SHA1(c8c43904257f718665f9f7a89838eba14bde9465) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mpr16528.10",    0x000000, 0x200000, CRC(9ce591f6) SHA1(e22fc8a70b533f7a6191f5952c581fb8f9627906))
	ROM_LOAD32_WORD("mpr16529.11",    0x000002, 0x200000, CRC(f7095eaf) SHA1(da3c922f950dd730ea348eae12aa1cb69cee9a58))
	ROM_LOAD32_WORD("mpr16808.8",    0x400000, 0x200000, CRC(44f1f5a0) SHA1(343866a6e2187a8ebc17f6727080f9f2f9ac9200))
	ROM_LOAD32_WORD("mpr16809.9",    0x400002, 0x200000, CRC(37a2dd12) SHA1(8192d8698d6bd52ee11cc28917aff5840c447627))
	ROM_LOAD32_WORD("epr16724a.6",   0x800000, 0x080000, CRC(469f10fd) SHA1(7fad3b8d03960e5e1f7a6cb36509238977e00fcc))
	ROM_RELOAD     (                0x900000, 0x080000 )
	ROM_RELOAD     (                0xa00000, 0x080000 )
	ROM_RELOAD     (                0xb00000, 0x080000 )
	ROM_RELOAD     (                0xc00000, 0x080000 )
	ROM_RELOAD     (                0xd00000, 0x080000 )
	ROM_RELOAD     (                0xe00000, 0x080000 )
	ROM_RELOAD     (                0xf00000, 0x080000 )
	ROM_LOAD32_WORD("epr16725a.7",   0x800002, 0x080000, CRC(ba0df8db) SHA1(d0c5581c56500b5266cab8e8151db24fcbdea0d7))
	ROM_RELOAD     (                0x900002, 0x080000 )
	ROM_RELOAD     (                0xa00002, 0x080000 )
	ROM_RELOAD     (                0xb00002, 0x080000 )
	ROM_RELOAD     (                0xc00002, 0x080000 )
	ROM_RELOAD     (                0xd00002, 0x080000 )
	ROM_RELOAD     (                0xe00002, 0x080000 )
	ROM_RELOAD     (                0xf00002, 0x080000 )

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
	ROM_LOAD32_WORD("mpr16537.28",    0x000000, 0x100000, CRC(44ce6834) SHA1(61b7ee8f9667683bb723bb930945e550ba84daa4))
	ROM_LOAD32_WORD("mpr16536.29",    0x000002, 0x100000, CRC(adea6e68) SHA1(fd9065ea1a19791e813ec163525c072a2e985e61))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mpr16523.16",    0x000000, 0x200000, CRC(2f484d42) SHA1(0b83a3fc92b7d913a14cfb01d688c63555c17c41))
	ROM_LOAD32_WORD("mpr16518.20",    0x000002, 0x200000, CRC(df683bf7) SHA1(16afe5029591f3536b5b75d9cf50a34d0ea72c3d))
	ROM_LOAD32_WORD("mpr16524.17",    0x400000, 0x100000, CRC(fd5ce5e7) SHA1(b33098977b6fc980e3f21096f96745199a2155a9))
	ROM_LOAD32_WORD("mpr16519.21",    0x400002, 0x100000, CRC(5348d519) SHA1(b0fa271e6d350ac5112a258bb9c0fa569e05c739))
	ROM_LOAD32_WORD("mpr16525.18",    0x600000, 0x100000, CRC(04d56b09) SHA1(62d28ba02cdcc5de89b558ea32ac92aca95d1ff8))
	ROM_LOAD32_WORD("mpr16520.22",    0x600002, 0x100000, CRC(937f390b) SHA1(e8fe943a2824101d0a37dada5103daa1009a531d))
	ROM_LOAD32_WORD("mpr16772.19",    0x800000, 0x200000, CRC(770ed912) SHA1(1789f35dd403f73f8be18495a0fe4ad1e6841417))
	ROM_LOAD32_WORD("mpr16771.23",    0x800002, 0x200000, CRC(a2205124) SHA1(257a3675e4ef6adbf61285a5daa5954223c28cb2))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mpr16521.24",    0x000000, 0x200000, CRC(af1934fb) SHA1(a6a21a23cd34d0de6d3e6a5c3c2687f905d0dc2a))
	ROM_LOAD32_WORD("mpr16522.25",    0x000002, 0x200000, CRC(55d39a57) SHA1(abf7b0fc0f111f90da42463d600db9fa32e95efe))
	ROM_LOAD32_WORD("mpr16769.26",    0x400000, 0x200000, CRC(e57429e9) SHA1(8c712ab09e61ef510741a55f29b3c4e497471372))
	ROM_LOAD32_WORD("mpr16770.27",    0x400002, 0x200000, CRC(f9fa7bfb) SHA1(8aa933b74d4e05dc49987238705e50b00e5dae73))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // Communication program
        ROM_LOAD( "16726.bin",    0x000000, 0x020000, CRC(c179b8c7) SHA1(86d3e65c77fb53b1d380b629348f4ab5b3d39228) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("epr16720.7",    0x000000,  0x20000, CRC(8e73cffd) SHA1(9933ccc0757e8c86e0adb938d1c89210b26841ea))
	ROM_LOAD16_WORD_SWAP("epr16721.8",    0x020000,  0x20000, CRC(1bb3b7b7) SHA1(ee2fd1480e535fc37e9932e6fe4e31344559fc87))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mpr16491.32",    0x000000, 0x200000, CRC(89920903) SHA1(06d1d55470ae99f8de0f8c88c694f34c4eb13668))
	ROM_LOAD("mpr16492.33",    0x200000, 0x200000, CRC(459e701b) SHA1(2054f69cecad677eb00c6a3051f5b5d90885e19b))

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) // Samples
	ROM_LOAD("mpr16493.4",    0x000000, 0x200000, CRC(9990db15) SHA1(ea9a8b45a07dccaae62be7cf095532ce7596a70c))
	ROM_LOAD("mpr16494.5",    0x200000, 0x200000, CRC(600e1d6c) SHA1(d4e246fc57a16ff562bbcbccf6a739b706f58696))

	MODEL2_CPU_BOARD /* Model 2 CPU board extra roms */

	ROM_REGION( 0x10000, REGION_USER7, 0 ) // Unknown ROM
	ROM_LOAD("14869c.25",   0x000000, 0x010000, CRC(24b68e64) SHA1(c19d044d4c2fe551474492aa51922587394dd371))
ROM_END

ROM_START( daytonam )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD( "maxx.12",      0x000000, 0x020000, CRC(604ef2d9) SHA1(b1d5f0d41bea2e74fb9346da35a5041f4464265e) )
        ROM_LOAD32_WORD( "maxx.13",      0x000002, 0x020000, CRC(7d319970) SHA1(5bc150a77f20a29f54acdf5043fb1e8e55f6b08b) )
        ROM_LOAD32_WORD( "maxx.14",      0x040000, 0x020000, CRC(2debfce0) SHA1(b0f578ae68d49a3eebaf9b453a1ad774c8620476) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mpr16528.10",    0x000000, 0x200000, CRC(9ce591f6) SHA1(e22fc8a70b533f7a6191f5952c581fb8f9627906))
	ROM_LOAD32_WORD("mpr16529.11",    0x000002, 0x200000, CRC(f7095eaf) SHA1(da3c922f950dd730ea348eae12aa1cb69cee9a58))
	ROM_LOAD32_WORD("mpr16808.8",    0x400000, 0x200000, CRC(44f1f5a0) SHA1(343866a6e2187a8ebc17f6727080f9f2f9ac9200))
	ROM_LOAD32_WORD("mpr16809.9",    0x400002, 0x200000, CRC(37a2dd12) SHA1(8192d8698d6bd52ee11cc28917aff5840c447627))
	ROM_LOAD32_WORD("epr16724a.6",   0x800000, 0x080000, CRC(469f10fd) SHA1(7fad3b8d03960e5e1f7a6cb36509238977e00fcc))
	ROM_RELOAD     (                0x900000, 0x080000 )
	ROM_RELOAD     (                0xa00000, 0x080000 )
	ROM_RELOAD     (                0xb00000, 0x080000 )
	ROM_RELOAD     (                0xc00000, 0x080000 )
	ROM_RELOAD     (                0xd00000, 0x080000 )
	ROM_RELOAD     (                0xe00000, 0x080000 )
	ROM_RELOAD     (                0xf00000, 0x080000 )
	ROM_LOAD32_WORD("epr16725a.7",   0x800002, 0x080000, CRC(ba0df8db) SHA1(d0c5581c56500b5266cab8e8151db24fcbdea0d7))
	ROM_RELOAD     (                0x900002, 0x080000 )
	ROM_RELOAD     (                0xa00002, 0x080000 )
	ROM_RELOAD     (                0xb00002, 0x080000 )
	ROM_RELOAD     (                0xc00002, 0x080000 )
	ROM_RELOAD     (                0xd00002, 0x080000 )
	ROM_RELOAD     (                0xe00002, 0x080000 )
	ROM_RELOAD     (                0xf00002, 0x080000 )

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
	ROM_LOAD32_WORD("mpr16537.28",    0x000000, 0x100000, CRC(44ce6834) SHA1(61b7ee8f9667683bb723bb930945e550ba84daa4))
	ROM_LOAD32_WORD("mpr16536.29",    0x000002, 0x100000, CRC(adea6e68) SHA1(fd9065ea1a19791e813ec163525c072a2e985e61))

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mpr16523.16",    0x000000, 0x200000, CRC(2f484d42) SHA1(0b83a3fc92b7d913a14cfb01d688c63555c17c41))
	ROM_LOAD32_WORD("mpr16518.20",    0x000002, 0x200000, CRC(df683bf7) SHA1(16afe5029591f3536b5b75d9cf50a34d0ea72c3d))
	ROM_LOAD32_WORD("mpr16524.17",    0x400000, 0x100000, CRC(fd5ce5e7) SHA1(b33098977b6fc980e3f21096f96745199a2155a9))
	ROM_LOAD32_WORD("mpr16519.21",    0x400002, 0x100000, CRC(5348d519) SHA1(b0fa271e6d350ac5112a258bb9c0fa569e05c739))
	ROM_LOAD32_WORD("mpr16525.18",    0x600000, 0x100000, CRC(04d56b09) SHA1(62d28ba02cdcc5de89b558ea32ac92aca95d1ff8))
	ROM_LOAD32_WORD("mpr16520.22",    0x600002, 0x100000, CRC(937f390b) SHA1(e8fe943a2824101d0a37dada5103daa1009a531d))
	ROM_LOAD32_WORD("mpr16772.19",    0x800000, 0x200000, CRC(770ed912) SHA1(1789f35dd403f73f8be18495a0fe4ad1e6841417))
	ROM_LOAD32_WORD("mpr16771.23",    0x800002, 0x200000, CRC(a2205124) SHA1(257a3675e4ef6adbf61285a5daa5954223c28cb2))

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mpr16521.24",    0x000000, 0x200000, CRC(af1934fb) SHA1(a6a21a23cd34d0de6d3e6a5c3c2687f905d0dc2a))
	ROM_LOAD32_WORD("mpr16522.25",    0x000002, 0x200000, CRC(55d39a57) SHA1(abf7b0fc0f111f90da42463d600db9fa32e95efe))
	ROM_LOAD32_WORD("mpr16769.26",    0x400000, 0x200000, CRC(e57429e9) SHA1(8c712ab09e61ef510741a55f29b3c4e497471372))
	ROM_LOAD32_WORD("mpr16770.27",    0x400002, 0x200000, CRC(f9fa7bfb) SHA1(8aa933b74d4e05dc49987238705e50b00e5dae73))

	ROM_REGION( 0x20000, REGION_CPU4, 0) // Communication program
        ROM_LOAD( "16726.bin",    0x000000, 0x020000, CRC(c179b8c7) SHA1(86d3e65c77fb53b1d380b629348f4ab5b3d39228) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("epr16720.7",    0x000000,  0x20000, CRC(8e73cffd) SHA1(9933ccc0757e8c86e0adb938d1c89210b26841ea))
	ROM_LOAD16_WORD_SWAP("epr16721.8",    0x020000,  0x20000, CRC(1bb3b7b7) SHA1(ee2fd1480e535fc37e9932e6fe4e31344559fc87))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mpr16491.32",    0x000000, 0x200000, CRC(89920903) SHA1(06d1d55470ae99f8de0f8c88c694f34c4eb13668))
	ROM_LOAD("mpr16492.33",    0x200000, 0x200000, CRC(459e701b) SHA1(2054f69cecad677eb00c6a3051f5b5d90885e19b))

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) // Samples
	ROM_LOAD("mpr16493.4",    0x000000, 0x200000, CRC(9990db15) SHA1(ea9a8b45a07dccaae62be7cf095532ce7596a70c))
	ROM_LOAD("mpr16494.5",    0x200000, 0x200000, CRC(600e1d6c) SHA1(d4e246fc57a16ff562bbcbccf6a739b706f58696))

	MODEL2_CPU_BOARD /* Model 2 CPU board extra roms */

	ROM_REGION( 0x10000, REGION_USER7, 0 ) // Unknown ROM
	ROM_LOAD("14869c.25",   0x000000, 0x010000, CRC(24b68e64) SHA1(c19d044d4c2fe551474492aa51922587394dd371))
ROM_END

ROM_START( vcop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
        ROM_LOAD32_WORD( "ep17166b.012", 0x000000, 0x020000, CRC(a5647c59) SHA1(0a9e0be447d3591e82efd40ef4acbfe7ae211579) )
        ROM_LOAD32_WORD( "ep17167b.013", 0x000002, 0x020000, CRC(f5dde26a) SHA1(95db029bc4206a44ea216afbcd1c19689f79115a) )
        ROM_LOAD32_WORD( "ep17160a.014", 0x040000, 0x020000, CRC(267f3242) SHA1(40ec09cda984bb80969bfae2278432153137c213) )
        ROM_LOAD32_WORD( "ep17161a.015", 0x040002, 0x020000, CRC(f7126876) SHA1(b0ceb1206edaa507ec15723497fcd447a511f423) )

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
        ROM_LOAD32_WORD( "mpr17164.010", 0x000000, 0x200000, CRC(ac5fc501) SHA1(e60deec1e79d207d37d3f4ddd83a1b2125c411ac) )
        ROM_LOAD32_WORD( "mpr17165.011", 0x000002, 0x200000, CRC(82296d00) SHA1(23327137b36c98dfb9175ea9d36478e7385dfac2) )
        ROM_LOAD32_WORD( "mpr17162.008", 0x400000, 0x200000, CRC(60ddd41e) SHA1(0894c9bcdedeb09f921419a309858e242cb8db3a) )
        ROM_LOAD32_WORD( "mpr17163.009", 0x400002, 0x200000, CRC(8c1f9dc8) SHA1(cf99a5bb4f343d59c8d6f5716287b6e16bef6412) )
        ROM_LOAD32_WORD( "ep17168a.006", 0x800000, 0x080000, CRC(59091a37) SHA1(14591c7015aaf126755be584aa94c04e6de222fa) )
        ROM_LOAD32_WORD( "ep17169a.007", 0x800002, 0x080000, CRC(0495808d) SHA1(5b86a9a68c2b52f942aa8d858ee7a491f546a921) )

	ROM_REGION( 0x400000, REGION_CPU3, ROMREGION_ERASE00 ) // TGP program

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
        ROM_LOAD32_WORD( "mpr17159.016", 0x000000, 0x200000, CRC(e218727d) SHA1(1458d01d49936a0b8d497b62ff9ea940ca753b37) )
        ROM_LOAD32_WORD( "mpr17156.020", 0x000002, 0x200000, CRC(c4f4aabf) SHA1(8814cd329609cc8a188fedd770230bb9a5d00361) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
        ROM_LOAD32_WORD( "mpr17157.024", 0x000000, 0x200000, CRC(cf31e33d) SHA1(0cb62d4f28b5ad8a7e4c82b0ca8aea3037b05455) )
        ROM_LOAD32_WORD( "mpr17158.025", 0x000002, 0x200000, CRC(1108d1ec) SHA1(e95d4166bd4b26c5f21b85821b410f53045f4309) )

	ROM_REGION( 0x20000, REGION_CPU5, 0) // Communication program
        ROM_LOAD32_WORD( "epr17181.006", 0x000000, 0x010000, CRC(1add2b82) SHA1(81892251d466f630a96af25bde652c20e47d7ede) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) // Sound program
        ROM_LOAD16_WORD_SWAP( "epr17170.007", 0x000000, 0x020000, CRC(06a38ae2) SHA1(a2c3d14d9266449ebfc6d976a956e0a8a602cfb0) )
        ROM_LOAD16_WORD_SWAP( "epr17171.008", 0x020000, 0x020000, CRC(b5e436f8) SHA1(1da3cb52d64f52d03a8de9954afffbc6e1549a5b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
        ROM_LOAD( "mpr17172.032", 0x000000, 0x100000, CRC(ab22cac3) SHA1(0e872158faeb8c0404b10cdf0a3fa36f89a5093e) )
        ROM_LOAD( "mpr17173.033", 0x200000, 0x100000, CRC(3cb4005c) SHA1(a56f436ea6dfe0968b73ae7bc92bb2f4c612460d) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) // Samples
        ROM_LOAD( "mpr17174.004", 0x000000, 0x200000, CRC(a50369cc) SHA1(69807157baf6e3679adc95633c82b0236db01247) )
        ROM_LOAD( "mpr17175.005", 0x200000, 0x200000, CRC(9136d43c) SHA1(741f80a8ff8165ffe171dc568e0da4ad0bde4809) )

	MODEL2_CPU_BOARD
ROM_END

ROM_START( desert )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) // i960 program
	ROM_LOAD32_WORD("epr16976.12",  0x000000, 0x020000, CRC(d036dff0) SHA1(f3e5f22ef1f3ff9c9a1ff7352cdad3e2c2977a51))
	ROM_LOAD32_WORD("epr16977.13",  0x000002, 0x020000, CRC(e91194bd) SHA1(cec8eb8d4b52c387d5750ee5a0c6e6ce7c0fe80d))
	ROM_LOAD32_WORD("epr16970.14",  0x040000, 0x020000, CRC(4ea12d1f) SHA1(75133b03a450518bae27d62f0a1c37451c8c49a0))
	ROM_LOAD32_WORD("epr16971.15",  0x040002, 0x020000, CRC(d630b220) SHA1(ca7bd1e01e396b8b6a0925e767cc714729e0fd42))

	ROM_REGION32_LE( 0x2000000, REGION_USER1, 0 ) // Data
	ROM_LOAD32_WORD("mpr16974.10",  0x000000, 0x200000, CRC(2ab491c5) SHA1(79deb3877d0ffc8ee75c01d3bf0a6dd71cc2b552))
	ROM_LOAD32_WORD("mpr16975.11",  0x000002, 0x200000, CRC(e24fe7d3) SHA1(f8ab28c95d421978b1517adeacf09e7ee203d8f6))
	ROM_LOAD32_WORD("mpr16972.8",   0x400000, 0x200000, CRC(23e53748) SHA1(9c8a1d8aec8f9e5504e5aac0390dfb3770ab8616))
	ROM_LOAD32_WORD("mpr16973.9",   0x400002, 0x200000, CRC(77d6f509) SHA1(c83bce7f7b0a15bd14b99e829640b7dd9948e671))
	ROM_LOAD32_WORD("epr16978.6",   0x800000, 0x080000, CRC(38b3e574) SHA1(a1133df608b0fbb9c53bbeb29138650c87845d2c))
	ROM_LOAD32_WORD("epr16979.7",   0x800002, 0x080000, CRC(c314eb8b) SHA1(0c851dedd5c42b026195faed7d028924698a8b27))

	ROM_REGION( 0x400000, REGION_CPU3, 0 ) // TGP program? (COPRO socket)
	ROM_LOAD32_WORD("epr16981.28",   0x000000, 0x080000, CRC(ae847571) SHA1(32d0f9e685667ae9fddacea0b9f4ad6fb3a6fdad) )
	ROM_LOAD32_WORD("epr16980.29",   0x000002, 0x080000, CRC(5239b864) SHA1(e889556e0f1ea80de52afff563b0923f87cef7ab) )

	ROM_REGION( 0x1000000, REGION_USER2, 0 ) // Models
	ROM_LOAD32_WORD("mpr16968.16",   0x000000, 0x200000, CRC(4a16f465) SHA1(411214ed65ce966040d4299b50bfaa40f7f5f266) )
	ROM_LOAD32_WORD("mpr16965.20",   0x000002, 0x200000, CRC(9ba7645f) SHA1(c04f369961f908bac16fad8e32b863202390c205) )
	ROM_LOAD32_WORD("mpr16969.17",   0x400000, 0x200000, CRC(887380ac) SHA1(03a9f601764d06cb0b2daaadf4f8433f327abd4a) )
	ROM_LOAD32_WORD("mpr16964.21",   0x400002, 0x200000, CRC(d4a769b6) SHA1(845c34f95a49e06e3996b0c67aa73b4886fa8996) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) // Textures
	ROM_LOAD32_WORD("mpr16966.24",   0x000000, 0x200000, CRC(7484efe9) SHA1(33e72139ad6c2990428e3fa041dbcdf39aca1c7a) )
	ROM_LOAD32_WORD("mpr16967.25",   0x000002, 0x200000, CRC(b8b84c9d) SHA1(00ef320988609e98c8af383b68d845e3be8d0a03) )

	ROM_REGION( 0x20000, REGION_CPU5, ROMREGION_ERASE00 ) // Communication program

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) // Sound program
	ROM_LOAD16_WORD_SWAP("epr16985.7",   0x000000,  0x20000, CRC(8c4d9056) SHA1(785752d761c648d1177c5f0cfa3e9fa44135d6dc))

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD("mpr16986.32",  0x000000, 0x200000, CRC(559612f9) SHA1(33bcaddfc7d8fe899707e663299e8f04e9004d51))

	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) // Samples
	ROM_LOAD("mpr16988.4",   0x000000, 0x200000, CRC(bc705875) SHA1(5351c6bd2d75df57ff92960e7f90493d95d9dfb9))
	ROM_LOAD("mpr16989.5",   0x200000, 0x200000, CRC(1b616b31) SHA1(35bd2bfd08514ba6f235cda2605c171cd51fd78e))

	MODEL2_CPU_BOARD
ROM_END

static DRIVER_INIT( genprot )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_w);
	protstate = protpos = 0;
}

static DRIVER_INIT( pltkids )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_w);
	protstate = protpos = 0;

	// fix bug in program: it destroys the interrupt table and never fixes it
	ROM[0x730/4] = 0x08000004;
}

static DRIVER_INIT( zerogun )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_w);
	protstate = protpos = 0;

	// fix bug in program: it destroys the interrupt table and never fixes it
	ROM[0x700/4] = 0x08000004;
}

static DRIVER_INIT( daytona )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	// kill TGP busywait
	ROM[0x11c7c/4] = 0x08000004;
}

static DRIVER_INIT( daytonam )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x240000, 0x24ffff, 0, 0, maxx_r );
}

static DRIVER_INIT( sgt24h )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_w);
	protstate = protpos = 0;

	ROM[0x56578/4] = 0x08000004;
	ROM[0x5b3e8/4] = 0x08000004;
}

static DRIVER_INIT( doa )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_CPU1);

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x01d80000, 0x01dfffff, 0, 0, model2_prot_w);
	protstate = protpos = 0;

	ROM[0x630/4] = 0x08000004;
	ROM[0x808/4] = 0x08000004;
}

// Model 2 (TGPs, Model 1 sound board)
GAME( 1993, daytona,         0, model2o, daytona, daytona, ROT0, "Sega", "Daytona USA (Japan)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1993, daytonat,  daytona, model2o, daytona, 0, ROT0, "Sega", "Daytona USA (Japan, Turbo hack)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1993, daytonam,  daytona, model2o, daytona, daytonam, ROT0, "Sega", "Daytona USA (Japan, To The MAXX)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1994, desert,          0, model2o, desert, 0, ROT0, "Sega/Martin Marietta", "Desert Tank", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1994, vcop,            0, model2o, daytona, 0, ROT0, "Sega", "Virtua Cop", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )

// Model 2A-CRX (TGPs, SCSP sound board)
GAME( 1995, manxtt,          0, model2a, model2, 0, ROT0, "Sega", "Manx TT Superbike", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, srallyc,         0, model2a, model2, 0, ROT0, "Sega", "Sega Rally Championship", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, vf2,             0, model2a, model2, 0, ROT0, "Sega", "Virtua Fighter 2 (ver 2.1)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, vf2b,          vf2, model2a, model2, 0, ROT0, "Sega", "Virtua Fighter 2 (ver B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, vf2o,          vf2, model2a, model2, 0, ROT0, "Sega", "Virtua Fighter 2 (original)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, vcop2,           0, model2a, model2, 0, ROT0, "Sega", "Virtua Cop 2", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, dynamcop,        0, model2a, model2, genprot, ROT0, "Sega", "Dynamite Cop (Model 2A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, dyndeka2, dynamcop, model2a, model2, genprot, ROT0, "Sega", "Dynamite Deka 2 (Japan, Model 2A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, doaa,          doa, model2a, model2, doa, ROT0, "Sega", "Dead or Alive (Model 2A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, zeroguna,  zerogun, model2a, model2, zerogun, ROT0, "Psikyo", "Zero Gunner (Model 2A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1998, pltkidsa,  pltkids, model2a, model2, pltkids, ROT0, "Psikyo", "Pilot Kids (Model 2A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )

// Model 2B-CRX (SHARC, SCSP sound board)
GAME( 1994, vstriker,        0, model2b, model2, 0, ROT0, "Sega", "Virtua Striker", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1994, vstrikra, vstriker, model2b, model2, 0, ROT0, "Sega", "Virtua Striker (Rev A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, fvipers,         0, model2b, model2, 0, ROT0, "Sega", "Fighting Vipers", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, gunblade,        0, model2b, model2, 0, ROT0, "Sega", "Gunblade NY", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, indy500,         0, model2b, model2, 0, ROT0, "Sega", "Indianapolis 500", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1995, skytargt,        0, model2b, model2, 0, ROT0, "Sega", "Sky Target", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, dynmcopb, dynamcop, model2b, model2, genprot, ROT0, "Sega", "Dynamite Cop (Model 2B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, schamp,          0, model2b, model2, 0, ROT0, "Sega", "Sonic The Fighters", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, lastbrnx,        0, model2b, model2, 0, ROT0, "Sega", "Last Bronx (Export, Rev A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, lastbrnj, lastbrnx, model2b, model2, 0, ROT0, "Sega", "Last Bronx (Japan, Rev A)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, doa,             0, model2b, model2, doa, ROT0, "Sega", "Dead or Alive (Model 2B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, sgt24h,          0, model2b, model2, sgt24h, ROT0, "Jaleco", "Super GT 24h", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, von,             0, model2b, model2, 0, ROT0, "Sega", "Virtual On Cyber Troopers (Japan)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, vonusa,        von, model2b, model2, 0, ROT0, "Sega", "Virtual On Cyber Troopers (US)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, dynabb,          0, model2b, model2, 0, ROT0, "Sega", "Dynamite Baseball '97", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, zerogun,         0, model2b, model2, zerogun, ROT0, "Psikyo", "Zero Gunner (Model 2B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, zerogunj,  zerogun, model2b, model2, zerogun, ROT0, "Psikyo", "Zero Gunner (Japan Model 2B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1998, pltkids,         0, model2b, model2, pltkids, ROT0, "Psikyo", "Pilot Kids (Model 2B)", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )

// Model 2C-CRX (TGPx4, SCSP sound board)
GAME( 1996, skisuprg,        0, model2c, model2, 0, ROT0, "Sega", "Sega Ski Super G", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1996, stcc,            0, model2c, model2, 0, ROT0, "Sega", "Sega Touring Car Championship", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, hotd,            0, model2c, model2, 0, ROT0, "Sega", "House of the Dead", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, overrev,         0, model2c, model2, 0, ROT0, "Jaleco", "Over Rev", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1997, topskatr,        0, model2c, model2, 0, ROT0, "Sega", "Top Skater", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )
GAME( 1998, bel,             0, model2c, bel,    0, ROT0, "Sega/EPL Productions", "Behind Enemy Lines", GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS )

