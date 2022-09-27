/********************************************************************
 *     DEVLOD.C - Jim Kyle - 08/20/90                               *
 *            Copyright 1990 by Jim Kyle - All Rights Reserved      *
 *     (minor revisions by Andrew Schulman - 9/12/90                *
 *     Dynamic loader for device drivers                            *
 *            Requires Turbo C; see DEVLOD.MAK also for ASM helpers.*
 ********************************************************************/
     
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

typedef unsigned char BYTE;

#define GETFLAGS __emit__(0x9F)     /* if any error, quit right now */
#define FIXDS    __emit__(0x16,0x1F)/* PUSH SS, POP DS              */
#define PUSH_BP  __emit__(0x55)
#define POP_BP   __emit__(0x5D)

unsigned _stklen = 0x200;
unsigned _heaplen = 0;

char FileName[65];  /* filename global buffer                       */
char * dvrarg;      /* points to char after name in cmdline buffer  */
unsigned movsize;   /* number of bytes to be moved up for driver    */
void (far * driver)(void);  /* used as pointer to call driver code      */
void far * drvptr;  /* holds pointer to device driver               */
void far * nuldrvr; /* additional driver pointers                   */
void far * nxtdrvr;
BYTE far * nblkdrs; /* points to block device count in List of Lists*/
unsigned lastdrive; /* value of LASTDRIVE in List of Lists          */
BYTE far * CDSbase; /* base of Current Dir Structure                */
int CDSsize;        /* size of CDS element                          */
unsigned nulseg;    /* hold parts of ListOfLists pointer            */
unsigned nulofs;
unsigned LoLofs;

#pragma pack(1)

struct packet{      /* device driver's command packet               */
    BYTE hdrlen;
    BYTE unit;
    BYTE command;       /* 0 to initialize      */
    unsigned status;    /* 0x8000 is error      */
    BYTE reserv[8];
    BYTE nunits;
    unsigned brkofs;    /* break adr on return  */
    unsigned brkseg;    /* break seg on return  */
    unsigned inpofs;    /* SI on input          */
    unsigned inpseg;    /* _psp on input        */
    BYTE NextDrv;       /* next available drive */
  } CmdPkt;
  
typedef struct {    /* Current Directory Structure (CDS)            */
    BYTE path[0x43];
    unsigned flags;
    void far *dpb;
    unsigned start_cluster;
    unsigned long ffff;
    unsigned slash_offset;  /* offset of '\' in current path field  */
    // next for DOS4+ only
    BYTE unknown;
    void far *ifs;
    unsigned unknown2;
    } CDS;

extern unsigned _psp;       /* established by startup code in c0    */
extern unsigned _heaptop;   /* established by startup code in c0    */
extern BYTE _osmajor;       /* established by startup code      */
extern BYTE _osminor;       /* established by startup code      */

void _exit( int );          /* established by startup code in c0    */
void abort( void );         /* established by startup code in c0    */

void movup( char far *, char far *, int ); /* in MOVUP.ASM file     */
void copyptr( void far *src, void far *dst ); /* in MOVUP.ASM file  */

void exit(int c)            /* called by startup code's sequence    */
{ _exit(c);}

int Get_Driver_Name ( void )
{ char *nameptr;
  int i, j, cmdlinesz;

  nameptr = (char *)0x80;   /* check command line for driver name   */
  cmdlinesz = (unsigned)*nameptr++;
  if (cmdlinesz < 1)        /* if nothing there, return FALSE       */
    return 0;
  for (i=0; i<cmdlinesz && nameptr[i]<'!'; i++) /* skip blanks      */
    ;
  dvrarg = (char *)&nameptr[i]; /* save to put in SI                */
  for ( j=0; i<cmdlinesz && nameptr[i]>' '; i++)    /* copy name    */
    FileName[j++] = nameptr[i];
  FileName[j] = '\0';

  return 1;                 /* and return TRUE to keep going        */
}

void Put_Msg ( char *msg )  /* replaces printf()                    */
{ 
#ifdef INT29
    /* gratuitous use of undocumented DOS */
    while (*msg)
    { _AL = *msg++;             /* MOV AL,*msg  */
      geninterrupt(0x29);       /* INT 29h */
    }
#else
    _AH = 2;    /* doesn't need to be inside loop */
    while (*msg)
    { _DL = *msg++;            
      geninterrupt(0x21);
    }
#endif
}

void Err_Halt ( char *msg )     /* print message and abort          */
{ Put_Msg ( msg );
  Put_Msg ( "\r\n" );           /* send CR,LF   */
  abort();
}

void Move_Loader ( void )       /* vacate lower part of RAM         */
{
    unsigned movsize, destseg;
    movsize = _heaptop - _psp; /* size of loader in paragraphs      */
    destseg = *(unsigned far *)MK_FP( _psp, 2 ); /* end of memory   */
    movup ( MK_FP( _psp, 0 ), MK_FP( destseg - movsize, 0 ),
            movsize << 4);      /* move and fix segregs             */
}

