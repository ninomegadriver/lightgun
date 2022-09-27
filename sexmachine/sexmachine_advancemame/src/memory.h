/***************************************************************************

    memory.h

    Functions which handle the CPU memory accesses.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "mamecore.h"



/***************************************************************************

    Parameters

***************************************************************************/

#ifdef MAME_DEBUG
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		1
#elif defined(MESS)
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	1
#define CPUREADOP_SAFETY_FULL		0
#else
#define CPUREADOP_SAFETY_NONE		1
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		0
#endif



/***************************************************************************

    Basic type definitions

    These types are used for data access handlers.

***************************************************************************/

/* ----- typedefs for data and offset types ----- */
typedef UINT32			offs_t;

/* ----- typedefs for the various common data access handlers ----- */
typedef UINT8			(*read8_handler)  (ATTR_UNUSED offs_t offset);
typedef void			(*write8_handler) (ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
typedef UINT16			(*read16_handler) (ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask);
typedef void			(*write16_handler)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask);
typedef UINT32			(*read32_handler) (ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask);
typedef void			(*write32_handler)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask);
typedef UINT64			(*read64_handler) (ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask);
typedef void			(*write64_handler)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask);
typedef offs_t			(*opbase_handler) (ATTR_UNUSED offs_t address);

/* ----- this struct contains pointers to the live read/write routines ----- */
struct _data_accessors
{
	UINT8			(*read_byte)(offs_t offset);
	UINT16			(*read_word)(offs_t offset);
	UINT32			(*read_dword)(offs_t offset);
	UINT64			(*read_qword)(offs_t offset);

	void			(*write_byte)(offs_t offset, UINT8 data);
	void			(*write_word)(offs_t offset, UINT16 data);
	void			(*write_dword)(offs_t offset, UINT32 data);
	void			(*write_qword)(offs_t offset, UINT64 data);
};
typedef struct _data_accessors data_accessors;



/***************************************************************************

    Basic macros

***************************************************************************/

/* ----- macros for declaring the various common data access handlers ----- */
#define READ8_HANDLER(name) 	UINT8  name(ATTR_UNUSED offs_t offset)
#define WRITE8_HANDLER(name) 	void   name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data)
#define READ16_HANDLER(name)	UINT16 name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask)
#define WRITE16_HANDLER(name)	void   name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask)
#define READ32_HANDLER(name)	UINT32 name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask)
#define WRITE32_HANDLER(name)	void   name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask)
#define READ64_HANDLER(name)	UINT64 name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask)
#define WRITE64_HANDLER(name)	void   name(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask)
#define OPBASE_HANDLER(name)	offs_t name(ATTR_UNUSED offs_t address)

/* ----- macros for accessing bytes and words within larger chunks ----- */
#ifdef LSB_FIRST
	#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE_XOR_LE(a)  	(a)
	#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define BYTE4_XOR_LE(a) 	(a)
	#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define WORD_XOR_LE(a)  	(a)
	#define BYTE8_XOR_BE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
	#define BYTE8_XOR_LE(a) 	(a)
	#define WORD2_XOR_BE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
	#define WORD2_XOR_LE(a)  	(a)
	#define DWORD_XOR_BE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
	#define DWORD_XOR_LE(a)  	(a)
#else
	#define BYTE_XOR_BE(a)  	(a)
	#define BYTE_XOR_LE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE4_XOR_BE(a) 	(a)
	#define BYTE4_XOR_LE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define WORD_XOR_BE(a)  	(a)
	#define WORD_XOR_LE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define BYTE8_XOR_BE(a) 	(a)
	#define BYTE8_XOR_LE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
	#define WORD2_XOR_BE(a)  	(a)
	#define WORD2_XOR_LE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
	#define DWORD_XOR_BE(a)  	(a)
	#define DWORD_XOR_LE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
#endif



/***************************************************************************

    Address array constants

    These apply to values in the array of read/write handlers that is
    declared within each driver.

***************************************************************************/

/* ----- definitions for the flags in the address maps ----- */
#define AM_FLAGS_EXTENDED		0x01					/* this is an extended entry with the below flags in the start field */
#define AM_FLAGS_MATCH_MASK		0x02					/* this entry should have the start/end pair treated as a match/mask pair */
#define AM_FLAGS_END			0x04					/* this is the terminating entry in the array */

/* ----- definitions for the extended flags in the address maps ----- */
#define AMEF_SPECIFIES_SPACE	0x00000001				/* set if the address space is specified */
#define AMEF_SPECIFIES_ABITS	0x00000002				/* set if the number of address space bits is specified */
#define AMEF_SPECIFIES_DBITS	0x00000004				/* set if the databus width is specified */
#define AMEF_SPECIFIES_UNMAP	0x00000008				/* set if the unmap value is specified */

/* ----- definitions for specifying the address space in the extended flags ----- */
#define AMEF_SPACE_SHIFT		8						/* shift to get at the address space */
#define AMEF_SPACE_MASK			(0x0f << AMEF_SPACE_SHIFT) /* mask to get at the address space */
#define AMEF_SPACE(x)			(((x) << AMEF_SPACE_SHIFT) | AMEF_SPECIFIES_SPACE) /* specifies a given address space */

/* ----- definitions for specifying the address bus width in the extended flags ----- */
#define AMEF_ABITS_SHIFT		12						/* shift to get the address bits count */
#define AMEF_ABITS_MASK			(0x3f << AMEF_ABITS_SHIFT) /* mask to get at the address bits count */
#define AMEF_ABITS(n)			(((n) << AMEF_ABITS_SHIFT) | AMEF_SPECIFIES_ABITS) /* specifies a given number of address  */

/* ----- definitions for specifying the data bus width in the extended flags ----- */
#define AMEF_DBITS_SHIFT		18						/* shift to get the data bits count */
#define AMEF_DBITS_MASK			(0x07 << AMEF_DBITS_SHIFT) /* mask to get at the data bits count */
#define AMEF_DBITS(n)			((((n)/8-1) << AMEF_DBITS_SHIFT) | AMEF_SPECIFIES_DBITS) /* specifies a given data bus width */

/* ----- definitions for specifying the unmap value in the extended flags ----- */
#define AMEF_UNMAP_SHIFT		21						/* shift to get the unmap value */
#define AMEF_UNMAP_MASK			(1 << AMEF_UNMAP_SHIFT)	/* mask to get at the unmap value */
#define AMEF_UNMAP(x)			(((x) << AMEF_UNMAP_SHIFT) | AMEF_SPECIFIES_UNMAP) /* specifies a given unmap value */

