 109;
	state->CH_Ctrl[20].bit[2] = 4;
	state->CH_Ctrl[20].val[2] = 0;
	state->CH_Ctrl[20].addr[3] = 109;
	state->CH_Ctrl[20].bit[3] = 5;
	state->CH_Ctrl[20].val[3] = 0;
	state->CH_Ctrl[20].addr[4] = 109;
	state->CH_Ctrl[20].bit[4] = 6;
	state->CH_Ctrl[20].val[4] = 0;
	state->CH_Ctrl[20].addr[5] = 109;
	state->CH_Ctrl[20].bit[5] = 7;
	state->CH_Ctrl[20].val[5] = 0;
	state->CH_Ctrl[20].addr[6] = 108;
	state->CH_Ctrl[20].bit[6] = 0;
	state->CH_Ctrl[20].val[6] = 0;
	state->CH_Ctrl[20].addr[7] = 108;
	state->CH_Ctrl[20].bit[7] = 1;
	state->CH_Ctrl[20].val[7] = 0;
	state->CH_Ctrl[20].addr[8] = 108;
	state->CH_Ctrl[20].bit[8] = 2;
	state->CH_Ctrl[20].val[8] = 1;
	state->CH_Ctrl[20].addr[9] = 108;
	state->CH_Ctrl[20].bit[9] = 3;
	state->CH_Ctrl[20].val[9] = 1;
	state->CH_Ctrl[20].addr[10] = 108;
	state->CH_Ctrl[20].bit[10] = 4;
	state->CH_Ctrl[20].val[10] = 1;

	state->CH_Ctrl[21].Ctrl_Num = TG_VCO_BIAS ;
	state->CH_Ctrl[21].size = 6 ;
	state->CH_Ctrl[21].addr[0] = 106;
	state->CH_Ctrl[21].bit[0] = 2;
	state->CH_Ctrl[21].val[0] = 0;
	state->CH_Ctrl[21].addr[1] = 106;
	state->CH_Ctrl[21].bit[1] = 3;
	state->CH_Ctrl[21].val[1] = 0;
	state->CH_Ctrl[21].addr[2] = 106;
	state->CH_Ctrl[21].bit[2] = 4;
	state->CH_Ctrl[21].val[2] = 0;
	state->CH_Ctrl[21].addr[3] = 106;
	state->CH_Ctrl[21].bit[3] = 5;
	state->CH_Ctrl[21].val[3] = 0;
	state->CH_Ctrl[21].addr[4] = 106;
	state->CH_Ctrl[21].bit[4] = 6;
	state->CH_Ctrl[21].val[4] = 0;
	state->CH_Ctrl[21].addr[5] = 106;
	state->CH_Ctrl[21].bit[5] = 7;
	state->CH_Ctrl[21].val[5] = 1;

	state->CH_Ctrl[22].Ctrl_Num = SEQ_EXTPOWERUP ;
	state->CH_Ctrl[22].size = 1 ;
	state->CH_Ctrl[22].addr[0] = 138;
	state->CH_Ctrl[22].bit[0] = 4;
	state->CH_Ctrl[22].val[0] = 1;

	state->CH_Ctrl[23].Ctrl_Num = OVERRIDE_2 ;
	state->CH_Ctrl[23].size = 1 ;
	state->CH_Ctrl[23].addr[0] = 17;
	state->CH_Ctrl[23].bit[0] = 5;
	state->CH_Ctrl[23].val[0] = 0;

	state->CH_Ctrl[24].Ctrl_Num = OVERRIDE_3 ;
	state->CH_Ctrl[24].size = 1 ;
	state->CH_Ctrl[24].addr[0] = 111;
	state->CH_Ctrl[24].bit[0] = 3;
	state->CH_Ctrl[24].val[0] = 0;

	state->CH_Ctrl[25].Ctrl_Num = OVERRIDE_4 ;
	state->CH_Ctrl[25].size = 1 ;
	state->CH_Ctrl[25].addr[0] = 112;
	state->CH_Ctrl[25].bit[0] = 7;
	state->CH_Ctrl[25].val[0] = 0;

	state->CH_Ctrl[26].Ctrl_Num = SEQ_FSM_PULSE ;
	state->CH_Ctrl[26].size = 1 ;
	state->CH_Ctrl[26].addr[0] = 136;
	state->CH_Ctrl[26].bit[0] = 7;
	state->CH_Ctrl[26].val[0] = 0;

	state->CH_Ctrl[27].Ctrl_Num = GPIO_4B ;
	state->CH_Ctrl[27].size = 1 ;
	state->CH_Ctrl[27].addr[0] = 149;
	state->CH_Ctrl[27].bit[0] = 7;
	state->CH_Ctrl[27].val[0] = 0;

	state->CH_Ctrl[28].Ctrl_Num = GPIO_3B ;
	state->CH_Ctrl[28].size = 1 ;
	state->CH_Ctrl[28].addr[0] = 149;
	state->CH_Ctrl[28].bit[0] = 6;
	state->CH_Ctrl[28].val[0] = 0;

	state->CH_Ctrl[29].Ctrl_Num = GPIO_4 ;
	state->CH_Ctrl[29].size = 1 ;
	state->CH_Ctrl[29].addr[0] = 149;
	state->CH_Ctrl[29].bit[0] = 5;
	state->CH_Ctrl[29].val[0] = 1;

	state->CH_Ctrl[30].Ctrl_Num = GPIO_3 ;
	state->CH_Ctrl[30].size = 1 ;
	state->CH_Ctrl[30].addr[0] = 149;
	state->CH_Ctrl[30].bit[0] = 4;
	state->CH_Ctrl[30].val[0] = 1;

	state->CH_Ctrl[31].Ctrl_Num = GPIO_1B ;
	state->CH_Ctrl[31].size = 1 ;
	state->CH_Ctrl[31].addr[0] = 149;
	state->CH_Ctrl[31].bit[0] = 3;
	state->CH_Ctrl[31].val[0] = 0;

	state->CH_Ctrl[32].Ctrl_Num = DAC_A_ENABLE ;
	state->CH_Ctrl[32].size = 1 ;
	state->CH_Ctrl[32].addr[0] = 93;
	state->CH_Ctrl[32].bit[0] = 1;
	state->CH_Ctrl[32].val[0] = 0;

	state->CH_Ctrl[33].Ctrl_Num = DAC_B_ENABLE ;
	state->CH_Ctrl[33].size = 1 ;
	state->CH_Ctrl[33].addr[0] = 93;
	state->CH_Ctrl[33].bit[0] = 0;
	state->CH_Ctrl[33].val[0] = 0;

	state->CH_Ctrl[34].Ctrl_Num = DAC_DIN_A ;
	state->CH_Ctrl[34].size = 6 ;
	state->CH_Ctrl[34].addr[0] = 92;
	state->CH_Ctrl[34].bit[0] = 2;
	state->CH_Ctrl[34].val[0] = 0;
	state->CH_Ctrl[34].addr[1] = 92;
	state->CH_Ctrl[34].bit[1] = 3;
	state->CH_Ctrl[34].val[1] = 0;
	state->CH_Ctrl[34].addr[2] = 92;
	state->CH_Ctrl[34].bit[2] = 4;
	state->CH_Ctrl[34].val[2] = 0;
	state->CH_Ctrl[34].addr[3] = 92;
	state->CH_Ctrl[34].bit[3] = 5;
	state->CH_Ctrl[34].val[3] = 0;
	state->CH_Ctrl[34].addr[4] = 92;
	state->CH_Ctrl[34].bit[4] = 6;
	state->CH_Ctrl[34].val[4] = 0;
	state->CH_Ctrl[34].addr[5] = 92;
	state->CH_Ctrl[34].bit[5] = 7;
	state->CH_Ctrl[34].val[5] = 0;

	state->CH_Ctrl[35].Ctrl_Num = DAC_DIN_B ;
	state->CH_Ctrl[35].size = 6 ;
	state->CH_Ctrl[35].addr[0] = 93;
	state->CH_Ctrl[35].bit[0] = 2;
	state->CH_Ctrl[35].val[0] = 0;
	state->CH_Ctrl[35].addr[1] = 93;
	state->CH_Ctrl[35].bit[1] = 3;
	state->CH_Ctrl[35].val[1] = 0;
	state->CH_Ctrl[35].addr[2] = 93;
	state->CH_Ctrl[35].bit[2] = 4;
	state->CH_Ctrl[35].val[2] = 0;
	state->CH_Ctrl[35].addr[3] = 93;
	state->CH_Ctrl[35].bit[3] = 5;
	state->CH_Ctrl[35].val[3] = 0;
	state->CH_Ctrl[35].addr[4] = 93;
	state->CH_Ctrl[35].bit[4] = 6;
	state->CH_Ctrl[35].val[4] = 0;
	state->CH_Ctrl[35].addr[5] = 93;
	state->CH_Ctrl[35].bit[5] = 7;
	state->CH_Ctrl[35].val[5] = 0;