void Load_Drvr ( void )         /* load driver file into RAM    */
{ unsigned handle;
  struct {
    unsigned LoadSeg;
    unsigned RelocSeg;
  } ExecBlock;

  ExecBlock.LoadSeg = _psp + 0x10;
  ExecBlock.RelocSeg = _psp + 0x10;
  _DX = (unsigned)&FileName[0];
  _BX = (unsigned)&ExecBlock;
  _ES = _SS;                    /* es:bx point to ExecBlock     */
  _AX = 0x4B03;                 /* load overlay                 */
  geninterrupt ( 0x21 );        /* DS is okay on this call      */
  GETFLAGS;
  if ( _AH & 1 )
    Err_Halt ( "Unable to load driver file." );
}

void Get_List ( void )          /* set up pointers via List     */
{ _AH = 0x52;                   /* find DOS List of Lists       */
  geninterrupt ( 0x21 );
  nulseg = _ES;                 /* DOS data segment             */
  LoLofs = _BX;                 /* current drive table offset   */

  switch( _osmajor )            /* NUL adr varies with version  */
    {
    case  0:
      Err_Halt ( "Drivers not used in DOS V1." );
    case  2:
      nblkdrs = NULL;
      nulofs = LoLofs + 0x17;
      break;
    case  3:
      if (_osminor == 0)
      {
          nblkdrs = (BYTE far *) MK_FP(nulseg, LoLofs + 0x10);
          lastdrive = *((BYTE far *) MK_FP(nulseg, LoLofs + 0x1b));
          nulofs = LoLofs + 0x28;
      }
      else
      {
          nblkdrs = (BYTE far *) MK_FP(nulseg, LoLofs + 0x20);
          lastdrive = *((BYTE far *) MK_FP(nulseg, LoLofs + 0x21));
          nulofs = LoLofs + 0x22;
      }
      CDSbase = *(BYTE far * far *)MK_FP(nulseg, LoLofs + 0x16);
      CDSsize = 81;
      break;
    case  4:
    case  5:
    case  6:
    case  7:
      nblkdrs = (BYTE far *) MK_FP(nulseg, LoLofs + 0x20);
      lastdrive = *((BYTE far *) MK_FP(nulseg, LoLofs + 0x21));
      nulofs  = LoLofs + 0x22;
      CDSbase = *(BYTE far * far *) MK_FP(nulseg, LoLofs + 0x16);
      CDSsize = 88;
      break;
    case 10:
    case 20:
      Err_Halt ( "OS2 DOS Box not supported." );
    default:
      Err_Halt ( "Unknown version of DOS!");
    }
}

void Fix_DOS_Chain ( void )     /* patches driver into DOS chn  */
{ unsigned i;

  nuldrvr = MK_FP( nulseg, nulofs+0x0A ); /* verify the drvr    */
  drvptr = "NUL     ";
  for ( i=0; i<8; ++i )
    if ( *((BYTE far *)nuldrvr+i) != *((BYTE far *)drvptr+i) )
      Err_Halt ( "Failed to find NUL driver." );

  nuldrvr = MK_FP( nulseg, nulofs );    /* point to NUL driver  */
  drvptr  = MK_FP( _psp+0x10, 0 );      /* new driver's address */

  copyptr ( nuldrvr, &nxtdrvr );        /* hold old head now    */
  copyptr ( &drvptr, nuldrvr );         /* put new after NUL    */
  copyptr ( &nxtdrvr, drvptr );         /* and old after new    */
}

// returns number of next free drive, -1 if none available
int Next_Drive ( void )
{
#ifdef USE_BLKDEV
  return (nblkdrs && (*nblkdrs < lastdrive)) ? *nblkdrs : -1;
#else
  /* The following approach takes account of SUBSTed and
     network-redirector drives */
  BYTE far *cds;
  int i;
  /* find first unused entry in CDS structure */
  for (i=0, cds=(BYTE far *)CDSbase; i<lastdrive; i++, cds+=CDSsize) {
    if (! ((CDS far *)cds)->flags)                /* found a free drive  */
        break;
  }
  return (i == lastdrive) ? -1 : i; 
#endif  
}

int Init_Drvr ( void )
{ unsigned tmp;
#define INIT 0
  CmdPkt.command = INIT;        /* build command packet         */
  CmdPkt.hdrlen = sizeof (struct packet);
  CmdPkt.unit = 0;
  CmdPkt.inpofs  = (unsigned)dvrarg;    /* points into cmd line */
  CmdPkt.inpseg  = _psp;
  /* can't really check for next drive here, because don't yet know
     if this is a block driver or not */
  CmdPkt.NextDrv = Next_Drive();
  drvptr  = MK_FP( _psp+0x10, 0 );      /* new driver's address */

  tmp = *((unsigned far *)drvptr+3);    /* STRATEGY pointer     */
  driver = MK_FP( FP_SEG( drvptr ), tmp );
  _ES = FP_SEG( (void far *)&CmdPkt );
  _BX = FP_OFF( (void far *)&CmdPkt );
  (*driver)();                  /* set up the packet address    */

  tmp = *((unsigned far *)drvptr+4);    /* COMMAND pointer      */
  driver = MK_FP( FP_SEG( drvptr ), tmp );
  (*driver)();                  /* do the initialization        */
  
  /* check status code in command packet                        */
  return (! ( CmdPkt.status & 0x8000 ));
}