/* ----- static data access handler constants ----- */
#define STATIC_INVALID			0						/* invalid - should never be used */
#define STATIC_BANK1			1						/* banked memory #1 */
#define STATIC_BANK2			2						/* banked memory #2 */
#define STATIC_BANK3			3						/* banked memory #3 */
#define STATIC_BANK4			4						/* banked memory #4 */
#define STATIC_BANK5			5						/* banked memory #5 */
#define STATIC_BANK6			6						/* banked memory #6 */
#define STATIC_BANK7			7						/* banked memory #7 */
#define STATIC_BANK8			8						/* banked memory #8 */
#define STATIC_BANK9			9						/* banked memory #9 */
#define STATIC_BANK10			10						/* banked memory #10 */
#define STATIC_BANK11			11						/* banked memory #11 */
#define STATIC_BANK12			12						/* banked memory #12 */
#define STATIC_BANK13			13						/* banked memory #13 */
#define STATIC_BANK14			14						/* banked memory #14 */
#define STATIC_BANK15			15						/* banked memory #15 */
#define STATIC_BANK16			16						/* banked memory #16 */
#define STATIC_BANK17			17						/* banked memory #17 */
#define STATIC_BANK18			18						/* banked memory #18 */
#define STATIC_BANK19			19						/* banked memory #19 */
#define STATIC_BANK20			20						/* banked memory #20 */
#define STATIC_BANK21			21						/* banked memory #21 */
#define STATIC_BANK22			22						/* banked memory #22 */
#define STATIC_BANK23			23						/* banked memory #23 */
#define STATIC_BANK24			24						/* banked memory #24 */
#define STATIC_BANK25			25						/* banked memory #25 */
#define STATIC_BANK26			26						/* banked memory #26 */
#define STATIC_BANK27			27						/* banked memory #27 */
#define STATIC_BANK28			28						/* banked memory #28 */
#define STATIC_BANK29			29						/* banked memory #29 */
#define STATIC_BANK30			30						/* banked memory #30 */
#define STATIC_BANK31			31						/* banked memory #31 */
#define STATIC_BANK32			32						/* banked memory #32 */
/* entries 33-67 are reserved for dynamically allocated internal banks */
#define STATIC_RAM				68						/* RAM - standard reads/writes */
#define STATIC_ROM				69						/* ROM - standard reads, no writes */
#define STATIC_NOP				70						/* unmapped - all unmapped memory goes here */
#define STATIC_UNMAP			71						/* unmapped - all unmapped memory goes here */
#define STATIC_COUNT			72						/* total number of static handlers */

/* ----- banking constants ----- */
#define MAX_BANKS				66						/* maximum number of banks */
#define MAX_BANK_ENTRIES		64						/* maximum number of possible bank values */
#define MAX_EXPLICIT_BANKS		32						/* maximum number of explicitly-defined banks */
#define STATIC_BANKMAX			(STATIC_RAM - 1)		/* handler constant of last bank */



/***************************************************************************

    Constants for static entries in address map read/write arrays

    The first 32 entries in the address lookup table are reserved for
    "static" handlers. These are internal handlers for RAM, ROM, banks,
    and unmapped areas in the address space. The following definitions
    are the properly-casted versions of the STATIC_ constants above.

***************************************************************************/

/* 8-bit reads */
#define MRA8_BANK1				((read8_handler)STATIC_BANK1)
#define MRA8_BANK2				((read8_handler)STATIC_BANK2)
#define MRA8_BANK3				((read8_handler)STATIC_BANK3)
#define MRA8_BANK4				((read8_handler)STATIC_BANK4)
#define MRA8_BANK5				((read8_handler)STATIC_BANK5)
#define MRA8_BANK6				((read8_handler)STATIC_BANK6)
#define MRA8_BANK7				((read8_handler)STATIC_BANK7)
#define MRA8_BANK8				((read8_handler)STATIC_BANK8)
#define MRA8_BANK9				((read8_handler)STATIC_BANK9)
#define MRA8_BANK10				((read8_handler)STATIC_BANK10)
#define MRA8_BANK11				((read8_handler)STATIC_BANK11)
#define MRA8_BANK12				((read8_handler)STATIC_BANK12)
#define MRA8_BANK13				((read8_handler)STATIC_BANK13)
#define MRA8_BANK14				((read8_handler)STATIC_BANK14)
#define MRA8_BANK15				((read8_handler)STATIC_BANK15)
#define MRA8_BANK16				((read8_handler)STATIC_BANK16)
#define MRA8_BANK17				((read8_handler)STATIC_BANK17)
#define MRA8_BANK18				((read8_handler)STATIC_BANK18)
#define MRA8_BANK19				((read8_handler)STATIC_BANK19)
#define MRA8_BANK20				((read8_handler)STATIC_BANK20)
#define MRA8_BANK21				((read8_handler)STATIC_BANK21)
#define MRA8_BANK22				((read8_handler)STATIC_BANK22)
#define MRA8_BANK23				((read8_handler)STATIC_BANK23)
#define MRA8_BANK24				((read8_handler)STATIC_BANK24)
#define MRA8_BANK25				((read8_handler)STATIC_BANK25)
#define MRA8_BANK26				((read8_handler)STATIC_BANK26)
#define MRA8_BANK27				((read8_handler)STATIC_BANK27)
#define MRA8_BANK28				((read8_handler)STATIC_BANK28)
#define MRA8_BANK29				((read8_handler)STATIC_BANK29)
#define MRA8_BANK30				((read8_handler)STATIC_BANK30)
#define MRA8_BANK31				((read8_handler)STATIC_BANK31)
#define MRA8_BANK32				((read8_handler)STATIC_BANK32)
#define MRA8_RAM				((read8_handler)STATIC_RAM)
#define MRA8_ROM				((read8_handler)STATIC_ROM)
#define MRA8_NOP				((read8_handler)STATIC_NOP)
#define MRA8_UNMAP				((read8_handler)STATIC_UNMAP)

/* 8-bit writes */
#define MWA8_BANK1				((write8_handler)STATIC_BANK1)
#define MWA8_BANK2				((write8_handler)STATIC_BANK2)
#define MWA8_BANK3				((write8_handler)STATIC_BANK3)
#define MWA8_BANK4				((write8_handler)STATIC_BANK4)
#define MWA8_BANK5				((write8_handler)STATIC_BANK5)
#define MWA8_BANK6				((write8_handler)STATIC_BANK6)
#define MWA8_BANK7				((write8_handler)STATIC_BANK7)
#define MWA8_BANK8				((write8_handler)STATIC_BANK8)
#define MWA8_BANK9				((write8_handler)STATIC_BANK9)
#define MWA8_BANK10				((write8_handler)STATIC_BANK10)
#define MWA8_BANK11				((write8_handler)STATIC_BANK11)
#define MWA8_BANK12				((write8_handler)STATIC_BANK12)
#define MWA8_BANK13				((write8_handler)STATIC_BANK13)
#define MWA8_BANK14				((write8_handler)STATIC_BANK14)
#define MWA8_BANK15				((write8_handler)STATIC_BANK15)
#define MWA8_BANK16				((write8_handler)STATIC_BANK16)
#define MWA8_BANK17				((write8_handler)STATIC_BANK17)
#define MWA8_BANK18				((write8_handler)STATIC_BANK18)
#define MWA8_BANK19				((write8_handler)STATIC_BANK19)
#define MWA8_BANK20				((write8_handler)STATIC_BANK20)
#define MWA8_BANK21				((write8_handler)STATIC_BANK21)
#define MWA8_BANK22				((write8_handler)STATIC_BANK22)
#define MWA8_BANK23				((write8_handler)STATIC_BANK23)
#define MWA8_BANK24				((write8_handler)STATIC_BANK24)
#define MWA8_BANK25				((write8_handler)STATIC_BANK25)
#define MWA8_BANK26				((write8_handler)STATIC_BANK26)
#define MWA8_BANK27				((write8_handler)STATIC_BANK27)
#define MWA8_BANK28				((write8_handler)STATIC_BANK28)
#define MWA8_BANK29				((write8_handler)STATIC_BANK29)
#define MWA8_BANK30				((write8_handler)STATIC_BANK30)
#define MWA8_BANK31				((write8_handler)STATIC_BANK31)
#define MWA8_BANK32				((write8_handler)STATIC_BANK32)
#define MWA8_RAM				((write8_handler)STATIC_RAM)
#define MWA8_ROM				((write8_handler)STATIC_ROM)
#define MWA8_NOP				((write8_handler)STATIC_NOP)
#define MWA8_UNMAP				((write8_handler)STATIC_UNMAP)

