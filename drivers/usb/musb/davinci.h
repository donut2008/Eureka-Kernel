efore DEC30)
//                    LVTMAEncoderControlTable  (Before DEC30)
//                    TMDSAEncoderControlTable  (Before DEC30)
/****************************************************************************/
typedef struct _LVDS_ENCODER_CONTROL_PARAMETERS
{
  USHORT usPixelClock;  // in 10KHz; for bios convenient
  UCHAR  ucMisc;        // bit0=0: Enable single link
                        //     =1: Enable dual link
                        // Bit1=0: 666RGB
                        //     =1: 888RGB
  UCHAR  ucAction;      // 0: turn off encoder
                        // 1: setup and turn on encoder
}LVDS_ENCODER_CONTROL_PARAMETERS;

#define LVDS_ENCODER_CONTROL_PS_ALLOCATION  LVDS_ENCODER_CONTROL_PARAMETERS

#define TMDS1_ENCODER_CONTROL_PARAMETERS    LVDS_ENCODER_CONTROL_PARAMETERS
#define TMDS1_ENCODER_CONTROL_PS_ALLOCATION TMDS1_ENCODER_CONTROL_PARAMETERS

#define TMDS2_ENCODER_CONTROL_PARAMETERS    TMDS1_ENCODER_CONTROL_PARAMETERS
#define TMDS2_ENCODER_CONTROL_PS_ALLOCATION TMDS2_ENCODER_CONTROL_PARAMETERS

//ucTableFormatRevision=1,ucTableContentRevision=2
typedef struct _LVDS_ENCODER_CONTROL_PARAMETERS_V2
{
  USHORT usPixelClock;  // in 10KHz; for bios convenient
  UCHAR  ucMisc;        // see PANEL_ENCODER_MISC_xx defintions below
  UCHAR  ucAction;      // 0: turn off encoder
                        // 1: setup and turn on encoder
  UCHAR  ucTruncate;    // bit0=0: Disable truncate
                        //     =1: Enable truncate
                        // bit4=0: 666RGB
                        //     =1: 888RGB
  UCHAR  ucSpatial;     // bit0=0: Disable spatial dithering
                        //     =1: Enable spatial dithering
                        // bit4=0: 666RGB
                        //     =1: 888RGB
  UCHAR  ucTemporal;    // bit0=0: Disable temporal dithering
                        //     =1: Enable temporal dithering
                        // bit4=0: 666RGB
                        //     =1: 888RGB
                        // bit5=0: Gray level 2
                        //     =1: Gray level 4
  UCHAR  ucFRC;         // bit4=0: 25FRC_SEL pattern E
                        //     =1: 25FRC_SEL pattern F
                        // bit6:5=0: 50FRC_SEL pattern A
                        //       =1: 50FRC_SEL pattern B
                        //       =2: 50FRC_SEL pattern C
                        //       =3: 50FRC_SEL pattern D
                        // bit7=0: 75FRC_SEL pattern E
                        //     =1: 75FRC_SEL pattern F
}LVDS_ENCODER_CONTROL_PARAMETERS_V2;

#define LVDS_ENCODER_CONTROL_PS_ALLOCATION_V2  LVDS_ENCODER_CONTROL_PARAMETERS_V2

#define TMDS1_ENCODER_CONTROL_PARAMETERS_V2    LVDS_ENCODER_CONTROL_PARAMETERS_V2
#define TMDS1_ENCODER_CONTROL_PS_ALLOCATION_V2 TMDS1_ENCODER_CONTROL_PARAMETERS_V2

#define TMDS2_ENCODER_CONTROL_PARAMETERS_V2    TMDS1_ENCODER_CONTROL_PARAMETERS_V2
#define TMDS2_ENCODER_CONTROL_PS_ALLOCATION_V2 TMDS2_ENCODER_CONTROL_PARAMETERS_V2


#define LVDS_ENCODER_CONTROL_PARAMETERS_V3     LVDS_ENCODER_CONTROL_PARAMETERS_V2
#define LVDS_ENCODER_CONTROL_PS_ALLOCATION_V3  LVDS_ENCODER_CONTROL_PARAMETERS_V3

#define TMDS1_ENCODER_CONTROL_PARAMETERS_V3    LVDS_ENCODER_CONTROL_PARAMETERS_V3
#define TMDS1_ENCODER_CONTROL_PS_ALLOCATION_V3 TMDS1_ENCODER_CONTROL_PARAMETERS_V3

#define TMDS2_ENCODER_CONTROL_PARAMETERS_V3    LVDS_ENCODER_CONTROL_PARAMETERS