28 */
	AGC_EN_RSSI,               /* 29 */
	RFA_ENCLKRFAGC,            /* 30 */
	RFA_RSSI_REFH,             /* 31 */
	RFA_RSSI_REF,              /* 32 */
	RFA_RSSI_REFL,             /* 33 */
	RFA_FLR,                   /* 34 */
	RFA_CEIL,                  /* 35 */
	SEQ_EXTIQFSMPULSE,         /* 36 */
	OVERRIDE_1,                /* 37 */
	BB_INITSTATE_DLPF_TUNE,    /* 38 */
	TG_R_DIV,                  /* 39 */
	EN_CHP_LIN_B,              /* 40 */

	/* Channel Change Control Names */
	DN_POLY = 51,              /* 51 */
	DN_RFGAIN,                 /* 52 */
	DN_CAP_RFLPF,              /* 53 */
	DN_EN_VHFUHFBAR,           /* 54 */
	DN_GAIN_ADJUST,            /* 55 */
	DN_IQTNBUF_AMP,            /* 56 */
	DN_IQTNGNBFBIAS_BST,       /* 57 */
	RFSYN_EN_OUTMUX,           /* 58 */
	RFSYN_SEL_VCO_OUT,         /* 59 */
	RFSYN_SEL_VCO_HI,          /* 60 */
	RFSYN_SEL_DIVM,            /* 61 */
	RFSYN_RF_DIV_BIAS,         /* 62 */
	DN_SEL_FREQ,               /* 63 */
	RFSYN_VCO_BIAS,            /* 64 */
	CHCAL_INT_MOD_RF,          /* 65 */
	CHCAL_FRAC_MOD_RF,         /* 66 */
	RFSYN_LPF_R,               /* 67 */
	CHCAL_EN_INT_RF,           /* 68 */
	TG_LO_DIVVAL,              /* 69 */
	TG_LO_SELVAL,              /* 70 */
	TG_DIV_VAL,                /* 71 */
	TG_VCO_BIAS,               /* 72 */
	SEQ_EXTPOWERUP,            /* 73 */
	OVERRIDE_2,                /* 74 */
	OVERRIDE_3,                /* 75 */
	OVERRIDE_4,                /* 76 */
	SEQ_FSM_PULSE,             /* 77 */
	GPIO_4B,                   /* 78 */
	GPIO_3B,                   /* 79 */
	GPIO_4,                    /* 80 */
	GPIO_3,                    /* 81 */
	GPIO_1B,                   /* 82 */
	DAC_A_ENABLE,              /* 83 */
	DAC_B_ENABLE,              /* 84 */
	DAC_DIN_A,                 /* 85 */
	DAC_DIN_B,                 /* 86 */
#ifdef _MXL_PRODUCTION
	RFSYN_EN_DIV,              /* 87 */
	RFSYN_DIVM,                /* 88 */
	DN_BYPASS_AGC_I2C          /* 89 */
#endif
};

/*
 * The following context is source code provided by MaxLinear.
 * MaxLinear source code - Common_MXL.h (?)
 */

/* Constants */
#define MXL5005S_REG_WRITING_TABLE_LEN_MAX	104
#define MXL5005S_LATCH_BYTE			0xfe

/* Register address, MSB, and LSB */
#define MXL5005S_BB_IQSWAP_ADDR			59
#define MXL5005S_BB_IQSWAP_MSB			0
#define MXL5005S_BB_IQSWAP_LSB			0

#define MXL5005S_BB_DLPF_BANDSEL_ADDR		53
#define MXL5005S_BB_DLPF_BANDSEL_MSB		4
#define MXL5005S_BB_DLPF_BANDSEL_LSB		3

/* Standard modes */
enum {
	MXL5005S_STANDARD_DVBT,
	MXL5005S_STANDARD_ATSC,
};
#define MXL5005S_STANDARD_MODE_NUM		2

/* Bandwidth modes */
enum {
	MXL5005S_BANDWIDTH_6MHZ = 6000000,
	MXL5005S_BANDWIDTH_7MHZ = 7000000,
	MXL5005S_BANDWIDTH_8MHZ = 8000000,
};
#define MXL5005S_BANDWIDTH_MODE_NUM		3

