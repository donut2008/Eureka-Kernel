 data;
  UCHAR                      ucDefaultMVDDC_ID; // Default MVDDC setting for this memory block, ID linking to MVDDC info table to find real set-up data;
  UCHAR                      ucReserved[2];
}ATOM_VRAM_MODULE_V1;


typedef struct _ATOM_VRAM_MODULE_V2
{
  ULONG                      ulReserved;
  ULONG                      ulFlags;              // To enable/disable functionalities based on memory type
  ULONG                      ulEngineClock;     // Override of default engine clock for particular memory type
  ULONG                      ulMemoryClock;     // Override of default memory clock for particular memory type
  USHORT                     usEMRS2Value;      // EMRS2 Value is used for GDDR2 and GDDR4 memory type
  USHORT                     usEMRS3Value;      // EMRS3 Value is used for GDDR2 and GDDR4 memory type
  USHORT                     usEMRSValue;
  USHORT                     usMRSValue;
  USHORT                     usReserved;
  UCHAR                      ucExtMemoryID;     // An external indicator (by hardcode, callback or pin) to tell what is the current memory module
  UCHAR                      ucMemoryType;      // [7:4]=0x1:DDR1;=0x2:DDR2;=0x3:DDR3;=0x4:DDR4;[3:0] - must not be used for now;
  UCHAR                      ucMemoryVenderID;  // Predefined,never change across designs or memory type/vender. If not predefined, vendor detection table gets executed
  UCHAR                      ucMemoryDeviceCfg; // [7:4]=0x0:4M;=0x1:8M;=0x2:16M;0x3:32M....[3:0]=0x0:x4;=0x1:x8;=0x2:x16;=0x3:x32...
  UCHAR                      ucRow;             // Number of Row,in power of 2;
  UCHAR                      ucColumn;          // Number of Column,in power of 2;
  UCHAR                      ucBank;            // Nunber of Bank;
  UCHAR                      ucRank;            // Number of Rank, in power of 2
  UCHAR                      ucChannelNum;      // Number of channel;
  UCHAR                      ucChannelConfig;   // [3:0]=Indication of what channel combination;[4:7]=Channel bit width, in number of 2
  UCHAR                      ucDefaultMVDDQ_ID; // Default MVDDQ setting for this memory block, ID linking to MVDDQ info table to find real set-up data;
  UCHAR                      ucDefaultMVDDC_ID; // Default MVDDC setting for this memory block, ID linking to MVDDC info table to find real set-up data;
  UCHAR                      ucRefreshRateFactor;
  UCHAR                      ucReserved[3];
}ATOM_VRAM_MODULE_V2;


typedef   struct _ATOM_MEMORY_TIMING_FORMAT
{
   ULONG                     ulClkRange;            // memory clock in 10kHz unit, when target memory clock is below this clock, use this memory timing
  union{
    USHORT                   usMRS;                 // mode register
    USHORT                   usDDR3_MR0;
  };
  union{
    USHORT                   usEMRS;                  // extended mode register
    USHORT                   usDDR3_MR1;
  };
   UCHAR                     ucCL;                    // CAS latency
   UCHAR                     ucWL;                    // WRITE Latency
   UCHAR                     uctRAS;                  // tRAS
   UCHAR                     uctRC;                   // tRC
   UCHAR                     uctRFC;                  // tRFC
   UCHAR                     uctRCDR;                 // tRCDR
   UCHAR                     uctRCDW;                 // tRCDW
   UCHAR                     uctRP;                   // tRP
   UCHAR                     uctRRD;                  // tRRD
   UCHAR                     uctWR;                   // tWR
   UCHAR                     uctWTR;                  // tWTR
   UCHAR                     uctPDIX;                 // tPDIX
   UCHAR                     uctFAW;                  // tFAW
   UCHAR                     uctAOND;                 // tAOND
  union
  {
    struct {
       UCHAR                                  ucflag;                  // flag to control memory timing calculation. bit0= control EMRS2 Infineon
       UCHAR                                  ucReserved;
    };
    USHORT                   usDDR3_MR2;
  };
}ATOM_MEMORY_TIMING_FORMAT;


