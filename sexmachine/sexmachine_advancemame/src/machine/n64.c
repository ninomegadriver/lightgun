/* machine/n64.c - contains N64 hardware emulation shared between MAME and MESS */

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "sound/custom.h"
#include "streams.h"
#include "includes/n64.h"

UINT32 *rdram;
UINT32 *rsp_imem;
UINT32 *rsp_dmem;

// MIPS Interface
static UINT32 mi_version;
static UINT32 mi_interrupt = 0;
static UINT32 mi_intr_mask = 0;

void signal_rcp_interrupt(int interrupt)
{
	if (mi_intr_mask & interrupt)
	{
		mi_interrupt |= interrupt;

		cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
	}
}

void clear_rcp_interrupt(int interrupt)
{
	mi_interrupt &= ~interrupt;

	//if (!mi_interrupt)
	{
		cpunum_set_input_line(0, INPUT_LINE_IRQ0, CLEAR_LINE);
	}
}

READ32_HANDLER( n64_mi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:			// MI_VERSION_REG
			return mi_version;

		case 0x08/4:			// MI_INTR_REG
			return mi_interrupt;

		case 0x0c/4:			// MI_INTR_MASK_REG
			return mi_intr_mask;

		default:
			logerror("mi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_mi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// MI_INIT_MODE_REG
			if (data & 0x0800)
			{
				clear_rcp_interrupt(DP_INTERRUPT);
			}
			break;

		case 0x04/4:		// MI_VERSION_REG
			mi_version = data;
			break;

		case 0x0c/4:		// MI_INTR_MASK_REG
		{
			if (data & 0x0001) mi_intr_mask &= ~0x1;		// clear SP mask
			if (data & 0x0002) mi_intr_mask |= 0x1;			// set SP mask
			if (data & 0x0004) mi_intr_mask &= ~0x2;		// clear SI mask
			if (data & 0x0008) mi_intr_mask |= 0x2;			// set SI mask
			if (data & 0x0010) mi_intr_mask &= ~0x4;		// clear AI mask
			if (data & 0x0020) mi_intr_mask |= 0x4;			// set AI mask
			if (data & 0x0040) mi_intr_mask &= ~0x8;		// clear VI mask
			if (data & 0x0080) mi_intr_mask |= 0x8;			// set VI mask
			if (data & 0x0100) mi_intr_mask &= ~0x10;		// clear PI mask
			if (data & 0x0200) mi_intr_mask |= 0x10;		// set PI mask
			if (data & 0x0400) mi_intr_mask &= ~0x20;		// clear DP mask
			if (data & 0x0800) mi_intr_mask |= 0x20;		// set DP mask
			break;
		}

		default:
			logerror("mi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// RSP Interface

static UINT32 rsp_sp_status = 0;
//static UINT32 cpu_sp_status = SP_STATUS_HALT;
static UINT32 sp_mem_addr;
static UINT32 sp_dram_addr;
static int sp_dma_length;
static int sp_dma_count;
static int sp_dma_skip;

static UINT32 sp_semaphore;

static void sp_dma(int direction)
{
	UINT8 *src, *dst;
	int i;

	if (sp_dma_length == 0)
	{
		return;
	}

	if (sp_mem_addr & 0x3)
	{
		fatalerror("sp_dma: sp_mem_addr unaligned: %08X\n", sp_mem_addr);
	}
	if (sp_dram_addr & 0x3)
	{
		fatalerror("sp_dma: sp_dram_addr unaligned: %08X\n", sp_dram_addr);
	}

	if (sp_dma_count > 0)
	{
		fatalerror("sp_dma: dma_count = %d\n", sp_dma_count);
	}
	if (sp_dma_skip > 0)
	{
		fatalerror("sp_dma: dma_skip = %d\n", sp_dma_skip);
	}

	if ((sp_mem_addr & 0xfff) + (sp_dma_length+1) > 0x1000)
	{
		fatalerror("sp_dma: dma out of memory area: %08X, %08X\n", sp_mem_addr, sp_dma_length+1);
	}

	if (direction == 0)		// RDRAM -> I/DMEM
	{
		src = (UINT8*)&rdram[sp_dram_addr / 4];
		dst = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];

		for (i=0; i <= sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

		/*dst = (sp_mem_addr & 0x1000) ? (UINT8*)rsp_imem : (UINT8*)rsp_dmem;
        for (i=0; i <= sp_dma_length; i++)
        {
            dst[BYTE4_XOR_BE(sp_mem_addr+i) & 0xfff] = src[BYTE4_XOR_BE(i)];
        }*/
	}
	else					// I/DMEM -> RDRAM
	{
		src = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];
		dst = (UINT8*)&rdram[sp_dram_addr / 4];

		for (i=0; i <= sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

		/*src = (sp_mem_addr & 0x1000) ? (UINT8*)rsp_imem : (UINT8*)rsp_dmem;
        for (i=0; i <= sp_dma_length; i++)
        {
            dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(sp_mem_addr+i) & 0xfff];
        }*/
	}
}




void sp_set_status(UINT32 status)
{
	if (status & 0x1)
	{
		cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
		rsp_sp_status |= SP_STATUS_HALT;
	}
	if (status & 0x2)
	{
		rsp_sp_status |= SP_STATUS_BROKE;

		if (rsp_sp_status & SP_STATUS_INT_ON_BRK)
		{
			signal_rcp_interrupt(SP_INTERRUPT);
		}
	}
}

READ32_HANDLER( n64_sp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// SP_MEM_ADDR_REG
			return sp_mem_addr;

		case 0x04/4:		// SP_DRAM_ADDR_REG
			return sp_dram_addr;

		case 0x08/4:		// SP_RD_LEN_REG
			return (sp_dma_skip << 20) | (sp_dma_count << 12) | sp_dma_length;

		case 0x10/4:		// SP_STATUS_REG
			return rsp_sp_status;

		case 0x14/4:		// SP_DMA_FULL_REG
			return 0;

		case 0x18/4:		// SP_DMA_BUSY_REG
			return 0;

		case 0x1c/4:		// SP_SEMAPHORE_REG
			return sp_semaphore;

		default:
			logerror("sp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_sp_reg_w )
{
	if ((offset & 0x10000) == 0)
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:		// SP_MEM_ADDR_REG
				sp_mem_addr = data;
				break;

			case 0x04/4:		// SP_DRAM_ADDR_REG
				sp_dram_addr = data & 0xffffff;
				break;

			case 0x08/4:		// SP_RD_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(0);
				break;

			case 0x0c/4:		// SP_WR_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(1);
				break;

			case 0x10/4:		// SP_STATUS_REG
			{
				if (data & 0x00000001)		// clear halt
				{
					cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
					rsp_sp_status &= ~SP_STATUS_HALT;
				}
				if (data & 0x00000002)		// set halt
				{
					cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
					rsp_sp_status |= SP_STATUS_HALT;
				}
				if (data & 0x00000004) rsp_sp_status &= ~SP_STATUS_BROKE;		// clear broke
				if (data & 0x00000008)		// clear interrupt
				{
					clear_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000010)		// set interrupt
				{
					signal_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000020) rsp_sp_status &= ~SP_STATUS_SSTEP;		// clear single step
				if (data & 0x00000040) rsp_sp_status |= SP_STATUS_SSTEP;		// set single step
				if (data & 0x00000080) rsp_sp_status &= ~SP_STATUS_INT_ON_BRK;	// clear interrupt on break
				if (data & 0x00000100) rsp_sp_status |= SP_STATUS_INT_ON_BRK;	// set interrupt on break
				if (data & 0x00000200) rsp_sp_status &= ~SP_STATUS_SIGNAL0;		// clear signal 0
				if (data & 0x00000400) rsp_sp_status |= SP_STATUS_SIGNAL0;		// set signal 0
				if (data & 0x00000800) rsp_sp_status &= ~SP_STATUS_SIGNAL1;		// clear signal 1
				if (data & 0x00001000) rsp_sp_status |= SP_STATUS_SIGNAL1;		// set signal 1
				if (data & 0x00002000) rsp_sp_status &= ~SP_STATUS_SIGNAL2;		// clear signal 2
				if (data & 0x00004000) rsp_sp_status |= SP_STATUS_SIGNAL2;		// set signal 2
				if (data & 0x00008000) rsp_sp_status &= ~SP_STATUS_SIGNAL3;		// clear signal 3
				if (data & 0x00010000) rsp_sp_status |= SP_STATUS_SIGNAL3;		// set signal 3
				if (data & 0x00020000) rsp_sp_status &= ~SP_STATUS_SIGNAL4;		// clear signal 4
				if (data & 0x00040000) rsp_sp_status |= SP_STATUS_SIGNAL4;		// set signal 4
				if (data & 0x00080000) rsp_sp_status &= ~SP_STATUS_SIGNAL5;		// clear signal 5
				if (data & 0x00100000) rsp_sp_status |= SP_STATUS_SIGNAL5;		// set signal 5
				if (data & 0x00200000) rsp_sp_status &= ~SP_STATUS_SIGNAL6;		// clear signal 6
				if (data & 0x00400000) rsp_sp_status |= SP_STATUS_SIGNAL6;		// set signal 6
				if (data & 0x00800000) rsp_sp_status &= ~SP_STATUS_SIGNAL7;		// clear signal 7
				if (data & 0x01000000) rsp_sp_status |= SP_STATUS_SIGNAL7;		// set signal 7
				break;
			}

			case 0x1c/4:		// SP_SEMAPHORE_REG
				sp_semaphore = data;
		//      printf("sp_semaphore = %08X\n", sp_semaphore);
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
		}
	}
	else
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:		// SP_PC_REG
				cpunum_set_info_int(1, CPUINFO_INT_PC, 0x04000000 | (data & 0x1fff));
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
		}
	}
}

UINT32 sp_read_reg(UINT32 reg)
{
	switch (reg)
	{
		//case 4:       return rsp_sp_status;
		default:	return n64_sp_reg_r(reg, 0x00000000);
	}
}

void sp_write_reg(UINT32 reg, UINT32 data)
{
	switch (reg)
	{
		default:	n64_sp_reg_w(reg, data, 0x00000000); break;
	}
}

// RDP Interface
UINT32 dp_start;
UINT32 dp_end;
UINT32 dp_current;
UINT32 dp_status = 0;


void dp_full_sync(void)
{
	signal_rcp_interrupt(DP_INTERRUPT);
}

READ32_HANDLER( n64_dp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// DP_START_REG
			return dp_start;

		case 0x04/4:		// DP_END_REG
			return dp_end;

		case 0x08/4:		// DP_CURRENT_REG
			return dp_current;

		case 0x0c/4:		// DP_STATUS_REG
			return dp_status;

		default:
			logerror("dp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_dp_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// DP_START_REG
			dp_start = data;
			dp_current = dp_start;
			break;

		case 0x04/4:		// DP_END_REG
			dp_end = data;
			rdp_process_list();
			break;

		case 0x0c/4:		// DP_STATUS_REG
			if (data & 0x00000001)	dp_status &= ~DP_STATUS_XBUS_DMA;
			if (data & 0x00000002)	dp_status |= DP_STATUS_XBUS_DMA;
			if (data & 0x00000004)	dp_status &= ~DP_STATUS_FREEZE;
			if (data & 0x00000008)	dp_status |= DP_STATUS_FREEZE;
			if (data & 0x00000010)	dp_status &= ~DP_STATUS_FLUSH;
			if (data & 0x00000020)	dp_status |= DP_STATUS_FLUSH;
			break;

		default:
			logerror("dp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}



// Video Interface
UINT32 vi_width;
UINT32 vi_origin;
UINT32 vi_control;
UINT32 vi_burst, vi_vsync, vi_hsync, vi_leap, vi_hstart, vi_vstart;
UINT32 vi_intr, vi_vburst, vi_xscale, vi_yscale;

READ32_HANDLER( n64_vi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:		// VI_ORIGIN_REG
			return vi_origin;

		case 0x08/4:		// VI_WIDTH_REG
			return vi_width;

		case 0x0c/4:
			return vi_intr;

		case 0x10/4:		// VI_CURRENT_REG
		{
			return cpu_getscanline();
		}

		case 0x14/4:		// VI_BURST_REG
			return vi_burst;

		case 0x18/4:		// VI_V_SYNC_REG
			return vi_vsync;

		case 0x1c/4:		// VI_H_SYNC_REG
			return vi_hsync;

		case 0x20/4:		// VI_LEAP_REG
			return vi_leap;

		case 0x24/4:		// VI_H_START_REG
			return vi_hstart;

		case 0x28/4:		// VI_V_START_REG
			return vi_vstart;

		case 0x2c/4:		// VI_V_BURST_REG
			return vi_vburst;

		case 0x30/4:		// VI_X_SCALE_REG
			return vi_xscale;

		case 0x34/4:		// VI_Y_SCALE_REG
			return vi_yscale;

		default:
			logerror("vi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}
	return 0;
}

WRITE32_HANDLER( n64_vi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// VI_CONTROL_REG
			if ((vi_control & 0x40) != (data & 0x40))
			{
				set_visible_area(0, vi_width-1, 0, (data & 0x40) ? 479 : 239);
			}
			vi_control = data;
			break;

		case 0x04/4:		// VI_ORIGIN_REG
			vi_origin = data & 0xffffff;
			break;

		case 0x08/4:		// VI_WIDTH_REG
			if (vi_width != data && data > 0)
			{
				set_visible_area(0, data-1, 0, (vi_control & 0x40) ? 479 : 239);
			}
			vi_width = data;
			break;

		case 0x0c/4:		// VI_INTR_REG
			vi_intr = data;
			break;

		case 0x10/4:		// VI_CURRENT_REG
			clear_rcp_interrupt(VI_INTERRUPT);
			break;

		case 0x14/4:		// VI_BURST_REG
			vi_burst = data;
			break;

		case 0x18/4:		// VI_V_SYNC_REG
			vi_vsync = data;
			break;

		case 0x1c/4:		// VI_H_SYNC_REG
			vi_hsync = data;
			break;

		case 0x20/4:		// VI_LEAP_REG
			vi_leap = data;
			break;

		case 0x24/4:		// VI_H_START_REG
			vi_hstart = data;
			break;

		case 0x28/4:		// VI_V_START_REG
			vi_vstart = data;
			break;

		case 0x2c/4:		// VI_V_BURST_REG
			vi_vburst = data;
			break;

		case 0x30/4:		// VI_X_SCALE_REG
			vi_xscale = data;
			break;

		case 0x34/4:		// VI_Y_SCALE_REG
			vi_yscale = data;
			break;

		default:
			logerror("vi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// Audio Interface
static UINT32 ai_dram_addr;
static UINT32 ai_len;
static UINT32 ai_control = 0;
static int ai_dacrate;
static int ai_bitrate;
static UINT32 ai_status = 0;

static int audio_dma_active = 0;
static void *audio_timer;

#define SOUNDBUFFER_LENGTH		0x80000
#define AUDIO_DMA_DEPTH		2

typedef struct
{
	UINT32 address;
	UINT32 length;
} AUDIO_DMA;

static AUDIO_DMA audio_fifo[AUDIO_DMA_DEPTH];
static int audio_fifo_wpos = 0;
static int audio_fifo_rpos = 0;
static int audio_fifo_num = 0;

static INT16 *soundbuffer[2];
static INT64 sb_dma_ptr = 0;
static INT64 sb_play_ptr = 0;

static void audio_fifo_push(UINT32 address, UINT32 length)
{
	if (audio_fifo_num == AUDIO_DMA_DEPTH)
	{
		printf("audio_fifo_push: tried to push to full DMA FIFO!!!\n");
	}

	audio_fifo[audio_fifo_wpos].address = address;
	audio_fifo[audio_fifo_wpos].length = length;

	audio_fifo_wpos++;
	audio_fifo_num++;

	if (audio_fifo_wpos >= AUDIO_DMA_DEPTH)
	{
		audio_fifo_wpos = 0;
	}

	if (audio_fifo_num >= AUDIO_DMA_DEPTH)
	{
		ai_status |= 0x80000001;	// FIFO full
		signal_rcp_interrupt(AI_INTERRUPT);
	}
}

static void audio_fifo_pop(void)
{
	audio_fifo_rpos++;
	audio_fifo_num--;

	if (audio_fifo_num < 0)
	{
		fatalerror("audio_fifo_pop: FIFO underflow!\n");
	}

	if (audio_fifo_rpos >= AUDIO_DMA_DEPTH)
	{
		audio_fifo_rpos = 0;
	}

	if (audio_fifo_num < AUDIO_DMA_DEPTH)
	{
		ai_status &= ~0x80000001;	// FIFO not full
	}

	ai_len = 0;
}

static AUDIO_DMA *audio_fifo_get_top(void)
{
	if (audio_fifo_num > 0)
	{
		return &audio_fifo[audio_fifo_rpos];
	}
	else
	{
		return NULL;
	}
}

static void start_audio_dma(void)
{
	UINT16 *ram = (UINT16*)rdram;
	int i;
	double time, frequency;
	AUDIO_DMA *current = audio_fifo_get_top();

	frequency = (double)48681812 / (double)(ai_dacrate+1);
	time = (double)(current->length / 4) / frequency;

	timer_adjust(audio_timer, TIME_IN_SEC(time), 0, 0);

	//sb_play_ptr += current->length;

	for (i=0; i < current->length; i+=4)
	{
		//INT16 l = program_read_word_32be(current->address + i + 0);
		//INT16 r = program_read_word_32be(current->address + i + 2);
		INT16 l = ram[(current->address + i + 0) / 2];
		INT16 r = ram[(current->address + i + 2) / 2];

		soundbuffer[0][sb_dma_ptr] = l;
		soundbuffer[1][sb_dma_ptr] = r;

		sb_dma_ptr++;
		if (sb_dma_ptr >= SOUNDBUFFER_LENGTH)
		{
			sb_dma_ptr = 0;
		}
	}
}

static void audio_timer_callback(int param)
{
	audio_fifo_pop();

	// keep playing if there's another DMA queued
	if (audio_fifo_get_top() != NULL)
	{
		start_audio_dma();
	}
	else
	{
		audio_dma_active = 0;
	}
}

READ32_HANDLER( n64_ai_reg_r )
{
	switch (offset)
	{
		case 0x04/4:		// AI_LEN_REG
		{
			return ai_len;
		}

		case 0x0c/4:		// AI_STATUS_REG
			return ai_status;

		default:
			logerror("ai_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_ai_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// AI_DRAM_ADDR_REG
//          printf("ai_dram_addr = %08X at %08X\n", data, activecpu_get_pc());
			ai_dram_addr = data & 0xffffff;
			break;

		case 0x04/4:		// AI_LEN_REG
//          printf("ai_len = %08X at %08X\n", data, activecpu_get_pc());
			ai_len = data & 0x3ffff;		// Hardware v2.0 has 18 bits, v1.0 has 15 bits
			//audio_dma();
			audio_fifo_push(ai_dram_addr, ai_len);

			// if no DMA transfer is active, start right away
			if (!audio_dma_active)
			{
				start_audio_dma();
			}
			break;

		case 0x08/4:		// AI_CONTROL_REG
//          printf("ai_control = %08X at %08X\n", data, activecpu_get_pc());
			ai_control = data;
			break;

		case 0x0c/4:
			clear_rcp_interrupt(AI_INTERRUPT);
			break;

		case 0x10/4:		// AI_DACRATE_REG
//          printf("ai_dacrate = %08X\n", data);
			ai_dacrate = data & 0x3fff;
			break;

		case 0x14/4:		// AI_BITRATE_REG
//          printf("ai_bitrate = %08X\n", data);
			ai_bitrate = data & 0xf;
			break;

		default:
			logerror("ai_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}


// Peripheral Interface

static UINT32 pi_dram_addr, pi_cart_addr;
static UINT32 pi_first_dma = 1;

READ32_HANDLER( n64_pi_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// PI_DRAM_ADDR_REG
			return pi_dram_addr;

		case 0x04/4:		// PI_CART_ADDR_REG
			return pi_cart_addr;

		case 0x10/4:		// PI_STATUS_REG
			return 0;

		default:
			logerror("pi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}
	return 0;
}

WRITE32_HANDLER( n64_pi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// PI_DRAM_ADDR_REG
		{
			pi_dram_addr = data;
			break;
		}

		case 0x04/4:		// PI_CART_ADDR_REG
		{
			pi_cart_addr = data;
			break;
		}

		case 0x08/4:		// PI_RD_LEN_REG
		{
			fatalerror("PI_RD_LEN_REG: %08X, %08X to %08X\n", data, pi_dram_addr, pi_cart_addr);
			break;
		}

		case 0x0c/4:		// PI_WR_LEN_REG
		{
			int i;
			UINT32 dma_length = (data + 1);

			//printf("PI DMA: %08X to %08X, length %08X\n", pi_cart_addr, pi_dram_addr, dma_length);

			if (pi_dram_addr != 0xffffffff)
			{
				for (i=0; i < dma_length; i++)
				{
					/*UINT32 d = program_read_dword_32be(pi_cart_addr);
                    program_write_dword_32be(pi_dram_addr, d);
                    pi_cart_addr += 4;
                    pi_dram_addr += 4;*/

					UINT8 b = program_read_byte_32be(pi_cart_addr);
					program_write_byte_32be(pi_dram_addr, b);
					pi_cart_addr += 1;
					pi_dram_addr += 1;
				}
			}
			signal_rcp_interrupt(PI_INTERRUPT);

			if (pi_first_dma)
			{
				// TODO: CIC-6105 has different address...
				program_write_dword_32be(0x00000318, 0x400000);
				pi_first_dma = 0;
			}

			break;
		}

		case 0x10/4:		// PI_STATUS_REG
		{
			if (data & 0x2)
			{
				clear_rcp_interrupt(PI_INTERRUPT);
			}
			break;
		}

		default:
			logerror("pi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

// RDRAM Interface
READ32_HANDLER( n64_ri_reg_r )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_ri_reg_w )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

// Serial Interface
UINT8 pif_ram[0x40];
UINT8 pif_cmd[0x40];
UINT32 si_dram_addr = 0;
UINT32 si_pif_addr = 0;
UINT32 si_status = 0;

static int pif_channel_handle_command(int channel, int slength, UINT8 *sdata, int rlength, UINT8 *rdata)
{
	int i;
	UINT8 command = sdata[0];

	switch (command)
	{
		case 0x00:		// Read status
		{
			if (slength != 1 || rlength != 3)
			{
				// osd_die("handle_pif: read status (bytes to send %d, bytes to receive %d)\n", bytes_to_send, bytes_to_recv);
			}

			switch (channel)
			{
				case 0:
				{
					rdata[0] = 0x05;
					rdata[1] = 0x00;
					rdata[2] = 0x01;
					return 0;
				}
				case 1:
				case 2:
				case 3:
				{
					// not connected
					return 1;
				}
				case 4:
				{
					rdata[0] = 0x00;
					rdata[1] = 0x80;
					rdata[2] = 0x00;
					return 0;
				}
				case 5:
				{
					printf("EEPROM2? read status\n");
					return 1;
				}
			}

			break;
		}

		case 0x01:		// Read button values
		{
			UINT16 buttons = 0;
			INT8 x = 0, y = 0;
			if (slength != 1 || rlength != 4)
			{
				fatalerror("handle_pif: read button values (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			switch (channel)
			{
				case 0:
				{
					buttons = readinputport((channel*3) + 0);
					x = readinputport((channel*3) + 1) - 128;
					y = readinputport((channel*3) + 2) - 128;

					rdata[0] = (buttons >> 8) & 0xff;
					rdata[1] = (buttons >> 0) & 0xff;
					rdata[2] = (UINT8)(x);
					rdata[3] = (UINT8)(y);
					return 0;
				}
				case 1:
				case 2:
				case 3:
				{
					// not connected
					return 1;
				}
			}

			break;
		}

		case 0x04:		// Read from EEPROM
		{
			return 0;
		}

		case 0x05:		// Write to EEPROM
		{
			UINT8 block_offset;

			if (channel != 4)
			{
				fatalerror("Tried to write to EEPROM on channel %d\n", channel);
			}

			if (slength != 10 || rlength != 1)
			{
				fatalerror("handle_pif: write EEPROM (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			block_offset = sdata[1];
			printf("Write EEPROM: offset %02X: ", block_offset);
			for (i=0; i < 8; i++)
			{
				printf("%02X ", sdata[2+i]);
			}
			printf("\n");

			rdata[0] = 0;

			return 0;
		}

		case 0xff:		// reset
		{
			rdata[0] = 0xff;
			rdata[1] = 0xff;
			rdata[2] = 0xff;
			return 0;
		}

		default:
		{
			printf("handle_pif: unknown/unimplemented command %02X\n", command);
			return 1;
		}
	}
	return 0;
}

static void handle_pif(void)
{
	int j;

	/*{
        int i;
        for (i=0; i < 8; i++)
        {
            int j = i * 8;
            printf("PIFCMD%d: %02X %02X %02X %02X %02X %02X %02X %02X\n", i, pif_cmd[j], pif_cmd[j+1], pif_cmd[j+2], pif_cmd[j+3], pif_cmd[j+4], pif_cmd[j+5], pif_cmd[j+6], pif_cmd[j+7]);
        }
        printf("\n");
    }*/


	if (pif_cmd[0x3f] == 0x1)		// only handle the command if the last byte is 1
	{
		int channel = 0;
		int end = 0;
		int cmd_ptr = 0;

		while (cmd_ptr < 0x3f && !end)
		{
			UINT8 bytes_to_send;
			UINT8 bytes_to_recv;

			bytes_to_send = pif_cmd[cmd_ptr++];

			if (bytes_to_send == 0xfe)
			{
				end = 1;
			}
			else if (bytes_to_send == 0xff)
			{
				// do nothing
			}
			else
			{
				if (bytes_to_send > 0 && (bytes_to_send & 0xc0) == 0)
				{
					int res;
					UINT8 recv_buffer[0x40];
					UINT8 send_buffer[0x40];

					bytes_to_recv = pif_cmd[cmd_ptr++];

					for (j=0; j < bytes_to_send; j++)
					{
						send_buffer[j] = pif_cmd[cmd_ptr++];
					}

					res = pif_channel_handle_command(channel, bytes_to_send, send_buffer, bytes_to_recv, recv_buffer);

					if (res == 0)
					{
						for (j=0; j < bytes_to_recv; j++)
						{
							pif_ram[cmd_ptr++] = recv_buffer[j];
						}
					}
					else if (res == 1)
					{
						pif_ram[cmd_ptr-2] |= 0x80;
					}
				}

				channel++;
			}
		}

		pif_ram[0x3f] = 0;
	}

	/*
    {
        int i;
        for (i=0; i < 8; i++)
        {
            int j = i * 8;
            printf("PIFRAM%d: %02X %02X %02X %02X %02X %02X %02X %02X\n", i, pif_ram[j], pif_ram[j+1], pif_ram[j+2], pif_ram[j+3], pif_ram[j+4], pif_ram[j+5], pif_ram[j+6], pif_ram[j+7]);
        }
        printf("\n");
    }
    */
}

static void pif_dma(int direction)
{
	int i;
	UINT32 *src, *dst;

	if (si_dram_addr & 0x3)
	{
		fatalerror("pif_dma: si_dram_addr unaligned: %08X\n", si_dram_addr);
	}

	if (direction)		// RDRAM -> PIF RAM
	{
		src = &rdram[(si_dram_addr & 0x1fffffff) / 4];

		for (i=0; i < 64; i+=4)
		{
			UINT32 d = *src++;
			pif_ram[i+0] = (d >> 24) & 0xff;
			pif_ram[i+1] = (d >> 16) & 0xff;
			pif_ram[i+2] = (d >>  8) & 0xff;
			pif_ram[i+3] = (d >>  0) & 0xff;
		}

		memcpy(pif_cmd, pif_ram, 0x40);
	}
	else				// PIF RAM -> RDRAM
	{
		handle_pif();

		dst = &rdram[(si_dram_addr & 0x1fffffff) / 4];

		for (i=0; i < 64; i+=4)
		{
			UINT32 d = 0;
			d |= pif_ram[i+0] << 24;
			d |= pif_ram[i+1] << 16;
			d |= pif_ram[i+2] <<  8;
			d |= pif_ram[i+3] <<  0;

			*dst++ = d;
		}
	}

	si_status |= 0x1000;
	signal_rcp_interrupt(SI_INTERRUPT);
}

READ32_HANDLER( n64_si_reg_r )
{
	switch (offset)
	{
		case 0x00/4:		// SI_DRAM_ADDR_REG
			return si_dram_addr;

		case 0x18/4:		// SI_STATUS_REG
			return si_status;
	}
	return 0;
}

WRITE32_HANDLER( n64_si_reg_w )
{
	switch (offset)
	{
		case 0x00/4:		// SI_DRAM_ADDR_REG
			si_dram_addr = data;
	//      printf("si_dram_addr = %08X\n", si_dram_addr);
			break;

		case 0x04/4:		// SI_PIF_ADDR_RD64B_REG
			// PIF RAM -> RDRAM
			si_pif_addr = data;
			pif_dma(0);
			break;

		case 0x10/4:		// SI_PIF_ADDR_WR64B_REG
			// RDRAM -> PIF RAM
			si_pif_addr = data;
			pif_dma(1);
			break;

		case 0x18/4:		// SI_STATUS_REG
			si_status &= ~0x1000;
			clear_rcp_interrupt(SI_INTERRUPT);
			break;

		default:
			logerror("si_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}
}

READ32_HANDLER( n64_pif_ram_r )
{
	return pif_ram[offset];
}

WRITE32_HANDLER( n64_pif_ram_w )
{
//  printf("pif_ram_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	pif_ram[offset] = data;

	signal_rcp_interrupt(SI_INTERRUPT);
}

static int audio_playing = 0;

static void n64_sh_update( void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length )
{
	INT64 play_step;
	double frequency;
	int i;

	frequency = (double)48681812 / (double)(ai_dacrate+1);
	play_step = (UINT32)((frequency / (double)Machine->sample_rate) * 65536.0);

	//printf("dma_ptr = %08X, play_ptr = %08X%08X\n", (UINT32)sb_dma_ptr, (UINT32)(sb_play_ptr >> 32),(UINT32)sb_play_ptr);
	//printf("frequency = %f, step = %08X\n", frequency, (UINT32)play_step);

	if (!audio_playing)
	{
		if (sb_dma_ptr >= 0x1000)
		{
			audio_playing = 1;
		}
		else
		{
			return;
		}
	}

	for (i = 0; i < length; i++)
	{
		short mix[2];
		mix[0] = soundbuffer[0][sb_play_ptr >> 16];
		mix[1] = soundbuffer[1][sb_play_ptr >> 16];

		sb_play_ptr += play_step;

		if ((sb_play_ptr >> 16) >= SOUNDBUFFER_LENGTH)
		{
			sb_play_ptr = 0;
		}

		// Update the buffers
		outputs[0][i] = (stream_sample_t)mix[0];
		outputs[1][i] = (stream_sample_t)mix[1];
	}
}

static void *n64_sh_start(int clock, const struct CustomSound_interface *config)
{
	stream_create( 0, 2, 44100, NULL, n64_sh_update );

	return auto_malloc(1);
}

struct CustomSound_interface n64_sound_interface =
{
	n64_sh_start
};

static UINT16 crc_seed = 0x3f;

void n64_machine_reset(void)
{
	int i;
	UINT32 *pif_rom	= (UINT32*)memory_region(REGION_USER1);
	UINT32 *cart = (UINT32*)memory_region(REGION_USER2);
	UINT64 boot_checksum;

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_DRC_OPTIONS, MIPS3DRC_FASTEST_OPTIONS + MIPS3DRC_STRICT_VERIFY);

		/* configure fast RAM regions for DRC */
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 0);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x00000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x003fffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rdram);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	soundbuffer[0] = auto_malloc(SOUNDBUFFER_LENGTH * sizeof(INT16));
	soundbuffer[1] = auto_malloc(SOUNDBUFFER_LENGTH * sizeof(INT16));
	audio_timer = timer_alloc(audio_timer_callback);
	timer_adjust(audio_timer, TIME_NEVER, 0, 0);

	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);

	// bootcode differs between CIC-chips, so we can use its checksum to detect the CIC-chip
	boot_checksum = 0;
	for (i=0x40; i < 0x1000; i+=4)
	{
		boot_checksum += cart[i/4]+i;
	}

	if (boot_checksum == U64(0x000000d057e84864))
	{
		// CIC-NUS-6101
		printf("CIC-NUS-6101 detected\n");
		crc_seed = 0x3f;
	}
	else if (boot_checksum == U64(0x000000d6499e376b))
	{
		// CIC-NUS-6103
		printf("CIC-NUS-6103 detected\n");
		crc_seed = 0x78;
	}
	else if (boot_checksum == U64(0x0000011a4a1604b6))
	{
		// CIC-NUS-6105
		printf("CIC-NUS-6105 detected\n");
		crc_seed = 0x91;
	}
	else if (boot_checksum == U64(0x000000d6d5de4ba0))
	{
		// CIC-NUS-6106
		printf("CIC-NUS-6106 detected\n");
		crc_seed = 0x85;
	}
	else
	{
		printf("Unknown BootCode Checksum %08X%08X\n", (UINT32)(boot_checksum>>32),(UINT32)(boot_checksum));
	}

	// The PIF Boot ROM is not dumped, the following code simulates it

	// clear all registers
	for (i=1; i < 32; i++)
	{
		*pif_rom++ = 0x00000000 | 0 << 21 | 0 << 16 | i << 11 | 0x20;		// ADD ri, r0, r0
	}

	// R20 <- 0x00000001
	*pif_rom++ = 0x34000000 | 20 << 16 | 0x0001;					// ORI r20, r0, 0x0001

	// R22 <- 0x0000003F
	*pif_rom++ = 0x34000000 | 22 << 16 | crc_seed;					// ORI r22, r0, 0x003f

	// R29 <- 0xA4001FF0
	*pif_rom++ = 0x3c000000 | 29 << 16 | 0xa400;					// LUI r29, 0xa400
	*pif_rom++ = 0x34000000 | 29 << 21 | 29 << 16 | 0x1ff0;			// ORI r29, r29, 0x1ff0

	// clear CP0 registers
	for (i=0; i < 32; i++)
	{
		*pif_rom++ = 0x40000000 | 4 << 21 | 0 << 16 | i << 11;		// MTC2 cp0ri, r0
	}

	// Random <- 0x0000001F
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x001f;
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 1 << 11;			// MTC2 Random, r1

	// Status <- 0x70400004
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x7040;						// LUI r1, 0x7040
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0004;			// ORI r1, r1, 0x0004
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 12 << 11;			// MTC2 Status, r1

	// PRId <- 0x00000B00
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x0b00;						// ORI r1, r0, 0x0b00
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 15 << 11;			// MTC2 PRId, r1

	// Config <- 0x0006E463
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0006;						// LUI r1, 0x0006
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0xe463;			// ORI r1, r1, 0xe463
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 16 << 11;			// MTC2 Config, r1

	// (0xa4300004) <- 0x01010101
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0101;						// LUI r1, 0x0101
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0101;			// ORI r1, r1, 0x0101
	*pif_rom++ = 0x3c000000 | 3 << 16 | 0xa430;						// LUI r3, 0xa430
	*pif_rom++ = 0xac000000 | 3 << 21 | 1 << 16 | 0x0004;			// SW r1, 0x0004(r3)

	// Copy 0xb0000000...1fff -> 0xa4000000...1fff
	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0400;						// ORI r3, r0, 0x0400
	*pif_rom++ = 0x3c000000 | 4 << 16 | 0xb000;						// LUI r4, 0xb000
	*pif_rom++ = 0x3c000000 | 5 << 16 | 0xa400;						// LUI r5, 0xa400
	*pif_rom++ = 0x8c000000 | 4 << 21 | 1 << 16;					// LW r1, 0x0000(r4)
	*pif_rom++ = 0xac000000 | 5 << 21 | 1 << 16;					// SW r1, 0x0000(r5)
	*pif_rom++ = 0x20000000 | 4 << 21 | 4 << 16 | 0x0004;			// ADDI r4, r4, 0x0004
	*pif_rom++ = 0x20000000 | 5 << 21 | 5 << 16 | 0x0004;			// ADDI r5, r5, 0x0004
	*pif_rom++ = 0x20000000 | 3 << 21 | 3 << 16 | 0xffff;			// ADDI r3, r3, -1
	*pif_rom++ = 0x14000000 | 3 << 21 | 0 << 16 | 0xfffa;			// BNE r3, r0, -6
	*pif_rom++ = 0x00000000;

	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0000;						// ORI r3, r0, 0x0000
	*pif_rom++ = 0x34000000 | 4 << 16 | 0x0000;						// ORI r4, r0, 0x0000
	*pif_rom++ = 0x34000000 | 5 << 16 | 0x0000;						// ORI r5, r0, 0x0000

	// Zelda and DK64 need these
	*pif_rom++ = 0x3c000000 | 9 << 16 | 0xa400;
	*pif_rom++ = 0x34000000 | 9 << 21 | 9 << 16 | 0x1ff0;
	*pif_rom++ = 0x3c000000 | 11 << 16 | 0xa400;
	*pif_rom++ = 0x3c000000 | 31 << 16 | 0xffff;
	*pif_rom++ = 0x34000000 | 31 << 21 | 31 << 16 | 0xffff;

	*pif_rom++ = 0x3c000000 | 1 << 16 | 0xa400;						// LUI r1, 0xa400
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0040;			// ORI r1, r1, 0x0040
	*pif_rom++ = 0x00000000 | 1 << 21 | 0x8;						// JR r1
}