/* 16-bit reads */
#define MRA16_BANK1				((read16_handler)STATIC_BANK1)
#define MRA16_BANK2				((read16_handler)STATIC_BANK2)
#define MRA16_BANK3				((read16_handler)STATIC_BANK3)
#define MRA16_BANK4				((read16_handler)STATIC_BANK4)
#define MRA16_BANK5				((read16_handler)STATIC_BANK5)
#define MRA16_BANK6				((read16_handler)STATIC_BANK6)
#define MRA16_BANK7				((read16_handler)STATIC_BANK7)
#define MRA16_BANK8				((read16_handler)STATIC_BANK8)
#define MRA16_BANK9				((read16_handler)STATIC_BANK9)
#define MRA16_BANK10			((read16_handler)STATIC_BANK10)
#define MRA16_BANK11			((read16_handler)STATIC_BANK11)
#define MRA16_BANK12			((read16_handler)STATIC_BANK12)
#define MRA16_BANK13			((read16_handler)STATIC_BANK13)
#define MRA16_BANK14			((read16_handler)STATIC_BANK14)
#define MRA16_BANK15			((read16_handler)STATIC_BANK15)
#define MRA16_BANK16			((read16_handler)STATIC_BANK16)
#define MRA16_BANK17			((read16_handler)STATIC_BANK17)
#define MRA16_BANK18			((read16_handler)STATIC_BANK18)
#define MRA16_BANK19			((read16_handler)STATIC_BANK19)
#define MRA16_BANK20			((read16_handler)STATIC_BANK20)
#define MRA16_BANK21			((read16_handler)STATIC_BANK21)
#define MRA16_BANK22			((read16_handler)STATIC_BANK22)
#define MRA16_BANK23			((read16_handler)STATIC_BANK23)
#define MRA16_BANK24			((read16_handler)STATIC_BANK24)
#define MRA16_BANK25			((read16_handler)STATIC_BANK25)
#define MRA16_BANK26			((read16_handler)STATIC_BANK26)
#define MRA16_BANK27			((read16_handler)STATIC_BANK27)
#define MRA16_BANK28			((read16_handler)STATIC_BANK28)
#define MRA16_BANK29			((read16_handler)STATIC_BANK29)
#define MRA16_BANK30			((read16_handler)STATIC_BANK30)
#define MRA16_BANK31			((read16_handler)STATIC_BANK31)
#define MRA16_BANK32			((read16_handler)STATIC_BANK32)
#define MRA16_RAM				((read16_handler)STATIC_RAM)
#define MRA16_ROM				((read16_handler)STATIC_ROM)
#define MRA16_NOP				((read16_handler)STATIC_NOP)
#define MRA16_UNMAP				((read16_handler)STATIC_UNMAP)

/* 16-bit writes */
#define MWA16_BANK1				((write16_handler)STATIC_BANK1)
#define MWA16_BANK2				((write16_handler)STATIC_BANK2)
#define MWA16_BANK3				((write16_handler)STATIC_BANK3)
#define MWA16_BANK4				((write16_handler)STATIC_BANK4)
#define MWA16_BANK5				((write16_handler)STATIC_BANK5)
#define MWA16_BANK6				((write16_handler)STATIC_BANK6)
#define MWA16_BANK7				((write16_handler)STATIC_BANK7)
#define MWA16_BANK8				((write16_handler)STATIC_BANK8)
#define MWA16_BANK9				((write16_handler)STATIC_BANK9)
#define MWA16_BANK10			((write16_handler)STATIC_BANK10)
#define MWA16_BANK11			((write16_handler)STATIC_BANK11)
#define MWA16_BANK12			((write16_handler)STATIC_BANK12)
#define MWA16_BANK13			((write16_handler)STATIC_BANK13)
#define MWA16_BANK14			((write16_handler)STATIC_BANK14)
#define MWA16_BANK15			((write16_handler)STATIC_BANK15)
#define MWA16_BANK16			((write16_handler)STATIC_BANK16)
#define MWA16_BANK17			((write16_handler)STATIC_BANK17)
#define MWA16_BANK18			((write16_handler)STATIC_BANK18)
#define MWA16_BANK19			((write16_handler)STATIC_BANK19)
#define MWA16_BANK20			((write16_handler)STATIC_BANK20)
#define MWA16_BANK21			((write16_handler)STATIC_BANK21)
#define MWA16_BANK22			((write16_handler)STATIC_BANK22)
#define MWA16_BANK23			((write16_handler)STATIC_BANK23)
#define MWA16_BANK24			((write16_handler)STATIC_BANK24)
#define MWA16_BANK25			((write16_handler)STATIC_BANK25)
#define MWA16_BANK26			((write16_handler)STATIC_BANK26)
#define MWA16_BANK27			((write16_handler)STATIC_BANK27)
#define MWA16_BANK28			((write16_handler)STATIC_BANK28)
#define MWA16_BANK29			((write16_handler)STATIC_BANK29)
#define MWA16_BANK30			((write16_handler)STATIC_BANK30)
#define MWA16_BANK31			((write16_handler)STATIC_BANK31)
#define MWA16_BANK32			((write16_handler)STATIC_BANK32)
#define MWA16_RAM				((write16_handler)STATIC_RAM)
#define MWA16_ROM				((write16_handler)STATIC_ROM)
#define MWA16_NOP				((write16_handler)STATIC_NOP)
#define MWA16_UNMAP				((write16_handler)STATIC_UNMAP)