typedef   struct _ATOM_MEMORY_TIMING_FORMAT_V1
{
   ULONG                      ulClkRange;            // memory clock in 10kHz unit, when target memory clock is below this clock, use this memory timing
   USHORT                     usMRS;                 // mode register
   USHORT                     usEMRS;                // extended mode register
   UCHAR                      ucCL;                  // CAS latency
   UCHAR                      ucWL;                  // WRITE Latency
   UCHAR                      uctRAS;                // tRAS
   UCHAR                      uctRC;                 // tRC
   UCHAR                      uctRFC;                // tRFC
   UCHAR                      uctRCDR;               // tRCDR
   UCHAR                      uctRCDW;               // tRCDW
   UCHAR                      uctRP;                 // tRP
   UCHAR                      uctRRD;                // tRRD
   UCHAR                      uctWR;                 // tWR
   UCHAR                      uctWTR;                // tWTR
   UCHAR                      uctPDIX;               // tPDIX
   UCHAR                      uctFAW;                // tFAW
   UCHAR                      uctAOND;               // tAOND
   UCHAR                      ucflag;                // flag to control memory timing calculation. bit0= control EMRS2 Infineon
////////////////////////////////////GDDR parameters///////////////////////////////////
   UCHAR                      uctCCDL;               //
   UCHAR                      uctCRCRL;              //
   UCHAR                      uctCRCWL;              //
   UCHAR                      uctCKE;                //
   UCHAR                      uctCKRSE;              //
   UCHAR                      uctCKRSX;              //
   UCHAR                      uctFAW32;              //
   UCHAR                      ucMR5lo;               //
   UCHAR                      ucMR5hi;               //
   UCHAR                      ucTerminator;
}ATOM_MEMORY_TIMING_FORMAT_V1;




typedef   struct _ATOM_MEMORY_TIMING_FORMAT_V2
{
   ULONG                                  ulClkRange;            // memory clock in 10kHz unit, when target memory clock is below this clock, use this memory timing
   USHORT                               usMRS;                     // mode register
   USHORT                               usEMRS;                  // extended mode register
   UCHAR                                  ucCL;                     // CAS latency
   UCHAR                                  ucWL;                     // WRITE Latency
   UCHAR                                  uctRAS;                  // tRAS
   UCHAR                                  uctRC;                     // tRC
   UCHAR                                  uctRFC;                  // tRFC
   UCHAR                                  uctRCDR;                  // tRCDR
   UCHAR                                  uctRCDW;                  // tRCDW
   UCHAR                                  uctRP;                     // tRP
   UCHAR                                  uctRRD;                  // tRRD
   UCHAR                                  uctWR;                     // tWR
   UCHAR                                  uctWTR;                  // tWTR
   UCHAR                                  uctPDIX;                  // tPDIX
   UCHAR                                  uctFAW;                  // tFAW
   UCHAR                                  uctAOND;                  // tAOND
   UCHAR                                  ucflag;                  // flag to control memory timing calculation. bit0= control EMRS2 Infineon
////////////////////////////////////GDDR parameters///////////////////////////////////
   UCHAR                                  uctCCDL;                  //
   UCHAR                                  uctCRCRL;                  //
   UCHAR                                  uctCRCWL;                  //
   UCHAR                                  uctCKE;                  //
   UCHAR                                  uctCKRSE;                  //
   UCHAR                                  uctCKRSX;                  //
   UCHAR                                  uctFAW32;                  //
   UCHAR                                  ucMR4lo;               //
   UCHAR                                  ucMR4hi;               //
   UCHAR                                  ucMR5lo;               //
   UCHAR                                  ucMR5hi;               //
   UCHAR                                  ucTerminator;
   UCHAR                                  ucReserved;
}ATOM_MEMORY_TIMING_FORMAT_V2;