#ifdef _MXL_PRODUCTION
	state->CH_Ctrl[36].Ctrl_Num = RFSYN_EN_DIV ;
	state->CH_Ctrl[36].size = 1 ;
	state->CH_Ctrl[36].addr[0] = 109;
	state->CH_Ctrl[36].bit[0] = 1;
	state->CH_Ctrl[36].val[0] = 1;

	state->CH_Ctrl[37].Ctrl_Num = RFSYN_DIVM ;
	state->CH_Ctrl[37].size = 2 ;
	state->CH_Ctrl[37].addr[0] = 112;
	state->CH_Ctrl[37].bit[0] = 5;
	state->CH_Ctrl[37].val[0] = 0;
	state->CH_Ctrl[37].addr[1] = 112;
	state->CH_Ctrl[37].bit[1] = 6;
	state->CH_Ctrl[37].val[1] = 0;

	state->CH_Ctrl[38].Ctrl_Num = DN_BYPASS_AGC_I2C ;
	state->CH_Ctrl[38].size = 1 ;
	state->CH_Ctrl[38].addr[0] = 65;
	state->CH_Ctrl[38].bit[0] = 1;
	state->CH_Ctrl[38].val[0] = 0;
#endif

	return 0 ;
}

static void InitTunerControls(struct dvb_frontend *fe)
{
	MXL5005_RegisterInit(fe);
	MXL5005_ControlInit(fe);
#ifdef _MXL_INTERNAL
	MXL5005_MXLControlInit(fe);
#endif
}

