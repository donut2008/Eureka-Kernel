CONFIG_V4_DPLINKRATE_MASK            0x03
#define ATOM_ENCODER_CONFIG_V4_DPLINKRATE_1_62GHZ        0x00
#define ATOM_ENCODER_CONFIG_V4_DPLINKRATE_2_70GHZ        0x01
#define ATOM_ENCODER_CONFIG_V4_DPLINKRATE_5_40GHZ        0x02
#define ATOM_ENCODER_CONFIG_V4_DPLINKRATE_3_24GHZ        0x03
#define ATOM_ENCODER_CONFIG_V4_ENCODER_SEL                 0x70
#define ATOM_ENCODER_CONFIG_V4_DIG0_ENCODER                 0x00
#define ATOM_ENCODER_CONFIG_V4_DIG1_ENCODER                 0x10
#define ATOM_ENCODER_CONFIG_V4_DIG2_ENCODER                 0x20
#define ATOM_ENCODER_CONFIG_V4_DIG3_ENCODER                 0x30
#define ATOM_ENCODER_CONFIG_V4_DIG4_ENCODER                 0x40
#define ATOM_ENCODER_CONFIG_V4_DIG5_ENCODER                 0x50
#define ATOM_ENCODER_CONFIG_V4_DIG6_ENCODER                 0x60

typedef struct _DIG_ENCODER_CONTROL_PARAMETERS_V4
{
  USHORT usPixelClock;      // in 10KHz; for bios convenient
  union{
  ATOM_DIG_ENCODER_CONFIG_V4 acConfig;
  UCHAR ucConfig;
  };
  UCHAR ucAction;
  union{
    UCHAR ucEncoderMode;
                            // =0: DP   encoder
                            // =1: LVDS encoder
                            // =2: DVI  encoder
                            // =3: HDMI encoder
                            // =4: SDVO encoder
                            // =5: DP audio
    UCHAR ucPanelMode;      // only valid when ucAction == ATOM_ENCODER_CMD_SETUP_PANEL_MODE
                            // =0:     external DP
                            // =0x1:   internal DP2
                            // =0x11:  internal DP1 for NutMeg/Travis DP translator
  };
  UCHAR ucLaneNum;          // how many lanes to enable
  UCHAR ucBitPerColor;      // only valid for DP mode when ucAction = ATOM_ENCODER_CMD_SETUP
  UCHAR ucHPD_ID;           // HPD ID (1-6). =0 means to skip HDP programming. New comparing to previous version
}DIG_ENCODER_CONTROL_PARAMETERS_V4;

// define ucBitPerColor:
#define PANEL_BPC_UNDEFINE                               0x00
#define PANEL_6BIT_PER_COLOR                             0x01
#define PANEL_8BIT_PER_COLOR                             0x02
#define PANEL_10BIT_PER_COLOR                            0x03
#define PANEL_12BIT_PER_COLOR                            0x04
#define PANEL_16BIT_PER_COLOR                            0x05

//define ucPanelMode
#define DP_PANEL_MODE_EXTERNAL_DP_MODE                   0x00
#define DP_PANEL_MODE_INTERNAL_DP2_MODE                  0x01
#define DP_PANEL_MODE_INTERNAL_DP1_MODE                  0x11

/****************************************************************************/
// Structures used by UNIPHYTransmitterControlTable
//                    LVTMATransmitterControlTable
//                    DVOOutputControlTable
/****************************************************************************/
typedef struct _ATOM_DP_VS_MODE
{
  UCHAR ucLaneSel;
  UCHAR ucLaneSet;
}ATOM_DP_VS_MODE;

