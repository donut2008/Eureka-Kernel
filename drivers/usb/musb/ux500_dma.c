GC_CAC_ACC_CU3__ACCUMULATOR_31_0__SHIFT 0x0
#define GC_CAC_ACC_CU4__ACCUMULATOR_31_0_MASK 0xffffffff
#define GC_CAC_ACC_CU4__ACCUMULATOR_31_0__SHIFT 0x0
#define GC_CAC_ACC_CU5__ACCUMULATOR_31_0_MASK 0xffffffff
#define GC_CAC_ACC_CU5__ACCUMULATOR_31_0__SHIFT 0x0
#define GC_CAC_ACC_CU6__ACCUMULATOR_31_0_MASK 0xffffffff
#define GC_CAC_ACC_CU6__ACCUMULATOR_31_0__SHIFT 0x0
#define GC_CAC_ACC_CU7__ACCUMULATOR_31_0_MASK 0xffffffff
#define GC_CAC_ACC_CU7__ACCUMULATOR_31_0__SHIFT 0x0
#define GC_CAC_OVRD_CU__OVRRD_SELECT_MASK 0xffff
#define GC_CAC_OVRD_CU__OVRRD_SELECT__SHIFT 0x0
#define GC_CAC_OVRD_CU__OVRRD_VALUE_MASK 0xffff0000
#define GC_CAC_OVRD_CU__OVRRD_VALUE__SHIFT 0x10

#endif /* SMU_8_0_SH_MASK_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright 2006-2007 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/****************************************************************************/
/*Portion I: Definitions  shared between VBIOS and Driver                   */
/****************************************************************************/

#ifndef _ATOMBIOS_H
#define _ATOMBIOS_H

#define ATOM_VERSION_MAJOR                   0x00020000
#define ATOM_VERSION_MINOR                   0x00000002

#define ATOM_HEADER_VERSION (ATOM_VERSION_MAJOR | ATOM_VERSION_MINOR)

/* Endianness should be specified before inclusion,
 * default to little endian
 */
#ifndef ATOM_BIG_ENDIAN
#error Endian not specified
#endif

#ifdef _H2INC
  #ifndef ULONG
    typedef unsigned long ULONG;
  #endif

  #ifndef UCHAR
    typedef unsigned char UCHAR;
  #endif

  #ifndef USHORT
    typedef unsigned short USHORT;
  #endif
#endif

#define ATOM_DAC_A            0
#define ATOM_DAC_B            1
#define ATOM_EXT_DAC          2

#define ATOM_CRTC1            0
#define ATOM_CRTC2            1
#define ATOM_CRTC3            2
#define ATOM_CRTC4            3
#define ATOM_CRTC5            4
#define ATOM_CRTC6            5

#define ATOM_UNDERLAY_PIPE0   16
#define ATOM_UNDERLAY_PIPE1   17

#define ATOM_CRTC_INVALID     0xFF

#define ATOM_DIGA             0
#define ATOM_DIGB             1

#define ATOM_PPLL1            0
#define ATOM_PPLL2            1
#define ATOM_DCPLL            2
#define ATOM_PPLL0            2
#define ATOM_PPLL3            3

#define ATOM_EXT_PLL1         8
#define ATOM_EXT_PLL2         9
#define ATOM_EXT_CLOCK        10
#define ATOM_PPLL_INVALID     0xFF

#define ENCODER_REFCLK_SRC_P1PLL       0
#define ENCODER_REFCLK_SRC_P2PLL       1
#define ENCODER_REFCLK_SRC_DCPLL       2
#define ENCODER_REFCLK_SRC_EXTCLK      3
#define ENCODER_REFCLK_SRC_INVALID     0xFF

#define ATOM_SCALER_DISABLE   0   //For Fudo, it's bypass and auto-cengter & no replication
#define ATOM_SCALER_CENTER    1   //For Fudo, it's bypass and auto-center & auto replication
#define ATOM_SCALER_EXPANSION 2   //For Fudo, it's 2 Tap alpha blending mode
#define ATOM_SCALER_MULTI_EX  3   //For Fudo only, it's multi-tap mode only used to drive TV or CV, only used by Bios

#define ATOM_DISABLE          0
#define ATOM_ENABLE           1
#define ATOM_LCD_BLOFF                          (ATOM_DISABLE+2)
#define ATOM_LCD_BLON                           (ATOM_ENABLE+2)
#define ATOM_LCD_BL_BRIGHTNESS_CONTROL          (ATOM_ENABLE+3)
#define ATOM_LCD_SELFTEST_START                 (ATOM_DISABLE+5)
#define ATOM_LCD_SELFTEST_STOP                  (ATOM_ENABLE+5)
#define ATOM_ENCODER_INIT                       (ATOM_DISABLE+7)
#define ATOM_INIT                               (ATOM_DISABLE+7)
#define ATOM_GET_STATUS                         (ATOM_DISABLE+8)

