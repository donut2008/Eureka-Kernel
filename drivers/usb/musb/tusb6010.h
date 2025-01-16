ATOM_DTD_MODE_SUPPORT_TBL_SIZE)

#define ATOM_DFP4_EDID_ADDR             (ATOM_DFP3_STD_MODE_TBL_ADDR + ATOM_STD_MODE_SUPPORT_TBL_SIZE)
#define ATOM_DFP4_DTD_MODE_TBL_ADDR     (ATOM_DFP4_EDID_ADDR + ATOM_EDID_RAW_DATASIZE)
#define ATOM_DFP4_STD_MODE_TBL_ADDR     (ATOM_DFP4_DTD_MODE_TBL_ADDR + ATOM_DTD_MODE_SUPPORT_TBL_SIZE)

#define ATOM_DFP5_EDID_ADDR             (ATOM_DFP4_STD_MODE_TBL_ADDR + ATOM_STD_MODE_SUPPORT_TBL_SIZE)
#define ATOM_DFP5_DTD_MODE_TBL_ADDR     (ATOM_DFP5_EDID_ADDR + ATOM_EDID_RAW_DATASIZE)
#define ATOM_DFP5_STD_MODE_TBL_ADDR     (ATOM_DFP5_DTD_MODE_TBL_ADDR + ATOM_DTD_MODE_SUPPORT_TBL_SIZE)

#define ATOM_DP_TRAINING_TBL_ADDR       (ATOM_DFP5_STD_MODE_TBL_ADDR + ATOM_STD_MODE_SUPPORT_TBL_SIZE)

#define ATOM_STACK_STORAGE_START        (ATOM_DP_TRAINING_TBL_ADDR + 1024)
#define ATOM_STACK_STORAGE_END          ATOM_STACK_STORAGE_START + 512

//The size below is in Kb!
#define ATOM_VRAM_RESERVE_SIZE         ((((ATOM_STACK_STORAGE_END - ATOM_HWICON1_SURFACE_ADDR)>>10)+4)&0xFFFC)

#define ATOM_VRAM_RESERVE_V2_SIZE      32

#define   ATOM_VRAM_OPERATION_FLAGS_MASK         0xC0000000L
#define ATOM_VRAM_OPERATION_FLAGS_SHIFT        30
#define   ATOM_VRAM_BLOCK_NEEDS_NO_RESERVATION   0x1
#define   ATOM_VRAM_BLOCK_NEEDS_RESERVATION      0x0

/***********************************************************************************/
// Structure used in VRAM_UsageByFirmwareTable
// Note1: This table is filled by SetBiosReservationStartInFB in CoreCommSubs.asm
//        at running time.
// note2: From RV770, the memory is more than 32bit addressable, so we will change
//        ucTableFormatRevision=1,ucTableContentRevision=4, the strcuture remains
//        exactly same as 1.1 and 1.2 (1.3 is never in use), but ulStartAddrUsedByFirmware
//        (in offset to start of memory address) is KB aligned instead of byte aligend.
// Note3:
/* If we change usReserved to "usFBUsedbyDrvInKB", then to VBIOS this usFBUsedbyDrvInKB is a predefined, unchanged
constant across VGA or non VGA adapter,
for CAIL, The size of FB access area is known, only thing missing is the Offset of FB Access area, so we can  have:

If (ulStartAddrUsedByFirmware!=0)
FBAccessAreaOffset= ulStartAddrUsedByFirmware - usFBUsedbyDrvInKB;
Reserved area has been claimed by VBIOS including this FB access area; CAIL doesn't need to reserve any extra area for this purpose
else   //Non VGA case
 if (FB_Size<=2Gb)
    FBAccessAreaOffset= FB_Size - usFBUsedbyDrvInKB;
 else
     FBAccessAreaOffset= Aper_Size - usFBUsedbyDrvInKB

CAIL needs to claim an reserved area defined by FBAccessAreaOffset and usFBUsedbyDrvInKB in non VGA case.*/

/***********************************************************************************/
#define ATOM_MAX_FIRMWARE_VRAM_USAGE_INFO         1

typedef struct _ATOM_FIRMWARE_VRAM_RESERVE_INFO
{
  ULONG   ulStartAddrUsedByFirmware;
  USHORT  usFirmwareUseInKb;
  USHORT  usReserved;
}ATOM_FIRMWARE_VRAM_RESERVE_INFO;

