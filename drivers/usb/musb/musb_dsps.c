D1_PANEL_ID_MASKb0    0x0FF
#define ATOM_S4_LCD1_REFRESH_MASKb1     ATOM_S4_LCD1_PANEL_ID_MASKb0
#define ATOM_S4_VRAM_INFO_MASKb2        ATOM_S4_LCD1_PANEL_ID_MASKb0

// BIOS_5_SCRATCH Definition, BIOS_5_SCRATCH is used by Firmware only !!!!
#define ATOM_S5_DOS_REQ_CRT1b0          0x01
#define ATOM_S5_DOS_REQ_LCD1b0          0x02
#define ATOM_S5_DOS_REQ_TV1b0           0x04
#define ATOM_S5_DOS_REQ_DFP1b0          0x08
#define ATOM_S5_DOS_REQ_CRT2b0          0x10
#define ATOM_S5_DOS_REQ_LCD2b0          0x20
#define ATOM_S5_DOS_REQ_DFP6b0          0x40
#define ATOM_S5_DOS_REQ_DFP2b0          0x80
#define ATOM_S5_DOS_REQ_CVb1            0x01
#define ATOM_S5_DOS_REQ_DFP3b1          0x02
#define ATOM_S5_DOS_REQ_DFP4b1          0x04
#define ATOM_S5_DOS_REQ_DFP5b1          0x08


#define ATOM_S5_DOS_REQ_DEVICEw0        0x0FFF

#define ATOM_S5_DOS_REQ_CRT1            0x0001
#define ATOM_S5_DOS_REQ_LCD1            0x0002
#define ATOM_S5_DOS_REQ_TV1             0x0004
#define ATOM_S5_DOS_REQ_DFP1            0x0008
#define ATOM_S5_DOS_REQ_CRT2            0x0010
#define ATOM_S5_DOS_REQ_LCD2            0x0020
#define ATOM_S5_DOS_REQ_DFP6            0x0040
#define ATOM_S5_DOS_REQ_DFP2            0x0080
#define ATOM_S5_DOS_REQ_CV              0x0100
#define ATOM_S5_DOS_REQ_DFP3            0x0200
#define ATOM_S5_DOS_REQ_DFP4            0x0400
#define ATOM_S5_DOS_REQ_DFP5            0x0800

#define ATOM_S5_DOS_FORCE_CRT1b2        ATOM_S5_DOS_REQ_CRT1b0
#define ATOM_S5_DOS_FORCE_TV1b2         ATOM_S5_DOS_REQ_TV1b0
#define ATOM_S5_DOS_FORCE_CRT2b2        ATOM_S5_DOS_REQ_CRT2b0
#define ATOM_S5_DOS_FORCE_CVb3          ATOM_S5_DOS_REQ_CVb1
#define ATOM_S5_DOS_FORCE_DEVICEw1      (ATOM_S5_DOS_FORCE_CRT1b2+ATOM_S5_DOS_FORCE_TV1b2+ATOM_S5_DOS_FORCE_CRT2b2+\
                                        (ATOM_S5_DOS_FORCE_CVb3<<8))
// BIOS_6_SCRATCH Definition
#define ATOM_S6_DEVICE_CHANGE           0x00000001L
#define ATOM_S6_SCALER_CHANGE           0x00000002L
#define ATOM_S6_LID_CHANGE              0x00000004L
#define ATOM_S6_DOCKING_CHANGE          0x00000008L
#define ATOM_S6_ACC_MODE                0x00000010L
#define ATOM_S6_EXT_DESKTOP_MODE        0x00000020L
#define ATOM_S6_LID_STATE               0x00000040L
#define ATOM_S6_DOCK_STATE              0x00000080L
#define ATOM_S6_CRITICAL_STATE          0x00000100L
#define ATOM_S6_HW_I2C_BUSY_STATE       0x00000200L
#define ATOM_S6_THERMAL_STATE_CHANGE    0x00000400L
#define ATOM_S6_INTERRUPT_SET_BY_BIOS   0x00000800L
#define ATOM_S6_REQ_LCD_EXPANSION_FULL         0x00001000L //Normal expansion Request bit for LCD
#define ATOM_S6_REQ_LCD_EXPANSION_ASPEC_RATIO  0x00002000L //Aspect ratio expansion Request bit for LCD

#define ATOM_S6_DISPLAY_STATE_CHANGE    0x00004000L        //This bit is recycled when ATOM_BIOS_INFO_BIOS_SCRATCH6_SCL2_REDEFINE is set,previously it's SCL2_H_expansion
#define ATOM_S6_I2C_STATE_CHANGE        0x00008000L        //This bit is recycled,when ATOM_BIOS_INFO_BIOS_SCRATCH6_SCL2_REDEFINE is set,previously it's SCL2_V_expansion

