ne ATOM_ENCODER_CONFIG_V2_TRANSMITTER3                0x10

// ucAction:
// ATOM_DISABLE
// ATOM_ENABLE
#define ATOM_ENCODER_CMD_DP_LINK_TRAINING_START       0x08
#define ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN1    0x09
#define ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN2    0x0a
#define ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN3    0x13
#define ATOM_ENCODER_CMD_DP_LINK_TRAINING_COMPLETE    0x0b
#define ATOM_ENCODER_CMD_DP_VIDEO_OFF                 0x0c
#define ATOM_ENCODER_CMD_DP_VIDEO_ON                  0x0d
#define ATOM_ENCODER_CMD_QUERY_DP_LINK_TRAINING_STATUS    0x0e
#define ATOM_ENCODER_CMD_SETUP                        0x0f
#define ATOM_ENCODER_CMD_SETUP_PANEL_MODE            0x10

// ucStatus
#define ATOM_ENCODER_STATUS_LINK_TRAINING_COMPLETE    0x10
#define ATOM_ENCODER_STATUS_LINK_TRAINING_INCOMPLETE  0x00

//ucTableFormatRevision=1
//ucTableContentRevision=3
// Following function ENABLE sub-function will be used by driver when TMDS/HDMI/LVDS is used, disable function will be used by driver
typedef struct _ATOM_DIG_ENCODER_CONFIG_V3
{
#if ATOM_BIG_ENDIAN
    UCHAR ucReserved1:1;
    UCHAR ucDigSel:3;             // =0/1/2/3/4/5: DIG0/1/2/3/4/5 (In register spec also referred as DIGA/B/C/D/E/F)
    UCHAR ucReserved:3;
    UCHAR ucDPLinkRate:1;         // =0: 1.62Ghz, =1: 2.7Ghz
#else
    UCHAR ucDPLinkRate:1;         // =0: 1.62Ghz, =1: 2.7Ghz
    UCHAR ucReserved:3;
    UCHAR ucDigSel:3;             // =0/1/2/3/4/5: DIG0/1/2/3/4/5 (In register spec also referred as DIGA/B/C/D/E/F)
    UCHAR ucReserved1:1;
#endif
}ATOM_DIG_ENCODER_CONFIG_V3;

#define ATOM_ENCODER_CONFIG_V3_DPLINKRATE_MASK            0x03
#define ATOM_ENCODER_CONFIG_V3_DPLINKRATE_1_62GHZ        0x00
#define ATOM_ENCODER_CONFIG_V3_DPLINKRATE_2_70GHZ        0x01
#define ATOM_ENCODER_CONFIG_V3_ENCODER_SEL                 0x70
#define ATOM_ENCODER_CONFIG_V3_DIG0_ENCODER                 0x00
#define ATOM_ENCODER_CONFIG_V3_DIG1_ENCODER                 0x10
#define ATOM_ENCODER_CONFIG_V3_DIG2_ENCODER                 0x20
#define ATOM_ENCODER_CONFIG_V3_DIG3_ENCODER                 0x30
#define ATOM_ENCODER_CONFIG_V3_DIG4_ENCODER                 0x40
#define ATOM_ENCODER_CONFIG_V3_DIG5_ENCODER                 0x50

typedef struct _DIG_ENCODER_CONTROL_PARAMETERS_V3
{
  USHORT usPixelClock;      // in 10KHz; for bios convenient
  ATOM_DIG_ENCODER_CONFIG_V3 acConfig;
  UCHAR ucAction;
  union{
    UCHAR ucEncoderMode;
                            // =0: DP   encoder
                            // =1: LVDS encoder
                            // =2: DVI  encoder
                            // =3: HDMI encoder
                            // =4: SDVO encoder
                            // =5: DP audio
    UCHAR ucPanelMode;        // only valid when ucAction == ATOM_ENCODER_CMD_SETUP_PANEL_MODE
                            // =0:     external DP
                            // =0x1:   internal DP2
                            // =0x11:  internal DP1 for NutMeg/Travis DP translator
  };
  UCHAR ucLaneNum;          // how many lanes to 