typedef struct _ATOM_VRAM_USAGE_BY_FIRMWARE
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_FIRMWARE_VRAM_RESERVE_INFO   asFirmwareVramReserveInfo[ATOM_MAX_FIRMWARE_VRAM_USAGE_INFO];
}ATOM_VRAM_USAGE_BY_FIRMWARE;

// change verion to 1.5, when allow driver to allocate the vram area for command table access.
typedef struct _ATOM_FIRMWARE_VRAM_RESERVE_INFO_V1_5
{
  ULONG   ulStartAddrUsedByFirmware;
  USHORT  usFirmwareUseInKb;
  USHORT  usFBUsedByDrvInKb;
}ATOM_FIRMWARE_VRAM_RESERVE_INFO_V1_5;

typedef struct _ATOM_VRAM_USAGE_BY_FIRMWARE_V1_5
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_FIRMWARE_VRAM_RESERVE_INFO_V1_5   asFirmwareVramReserveInfo[ATOM_MAX_FIRMWARE_VRAM_USAGE_INFO];
}ATOM_VRAM_USAGE_BY_FIRMWARE_V1_5;

/****************************************************************************/
// Structure used in GPIO_Pin_LUTTable
/****************************************************************************/
typedef struct _ATOM_GPIO_PIN_ASSIGNMENT
{
  USHORT                   usGpioPin_AIndex;
  UCHAR                    ucGpioPinBitShift;
  UCHAR                    ucGPIO_ID;
}ATOM_GPIO_PIN_ASSIGNMENT;

//ucGPIO_ID pre-define id for multiple usage
// GPIO use to control PCIE_VDDC in certain SLT board
#define PCIE_VDDC_CONTROL_GPIO_PINID        56

//from SMU7.x, if ucGPIO_ID=PP_AC_DC_SWITCH_GPIO_PINID in GPIO_LUTTable, AC/DC swithing feature is enable
#define PP_AC_DC_SWITCH_GPIO_PINID          60
//from SMU7.x, if ucGPIO_ID=VDDC_REGULATOR_VRHOT_GPIO_PINID in GPIO_LUTable, VRHot feature is enable
#define VDDC_VRHOT_GPIO_PINID               61
//if ucGPIO_ID=VDDC_PCC_GPIO_PINID in GPIO_LUTable, Peak Current Control feature is enabled
#define VDDC_PCC_GPIO_PINID                 62
// Only used on certain SLT/PA board to allow utility to cut Efuse.
#define EFUSE_CUT_ENABLE_GPIO_PINID         63
// ucGPIO=DRAM_SELF_REFRESH_GPIO_PIND uses  for memory self refresh (ucGPIO=0, DRAM self-refresh; ucGPIO=
#define DRAM_SELF_REFRESH_GPIO_PINID        64
// Thermal interrupt output->system thermal chip GPIO pin
#define THERMAL_INT_OUTPUT_GPIO_PINID       65


typedef struct _ATOM_GPIO_PIN_LUT
{
  ATOM_COMMON_TABLE_HEADER  sHeader;
  ATOM_GPIO_PIN_ASSIGNMENT   asGPIO_Pin[1];
}ATOM_GPIO_PIN_LUT;

/****************************************************************************/
// Structure used in ComponentVideoInfoTable
/****************************************************************************/
#define GPIO_PIN_ACTIVE_HIGH          0x1
#define MAX_SUPPORTED_CV_STANDARDS    5

// definitions for ATOM_D_INFO.ucSettings
#define ATOM_GPIO_SETTINGS_BITSHIFT_MASK  0x1F    // [4:0]
#define ATOM_GPIO_SETTINGS_RESERVED_MASK  0x60    // [6:5] = must be zeroed out
#define ATOM_GPIO_SETTINGS_ACTIVE_MASK    0x80    // [7]

typedef struct _ATOM_GPIO_INFO
{
  USHORT  usAOffset;
  UCHAR   ucSettings;
  UCHAR   ucReserved;
}ATOM_GPIO_INFO;

// definitions for ATOM_COMPONENT_VIDEO_INFO.ucMiscInfo (bit vector)
#define ATOM_CV_RESTRICT_FORMAT_SELECTION           0x2