/* 32-bit reads */
#define MRA32_BANK1				((read32_handler)STATIC_BANK1)
#define MRA32_BANK2				((read32_handler)STATIC_BANK2)
#define MRA32_BANK3				((read32_handler)STATIC_BANK3)
#define MRA32_BANK4				((read32_handler)STATIC_BANK4)
#define MRA32_BANK5				((read32_handler)STATIC_BANK5)
#define MRA32_BANK6				((read32_handler)STATIC_BANK6)
#define MRA32_BANK7				((read32_handler)STATIC_BANK7)
#define MRA32_BANK8				((read32_handler)STATIC_BANK8)
#define MRA32_BANK9				((read32_handler)STATIC_BANK9)
#define MRA32_BANK10			((read32_handler)STATIC_BANK10)
#define MRA32_BANK11			((read32_handler)STATIC_BANK11)
#define MRA32_BANK12			((read32_handler)STATIC_BANK12)
#define MRA32_BANK13			((read32_handler)STATIC_BANK13)
#define MRA32_BANK14			((read32_handler)STATIC_BANK14)
#define MRA32_BANK15			((read32_handler)STATIC_BANK15)
#define MRA32_BANK16			((read32_handler)STATIC_BANK16)
#define MRA32_BANK17			((read32_handler)STATIC_BANK17)
#define MRA32_BANK18			((read32_handler)STATIC_BANK18)
#define MRA32_BANK19			((read32_handler)STATIC_BANK19)
#define MRA32_BANK20			((read32_handler)STATIC_BANK20)
#define MRA32_BANK21			((read32_handler)STATIC_BANK21)
#define MRA32_BANK22			((read32_handler)STATIC_BANK22)
#define MRA32_BANK23			((read32_handler)STATIC_BANK23)
#define MRA32_BANK24			((read32_handler)STATIC_BANK24)
#define MRA32_BANK25			((read32_handler)STATIC_BANK25)
#define MRA32_BANK26			((read32_handler)STATIC_BANK26)
#define MRA32_BANK27			((read32_handler)STATIC_BANK27)
#define MRA32_BANK28			((read32_handler)STATIC_BANK28)
#define MRA32_BANK29			((read32_handler)STATIC_BANK29)
#define MRA32_BANK30			((read32_handler)STATIC_BANK30)
#define MRA32_BANK31			((read32_handler)STATIC_BANK31)
#define MRA32_BANK32			((read32_handler)STATIC_BANK32)
#define MRA32_RAM				((read32_handler)STATIC_RAM)
#define MRA32_ROM				((read32_handler)STATIC_ROM)
#define MRA32_NOP				((read32_handler)STATIC_NOP)
#define MRA32_UNMAP				((read32_handler)STATIC_UNMAP)

/* 32-bit writes */
#define MWA32_BANK1				((write32_handler)STATIC_BANK1)
#define MWA32_BANK2				((write32_handler)STATIC_BANK2)
#define MWA32_BANK3				((write32_handler)STATIC_BANK3)
#define MWA32_BANK4				((write32_handler)STATIC_BANK4)
#define MWA32_BANK5				((write32_handler)STATIC_BANK5)
#define MWA32_BANK6				((write32_handler)STATIC_BANK6)
#define MWA32_BANK7				((write32_handler)STATIC_BANK7)
#define MWA32_BANK8				((write32_handler)STATIC_BANK8)
#define MWA32_BANK9				((write32_handler)STATIC_BANK9)
#define MWA32_BANK10			((write32_handler)STATIC_BANK10)
#define MWA32_BANK11			((write32_handler)STATIC_BANK11)
#define MWA32_BANK12			((write32_handler)STATIC_BANK12)
#define MWA32_BANK13			((write32_handler)STATIC_BANK13)
#define MWA32_BANK14			((write32_handler)STATIC_BANK14)
#define MWA32_BANK15			((write32_handler)STATIC_BANK15)
#define MWA32_BANK16			((write32_handler)STATIC_BANK16)
#define MWA32_BANK17			((write32_handler)STATIC_BANK17)
#define MWA32_BANK18			((write32_handler)STATIC_BANK18)
#define MWA32_BANK19			((write32_handler)STATIC_BANK19)
#define MWA32_BANK20			((write32_handler)STATIC_BANK20)
#define MWA32_BANK21			((write32_handler)STATIC_BANK21)
#define MWA32_BANK22			((write32_handler)STATIC_BANK22)
#define MWA32_BANK23			((write32_handler)STATIC_BANK23)
#define MWA32_BANK24			((write32_handler)STATIC_BANK24)
#define MWA32_BANK25			((write32_handler)STATIC_BANK25)
#define MWA32_BANK26			((write32_handler)STATIC_BANK26)
#define MWA32_BANK27			((write32_handler)STATIC_BANK27)
#define MWA32_BANK28			((write32_handler)STATIC_BANK28)
#define MWA32_BANK29			((write32_handler)STATIC_BANK29)
#define MWA32_BANK30			((write32_handler)STATIC_BANK30)
#define MWA32_BANK31			((write32_handler)STATIC_BANK31)
#define MWA32_BANK32			((write32_handler)STATIC_BANK32)
#define MWA32_RAM				((write32_handler)STATIC_RAM)
#define MWA32_ROM				((write32_handler)STATIC_ROM)
#define MWA32_NOP				((write32_handler)STATIC_NOP)
#define MWA32_UNMAP				((write32_handler)STATIC_UNMAP)

/* 64-bit reads */
#define MRA64_BANK1				((read64_handler)STATIC_BANK1)
#define MRA64_BANK2				((read64_handler)STATIC_BANK2)
#define MRA64_BANK3				((read64_handler)STATIC_BANK3)
#define MRA64_BANK4				((read64_handler)STATIC_BANK4)
#define MRA64_BANK5				((read64_handler)STATIC_BANK5)
#define MRA64_BANK6				((read64_handler)STATIC_BANK6)
#define MRA64_BANK7				((read64_handler)STATIC_BANK7)
#define MRA64_BANK8				((read64_handler)STATIC_BANK8)
#define MRA64_BANK9				((read64_handler)STATIC_BANK9)
#define MRA64_BANK10			((read64_handler)STATIC_BANK10)
#define MRA64_BANK11			((read64_handler)STATIC_BANK11)
#define MRA64_BANK12			((read64_handler)STATIC_BANK12)
#define MRA64_BANK13			((read64_handler)STATIC_BANK13)
#define MRA64_BANK14			((read64_handler)STATIC_BANK14)
#define MRA64_BANK15			((read64_handler)STATIC_BANK15)
#define MRA64_BANK16			((read64_handler)STATIC_BANK16)
#define MRA64_BANK17			((read64_handler)STATIC_BANK17)
#define MRA64_BANK18			((read64_handler)STATIC_BANK18)
#define MRA64_BANK19			((read64_handler)STATIC_BANK19)
#define MRA64_BANK20			((read64_handler)STATIC_BANK20)
#define MRA64_BANK21			((read64_handler)STATIC_BANK21)
#define MRA64_BANK22			((read64_handler)STATIC_BANK22)
#define MRA64_BANK23			((read64_handler)STATIC_BANK23)
#define MRA64_BANK24			((read64_handler)STATIC_BANK24)
#define MRA64_BANK25			((read64_handler)STATIC_BANK25)
#define MRA64_BANK26			((read64_handler)STATIC_BANK26)
#define MRA64_BANK27			((read64_handler)STATIC_BANK27)
#define MRA64_BANK28			((read64_handler)STATIC_BANK28)
#define MRA64_BANK29			((read64_handler)STATIC_BANK29)
#define MRA64_BANK30			((read64_handler)STATIC_BANK30)
#define MRA64_BANK31			((read64_handler)STATIC_BANK31)
#define MRA64_BANK32			((read64_handler)STATIC_BANK32)
#define MRA64_RAM				((read64_handler)STATIC_RAM)
#define MRA64_ROM				((read64_handler)STATIC_ROM)
#define MRA64_NOP				((read64_handler)STATIC_NOP)
#define MRA64_UNMAP				((read64_handler)STATIC_UNMAP)