/* MXL5005 Tuner Control Struct */
struct TunerControl {
	u16 Ctrl_Num;	/* Control Number */
	u16 size;	/* Number of bits to represent Value */
	u16 addr[25];	/* Array of Tuner Register Address for each bit pos */
	u16 bit[25];	/* Array of bit pos in Reg Addr for each bit pos */
	u16 val[25];	/* Binary representation of Value */
};

/* MXL5005 Tuner Struct */
struct mxl5005s_state {
	u8	Mode;		/* 0: Analog Mode ; 1: Digital Mode */
	u8	IF_Mode;	/* for Analog Mode, 0: zero IF; 1: low IF */
	u32	Chan_Bandwidth;	/* filter  channel bandwidth (6, 7, 8) */
	u32	IF_OUT;		/* Desired IF Out Frequency */
	u16	IF_OUT_LOAD;	/* IF Out Load Resistor (200/300 Ohms) */
	u32	RF_IN;		/* RF Input Frequency */
	u32	Fxtal;		/* XTAL Frequency */
	u8	AGC_Mode;	/* AGC Mode 0: Dual AGC; 1: Single AGC */
	u16	TOP;		/* Value: take over point */
	u8	CLOCK_OUT;	/* 0: turn off clk out; 1: turn on clock out */
	u8	DIV_OUT;	/* 4MHz or 16MHz */
	u8	CAPSELECT;	/* 0: disable On-Chip pulling cap; 1: enable */
	u8	EN_RSSI;	/* 0: disable RSSI; 1: enable RSSI */

	/* Modulation Type; */
	/* 0 - Default;	1 - DVB-T; 2 - ATSC; 3 - QAM; 4 - Analog Cable */
	u8	Mod_Type;

	/* Tracking Filter Type */
	/* 0 - Default; 1 - Off; 2 - Type C; 3 - Type C-H */
	u8	TF_Type;

	/* Calculated Settings */
	u32	RF_LO;		/* Synth RF LO Frequency */
	u32	IF_LO;		/* Synth IF LO Frequency */
	u32	TG_LO;		/* Synth TG_LO Frequency */

	/* Pointers to ControlName Arrays */
	u16	Init_Ctrl_Num;		/* Number of INIT Control Names */
	struct TunerControl
		Init_Ctrl[INITCTRL_NUM]; /* INIT Control Names Array Pointer */

	u16	CH_Ctrl_Num;		/* Number of CH Control Names */
	struct TunerControl
		CH_Ctrl[CHCTRL_NUM];	/* CH Control Name Array Pointer */

	u16	MXL_Ctrl_Num;		/* Number of MXL Control Names */
	struct TunerControl
		MXL_Ctrl[MXLCTRL_NUM];	/* MXL Control Name Array Pointer */

	/* Pointer to Tuner Register Array */
	u16	TunerRegs_Num;		/* Number of Tuner Registers */
	struct TunerReg
		TunerRegs[TUNER_REGS_NUM]; /* Tuner Register Array Pointer */

	/* Linux driver framework specific */
	struct mxl5005s_config *config;
	struct dvb_frontend *frontend;
	struct i2c_adapter *i2c;

	/* Cache values */
	u32 current_mode;

};

static u16 MXL_GetMasterControl(u8 *MasterReg, int state);
static u16 MXL_ControlWrite(struct dvb_frontend *fe, u16 ControlNum, u32 value);
static u16 MXL_ControlRead(struct dvb_frontend *fe, u16 controlNum, u32 *value);
static void MXL_RegWriteBit(struct dvb_frontend *fe, u8 address, u8 bit,
	u8 bitVal);
static u16 MXL_GetCHRegister(struct dvb_frontend *fe, u8 *RegNum,
	u8 *RegVal, int *count);
static u32 MXL_Ceiling(u32 value, u32 resolution);
static u16 MXL_RegRead(struct dvb_frontend *fe, u8 RegNum, u8 *RegVal);
static u16 MXL_ControlWrite_Group(struct dvb_frontend *fe, u16 controlNum,
	u32 value, u16 controlGroup);
static u16 MXL_SetGPIO(struct dvb_frontend *fe, u8 GPIO_Num, u8 GPIO_Val);
static u16 MXL_GetInitRegister(struct dvb_frontend *fe, u8 *RegNum,
	u8 *RegVal, int *count);
