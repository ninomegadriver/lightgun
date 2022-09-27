/***************************************************************************

    Apache 3                                            ATF-011
    Round Up 5                                          ATC-011
    Cycle Warriors                                      ABA-011

    Incredibly complex hardware!  These are all different boards, but share
    a similar sprite chip (TZB215 on Apache 3, TZB315 on others).  Other
    graphics (road, sky, bg/fg layers) all differ between games.

    Todo:
        Sprite rotation
        Finish road layer (Round Up 5)
        Implement road layer (Apache 3, Cycle Warriors)
        BG layer(s) (Cycle Warriors)
        BG layer (Round Up 5) - May be driven by missing VRAM data
        Round Up 5 always boots with a coin inserted
        Round Up 5 doesn't survive a reset
        Dip switches
        Various other things..

    Emulation by Bryan McPhail, mish@tendril.co.uk


    Cycle Warriors Board Layout

    ABA-011


            6296             CW24A                   5864
                                CW25A                   5864
            YM2151                  50MHz

                                TZ8315                 CW26A
                                                    5864
        TC51821  TC51832                               D780C-1
        TC51821  TC51832
        TC51821  TC51832                     16MHz
        TC51821  TC51832

        CW00A   CW08A
        CW01A   CW09A
        CW02A   CW10A
        CW03A   CW11A            68000-12              81C78
        CW04A   CW12A                                  81C78
        CW05A   CW13A                CW16B  CW18B      65256
        CW06A   CW14A                CW17A  CW19A      65256
        CW07A   CW15A                       CW20A
                                            CW21       65256
                                68000-12   CW22A      65256
                                            CW23

    ABA-012

                            HD6445


                            51832
                            51832
                            51832
                            51832

                            CW28
                            CW29
                            CW30

    CW27

***************************************************************************/

#include "driver.h"
#include "tatsumi.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static UINT16 *cyclwarr_cpua_ram, *cyclwarr_cpub_ram;
UINT16 *tatsumi_c_ram, *apache3_g_ram;
UINT16 *roundup5_d0000_ram, *roundup5_e0000_ram;
UINT8 *tatsumi_rom_sprite_lookup1, *tatsumi_rom_sprite_lookup2;
UINT8 *tatsumi_rom_clut0, *tatsumi_rom_clut1;
UINT8 *roundup5_unknown0, *roundup5_unknown1, *roundup5_unknown2;
UINT8 *apache3_bg_ram;

/***************************************************************************/

//static READ16_HANDLER(cyclwarr_cpu_b_r) { return cyclwarr_cpub_ram[offset+0x800]; }
//static WRITE16_HANDLER(cyclwarr_cpu_b_w){ COMBINE_DATA(&cyclwarr_cpub_ram[offset+0x800]); }
static READ16_HANDLER(cyclwarr_cpu_bb_r){ return cyclwarr_cpub_ram[offset]; }
static WRITE16_HANDLER(cyclwarr_cpu_bb_w) { COMBINE_DATA(&cyclwarr_cpub_ram[offset]); }
static READ16_HANDLER(cyclwarr_palette_r) { return paletteram16[offset]; }
static READ16_HANDLER(cyclwarr_sprite_r) { return spriteram16[offset]; }
static WRITE16_HANDLER(cyclwarr_sprite_w) { COMBINE_DATA(&spriteram16[offset]); }
static READ16_HANDLER(cyclwarr_input_r) { return readinputport(offset); }
static READ16_HANDLER(cyclwarr_input2_r) { return readinputport(offset+3); }

/***************************************************************************/

