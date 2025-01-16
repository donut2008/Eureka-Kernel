set;
  USHORT  usHSyncWidth;
  USHORT  usVSyncOffset;
  USHORT  usVSyncWidth;
  USHORT  usImageHSize;
  USHORT  usImageVSize;
  UCHAR   ucHBorder;
  UCHAR   ucVBorder;
  ATOM_MODE_MISC_INFO_ACCESS susModeMiscInfo;
  UCHAR   ucInternalModeNumber;
  UCHAR   ucRefreshRate;
}ATOM_DTD_FORMAT;

/****************************************************************************/
// Structure used in LVDS_InfoTable
//  * Need a document to describe this table
/****************************************************************************/
#define SUPPORTED_LCD_REFRESHRATE_30Hz          0x0004
#define SUPPORTED_LCD_REFRESHRATE_40Hz          0x0008
#define SUPPORTED_LCD_REFRESHRATE_50Hz          0x0010
#define SUPPORTED_LCD_REFRESHRATE_60Hz          0x0020
#define SUPPORTED_LCD_REFRESHRATE_48Hz          0x0040

//ucTableFormatRevision=1
//ucTableContentRevision=1
typedef struct _ATOM_LVDS_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_DTD_FORMAT     sLCDTiming;
  USHORT              usModePatchTableOffset;
  USHORT              usSupportedRefreshRate;     //Refer to panel info table in ATOMBIOS extension Spec.
  USHORT              usOffDelayInMs;
  UCHAR               ucPowerSequenceDigOntoDEin10Ms;
  UCHAR               ucPowerSequenceDEtoBLOnin10Ms;
  UCHAR               ucLVDS_Misc;               // Bit0:{=0:single, =1:dual},Bit1 {=0:666RGB, =1:888RGB},Bit2:3:{Grey level}
                                                 // Bit4:{=0:LDI format for RGB888, =1 FPDI format for RGB888}
                                                 // Bit5:{=0:Spatial Dithering disabled;1 Spatial Dithering enabled}
                                                 // Bit6:{=0:Temporal Dithering disabled;1 Temporal Dithering enabled}
  UCHAR               ucPanelDefaultRefreshRate;
  UCHAR               ucPanelIdentification;
  UCHAR               ucSS_Id;
}ATOM_LVDS_INFO;

//ucTableFormatRevision=1
//ucTableContentRevision=2
typedef struct _ATOM_LVDS_INFO_V12
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_DTD_FORMAT     sLCDTiming;
  USHORT              usExtInfoTableOffset;
  USHORT              usSupportedRefreshRate;     //Refer to panel info table in ATOMBIOS extension Spec.
  USHORT              usOffDelayInMs;
  UCHAR               ucPowerSequenceDigOntoDEin10Ms;
  UCHAR               ucPowerSequenceDEtoBLOnin10Ms;
  UCHAR               ucLVDS_Misc;               // Bit0:{=0:single, =1:dual},Bit1 {=0:666RGB, =1:888RGB},Bit2:3:{Grey level}
                                                 // Bit4:{=0:LDI format for RGB888, =1 FPDI format for RGB888}
                                                 // Bit5:{=0:Spatial Dithering disabled;1 Spatial Dithering enabled}
                                                 // Bit6:{=0:Temporal Dithering disabled;1 Temporal Dithering enabled}
  UCHAR               ucPanelDefaultRefreshRate;
  UCHAR               ucPanelIdentification;
  UCHAR               ucSS_Id;
  USHORT              usLCDVenderID;
  USHORT              usLCDProductID;
  UCHAR               ucLCDPanel_SpecialHandlingCap;
   UCHAR                        ucPanelInfoSize;               //  start from ATOM_DTD_FORMAT to end of panel info, include ExtInfoTable
  UCHAR               ucReserved[2];
}ATOM_LVDS_INFO_V12;

//Definitions for ucLCDPanel_SpecialHandlingCap:

//Once DAL sees this CAP is set, it will read EDID from LCD on its own instead of using sLCDTiming in ATOM_LVDS_INFO_V12.
//Other entries in ATOM_LVDS_INFO_V12 are still valid/useful to DAL
#define   LCDPANEL_CAP_READ_EDID                  0x1

//If a design supports DRR (dynamic refresh rate) on internal panels (LVDS or EDP), this cap is set in ucLCDPanel_SpecialHandlingCap together
//with multiple supported refresh rates@usSupportedRefreshRate. This cap should not be set when only slow refresh rate is supported (static
//refresh rate switch by SW. This is only valid from ATOM_LVDS_INFO_V12
#define   LCDPANEL_CAP_DRR_SUPPORTED              0x2

//Use this cap bit for a quick reference whether an embadded panel (LCD1 ) is LVDS or eDP.
#define   LCDPANEL_CAP_eDP                        0x4


//Color Bit Depth definition in EDID V1.4 @BYTE 14h
//Bit 6  5  4
                              //      0  0  0  -  Color bit depth is undefined
                              //      0  0  1  -  6 Bits per Primary Color
                              //      0  1  0  -  8 Bits per Primary Color
                              //      0  1  1  - 10 Bits per Primary Color
                              //      1  0  0  - 12 Bits per Primary Color
              