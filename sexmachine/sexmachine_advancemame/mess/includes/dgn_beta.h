#ifndef DGN_BETA
#define DGN_BETA

#define DGNBETA_CPU_SPEED_HZ		2000000	/* 2MHz */
#define DGNBETA_FRAMES_PER_SECOND	50

#define RamSize				256	/* 256K by default */
#define RamPageSize			4096	/* ram pages are 4096 bytes */

#define MaxTasks			16	/* Tasks 0..15 */
#define MaxPage				16	/* 16 4K pages */
#define NoPagingTask			MaxTasks	/* Task registers to use when paging disabled 16 */

#define RAMPage				0		/* Page with RAM in at power on */
#define VideoPage			6		/* Page where video ram mapped */
#define IOPage				MaxPage-1	/* Page for I/O */
#define ROMPage				MaxPage-2	/* Page for ROM */

#define RAMPageValue			0x00		/* page with RAM at power on */
#define VideoPageValue			0x1F		/* Default page for video ram */
#define NoMemPageValue			0xC0		/* Page garanteed not to have memory in */
#define ROMPageValue			0xFE		/* Page with boot ROM */
#define IOPageValue			0xFF		/* Page with I/O & Boot ROM */

/***** Keyboard stuff *****/
#define	NoKeyrows			0x0a		/* Number of rows in keyboard */

/* From Dragon Beta OS9 keyboard driver */
#define KAny				0x04		/* Any key pressed mask PB2 */
#define KOutClk				0x08		/* Ouput shift register clock */
#define KInClk				0x10		/* Input shift register clock */
#define KOutDat				KInClk		/* Also used for data into output shifter */
#define KInDat				0x20		/* Keyboard data in from keyboard (serial stream) */

/***** WD2797 pins *****/

#define DSMask				0x03		/* PA0 & PA1 are binary encoded drive */
#define ENPCtrl				0x20		/* PA5 on PIA */
#define DDenCtrl			0x40		/* PA6 on PIA */

	
MACHINE_START( dgnbeta );

// Page IO at FE00
READ8_HANDLER( dgn_beta_page_r );
WRITE8_HANDLER( dgn_beta_page_w );

// Ram banking handlers.
void dgnbeta_ram_b0_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b1_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b2_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b3_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b4_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b5_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b6_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b7_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b8_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b9_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bA_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bB_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bC_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bD_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bE_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bF_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bG_w(offs_t offset, UINT8 data);

/* mc6845 video display generator */
void init_video(void);
extern VIDEO_UPDATE( dgnbeta );
INTERRUPT_GEN( dgn_beta_frame_interrupt );

/*  WD2797 FDC */
READ8_HANDLER(dgnbeta_wd2797_r);
WRITE8_HANDLER(dgnbeta_wd2797_w);

extern int dgnbeta_font;


#define iosize	(0xfEFF-0xfc00)
extern UINT8 *ioarea;

#endif
