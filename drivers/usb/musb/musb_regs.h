et;
  USHORT usCRC_BlockOffset;
  USHORT usBIOS_BootupMessageOffset;
  USHORT usInt10Offset;
  USHORT usPciBusDevInitCode;
  USHORT usIoBaseAddress;
  USHORT usSubsystemVendorID;
  USHORT usSubsystemID;
  USHORT usPCI_InfoOffset;
  USHORT usMasterCommandTableOffset;//Offest for SW to get all command table offsets, Don't change the position
  USHORT usMasterDataTableOffset;   //Offest for SW to get all data table offsets, Don't change the position
  UCHAR  ucExtendedFunctionCode;
  UCHAR  ucReserved;
}ATOM_ROM_HEADER;

//==============================Command Table Portion====================================


/****************************************************************************/
// Structures used in Command.mtb
/****************************************************************************/
typedef struct _ATOM_MASTER_LIST_OF_COMMAND_TABLES{
  USHORT ASIC_Init;                              //Function Table, used by various SW components,latest version 1.1
  USHORT GetDisplaySurfaceSize;                  //Atomic Table,  Used by Bios when enabling HW ICON
  USHORT ASIC_RegistersInit;                     //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
  USHORT VRAM_BlockVenderDetection;              //Atomic Table,  used only by Bios
  USHORT DIGxEncoderControl;                     //Only used by Bios
  USHORT MemoryControllerInit;                   //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
  USHORT EnableCRTCMemReq;                       //Function Table,directly used by various SW components,latest version 2.1
  USHORT MemoryParamAdjust;                      //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock if needed
  USHORT DVOEncoderControl;                      //Function Table,directly used by various SW components,latest version 1.2
  USHORT GPIOPinControl;                         //Atomic Table,  only used by Bios
  USHORT SetEngineClock;                         //Function Table,directly used by various SW components,latest version 1.1
  USHORT SetMemoryClock;                         //Function Table,directly used by various SW components,latest version 1.1
  USHORT SetPixelClock;                          //Function Table,directly used by various SW components,latest version 1.2
  USHORT EnableDispPowerGating;                  //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
  USHORT ResetMemoryDLL;                         //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT ResetMemoryDevice;                      //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT MemoryPLLInit;                          //Atomic Table,  used only by Bios
  USHORT AdjustDisplayPll;                       //Atomic Table,  used by various SW componentes.
  USHORT AdjustMemoryController;                 //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT EnableASIC_StaticPwrMgt;                //Atomic Table,  only used by Bios
  USHORT SetUniphyInstance;                      //Atomic Table,  only used by Bios
  USHORT DAC_LoadDetection;                      //Atomic Table,  directly used by various SW components,latest version 1.2
  USHORT LVTMAEncoderControl;                    //Atomic Table,directly used by various SW components,latest version 1.3
  USHORT HW_Misc_Operation;                      //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT DAC1EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT DAC2EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT DVOOutputControl;                       //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT CV1OutputControl;                       //Atomic Table,  Atomic Table,  Obsolete from Ry6xx, use DAC2 Output instead
  USHORT GetConditionalGoldenSetting;            //Only used by Bios
  USHORT SMC_Init;                               //Function Table,directly used by various SW components,latest version 1.1
  USHORT PatchMCSetting;                         //only used by BIOS
  USHORT MC_SEQ_Control;                         //only used by BIOS
  USHORT Gfx_Harvesting;                         //Atomic Table,  Obsolete from Ry6xx, Now only used by BIOS for GFX harvesting
  USHORT EnableScaler;                           //Atomic Table,  used only by Bios
  USHORT BlankCRTC;                              //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT EnableCRTC;                             //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT GetPixelClock;                          //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT EnableVGA_Render;                       //Function Table,directly used by various SW components,latest version 1.1
  USHORT GetSCLKOverMCLKRatio;                   //Atomic Table,  only used by Bios
  USHORT SetCRTC_Timing;                         //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT SetCRTC_OverScan;                       //Atomic Table,  used by various SW components,latest version 1.1
  USHORT SetCRTC_Replication;                    //Atomic Table,  used only by Bios
  USHORT SelectCRTC_Source;                      //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT EnableGraphSurfaces;                    //Atomic Table,  used only by Bios
  USHORT UpdateCRTC_DoubleBufferRegisters;       //Atomic Table,  used only by Bios
  USHORT LUT_AutoFill;                           //Atomic Table,  only used by Bios
  USHORT EnableHW_IconCursor;                    //Atomic Table,  only used by Bios
  USHORT GetMemoryClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT GetEngineClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT SetCRTC_UsingDTDTiming;                 //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT ExternalEncoderControl;                 //Atomic Table,  directly used by various SW components,latest version 2.1
  USHORT LVTMAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT VRAM_BlockDetectionByStrap;             //Atomic Table,  used only by Bios
  USHORT MemoryCleanUp;                          //Atomic Table,  only used by Bios
  USHORT ProcessI2cChannelTransaction;           //Function Table,only used by Bios
  USHORT WriteOneByteToHWAssistedI2C;            //Function Table,indirectly used by various SW components
  USHORT ReadHWAssistedI2CStatus;                //Atomic Table,  indirectly used by various SW components
  USHORT SpeedFanControl;                        //Function Table,indirectly used by various SW components,called from ASIC_Init
  USHORT PowerConnectorDetection;                //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT MC_Synchronization;                     //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT ComputeMemoryEnginePLL;                 //Atomic Table,  indirectly used by various SW components,called from SetMemory/EngineClock
  USHORT MemoryRefreshConversion;                //Atomic Table,  indirectly used by various SW components,called from SetMemory or SetEngineClock
  USHORT VRAM_GetCurrentInfoBlock;               //Atomic Table,  used only by Bios
  USHORT DynamicMemorySettings;                  //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT MemoryTraining;                         //Atomic Table,  used only by Bios
  USHORT EnableSpreadSpectrumOnPPLL;             //Atomic Table,  directly used by various SW components,latest version 1.2
  USHORT TMDSAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT SetVoltage;                             //Function Table,directly and/or indirectly used by various SW components,latest version 1.1
  USHORT DAC1OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT ReadEfuseValue;                         //Atomic Table,  directly used by various SW components,latest version 1.1
  USHORT ComputeMemoryClockParam;                //Function Table,only used by Bios, obsolete soon.Switch to use "ReadEDIDFromHWAssistedI2C"
  USHORT ClockSource;                            //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
  USHORT MemoryDeviceInit;                       //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
  USHORT GetDispObjectInfo;                      //Atomic Table,  indirectly used by various SW components,called from EnableVGARender
  USHORT DIG1EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
  USHORT DIG2EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
  USHORT DIG1TransmitterControl;                 //Atomic Table,directly used by various SW components,latest version 1.1
  USHORT DIG2TransmitterControl;                 //Atomic Table,directly used by various SW components,latest version 1.1
  USHORT ProcessAuxChannelTransaction;           //Function Table,only used by Bios
  USHORT DPEncoderService;                       //Function Table,only used by Bios
  USHORT GetVoltageInfo;                         //Function Table,only used by Bios since SI
}ATOM_MASTER_LIST_OF_COMMAND_TABLES;