// definitions for ATOM_COMPONENT_VIDEO_INFO.uc480i/uc480p/uc720p/uc1080i
#define ATOM_GPIO_DEFAULT_MODE_EN                   0x80 //[7];
#define ATOM_GPIO_SETTING_PERMODE_MASK              0x7F //[6:0]

// definitions for ATOM_COMPONENT_VIDEO_INFO.ucLetterBoxMode
//Line 3 out put 5V.
#define ATOM_CV_LINE3_ASPECTRATIO_16_9_GPIO_A       0x01     //represent gpio 3 state for 16:9
#define ATOM_CV_LINE3_ASPECTRATIO_16_9_GPIO_B       0x02     //represent gpio 4 state for 16:9
#define ATOM_CV_LINE3_ASPECTRATIO_16_9_GPIO_SHIFT   0x0

//Line 3 out put 2.2V
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_LETBOX_GPIO_A 0x04     //represent gpio 3 state for 4:3 Letter box
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_LETBOX_GPIO_B 0x08     //represent gpio 4 state for 4:3 Letter box
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_LETBOX_GPIO_SHIFT 0x2

//Line 3 out put 0V
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_GPIO_A        0x10     //represent gpio 3 state for 4:3
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_GPIO_B        0x20     //represent gpio 4 state for 4:3
#define ATOM_CV_LINE3_ASPECTRATIO_4_3_GPIO_SHIFT    0x4

#define ATOM_CV_LINE3_ASPECTRATIO_MASK              0x3F     // bit [5:0]

#define ATOM_CV_LINE3_ASPECTRATIO_EXIST             0x80     //bit 7

//GPIO bit index in gpio setting per mode value, also represend the block no. in gpio blocks.
#define ATOM_GPIO_INDEX_LINE3_ASPECRATIO_GPIO_A   3   //bit 3 in uc480i/uc480p/uc720p/uc1080i, which represend the default gpio bit setting for the mode.
#define ATOM_GPIO_INDEX_LINE3_ASPECRATIO_GPIO_B   4   //bit 4 in uc480i/uc480p/uc720p/uc1080i, which represend the default gpio bit setting for the mode.


typedef struct _ATOM_COMPONENT_VIDEO_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  USHORT             usMask_PinRegisterIndex;
  USHORT             usEN_PinRegisterIndex;
  USHORT             usY_PinRegisterIndex;
  USHORT             usA_PinRegisterIndex;
  UCHAR              ucBitShift;
  UCHAR              ucPinActiveState;  //ucPinActiveState: Bit0=1 active high, =0 active low
  ATOM_DTD_FORMAT    sReserved;         // must be zeroed out
  UCHAR              ucMiscInfo;
  UCHAR              uc480i;
  UCHAR              uc480p;
  UCHAR              uc720p;
  UCHAR              uc1080i;
  UCHAR              ucLetterBoxMode;
  UCHAR              ucReserved[3];
  UCHAR              ucNumOfWbGpioBlocks; //For Component video D-Connector support. If zere, NTSC type connector
  ATOM_GPIO_INFO     aWbGpioStateBlock[MAX_SUPPORTED_CV_STANDARDS];
  ATOM_DTD_FORMAT    aModeTimings[MAX_SUPPORTED_CV_STANDARDS];
}ATOM_COMPONENT_VIDEO_INFO;

//ucTableFormatRevision=2
//ucTableContentRevision=1
typedef struct _ATOM_COMPONENT_VIDEO_INFO_V21
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  UCHAR              ucMiscInfo;
  UCHAR              uc480i;
  UCHAR              uc480p;
  UCHAR              uc720p;
  UCHAR              uc1080i;
  UCHAR              ucReserved;
  UCHAR              ucLetterBoxMode;
  UCHAR              ucNumOfWbGpioBlocks; //For Component video D-Connector support. If zere, NTSC type connector
  ATOM_GPIO_INFO     aWbGpioStateBlock[MAX_SUPPORTED_CV_STANDARDS];
  ATOM_DTD_FORMAT    aModeTimings[MAX_SUPPORTED_CV_STANDARDS];
}ATOM_COMPONENT_VIDEO_INFO_V21;

#define ATOM_COMPONENT_VIDEO_INFO_LAST  ATOM_COMPONENT_VIDEO_INFO_V21

/***********************