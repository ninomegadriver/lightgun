/*********************************************************************

	sgi.c

	Silicon Graphics MC (Memory Controller) code

*********************************************************************/

#include "sgi.h"

#define VERBOSE_LEVEL ( 1 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

void *tMC_UpdateTimer;
static UINT32 nMC_CPUControl0;
static UINT32 nMC_CPUControl1;
static UINT32 nMC_Watchdog;
static UINT32 nMC_SysID;
static UINT32 nMC_RPSSDiv;
static UINT32 nMC_RefCntPreload;
static UINT32 nMC_RefCnt;
static UINT32 nMC_GIO64ArbParam;
static UINT32 nMC_ArbCPUTime;
static UINT32 nMC_ArbBurstTime;
static UINT32 nMC_MemCfg0;
static UINT32 nMC_MemCfg1;
static UINT32 nMC_CPUMemAccCfg;
static UINT32 nMC_GIOMemAccCfg;
static UINT32 nMC_CPUErrorAddr;
static UINT32 nMC_CPUErrorStatus;
static UINT32 nMC_GIOErrorAddr;
static UINT32 nMC_GIOErrorStatus;
static UINT32 nMC_SysSemaphore;
static UINT32 nMC_GIOLock;
static UINT32 nMC_EISALock;
static UINT32 nMC_GIO64TransMask;
static UINT32 nMC_GIO64Subst;
static UINT32 nMC_DMAIntrCause;
static UINT32 nMC_DMAControl;
static UINT32 nMC_DMATLBEntry0Hi;
static UINT32 nMC_DMATLBEntry0Lo;
static UINT32 nMC_DMATLBEntry1Hi;
static UINT32 nMC_DMATLBEntry1Lo;
static UINT32 nMC_DMATLBEntry2Hi;
static UINT32 nMC_DMATLBEntry2Lo;
static UINT32 nMC_DMATLBEntry3Hi;
static UINT32 nMC_DMATLBEntry3Lo;
static UINT32 nMC_RPSSCounter;
static UINT32 nMC_DMAMemAddr;
static UINT32 nMC_DMAMemAddr;
static UINT32 nMC_DMALineCntWidth;
static UINT32 nMC_DMALineZoomStride;
static UINT32 nMC_DMAGIO64Addr;
static UINT32 nMC_DMAMode;
static UINT32 nMC_DMAZoomByteCnt;
static UINT32 nMC_DMARunning;