/* 64-bit writes */
#define MWA64_BANK1				((write64_handler)STATIC_BANK1)
#define MWA64_BANK2				((write64_handler)STATIC_BANK2)
#define MWA64_BANK3				((write64_handler)STATIC_BANK3)
#define MWA64_BANK4				((write64_handler)STATIC_BANK4)
#define MWA64_BANK5				((write64_handler)STATIC_BANK5)
#define MWA64_BANK6				((write64_handler)STATIC_BANK6)
#define MWA64_BANK7				((write64_handler)STATIC_BANK7)
#define MWA64_BANK8				((write64_handler)STATIC_BANK8)
#define MWA64_BANK9				((write64_handler)STATIC_BANK9)
#define MWA64_BANK10			((write64_handler)STATIC_BANK10)
#define MWA64_BANK11			((write64_handler)STATIC_BANK11)
#define MWA64_BANK12			((write64_handler)STATIC_BANK12)
#define MWA64_BANK13			((write64_handler)STATIC_BANK13)
#define MWA64_BANK14			((write64_handler)STATIC_BANK14)
#define MWA64_BANK15			((write64_handler)STATIC_BANK15)
#define MWA64_BANK16			((write64_handler)STATIC_BANK16)
#define MWA64_BANK17			((write64_handler)STATIC_BANK17)
#define MWA64_BANK18			((write64_handler)STATIC_BANK18)
#define MWA64_BANK19			((write64_handler)STATIC_BANK19)
#define MWA64_BANK20			((write64_handler)STATIC_BANK20)
#define MWA64_BANK21			((write64_handler)STATIC_BANK21)
#define MWA64_BANK22			((write64_handler)STATIC_BANK22)
#define MWA64_BANK23			((write64_handler)STATIC_BANK23)
#define MWA64_BANK24			((write64_handler)STATIC_BANK24)
#define MWA64_BANK25			((write64_handler)STATIC_BANK25)
#define MWA64_BANK26			((write64_handler)STATIC_BANK26)
#define MWA64_BANK27			((write64_handler)STATIC_BANK27)
#define MWA64_BANK28			((write64_handler)STATIC_BANK28)
#define MWA64_BANK29			((write64_handler)STATIC_BANK29)
#define MWA64_BANK30			((write64_handler)STATIC_BANK30)
#define MWA64_BANK31			((write64_handler)STATIC_BANK31)
#define MWA64_BANK32			((write64_handler)STATIC_BANK32)
#define MWA64_RAM				((write64_handler)STATIC_RAM)
#define MWA64_ROM				((write64_handler)STATIC_ROM)
#define MWA64_NOP				((write64_handler)STATIC_NOP)
#define MWA64_UNMAP				((write64_handler)STATIC_UNMAP)



/***************************************************************************

    Address space array type definitions

    Note that the data access handlers are not passed the actual address
    where the operation takes place, but the offset from the beginning
    of the block they are assigned to. This makes handling of mirror
    addresses easier, and makes the handlers a bit more "object oriented".
    If you handler needs to read/write the main memory area, provide a
    "base" pointer: it will be initialized by the main engine to point to
    the beginning of the memory block assigned to the handler. You may
    also provide a pointer to "size": it will be set to the length of
    the memory area processed by the handler.

***************************************************************************/

typedef struct _handler_data handler_data;

/* ----- a union of all the different read handler types ----- */
union _read_handlers
{
	genf *				handler;
	read8_handler		handler8;
	read16_handler		handler16;
	read32_handler		handler32;
	read64_handler		handler64;
};
typedef union _read_handlers read_handlers;

/* ----- a union of all the different write handler types ----- */
union _write_handlers
{
	genf *				handler;
	write8_handler		handler8;
	write16_handler		handler16;
	write32_handler		handler32;
	write64_handler		handler64;
};
typedef union _write_handlers write_handlers;

/* ----- a generic address map type ----- */
struct _address_map
{
	UINT32				flags;				/* flags and additional info about this entry */
	offs_t				start, end;			/* start/end (or mask/match) values */
	offs_t				mirror;				/* mirror bits */
	offs_t				mask;				/* mask bits */
	read_handlers 		read;				/* read handler callback */
	const char *		read_name;			/* read handler callback name */
	write_handlers 		write;				/* write handler callback */
	const char *		write_name;			/* write handler callback name */
	void *				memory;				/* pointer to memory backing this entry */
	UINT32				share;				/* index of a shared memory block */
	void **				base;				/* receives pointer to memory (optional) */
	size_t *			size;				/* receives size of area in bytes (optional) */
	UINT32				region;				/* region containing the memory backing this entry */
	offs_t				region_offs;		/* offset within the region */
};
typedef struct _address_map address_map;

/* ----- structs to contain internal data ----- */
struct _address_space
{
	offs_t				addrmask;			/* address mask */
	UINT8 *				readlookup;			/* read table lookup */
	UINT8 *				writelookup;		/* write table lookup */
	handler_data *		readhandlers;		/* read handlers */
	handler_data *		writehandlers;		/* write handlers */
	data_accessors *	accessors;			/* pointers to the data access handlers */
};
typedef struct _address_space address_space;



/***************************************************************************

    Address map array constructors

***************************************************************************/

/* ----- a typedef for pointers to these functions ----- */
typedef address_map *(*construct_map_t)(address_map *map);

/* use this to declare external references to a machine driver */
#define ADDRESS_MAP_EXTERN(_name)										\
address_map *construct_map_##_name(address_map *map)					\

/* ----- macros for starting, ending, and setting map flags ----- */
#define ADDRESS_MAP_START(_name,_space,_bits)							\
address_map *construct_map_##_name(address_map *map)					\
{																		\
	typedef read##_bits##_handler _rh_t;								\
	typedef write##_bits##_handler _wh_t;								\
	_rh_t read;															\
	_wh_t write;														\
	UINT##_bits **base;													\
																		\
	(void)read; (void)write; (void)base;								\
	map->flags = AM_FLAGS_EXTENDED;										\
	map->start = AMEF_DBITS(_bits) | AMEF_SPACE(_space);				\

#define ADDRESS_MAP_FLAGS(_flags)										\
	map++;																\
	map->flags = AM_FLAGS_EXTENDED;										\
	map->start = (_flags);												\

#define ADDRESS_MAP_END													\
	map++;																\
	map->flags = AM_FLAGS_END;											\
	return map;															\
}																		\

/* ----- each map entry begins with one of these ----- */
#define AM_RANGE(_start,_end)											\
	map++;																\
	map->flags = 0;														\
	map->start = (_start);												\
	map->end = (_end);													\

#define AM_SPACE(_match,_mask)											\
	map++;																\
	map->flags = AM_FLAGS_MATCH_MASK;									\
	map->start = (_match);												\
	map->end = (_mask);													\

/* ----- these are optional entries after each map entry ----- */
#define AM_MASK(_mask)													\
	map->mask = (_mask);												\

#define AM_MIRROR(_mirror)												\
	map->mirror = (_mirror);											\

#define AM_READ(_handler)												\
	map->read.handler = (genf *)(read = _handler);						\
	map->read_name = #_handler;											\

#define AM_WRITE(_handler)												\
	map->write.handler = (genf *)(write = _handler);					\
	map->write_name = #_handler;										\

#define AM_REGION(_region, _offs)										\
	map->region = (_region);											\
	map->region_offs = (_offs);											\

#define AM_SHARE(_index)												\
	map->share = _index;												\

#define AM_BASE(_base)													\
	map->base = (void **)(base = _base);								\

#define AM_SIZE(_size)													\
	map->size = _size;													\