// For backward compatible
#define ReadEDIDFromHWAssistedI2C                ProcessI2cChannelTransaction
#define DPTranslatorControl                      DIG2EncoderControl
#define UNIPHYTransmitterControl                 DIG1TransmitterControl
#define LVTMATransmitterControl                  DIG2TransmitterControl
#define SetCRTC_DPM_State                        GetConditionalGoldenSetting
#define ASIC_StaticPwrMgtStatusChange            SetUniphyInstance
#define HPDInterruptService                      ReadHWAssistedI2CStatus
#define EnableVGA_Access                         GetSCLKOverMCLKRatio
#define EnableYUV                                GetDispObjectInfo
#define DynamicClockGating                       EnableDispPowerGating
#define SetupHWAssistedI2CStatus                 ComputeMemoryClockParam
#define DAC2OutputControl                        ReadEfuseValue

#define TMDSAEncoderControl                      PatchMCSetting
#define LVDSEncoderControl                       MC_SEQ_Control
#define LCD1OutputControl                        HW_Misc_Operation
#define TV1OutputControl                         Gfx_Harvesting
#define TVEncoderControl                         SMC_Init

typedef struct _ATOM_MASTER_COMMAND_TABLE
{
  ATOM_COMMON_TABLE_HEADER           sHeader;
  ATOM_MASTER_LIST_OF_COMMAND_TABLES ListOfCommandTables;
}ATOM_MASTER_COMMAND_TABLE;