int  Put_Blk_Dev ( void )   /* TRUE if Block Device failed      */
{ int newdrv;
  int retval = 1;           /* pre-set for failure              */
  int unit = 0;
  BYTE far *DPBlink;
  CDS far *cds;
  int i;

  if ((Next_Drive() == -1) || CmdPkt.nunits == 0)
    return retval;          /* cannot install block driver      */
  if (CmdPkt.brkofs != 0)   /* align to next paragraph          */
  {
    CmdPkt.brkseg += (CmdPkt.brkofs >> 4) + 1;
    CmdPkt.brkofs = 0;
  }
  while( CmdPkt.nunits-- )
  {
    if ((newdrv = Next_Drive()) == -1)
        return 1;
    (*nblkdrs)++;
    _AH = 0x32;             /* get last DPB and set poiner      */
    _DL = newdrv;
    geninterrupt ( 0x21 );
    _AX = _DS;              /* save segment to make the pointer */
    FIXDS;
    DPBlink = MK_FP(_AX, _BX);
    (unsigned) DPBlink += (_osmajor < 4 ? 24 : 25 );
    _SI = *(unsigned far *)MK_FP(CmdPkt.inpseg, CmdPkt.inpofs);
    _ES = CmdPkt.brkseg;
    _DS = CmdPkt.inpseg;
    _AH = 0x53;
    PUSH_BP;
    _BP = 0;
    geninterrupt ( 0x21 );    /* build the DPB for this unit    */
    POP_BP;
    FIXDS;
    *(void far * far *)DPBlink = MK_FP( CmdPkt.brkseg, 0 );

    /* set up the Current Directory Structure for this drive */
    cds = (CDS far *) (CDSbase + (newdrv * CDSsize));
    cds->flags = 1 << 14;       /* PHYSICAL DRIVE */
    cds->dpb = MK_FP(CmdPkt.brkseg, 0);
    cds->start_cluster = 0xFFFF;
    cds->ffff = -1L;
    cds->slash_offset = 2;
    if (_osmajor > 3)
      { cds->unknown = 0;
        cds->ifs = (void far *) 0;
        cds->unknown2 = 0;
      }
      
    /* set up pointers for DPB, driver  */
    DPBlink = MK_FP( CmdPkt.brkseg, 0);
    *DPBlink = newdrv;
    *(DPBlink+1) = unit++;
    if (_osmajor > 3)
      DPBlink++;          /* add one if DOS 4                 */
    *(long far *)(DPBlink+0x12) = (long)MK_FP( _psp+0x10, 0 );
    *(long far *)(DPBlink+0x18) = 0xFFFFFFFF;
    CmdPkt.brkseg += 2;       /* Leave two paragraphs for DPB   */
    CmdPkt.inpofs += 2;       /* Point to next BPB pointer      */
  }     /* end of nunits loop   */
  return 0;                 /* all went okay                    */
}

void Get_Out ( void )
{ unsigned temp;

  temp = *((unsigned far *)drvptr+2);   /* attribute word       */
  if ((temp & 0x8000) == 0 )    /* if block device, set up tbls */
    if (Put_Blk_Dev() )
      Err_Halt( "Could not install block device" );

  Fix_DOS_Chain ();             /* else patch it into DOS       */

  _ES = *((unsigned *)MK_FP( _psp, 0x002C ));
  _AH = 0x49;                   /* release environment space    */
  geninterrupt ( 0x21 );

  /* then set up regs for KEEP function, and go resident        */
  temp = (CmdPkt.brkofs + 15);  /* normalize the offset         */
  temp >>= 4;
  temp += CmdPkt.brkseg;        /* add the segment address      */
  temp -= _psp;                 /* convert to paragraph count   */
  _AX = 0x3100;                 /* KEEP function of DOS         */
  _DX = (unsigned)temp;         /* paragraphs to retain         */
  geninterrupt ( 0x21 );        /* won't come back from here!   */
}

void main ( void )
{ if (!Get_Driver_Name() )
    Err_Halt ( "Device driver name required.");
  Move_Loader ();               /* move code high and jump      */
  Load_Drvr ();                 /* bring driver into freed RAM  */
  Get_List();                   /* get DOS internal variables   */
  if (Init_Drvr ())             /* let driver do its thing      */
      Get_Out();                /* check init status, go TSR    */
  else
      Err_Halt ( "Driver initialization failed." );
}