/* ----- common shortcuts ----- */
#define AM_READWRITE(_read,_write)			AM_READ(_read) AM_WRITE(_write)
#define AM_ROM								AM_READ((_rh_t)STATIC_ROM)
#define AM_RAM								AM_READWRITE((_rh_t)STATIC_RAM, (_wh_t)STATIC_RAM)
#define AM_UNMAP							AM_READWRITE((_rh_t)STATIC_UNMAP, (_wh_t)STATIC_UNMAP)
#define AM_ROMBANK(_bank)					AM_READ((_rh_t)(STATIC_BANK1 + (_bank) - 1))
#define AM_RAMBANK(_bank)					AM_READWRITE((_rh_t)(STATIC_BANK1 + (_bank) - 1), (_wh_t)(STATIC_BANK1 + (_bank) - 1))
#define AM_NOP								AM_READWRITE((_rh_t)STATIC_NOP, (_wh_t)STATIC_NOP)
#define AM_READNOP							AM_READ((_rh_t)STATIC_NOP)
#define AM_WRITENOP							AM_WRITE((_wh_t)STATIC_NOP)



/***************************************************************************

    Address map array helper macros

***************************************************************************/

/* ----- macros for identifying address map struct markers ----- */
#define IS_AMENTRY_EXTENDED(ma)				(((ma)->flags & AM_FLAGS_EXTENDED) != 0)
#define IS_AMENTRY_MATCH_MASK(ma)			(((ma)->flags & AM_FLAGS_MATCH_MASK) != 0)
#define IS_AMENTRY_END(ma)					(((ma)->flags & AM_FLAGS_END) != 0)

#define AM_EXTENDED_FLAGS(ma)				((ma)->start)



/***************************************************************************

    Address map lookup constants

    These apply to values in the internal lookup table.

***************************************************************************/

/* ----- address spaces ----- */
#define ADDRESS_SPACES			3						/* maximum number of address spaces */
#define ADDRESS_SPACE_PROGRAM	0						/* program address space */
#define ADDRESS_SPACE_DATA		1						/* data address space */
#define ADDRESS_SPACE_IO		2						/* I/O address space */

extern const char *address_space_names[ADDRESS_SPACES];

/* ----- address map lookup table definitions ----- */
#define SUBTABLE_COUNT			64						/* number of slots reserved for subtables */
#define SUBTABLE_BASE			(256-SUBTABLE_COUNT)	/* first index of a subtable */
#define ENTRY_COUNT				(SUBTABLE_BASE)			/* number of legitimate (non-subtable) entries */
#define SUBTABLE_ALLOC			8						/* number of subtables to allocate at a time */

/* ----- bit counts ----- */
#define LEVEL1_BITS				18						/* number of address bits in the level 1 table */
#define LEVEL2_BITS				(32 - LEVEL1_BITS)		/* number of address bits in the level 2 table */

/* ----- other address map constants ----- */
#define MAX_ADDRESS_MAP_SIZE	256						/* maximum entries in an address map */
#define MAX_MEMORY_BLOCKS		1024					/* maximum memory blocks we can track */
#define MAX_SHARED_POINTERS		256						/* maximum number of shared pointers in memory maps */
#define MEMORY_BLOCK_SIZE		65536					/* size of allocated memory blocks */



/***************************************************************************

    Address map lookup macros

    These are used for accessing the internal lookup table.

***************************************************************************/

/* ----- table lookup helpers ----- */
#define LEVEL1_INDEX(a)			((a) >> LEVEL2_BITS)
#define LEVEL2_INDEX(e,a)		((1 << LEVEL1_BITS) + (((e) - SUBTABLE_BASE) << LEVEL2_BITS) + ((a) & ((1 << LEVEL2_BITS) - 1)))



/***************************************************************************

    Function prototypes for core readmem/writemem routines

***************************************************************************/

/* ----- declare program address space handlers ----- */
UINT8 program_read_byte_8(offs_t address);
void program_write_byte_8(offs_t address, UINT8 data);

UINT8 program_read_byte_16be(offs_t address);
UINT16 program_read_word_16be(offs_t address);
void program_write_byte_16be(offs_t address, UINT8 data);
void program_write_word_16be(offs_t address, UINT16 data);

UINT8 program_read_byte_16le(offs_t address);
UINT16 program_read_word_16le(offs_t address);
void program_write_byte_16le(offs_t address, UINT8 data);
void program_write_word_16le(offs_t address, UINT16 data);

UINT8 program_read_byte_32be(offs_t address);
UINT16 program_read_word_32be(offs_t address);
UINT32 program_read_dword_32be(offs_t address);
void program_write_byte_32be(offs_t address, UINT8 data);
void program_write_word_32be(offs_t address, UINT16 data);
void program_write_dword_32be(offs_t address, UINT32 data);

UINT8 program_read_byte_32le(offs_t address);
UINT16 program_read_word_32le(offs_t address);
UINT32 program_read_dword_32le(offs_t address);
void program_write_byte_32le(offs_t address, UINT8 data);
void program_write_word_32le(offs_t address, UINT16 data);
void program_write_dword_32le(offs_t address, UINT32 data);

UINT8 program_read_byte_64be(offs_t address);
UINT16 program_read_word_64be(offs_t address);
UINT32 program_read_dword_64be(offs_t address);
UINT64 program_read_qword_64be(offs_t address);
void program_write_byte_64be(offs_t address, UINT8 data);
void program_write_word_64be(offs_t address, UINT16 data);
void program_write_dword_64be(offs_t address, UINT32 data);
void program_write_qword_64be(offs_t address, UINT64 data);

UINT8 program_read_byte_64le(offs_t address);
UINT16 program_read_word_64le(offs_t address);
UINT32 program_read_dword_64le(offs_t address);
UINT64 program_read_qword_64le(offs_t address);
void program_write_byte_64le(offs_t address, UINT8 data);
void program_write_word_64le(offs_t address, UINT16 data);
void program_write_dword_64le(offs_t address, UINT32 data);
void program_write_qword_64le(offs_t address, UINT64 data);

/* ----- declare data address space handlers ----- */
UINT8 data_read_byte_8(offs_t address);
void data_write_byte_8(offs_t address, UINT8 data);

UINT8 data_read_byte_16be(offs_t address);
UINT16 data_read_word_16be(offs_t address);
void data_write_byte_16be(offs_t address, UINT8 data);
void data_write_word_16be(offs_t address, UINT16 data);

UINT8 data_read_byte_16le(offs_t address);
UINT16 data_read_word_16le(offs_t address);
void data_write_byte_16le(offs_t address, UINT8 data);
void data_write_word_16le(offs_t address, UINT16 data);

UINT8 data_read_byte_32be(offs_t address);
UINT16 data_read_word_32be(offs_t address);
UINT32 data_read_dword_32be(offs_t address);
void data_write_byte_32be(offs_t address, UINT8 data);
void data_write_word_32be(offs_t address, UINT16 data);
void data_write_dword_32be(offs_t address, UINT32 data);

UINT8 data_read_byte_32le(offs_t address);
UINT16 data_read_word_32le(offs_t address);
UINT32 data_read_dword_32le(offs_t address);
void data_write_byte_32le(offs_t address, UINT8 data);
void data_write_word_32le(offs_t address, UINT16 data);
void data_write_dword_32le(offs_t address, UINT32 data);