static u16 MXL5005_TunerConfig(struct dvb_frontend *fe,
	u8	Mode,		/* 0: Analog Mode ; 1: Digital Mode */
	u8	IF_mode,	/* for Analog Mode, 0: zero IF; 1: low IF */
	u32	Bandwidth,	/* filter  channel bandwidth (6, 7, 8) */
	u32	IF_out,		/* Desired IF Out Frequency */
	u32	Fxtal,		/* XTAL Frequency */
	u8	AGC_Mode,	/* AGC Mode - Dual AGC: 0, Single AGC: 1 */
	u16	TOP,		/* 0: Dual AGC; Value: take over point */
	u16	IF_OUT_LOAD,	/* IF Out Load Resistor (200 / 300 Ohms) */
	u8	CLOCK_OUT, 	/* 0: turn off clk out; 1: turn on clock out */
	u8	DIV_OUT,	/* 0: Div-1; 1: Div-4 */
	u8	CAPSELECT, 	/* 0: disable On-Chip pulling cap; 1: enable */
	u8	EN_RSSI, 	/* 0: disable RSSI; 1: enable RSSI */

	/* Modulation Type; */
	/* 0 - Default;	1 - DVB-T; 2 - ATSC; 3 - QAM; 4 - Analog Cable */
	u8	Mod_Type,

	/* Tracking Filter */
	/* 0 - Default; 1 - Off; 2 - Type C; 3 - Type C-H */
	u8	TF_Type
	)
{
	struct mxl5005s_state *state = fe->tuner_priv;

	state->Mode = Mode;
	state->IF_Mode = IF_mode;
	state->Chan_Bandwidth = Bandwidth;
	state->IF_OUT = IF_out;
	state->Fxtal = Fxtal;
	state->AGC_Mode = AGC_Mode;
	state->TOP = TOP;
	state->IF_OUT_LOAD = IF_OUT_LOAD;
	state->CLOCK_OUT = CLOCK_OUT;
	state->DIV_OUT = DIV_OUT;
	state->CAPSELECT = CAPSELECT;
	state->EN_RSSI = EN_RSSI;
	state->Mod_Type = Mod_Type;
	state->TF_Type = TF_Type;

	/* Initialize all the controls and registers */
	InitTunerControls(fe);

	/* Synthesizer LO frequency calculation */
	MXL_SynthIFLO_Calc(fe);

	return 0;
}