READ32_HANDLER( mc_r )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x0000:
	case 0x0004:
		verboselog( 2, "CPU Control 0 Read: %08x (%08x)\n", nMC_CPUControl0, mem_mask );
		return nMC_CPUControl0;
		break;
	case 0x0008:
	case 0x000c:
		verboselog( 2, "CPU Control 1 Read: %08x (%08x)\n", nMC_CPUControl1, mem_mask );
		return nMC_CPUControl1;
		break;
	case 0x0010:
	case 0x0014:
		verboselog( 2, "Watchdog Timer Read: %08x (%08x)\n", nMC_Watchdog, mem_mask );
		return nMC_Watchdog;
		break;
	case 0x0018:
	case 0x001c:
		verboselog( 2, "System ID Read: %08x (%08x)\n", nMC_SysID, mem_mask );
		return nMC_SysID;
		break;
	case 0x0028:
	case 0x002c:
		verboselog( 2, "RPSS Divider Read: %08x (%08x)\n", nMC_RPSSDiv, mem_mask );
		return nMC_RPSSDiv;
		break;
	case 0x0030:
	case 0x0034:
		verboselog( 2, "R4000 EEPROM Read\n" );
		return 0;
		break;
	case 0x0040:
	case 0x0044:
		verboselog( 2, "Refresh Count Preload Read: %08x (%08x)\n", nMC_RefCntPreload, mem_mask );
		return nMC_RefCntPreload;
		break;
	case 0x0048:
	case 0x004c:
		verboselog( 2, "Refresh Count Read: %08x (%08x)\n", nMC_RefCnt, mem_mask );
		return nMC_RefCnt;
		break;
	case 0x0080:
	case 0x0084:
		verboselog( 2, "GIO64 Arbitration Param Read: %08x (%08x)\n", nMC_GIO64ArbParam, mem_mask );
		return nMC_GIO64ArbParam;
		break;
	case 0x0088:
	case 0x008c:
		verboselog( 2, "Arbiter CPU Time Read: %08x (%08x)\n", nMC_ArbCPUTime, mem_mask );
		return nMC_ArbCPUTime;
		break;
	case 0x0098:
	case 0x009c:
		verboselog( 2, "Arbiter Long Burst Time Read: %08x (%08x)\n", nMC_ArbBurstTime, mem_mask );
		return nMC_ArbBurstTime;
		break;
	case 0x00c0:
	case 0x00c4:
		verboselog( 3, "Memory Configuration Register 0 Read: %08x (%08x)\n", nMC_MemCfg0, mem_mask );
		return nMC_MemCfg0;
		break;
	case 0x00c8:
	case 0x00cc:
		verboselog( 3, "Memory Configuration Register 1 Read: %08x (%08x)\n", nMC_MemCfg1, mem_mask );
		return nMC_MemCfg1;
		break;
	case 0x00d0:
	case 0x00d4:
		verboselog( 2, "CPU Memory Access Config Params Read: %08x (%08x)\n", nMC_CPUMemAccCfg, mem_mask );
		return nMC_CPUMemAccCfg;
		break;
	case 0x00d8:
	case 0x00dc:
		verboselog( 2, "GIO Memory Access Config Params Read: %08x (%08x)\n", nMC_GIOMemAccCfg, mem_mask );
		return nMC_GIOMemAccCfg;
		break;
	case 0x00e0:
	case 0x00e4:
		verboselog( 2, "CPU Error Address Read: %08x (%08x)\n", nMC_CPUErrorAddr, mem_mask );
		return nMC_CPUErrorAddr;
		break;
	case 0x00e8:
	case 0x00ec:
		verboselog( 2, "CPU Error Status Read: %08x (%08x)\n", nMC_CPUErrorStatus, mem_mask );
		return nMC_CPUErrorStatus;
		break;
	case 0x00f0:
	case 0x00f4:
		verboselog( 2, "GIO Error Address Read: %08x (%08x)\n", nMC_GIOErrorAddr, mem_mask );
		return nMC_GIOErrorAddr;
		break;
	case 0x00f8:
	case 0x00fc:
		verboselog( 2, "GIO Error Status Read: %08x (%08x)\n", nMC_GIOErrorStatus, mem_mask );
		return nMC_GIOErrorStatus;
		break;
	case 0x0100:
	case 0x0104:
		verboselog( 2, "System Semaphore Read: %08x (%08x)\n", nMC_SysSemaphore, mem_mask );
		return nMC_SysSemaphore;
		break;
	case 0x0108:
	case 0x010c:
		verboselog( 2, "GIO Lock Read: %08x (%08x)\n", nMC_GIOLock, mem_mask );
		return nMC_GIOLock;
		break;
	case 0x0110:
	case 0x0114:
		verboselog( 2, "EISA Lock Read: %08x (%08x)\n", nMC_EISALock, mem_mask );
		return nMC_EISALock;
		break;
	case 0x0150:
	case 0x0154:
		verboselog( 2, "GIO64 Translation Address Mask Read: %08x (%08x)\n", nMC_GIO64TransMask, mem_mask );
		return nMC_GIO64TransMask;
		break;
	case 0x0158:
	case 0x015c:
		verboselog( 2, "GIO64 Translation Address Substitution Bits Read: %08x (%08x)\n", nMC_GIO64Subst, mem_mask );
		return nMC_GIO64Subst;
		break;
	case 0x0160:
	case 0x0164:
		verboselog( 2, "DMA Interrupt Cause: %08x (%08x)\n", nMC_DMAIntrCause, mem_mask );
		return nMC_DMAIntrCause;
	case 0x0168:
	case 0x016c:
		verboselog( 2, "DMA Control Read: %08x (%08x)\n", nMC_DMAControl, mem_mask );
		return nMC_DMAControl;
		break;
	case 0x0180:
	case 0x0184:
		verboselog( 2, "DMA TLB Entry 0 High Read: %08x (%08x)\n", nMC_DMATLBEntry0Hi, mem_mask );
		return nMC_DMATLBEntry0Hi;
		break;
	case 0x0188:
	case 0x018c:
		verboselog( 2, "DMA TLB Entry 0 Low Read: %08x (%08x)\n", nMC_DMATLBEntry0Lo, mem_mask );
		return nMC_DMATLBEntry0Lo;
		break;
	case 0x0190:
	case 0x0194:
		verboselog( 2, "DMA TLB Entry 1 High Read: %08x (%08x)\n", nMC_DMATLBEntry1Hi, mem_mask );
		return nMC_DMATLBEntry1Hi;
		break;
	case 0x0198:
	case 0x019c:
		verboselog( 2, "DMA TLB Entry 1 Low Read: %08x (%08x)\n", nMC_DMATLBEntry1Lo, mem_mask );
		return nMC_DMATLBEntry1Lo;
		break;
	case 0x01a0:
	case 0x01a4:
		verboselog( 2, "DMA TLB Entry 2 High Read: %08x (%08x)\n", nMC_DMATLBEntry2Hi, mem_mask );
		return nMC_DMATLBEntry2Hi;
		break;
	case 0x01a8:
	case 0x01ac:
		verboselog( 2, "DMA TLB Entry 2 Low Read: %08x (%08x)\n", nMC_DMATLBEntry2Lo, mem_mask );
		return nMC_DMATLBEntry2Lo;
		break;
	case 0x01b0:
	case 0x01b4:
		verboselog( 2, "DMA TLB Entry 3 High Read: %08x (%08x)\n", nMC_DMATLBEntry3Hi, mem_mask );
		return nMC_DMATLBEntry3Hi;
		break;
	case 0x01b8:
	case 0x01bc:
		verboselog( 2, "DMA TLB Entry 3 Low Read: %08x (%08x)\n", nMC_DMATLBEntry3Lo, mem_mask );
		return nMC_DMATLBEntry3Lo;
		break;
	case 0x1000:
	case 0x1004:
		verboselog( 2, "RPSS 100ns Counter Read: %08x (%08x)\n", nMC_RPSSCounter, mem_mask );
		return nMC_RPSSCounter;
		break;
	case 0x2000:
	case 0x2004:
	case 0x2008:
	case 0x200c:
		verboselog( 0, "DMA Memory Address Read: %08x (%08x)\n", nMC_DMAMemAddr, mem_mask );
		return nMC_DMAMemAddr;
		break;
	case 0x2010:
	case 0x2014:
		verboselog( 0, "DMA Line Count and Width Read: %08x (%08x)\n", nMC_DMALineCntWidth, mem_mask );
		return nMC_DMALineCntWidth;
		break;
	case 0x2018:
	case 0x201c:
		verboselog( 0, "DMA Line Zoom and Stride Read: %08x (%08x)\n", nMC_DMALineZoomStride, mem_mask );
		return nMC_DMALineZoomStride;
		break;
	case 0x2020:
	case 0x2024:
	case 0x2028:
	case 0x202c:
		verboselog( 0, "DMA GIO64 Address Read: %08x (%08x)\n", nMC_DMAGIO64Addr, mem_mask );
		return nMC_DMAGIO64Addr;
		break;
	case 0x2030:
	case 0x2034:
		verboselog( 0, "DMA Mode Write: %08x (%08x)\n", nMC_DMAMode, mem_mask );
		return nMC_DMAMode;
		break;
	case 0x2038:
	case 0x203c:
		verboselog( 0, "DMA Zoom Count Read: %08x (%08x)\n", nMC_DMAZoomByteCnt, mem_mask );
		return nMC_DMAZoomByteCnt;
		break;