/****************************************************************************/
// Structures used in every command table
/****************************************************************************/
typedef struct _ATOM_TABLE_ATTRIBUTE
{
#if ATOM_BIG_ENDIAN
  USHORT  UpdatedByUtility:1;         //[15]=Table updated by utility flag
  USHORT  PS_SizeInBytes:7;           //[14:8]=Size of parameter space in Bytes (multiple of a dword),
  USHORT  WS_SizeInBytes:8;           //[7:0]=Size of workspace in Bytes (in multiple of a dword),
#else
  USHORT  WS_SizeInBytes:8;           //[7:0]=Size of workspace in Bytes (in multiple of a dword),
  USHORT  PS_SizeInBytes:7;           //[14:8]=Size of parameter space in Bytes (multiple of a dword),
  USHORT  UpdatedByUtility:1;         //[15]=Table updated by utility flag
#endif
}ATOM_TABLE_ATTRIBUTE;

/****************************************************************************/
// Common header for all command tables.
// Every table pointed by _ATOM_MASTER_COMMAND_TABLE has this common header.
// And the pointer actually points to this header.
/****************************************************************************/
typedef struct _ATOM_COMMON_ROM_COMMAND_TABLE_HEADER
{
  ATOM_COMMON_TABLE_HEADER CommonHeader;
  ATOM_TABLE_ATTRIBUTE     TableAttribute;
}ATOM_COMMON_ROM_COMMAND_TABLE_HEADER;

/****************************************************************************/
// Structures used by ComputeMemoryEnginePLLTable
/****************************************************************************/

#define COMPUTE_MEMORY_PLL_PARAM        1
#define COMPUTE_ENGINE_PLL_PARAM        2
#define ADJUST_MC_SETTING_PARAM         3

/****************************************************************************/
// Structures used by AdjustMemoryControllerTable
/****************************************************************************/
typedef struct _ATOM_ADJUST_MEMORY_CLOCK_FREQ
{
#if ATOM_BIG_ENDIAN
  ULONG ulPointerReturnFlag:1;      // BYTE_3[7]=1 - Return the pointer to the right Data Block; BYTE_3[7]=0 - Program the right Data Block
  ULONG ulMemoryModuleNumber:7;     // BYTE_3[6:0]
  ULONG ulClockFreq:24;
#else
  ULONG ulClockFreq:24;
  ULONG ulMemoryModuleNumber:7;     // BYTE_3[6:0]
  ULONG ulPointerReturnFlag:1;      // BYTE_3[7]=1 - Return the pointer to the right Data Block; BYTE_3[7]=0 - Program the right Data Block
#endif
}ATOM_ADJUST_MEMORY_CLOCK_FREQ;
#define POINTER_RETURN_FLAG             0x80

typedef struct _COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS
{
  ULONG   ulClock;        //When returen, it's the re-calculated clock based on given Fb_div Post_Div and ref_div
  UCHAR   ucAction;       //0:reserved //1:Memory //2:Engine
  UCHAR   ucReserved;     //may expand to return larger Fbdiv later
  UCHAR   ucFbDiv;        //return value
  UCHAR   ucPostDiv;      //return value
}COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS;