static ADDRESS_MAP_START( readmem_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x07fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x08000, 0x08fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0c000, 0x0dfff) AM_READ(MRA8_RAM)
//  AM_RANGE(0x0e800, 0x0e803) AM_READ(MRA_NOP) // CRT
	AM_RANGE(0x0f000, 0x0f000) AM_READ(input_port_3_r) // Dip 1
	AM_RANGE(0x0f001, 0x0f001) AM_READ(input_port_4_r) // Dip 2
	AM_RANGE(0x0f800, 0x0f801) AM_READ(apache3_bank_r)
	AM_RANGE(0x10000, 0x1ffff) AM_READ(apache3_v30_v20_r)
	AM_RANGE(0x20000, 0x2ffff) AM_READ(tatsumi_v30_68000_r)
	AM_RANGE(0xa0000, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x07fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x08000, 0x08fff) AM_WRITE(apache3_palette_w) AM_BASE(&paletteram)
	AM_RANGE(0x0c000, 0x0dfff) AM_WRITE(roundup5_text_w) AM_BASE(&videoram)
	AM_RANGE(0x0e800, 0x0e803) AM_WRITE(MWA8_NOP) // CRT
	AM_RANGE(0x0f000, 0x0f001) AM_WRITE(MWA8_NOP) // todo
	AM_RANGE(0x0f800, 0x0f801) AM_WRITE(apache3_bank_w)
	AM_RANGE(0x10000, 0x1ffff) AM_WRITE(apache3_v30_v20_w)
	AM_RANGE(0x20000, 0x23fff) AM_WRITE(tatsumi_v30_68000_w)
	AM_RANGE(0xa0000, 0xfffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_readmem_apache3, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x80000, 0x83fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x90000, 0x93fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xd0000, 0xdffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xe0000, 0xe7fff) AM_READ(apache3_z80_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_writemem_apache3, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x80000, 0x83fff) AM_WRITE(MWA16_RAM) AM_BASE(&tatsumi_68k_ram)
	AM_RANGE(0x90000, 0x93fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)
	AM_RANGE(0x9a000, 0x9a1ff) AM_WRITE(tatsumi_sprite_control_w) AM_BASE(&tatsumi_sprite_control_ram)
	AM_RANGE(0xa0000, 0xa0001) AM_WRITE(apache3_a0000_w)
	AM_RANGE(0xb0000, 0xb0001) AM_WRITE(apache3_irq_ack_w) //todo - z80 reset?
	AM_RANGE(0xc0000, 0xc0001) AM_WRITE(MWA16_RAM) AM_BASE(&tatsumi_c_ram)
	AM_RANGE(0xd0000, 0xdffff) AM_WRITE(MWA16_RAM) AM_BASE(&apache3_g_ram)
	AM_RANGE(0xe0000, 0xe7fff) AM_WRITE(apache3_z80_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_iop_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x01fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x04000, 0x04003) AM_READ(MRA8_NOP) // piu select .. ?
	AM_RANGE(0x06000, 0x06000) AM_READ(input_port_0_r) // esw
	AM_RANGE(0x08001, 0x08001) AM_READ(tatsumi_hack_ym2151_r) //YM2151_status_port_0_r)
	AM_RANGE(0x0a000, 0x0a000) AM_READ(tatsumi_hack_oki_r) //OKIM6295_status_0_r)
	AM_RANGE(0x0e000, 0x0e000) AM_READ(apache3_adc_r) // adc
	AM_RANGE(0xf0000, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_iop_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x01fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x04000, 0x04003) AM_WRITE(MWA8_NOP) // piu select .. ?
	AM_RANGE(0x08000, 0x08000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x08001, 0x08001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x0a000, 0x0a000) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0x0e000, 0x0e007) AM_WRITE(apache3_adc_w) //adc select
	AM_RANGE(0xf0000, 0xfffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_readmem_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x08000, 0x83ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_writemem_apache3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x1fff) AM_WRITE(MWA8_RAM) AM_BASE(&apache3_z80_ram)
	AM_RANGE(0x08000, 0x83ff) AM_WRITE(MWA8_RAM) AM_BASE(&apache3_bg_ram)
ADDRESS_MAP_END

/*****************************************************************/

static ADDRESS_MAP_START( readmem_roundup5, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x07fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x08000, 0x0bfff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0d000, 0x0d000) AM_READ(input_port_3_r) /* Dip 1 */
	AM_RANGE(0x0d001, 0x0d001) AM_READ(input_port_4_r) /* Dip 2 */
	AM_RANGE(0x0f000, 0x0ffff) AM_READ(MRA8_RAM)
	AM_RANGE(0x10000, 0x1ffff) AM_READ(roundup_v30_z80_r)
	AM_RANGE(0x20000, 0x2ffff) AM_READ(tatsumi_v30_68000_r)
	AM_RANGE(0x30000, 0x3ffff) AM_READ(roundup5_vram_r)
	AM_RANGE(0x80000, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_roundup5, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x07fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x08000, 0x0bfff) AM_WRITE(roundup5_text_w) AM_BASE(&videoram)
	AM_RANGE(0x0c000, 0x0c003) AM_WRITE(roundup5_crt_w)
	AM_RANGE(0x0d400, 0x0d40f) AM_WRITE(MWA8_RAM) AM_BASE(&roundup5_unknown0)
	AM_RANGE(0x0d800, 0x0d801) AM_WRITE(MWA8_RAM) AM_BASE(&roundup5_unknown1) // VRAM2 X scroll (todo)
	AM_RANGE(0x0dc00, 0x0dc01) AM_WRITE(MWA8_RAM) AM_BASE(&roundup5_unknown2) // VRAM2 Y scroll (todo)
	AM_RANGE(0x0e000, 0x0e001) AM_WRITE(roundup5_control_w)
	AM_RANGE(0x0f000, 0x0ffff) AM_WRITE(roundup5_palette_w) AM_BASE(&paletteram)
	AM_RANGE(0x10000, 0x1ffff) AM_WRITE(roundup_v30_z80_w)
	AM_RANGE(0x20000, 0x23fff) AM_WRITE(tatsumi_v30_68000_w)
	AM_RANGE(0x30000, 0x3ffff) AM_WRITE(roundup5_vram_w)
	AM_RANGE(0x80000, 0xfffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_roundup5_sub, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x80000, 0x83fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x90000, 0x93fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xa0000, 0xa0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xb0000, 0xb0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xc0000, 0xc0fff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_roundup5_sub, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x80000, 0x83fff) AM_WRITE(MWA16_RAM) AM_BASE(&tatsumi_68k_ram)
	AM_RANGE(0x90000, 0x93fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)
	AM_RANGE(0x9a000, 0x9a1ff) AM_WRITE(tatsumi_sprite_control_w) AM_BASE(&tatsumi_sprite_control_ram)
	AM_RANGE(0xa0000, 0xa0fff) AM_WRITE(MWA16_RAM) AM_BASE(&roundup_r_ram) // Road control data
	AM_RANGE(0xb0000, 0xb0fff) AM_WRITE(MWA16_RAM) AM_BASE(&roundup_p_ram) // Road pixel data
	AM_RANGE(0xc0000, 0xc0fff) AM_WRITE(MWA16_RAM) AM_BASE(&roundup_l_ram) // Road colour data
	AM_RANGE(0xd0002, 0xd0003) AM_WRITE(roundup5_d0000_w) AM_BASE(&roundup5_d0000_ram)
	AM_RANGE(0xe0000, 0xe0001) AM_WRITE(roundup5_e0000_w) AM_BASE(&roundup5_e0000_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_roundup5_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xffef) AM_READ(MRA8_RAM) // maybe less than this...
	AM_RANGE(0xfff1, 0xfff1) AM_READ(tatsumi_hack_ym2151_r) //YM2151_status_port_0_r)
	AM_RANGE(0xfff4, 0xfff4) AM_READ(tatsumi_hack_oki_r) //OKIM6295_status_0_r)
	AM_RANGE(0xfff8, 0xfff8) AM_READ(input_port_0_r)
	AM_RANGE(0xfff9, 0xfff9) AM_READ(input_port_1_r)
	AM_RANGE(0xfffc, 0xfffc) AM_READ(input_port_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_roundup5_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xffef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xfff0, 0xfff0) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xfff1, 0xfff1) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xfff4, 0xfff4) AM_WRITE(OKIM6295_data_0_w)

	AM_RANGE(0xfff9, 0xfff9) AM_WRITE(MWA8_NOP) //irq ack?
	AM_RANGE(0xfffa, 0xfffa) AM_WRITE(MWA8_NOP) //irq ack?
