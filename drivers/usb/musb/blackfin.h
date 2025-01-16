Reservd;
  ULONG               ulReserved[2];
}ATOM_LCD_INFO_V13;

#define ATOM_LCD_INFO_LAST  ATOM_LCD_INFO_V13

//Definitions for ucLCD_Misc
#define ATOM_PANEL_MISC_V13_DUAL                   0x00000001
#define ATOM_PANEL_MISC_V13_FPDI                   0x00000002
#define ATOM_PANEL_MISC_V13_GREY_LEVEL             0x0000000C
#define ATOM_PANEL_MISC_V13_GREY_LEVEL_SHIFT       2
#define ATOM_PANEL_MISC_V13_COLOR_BIT_DEPTH_MASK   0x70
#define ATOM_PANEL_MISC_V13_6BIT_PER_COLOR         0x10
#define ATOM_PANEL_MISC_V13_8BIT_PER_COLOR         0x20

//Color Bit Depth definition in EDID V1.4 @BYTE 14h
//Bit 6  5  4
                              //      0  0  0  -  Color bit depth is undefined
                              //      0  0  1  -  6 Bits per Primary Color
                              //      0  1  0  -  8 Bits per Primary Color
                              //      0  1  1  - 10 Bits per Primary Color
                              //      1  0  0  - 12 Bits per Primary Color
                              //      1  0  1  - 14 Bits per Primary Color
                              //      1  1  0  - 16 Bits per Primary Color
                              //      1  1  1  - Reserved

//Definitions for ucLCDPanel_SpecialHandlingCap:

//Once DAL sees this CAP is set, it will read EDID from LCD on its own instead of using sLCDTiming in ATOM_LVDS_INFO_V12.
//Other entries in ATOM_LVDS_INFO_V12 are still valid/useful to DAL
#define   LCDPANEL_CAP_V13_READ_EDID              0x1        // = LCDPANEL_CAP_READ_EDID no change comparing to previous version

//If a design supports DRR (dynamic refresh rate) on internal panels (LVDS or EDP), this cap is set in ucLCDPanel_SpecialHandlingCap together
//with multiple supported refresh rates@usSupportedRefreshRate. This cap should not be set when only slow refresh rate is supported (static
//refresh rate switch by SW. This is only valid from ATOM_LVDS_INFO_V12
#define   LCDPANEL_CAP_V13_DRR_SUPPORTED          0x2        // = LCDPANEL_CAP_DRR_SUPPORTED no change comparing to previous version

//Use this cap bit for a quick reference whether an embadded panel (LCD1 ) is LVDS or eDP.
#define   LCDPANEL_CAP_V13_eDP                    0x4        // = LCDPANEL_CAP_eDP no change comparing to previous version

//uceDPToLVDSRxId
#define eDP_TO_LVDS_RX_DISABLE                  0x00       // no eDP->LVDS translator chip
#define eDP_TO_LVDS_COMMON_ID                   0x01       // common eDP->LVDS translator chip without AMD SW init
#define eDP_TO_LVDS_RT_ID                       0x02       // RT tansaltor which require AMD SW init

typedef struct  _ATOM_PATCH_RECORD_MODE
{
  UCHAR     ucRecordType;
  USHORT    usHDisp;
  USHORT    usVDisp;
}ATOM_PATCH_RECORD_MODE;

typedef struct  _ATOM_LCD