typedef struct _COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS_V2
{
  ULONG   ulClock;        //When return, [23:0] return real clock
  UCHAR   ucAction;       //0:reserved;COMPUTE_MEMORY_PLL_PARAM:Memory;COMPUTE_ENGINE_PLL_PARAM:Engine. it return ref_div to be written to register
  USHORT  usFbDiv;          //return Feedback value to be written to register
  UCHAR   ucPostDiv;      //return post div to be written to register
}COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS_V2;

#define COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS_PS_ALLOCATION   COMPUTE_MEMORY_ENGINE_PLL_PARAMETERS

#define SET_CLOCK_FREQ_MASK                       0x00FFFFFF  //Clock change tables only take bit [23:0] as the requested clock value
#define USE_NON_BUS_CLOCK_MASK                    0x01000000  //Applicable to both memory and engine clock change, when set, it uses another clock as the temporary clock (engine uses memory and vice versa)
#define USE_MEMORY_SELF_REFRESH_MASK              0x02000000   //Only applicable to memory clock change, when set, using memory self refresh during clock transition
#define SKIP_INTERNAL_MEMORY_PARAMETER_CHANGE     0x04000000  //Only applicable to memory clock change, when set, the table will skip predefined internal memory parameter change
#define FIRST_TIME_CHANGE_CLOCK                   0x08000000   //Applicable to both memory and engine clock change,when set, it means this is 1st time to change clock after ASIC bootup
#define SKIP_SW_PROGRAM_PLL                       0x10000000   //Applicable to both memory and engine clock change, when set, it means the table will not program SPLL/MPLL
#define USE_SS_ENABLED_PIXEL_CLOCK                USE_NON_BUS_CLOCK_MASK

#define b3USE_NON_BUS_CLOCK_MASK                  0x01       //Applicable to both memory and engine clock change, when set, it uses another clock as the temporary clock (engine uses memory and vice versa)
#define b3USE_MEMORY_SELF_REFRESH                 0x02        //Only applicable to memory clock change, when set, using memory self refresh during clock transition
#define b3SKIP_INTERNAL_MEMORY_PARAMETER_CHANGE   0x04       //Only applicable to memory clock change, when set, the table will skip predefined internal memory parameter change
#define b3FIRST_TIME_CHANGE_CLOCK                 0x08       //Applicable to both memory and engine clock change,when set, it means this is 1st time to change clock after ASIC bootup
#define b3SKIP_SW_PROGRAM_PLL                     0x10       //Applicable to both memory and engine clock change, when set, it means the table will not program SPLL/MPLL
#define b3DRAM_SELF_REFRESH_EXIT                  0x20       //Applicable to DRAM self refresh exit only. when set, it means it will go to program DRAM self refresh exit path

typedef struct _ATOM_COMPUTE_CLOCK_FREQ
{
#if ATOM_BIG_ENDIAN
  ULONG ulComputeClockFlag:8;                 // =1: COMPUTE_MEMORY_PLL_PARAM, =2: COMPUTE_ENGINE_PLL_PARAM
  ULONG ulClockFreq:24;                       // in unit of 10kHz
#else
  ULONG ulClockFreq:24;                       // in unit of 10kHz
  ULONG ulComputeClockFlag:8;                 // =1: COMPUTE_MEMORY_PLL_PARAM, =2: COMPUTE_ENGINE_PLL_PARAM
#endif
}ATOM_COMPUTE_CLOCK_FREQ;

typedef struct _ATOM_S_MPLL_FB_DIVIDER
{
  USHORT usFbDivFrac;
  USHORT usFbDiv;
}ATOM_S_MPLL_FB_DIVIDER;

typedef struct _COMPUTE_MEMORY_ENGINE_P