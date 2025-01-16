s master link of dual link is B or D or F
  UCHAR ucEncoderSel:1;             //bit3=0: Data/Clk path source from DIGA/C/E. =1: Data/clk path source from DIGB/D/F
  UCHAR ucRefClkSource:2;           //bit5:4: PPLL1 =0, PPLL2=1, DCPLL=2, EXT_CLK=3   <= New
  UCHAR ucTransmitterSel:2;         //bit7:6: =0 Dig Transmitter 1 ( Uniphy AB )
                                    //        =1 Dig Transmitter 2 ( Uniphy CD )
                                    //        =2 Dig Transmitter 3 ( Uniphy EF )
#endif
}ATOM_DIG_TRANSMITTER_CONFIG_V4;

typedef struct _DIG_TRANSMITTER_CONTROL_PARAMETERS_V4
{
  union
  {
    USHORT usPixelClock;      // in 10KHz; for bios convenient
    USHORT usInitInfo;         // when init uniphy,lower 8bit is used for connector type defined in objectid.h
    ATOM_DP_VS_MODE_V4 asMode; // DP Voltage swing mode     Redefined comparing to previous version
  };
  union
  {
  ATOM_DIG_TRANSMITTER_CONFIG_V4 acConfig;
  UCHAR ucConfig;
  };
  UCHAR ucAction;                // define as ATOM_TRANSMITER_ACTION_XXX
  UCHAR ucLaneNum;
  UCHAR ucReserved[3];
}DIG_TRANSMITTER_CONTROL_PARAMETERS_V4;

//ucConfig
//Bit0
#define ATOM_TRANSMITTER_CONFIG_V4_DUAL_LINK_CONNECTOR         0x01
//Bit1
#define ATOM_TRANSMITTER_CONFIG_V4_COHERENT                      0x02
//Bit2
#define ATOM_TRANSMITTER_CONFIG_V4_LINK_SEL_MASK              0x04
#define ATOM_TRANSMITTER_CONFIG_V4_LINKA                       0x00
#define ATOM_TRANSMITTER_CONFIG_V4_LINKB                        0x04
// Bit3
#define ATOM_TRANSMITTER_CONFIG_V4_ENCODER_SEL_MASK           0x08
#define ATOM_TRANSMITTER_CONFIG_V4_DIG1_ENCODER                0x00
#define ATOM_TRANSMITTER_CONFIG_V4_DIG2_ENCODER                0x08
// Bit5:4
#define ATOM_TRANSMITTER_CONFIG_V4_REFCLK_SEL_MASK            0x30
#define ATOM_TRANSMITTER_CONFIG_V4_P1PLL                       0x00
#define ATOM_TRANSMITTER_CONFIG_V4_P2PLL                      0x10
#define ATOM_TRANSMITTER_CONFIG_V4_DCPLL                      0x20   // New in _V4
#define ATOM_TRANSMITTER_CONFIG_V4_REFCLK_SRC_EXT           0x30   // Changed comparing to V3
// Bit7:6
#define ATOM_TRANSMITTER_CONFIG_V4_TRANSMITTER_SEL_MASK     0xC0
#define ATOM_TRANSMITTER_CONFIG_V4_TRANSMITTER1              0x00   //AB
#define ATOM_TRANSMITTER_CONFIG_V4_TRANSMITTER2              0x40   //CD
#define ATOM_TRANSMITTER_CONFIG_V4_TRANSMITTER3              0x80   //EF


typedef struct _ATOM_DIG_TRANSMITTER_CONFIG_V5
{
#if ATOM_BIG_ENDIAN
  UCHAR ucReservd1:1;
  UCHAR ucHPDSel:3;
  UCHAR ucPhyClkSrcId:2;
  UCHAR ucCoherentMode:1;
  UCHAR ucReserved:1;
#else
  UCHAR ucReserved:1;
  UCHAR ucCoherentMode:1;
  UCHAR ucPhyClkSrcId:2;
  UCHAR ucHPDSel:3;
  UCHAR ucReservd1:1;
#endif
}ATOM_DIG_TRANSMITTER_CONFIG_V5;