static u16 MXL_TuneRF(struct dvb_frontend *fe, u32 RF_Freq);
static void MXL_SynthIFLO_Calc(struct dvb_frontend *fe);
static void MXL_SynthRFTGLO_Calc(struct dvb_frontend *fe);
static u16 MXL_GetCHRegister_ZeroIF(struct dvb_frontend *fe, u8 *RegNum,
	u8 *RegVal, int *count);
static int mxl5005s_writeregs(struct dvb_frontend *fe, u8 *addrtable,
	u8 *datatable, u8 len);
static u16 MXL_IFSynthInit(struct dvb_frontend *fe);
static int mxl5005s_AssignTunerMode(struct dvb_frontend *fe, u32 mod_type,
	u32 bandwidth);
static int mxl5005s_reconfigure(struct dvb_frontend *fe, u32 mod_type,
	u32 bandwidth);

/* ----------------------------------------------------------------
 * Begin: Custom code salvaged from the Realtek driver.
 * Copyright (C) 2008 Realtek
 * Copyright (C) 2008 Jan Hoogenraad
 * This code is placed under the terms of the GNU General Public License
 *
 * Released by Realtek under GPLv2.
 * Thanks to Realtek for a lot of support we received !
 *
 *  Revision: 080314 - original version
 */

static int mxl5005s_SetRfFreqHz(struct dvb_frontend *fe, unsigned long RfFreqHz)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	unsigned char AddrTable[MXL5005S_REG_WRITING_TABLE_LEN_MAX];
	unsigned char ByteTable[MXL5005S_REG_WRITING_TABLE_LEN_MAX];
	int TableLen;

	u32 IfDivval = 0;
	unsigned char MasterControlByte;

	dprintk(1, "%s() freq=%ld\n", __func__, RfFreqHz);

	/* Set MxL5005S tuner RF frequency according to example code. */

	/* Tuner RF frequency setting stage 0 */
	MXL_GetMasterControl(ByteTable, MC_SYNTH_RESET);
	AddrTable[0] = MASTER_CONTROL_ADDR;
	ByteTable[0] |= state->config->AgcMasterByte;

	mxl5005s_writeregs(fe, AddrTable, ByteTable, 1);

	/* Tuner RF frequency setting stage 1 */
	MXL_TuneRF(fe, RfFreqHz);

	MXL_ControlRead(fe, IF_DIVVAL, &IfDivval);

	MXL_ControlWrite(fe, SEQ_FSM_PULSE, 0);
	MXL_ControlWrite(fe, SEQ_EXTPOWERUP, 1);
	MXL_ControlWrite(fe, IF_DIVVAL, 8);
	MXL_GetCHRegister(fe, AddrTable, ByteTable, &TableLen);

	MXL_GetMasterControl(&MasterControlByte, MC_LOAD_START);
	AddrTable[TableLen] = MASTER_CONTROL_ADDR ;
	ByteTable[TableLen] = MasterControlByte |
		state->config->AgcMasterByte;
	TableLen += 1;

	mxl5005s_writeregs(fe, AddrTable, ByteTable, TableLen);

	/* Wait 30 ms. */
	msleep(150);

	/* Tuner RF frequency setting stage 2 */
	MXL_ControlWrite(fe, SEQ_FSM_PULSE, 1);
	MXL_ControlWrite(fe, IF_DIVVAL, IfDivval);
	MXL_GetCHRegister_ZeroIF(fe, AddrTable, ByteTable, &TableLen);

	MXL_GetMasterControl(&MasterControlByte, MC_LOAD_START);
	AddrTable[TableLen] = MASTER_CONTROL_ADDR ;
	ByteTable[TableLen] = MasterControlByte |
		state->config->AgcMasterByte ;
	TableLen += 1;

	mxl5005s_writeregs(fe, AddrTable, ByteTable, TableLen);

	msleep(100);

	return 0;
}
/* End: Custom code taken from the Realtek driver */

/* ----------------------------------------------------------------
 * Begin: Reference driver code found in the Realtek driver.
 * Copyright (C) 2008 MaxLinear
 */