#define ATOM_S6_ACC_REQ_CRT1            0x00010000L
#define ATOM_S6_ACC_REQ_LCD1            0x00020000L
#define ATOM_S6_ACC_REQ_TV1             0x00040000L
#define ATOM_S6_ACC_REQ_DFP1            0x00080000L
#define ATOM_S6_ACC_REQ_CRT2            0x00100000L
#define ATOM_S6_ACC_REQ_LCD2            0x00200000L
#define ATOM_S6_ACC_REQ_DFP6            0x00400000L
#define ATOM_S6_ACC_REQ_DFP2            0x00800000L
#define ATOM_S6_ACC_REQ_CV              0x01000000L
#define ATOM_S6_ACC_REQ_DFP3                  0x02000000L
#define ATOM_S6_ACC_REQ_DFP4                  0x04000000L
#define ATOM_S6_ACC_REQ_DFP5                  0x08000000L

#define ATOM_S6_ACC_REQ_MASK                0x0FFF0000L
#define ATOM_S6_SYSTEM_POWER_MODE_CHANGE    0x10000000L
#define ATOM_S6_ACC_BLOCK_DISPLAY_SWITCH    0x20000000L
#define ATOM_S6_VRI_BRIGHTNESS_CHANGE       0x40000000L
#define ATOM_S6_CONFIG_DISPLAY_CHANGE_MASK  0x80000000L

//Byte aligned defintion for BIOS usage
#define ATOM_S6_DEVICE_CHANGEb0         0x01
#define ATOM_S6_SCALER_CHANGEb0         0x02
#define ATOM_S6_LID_CHANGEb0            0x04
#define ATOM_S6_DOCKING_CHANGEb0        0x08
#define ATOM_S6_ACC_MODEb0              0x10
#define ATOM_S6_EXT_DESKTOP_MODEb0      0x20
#define ATOM_S6_LID_STATEb0             0x40
#define ATOM_S6_DOCK_STATEb0            0x80
#define ATOM_S6_CRITICAL_STATEb1        0x01
#define ATOM_S6_HW_I2C_BUSY_STATEb1     0x02
#define ATOM_S6_THERMAL_STATE_CHANGEb1  0x04
#define ATOM_S6_INTERRUPT_SET_BY_BIOSb1 0x08
#define ATOM_S6_REQ_LCD_EXPANSION_FULLb1        0x10
#define ATOM_S6_REQ_LCD_EXPANSION_ASPEC_RATIOb1 0x20

#define ATOM_S6_ACC_REQ_CRT1b2          0x01
#define ATOM_S6_ACC_REQ_LCD1b2          0x02
#define ATOM_S6_ACC_REQ_TV1b2           0x04
#define ATOM_S6_ACC_REQ_DFP1b2          0x08
#define ATOM_S6_ACC_REQ_CRT2b2          0x10
#define ATOM_S6_ACC_REQ_LCD2b2          0x20
#define ATOM_S6_ACC_REQ_DFP6b2          0x40
#define ATOM_S6_ACC_REQ_DFP2b2          0x80
#define ATOM_S6_ACC_REQ_CVb3            0x01
#define ATOM_S6_ACC_REQ_DFP3b3          0x02
#define ATOM_S6_ACC_REQ_DFP4b3          0x04
#define ATOM_S6_ACC_REQ_DFP5b3          0x08

#define ATOM_S6_ACC_REQ_DEVICEw1        ATOM_S5_DOS_REQ_DEVICEw0
#define ATOM_S6_SYSTEM_POWER_MODE_CHANGEb3 0x10
#define ATOM_S6_ACC_BLOCK_DISPLAY_SWITCHb3 0x20
#define ATOM_S6_VRI_BRIGHTNESS_CHANGEb3    0x40
#define ATOM_S6_CONFIG_DISPLAY_CHANGEb3    0x80