//	case 0x2040:
//	case 0x2044:
//		verboselog( 2, "DMA Start Write: %08x (%08x)\n", data, mem_mask );
		// Start DMA
//		nMC_DMARunning = 1;
//		break;
	case 0x2048:
	case 0x204c:
		verboselog( 0, "VDMA Running Read: %08x (%08x)\n", nMC_DMARunning, mem_mask );
		if( nMC_DMARunning == 1 )
		{
			nMC_DMARunning = 0;
			return 0x00000040;
		}
		else
		{
			return 0;
		}
		break;
	}
	verboselog( 0, "Unmapped MC read: 0x%08x (%08x)\n", 0x1fa00000 + offset, mem_mask );
	return 0;
}

WRITE32_HANDLER( mc_w )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x0000:
	case 0x0004:
		verboselog( 2, "CPU Control 0 Write: %08x (%08x)\n", data, mem_mask );
		nMC_CPUControl0 = data;
		break;
	case 0x0008:
	case 0x000c:
		verboselog( 2, "CPU Control 1 Write: %08x (%08x)\n", data, mem_mask );
		nMC_CPUControl1 = data;
		break;
	case 0x0010:
	case 0x0014:
		verboselog( 2, "Watchdog Timer Clear" );
		nMC_Watchdog = 0;
		break;
	case 0x0028:
	case 0x002c:
		verboselog( 2, "RPSS Divider Write: %08x (%08x)\n", data, mem_mask );
		nMC_RPSSDiv = data;
		break;
	case 0x0030:
	case 0x0034:
		verboselog( 2, "R4000 EEPROM Write\n" );
		break;
	case 0x0040:
	case 0x0044:
		verboselog( 2, "Refresh Count Preload Write: %08x (%08x)\n", data, mem_mask );
		nMC_RefCntPreload = data;
		break;
	case 0x0080:
	case 0x0084:
		verboselog( 2, "GIO64 Arbitration Param Write: %08x (%08x)\n", data, mem_mask );
		nMC_GIO64ArbParam = data;
		break;
	case 0x0088:
	case 0x008c:
		verboselog( 2, "Arbiter CPU Time Write: %08x (%08x)\n", data, mem_mask );
		nMC_ArbCPUTime = data;
		break;
	case 0x0098:
	case 0x009c:
		verboselog( 2, "Arbiter Long Burst Time Write: %08x (%08x)\n", data, mem_mask );
		nMC_ArbBurstTime = data;
		break;
	case 0x00c0:
	case 0x00c4:
		verboselog( 3, "Memory Configuration Register 0 Write: %08x (%08x)\n", data, mem_mask );
		nMC_MemCfg0 = data;
		break;
	case 0x00c8:
	case 0x00cc:
		verboselog( 3, "Memory Configuration Register 1 Write: %08x (%08x)\n", data, mem_mask );
		nMC_MemCfg1 = data;
		break;
	case 0x00d0:
	case 0x00d4:
		verboselog( 2, "CPU Memory Access Config Params Write: %08x (%08x)\n", data, mem_mask );
		nMC_CPUMemAccCfg = data;
		break;
	case 0x00d8:
	case 0x00dc:
		verboselog( 2, "GIO Memory Access Config Params Write: %08x (%08x)\n", data, mem_mask );
		nMC_GIOMemAccCfg = data;
		break;
	case 0x00e8:
	case 0x00ec:
		verboselog( 2, "CPU Error Status Clear\n" );
		nMC_CPUErrorStatus = 0;
		break;
	case 0x00f8:
	case 0x00fc:
		verboselog( 2, "GIO Error Status Clear\n" );
		nMC_GIOErrorStatus = 0;
		break;
	case 0x0100:
	case 0x0104:
		verboselog( 2, "System Semaphore Write: %08x (%08x)\n", data, mem_mask );
		nMC_SysSemaphore = data;
		break;
	case 0x0108:
	case 0x010c:
		verboselog( 2, "GIO Lock Write: %08x (%08x)\n", data, mem_mask );
		nMC_GIOLock = data;
		break;
	case 0x0110:
	case 0x0114:
		verboselog( 2, "EISA Lock Write: %08x (%08x)\n", data, mem_mask );
		nMC_EISALock = data;
		break;
	case 0x0150:
	case 0x0154:
		verboselog( 2, "GIO64 Translation Address Mask Write: %08x (%08x)\n", data, mem_mask );
		nMC_GIO64TransMask = data;
		break;
	case 0x0158:
	case 0x015c:
		verboselog( 2, "GIO64 Translation Address Substitution Bits Write: %08x (%08x)\n", data, mem_mask );
		nMC_GIO64Subst = data;
		break;
	case 0x0160:
	case 0x0164:
		verboselog( 0, "DMA Interrupt Cause Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAIntrCause = data;
		break;
	case 0x0168:
	case 0x016c:
		verboselog( 0, "DMA Control Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAControl = data;
		break;
	case 0x0180:
	case 0x0184:
		verboselog( 0, "DMA TLB Entry 0 High Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry0Hi = data;
		break;
	case 0x0188:
	case 0x018c:
		verboselog( 0, "DMA TLB Entry 0 Low Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry0Lo = data;
		break;
	case 0x0190:
	case 0x0194:
		verboselog( 0, "DMA TLB Entry 1 High Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry1Hi = data;
		break;
	case 0x0198:
	case 0x019c:
		verboselog( 0, "DMA TLB Entry 1 Low Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry1Lo = data;
		break;
	case 0x01a0:
	case 0x01a4:
		verboselog( 0, "DMA TLB Entry 2 High Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry2Hi = data;
		break;
	case 0x01a8:
	case 0x01ac:
		verboselog( 0, "DMA TLB Entry 2 Low Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry2Lo = data;
		break;
	case 0x01b0:
	case 0x01b4:
		verboselog( 0, "DMA TLB Entry 3 High Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry3Hi = data;
		break;
	case 0x01b8:
	case 0x01bc:
		verboselog( 0, "DMA TLB Entry 3 Low Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMATLBEntry3Lo = data;
		break;
	case 0x2000:
	case 0x2004:
		verboselog( 0, "DMA Memory Address Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAMemAddr = data;
		break;
	case 0x2008:
	case 0x200c:
		verboselog( 0, "DMA Memory Address + Default Params Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAMemAddr = data;
		break;
	case 0x2010:
	case 0x2014:
		verboselog( 0, "DMA Line Count and Width Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMALineCntWidth = data;
		break;
	case 0x2018:
	case 0x201c:
		verboselog( 0, "DMA Line Zoom and Stride Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMALineZoomStride = data;
		break;
	case 0x2020:
	case 0x2024:
		verboselog( 0, "DMA GIO64 Address Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAGIO64Addr = data;
		break;
	case 0x2028:
	case 0x202c:
		verboselog( 0, "DMA GIO64 Address Write + Start DMA: %08x (%08x)\n", data, mem_mask );
		nMC_DMAGIO64Addr = data;
		// Start DMA
		nMC_DMARunning = 1;
		break;
	case 0x2030:
	case 0x2034:
		verboselog( 0, "DMA Mode Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAMode = data;
		break;
	case 0x2038:
	case 0x203c:
		verboselog( 0, "DMA Zoom Count + Byte Count Write: %08x (%08x)\n", data, mem_mask );
		nMC_DMAZoomByteCnt = data;
		break;
	case 0x2040:
	case 0x2044:
		verboselog( 0, "DMA Start Write: %08x (%08x)\n", data, mem_mask );
		// Start DMA
		nMC_DMARunning = 1;
		break;
	case 0x2070:
	case 0x2074:
		verboselog( 0, "DMA GIO64 Address Write + Default Params Write + Start DMA: %08x (%08x)\n", data, mem_mask );
		nMC_DMAGIO64Addr = data;
		// Start DMA
		nMC_DMARunning = 1;
		break;
	default:
		verboselog( 0, "Unmapped MC write: 0x%08x: %08x (%08x)\n", 0x1fa00000 + offset, data, mem_mask );
		break;
	}
}

void mc_update( int nParam )
{
	nMC_RPSSCounter += 1000;
}

void mc_init()
{
	nMC_CPUControl0 = 0;
	nMC_CPUControl1 = 0;
	nMC_Watchdog = 0;
	nMC_SysID = 0;
	nMC_RPSSDiv = 0;
	nMC_RefCntPreload = 0;
	nMC_RefCnt = 0;
	nMC_GIO64ArbParam = 0;
	nMC_ArbCPUTime = 0;
	nMC_ArbBurstTime = 0;
	nMC_MemCfg0 = 0;
	nMC_MemCfg1 = 0;
	nMC_CPUMemAccCfg = 0;
	nMC_GIOMemAccCfg = 0;
	nMC_CPUErrorAddr = 0;
	nMC_CPUErrorStatus = 0;
	nMC_GIOErrorAddr = 0;
	nMC_GIOErrorStatus = 0;
	nMC_SysSemaphore = 0;
	nMC_GIOLock = 0;
	nMC_EISALock = 0;
	nMC_GIO64TransMask = 0;
	nMC_GIO64Subst = 0;
	nMC_DMAIntrCause = 0;
	nMC_DMAControl = 0;
	nMC_DMATLBEntry0Hi = 0;
	nMC_DMATLBEntry0Lo = 0;
	nMC_DMATLBEntry1Hi = 0;
	nMC_DMATLBEntry1Lo = 0;
	nMC_DMATLBEntry2Hi = 0;
	nMC_DMATLBEntry2Lo = 0;
	nMC_DMATLBEntry3Hi = 0;
	nMC_DMATLBEntry3Lo = 0;
	nMC_RPSSCounter = 0;
	nMC_DMAMemAddr = 0;
	nMC_DMAMemAddr = 0;
	nMC_DMALineCntWidth = 0;
	nMC_DMALineZoomStride = 0;
	nMC_DMAGIO64Addr = 0;
	nMC_DMAMode = 0;
	nMC_DMAZoomByteCnt = 0;
	tMC_UpdateTimer = timer_alloc( mc_update );
	timer_adjust( tMC_UpdateTimer, TIME_IN_SEC(1.0/10000.0), 0, TIME_IN_SEC(1.0/10000.0) );

	// if Indigo2, ID appropriately
	if (!strcmp(Machine->gamedrv->name, "ip244415"))
	{
		nMC_SysID = 0x11;	// rev. B MC, EISA bus present
	}
}