UINT8 data_read_byte_64be(offs_t address);
UINT16 data_read_word_64be(offs_t address);
UINT32 data_read_dword_64be(offs_t address);
UINT64 data_read_qword_64be(offs_t address);
void data_write_byte_64be(offs_t address, UINT8 data);
void data_write_word_64be(offs_t address, UINT16 data);
void data_write_dword_64be(offs_t address, UINT32 data);
void data_write_qword_64be(offs_t address, UINT64 data);

UINT8 data_read_byte_64le(offs_t address);
UINT16 data_read_word_64le(offs_t address);
UINT32 data_read_dword_64le(offs_t address);
UINT64 data_read_qword_64le(offs_t address);
void data_write_byte_64le(offs_t address, UINT8 data);
void data_write_word_64le(offs_t address, UINT16 data);
void data_write_dword_64le(offs_t address, UINT32 data);
void data_write_qword_64le(offs_t address, UINT64 data);

/* ----- declare I/O address space handlers ----- */
UINT8 io_read_byte_8(offs_t address);
void io_write_byte_8(offs_t address, UINT8 data);

UINT8 io_read_byte_16be(offs_t address);
UINT16 io_read_word_16be(offs_t address);
void io_write_byte_16be(offs_t address, UINT8 data);
void io_write_word_16be(offs_t address, UINT16 data);

UINT8 io_read_byte_16le(offs_t address);
UINT16 io_read_word_16le(offs_t address);
void io_write_byte_16le(offs_t address, UINT8 data);
void io_write_word_16le(offs_t address, UINT16 data);

UINT8 io_read_byte_32be(offs_t address);
UINT16 io_read_word_32be(offs_t address);
UINT32 io_read_dword_32be(offs_t address);
void io_write_byte_32be(offs_t address, UINT8 data);
void io_write_word_32be(offs_t address, UINT16 data);
void io_write_dword_32be(offs_t address, UINT32 data);

UINT8 io_read_byte_32le(offs_t address);
UINT16 io_read_word_32le(offs_t address);
UINT32 io_read_dword_32le(offs_t address);
void io_write_byte_32le(offs_t address, UINT8 data);
void io_write_word_32le(offs_t address, UINT16 data);
void io_write_dword_32le(offs_t address, UINT32 data);

UINT8 io_read_byte_64be(offs_t address);
UINT16 io_read_word_64be(offs_t address);
UINT32 io_read_dword_64be(offs_t address);
UINT64 io_read_qword_64be(offs_t address);
void io_write_byte_64be(offs_t address, UINT8 data);
void io_write_word_64be(offs_t address, UINT16 data);
void io_write_dword_64be(offs_t address, UINT32 data);
void io_write_qword_64be(offs_t address, UINT64 data);

UINT8 io_read_byte_64le(offs_t address);
UINT16 io_read_word_64le(offs_t address);
UINT32 io_read_dword_64le(offs_t address);
UINT64 io_read_qword_64le(offs_t address);
void io_write_byte_64le(offs_t address, UINT8 data);
void io_write_word_64le(offs_t address, UINT16 data);
void io_write_dword_64le(offs_t address, UINT32 data);
void io_write_qword_64le(offs_t address, UINT64 data);



/***************************************************************************

    Function prototypes for core memory functions

***************************************************************************/

/* ----- memory setup function ----- */
int			memory_init(void);
void		memory_exit(void);
void		memory_set_context(int activecpu);

/* ----- address map functions ----- */
const address_map *memory_get_map(int cpunum, int spacenum);

/* ----- opcode base control ---- */
opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function);
void		memory_set_opbase(offs_t offset);

/* ----- separate opcode/data encryption helper ---- */
void 		memory_set_decrypted_region(int cpunum, offs_t start, offs_t end, void *base);

/* ----- return a base pointer to memory ---- */
void *		memory_get_read_ptr(int cpunum, int spacenum, offs_t offset);
void *		memory_get_write_ptr(int cpunum, int spacenum, offs_t offset);
void *		memory_get_op_ptr(int cpunum, offs_t offset, int arg);

/* ----- memory banking ----- */
void		memory_configure_bank(int banknum, int startentry, int numentries, void *base, offs_t stride);
void		memory_configure_bank_decrypted(int banknum, int startentry, int numentries, void *base, offs_t stride);
void		memory_set_bank(int banknum, int entrynum);
void		memory_set_bankptr(int banknum, void *base);

/* ----- debugging ----- */
void		memory_set_debugger_access(int debugger);
void		memory_set_log_unmap(int spacenum, int log);
int			memory_get_log_unmap(int spacenum);

/* ----- dynamic address space mapping ----- */
void *		_memory_install_read_handler   (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, int handler, const char *handler_name);
UINT8 *		_memory_install_read8_handler  (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_handler handler, const char *handler_name);
UINT16 *	_memory_install_read16_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read16_handler handler, const char *handler_name);
UINT32 *	_memory_install_read32_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read32_handler handler, const char *handler_name);
UINT64 *	_memory_install_read64_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, read64_handler handler, const char *handler_name);
void *		_memory_install_write_handler  (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, int handler, const char *handler_name);
UINT8 *		_memory_install_write8_handler (int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write8_handler handler, const char *handler_name);
UINT16 *	_memory_install_write16_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write16_handler handler, const char *handler_name);
UINT32 *	_memory_install_write32_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write32_handler handler, const char *handler_name);
UINT64 *	_memory_install_write64_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t mask, offs_t mirror, write64_handler handler, const char *handler_name);

void *		_memory_install_read_matchmask_handler   (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, int handler, const char *handler_name);
UINT8 *		_memory_install_read8_matchmask_handler  (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read8_handler handler, const char *handler_name);
UINT16 *	_memory_install_read16_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read16_handler handler, const char *handler_name);
UINT32 *	_memory_install_read32_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read32_handler handler, const char *handler_name);
UINT64 *	_memory_install_read64_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, read64_handler handler, const char *handler_name);
void *		_memory_install_write_matchmask_handler  (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, int handler, const char *handler_name);
UINT8 *		_memory_install_write8_matchmask_handler (int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write8_handler handler, const char *handler_name);
UINT16 *	_memory_install_write16_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write16_handler handler, const char *handler_name);
UINT32 *	_memory_install_write32_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write32_handler handler, const char *handler_name);
UINT64 *	_memory_install_write64_matchmask_handler(int cpunum, int spacenum, offs_t matchval, offs_t maskval, offs_t mask, offs_t mirror, write64_handler handler, const char *handler_name);

/* ----- memory debugging ----- */
void 		memory_dump(FILE *file);
const char *memory_get_handler_string(int read0_or_write1, int cpunum, int spacenum, offs_t offset);



/***************************************************************************

    Global variables

***************************************************************************/

extern UINT8 			opcode_entry;				/* current entry for opcode fetching */
extern UINT8 *			opcode_base;				/* opcode ROM base */
extern UINT8 *			opcode_arg_base;			/* opcode RAM base */
extern offs_t			opcode_mask;				/* mask to apply to the opcode address */
extern offs_t			opcode_memory_min;			/* opcode memory minimum */
extern offs_t			opcode_memory_max;			/* opcode memory maximum */
extern address_space	active_address_space[];		/* address spaces */
extern address_map *	construct_map_0(address_map *map);



/***************************************************************************

    Helper macros and inlines

***************************************************************************/

/* ----- 16/32-bit memory accessing ----- */
#define COMBINE_DATA(varptr)		(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))