#define ATOM_S6_DEVICE_CHANGE_SHIFT             0
#define ATOM_S6_SCALER_CHANGE_SHIFT             1
#define ATOM_S6_LID_CHANGE_SHIFT                2
#define ATOM_S6_DOCKING_CHANGE_SHIFT            3
#define ATOM_S6_ACC_MODE_SHIFT                  4
#define ATOM_S6_EXT_DESKTOP_MODE_SHIFT          5
#define ATOM_S6_LID_STATE_SHIFT                 6
#define ATOM_S6_DOCK_STATE_SHIFT                7
#define ATOM_S6_CRITICAL_STATE_SHIFT            8
#define ATOM_S6_HW_I2C_BUSY_STATE_SHIFT         9
#define ATOM_S6_THERMAL_STATE_CHANGE_SHIFT      10
#define ATOM_S6_INTERRUPT_SET_BY_BIOS_SHIFT     11
#define ATOM_S6_REQ_SCALER_SHIFT                12
#define ATOM_S6_REQ_SCALER_ARATIO_SHIFT         13
#define ATOM_S6_DISPLAY_STATE_CHANGE_SHIFT      14
#define ATOM_S6_I2C_STATE_CHANGE_SHIFT          15
#define ATOM_S6_SYSTEM_POWER_MODE_CHANGE_SHIFT  28
#define ATOM_S6_ACC_BLOCK_DISPLAY_SWITCH_SHIFT  29
#define ATOM_S6_VRI_BRIGHTNESS_CHANGE_SHIFT     30
#define ATOM_S6_CONFIG_DISPLAY_CHANGE_SHIFT     31

// BIOS_7_SCRATCH Definition, BIOS_7_SCRATCH is used by Firmware only !!!!
#define ATOM_S7_DOS_MODE_TYPEb0             0x03
#define ATOM_S7_DOS_MODE_VGAb0              0x00
#define ATOM_S7_DOS_MODE_VESAb0             0x01
#define ATOM_S7_DOS_MODE_EXTb0              0x02
#define ATOM_S7_DOS_MODE_PIXEL_DEPTHb0      0x0C
#define ATOM_S7_DOS_MODE_PIXEL_FORMATb0     0xF0
#define ATOM_S7_DOS_8BIT_DAC_ENb1           0x01
#define ATOM_S7_ASIC_INIT_COMPLETEb1        0x02
#define ATOM_S7_ASIC_INIT_COMPLETE_MASK     0x00000200
#define ATOM_S7_DOS_MODE_NUMBERw1           0x0FFFF

#define ATOM_S7_DOS_8BIT_DAC_EN_SHIFT       8

// BIOS_8_SCRATCH Definition
#define ATOM_S8_I2C_CHANNEL_BUSY_MASK       0x00000FFFF
#define ATOM_S8_I2C_HW_ENGINE_BUSY_MASK     0x0FFFF0000

#define ATOM_S8_I2C_CHANNEL_BUSY_SHIFT      0
#define ATOM_S8_I2C_ENGINE_BUSY_SHIFT       16

// BIOS_9_SCRATCH Definition
#ifndef ATOM_S9_I2C_CHANNEL_COMPLETED_MASK
#define ATOM_S9_I2C_CHANNEL_COMPLETED_MASK  0x0000FFFF
#endif
#ifndef ATOM_S9_I2C_CHANNEL_ABORTED_MASK
#define ATOM_S9_I2C_CHANNEL_ABORTED_MASK    0xFFFF0000
#endif
#ifndef ATOM_S9_I2C_CHANNEL_COMPLETED_SHIFT
#define ATOM_S9_I2C_CHANNEL_COMPLETED_SHIFT 0
#endif
#ifndef ATOM_S9_I2C_CHANNEL_ABORTED_SHIFT
#define ATOM_S9_I2C_CHANNEL_ABORTED_SHIFT   16
#endif


#define ATOM_FLAG_SET                         0x20
#define ATOM_FLAG_CLEAR                       0
#define CLEAR_ATOM_S6_ACC_MODE                ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_ACC_MODE_SHIFT | ATOM_FLAG_CLEAR)
#define SET_ATOM_S6_DEVICE_CHANGE             ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_DEVICE_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_VRI_BRIGHTNESS_CHANGE     ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_VRI_BRIGHTNESS_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_SCALER_CHANGE             ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_SCALER_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_LID_CHANGE                ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_LID_CHANGE_SHIFT | ATOM_FLAG_SET)

#define SET_ATOM_S6_LID_STATE                 ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_LID_STATE_SHIFT | ATOM_FLAG_SET)
#define CLEAR_ATOM_S6_LID_STATE               ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_LID_STATE_SHIFT | ATOM_FLAG_CLEAR)

#define SET_ATOM_S6_DOCK_CHANGE                   ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_DOCKING_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_DOCK_STATE                ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_DOCK_STATE_SHIFT | ATOM_FLAG_SET)
#define CLEAR_ATOM_S6_DOCK_STATE              ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_DOCK_STATE_SHIFT | ATOM_FLAG_CLEAR)