static u16 MXL5005_RegisterInit(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	state->TunerRegs_Num = TUNER_REGS_NUM ;

	state->TunerRegs[0].Reg_Num = 9 ;
	state->TunerRegs[0].Reg_Val = 0x40 ;

	state->TunerRegs[1].Reg_Num = 11 ;
	state->TunerRegs[1].Reg_Val = 0x19 ;

	state->TunerRegs[2].Reg_Num = 12 ;
	state->TunerRegs[2].Reg_Val = 0x60 ;

	state->TunerRegs[3].Reg_Num = 13 ;
	state->TunerRegs[3].Reg_Val = 0x00 ;

	state->TunerRegs[4].Reg_Num = 14 ;
	state->TunerRegs[4].Reg_Val = 0x00 ;

	state->TunerRegs[5].Reg_Num = 15 ;
	state->TunerRegs[5].Reg_Val = 0xC0 ;

	state->TunerRegs[6].Reg_Num = 16 ;
	state->TunerRegs[6].Reg_Val = 0x00 ;

	state->TunerRegs[7].Reg_Num = 17 ;
	state->TunerRegs[7].Reg_Val = 0x00 ;

	state->TunerRegs[8].Reg_Num = 18 ;
	state->TunerRegs[8].Reg_Val = 0x00 ;

	state->TunerRegs[9].Reg_Num = 19 ;
	state->TunerRegs[9].Reg_Val = 0x34 ;

	state->TunerRegs[10].Reg_Num = 21 ;
	state->TunerRegs[10].Reg_Val = 0x00 ;

	state->TunerRegs[11].Reg_Num = 22 ;
	state->TunerRegs[11].Reg_Val = 0x6B ;

	state->TunerRegs[12].Reg_Num = 23 ;
	state->TunerRegs[12].Reg_Val = 0x35 ;

	state->TunerRegs[13].Reg_Num = 24 ;
	state->TunerRegs[13].Reg_Val = 0x70 ;

	state->TunerRegs[14].Reg_Num = 25 ;
	state->TunerRegs[14].Reg_Val = 0x3E ;

	state->TunerRegs[15].Reg_Num = 26 ;
	state->TunerRegs[15].Reg_Val = 0x82 ;

	state->TunerRegs[16].Reg_Num = 31 ;
	state->TunerRegs[16].Reg_Val = 0x00 ;

	state->TunerRegs[17].Reg_Num = 32 ;
	state->TunerRegs[17].Reg_Val = 0x40 ;

	state->TunerRegs[18].Reg_Num = 33 ;
	state->TunerRegs[18].Reg_Val = 0x53 ;

	state->TunerRegs[19].Reg_Num = 34 ;
	state->TunerRegs[19].Reg_Val = 0x81 ;

	state->TunerRegs[20].Reg_Num = 35 ;
	state->TunerRegs[20].Reg_Val = 0xC9 ;

	state->TunerRegs[21].Reg_Num = 36 ;
	state->TunerRegs[21].Reg_Val = 0x01 ;

	state->TunerRegs[22].Reg_Num = 37 ;
	state->TunerRegs[22].Reg_Val = 0x00 ;

	state->TunerRegs[23].Reg_Num = 41 ;
	state->TunerRegs[23].Reg_Val = 0x00 ;

	state->TunerRegs[24].Reg_Num = 42 ;
	state->TunerRegs[24].Reg_Val = 0xF8 ;

	state->TunerRegs[25].Reg_Num = 43 ;
	state->TunerRegs[25].Reg_Val = 0x43 ;

	state->TunerRegs[26].Reg_Num = 44 ;
	state->TunerRegs[26].Reg_Val = 0x20 ;

	state->TunerRegs[27].Reg_Num = 45 ;
	state->TunerRegs[27].Reg_Val = 0x80 ;

	state->TunerRegs[28].Reg_Num = 46 ;
	state->TunerRegs[28].Reg_Val = 0x88 ;

	state->TunerRegs[29].Reg_Num = 47 ;
	state->TunerRegs[29].Reg_Val = 0x86 ;

	state->TunerRegs[30].Reg_Num = 48 ;
	state->TunerRegs[30].Reg_Val = 0x00 ;

	state->TunerRegs[31].Reg_Num = 49 ;
	state->TunerRegs[31].Reg_Val = 0x00 ;

	state->TunerRegs[32].Reg_Num = 53 ;
	state->TunerRegs[32].Reg_Val = 0x94 ;

	state->TunerRegs[33].Reg_Num = 54 ;
	state->TunerRegs[33].Reg_Val = 0xFA ;

	state->TunerRegs[34].Reg_Num = 55 ;
	state->TunerRegs[34].Reg_Val = 0x92 ;

	state->TunerRegs[35].Reg_Num = 56 ;
	state->TunerRegs[35].Reg_Val = 0x80 ;

	state->TunerRegs[36].Reg_Num = 57 ;
	state->TunerRegs[36].Reg_Val = 0x41 ;

	state->TunerRegs[37].Reg_Num = 58 ;
	state->TunerRegs[37].Reg_Val = 0xDB ;

	state->TunerRegs[38].Reg_Num = 59 ;
	state->TunerRegs[38].Reg_Val = 0x00 ;

	state->TunerRegs[39].Reg_Num = 60 ;
	state->TunerRegs[39].Reg_Val = 0x00 ;

	state->TunerRegs[40].Reg_Num = 61 ;
	state->TunerRegs[40].Reg_Val = 0x00 ;

	state->TunerRegs[41].Reg_Num = 62 ;
	state->TunerRegs[41].Reg_Val = 0x00 ;

	state->TunerRegs[42].Reg_Num = 65 ;
	state->TunerRegs[42].Reg_Val = 0xF8 ;

	state->TunerRegs[43].Reg_Num = 66 ;
	state->TunerRegs[43].Reg_Val = 0xE4 ;

	state->TunerRegs[44].Reg_Num = 67 ;
	state->TunerRegs[44].Reg_Val = 0x90 ;

	state->TunerRegs[45].Reg_Num = 68 ;
	state->TunerRegs[45].Reg_Val = 0xC0 ;

	state->TunerRegs[46].Reg_Num = 69 ;
	state->TunerRegs[46].Reg_Val = 0x01 ;

	state->TunerRegs[47].Reg_Num = 70 ;
	state->TunerRegs[47].Reg_Val = 0x50 ;

	state->TunerRegs[48].Reg_Num = 71 ;
	state->TunerRegs[48].Reg_Val = 0x06 ;

	state->TunerRegs[49].Reg_Num = 72 ;
	state->TunerRegs[49].Reg_Val = 0x00 ;

	state->TunerRegs[50].Reg_Num = 73 ;
	state->TunerRegs[50].Reg_Val = 0x20 ;

	state->TunerRegs[51].Reg_Num = 76 ;
	state->TunerRegs[51].Reg_Val = 0xBB ;

	state->TunerRegs[52].Reg_Num = 77 ;
	state->TunerRegs[52].Reg_Val = 0x13 ;

	state->TunerRegs[53].Reg_Num = 81 ;
	state->TunerRegs[53].Reg_Val = 0x04 ;

	state->TunerRegs[54].Reg_Num = 82 ;
	state->TunerRegs[54].Reg_Val = 0x75 ;

	state->TunerRegs[55].Reg_Num = 83 ;
	state->TunerRegs[55].Reg_Val = 0x00 ;

	state->TunerRegs[56].Reg_Num = 84 ;
	state->TunerRegs[56].Reg_Val = 0x00 ;

	state->TunerRegs[57].Reg_Num = 85 ;
	state->TunerRegs[57].Reg_Val = 0x00 ;

	state->TunerRegs[58].Reg_Num = 91 ;
	state->TunerRegs[58].Reg_Val = 0x70 ;

	state->TunerRegs[59].Reg_Num = 92 ;
	state->TunerRegs[59].Reg_Val = 0x00 ;

	state->TunerRegs[60].Reg_Num = 93 ;
	state->TunerRegs[60].Reg_Val = 0x00 ;

	state->TunerRegs[61].Reg_Num = 94 ;
	state->TunerRegs[61].Reg_Val = 0x00 ;

	state->TunerRegs[62].Reg_Num = 95 ;
	state->TunerRegs[62].Reg_Val = 0x0C ;

	state->TunerRegs[63].Reg_Num = 96 ;
	state->TunerRegs[63].Reg_Val = 0x00 ;

	state->TunerRegs[64].Reg_Num = 97 ;
	state->TunerRegs[64].Reg_Val = 0x00 ;

	state->TunerRegs[65].Reg_Num = 98 ;
	state->TunerRegs[65].Reg_Val = 0xE2 ;

	state->TunerRegs[66].Reg_Num = 99 ;
	state->TunerRegs[66].Reg_Val = 0x00 ;

	state->TunerRegs[67].Reg_Num = 100 ;
	state->TunerRegs[67].Reg_Val = 0x00 ;

	state->TunerRegs[68].Reg_Num = 101 ;
	state->TunerRegs[68].Reg_Val = 0x12 ;

	state->TunerRegs[69].Reg_Num = 102 ;
	state->TunerRegs[69].Reg_Val = 0x80 ;

	state->TunerRegs[70].Reg_Num = 103 ;
	state->TunerRegs[70].Reg_Val = 0x32 ;

	state->TunerRegs[71].Reg_Num = 104 ;
	state->TunerRegs[71].Reg_Val = 0xB4 ;

	state->TunerRegs[72].Reg_Num = 105 ;
	state->TunerRegs[72].Reg_Val = 0x60 ;

	state->TunerRegs[73].Reg_Num = 106 ;
	state->TunerRegs[73].Reg_Val = 0x83 ;

	state->TunerRegs[74].Reg_Num = 107 ;
	state->TunerRegs[74].Reg_Val = 0x84 ;

	state->TunerRegs[75].Reg_Num = 108 ;
	state->TunerRegs[75].Reg_Val = 0x9C ;

	state->TunerRegs[76].Reg_Num = 109 ;
	state->TunerRegs[76].Reg_Val = 0x02 ;

	state->TunerRegs[77].Reg_Num = 110 ;
	state->TunerRegs[77].Reg_Val = 0x81 ;

	state->TunerRegs[78].Reg_Num = 111 ;
	state->TunerRegs[78].Reg_Val = 0xC0 ;

	state->TunerRegs[79].Reg_Num = 112 ;
	state->TunerRegs[79].Reg_Val = 0x10 ;

	state->TunerRegs[80].Reg_Num = 131 ;
	state->TunerRegs[80].Reg_Val = 0x8A ;

	state->TunerRegs[81].Reg_Num = 132 ;
	state->TunerRegs[81].Reg_Val = 0x10 ;

	state->TunerRegs[82].Reg_Num = 133 ;
	state->TunerRegs[82].Reg_Val = 0x24 ;

	state->TunerRegs[83].Reg_Num = 134 ;
	state->TunerRegs[83].Reg_Val = 0x00 ;

	state->TunerRegs[84].Reg_Num = 135 ;
	state->TunerRegs[84].Reg_Val = 0x00 ;

	state->TunerRegs[85].Reg_Num = 136 ;
	state->TunerRegs[85].Reg_Val = 0x7E ;

	state->TunerRegs[86].Reg_Num = 137 ;
	state->TunerRegs[86].Reg_Val = 0x40 ;

	state->TunerRegs[87].Reg_Num = 138 ;
	state->TunerRegs[87].Reg_Val = 0x38 ;

	state->TunerRegs[88].Reg_Num = 146 ;
	state->TunerRegs[88].Reg_Val = 0xF6 ;

	state->TunerRegs[89].Reg_Num = 147 ;
	state->TunerRegs[89].Reg_Val = 0x1A ;

	state->TunerRegs[90].Reg_Num = 148 ;
	state->TunerRegs[90].Reg_Val = 0x62 ;

	state->TunerRegs[91].Reg_Num = 149 ;
	state->TunerRegs[91].Reg_Val = 0x33 ;

	state->TunerRegs[92].Reg_Num = 150 ;
	state->TunerRegs[92].Reg_Val = 0x80 ;

	state->TunerRegs[93].Reg_Num = 156 ;
	state->TunerRegs[93].Reg_Val = 0x56 ;

	state->TunerRegs[94].Reg_Num = 157 ;
	state->TunerRegs[94].Reg_Val = 0x17 ;

	state->TunerRegs[95].Reg_Num = 158 ;
	state->TunerRegs[95].Reg_Val = 0xA9 ;

	state->TunerRegs[96].Reg_Num = 159 ;
	state->TunerRegs[96].Reg_Val = 0x00 ;

	state->TunerRegs[97].Reg_Num = 160 ;
	state->TunerRegs[97].Reg_Val = 0x00 ;

	state->TunerRegs[98].Reg_Num = 161 ;
	state->TunerRegs[98].Reg_Val = 0x00 ;

	state->TunerRegs[99].Reg_Num = 162 ;
	state->TunerRegs[99].Reg_Val = 0x40 ;

	state->TunerRegs[100].Reg_Num = 166 ;
	state->TunerRegs[100].Reg_Val = 0xAE ;

	state->TunerRegs[101].Reg_Num = 167 ;
	state->TunerRegs[101].Reg_Val = 0x1B ;

	state->TunerRegs[102].Reg_Num = 168 ;
	state->TunerRegs[102].Reg_Val = 0xF2 ;

	state->TunerRegs[103].Reg_Num = 195 ;
	state->TunerRegs[103].Reg_Val = 0x00 ;

	return 0 ;
}