static void MXL_SynthIFLO_Calc(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	if (state->Mode == 1) /* Digital Mode */
		state->IF_LO = state->IF_OUT;
	else /* Analog Mode */ {
		if (state->IF_Mode == 0) /* Analog Zero IF mode */
			state->IF_LO = state->IF_OUT + 400000;
		else /* Analog Low IF mode */
			state->IF_LO = state->IF_OUT + state->Chan_Bandwidth/2;
	}
}

static void MXL_SynthRFTGLO_Calc(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;

	if (state->Mode == 1) /* Digital Mode */ {
			/* remove 20.48MHz setting for 2.6.10 */
			state->RF_LO = state->RF_IN;
			/* change for 2.6.6 */
			state->TG_LO = state->RF_IN - 750000;
	} else /* Analog Mode */ {
		if (state->IF_Mode == 0) /* Analog Zero IF mode */ {
			state->RF_LO = state->RF_IN - 400000;
			state->TG_LO = state->RF_IN - 1750000;
		} else /* Analog Low IF mode */ {
			state->RF_LO = state->RF_IN - state->Chan_Bandwidth/2;
			state->TG_LO = state->RF_IN -
				state->Chan_Bandwidth + 500000;
		}
	}
}

static u16 MXL_OverwriteICDefault(struct dvb_frontend *fe)
{
	u16 status = 0;

	status += MXL_ControlWrite(fe, OVERRIDE_1, 1);
	status += MXL_ControlWrite(fe, OVERRIDE_2, 1);
	status += MXL_ControlWrite(fe, OVERRIDE_3, 1);
	status += MXL_ControlWrite(fe, OVERRIDE_4, 1);

	return status;
}