typedef struct _DIG_TRANSMITTER_CONTROL_PARAMETERS
{
   union
   {
  USHORT usPixelClock;      // in 10KHz; for bios convenient
   USHORT usInitInfo;         // when init uniphy,lower 8bit is used for connector type defined in objectid.h
  ATOM_DP_VS_MODE asMode; // DP Voltage swing mode
   };
  UCHAR ucConfig;
                                       // [0]=0: 4 lane Link,
                                       //    =1: 8 lane Link ( Dual Links TMDS )
                          // [1]=0: InCoherent mode
                                       //    =1: Coherent Mode
                                       // [2] Link Select:
                                      // =0: PHY linkA   if bfLane<3
                                       // =1: PHY linkB   if bfLanes<3
                                      // =0: PHY linkA+B if bfLanes=3
                          // [5:4]PCIE lane Sel
                          // =0: lane 0~3 or 0~7
                          // =1: lane 4~7
                          // =2: lane 8~11 or 8~15
                          // =3: lane 12~15
   UCHAR ucAction;              // =0: turn off encoder
                           // =1: turn on encoder
  UCHAR ucReserved[4];
}DIG_TRANSMITTER_CONTROL_PARAMETERS;

#define DIG_TRANSMITTER_CONTROL_PS_ALLOCATION      DIG_TRANSMITTER_CONTROL_PARAMETERS

//ucInitInfo
#define ATOM_TRAMITTER_INITINFO_CONNECTOR_MASK   0x00ff

//ucConfig
#define ATOM_TRANSMITTER_CONFIG_8LANE_LINK         0x01
#define ATOM_TRANSMITTER_CONFIG_COHERENT            0x02
#define ATOM_TRANSMITTER_CONFIG_LINK_SEL_MASK      0x04
#define ATOM_TRANSMITTER_CONFIG_LINKA                  0x00
#define ATOM_TRANSMITTER_CONFIG_LINKB                  0x04
#define ATOM_TRANSMITTER_CONFIG_LINKA_B               0x00
#define ATOM_TRANSMITTER_CONFIG_LINKB_A               0x04

#define ATOM_TRANSMITTER_CONFIG_ENCODER_SEL_MASK   0x08         // only used when ATOM_TRANSMITTER_ACTION_ENABLE
#define ATOM_TRANSMITTER_CONFIG_DIG1_ENCODER      0x00            // only used when ATOM_TRANSMITTER_ACTION_ENABLE
#define ATOM_TRANSMITTER_CONFIG_DIG2_ENCODER      0x08            // only used when ATOM_TRANSMITTER_ACTION_ENABLE

#define ATOM_TRANSMITTER_CONFIG_CLKSRC_MASK         0x30
#define ATOM_TRANSMITTER_CONFIG_CLKSRC_PPLL         0x00
#define ATOM_TRANSMITTER_CONFIG_CLKSRC_PCIE         0x20
#define ATOM_TRANSMITTER_CONFIG_CLKSRC_XTALIN      0x30
#define ATOM_TRANSMITTER_CONFIG_LANE_SEL_MASK      0xc0
#define ATOM_TRANSMITTER_CONFIG_LANE_0_3            0x00
#define ATOM_TRANSMITTER_CONFIG_LANE_0_7            0x00
#define ATOM_TRANSMITTER_CONFIG_LANE_4_7            0x40
#define ATOM_TRANSMITTER_CONFIG_LANE_8_11            0x80
#define ATOM_TRANSMITTER_CONFIG_LANE_8_15            0x80
#define ATOM_TRANSMITTER_CONFIG_LANE_12_15         0xc0

//ucAction
#define ATOM_TRANSMITTER_ACTION_DISABLE                      0
#define ATOM_TRANSMITTER_ACTION_ENABLE                      1
#define ATOM_TRANSMITTER_ACTION_LCD_BLOFF                   2
#define ATOM_TRANSMITTER_ACTION_LCD_BLON                   3
#define ATOM_TRANSMITTER_ACTION_BL_BRIGHTNESS_CONTROL  4
#define ATOM_TRANSMITTER_ACTION_LCD_SELFTEST_START       5
#define ATOM_TRANSMITTER_ACTION_LCD_SELFTEST_STOP          6
#define ATOM_TRANSMITTER_ACTION_INIT                         7
#define ATOM_TRANSMITTER_ACTION_DISABLE_OUTPUT          8
#define ATOM_TRANSMITTER_ACTION_ENABLE_OUTPUT             9
#define ATOM_TRANSMITTER_ACTION_SETUP                         10
#define ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH           11
#define ATOM_TRANSMITTER_ACTION_POWER_ON               12
#define ATOM_TRANSMITTER_ACTION_POWER_OFF              13