typedef struct _DIG_TRANSMITTER_CONTROL_PARAMETERS_V1_5
{
  USHORT usSymClock;              // Encoder Clock in 10kHz,(DP mode)= linkclock/10, (TMDS/LVDS/HDMI)= pixel clock,  (HDMI deep color), =pixel clock * deep_color_ratio
  UCHAR  ucPhyId;                   // 0=UNIPHYA, 1=UNIPHYB, 2=UNIPHYC, 3=UNIPHYD, 4= UNIPHYE 5=UNIPHYF
  UCHAR  ucAction;                // define as ATOM_TRANSMITER_ACTION_xxx
  UCHAR  ucLaneNum;                 // indicate lane number 1-8
  UCHAR  ucConnObjId;               // Connector Object Id defined in ObjectId.h
  UCHAR  ucDigMode;                 // indicate DIG mode
  union{
  ATOM_DIG_TRANSMITTER_CONFIG_V5 asConfig;
  UCHAR ucConfig;
  };
  UCHAR  ucDigEncoderSel;           // indicate DIG front end encoder
  UCHAR  ucDPLaneSet;
  UCHAR  ucReserved;
  UCHAR  ucReserved1;
}DIG_TRANSMITTER_CONTROL_PARAMETERS_V1_5;

//ucPhyId
#define ATOM_PHY_ID_UNIPHYA                                 0
#define ATOM_PHY_ID_UNIPHYB                                 1
#define ATOM_PHY_ID_UNIPHYC                                 2
#define ATOM_PHY_ID_UNIPHYD                                 3
#define ATOM_PHY_ID_UNIPHYE                                 4
#define ATOM_PHY_ID_UNIPHYF                                 5
#define ATOM_PHY_ID_UNIPHYG                                 6

// ucDigEncoderSel
#define ATOM_TRANMSITTER_V5__DIGA_SEL                       0x01
#define ATOM_TRANMSITTER_V5__DIGB_SEL                       0x02
#define ATOM_TRANMSITTER_V5__DIGC_SEL                       0x04
#define ATOM_TRANMSITTER_V5__DIGD_SEL                       0x08
#define ATOM_TRANMSITTER_V5__DIGE_SEL                       0x10
#define ATOM_TRANMSITTER_V5__DIGF_SEL                       0x20
#define ATOM_TRANMSITTER_V5__DIGG_SEL                       0x40

// ucDigMode
#define ATOM_TRANSMITTER_DIGMODE_V5_DP                      0
#define ATOM_TRANSMITTER_DIGMODE_V5_LVDS                    1
#define ATOM_TRANSMITTER_DIGMODE_V5_DVI                     2
#define ATOM_TRANSMITTER_DIGMODE_V5_HDMI                    3
#define ATOM_TRANSMITTER_DIGMODE_V5_SDVO                    4
#define ATOM_TRANSMITTER_DIGMODE_V5_DP_MST                  5

// ucDPLaneSet
#define DP_LANE_SET__0DB_0_4V                               0x00
#define DP_LANE_SET__0DB_0_6V                               0x01
#define DP_LANE_SET__0DB_0_8V                               0x02
#define DP_LANE_SET__0DB_1_2V                               0x03
#define DP_LANE_SET__3_5DB_0_4V                             0x08
#define DP_LANE_SET__3_5DB_0_6V                             0x09
#define DP_LANE_SET__3_5DB_0_8V                             0x0a
#define DP_LANE_SET__6DB_0_4V                               0x10
#define DP_LANE_SET__6DB_0_6V                               0x11
#define DP_LANE_SET__9_5DB_0_4V                             0x18

// ATOM_DIG_TRANSMITTER_CONFIG_V5 asConfig;
// Bit1
#define ATOM_TRANSMITTER_CONFIG_V5_COHERENT                      0x02

// Bit3:2
#define ATOM_TRANSMITTER_CONFIG_V5_REFCLK_SEL_MASK            0x0c
#define ATOM_TRANSMITTER_CONFIG_V5_REFCLK_SEL_SHIFT          0x02

#define ATOM_TRANSMITTER_CONFIG_V5_P1PLL                       0x00
#define ATOM_TRANSMITTER_CONFIG_V5_P2PLL                      0x04
#define ATOM_TRANSMITTER_CONFIG_V5_P0PLL                      0x08
#define ATOM_TRANSMITTER_CONFIG_V5_REFCLK_SRC_EXT           0x0c
// Bit6:4
#define ATOM_TRANSMITTER_CONFIG_V5_HPD_SEL_MASK                0x70
#define ATOM_TRANSMITTER_CONFIG_V5_HPD_SEL_SHIFT            0x04