static u16 MXL_BlockInit(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u16 status = 0;

	status += MXL_OverwriteICDefault(fe);

	/* Downconverter Control Dig Ana */
	status += MXL_ControlWrite(fe, DN_IQTN_AMP_CUT, state->Mode ? 1 : 0);

	/* Filter Control  Dig  Ana */
	status += MXL_ControlWrite(fe, BB_MODE, state->Mode ? 0 : 1);
	status += MXL_ControlWrite(fe, BB_BUF, state->Mode ? 3 : 2);
	status += MXL_ControlWrite(fe, BB_BUF_OA, state->Mode ? 1 : 0);
	status += MXL_ControlWrite(fe, BB_IQSWAP, state->Mode ? 0 : 1);
	status += MXL_ControlWrite(fe, BB_INITSTATE_DLPF_TUNE, 0);

	/* Initialize Low-Pass Filter */
	if (state->Mode) { /* Digital Mode */
		switch (state->Chan_Bandwidth) {
		case 8000000:
			status += MXL_ControlWrite(fe, BB_DLPF_BANDSEL, 0);
			break;
		case 7000000:
			status += MXL_ControlWrite(fe, BB_DLPF_BANDSEL, 2);
			break;
		case 6000000:
			status += MXL_ControlWrite(fe,
					BB_DLPF_BANDSEL, 3);
			break;
		}
	} else { /* Analog Mode */
		switch (state->Chan_Bandwidth) {
		case 8000000:	/* Low Zero */
			status += MXL_ControlWrite(fe, BB_ALPF_BANDSELECT,
					(state->IF_Mode ? 0 : 3));
			break;
		case 7000000:
			status += MXL_ControlWrite(fe, BB_ALPF_BANDSELECT,
					(state->IF_Mode ? 1 : 4));
			break;
		case 6000000:
			status += MXL_ControlWrite(fe, BB_ALPF_BANDSELECT,
					(state->IF_Mode ? 2 : 5));
			break;
		}
	}

	/* Charge Pump Control Dig  Ana */
	status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, state->Mode ? 5 : 8);
	status += MXL_ControlWrite(fe,
		RFSYN_EN_CHP_HIGAIN, state->Mode ? 1 : 1);
	status += MXL_ControlWrite(fe, EN_CHP_LIN_B, state->Mode ? 0 : 0);

	/* AGC TOP Control */
	if (state->AGC_Mode == 0) /* Dual AGC */ {
		status += MXL_ControlWrite(fe, AGC_IF, 15);
		status += MXL_ControlWrite(fe, AGC_RF, 15);
	} else /*  Single AGC Mode Dig  Ana */
		status += MXL_ControlWrite(fe, AGC_RF, state->Mode ? 15 : 12);

	if (state->TOP == 55) /* TOP == 5.5 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x0);

	if (state->TOP == 72) /* TOP == 7.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x1);

	if (state->TOP == 92) /* TOP == 9.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x2);

	if (state->TOP == 110) /* TOP == 11.0 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x3);

	if (state->TOP == 129) /* TOP == 12.9 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x4);

	if (state->TOP == 147) /* TOP == 14.7 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x5);

	if (state->TOP == 168) /* TOP == 16.8 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x6);

	if (state->TOP == 194) /* TOP == 19.4 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x7);

	if (state->TOP == 212) /* TOP == 21.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0x9);

	if (state->TOP == 232) /* TOP == 23.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xA);

	if (state->TOP == 252) /* TOP == 25.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xB);

	if (state->TOP == 271) /* TOP == 27.1 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xC);

	if (state->TOP == 292) /* TOP == 29.2 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xD);

	if (state->TOP == 317) /* TOP == 31.7 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xE);

	if (state->TOP == 349) /* TOP == 34.9 */
		status += MXL_ControlWrite(fe, AGC_IF, 0xF);

	/* IF Synthesizer Control */
	status += MXL_IFSynthInit(fe);

	/* IF UpConverter Control */
	if (state->IF_OUT_LOAD == 200) {
		status += MXL_ControlWrite(fe, DRV_RES_SEL, 6);
		status += MXL_ControlWrite(fe, I_DRIVER, 2);
	}
	if (state->IF_OUT_LOAD == 300) {
		status += MXL_ControlWrite(fe, DRV_RES_SEL, 4);
		status += MXL_ControlWrite(fe, I_DRIVER, 1);
	}

	/* Anti-Alias Filtering Control
	 * initialise Anti-Aliasing Filter
	 */
	if (state->Mode) { /* Digital Mode */
		if (state->IF_OUT >= 4000000UL && state->IF_OUT <= 6280000UL) {
			status += MXL_ControlWrite(fe, EN_AAF, 1);
			status += MXL_ControlWrite(fe, EN_3P, 1);
			status += MXL_ControlWrite(fe, EN_AUX_3P, 1);
			status += MXL_ControlWrite(fe, SEL_AAF_BAND, 0);
		}
		if ((state->IF_OUT == 36125000UL) ||
			(state->IF_OUT == 36150000UL)) {
			status += MXL_ControlWrite(fe, EN_AAF, 1);
			status += MXL_ControlWrite(fe, EN_3P, 1);
			status += MXL_ControlWrite(fe, EN_AUX_3P, 1);
			status += MXL_ControlWrite(fe, SEL_AAF_BAND, 1);
		}
		if (state->IF_OUT > 36150000UL) {
			status += MXL_ControlWrite(fe, EN_AAF, 0);
			status += MXL_ControlWrite(fe, EN_3P, 1);
			status += MXL_ControlWrite(fe, EN_AUX_3P, 1);
			status += MXL_ControlWrite(fe, SEL_AAF_BAND, 1);
		}
	} else { /* Analog Mode */
		if (state->IF_OUT >= 4000000UL && state->IF_OUT <= 5000000UL) {
			status += MXL_ControlWrite(fe, EN_AAF, 1);
			status += MXL_ControlWrite(fe, EN_3P, 1);
			status += MXL_ControlWrite(fe, EN_AUX_3P, 1);
			status += MXL_ControlWrite(fe, SEL_AAF_BAND, 0);
		}
		if (state->IF_OUT > 5000000UL) {
			status += MXL_ControlWrite(fe, EN_AAF, 0);
			status += MXL_ControlWrite(fe, EN_3P, 0);
			status += MXL_ControlWrite(fe, EN_AUX_3P, 0);
			status += MXL_ControlWrite(fe, SEL_AAF_BAND, 0);
		}
	}

	/* Demod Clock Out */
	if (state->CLOCK_OUT)
		status += MXL_ControlWrite(fe, SEQ_ENCLK16_CLK_OUT, 1);
	else
		status += MXL_ControlWrite(fe, SEQ_ENCLK16_CLK_OUT, 0);

	if (state->DIV_OUT == 1)
		status += MXL_ControlWrite(fe, SEQ_SEL4_16B, 1);
	if (state->DIV_OUT == 0)
		status += MXL_ControlWrite(fe, SEQ_SEL4_16B, 0);

	/* Crystal Control */
	if (state->CAPSELECT)
		status += MXL_ControlWrite(fe, XTAL_CAPSELECT, 1);
	else
		status += MXL_ControlWrite(fe, XTAL_CAPSELECT, 0);

	if (state->Fxtal >= 12000000UL && state->Fxtal <= 16000000UL)
		status += MXL_ControlWrite(fe, IF_SEL_DBL, 1);
	if (state->Fxtal > 16000000UL && state->Fxtal <= 32000000UL)
		status += MXL_ControlWrite(fe, IF_SEL_DBL, 0);

	if (state->Fxtal >= 12000000UL && state->Fxtal <= 22000000UL)
		status += MXL_ControlWrite(fe, RFSYN_R_DIV, 3);
	if (state->Fxtal > 22000000UL && state->Fxtal <= 32000000UL)
		status += MXL_ControlWrite(fe, RFSYN_R_DIV, 0);

	/* Misc Controls */
	if (state->Mode == 0 && state->IF_Mode == 1) /* Analog LowIF mode */
		status += MXL_ControlWrite(fe, SEQ_EXTIQFSMPULSE, 0);
	else
		status += MXL_ControlWrite(fe, SEQ_EXTIQFSMPULSE, 1);

	/* status += MXL_ControlRead(fe, IF_DIVVAL, &IF_DIVVAL_Val); */

	/* Set TG_R_DIV */
	status += MXL_ControlWrite(fe, TG_R_DIV,
		MXL_Ceiling(state->Fxtal, 1000000));

	/* Apply Default value to BB_INITSTATE_DLPF_TUNE */

	/* RSSI Control */
	if (state->EN_RSSI) {
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 1);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

		/* RSSI reference point */
		status += MXL_ControlWrite(fe, RFA_RSSI_REF, 2);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 3);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 1);

		/* TOP point */
		status += MXL_ControlWrite(fe, RFA_FLR, 0);
		status += MXL_ControlWrite(fe, RFA_CEIL, 12);
	}

	/* Modulation type bit settings
	 * Override the control values preset
	 */
	if (state->Mod_Type == MXL_DVBT) /* DVB-T Mode */ {
		state->AGC_Mode = 1; /* Single AGC Mode */

		/* Enable RSSI */
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 1);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

		/* RSSI reference point */
		status += MXL_ControlWrite(fe, RFA_RSSI_REF, 3);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 5);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 1);

		/* TOP point */
		status += MXL_ControlWrite(fe, RFA_FLR, 2);
		status += MXL_ControlWrite(fe, RFA_CEIL, 13);
		if (state->IF_OUT <= 6280000UL)	/* Low IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 0);
		else /* High IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 1);

	}
	if (state->Mod_Type == MXL_ATSC) /* ATSC Mode */ {
		state->AGC_Mode = 1;	/* Single AGC Mode */

		/* Enable RSSI */
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 1);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

		/* RSSI reference point */
		status += MXL_ControlWrite(fe, RFA_RSSI_REF, 2);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 4);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 1);

		/* TOP point */
		status += MXL_ControlWrite(fe, RFA_FLR, 2);
		status += MXL_ControlWrite(fe, RFA_CEIL, 13);
		status += MXL_ControlWrite(fe, BB_INITSTATE_DLPF_TUNE, 1);
		/* Low Zero */
		status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 5);

		if (state->IF_OUT <= 6280000UL)	/* Low IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 0);
		else /* High IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 1);
	}
	if (state->Mod_Type == MXL_QAM) /* QAM Mode */ {
		state->Mode = MXL_DIGITAL_MODE;

		/* state->AGC_Mode = 1; */ /* Single AGC Mode */

		/* Disable RSSI */	/* change here for v2.6.5 */
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 0);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

		/* RSSI reference point */
		status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 5);
		status += MXL_ControlWrite(fe, RFA_RSSI_REF, 3);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 2);
		/* change here for v2.6.5 */
		status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 3);

		if (state->IF_OUT <= 6280000UL)	/* Low IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 0);
		else /* High IF */
			status += MXL_ControlWrite(fe, BB_IQSWAP, 1);
		status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 2);

	}
	if (state->Mod_Type == MXL_ANALOG_CABLE) {
		/* Analog Cable Mode */
		/* state->Mode = MXL_DIGITAL_MODE; */

		state->AGC_Mode = 1; /* Single AGC Mode */

		/* Disable RSSI */
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 0);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);
		/* change for 2.6.3 */
		status += MXL_ControlWrite(fe, AGC_IF, 1);
		status += MXL_ControlWrite(fe, AGC_RF, 15);
		status += MXL_ControlWrite(fe, BB_IQSWAP, 1);
	}

	if (state->Mod_Type == MXL_ANALOG_OTA) {
		/* Analog OTA Terrestrial mode add for 2.6.7 */
		/* state->Mode = MXL_ANALOG_MODE; */

		/* Enable RSSI */
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 1);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

		/* RSSI reference point */
		status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 5);
		status += MXL_ControlWrite(fe, RFA_RSSI_REF, 3);
		status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 2);
		status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 3);
		status += MXL_ControlWrite(fe, BB_IQSWAP, 1);
	}

	/* RSSI disable */
	if (state->EN_RSSI == 0) {
		status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
		status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 0);
		status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);
	}

	return status;
}