#define SET_ATOM_S6_THERMAL_STATE_CHANGE      ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_THERMAL_STATE_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_SYSTEM_POWER_MODE_CHANGE  ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_SYSTEM_POWER_MODE_CHANGE_SHIFT | ATOM_FLAG_SET)
#define SET_ATOM_S6_INTERRUPT_SET_BY_BIOS     ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_INTERRUPT_SET_BY_BIOS_SHIFT | ATOM_FLAG_SET)

#define SET_ATOM_S6_CRITICAL_STATE            ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_CRITICAL_STATE_SHIFT | ATOM_FLAG_SET)
#define CLEAR_ATOM_S6_CRITICAL_STATE          ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_CRITICAL_STATE_SHIFT | ATOM_FLAG_CLEAR)

#define SET_ATOM_S6_REQ_SCALER                ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_REQ_SCALER_SHIFT | ATOM_FLAG_SET)
#define CLEAR_ATOM_S6_REQ_SCALER              ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_REQ_SCALER_SHIFT | ATOM_FLAG_CLEAR )

#define SET_ATOM_S6_REQ_SCALER_ARATIO         ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_REQ_SCALER_ARATIO_SHIFT | ATOM_FLAG_SET )
#define CLEAR_ATOM_S6_REQ_SCALER_ARATIO       ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_REQ_SCALER_ARATIO_SHIFT | ATOM_FLAG_CLEAR )

#define SET_ATOM_S6_I2C_STATE_CHANGE          ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_I2C_STATE_CHANGE_SHIFT | ATOM_FLAG_SET )

#define SET_ATOM_S6_DISPLAY_STATE_CHANGE      ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_DISPLAY_STATE_CHANGE_SHIFT | ATOM_FLAG_SET )

#define SET_ATOM_S6_DEVICE_RECONFIG           ((ATOM_ACC_CHANGE_INFO_DEF << 8 )|ATOM_S6_CONFIG_DISPLAY_CHANGE_SHIFT | ATOM_FLAG_SET)
#define CLEAR_ATOM_S0_LCD1                    ((ATOM_DEVICE_CONNECT_INFO_DEF << 8 )|  ATOM_S0_LCD1_SHIFT | ATOM_FLAG_CLEAR )
#define SET_ATOM_S7_DOS_8BIT_DAC_EN           ((ATOM_DOS_MODE_INFO_DEF << 8 )|ATOM_S7_DOS_8BIT_DAC_EN_SHIFT | ATOM_FLAG_SET )
#define CLEAR_ATOM_S7_DOS_8BIT_DAC_EN         ((ATOM_DOS_MODE_INFO_DEF << 8 )|ATOM_S7_DOS_8BIT_DAC_EN_SHIFT | ATOM_FLAG_CLEAR )

/****************************************************************************/
//Portion II: Definitinos only used in Driver
/****************************************************************************/

// Macros used by driver

#ifdef __cplusplus
#define GetIndexIntoMasterTable(MasterOrData, FieldName) ((reinterpret_cast<char*>(&(static_cast<ATOM_MASTER_LIST_OF_##MasterOrData##_TABLES*>(0))->FieldName)-static_cast<char*>(0))/sizeof(USHORT))

#define GET_COMMAND_TABLE_COMMANDSET_REVISION(TABLE_HEADER_OFFSET) (((static_cast<ATOM_COMMON_TABLE_HEADER*>(TABLE_HEADER_OFFSET))->ucTableFormatRevision )&0x3F)
#define GET_COMMAND_TABLE_PARAMETER_REVISION(TABLE_HEADER_OFFSET)  (((static_cast<ATOM_COMMON_TABLE_HEADER*>(TABLE_HEADER_OFFSET))->ucTableContentRevision)&0x3F)
#else // not __cplusplus
#define   GetIndexIntoMasterTable(MasterOrData, FieldName) (((char*)(&((ATOM_MASTER_LIST_OF_##MasterOrData##_TABLES*)0)->FieldName)-(char*)0)/sizeof(USHORT))

#define GET_COMMAND_TABLE_COMMANDSET_REVISION(TABLE_HEADER_OFFSET) ((((ATOM_COMMON_TABLE_HEADER*)TABLE_HEADER_OFFSET)->ucTableFormatRevision)&0x3F)
#define GET_COMMAND_TABLE_PARAMETER_REVISION(TABLE_HEADER_OFFSET)  ((((ATOM_COMMON_TABLE_HEADER*)TABLE_HEADER_OFFSET)->ucTableContentRevision)&0x3F)
#endif // __cplusplus

