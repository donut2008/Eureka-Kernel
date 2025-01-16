ef struct _ATOM_INTEGRATED_SYSTEM_INFO_V5
{
  ATOM_COMMON_TABLE_HEADER   sHeader;
  ULONG                        ulBootUpEngineClock;       //in 10kHz unit
  ULONG                      ulDentistVCOFreq;          //Dentist VCO clock in 10kHz unit, the source of GPU SCLK, LCLK, UCLK and VCLK.
  ULONG                      ulLClockFreq;              //GPU Lclk freq in 10kHz unit, have relationship with NCLK in NorthBridge
  ULONG                        ulBootUpUMAClock;          //in 10kHz unit
  ULONG                      ulReserved1[8];            //must be 0x0 for the reserved
  ULONG                      ulBootUpReqDisplayVector;
  ULONG                      ulOtherDisplayMisc;
  ULONG                      ulReserved2[4];            //must be 0x0 for the reserved
  ULONG                      ulSystemConfig;            //TBD
  ULONG                      ulCPUCapInfo;              //TBD
  USHORT                     usMaxNBVoltage;            //high NB voltage, calculated using current VDDNB (D24F2xDC) and VDDNB offset fuse;
  USHORT                     usMinNBVoltage;            //low NB voltage, calculated using current VDDNB (D24F2xDC) and VDDNB offset fuse;
  USHORT                     usBootUpNBVoltage;         //boot up NB voltage
  UCHAR                      ucHtcTmpLmt;               //bit [22:16] of D24F3x64 Hardware Thermal Control (HTC) Register, may not 