// Following are used for DigTransmitterControlTable ver1.2
typedef struct _ATOM_DIG_TRANSMITTER_CONFIG_V2
{
#if ATOM_BIG_ENDIAN
  UCHAR ucTransmitterSel:2;         //bit7:6: =0 Dig Transmitter 1 ( Uniphy AB )
                                    //        =1 Dig Transmitter 2 ( Uniphy CD )
                                    //        =2 Dig Transmitter 3 ( Uniphy EF )
  UCHAR ucReserved:1;
  UCHAR fDPConnector:1;             //bit4=0: DP connector  =1: None DP connector
  UCHAR ucEncoderSel:1;             //bit3=0: Data/Clk path source from DIGA( DIG inst0 ). =1: Data/clk path source from DIGB ( DIG inst1 )
  UCHAR ucLinkSel:1;                //bit2=0: Uniphy LINKA or C or E when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is A or C or E
                                    //    =1: Uniphy LINKB or D or F when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is B or D or F

  UCHAR fCoherentMode:1;            //bit1=1: Coherent Mode ( for DVI/HDMI mode )
  UCHAR fDualLinkConnector:1;       //bit0=1: Dual Link DVI connector
#else
  UCHAR fDualLinkConnector:1;       //bit0=1: Dual Link DVI connector
  UCHAR fCoherentMode:1;            //bit1=1: Coherent Mode ( for DVI/HDMI mode )
  UCHAR ucLinkSel:1;                //bit2=0: Uniphy LINKA or C or E when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is A or C or E
                                    //    =1: Uniphy LINKB or D or F when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is B or D or F
  UCHAR ucEncoderSel:1;             //bit3=0: Data/Clk path source from DIGA( DIG inst0 ). =1: Data/clk path source from DIGB ( DIG inst1 )
  UCHAR fDPConnector:1;             //bit4=0: DP connector  =1: None DP connector
  UCHAR ucReserved:1;
  UCHAR ucTransmitterSel:2;         //bit7:6: =0 Dig Transmitter 1 ( Uniphy AB )
                                    //        =1 Dig Transmitter 2 ( Uniphy CD )
                                    //        =2 Dig Transmitter 3 ( Uniphy EF )
#endif
}ATOM_DIG_TRANSMITTER_CONFIG_V2;

//ucConfig
//Bit0
#define ATOM_TRANSMITTER_CONFIG_V2_DUAL_LINK_CONNECTOR         0x01

//Bit1
#define ATOM_TRANSMITTER_CONFIG_V2_COHERENT                      0x02

//Bit2
#define ATOM_TRANSMITTER_CONFIG_V2_LINK_SEL_MASK              0x04
#define ATOM_TRANSMITTER_CONFIG_V2_LINKA                       0x00
#define ATOM_TRANSMITTER_CONFIG_V2_LINKB                        0x04

// Bit3
#define ATOM_TRANSMITTER_CONFIG_V2_ENCODER_SEL_MASK           0x08
#define ATOM_TRANSMITTER_CONFIG_V2_DIG1_ENCODER                0x00            // only used when ucAction == ATOM_TRANSMITTER_ACTION_ENABLE or ATOM_TRANSMITTER_ACTION_SETUP
#define ATOM_TRANSMITTER_CONFIG_V2_DIG2_ENCODER                0x08            // only used when ucAction == ATOM_TRANSMITTER_ACTION_ENABLE or ATOM_TRANSMITTER_ACTION_SETUP