typedef   struct _ATOM_MEMORY_FORMAT
{
   ULONG                       ulDllDisClock;     // memory DLL will be disable when target memory clock is below this clock
  union{
    USHORT                     usEMRS2Value;      // EMRS2 Value is used for GDDR2 and GDDR4 memory type
    USHORT                     usDDR3_Reserved;   // Not used for DDR3 memory
  };
  union{
    USHORT                     usEMRS3Value;      // EMRS3 Value is used for GDDR2 and GDDR4 memory type
    USHORT                     usDDR3_MR3;        // Used for DDR3 memory
  };
  UCHAR                        ucMemoryType;      // [7:4]=0x1:DDR1;=0x2:DDR2;=0x3:DDR3;=0x4:DDR4;[3:0] - must not be used for now;
  UCHAR                        ucMemoryVenderID;  // Predefined,never change across designs or memory type/vender. If not predefined, vendor detection table gets executed
  UCHAR                        ucRow;             // Number of Row,in power of 2;
  UCHAR                        ucColumn;          // Number of Column,in power of 2;
  UCHAR                        ucBank;            // Nunber of Bank;
  UCHAR                        ucRank;            // Number of Rank, in power of 2
  UCHAR                        ucBurstSize;           // burst size, 0= burst size=4  1= burst size=8
  UCHAR                        ucDllDisBit;           // position of DLL Enable/Disable bit in EMRS ( Extended Mode Register )
  UCHAR                        ucRefreshRateFactor;   // memory refresh rate in unit of ms
  UCHAR                        ucDensity;             // _8Mx32, _16Mx32, _16Mx16, _32Mx16
  UCHAR                        ucPreamble;            // [7:4] Write Preamble, [3:0] Read Preamble
  UCHAR                        ucMemAttrib;           // Memory Device Addribute, like RDBI/WDBI etc
  ATOM_MEMORY_TIMING_FORMAT    asMemTiming[5];        // Memory Timing block sort from lower clock to higher clock
}ATOM_MEMORY_FORMAT;


typedef struct _ATOM_VRAM_MODULE_V3
{
  ULONG                      ulChannelMapCfg;     // board dependent paramenter:Channel combination
  USHORT                     usSize;              // size of ATOM_VRAM_MODULE_V3
  USHORT                     usDefaultMVDDQ;      // board dependent parameter:Default Memory Core Voltage
  USHORT                     usDefaultMVDDC;      // board dependent parameter:Default Memory IO Voltage
  UCHAR                      ucExtMemoryID;       // An external indicator (by hardcode, callback or pin) to tell what is the current memory module
  UCHAR                      ucChannelNum;        // board dependent parameter:Number of channel;
  UCHAR                      ucChannelSize;       // board dependent parameter:32bit or 64bit
  UCHAR                      ucVREFI;             // board dependnt parameter: EXT or INT +160mv to -140mv
  UCHAR                      ucNPL_RT;            // board dependent parameter:NPL round trip delay, used for calculate memory timing parameters
  UCHAR                      ucFlag;              // To enable/disable functionalities based on memory type
  ATOM_MEMORY_FORMAT         asMemory;            // describ all of video memory parameters from memory spec
}ATOM_VRAM_MODULE_V3;


//ATOM_VRAM_MODULE_V3.ucNPL_RT
#define NPL_RT_MASK                                         0x0f
#define BATTERY_ODT_MASK                                    0xc0

#define ATOM_VRAM_MODULE       ATOM_VRAM_MODULE_V3