#define GET_DATA_TABLE_MAJOR_REVISION GET_COMMAND_TABLE_COMMANDSET_REVISION
#define GET_DATA_TABLE_MINOR_REVISION GET_COMMAND_TABLE_PARAMETER_REVISION

/****************************************************************************/
//Portion III: Definitinos only used in VBIOS
/****************************************************************************/
#define ATOM_DAC_SRC               0x80
#define ATOM_SRC_DAC1               0
#define ATOM_SRC_DAC2               0x80



typedef struct _MEMORY_PLLINIT_PARAMETERS
{
  ULONG ulTargetMemoryClock; //In 10Khz unit
  UCHAR   ucAction;                //not define yet
  UCHAR   ucFbDiv_Hi;             //Fbdiv Hi byte
  UCHAR   ucFbDiv;                //FB value
  UCHAR   ucPostDiv;             //Post div
}MEMORY_PLLINIT_PARAMETERS;

#define MEMORY_PLLINIT_PS_ALLOCATION  MEMORY_PLLINIT_PARAMETERS


#define   GPIO_PIN_WRITE                                       0x01
#define   GPIO_PIN_READ                                          0x00

typedef struct  _GPIO_PIN_CONTROL_PARAMETERS
{
  UCHAR ucGPIO_ID;           //return value, read from GPIO pins
  UCHAR ucGPIOBitShift;        //define which bit in uGPIOBitVal need to be update
   UCHAR ucGPIOBitVal;           //Set/Reset corresponding bit defined in ucGPIOBitMask
  UCHAR ucAction;                 //=GPIO_PIN_WRITE: Read; =GPIO_PIN_READ: Write
}GPIO_PIN_CONTROL_PARAMETERS;

typedef struct _ENABLE_SCALER_PARAMETERS
{
  UCHAR ucScaler;            // ATOM_SCALER1, ATOM_SCALER2
  UCHAR ucEnable;            // ATOM_SCALER_DISABLE or ATOM_SCALER_CENTER or ATOM_SCALER_EXPANSION
  UCHAR ucTVStandard;        //
  UCHAR ucPadding[1];
}ENABLE_SCALER_PARAMETERS;
#define ENABLE_SCALER_PS_ALLOCATION ENABLE_SCALER_PARAMETERS

//ucEnable:
#define SCALER_BYPASS_AUTO_CENTER_NO_REPLICATION    0
#define SCALER_BYPASS_AUTO_CENTER_AUTO_REPLICATION  1
#define SCALER_ENABLE_2TAP_ALPHA_MODE               2
#define SCALER_ENABLE_MULTITAP_MODE                 3

typedef struct _ENABLE_HARDWARE_ICON_CURSOR_PARAMETERS
{
  ULONG  usHWIconHorzVertPosn;        // Hardware Icon Vertical position
  UCHAR  ucHWIconVertOffset;          // Hardware Icon Vertical offset
  UCHAR  ucHWIconHorzOffset;          // Hardware Icon Horizontal offset
  UCHAR  ucSelection;                 // ATOM_CURSOR1 or ATOM_ICON1 or ATOM_CURSOR2 or ATOM_ICON2
  UCHAR  ucEnable;                    // ATOM_ENABLE or ATOM_DISABLE
}ENABLE_HARDWARE_ICON_CURSOR_PARAMETERS;

typedef struct _ENABLE_HARDWARE_ICON_CURSOR_PS_ALLOCATION
{
  ENABLE_HARDWARE_ICON_CURSOR_PARAMETERS  sEnableIcon;
  ENABLE_CRTC_PARAMETERS                  sReserved;
}ENABLE_HARDWARE_ICON_CURSOR_PS_ALLOCATION;

typedef struct _ENABLE_GRAPH_SURFACE_PARAMETERS
{
  USHORT usHight;                     // Image Hight
  USHORT usWidth;                     // Image Width
  UCHAR  ucSurface;                   // Surface 1 or 2
  UCHAR  ucPadding[3];
}ENABLE_GRAPH_SURFACE_PARAMETERS;

typedef struct _ENABLE_GRAPH_SURFACE_PARAMETERS_V1_2
{
  USHORT usHight;                     // Image Hight
  USHORT usWidth;                     // Image Width
  UCHAR  ucSurface;                   // Surface 1 or 2
  UCHAR  ucEnable;                    // ATOM_ENABLE or ATOM_DISABLE
  UCHAR  ucPadding[2];
}ENABLE_GRAPH_SURFACE_PARAMETERS_V1_2;