// Bit4
#define ATOM_TRASMITTER_CONFIG_V2_DP_CONNECTOR                 0x10

// Bit7:6
#define ATOM_TRANSMITTER_CONFIG_V2_TRANSMITTER_SEL_MASK     0xC0
#define ATOM_TRANSMITTER_CONFIG_V2_TRANSMITTER1              0x00   //AB
#define ATOM_TRANSMITTER_CONFIG_V2_TRANSMITTER2              0x40   //CD
#define ATOM_TRANSMITTER_CONFIG_V2_TRANSMITTER3              0x80   //EF

typedef struct _DIG_TRANSMITTER_CONTROL_PARAMETERS_V2
{
   union
   {
  USHORT usPixelClock;      // in 10KHz; for bios convenient
   USHORT usInitInfo;         // when init uniphy,lower 8bit is used for connector type defined in objectid.h
  ATOM_DP_VS_MODE asMode; // DP Voltage swing mode
   };
  ATOM_DIG_TRANSMITTER_CONFIG_V2 acConfig;
   UCHAR ucAction;              // define as ATOM_TRANSMITER_ACTION_XXX
  UCHAR ucReserved[4];
}DIG_TRANSMITTER_CONTROL_PARAMETERS_V2;

typedef struct _ATOM_DIG_TRANSMITTER_CONFIG_V3
{
#if ATOM_BIG_ENDIAN
  UCHAR ucTransmitterSel:2;         //bit7:6: =0 Dig Transmitter 1 ( Uniphy AB )
                                    //        =1 Dig Transmitter 2 ( Uniphy CD )
                                    //        =2 Dig Transmitter 3 ( Uniphy EF )
  UCHAR ucRefClkSource:2;           //bit5:4: PPLL1 =0, PPLL2=1, EXT_CLK=2
  UCHAR ucEncoderSel:1;             //bit3=0: Data/Clk path source from DIGA/C/E. =1: Data/clk path source from DIGB/D/F
  UCHAR ucLinkSel:1;                //bit2=0: Uniphy LINKA or C or E when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is A or C or E
                                    //    =1: Uniphy LINKB or D or F when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is B or D or F
  UCHAR fCoherentMode:1;            //bit1=1: Coherent Mode ( for DVI/HDMI mode )
  UCHAR fDualLinkConnector:1;       //bit0=1: Dual Link DVI connector
#else
  UCHAR fDualLinkConnector:1;       //bit0=1: Dual Link DVI connector
  UCHAR fCoherentMode:1;            //bit1=1: Coherent Mode ( for DVI/HDMI mode )
  UCHAR ucLinkSel:1;                //bit2=0: Uniphy LINKA or C or E when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is A or C or E
                                    //    =1: Uniphy LINKB or D or F when fDualLinkConnector=0. when fDualLinkConnector=1, it means master link of dual link is B or D or F
  UCHAR ucEncoderSel:1;             //bit3=0: Data/Clk path source from DIGA/C/E. =1: Data/clk path source from DIGB/D/F
  UCHAR ucRefClkSource:2;           //bit5:4: PPLL1 =0, PPLL2=1, EXT_CLK=2
  UCHAR ucTransmitterSel:2;         //bit7:6: =0 Dig Transmitter 1 ( Uniphy AB )
                                    //        =1 Dig Transmitter 2 ( Uniphy CD )
                                    //        =2 Dig Transmitter 3 ( Uniphy EF )
#endif
}ATOM_DIG_TRANSMITTER_CONFIG_V3;


typedef struct _DIG_TRANSMITTER_CONTROL_PARAMETERS_V3
{
   union
   {
    USHORT usPixelClock;      // in 10KHz; for bios convenient
     USHORT usInitInfo;         // when init uniphy,lower 8bit is used for connector type defined in objectid.h
    ATOM_DP_VS_MODE as