static u16 MXL5005_ControlInit(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	state->Init_Ctrl_Num = INITCTRL_NUM;

	state->Init_Ctrl[0].Ctrl_Num = DN_IQTN_AMP_CUT ;
	state->Init_Ctrl[0].size = 1 ;
	state->Init_Ctrl[0].addr[0] = 73;
	state->Init_Ctrl[0].bit[0] = 7;
	state->Init_Ctrl[0].val[0] = 0;

	state->Init_Ctrl[1].Ctrl_Num = BB_MODE ;
	state->Init_Ctrl[1].size = 1 ;
	state->Init_Ctrl[1].addr[0] = 53;
	state->Init_Ctrl[1].bit[0] = 2;
	state->Init_Ctrl[1].val[0] = 1;

	state->Init_Ctrl[2].Ctrl_Num = BB_BUF ;
	state->Init_Ctrl[2].size = 2 ;
	state->Init_Ctrl[2].addr[0] = 53;
	state->Init_Ctrl[2].bit[0] = 1;
	state->Init_Ctrl[2].val[0] = 0;
	state->Init_Ctrl[2].addr[1] = 57;
	state->Init_Ctrl[2].bit[1] = 0;
	state->Init_Ctrl[2].val[1] = 1;

	state->Init_Ctrl[3].Ctrl_Num = BB_BUF_OA ;
	state->Init_Ctrl[3].size = 1 ;
	state->Init_Ctrl[3].addr[0] = 53;
	state->Init_Ctrl[3].bit[0] = 0;
	state->Init_Ctrl[3].val[0] = 0;

	state->Init_Ctrl[4].Ctrl_Num = BB_ALPF_BANDSELECT ;
	state->Init_Ctrl[4].size = 3 ;
	state->Init_Ctrl[4].addr[0] = 53;
	state->Init_Ctrl[4].bit[0] = 5;
	state->Init_Ctrl[4].val[0] = 0;
	state->Init_Ctrl[4].addr[1] = 53;
	state->Init_Ctrl[4].bit[1] = 6;
	state->Init_Ctrl[4].val[1] = 0;
	state->Init_Ctrl[4].addr[2] = 53;
	state->Init_Ctrl[4].bit[2] = 7;
	state->Init_Ctrl[4].val[2] = 1;

	state->Init_Ctrl[5].Ctrl_Num = BB_IQSWAP ;
	state->Init_Ctrl[5].size = 1 ;
	state->Init_Ctrl[5].addr[0] = 59;
	state->Init_Ctrl[5].bit[0] = 0;
	state->Init_Ctrl[5].val[0] = 0;

	state->Init_Ctrl[6].Ctrl_Num = BB_DLPF_BANDSEL ;
	state->Init_Ctrl[6].size = 2 ;
	state->Init_Ctrl[6].addr[0] = 53;
	state->Init_Ctrl[6].bit[0] = 3;
	state->Init_Ctrl[6].val[0] = 0;
	state->Init_Ctrl[6].addr[1] = 53;
	state->Init_Ctrl[6].bit[1] = 4;
	state->Init_Ctrl[6].val[1] = 1;

	state->Init_Ctrl[7].Ctrl_Num = RFSYN_CHP_GAIN ;
	state->Init_Ctrl[7].size = 4 ;
	state->Init_Ctrl[7].addr[0] = 22;
	state->Init_Ctrl[7].bit[0] = 4;
	state->Init_Ctrl[7].val[0] = 0;
	state->Init_Ctrl[7].addr[1] = 22;
	state->Init_Ctrl[7].bit[1] = 5;
	state->Init_Ctrl[7].val[1] = 1;
	state->Init_Ctrl[7].addr[2] = 22;
	state->Init_Ctrl[7].bit[2] = 6;
	state->Init_Ctrl[7].val[2] = 1;
	state->Init_Ctrl[7].addr[3] = 22;
	state->Init_Ctrl[7].bit[3] = 7;
	state->Init_Ctrl[7].val[3] = 0;

	state->Init_Ctrl[8].Ctrl_Num = RFSYN_EN_CHP_HIGAIN ;
	state->Init_Ctrl[8].size = 1 ;
	state->Init_Ctrl[8].addr[0] = 22;
	state->Init_Ctrl[8].bit[0] = 2;
	state->Init_Ctrl[8].val[0] = 0;

	state->Init_Ctrl[9].Ctrl_Num = AGC_IF ;
	state->Init_Ctrl[9].size = 4 ;
	state->Init_Ctrl[9].addr[0] = 76;
	state->Init_Ctrl[9].bit[0] = 0;
	state->Init_Ctrl[9].val[0] = 1;
	state->Init_Ctrl[9].addr[1] = 76;
	state->Init_Ctrl[9].bit[1] = 1;
	state->Init_Ctrl[9].val[1] = 1;
	state->Init_Ctrl[9].addr[2] = 76;
	state->Init_Ctrl[9].bit[2] = 2;
	state->Init_Ctrl[9].val[2] = 0;
	state->Init_Ctrl[9].addr[3] = 76;
	state->Init_Ctrl[9].bit[3] = 3;
	state->Init_Ctrl[9].val[3] = 1;

	state->Init_Ctrl[10].Ctrl_Num = AGC_RF ;
	state->Init_Ctrl[10].size = 4 ;
	state->Init_Ctrl[10].addr[0] = 76;
	state->Init_Ctrl[10].bit[0] = 4;
	state->Init_Ctrl[10].val[0] = 1;
	state->Init_Ctrl[10].addr[1] = 76;
	state->Init_Ctrl[10].bit[1] = 5;
	state->Init_Ctrl[10].val[1] = 1;
	state->Init_Ctrl[10].addr[2] = 76;
	state->Init_Ctrl[10].bit[2] = 6;
	state->Init_Ctrl[10].val[2] = 0;
	state->Init_Ctrl[10].addr[3] = 76;
	state->Init_Ctrl[10].bit[3] = 7;
	state->Init_Ctrl[10].val[3] = 1;

	state->Init_Ctrl[11].Ctrl_Num = IF_DIVVAL ;
	state->Init_Ctrl[11].size = 5 ;
	state->Init_Ctrl[11].addr[0] = 43;
	state->Init_Ctrl[11].bit[0] = 3;
	state->Init_Ctrl[11].val[0] = 0;
	state->Init_Ctrl[11].addr[1] = 43;
	state->Init_Ctrl[11].bit[1] = 4;
	state->Init_Ctrl[11].val[1] = 0;
	state->Init_Ctrl[11].addr[2] = 43;
	state->Init_Ctrl[11].bit[2] = 5;
	state->Init_Ctrl[11].val[2] = 0;
	state->Init_Ctrl[11].addr[3] = 43;
	state->Init_Ctrl[11].bit[3] = 6;
	state->Init_Ctrl[11].val[3] = 1;
	state->Init_Ctrl[11].addr[4] = 43;
	state->Init_Ctrl[11].bit[4] = 7;
	state->Init_Ctrl[11].val[4] = 0;

	state->Init_Ctrl[12].Ctrl_Num = IF_VCO_BIAS ;
	state->Init_Ctrl[12].size = 6 ;
	state->Init_Ctrl[12].addr[0] = 44;
	state->Init_Ctrl[12].bit[0] = 2;
	state->Init_Ctrl[12].val[0] = 0;
	state->Init_Ctrl[12].addr[1] = 44;
	state->Init_Ctrl[12].bit[1] = 3;
	state->Init_Ctrl[12].val[1] = 0;
	state->Init_Ctrl[12].addr[2] = 44;
	state->Init_Ctrl[12