typedef struct _ENABLE_GRAPH_SURFACE_PARAMETERS_V1_3
{
  USHORT usHight;                     // Image Hight
  USHORT usWidth;                     // Image Width
  UCHAR  ucSurface;                   // Surface 1 or 2
  UCHAR  ucEnable;                    // ATOM_ENABLE or ATOM_DISABLE
  USHORT usDeviceId;                  // Active Device Id for this surface. If no device, set to 0.
}ENABLE_GRAPH_SURFACE_PARAMETERS_V1_3;

typedef struct _ENABLE_GRAPH_SURFACE_PARAMETERS_V1_4
{
  USHORT usHight;                     // Image Hight
  USHORT usWidth;                     // Image Width
  USHORT usGraphPitch;
  UCHAR  ucColorDepth;
  UCHAR  ucPixelFormat;
  UCHAR  ucSurface;                   // Surface 1 or 2
  UCHAR  ucEnable;                    // ATOM_ENABLE or ATOM_DISABLE
  UCHAR  ucModeType;
  UCHAR  ucReserved;
}ENABLE_GRAPH_SURFACE_PARAMETERS_V1_4;

// ucEnable
#define ATOM_GRAPH_CONTROL_SET_PITCH             0x0f
#define ATOM_GRAPH_CONTROL_SET_DISP_START        0x10

typedef struct _ENABLE_GRAPH_SURFACE_PS_ALLOCATION
{
  ENABLE_GRAPH_SURFACE_PARAMETERS sSetSurface;
  ENABLE_YUV_PS_ALLOCATION        sReserved; // Don't set this one
}ENABLE_GRAPH_SURFACE_PS_ALLOCATION;

typedef struct _MEMORY_CLEAN_UP_PARAMETERS
{
  USHORT  usMemoryStart;                //in 8Kb boundry, offset from memory base address
  USHORT  usMemorySize;                 //8Kb blocks aligned
}MEMORY_CLEAN_UP_PARAMETERS;

#define MEMORY_CLEAN_UP_PS_ALLOCATION MEMORY_CLEAN_UP_PARAMETERS

typedef struct  _GET_DISPLAY_SURFACE_SIZE_PARAMETERS
{
  USHORT  usX_Size;                     //When use as input parameter, usX_Size indicates which CRTC
  USHORT  usY_Size;
}GET_DISPLAY_SURFACE_SIZE_PARAMETERS;

typedef struct  _GET_DISPLAY_SURFACE_SIZE_PARAMETERS_V2
{
  union{
    USHORT  usX_Size;                     //When use as input parameter, usX_Size indicates which CRTC
    USHORT  usSurface;
  };
  USHORT usY_Size;
  USHORT usDispXStart;
  USHORT usDispYStart;
}GET_DISPLAY_SURFACE_SIZE_PARAMETERS_V2;


typedef struct _PALETTE_DATA_CONTROL_PARAMETERS_V3
{
  UCHAR  ucLutId;
  UCHAR  ucAction;
  USHORT usLutStartIndex;
  USHORT usLutLength;
  USHORT usLutOffsetInVram;
}PALETTE_DATA_CONTROL_PARAMETERS_V3;

// ucAction:
#define PALETTE_DATA_AUTO_FILL            1
#define PALETTE_DATA_READ                 2
#define PALETTE_DATA_WRITE                3


typedef struct _INTERRUPT_SERVICE_PARAMETERS_V2
{
  UCHAR  ucInterruptId;
  UCHAR  ucServiceId;
  UCHAR  ucStatus;
  UCHAR  ucReserved;
}INTERRUPT_SERVICE_PARAMETER_V2;

// ucInterruptId
#define HDP1_INTERRUPT_ID                 1
#define HDP2_INTERRUPT_ID                 2
#define HDP3_INTERRUPT_ID                 3
#define HDP4_INTERRUPT_ID                 4
#define HDP5_INTERRUPT_ID                 5
#define HDP6_INTERRUPT_ID                 6
#define SW_INTERRUPT_ID                   11

// ucAction
#define INTERRUPT_SERVICE_GEN_SW_INT      1
#define INTERRUPT_SERVICE_GET_STATUS      2

 // ucStatus
#define INTERRUPT_STATUS__INT_TRIGGER     1
#define INTERRUPT_STATUS__HPD_HIGH        2

typedef struct _EFUSE_INPUT_PARAMETER
{
  USHORT usEfuseIndex;
  UCHAR  ucBitShift;
  UCHAR  ucBitLength;
}EFUSE_INPUT_PARAMETER;

// ReadEfuseValue command table input/output parameter
typedef union _READ_EFUSE_VALUE_PARAMETER
{
  EFUSE_INPUT_PARAMETER sEfuse;
  ULONG                 ulEfuseValue;
}READ_EFUSE_VALUE_PARAMETER;