ADDRESS_MAP_END

/******************************************************************************/

static ADDRESS_MAP_START( readmem_cyclwarr_a, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x03e000, 0x03efff) AM_READ(MRA16_RAM)
	AM_RANGE(0x040000, 0x043fff) AM_READ(cyclwarr_cpu_bb_r)
	AM_RANGE(0x080000, 0x08ffff) AM_READ(cyclwarr_videoram2_r)
	AM_RANGE(0x090000, 0x09ffff) AM_READ(cyclwarr_videoram_r)
	AM_RANGE(0x0b9002, 0x0b9009) AM_READ(cyclwarr_input_r) //b9008 - dips
	// ba000 + ba002 - dips
	AM_RANGE(0x0ba000, 0x0ba003) AM_READ(cyclwarr_input2_r) //temp
	AM_RANGE(0x0ba004, 0x0ba007) AM_READ(cyclwarr_input2_r)
	AM_RANGE(0x0ba008, 0x0ba009) AM_READ(cyclwarr_control_r)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_READ(cyclwarr_sprite_r)
	AM_RANGE(0x0d0000, 0x0d3fff) AM_READ(cyclwarr_palette_r)
	AM_RANGE(0x140000, 0x1bffff) AM_READ(MRA16_BANK2) /* CPU B ROM */
	AM_RANGE(0x2c0000, 0x33ffff) AM_READ(MRA16_BANK1) /* CPU A ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_cyclwarr_a, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00dfff) AM_WRITE(MWA16_RAM) AM_BASE(&cyclwarr_cpua_ram)
	AM_RANGE(0x00e000, 0x00ffff) AM_WRITE(MWA16_RAM) AM_BASE(&videoram16)
	AM_RANGE(0x03e000, 0x03efff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x040000, 0x043fff) AM_WRITE(cyclwarr_cpu_bb_w)
	AM_RANGE(0x080000, 0x08ffff) AM_WRITE(cyclwarr_videoram2_w) AM_BASE(&cyclwarr_videoram2)
	AM_RANGE(0x090000, 0x09ffff) AM_WRITE(cyclwarr_videoram_w) AM_BASE(&cyclwarr_videoram)
	AM_RANGE(0x0ba008, 0x0ba009) AM_WRITE(cyclwarr_control_w)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_WRITE(cyclwarr_sprite_w) AM_BASE(&spriteram16)
	AM_RANGE(0x0ca000, 0x0ca1ff) AM_WRITE(tatsumi_sprite_control_w) AM_BASE(&tatsumi_sprite_control_ram)
	AM_RANGE(0x0d0000, 0x0d3fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_cyclwarr_b, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x080000, 0x08ffff) AM_READ(cyclwarr_videoram2_r)
	AM_RANGE(0x090000, 0x09ffff) AM_READ(cyclwarr_videoram_r)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_READ(cyclwarr_sprite_r)
	AM_RANGE(0x0d0000, 0x0d3fff) AM_READ(cyclwarr_palette_r)
	AM_RANGE(0x140000, 0x1bffff) AM_READ(MRA16_BANK2)
	AM_RANGE(0x2c0000, 0x33ffff) AM_READ(MRA16_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_cyclwarr_b, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00dfff) AM_WRITE(MWA16_RAM) AM_BASE(&cyclwarr_cpub_ram)
	AM_RANGE(0x00e000, 0x00ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x080000, 0x08ffff) AM_WRITE(cyclwarr_videoram2_w)
	AM_RANGE(0x090000, 0x09ffff) AM_WRITE(cyclwarr_videoram_w)
	AM_RANGE(0x0c0000, 0x0c3fff) AM_WRITE(cyclwarr_sprite_w)
	AM_RANGE(0x0ca000, 0x0ca1ff) AM_WRITE(tatsumi_sprite_control_w)
	AM_RANGE(0x0d0000, 0x0d3fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_cyclwarr_c, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xffef) AM_READ(MRA8_RAM) // maybe less than this...
	AM_RANGE(0xfff1, 0xfff1) AM_READ(tatsumi_hack_ym2151_r) //YM2151_status_port_0_r)
	AM_RANGE(0xfff4, 0xfff4) AM_READ(tatsumi_hack_oki_r) // OKIM6295_status_0_r)
	AM_RANGE(0xfff8, 0xfff8) AM_READ(input_port_0_r)
	AM_RANGE(0xfff9, 0xfff9) AM_READ(input_port_1_r)
//  AM_RANGE(0xfffa, 0xfffa) AM_READ(input_port_0_r)// MRA_NOP) //irq ack???
	AM_RANGE(0xfffc, 0xfffc) AM_READ(input_port_2_r)// MRA_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_cyclwarr_c, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xffef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xfff0, 0xfff0) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xfff1, 0xfff1) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xfff4, 0xfff4) AM_WRITE(OKIM6295_data_0_w)
	AM_RANGE(0xfff9, 0xfff9) AM_WRITE(MWA8_NOP) //irq ack?
	AM_RANGE(0xfffa, 0xfffa) AM_WRITE(MWA8_NOP) //irq ack?
ADDRESS_MAP_END

/******************************************************************************/