/* ----- 16-bit memory accessing ----- */
#define ACCESSING_LSB16				((mem_mask & 0x00ff) == 0)
#define ACCESSING_MSB16				((mem_mask & 0xff00) == 0)
#define ACCESSING_LSB				ACCESSING_LSB16
#define ACCESSING_MSB				ACCESSING_MSB16

/* ----- 32-bit memory accessing ----- */
#define ACCESSING_LSW32				((mem_mask & 0x0000ffff) == 0)
#define ACCESSING_MSW32				((mem_mask & 0xffff0000) == 0)
#define ACCESSING_LSB32				((mem_mask & 0x000000ff) == 0)
#define ACCESSING_MSB32				((mem_mask & 0xff000000) == 0)

/* ----- opcode range safety checks ----- */
#if CPUREADOP_SAFETY_NONE
#define address_is_unsafe(A)		(0)
#elif CPUREADOP_SAFETY_PARTIAL
#define address_is_unsafe(A)		(UNEXPECTED((A) > opcode_memory_max))
#elif CPUREADOP_SAFETY_FULL
#define address_is_unsafe(A)		((UNEXPECTED((A) < opcode_memory_min) || UNEXPECTED((A) > opcode_memory_max)))
#else
#error Must set either CPUREADOP_SAFETY_NONE, CPUREADOP_SAFETY_PARTIAL or CPUREADOP_SAFETY_FULL
#endif

/* ----- dynamic memory installation ----- */
#define memory_install_read_handler(cpu, space, start, end, mask, mirror, handler)				\
	_memory_install_read_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read8_handler(cpu, space, start, end, mask, mirror, handler)				\
	_memory_install_read8_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read16_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read16_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read32_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read32_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read64_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read64_handler(cpu, space, start, end, mask, mirror, handler, #handler)

#define memory_install_write_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write8_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write8_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write16_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write16_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write32_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write32_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write64_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write64_handler(cpu, space, start, end, mask, mirror, handler, #handler)

#define memory_install_read_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read8_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read8_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read16_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read16_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read32_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read32_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_read64_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_read64_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)

#define memory_install_write_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write8_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write8_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write16_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write16_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write32_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write32_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)
#define memory_install_write64_matchmask_handler(cpu, space, start, end, mask, mirror, handler)			\
	_memory_install_write64_matchmask_handler(cpu, space, start, end, mask, mirror, handler, #handler)

/* ----- generic memory access ----- */
INLINE UINT8  program_read_byte (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_byte)(offset); }
INLINE UINT16 program_read_word (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_word)(offset); }
INLINE UINT32 program_read_dword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_dword)(offset); }
INLINE UINT64 program_read_qword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_qword)(offset); }

INLINE void	program_write_byte (offs_t offset, UINT8  data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_byte)(offset, data); }
INLINE void	program_write_word (offs_t offset, UINT16 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_word)(offset, data); }
INLINE void	program_write_dword(offs_t offset, UINT32 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_dword)(offset, data); }
INLINE void	program_write_qword(offs_t offset, UINT64 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_qword)(offset, data); }

INLINE UINT8  data_read_byte (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_byte)(offset); }
INLINE UINT16 data_read_word (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_word)(offset); }
INLINE UINT32 data_read_dword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_dword)(offset); }
INLINE UINT64 data_read_qword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_qword)(offset); }

INLINE void	data_write_byte (offs_t offset, UINT8  data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_byte)(offset, data); }
INLINE void	data_write_word (offs_t offset, UINT16 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_word)(offset, data); }
INLINE void	data_write_dword(offs_t offset, UINT32 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_dword)(offset, data); }
INLINE void	data_write_qword(offs_t offset, UINT64 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_qword)(offset, data); }

INLINE UINT8  io_read_byte (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_byte)(offset); }
INLINE UINT16 io_read_word (offs_t offset) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_word)(offset); }
INLINE UINT32 io_read_dword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_dword)(offset); }
INLINE UINT64 io_read_qword(offs_t offset) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_qword)(offset); }

INLINE void	io_write_byte (offs_t offset, UINT8  data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_byte)(offset, data); }
INLINE void	io_write_word (offs_t offset, UINT16 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_word)(offset, data); }
INLINE void	io_write_dword(offs_t offset, UINT32 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_dword)(offset, data); }
INLINE void	io_write_qword(offs_t offset, UINT64 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_qword)(offset, data); }

/* ----- safe opcode and opcode argument reading ----- */
UINT8	cpu_readop_safe(offs_t offset);
UINT16	cpu_readop16_safe(offs_t offset);
UINT32	cpu_readop32_safe(offs_t offset);
UINT64	cpu_readop64_safe(offs_t offset);
UINT8	cpu_readop_arg_safe(offs_t offset);
UINT16	cpu_readop_arg16_safe(offs_t offset);
UINT32	cpu_readop_arg32_safe(offs_t offset);
UINT64	cpu_readop_arg64_safe(offs_t offset);

/* ----- unsafe opcode and opcode argument reading ----- */
#define cpu_opptr_unsafe(A)			((void *)&opcode_base[(A) & opcode_mask])
#define cpu_readop_unsafe(A)		(opcode_base[(A) & opcode_mask])
#define cpu_readop16_unsafe(A)		(*(UINT16 *)&opcode_base[(A) & opcode_mask])
#define cpu_readop32_unsafe(A)		(*(UINT32 *)&opcode_base[(A) & opcode_mask])
#define cpu_readop64_unsafe(A)		(*(UINT64 *)&opcode_base[(A) & opcode_mask])
#define cpu_readop_arg_unsafe(A)	(opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg16_unsafe(A)	(*(UINT16 *)&opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg32_unsafe(A)	(*(UINT32 *)&opcode_arg_base[(A) & opcode_mask])
#define cpu_readop_arg64_unsafe(A)	(*(UINT64 *)&opcode_arg_base[(A) & opcode_mask])

/* ----- opcode and opcode argument reading ----- */
INLINE void * cpu_opptr(offs_t A)			{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_opptr_unsafe(A); }
INLINE UINT8  cpu_readop(offs_t A)			{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_unsafe(A); }
INLINE UINT16 cpu_readop16(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop16_unsafe(A); }
INLINE UINT32 cpu_readop32(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop32_unsafe(A); }
INLINE UINT64 cpu_readop64(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop64_unsafe(A); }
INLINE UINT8  cpu_readop_arg(offs_t A)		{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg_unsafe(A); }
INLINE UINT16 cpu_readop_arg16(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg16_unsafe(A); }
INLINE UINT32 cpu_readop_arg32(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg32_unsafe(A); }
INLINE UINT64 cpu_readop_arg64(offs_t A)	{ if (address_is_unsafe(A)) { memory_set_opbase(A); } return cpu_readop_arg64_unsafe(A); }

/* ----- bank switching for CPU cores ----- */
#define change_pc(pc)																	\
do {																					\
	if (active_address_space[ADDRESS_SPACE_PROGRAM].readlookup[LEVEL1_INDEX((pc) & active_address_space[ADDRESS_SPACE_PROGRAM].addrmask)] != opcode_entry)	\
		memory_set_opbase(pc);															\
} while (0)																				\

/* ----- forces the next branch to generate a call to the opbase handler ----- */
#define catch_nextBranch()			(opcode_entry = 0xff)


#endif	/* __MEMORY_H__ */