typedef struct _INDIRECT_IO_ACCESS
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  UCHAR                    IOAccessSequence[256];
} INDIRECT_IO_ACCESS;

#define INDIRECT_READ              0x00
#define INDIRECT_WRITE             0x80

#define INDIRECT_IO_MM             0
#define INDIRECT_IO_PLL            1
#define INDIRECT_IO_MC             2
#define INDIRECT_IO_PCIE           3
#define INDIRECT_IO_PCIEP          4
#define INDIRECT_IO_NBMISC         5
#define INDIRECT_IO_SMU            5

#define INDIRECT_IO_PLL_READ       INDIRECT_IO_PLL   | INDIRECT_READ
#define INDIRECT_IO_PLL_WRITE      INDIRECT_IO_PLL   | INDIRECT_WRITE
#define INDIRECT_IO_MC_READ        INDIRECT_IO_MC    | INDIRECT_READ
#define INDIRECT_IO_MC_WRITE       INDIRECT_IO_MC    | INDIRECT_WRITE
#define INDIRECT_IO_PCIE_READ      INDIRECT_IO_PCIE  | INDIRECT_READ
#define INDIRECT_IO_PCIE_WRITE     INDIRECT_IO_PCIE  | INDIRECT_WRITE
#define INDIRECT_IO_PCIEP_READ     INDIRECT_IO_PCIEP | INDIRECT_READ
#define INDIRECT_IO_PCIEP_WRITE    INDIRECT_IO_PCIEP | INDIRECT_WRITE
#define INDIRECT_IO_NBMISC_READ    INDIRECT_IO_NBMISC | INDIRECT_READ
#define INDIRECT_IO_NBMISC_WRITE   INDIRECT_IO_NBMISC | INDIRECT_WRITE
#define INDIRECT_IO_SMU_READ       INDIRECT_IO_SMU | INDIRECT_READ
#define INDIRECT_IO_SMU_WRITE      INDIRECT_IO_SMU | INDIRECT_WRITE


typedef struct _ATOM_OEM_INFO
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ATOM_I2C_ID_CONFIG_ACCESS sucI2cId;
}ATOM_OEM_INFO;

typedef struct _ATOM_TV_MODE
{
   UCHAR   ucVMode_Num;           //Video mode number
   UCHAR   ucTV_Mode_Num;         //Internal TV mode number
}ATOM_TV_MODE;

typedef struct _ATOM_BIOS_INT_TVSTD_MODE
{
  ATOM_COMMON_TABLE_HEADER sHeader;
   USHORT   usTV_Mode_LUT_Offset;   // Pointer to standard to internal number conversion table
   USHORT   usTV_FIFO_Offset;        // Pointer to FIFO entry table
   USHORT   usNTSC_Tbl_Offset;      // Pointer to SDTV_Mode_NTSC table
   USHORT   usPAL_Tbl_Offset;        // Pointer to SDTV_Mode_PAL table
   USHORT   usCV_Tbl_Offset;        // Pointer to SDTV_Mode_PAL table
}ATOM_BIOS_INT_TVSTD_MODE;


typedef struct _ATOM_TV_MODE_SCALER_PTR
{
   USHORT   ucFilter0_Offset;      //Pointer to filter format 0 coefficients
   USHORT   usFilter1_Offset;      //Pointer to filter format 0 coefficients
   UCHAR   ucTV_Mode_Num;
}ATOM_TV_MODE_SCALER_PTR;

typedef struct _ATOM_STANDARD_VESA_TIMING
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  ATOM_DTD_FORMAT              aModeTimings[16];      // 16 is not the real array number, just for initial allocation
}ATOM_STANDARD_VESA_TIMING;


typedef struct _ATOM_STD_FORMAT
{
  USHORT    usSTD_HDisp;
  USHORT    usSTD_VDisp;
  USHORT    usSTD_RefreshRate;
  USHORT    usReserved;
}ATOM_STD_FORMAT;

typedef struct _ATOM_VESA_TO_EXTENDED_MODE
{
  USHORT  usVESA_ModeNumber;
  USHORT  usExtendedModeNumber;
}ATOM_VESA_TO_EXTENDED_MODE;

typedef struct _ATOM_VESA_TO_INTENAL_MODE_LUT
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ATOM_VESA_TO_EXTENDED_MODE asVESA_ToExtendedModeInfo[76];
}ATOM_VESA_TO_INTENAL_MODE_LUT;