INPUT_PORTS_START( apache3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME( "Trigger" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME( "Power" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME( "Missile" )

	PORT_START
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( roundup5 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("Accelerator")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("Brake")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("Shift")
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("Turbo")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME(DEF_STR(Free_Play)) PORT_TOGGLE
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("Extra 2") PORT_TOGGLE
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("Extra 3") PORT_TOGGLE
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("Extra 4") PORT_TOGGLE

	PORT_START
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x40, 0x00, "Stage 5 Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Output Mode" )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x80, "B" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Test ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cyclwarr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(3)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(4)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON4  )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON4  )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON4  )
INPUT_PORTS_END

/******************************************************************************/

static const gfx_layout roundup5_charlayout =
{
	8,8,	/* 16*16 sprites */
	RGN_FRAC(1,1),	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8,12,0,4, 24,28, 16,20},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout cyclwarr_charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3)},
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8
};
static const gfx_layout cyclwarr_charlayout2 =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3)},
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8
};
static const gfx_layout roundup5_vramlayout =
{
	8,8,
	4096 + 2048,
	3,
	{ 0x30000 * 8, 0x18000 * 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16},
	8*16
};

static const gfx_decode gfxdecodeinfo_apache3[] =
{
	{ REGION_GFX1, 0, &roundup5_charlayout,    1024, 128},
	{ REGION_GFX4, 0, &cyclwarr_charlayout,     768, 16},
	{ -1 } /* end of array */
};

static const gfx_decode roundup5_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &roundup5_charlayout,     1024, 256},
	{ 0, 0, &roundup5_vramlayout,					0, 16},
	{ -1 } /* end of array */
};

static const gfx_decode cyclwarr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &roundup5_charlayout,    8192, 512},
	{ REGION_GFX5, 0, &cyclwarr_charlayout,    0, 512},
	{ REGION_GFX5, 0, &cyclwarr_charlayout2,   0, 512},
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(int state)
{
	cpunum_set_input_line(2, INPUT_LINE_IRQ0, state);
}

static struct YM2151interface ym2151_interface =
{
	sound_irq
};

static INTERRUPT_GEN( roundup5_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xc8/4);	/* VBL */
}