static u16 MXL_IFSynthInit(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u16 status = 0 ;
	u32	Fref = 0 ;
	u32	Kdbl, intModVal ;
	u32	fracModVal ;
	Kdbl = 2 ;

	if (state->Fxtal >= 12000000UL && state->Fxtal <= 16000000UL)
		Kdbl = 2 ;
	if (state->Fxtal > 16000000UL && state->Fxtal <= 32000000UL)
		Kdbl = 1 ;

	/* IF Synthesizer Control */
	if (state->Mode == 0 && state->IF_Mode == 1) /* Analog Low IF mode */ {
		if (state->IF_LO == 41000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 328000000UL ;
		}
		if (state->IF_LO == 47000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 376000000UL ;
		}
		if (state->IF_LO == 54000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x10);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 324000000UL ;
		}
		if (state->IF_LO == 60000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x10);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 39250000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 314000000UL ;
		}
		if (state->IF_LO == 39650000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 317200000UL ;
		}
		if (state->IF_LO == 40150000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 321200000UL ;
		}
		if (state->IF_LO == 40650000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 325200000UL ;
		}
	}

	if (state->Mode || (state->Mode == 0 && state->IF_Mode == 0)) {
		if (state->IF_LO == 57000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x10);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 342000000UL ;
		}
		if (state->IF_LO == 44000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 352000000UL ;
		}
		if (state->IF_LO == 43750000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 350000000UL ;
		}
		if (state->IF_LO == 36650000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 366500000UL ;
		}
		if (state->IF_LO == 36150000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 361500000UL ;
		}
		if (state->IF_LO == 36000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 35250000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 352500000UL ;
		}
		if (state->IF_LO == 34750000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 347500000UL ;
		}
		if (state->IF_LO == 6280000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x07);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 376800000UL ;
		}
		if (state->IF_LO == 5000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x09);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 4500000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x06);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 4570000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x06);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 365600000UL ;
		}
		if (state->IF_LO == 4000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x05);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 57400000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x10);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 344400000UL ;
		}
		if (state->IF_LO == 44400000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 355200000UL ;
		}
		if (state->IF_LO == 44150000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x08);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 353200000UL ;
		}
		if (state->IF_LO == 37050000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 370500000UL ;
		}
		if (state->IF_LO == 36550000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 365500000UL ;
		}
		if (state->IF_LO == 36125000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x04);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 361250000UL ;
		}
		if (state->IF_LO == 6000000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x07);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 360000000UL ;
		}
		if (state->IF_LO == 5400000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x07);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 324000000UL ;
		}
		if (state->IF_LO == 5380000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x07);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x0C);
			Fref = 322800000UL ;
		}
		if (state->IF_LO == 5200000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x09);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 374400000UL ;
		}
		if (state->IF_LO == 4900000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x09);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 352800000UL ;
		}
		if (state->IF_LO == 4400000UL) {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x06);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 352000000UL ;
		}
		if (state->IF_LO == 4063000UL)  /* add for 2.6.8 */ {
			status += MXL_ControlWrite(fe, IF_DIVVAL,   0x05);
			status += MXL_ControlWrite(fe, IF_VCO_BIAS, 0x08);
			Fref = 365670000UL ;
		}
	}
	/* CHCAL_INT_MOD_IF */
	/* CHCAL_FRAC_MOD_IF */
	intModVal = Fref / (state->Fxtal * Kdbl/2);
	status += MXL_ControlWrite(fe, CHCAL_INT_MOD_IF, intModVal);

	fracModVal = (2<<15)*(Fref/1000 - (state->Fxtal/1000 * Kdbl/2) *
		intModVal);

	fracModVal = fracModVal / ((state->Fxtal * Kdbl/2)/1000);
	status += MXL_ControlWrite(fe, CHCAL_FRAC_MOD_IF, fracModVal);

	return status ;
}