/*************** ATOM Memory Related Data Structure ***********************/
typedef struct _ATOM_MEMORY_VENDOR_BLOCK{
   UCHAR                                    ucMemoryType;
   UCHAR                                    ucMemoryVendor;
   UCHAR                                    ucAdjMCId;
   UCHAR                                    ucDynClkId;
   ULONG                                    ulDllResetClkRange;
}ATOM_MEMORY_VENDOR_BLOCK;


typedef struct _ATOM_MEMORY_SETTING_ID_CONFIG{
#if ATOM_BIG_ENDIAN
	ULONG												ucMemBlkId:8;
	ULONG												ulMemClockRange:24;
#else
	ULONG												ulMemClockRange:24;
	ULONG												ucMemBlkId:8;
#endif
}ATOM_MEMORY_SETTING_ID_CONFIG;

typedef union _ATOM_MEMORY_SETTING_ID_CONFIG_ACCESS
{
  ATOM_MEMORY_SETTING_ID_CONFIG slAccess;
  ULONG                         ulAccess;
}ATOM_MEMORY_SETTING_ID_CONFIG_ACCESS;


typedef struct _ATOM_MEMORY_SETTING_DATA_BLOCK{
   ATOM_MEMORY_SETTING_ID_CONFIG_ACCESS  ulMemoryID;
   ULONG                                 aulMemData[1];
}ATOM_MEMORY_SETTING_DATA_BLOCK;


typedef struct _ATOM_INIT_REG_INDEX_FORMAT{
    USHORT usRegIndex;                                     // MC register index
    UCHAR  ucPreRegDataLength;                             // offset in ATOM_INIT_REG_DATA_BLOCK.saRegDataBuf
}ATOM_INIT_REG_INDEX_FORMAT;


typedef struct _ATOM_INIT_REG_BLOCK{
   USHORT                           usRegIndexTblSize;          //size of asRegIndexBuf
   USHORT                           usRegDataBlkSize;           //size of ATOM_MEMORY_SETTING_DATA_BLOCK
   ATOM_INIT_REG_INDEX_FORMAT       asRegIndexBuf[1];
   ATOM_MEMORY_SETTING_DATA_BLOCK   asRegDataBuf[1];
}ATOM_INIT_REG_BLOCK;

#define END_OF_REG_INDEX_BLOCK  0x0ffff
#define END_OF_REG_DATA_BLOCK   0x00000000
#define ATOM_INIT_REG_MASK_FLAG 0x80               //Not used in BIOS
#define CLOCK_RANGE_HIGHEST     0x00ffffff

#define VALUE_DWORD             SIZEOF ULONG
#define VALUE_SAME_AS_ABOVE     0
#define VALUE_MASK_DWORD        0x84

#define INDEX_ACCESS_RANGE_BEGIN       (VALUE_DWORD + 1)
#define INDEX_ACCESS_RANGE_END          (INDEX_ACCESS_RANGE_BEGIN + 1)
#define VALUE_INDEX_ACCESS_SINGLE       (INDEX_ACCESS_RANGE_END + 1)
//#define ACCESS_MCIODEBUGIND            0x40       //defined in BIOS code
#define ACCESS_PLACEHOLDER             0x80


typedef struct _ATOM_MC_INIT_PARAM_TABLE
{
  ATOM_COMMON_TABLE_HEADER      sHeader;
  USHORT                        usAdjustARB_SEQDataOffset;
  USHORT                        usMCInitMemTypeTblOffset;
  USHORT                        usMCInitCommonTblOffset;
  USHORT                        usMCInitPowerDownTblOffset;
  ULONG                         ulARB_SEQDataBuf[32];
  ATOM_INIT_REG_BLOCK           asMCInitMemType;
  ATOM_INIT_REG_BLOCK           asMCInitCommon;
}ATOM_MC_INIT_PARAM_TABLE;


typedef struct _ATOM_REG_INIT_SETTING
{
  USHORT  usRegIndex;
  ULONG   ulRegValue;
}ATOM_REG_INIT_SETTING;

typedef struct _ATOM_MC_INIT_PARAM_TABLE_V2_1
{
  ATOM_COMMON_TABLE_HEADER      sHeader;
  ULONG                         ulMCUcodeVersion;
  ULONG                         ulMCUcodeRomStartAddr;
  ULONG                         ulMCUcodeLength;
  USHORT                        usMcRegInitTableOffset;     // offset of ATOM_REG_INIT_SETTING array for MC core register settings.
  USHORT                        usReserved;                 // offset of ATOM_INIT_REG_BLOCK for MC SEQ/PHY register setting
}ATOM_MC_INIT_PARAM_TABLE_V2_1;