#define ATOM_BLANKING         1
#define ATOM_BLANKING_OFF     0


#define ATOM_CRT1             0
#define ATOM_CRT2             1

#define ATOM_TV_NTSC          1
#define ATOM_TV_NTSCJ         2
#define ATOM_TV_PAL           3
#define ATOM_TV_PALM          4
#define ATOM_TV_PALCN         5
#define ATOM_TV_PALN          6
#define ATOM_TV_PAL60         7
#define ATOM_TV_SECAM         8
#define ATOM_TV_CV            16

#define ATOM_DAC1_PS2         1
#define ATOM_DAC1_CV          2
#define ATOM_DAC1_NTSC        3
#define ATOM_DAC1_PAL         4

#define ATOM_DAC2_PS2         ATOM_DAC1_PS2
#define ATOM_DAC2_CV          ATOM_DAC1_CV
#define ATOM_DAC2_NTSC        ATOM_DAC1_NTSC
#define ATOM_DAC2_PAL         ATOM_DAC1_PAL

#define ATOM_PM_ON            0
#define ATOM_PM_STANDBY       1
#define ATOM_PM_SUSPEND       2
#define ATOM_PM_OFF           3

// For ATOM_LVDS_INFO_V12
// Bit0:{=0:single, =1:dual},
// Bit1 {=0:666RGB, =1:888RGB},
// Bit2:3:{Grey level}
// Bit4:{=0:LDI format for RGB888, =1 FPDI format for RGB888}
#define ATOM_PANEL_MISC_DUAL               0x00000001
#define ATOM_PANEL_MISC_888RGB             0x00000002
#define ATOM_PANEL_MISC_GREY_LEVEL         0x0000000C
#define ATOM_PANEL_MISC_FPDI               0x00000010
#define ATOM_PANEL_MISC_GREY_LEVEL_SHIFT   2
#define ATOM_PANEL_MISC_SPATIAL            0x00000020
#define ATOM_PANEL_MISC_TEMPORAL           0x00000040
#define ATOM_PANEL_MISC_API_ENABLED        0x00000080

#define MEMTYPE_DDR1                       "DDR1"
#define MEMTYPE_DDR2                       "DDR2"
#define MEMTYPE_DDR3                       "DDR3"
#define MEMTYPE_DDR4                       "DDR4"

#define ASIC_BUS_TYPE_PCI                  "PCI"
#define ASIC_BUS_TYPE_AGP                  "AGP"
#define ASIC_BUS_TYPE_PCIE                 "PCI_EXPRESS"

//Maximum size of that FireGL flag string
#define ATOM_FIREGL_FLAG_STRING            "FGL"      //Flag used to enable FireGL Support
#define ATOM_MAX_SIZE_OF_FIREGL_FLAG_STRING     3     //sizeof( ATOM_FIREGL_FLAG_STRING )

#define ATOM_FAKE_DESKTOP_STRING           "DSK"      //Flag used to enable mobile ASIC on Desktop
#define ATOM_MAX_SIZE_OF_FAKE_DESKTOP_STRING    ATOM_MAX_SIZE_OF_FIREGL_FLAG_STRING

#define ATOM_M54T_FLAG_STRING              "M54T"     //Flag used to enable M54T Support
#define ATOM_MAX_SIZE_OF_M54T_FLAG_STRING  4          //sizeof( ATOM_M54T_FLAG_STRING )

#define HW_ASSISTED_I2C_STATUS_FAILURE     2
#define HW_ASSISTED_I2C_STATUS_SUCCESS     1

#pragma pack(1)                                       // BIOS data must use byte aligment

// Define offset to location of ROM header.
#define OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER         0x00000048L
#define OFFSET_TO_ATOM_ROM_IMAGE_SIZE                0x00000002L

#define OFFSET_TO_ATOMBIOS_ASIC_BUS_MEM_TYPE         0x94
#define MAXSIZE_OF_ATOMBIOS_ASIC_BUS_MEM_TYPE        20    //including the terminator 0x0!
#define OFFSET_TO_GET_ATOMBIOS_STRINGS_NUMBER      0x002f
#define OFFSET_TO_GET_ATOMBIOS_STRINGS_START       0x006e

/****************************************************************************/
// Common header for all tables (Data table, Command table).
// Every table pointed  _ATOM_MASTER_DATA_TABLE has this common header.
// And the pointer actually points to this header.
/****************************************************************************/

typedef struct _ATOM_COMMON_TABLE_HEADER
{
  USHORT usStructureSize;
  UCHAR  ucTableFormatRevision;   //Change it when the Parser is not backward compatible
  UCHAR  ucTableContentRevision;  //Change it only when the table needs to change but the firmware
                                  //Image can't be updated, while Driver needs to carry the new table!
}ATOM_COMMON_TABLE_HEADER;

/****************************************************************************/
// Structure stores the ROM header.
/****************************************************************************/
typedef struct _ATOM_ROM_HEADER
{
  ATOM_COMMON