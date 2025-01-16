ndicate max Voltage
    UCHAR      ucReserved[3];
    VOLTAGE_LUT_ENTRY asVIDAdjustEntries[32];// 32 is for allocation, the actual number of entries is in ucNumOfVoltageEntries
}ATOM_VOLTAGE_FORMULA_V2;

typedef struct _ATOM_VOLTAGE_CONTROL
{
  UCHAR    ucVoltageControlId;                     //Indicate it is controlled by I2C or GPIO or HW state machine
  UCHAR    ucVoltageControlI2cLine;
  UCHAR    ucVoltageControlAddress;
  UCHAR    ucVoltageControlOffset;
  USHORT   usGpioPin_AIndex;                       //GPIO_PAD register index
  UCHAR    ucGpioPinBitShift[9];                   //at most 8 pin support 255 VIDs, termintate with 0xff
  UCHAR    ucReserved;
}ATOM_VOLTAGE_CONTROL;

// Define ucVoltageControlId
#define VOLTAGE_CONTROLLED_BY_HW              0x00
#define VOLTAGE_CONTROLLED_BY_I2C_MASK        0x7F
#define VOLTAGE_CONTROLLED_BY_GPIO            0x80
#define VOLTAGE_CONTROL_ID_LM64               0x01                           //I2C control, used for R5xx Core Voltage
#define VOLTAGE_CONTROL_ID_DAC                0x02                           //I2C control, used for R5xx/R6xx MVDDC,MVDDQ or VDDCI
#define VOLTAGE_CONTROL_ID_VT116xM            0x03                           //I2C control, used for R6xx Core Voltage
#define VOLTAGE_CONTROL_ID_DS4402             0x04
#define VOLTAGE_CONTROL_ID_UP6266             0x05
#define VOLTAGE_CONTROL_ID_SCORPIO            0x06
#define VOLTAGE_CONTROL_ID_VT1556M            0x07
#define VOLTAGE_CONTROL_ID_CHL822x            0x08
#define VOLTAGE_CONTROL_ID_VT1586M            0x09
#define VOLTAGE_CONTROL_ID_UP1637             0x0A
#define VOLTAGE_CONTROL_ID_CHL8214            0x0B
#define VOLTAGE_CONTROL_ID_UP1801             0x0C
#define VOLTAGE_CONTROL_ID_ST6788A            0x0D
#define VOLTAGE_CONTROL_ID_CHLIR3564SVI2      0x0E
#define VOLTAGE_CONTROL_ID_AD527x      	      0x0F
#define VOLTAGE_CONTROL_ID_NCP81022    	      0x10
#define VOLTAGE_CONTROL_ID_LTC2635			  0x11
#define VOLTAGE_CONTROL_ID_NCP4208	          0x12
#define VOLTAGE_CONTROL_ID_IR35xx             0x13
#define VOLTAGE_CONTROL_ID_RT9403	          0x14

#define VOLTAGE_CONTROL_ID_GENERIC_I2C        0x40

typedef struct  _ATOM_VOLTAGE_OBJECT
{
   UCHAR      ucVoltageType;           