typedef struct _ATOM_VRAM_MODULE_V4
{
  ULONG     ulChannelMapCfg;                   // board dependent parameter: Channel combination
  USHORT  usModuleSize;                     // size of ATOM_VRAM_MODULE_V4, make it easy for VBIOS to look for next entry of VRAM_MODULE
  USHORT  usPrivateReserved;                // BIOS internal reserved space to optimize code size, updated by the compiler, shouldn't be modified manually!!
                                            // MC_ARB_RAMCFG (includes NOOFBANK,NOOFRANKS,NOOFROWS,NOOFCOLS)
  USHORT  usReserved;
  UCHAR   ucExtMemoryID;                      // An external indicator (by hardcode, callback or pin) to tell what is the current memory module
  UCHAR   ucMemoryType;                     // [7:4]=0x1:DDR1;=0x2:DDR2;=0x3:DDR3;=0x4:DDR4; 0x5:DDR5 [3:0] - Must be 0x0 for now;
  UCHAR   ucChannelNum;                     // Number of channels present in this module config
  UCHAR   ucChannelWidth;                   // 0 - 32 bits; 1 - 64 bits
   UCHAR   ucDensity;                        // _8Mx32, _16Mx32, _16Mx16, _32Mx16
   UCHAR     ucFlag;                                  // To enable/disable functionalities based on memory type
   UCHAR     ucMisc;                                  // bit0: 0 - single rank; 1 - dual rank;   bit2: 0 - burstlength 4, 1 - burstlength 8
  UCHAR      ucVREFI;                          // board dependent parameter
  UCHAR   ucNPL_RT;                         // board dependent parameter:NPL round trip delay, used for calculate memory timing parameters
  UCHAR      ucPreamble;                       // [7:4] Write Preamble, [3:0] Read Preamble
  UCHAR   ucMemorySize;                     // BIOS internal reserved space to optimize code size, updated by the compiler, shouldn't be modified manually!!
                                            // Total memory size in unit of 16MB for CONFIG_MEMSIZE - bit[23:0] zeros
  UCHAR   ucReserved[3];

//compare with V3, we flat the struct by merging ATOM_MEMORY_FORMAT (as is) into V4 as the same level
  union{
    USHORT   usEMRS2Value;                   // EMRS2 Value is used for GDDR2 and GDDR4 memory type
    USHORT  usDDR3_Reserved;
  };
  union{
    USHORT   usEMRS3Value;                   // EMRS3 Value is used for GDDR2 and GDDR4 memory type
    USHORT  usDDR3_MR3;                     // Used for DDR3 memory
  };
  UCHAR   ucMemoryVenderID;                    // Predefined, If not predefined, vendor detection table gets executed
  UCHAR     ucRefreshRateFactor;              // [1:0]=RefreshFactor (00=8ms, 01=16ms, 10=32ms,11=64ms)
  UCHAR   ucReserved2[2];
  ATOM_MEMORY_TIMING_FORMAT  asMemTiming[5];//Memory Timing block sort from lower clock to higher clock
}ATOM_VRAM_MODULE_V4;

#define VRAM_MODULE_V4_MISC_RANK_MASK       0x3
#define VRAM_MODULE_V4_MISC_DUAL_RANK       0x1
#define VRAM_MODULE_V4_MISC_BL_MASK         0x4
#define VRAM_MODULE_V4_MISC_BL8             0x4
#define VRAM_MODULE_V4_MISC_DUAL_CS         0x10

typedef struct _ATOM_VRAM_MODULE_V5
{
  ULONG     ulChannelMapCfg;                   // board dependent parameter: Channel combination
  USHORT  usModuleSize;                     // size of ATOM_VRAM_MODULE_V4, make it easy for VBIOS to look for next entry of VRAM_MODULE
  USHORT  usPrivateReserved;                // BIOS internal reserved space to optimize code size, updated by the compiler, shouldn't be modified manually!!
                                            // MC_ARB_RAMCFG (includes NOOFBANK,NOOFRANKS,NOOFROWS,NOOFCOLS)
  USHORT  usReserved;
  UCHAR   ucExtMemoryID;                      // An external indicator (by hardcode, callback or pin) to tell what is the current memory module
  UCHAR   ucMemoryType;                     // [7:4]=0x1:DDR1;=0x2:DDR2;=0x3:DDR3;=0x4:DDR4; 0x5:DDR5 [3:0] - Must be 0x0 for now;
  UCHAR   ucChannelNum;                     // Number of channels present in this module config
  UCHAR   ucChannelWidth;                   // 0 - 32 bits; 1 - 64 bits
   UCHAR   ucDensity;                        // _8Mx32, _16Mx32, _16Mx16, _32Mx16
   UCHAR     ucFlag;                                  // To enable/disable functionalities based on memory type
   UCHAR     ucMisc;                                  // bit0: 0 - single rank; 1 - dual rank;   bit2: 0 - burstlength 4, 1 - burstlength 8
  UCHAR      ucVREFI;                          // board dependent parameter
  UCHAR   ucNPL_RT;                         // board dependent parameter:NPL round trip delay, used for calculate memory timing parameters
  UCHAR     