static u16 MXL_TuneRF(struct dvb_frontend *fe, u32 RF_Freq)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u16 status = 0;
	u32 divider_val, E3, E4, E5, E5A;
	u32 Fmax, Fmin, FmaxBin, FminBin;
	u32 Kdbl_RF = 2;
	u32 tg_divval;
	u32 tg_lo;

	u32 Fref_TG;
	u32 Fvco;

	state->RF_IN = RF_Freq;

	MXL_SynthRFTGLO_Calc(fe);

	if (state->Fxtal >= 12000000UL && state->Fxtal <= 22000000UL)
		Kdbl_RF = 2;
	if (state->Fxtal > 22000000 && state->Fxtal <= 32000000)
		Kdbl_RF = 1;

	/* Downconverter Controls
	 * Look-Up Table Implementation for:
	 *	DN_POLY
	 *	DN_RFGAIN
	 *	DN_CAP_RFLPF
	 *	DN_EN_VHFUHFBAR
	 *	DN_GAIN_ADJUST
	 *  Change the boundary reference from RF_IN to RF_LO
	 */
	if (state->RF_LO < 40000000UL)
		return -1;

	if (state->RF_LO >= 40000000UL && state->RF_LO <= 75000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              2);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            3);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         423);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      1);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       1);
	}
	if (state->RF_LO > 75000000UL && state->RF_LO <= 100000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            3);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         222);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      1);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       1);
	}
	if (state->RF_LO > 100000000UL && state->RF_LO <= 150000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            3);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         147);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      1);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       2);
	}
	if (state->RF_LO > 150000000UL && state->RF_LO <= 200000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            3);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         9);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      1);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       2);
	}
	if (state->RF_LO > 200000000UL && state->RF_LO <= 300000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            3);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         0);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      1);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       3);
	}
	if (state->RF_LO > 300000000UL && state->RF_LO <= 650000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            1);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         0);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      0);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       3);
	}
	if (state->RF_LO > 650000000UL && state->RF_LO <= 900000000UL) {
		status += MXL_ControlWrite(fe, DN_POLY,              3);
		status += MXL_ControlWrite(fe, DN_RFGAIN,            2);
		status += MXL_ControlWrite(fe, DN_CAP_RFLPF,         0);
		status += MXL_ControlWrite(fe, DN_EN_VHFUHFBAR,      0);
		status += MXL_ControlWrite(fe, DN_GAIN_ADJUST,       3);
	}
	if (state->RF_LO > 900000000UL)
		return -1;

	/*	DN_IQTNBUF_AMP */
	/*	DN_IQTNGNBFBIAS_BST */
	if (state->RF_LO >= 40000000UL && state->RF_LO <= 75000000UL) {
		status += MXL_ControlWrite(fe, DN_IQTNBUF_AMP,       1);
		status += MXL_ControlWrite(fe, DN_IQTNGNBFBIAS_BST,  0);
	}
	if (state->RF_LO > 75000000UL && state->RF_LO <= 100000000UL) {
		status += MXL_ControlWrite(fe, DN_IQTNBUF_AMP,       1);
		status += MXL_ControlWrite(fe, DN_IQTNGNBFBIAS_BST,  0);
	}
	if (state->RF_LO > 100000000UL && state->RF_LO <= 150000000UL) {
		status += MXL_ControlWrite(fe, DN_IQTNBUF_AMP,       1);
		status += MXL_ControlWrite(fe, DN_IQTNGNBFBIAS_BST,  0);
	}
	if (state->RF_LO > 150000000UL && state->RF_LO <= 200000000UL) {
		status += MXL_ControlWrite(fe, DN_IQTNBUF_AMP,       1);
		status += MXL_ControlWrite(fe, DN_IQTNGNBFBIAS_BST,  0);
	}
	if (state->RF_LO > 200000000UL