#define ATOM_TRANSMITTER_CONFIG_V5_NO_HPD_SEL                    0x00
#define ATOM_TRANSMITTER_CONFIG_V5_HPD1_SEL                      0x10
#define ATOM_TRANSMITTER_CONFIG_V5_HPD2_SEL                      0x20
#define ATOM_TRANSMITTER_CONFIG_V5_HPD3_SEL                      0x30
#define ATOM_TRANSMITTER_CONFIG_V5_HPD4_SEL                      0x40
#define ATOM_TRANSMITTER_CONFIG_V5_HPD5_SEL                      0x50
#define ATOM_TRANSMITTER_CONFIG_V5_HPD6_SEL                      0x60

#define DIG_TRANSMITTER_CONTROL_PS_ALLOCATION_V1_5            DIG_TRANSMITTER_CONTROL_PARAMETERS_V1_5


/****************************************************************************/
// Structures used by ExternalEncoderControlTable V1.3
// ASIC Families: Evergreen, Llano, NI
// ucTableFormatRevision=1
// ucTableContentRevision=3
/****************************************************************************/

typedef struct _EXTERNAL_ENCODER_CONTROL_PARAMETERS_V3
{
  union{
  USHORT usPixelClock;      // pixel clock in 10Khz, valid when ucAction=SETUP/ENABLE_OUTPUT
  USHORT usConnectorId;     // connector id, valid when ucAction = INIT
  };
  UCHAR  ucConfig;          // indicate which encoder, and DP link rate when ucAction = SETUP/ENABLE_OUTPUT
  UCHAR  ucAction;          //
  UCHAR  ucEncoderMode;     // encoder mode, only used when ucAction = SETUP/ENABLE_OUTPUT
  UCHAR  ucLaneNum;         // lane number, only used when ucAction = SETUP/ENABLE_OUTPUT
  UCHAR  ucBitPerColor;     // output bit per color, only valid when ucAction = SETUP/ENABLE_OUTPUT and ucEncodeMode= DP
  UCHAR  ucReserved;
}EXTERNAL_ENCODER_CONTROL_PARAMETERS_V3;

// ucAction
#define EXTERANL_ENCODER_ACTION_V3_DISABLE_OUTPUT         0x00
#define EXTERANL_ENCODER_ACTION_V3_ENABLE_OUTPUT          0x01
#define EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT           0x07
#define EXTERNAL_ENCODER_ACTION_V3_ENCODER_SETUP          0x0f
#define EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING_OFF   0x10
#define EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING       0x11
#define EXTERNAL_ENCODER_ACTION_V3_DACLOAD_DETECTION      0x12
#define EXTERNAL_ENCODER_ACTION_V3_DDC_SETUP              0x14

// ucConfig
#define EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_MASK            0x03
#define EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_1_62GHZ        0x00
#define EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_2_70GHZ        0x01
#define EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_5_40GHZ        0x02
#define EXTERNAL_ENCODER_CONFIG_V3_ENCODER_SEL_MAKS          0x70
#define EXTERNAL_ENCODER_CONFIG_V3_ENCODER1                  0x00
#define EXTERNAL_ENCODER_CONFIG_V3_ENCODER2                  0x10
#define EXTERNAL_ENCODER_CONFIG_V3_ENCODER3                  0x20

typedef struct _EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION_V3
{
  EXTERNAL_ENCODER_CONTROL_PARAMETERS_V3 sExtEncoder;
  ULONG ulReserved[2];
}EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION_V3;


/****************************************************************************/
// Structures used by DAC1OuputControlTable
//                    DAC2OuputControlTable
//                    LVTMAOutputControlTable  (Before DEC30)
//                    TMDSAOutputControlTable  (Before DEC30)
/****************************************************************************/
typedef struct _DISPLAY_DEVICE_OUTPUT_CONTROL_PARAMETERS
{
  UCHAR  ucAction;                    // Possible input:ATOM_ENABLE||ATOMDISABLE
                                      // When the display is LCD, in addition to above