static MACHINE_DRIVER_START( apache3 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,20000000 / 2) /* NEC V30 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(readmem_apache3,writemem_apache3)
	MDRV_CPU_VBLANK_INT(roundup5_interrupt,1)

	MDRV_CPU_ADD(M68000,20000000 / 2) /* 68000 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(sub_readmem_apache3,sub_writemem_apache3)

	MDRV_CPU_ADD(V20, 8000000) //???
	MDRV_CPU_PROGRAM_MAP(readmem_iop_apache3,writemem_iop_apache3)

	MDRV_CPU_ADD(Z80, 8000000) //???
	MDRV_CPU_PROGRAM_MAP(z80_readmem_apache3,z80_writemem_apache3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_apache3)
	MDRV_PALETTE_LENGTH(1024 + 4096) /* 1024 real colours, and 4096 arranged as series of cluts */

	MDRV_VIDEO_START(apache3)
	MDRV_VIDEO_UPDATE(apache3)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 16000000/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 20000000/8/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.75)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.75)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( roundup5 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,20000000 / 2) /* NEC V30 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(readmem_roundup5,writemem_roundup5)
	MDRV_CPU_VBLANK_INT(roundup5_interrupt,1)

	MDRV_CPU_ADD(M68000,20000000 / 2) /* 68000 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(readmem_roundup5_sub,writemem_roundup5_sub)

	MDRV_CPU_ADD(Z80, 4000000) //???
	MDRV_CPU_PROGRAM_MAP(readmem_roundup5_sound,writemem_roundup5_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(roundup5_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024 + 4096) /* 1024 real colours, and 4096 arranged as series of cluts */

	MDRV_VIDEO_START(roundup5)
	MDRV_VIDEO_UPDATE(roundup5)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 16000000/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 20000000/8/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.75)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.75)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cyclwarr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000 / 2) /* NEC V30 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(readmem_cyclwarr_a,writemem_cyclwarr_a )
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(M68000,20000000 / 2) /* 68000 CPU, 20MHz / 2 */
	MDRV_CPU_PROGRAM_MAP(readmem_cyclwarr_b,writemem_cyclwarr_b)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000) //???
	MDRV_CPU_PROGRAM_MAP(readmem_cyclwarr_c,writemem_cyclwarr_c)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(200)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(cyclwarr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192 + 8192) //todo

	MDRV_VIDEO_START(cyclwarr)
	MDRV_VIDEO_UPDATE(cyclwarr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 16000000/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.45)
	MDRV_SOUND_ROUTE(1, "right", 0.45)

	MDRV_SOUND_ADD(OKIM6295, 20000000/8/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.75)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.75)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( apache3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE( "ap-25f.125",   0x0a0001, 0x10000, CRC(3c7530f4) SHA1(9f7b58a3abddbdc3081ba9dfc1732406eb8c1752) )
	ROM_LOAD16_BYTE( "ap-26f.133",   0x0a0000, 0x10000, CRC(2955997f) SHA1(86e37def923d9cf4eb33e7979118ec6f1ef62678) )
	ROM_LOAD16_BYTE( "ap-23f.110",   0x0e0001, 0x10000, CRC(d7077149) SHA1(b08f5a9ee03641c20bdd5e5c9671a22c740150c6) )
	ROM_LOAD16_BYTE( "ap-24f.118",   0x0e0000, 0x10000, CRC(0bdef11b) SHA1(ed687600962ed2ca3a8e67cbd84fa5486778eade) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "ap-19c.80",   0x000001, 0x10000, CRC(0908e468) SHA1(a2d725993bd4cd5425468736154fd3dd9dd7b060) )
	ROM_LOAD16_BYTE( "ap-21c.97",   0x000000, 0x10000, CRC(38a056fb) SHA1(67c8ae58670cebde0771854e1fb5fc2eb2543ecc) )
	ROM_LOAD16_BYTE( "ap-20a.89",   0x040001, 0x20000, CRC(92d24b5e) SHA1(1ea270d46a607e47b7e0961b532316aa05dc8f4e) )
	ROM_LOAD16_BYTE( "ap-22a.105",  0x040000, 0x20000, CRC(a8458a92) SHA1(43674731c2e9962c2bfbb73a85484cf03d6be223) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 64k code for sound V20 */
	ROM_LOAD( "ap-27d.151",   0x0f0000, 0x10000, CRC(294b4d79) SHA1(2b03418a12a2aaf3919b98161d8d0ce6ae29a2bb) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ap-00c.15",   0x000000, 0x20000, CRC(ad1ddc2b) SHA1(81f64663c4892ab5fb0e2dc99513dbfee73f15b8) )
	ROM_LOAD32_BYTE( "ap-01c.22",   0x000001, 0x20000, CRC(6286ff00) SHA1(920da4a3a441dbf54ad86c0f4fb6f47a867e9cda) )
	ROM_LOAD32_BYTE( "ap-04c.58",   0x000002, 0x20000, CRC(dc6d55e4) SHA1(9f48f8d6aa1a329a71913139a8d5a50d95a9b9e5) )
	ROM_LOAD32_BYTE( "ap-05c.65",   0x000003, 0x20000, CRC(2e6e495f) SHA1(af610f265da53735b20ddc6df1bda47fc54ee0c3) )
	ROM_LOAD32_BYTE( "ap-02c.34",   0x080000, 0x20000, CRC(af4ee7cb) SHA1(4fe2361b7431971b07671f145abf1ea5861d01db) )
	ROM_LOAD32_BYTE( "ap-03c.46",   0x080001, 0x20000, CRC(60ab495c) SHA1(18340d4fba550495b1e52f8023a0a2ec6349dfeb) )
	ROM_LOAD32_BYTE( "ap-06c.71",   0x080002, 0x20000, CRC(0ea90e55) SHA1(b16d6b8be4853797507d3e5c933a9dd1d451308e) )
	ROM_LOAD32_BYTE( "ap-07c.75",   0x080003, 0x20000, CRC(ba685543) SHA1(140a2b708d4e4de4d207fc2c4a96a5cab8639988) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ap-08c.14",   0x000000, 0x20000, CRC(6437b580) SHA1(2b2ba42add18bbec04fbcf53645a8d44b972e26a) )
	ROM_LOAD32_BYTE( "ap-09c.21",   0x000001, 0x20000, CRC(54d18ef9) SHA1(40ebc6ea49b2a501fe843d60bec8c32d07f2d25d) )
	ROM_LOAD32_BYTE( "ap-12c.57",   0x000002, 0x20000, CRC(f95cf5cf) SHA1(ce373c648cbf3e4863bbc3a1175efe065c75eb13) )
	ROM_LOAD32_BYTE( "ap-13c.64",   0x000003, 0x20000, CRC(67a248c3) SHA1(cc945f7cfecaaab5075c1a3d202369b070d4c656) )
	ROM_LOAD32_BYTE( "ap-10c.33",   0x080000, 0x20000, CRC(74418df4) SHA1(cc1206b10afc2de919b2fb9899486122d27290a4) )
	ROM_LOAD32_BYTE( "ap-11c.45",   0x080001, 0x20000, CRC(195bf78e) SHA1(c3c472f3c4244545b89491b6ebec4f838a6bbb73) )
	ROM_LOAD32_BYTE( "ap-14c.70",   0x080002, 0x20000, CRC(58f7fe16) SHA1(a5b87b42b85808c226df0d2a7b7cdde12d474a41) )
	ROM_LOAD32_BYTE( "ap-15c.74",   0x080003, 0x20000, CRC(1ffd5496) SHA1(25efb568957fc9441a40a7d64cc6afe1a14b392b) )

	ROM_REGION( 0x18000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ap-18d.73",   0x000000, 0x8000, CRC(55e664bf) SHA1(505bec8b5ff3f9fa2c5fb1213d54683347905be1) )
	ROM_LOAD( "ap-17d.68",   0x008000, 0x8000, CRC(6199afe4) SHA1(ad8c0ed6c33d984bb29c89f2e7fc7e5a923cefe3) )
	ROM_LOAD( "ap-16d.63",   0x010000, 0x8000, CRC(f115656d) SHA1(61798858dc0172192d89e666696b2c7642756899) )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "ap-28c.171",   0x000000, 0x20000, CRC(b349f0c2) SHA1(cb1ff1c0e784f669c87ab1eccd3b358950761b74) )
	ROM_LOAD( "ap-29c.176",   0x020000, 0x10000, CRC(b38fced3) SHA1(72f61a719f393957bcccf14687bfbb2e7a5f7aee) )
ROM_END

ROM_START( roundup5 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE( "ru-23s",   0x080000, 0x20000, CRC(2dc8c521) SHA1(b78de101db3ef00fc4375ae32a7871e0da2dac6c) )
	ROM_LOAD16_BYTE( "ru-26s",   0x080001, 0x20000, CRC(1e16b531) SHA1(d7badef29cf1c4a9bd262933ecd1ca3343ea94bd) )
	ROM_LOAD16_BYTE( "ru-22t",   0x0c0000, 0x20000, CRC(9611382e) SHA1(c99258782dbad6d69ba7f54115ee3aa218f9b6ee) )
	ROM_LOAD16_BYTE( "ru-25t",   0x0c0001, 0x20000, CRC(b6cd0f2d) SHA1(61925c2346d79baaf9bce3d19a7dfc45b8232f92) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "ru-20s",   0x000000, 0x20000, CRC(c5524558) SHA1(a94e7e4548148c83a332524ab4e06607732e13d5) )
	ROM_LOAD16_BYTE( "ru-18s",   0x000001, 0x20000, CRC(163ef03d) SHA1(099ac2d74164bdc6402b08efb521f49275780858) )
	ROM_LOAD16_BYTE( "ru-24s",   0x040000, 0x20000, CRC(b9f91b70) SHA1(43c5d9dafb60ed3e5c3eb0e612c2dbc5497f8a6c) )
	ROM_LOAD16_BYTE( "ru-19s",   0x040001, 0x20000, CRC(e3953800) SHA1(28fbc6bf154b512fcefeb04fe12db598b1b20cfe) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "ru-28d",   0x000000, 0x10000, CRC(df36c6c5) SHA1(c046482043f6b54c55696ba3d339ffb11d78f674) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x0c0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ru-00b",   0x000000, 0x20000, CRC(388a0647) SHA1(e4ab43832872f44c0fe1aaede4372cc00ca7d32b) )
	ROM_LOAD32_BYTE( "ru-02b",   0x000001, 0x20000, CRC(eff33945) SHA1(3f4c3aaa11ccf945c2f898dfdf815705d8539e21) )
	ROM_LOAD32_BYTE( "ru-04b",   0x000002, 0x20000, CRC(40fda247) SHA1(f5fbc07fda024baedf35ac209210e94df9f15065) )
	ROM_LOAD32_BYTE( "ru-06b",   0x000003, 0x20000, CRC(cd2484f3) SHA1(a23a4d36a8b913104bcc75228317b2979afec888) )
	ROM_LOAD32_BYTE( "ru-01b",   0x080000, 0x10000, CRC(5e91f401) SHA1(df976c5ba0f14b14f5642b5ca35b996bca64e369) )
	ROM_LOAD32_BYTE( "ru-03b",   0x080001, 0x10000, CRC(2fb109de) SHA1(098c103e6bae0f52ec66f0cdda2da60bd7108736) )
	ROM_LOAD32_BYTE( "ru-05b",   0x080002, 0x10000, CRC(23dd10e1) SHA1(f30ff1a8c7ed9bc567b901cbdd202028fffb9f80) )
	ROM_LOAD32_BYTE( "ru-07b",   0x080003, 0x10000, CRC(bb40f46e) SHA1(da694e16d19f60a0dee47551f00f3e50b2d5dcaf) )

	ROM_REGION( 0x0c0000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "ru-08b",   0x000000, 0x20000, CRC(01729e3c) SHA1(1445287fde0b993d053aab73efafc902a6b7e2cc) )
	ROM_LOAD32_BYTE( "ru-10b",   0x000001, 0x20000, CRC(cd2357a7) SHA1(313460a74244325ce2c659816f2b738f3dc5358a) )
	ROM_LOAD32_BYTE( "ru-12b",   0x000002, 0x20000, CRC(ca63b1f8) SHA1(a50ef8259745dc166eb0a1b2c812ff620818a755) )
	ROM_LOAD32_BYTE( "ru-14b",   0x000003, 0x20000, CRC(dde79bfc) SHA1(2d5888189a6f954801f248a3365e328370fed837) )
	ROM_LOAD32_BYTE( "ru-09b",   0x080000, 0x10000, CRC(629ac0a6) SHA1(c3eeccd6c07be7455cf180c9c7d5efcd6d08c0b5) )
	ROM_LOAD32_BYTE( "ru-11b",   0x080001, 0x10000, CRC(fe3fbf53) SHA1(7400c088025ac22e5d9db816792533fc02f2dcf5) )
	ROM_LOAD32_BYTE( "ru-13b",   0x080002, 0x10000, CRC(d0f6e747) SHA1(ef15ed41124b2d37bc6e92254138690dd644e50f) )
	ROM_LOAD32_BYTE( "ru-15b",   0x080003, 0x10000, CRC(6ee6b22e) SHA1(a28edaf23ca6c7231264de962d5ea37bad39f996) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "ru-17b",   0x000000, 0x20000, CRC(82391b47) SHA1(6b1977522c6e906503abc50bdd24c4c38cdc9bdb) )
	ROM_LOAD( "ru-16e",   0x020000, 0x10000, CRC(374fe170) SHA1(5d190a2735698b0384948bfdb1a900f56f0d7ebc) )
ROM_END

ROM_START( cyclwarr )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 main cpu */
	ROM_LOAD16_BYTE( "cw16b",   0x100000, 0x20000, CRC(cb1a737a) SHA1(a603ee1256be5641d00a72f64efaaacb65ed9d7d) )
	ROM_LOAD16_BYTE( "cw18b",   0x100001, 0x20000, CRC(0633ddcb) SHA1(1196ab17065352ec5b37f2f6b383a43a2d0fa3a6) )
	ROM_LOAD16_BYTE( "cw17a",   0x140000, 0x20000, CRC(2ad6f836) SHA1(5fa4275b433013943ba1d1b64a3c725097f946f9) )
	ROM_LOAD16_BYTE( "cw19a",   0x140001, 0x20000, CRC(d3853658) SHA1(c9338083a04f55bd22285176831f4b0bdb78564f) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "cw20a",   0x100000, 0x20000, CRC(c3578ac1) SHA1(21d369da874f01922d0f0b757a42b4321df891d4) )
	ROM_LOAD16_BYTE( "cw22a",   0x100001, 0x20000, CRC(5339ed24) SHA1(5b0a54c2442dcf7373ff8b55b91af9772473ff77) )
	ROM_LOAD16_BYTE( "cw21",    0x140000, 0x20000, CRC(ed90d956) SHA1(f533f93da31ac6eb631fb506357717e7cac8e186) )
	ROM_LOAD16_BYTE( "cw23",    0x140001, 0x20000, CRC(009cdc78) SHA1(a77933a7736546397e8c69226703d6f9be7b55e5) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "cw26a",   0x000000, 0x10000, CRC(f7a70e3a) SHA1(5581633bf1f15d7f5c1e03de897d65d60f9f1e33) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "cw00a",   0x000000, 0x20000, CRC(058a77f1) SHA1(93f99fcf6ce6714d76af6f6e930115516f0379d3) )
	ROM_LOAD32_BYTE( "cw08a",   0x000001, 0x20000, CRC(f53993e7) SHA1(ef2d502ab180d2bc0bdb698c2878fdee9a2c33a8) )
	ROM_LOAD32_BYTE( "cw02a",   0x000002, 0x20000, CRC(4dadf3cb) SHA1(e42c56e295a443cb605d48eba23a16fab3c86525) )
	ROM_LOAD32_BYTE( "cw10a",   0x000003, 0x20000, CRC(3b7cd251) SHA1(52b9637404fa193421294dfb52c1a7bba0d94c9b) )
	ROM_LOAD32_BYTE( "cw01a",   0x080000, 0x20000, CRC(7c639948) SHA1(d58ff5735cd3179ffafead385a625baa7962e1d0) )
	ROM_LOAD32_BYTE( "cw09a",   0x080001, 0x20000, CRC(4ba24af5) SHA1(9203c2639e04aaa09996339f11259750ff8129b9) )
	ROM_LOAD32_BYTE( "cw03a",   0x080002, 0x20000, CRC(3ca6f98e) SHA1(8526fe38d3b4c66e09049ba18651a9e7255d85d6) )
	ROM_LOAD32_BYTE( "cw11a",   0x080003, 0x20000, CRC(5d760392) SHA1(7bbda2880af4659c267193ce10ed887a1b54a981) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "cw04a",   0x000000, 0x20000, CRC(f05f594d) SHA1(80effaa517b2154c013419e0bc05fd0797b74c8d) )
	ROM_LOAD32_BYTE( "cw12a",   0x000001, 0x20000, CRC(4ac07e8b) SHA1(f9de96fba39d5752d61b8f6be87fb605694624ed) )
	ROM_LOAD32_BYTE( "cw06a",   0x000002, 0x20000, CRC(f628edc9) SHA1(473f7ec28000e6bf72782c1c3f4afb5e021bd430) )
	ROM_LOAD32_BYTE( "cw14a",   0x000003, 0x20000, CRC(a9131f5f) SHA1(3a2059946984733e6939f3298f0db676e6a3301b)	)
	ROM_LOAD32_BYTE( "cw05a",   0x080000, 0x20000, CRC(c8f5faa9) SHA1(f374531ffd645597eeb1440fd2cadb426fcd3d79) )
	ROM_LOAD32_BYTE( "cw13a",   0x080001, 0x20000, CRC(8091d381) SHA1(7faf068ce20b2877559f0335df55d61be13146b4) )
	ROM_LOAD32_BYTE( "cw07a",   0x080002, 0x20000, CRC(314579b5) SHA1(3c10ec490f7821a5b5412295232bbb104d0e4b83) )
	ROM_LOAD32_BYTE( "cw15a",   0x080003, 0x20000, CRC(7ed4b721) SHA1(b87865effeff77a9ea74354ef2b5911a5102a647) )

	ROM_REGION( 0x20000, REGION_GFX4, 0 )
	ROM_LOAD( "cw27",   0x000000, 0x20000, CRC(2db48a9e) SHA1(16c307340d17cd3b5455ebcee681fbe0335dec58) )

	ROM_REGION( 0x60000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "cw28",   0x000000, 0x20000, CRC(3fc568ed) SHA1(91125c9deddc659449ca6791a847fe908c2818b2) )
	ROM_LOAD( "cw29",   0x020000, 0x20000, CRC(64dd519c) SHA1(e23611fc2be896861997063546c3eb03527eaf8e) )
	ROM_LOAD( "cw30",   0x040000, 0x20000, CRC(331d0711) SHA1(82251fe1f1d36f079080943ab1fd04a60077c353) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "cw24a",   0x000000, 0x20000, CRC(22600cba) SHA1(a1514fbe037942f1493a17eb0b7986949470cb22) )
	ROM_LOAD( "cw25a",   0x020000, 0x20000, CRC(372c6bc8) SHA1(d4875bf3bffecf338bebba3b8d6a791585556a06) )
ROM_END

/***************************************************************************/

static DRIVER_INIT( apache3 )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;

	cpunum_set_input_line(3, INPUT_LINE_HALT, ASSERT_LINE); // ?

	for (i=0; i<0x100000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	// Copy sprite & palette data out of GFX rom area
	tatsumi_rom_sprite_lookup1=auto_malloc(0x4000);
	tatsumi_rom_sprite_lookup2=auto_malloc(0x4000);
	tatsumi_rom_clut0=auto_malloc(0x1000);
	tatsumi_rom_clut1=auto_malloc(0x1000);
	memcpy(tatsumi_rom_sprite_lookup1, memory_region(REGION_GFX2),0x4000);
	memcpy(tatsumi_rom_sprite_lookup2, memory_region(REGION_GFX3),0x4000);
	memcpy(tatsumi_rom_clut0, memory_region(REGION_GFX2)+0x100000-0x800,0x800);
	memcpy(tatsumi_rom_clut1, memory_region(REGION_GFX3)+0x100000-0x800,0x800);

	tatsumi_reset();
}

static DRIVER_INIT( roundup5 )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;

	for (i=0; i<0xc0000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	// Copy sprite & palette data out of GFX rom area
	tatsumi_rom_sprite_lookup1=auto_malloc(0x4000);
	tatsumi_rom_sprite_lookup2=auto_malloc(0x4000);
	tatsumi_rom_clut0=auto_malloc(0x800);
	tatsumi_rom_clut1=auto_malloc(0x800);
	memcpy(tatsumi_rom_sprite_lookup1, memory_region(REGION_GFX2),0x4000);
	memcpy(tatsumi_rom_sprite_lookup2, memory_region(REGION_GFX3),0x4000);
	memcpy(tatsumi_rom_clut0, memory_region(REGION_GFX2)+0xc0000-0x800,0x800);
	memcpy(tatsumi_rom_clut1, memory_region(REGION_GFX3)+0xc0000-0x800,0x800);

	tatsumi_reset();
}

static DRIVER_INIT( cyclwarr )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;
	for (i=0; i<0x100000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	dst = memory_region(REGION_CPU1);
	memcpy(cyclwarr_cpua_ram,dst+0x100000,8);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x100000);

	dst = memory_region(REGION_CPU2);
	memcpy(cyclwarr_cpub_ram,dst+0x100000,8);
	memory_set_bankptr(2, memory_region(REGION_CPU2) + 0x100000);

	// Copy sprite & palette data out of GFX rom area
	tatsumi_rom_sprite_lookup1=auto_malloc(0x4000);
	tatsumi_rom_sprite_lookup2=auto_malloc(0x4000);
	tatsumi_rom_clut0=auto_malloc(0x1000);
	tatsumi_rom_clut1=auto_malloc(0x1000);
	memcpy(tatsumi_rom_sprite_lookup1, memory_region(REGION_GFX2),0x4000);
	memcpy(tatsumi_rom_sprite_lookup2, memory_region(REGION_GFX3),0x4000);
	memcpy(tatsumi_rom_clut0, memory_region(REGION_GFX2)+0x100000-0x1000,0x1000);
	memcpy(tatsumi_rom_clut1, memory_region(REGION_GFX3)+0x100000-0x1000,0x1000);

	tatsumi_reset();
}

/***************************************************************************/

/* http://www.tatsu-mi.co.jp/game/HTML/history.html */

/* 1986 Lock On */
/* 1987 Gray Out */
GAME( 1988, apache3,  0, apache3,   apache3,  apache3,  ROT0, "Tatsumi", "Apache 3", GAME_IMPERFECT_GRAPHICS )
GAME( 1989, roundup5, 0, roundup5,  roundup5, roundup5, ROT0, "Tatsumi", "Round Up 5 - Super Delta Force", GAME_IMPERFECT_GRAPHICS )
GAME( 1991, cyclwarr, 0, cyclwarr,  cyclwarr, cyclwarr, ROT0, "Tatsumi", "Cycle Warriors", GAME_IMPERFECT_GRAPHICS )
/* 1992 Big Fight */
