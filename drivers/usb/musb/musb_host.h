PE     6
#define ATOM_RECORD_END_TYPE                  0xFF

/****************************Spread Spectrum Info Table Definitions **********************/

//ucTableFormatRevision=1
//ucTableContentRevision=2
typedef struct _ATOM_SPREAD_SPECTRUM_ASSIGNMENT
{
  USHORT              usSpreadSpectrumPercentage;
  UCHAR               ucSpreadSpectrumType;       //Bit1=0 Down Spread,=1 Center Spread. Bit1=1 Ext. =0 Int. Bit2=1: PCIE REFCLK SS =0 iternal PPLL SS  Others:TBD
  UCHAR               ucSS_Step;
  UCHAR               ucSS_Delay;
  UCHAR               ucSS_Id;
  UCHAR               ucRecommendedRef_Div;
  UCHAR               ucSS_Range;               //it was reserved for V11
}ATOM_SPREAD_SPECTRUM_ASSIGNMENT;

#define ATOM_MAX_SS_ENTRY                      16
#define ATOM_DP_SS_ID1                                     0x0f1         // SS ID for internal DP stream at 2.7Ghz. if ATOM_DP_SS_ID2 does not exist in SS_InfoTable, it is used for internal DP stream at 1.62Ghz as well.
#define ATOM_DP_SS_ID2                                     0x0f2         // SS ID for internal DP stream at 1.62Ghz, if it exists in SS_InfoTable.
#define ATOM_LVLINK_2700MHz_SS_ID              0x0f3      // SS ID for LV link translator chip at 2.7Ghz
#define ATOM_LVLINK_1620MHz_SS_ID              0x0f4      // SS ID for LV link translator chip at 1.62Ghz



#define ATOM_SS_DOWN_SPREAD_MODE_MASK          0x00000000
#define ATOM_SS_DOWN_SPREAD_MODE               0x00000000
#define ATOM_SS_CENTRE_SPREAD_MODE_MASK        0x00000001
#define ATOM_SS_CENTRE_SPREAD_MODE             0x00000001
#define ATOM_INTERNAL_SS_MASK                  0x00000000
#define ATOM_EXTERNAL_SS_MASK                  0x00000002
#define EXEC_SS_STEP_SIZE_SHIFT                2
#define EXEC_SS_DELAY_SHIFT                    4
#define ACTIVEDATA_TO_BLON_DELAY_SHIFT         4

typedef struct _ATOM_SPREAD_SPECTRUM_INFO
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ATOM_SPREAD_SPECTRUM_ASSIGNMENT   asSS_Info[ATOM_MAX_SS_ENTRY];
}ATOM_SPREAD_SPECTRUM_INFO;


/****************************************************************************/
// Structure used in AnalogTV_InfoTable (Top level)
/****************************************************************************/
//ucTVBootUpDefaultStd definiton:

//ATOM_TV_NTSC                1
//ATOM_TV_NTSCJ               2
//ATOM_TV_PAL                 3
//ATOM_TV_PALM                4
//ATOM_TV_PALCN               5
//ATOM_TV_PALN                6
//ATOM_TV_PAL60               7
//ATOM_TV_SECAM               8

//ucTVSuppportedStd definition:
#define NTSC_SUPPORT          0x1
#define NTSCJ_SUPPORT         0x2

#define PAL_SUPPORT           0x4
#define PALM_SUPPORT          0x8
#define PALCN_SUPPORT         0x10
#define PALN_SUPPORT          0x20
#define PAL60_SUPPORT         0x40
#define SECAM_SUPPORT         0x80

#define MAX_SUPPORTED_TV_TIMING    2

typedef struct _ATOM_ANALOG_TV_INFO
{
  ATOM_COMMON_TABLE_HEADER sHeader;
  UCHAR                    ucTV_SuppportedStandard;
  UCHAR                    ucTV_BootUpDefaultStandard;
  UCHAR                    ucExt_TV_ASIC_ID;
  UCHAR                    ucExt_TV_ASIC_SlaveAddr;
  ATOM_DTD_FORMAT          aModeTimings[MAX_SUPPORTED_TV_TIMING];
}ATOM_ANALOG_TV_INFO;

typedef struct _ATOM_DPCD_INFO
{
  UCHAR   ucRevisionNumber;        //10h : Revision 1.0; 11h : Revision 1.1
  UCHAR   ucMaxLinkRate;           //06h : 1.62Gbps per lane; 0Ah = 2.7Gbps per lane
  UCHAR   ucMaxLane;               //Bits 4:0 = MAX_LANE_COUNT (1/2/4). Bit 7 = ENHANCED_FRAME_CAP
  UCHAR   ucMaxDownSpread;         //Bit0 = 0: No Down spread; Bit0 = 1: 0.5% (Subject to change according to DP spec)
}ATOM_DPCD_INFO;

#define ATOM_DPCD_MAX_LANE_MASK    0x1F

/**************************************************************************/
// VRAM usage and their defintions

// One chunk of VRAM used by Bios are for HWICON surfaces,EDID data.
// Current Mode timing and Dail Timing and/or STD timing data EACH device. They can be broken down as below.
// All the addresses below are the offsets from the frame buffer start.They all MUST be Dword aligned!
// To driver: The physical address of this memory portion=mmFB_START(4K aligned)+ATOMBIOS_VRAM_USAGE_START_ADDR+ATOM_x_ADDR
// To Bios:  ATOMBIOS_VRAM_USAGE_START_ADDR+ATOM_x_ADDR->MM_INDEX

// Moved VESA_MEMORY_IN_64K_BLOCK definition to "AtomConfig.h" so that it can be redefined in design (SKU).
//#ifndef VESA_MEMORY_IN_64K_BLOCK
//#define VESA_MEMORY_IN_64K_BLOCK        0x100       //256*64K=16Mb (Max. VESA memory is 16Mb!)
//#endif

#define ATOM_EDID_RAW_DATASIZE          256         //In Bytes
#define ATOM_HWICON_SURFACE_SIZE        4096        //In Bytes
#define ATOM_HWICON_INFOTABLE_SIZE      32
#define MAX_DTD_MODE_IN_VRAM            6
#define ATOM_DTD_MODE_SUPPORT_TBL_SIZE  (MAX_DTD_MODE_IN_VRAM*28)    //28= (SIZEOF ATOM_DTD_FORMAT)
#define ATOM_STD_MODE_SUPPORT_TBL_SIZE  32*8                         //32 is a predefined number,8= (SIZEOF ATOM_STD_FORMAT)
//20 bytes for Encoder Type and DPCD in STD EDID area
#define DFP_ENCODER_TYPE_OFFSET         (ATOM_EDID_RAW_DATASIZE + ATOM_DTD_MODE_SUPPORT_TBL_SIZE + ATOM_STD_MODE_SUPPORT_TBL_SIZE - 20)
#define ATOM_DP_DPCD_OFFSET             (DFP_ENCODER_TYPE_OFFSET + 4 )

#define ATOM_HWICON1_SURFACE_ADDR       0
#define ATOM_HWICON2_SURFACE_ADDR       (ATOM_HWICON1_SURFACE_ADDR + ATOM_HWICON_SURFACE_SIZE)
#define ATOM_HWICON_INFOTABLE_ADDR  