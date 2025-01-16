tus += MXL_SetGPIO(fe, 3, 1);
			}
			if (state->RF_IN >= 620000000 &&
				state->RF_IN < 760000000) {
				status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
				status += MXL_SetGPIO(fe, 4, 0);
				status += MXL_SetGPIO(fe, 1, 1);
				status += MXL_SetGPIO(fe, 3, 1);
			}
			if (state->RF_IN >= 760000000 &&
				state->RF_IN <= 900000000) {
				status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
				status += MXL_SetGPIO(fe, 4, 1);
				status += MXL_SetGPIO(fe, 1, 1);
				status += MXL_SetGPIO(fe, 3, 1);
			}
		}
	}

	if (state->TF_Type == MXL_TF_E) /* Tracking Filter type E */ {

		status += MXL_ControlWrite(fe, DAC_DIN_B, 0);

		if (state->RF_IN >= 43000000 && state->RF_IN < 174000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 174000000 && state->RF_IN < 250000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 250000000 && state->RF_IN < 310000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 310000000 && state->RF_IN < 360000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 360000000 && state->RF_IN < 470000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 470000000 && state->RF_IN < 640000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 640000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
	}

	if (state->TF_Type == MXL_TF_F) {

		/* Tracking Filter type F */
		status += MXL_ControlWrite(fe, DAC_DIN_B, 0);

		if (state->RF_IN >= 43000000 && state->RF_IN < 160000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 160000000 && state->RF_IN < 210000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 210000000 && state->RF_IN < 300000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 300000000 && state->RF_IN < 390000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 390000000 && state->RF_IN < 515000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 515000000 && state->RF_IN < 650000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 650000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
	}

	if (state->TF_Type == MXL_TF_E_2) {

		/* Tracking Filter type E_2 */
		status += MXL_ControlWrite(fe, DAC_DIN_B, 0);

		if (state->RF_IN >= 43000000 && state->RF_IN < 174000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 174000000 && state->RF_IN < 250000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 250000000 && state->RF_IN < 350000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 350000000 && state->RF_IN < 400000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 400000000 && state->RF_IN < 570000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 570000000 && state->RF_IN < 770000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 770000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
	}

	if (state->TF_Type == MXL_TF_G) {

		/* Tracking Filter type G add for v2.6.8 */
		status += MXL_ControlWrite(fe, DAC_DIN_B, 0);

		if (state->RF_IN >= 50000000 && state->RF_IN < 190000000) {

			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 190000000 && state->RF_IN < 280000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 280000000 && state->RF_IN < 350000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 350000000 && state->RF_IN < 400000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 400000000 && state->RF_IN < 470000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 470000000 && state->RF_IN < 640000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 640000000 && state->RF_IN < 820000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 820000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
	}

	if (state->TF_Type == MXL_TF_E_NA) {

		/* Tracking Filter type E-NA for Empia ONLY change for 2.6.8 */
		status += MXL_ControlWrite(fe, DAC_DIN_B, 0);

		/* if UHF and terrestrial=> Turn off Tracking Filter */
		if (state->RF_IN >= 471000000 &&
			(state->RF_IN - 471000000)%6000000 != 0) {

			/* Turn off all the banks */
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);

			/* 2.6.12 Turn on RSSI */
			status += MXL_ControlWrite(fe, SEQ_EXTSYNTHCALIF, 1);
			status += MXL_ControlWrite(fe, SEQ_EXTDCCAL, 1);
			status += MXL_ControlWrite(fe, AGC_EN_RSSI, 1);
			status += MXL_ControlWrite(fe, RFA_ENCLKRFAGC, 1);

			/* RSSI reference point */
			status += MXL_ControlWrite(fe, RFA_RSSI_REFH, 5);
			status += MXL_ControlWrite(fe, RFA_RSSI_REF, 3);
			status += MXL_ControlWrite(fe, RFA_RSSI_REFL, 2);

			/* following parameter is from analog OTA mode,
			 * can be change to seek better performance */
			status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 3);
		} else {
		/* if VHF or Cable =>  Turn on Tracking Filter */

		/* 2.6.12 Turn off RSSI */
		status += MXL_ControlWrite(fe, AGC_EN_RSSI, 0);

		/* change back from above condition */
		status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 5);


		if (state->RF_IN >= 43000000 && state->RF_IN < 174000000) {

			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 174000000 && state->RF_IN < 250000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 0);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 250000000 && state->RF_IN < 350000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		if (state->RF_IN >= 350000000 && state->RF_IN < 400000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 400000000 && state->RF_IN < 570000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 570000000 && state->RF_IN < 770000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 0);
		}
		if (state->RF_IN >= 770000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
			status += MXL_SetGPIO(fe, 4, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 3, 1);
		}
		}
	}
	return status ;
}

static u16 MXL_SetGPIO(struct dvb_frontend *fe, u8 GPIO_Num, u8 GPIO_Val)
{
	u16 status = 0;

	if (GPIO_Num == 1)
		status += MXL_ControlWrite(fe, GPIO_1B, GPIO_Val ? 0 : 1);

	/* GPIO2 is not available */

	if (GPIO_Num == 3) {
		if (GPIO_Val == 1) {
			status += MXL_ControlWrite(fe, GPIO_3, 0);
			status += MXL_ControlWrite(fe, GPIO_3B, 0);
		}
		if (GPIO_Val == 0) {
			status += MXL_ControlWrite(fe, GPIO_3, 1);
			status += MXL_ControlWrite(fe, GPIO_3B, 1);
		}
		if (GPIO_Val == 3) { /* tri-state */
			status += MXL_ControlWrite(fe, GPIO_3, 0);
			status += MXL_ControlWrite(fe, GPIO_3B, 1);
		}
	}
	if (GPIO_Num == 4) {
		if (GPIO_Val == 1) {
			status += MXL_ControlWrite(fe, GPIO_4, 0);
			status += MXL_ControlWrite(fe, GPIO_4B, 0);
		}
		if (GPIO_Val == 0) {
			status += MXL_ControlWrite(fe, GPIO_4, 1);
			status += MXL_ControlWrite(fe, GPIO_4B, 1);
		}
		if (GPIO_Val == 3) { /* tri-state */
			status += MXL_ControlWrite(fe, GPIO_4, 0);
			status += MXL_ControlWrite(fe, GPIO_4B, 1);
		}
	}

	return status;
}

static u16 MXL_ControlWrite(struct dvb_frontend *fe, u16 ControlNum, u32 value)
{
	u16 status = 0;

	/* Will write ALL Matching Control Name */
	/* Write Matching INIT Control */
	status += MXL_ControlWrite_Group(fe, ControlNum, value, 1);
	/* Write Matching CH Control */
	status += MXL_ControlWrite_Group(fe, ControlNum, value, 2);
#ifdef _MXL_INTERNAL
	/* Write Matching MXL Control */
	status += MXL_ControlWrite_Group(fe, ControlNum, value, 3);
#endif
	return status;
}

static u16 MXL_ControlWrite_Group(struct dvb_frontend *fe, u16 controlNum,
	u32 value, u16 controlGroup)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u16 i, j, k;
	u32 highLimit;
	u32 ctrlVal;

	if (controlGroup == 1) /* Initial Control */ {

		for (i = 0; i < state->Init_Ctrl_Num; i++) {

			if (controlNum == state->Init_Ctrl[i].Ctrl_Num) {

				highLimit = 1 << state->Init_Ctrl[i].size;
				if (value < highLimit) {
					for (j = 0; j < state->Init_Ctrl[i].size; j++) {
						state->Init_Ctrl[i].val[j] = (u8)((value >> j) & 0x01);
						MXL_RegWriteBit(fe, (u8)(state->Init_Ctrl[i].addr[j]),
							(u8)(state->Init_Ctrl[i].bit[j]),
							(u8)((value>>j) & 0x01));
					}
					ctrlVal = 0;
					for (k = 0; k < state->Init_Ctrl[i].size; k++)
						ctrlVal += state->Init_Ctrl[i].val[k] * (1 << k);
				} else
					return -1;
			}
		}
	}
	if (controlGroup == 2) /* Chan change Control */ {

		for (i = 0; i < state->CH_Ctrl_Num; i++) {

			if (controlNum == state->CH_Ctrl[i].Ctrl_Num) {

				highLimit = 1 << state->CH_Ctrl[i].size;
				if (value < highLimit) {
					for (j = 0; j < state->CH_Ctrl[i].size; j++) {
						state->CH_Ctrl[i].val[j] = (u8)((value >> j) & 0x01);
						MXL_RegWriteBit(fe, (u8)(state->CH_Ctrl[i].addr[j]),
							(u8)(state->CH_Ctrl[i].bit[j]),
							(u8)((value>>j) & 0x01));
					}
					ctrlVal = 0;
					for (k = 0; k < state->CH_Ctrl[i].size; k++)
						ctrlVal += state->CH_Ctrl[i].val[k] * (1 << k);
				} else
					return -1;
			}
		}
	}
#ifdef _MXL_INTERNAL
	if (controlGroup == 3) /* Maxlinear Control */ {

		for (i = 0; i < state->MXL_Ctrl_Num; i++) {

			if (controlNum == state->MXL_Ctrl[i].Ctrl_Num) {

				highLimit = (1 << state->MXL_Ctrl[i].size);
				if (value < highLimit) {
					for (j = 0; j < state->MXL_Ctrl[i].size; j++) {
						state->MXL_Ctrl[i].val[j] = (u8)((value >> j) & 0x01);
						MXL_RegWriteBit(fe, (u8)(state->MXL_Ctrl[i].addr[j]),
							(u8)(state->MXL_Ctrl[i].bit[j]),
							(u8)((value>>j) & 0x01));
					}
					ctrlVal = 0;
					for (k = 0; k < state->MXL_Ctrl[i].size; k++)
						ctrlVal += state->
							MXL_Ctrl[i].val[k] *
							(1 << k);
				} else
					return -1;
			}
		}
	}
#endif
	return 0 ; /* successful return */
}

static u16 MXL_RegRead(struct dvb_frontend *fe, u8 RegNum, u8 *RegVal)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	int i ;

	for (i = 0; i < 104; i++) {
		if (RegNum == state->TunerRegs[i].Reg_Num) {
			*RegVal = (u8)(state->TunerRegs[i].Reg_Val);
			return 0;
		}
	}

	return 1;
}

static u16 MXL_ControlRead(struct dvb_frontend *fe, u16 controlNum, u32 *value)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u32 ctrlVal ;
	u16 i, k ;

	for (i = 0; i < state->Init_Ctrl_Num ; i++) {

		if (controlNum == state->Init_Ctrl[i].Ctrl_Num) {

			ctrlVal = 0;
			for (k = 0; k < state->Init_Ctrl[i].size; k++)
				ctrlVal += state->Init_Ctrl[i].val[k] * (1<<k);
			*value = ctrlVal;
			return 0;
		}
	}

	for (i = 0; i < state->CH_Ctrl_Num ; i++) {

		if (controlNum == state->CH_Ctrl[i].Ctrl_Num) {

			ctrlVal = 0;
			for (k = 0; k < state->CH_Ctrl[i].size; k++)
				ctrlVal += state->CH_Ctrl[i].val[k] * (1 << k);
			*value = ctrlVal;
			return 0;

		}
	}

#ifdef _MXL_INTERNAL
	for (i = 0; i < state->MXL_Ctrl_Num ; i++) {

		if (controlNum == state->MXL_Ctrl[i].Ctrl_Num) {

			ctrlVal = 0;
			for (k = 0; k < state->MXL_Ctrl[i].size; k++)
				ctrlVal += state->MXL_Ctrl[i].val[k] * (1<<k);
			*value = ctrlVal;
			return 0;

		}
	}
#endif
	return 1;
}

static void MXL_RegWriteBit(struct dvb_frontend *fe, u8 address, u8 bit,
	u8 bitVal)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	int i ;

	const u8 AND_MAP[8] = {
		0xFE, 0xFD, 0xFB, 0xF7,
		0xEF, 0xDF, 0xBF, 0x7F } ;

	const u8 OR_MAP[8] = {
		0x01, 0x02, 0x04, 0x08,
		0x10, 0x20, 0x40, 0x80 } ;

	for (i = 0; i < state->TunerRegs_Num; i++) {
		if (state->TunerRegs[i].Reg_Num == address) {
			if (bitVal)
				state->TunerRegs[i].Reg_Val |= OR_MAP[bit];
			else
				state->TunerRegs[i].Reg_Val &= AND_MAP[bit];
			break ;
		}
	}
}

static u32 MXL_Ceiling(u32 value, u32 resolution)
{
	return value / resolution + (value % resolution > 0 ? 1 : 0);
}

/* Retrieve the Initialzation Registers */
static u16 MXL_GetInitRegister(struct dvb_frontend *fe, u8 *RegNum,
	u8 *RegVal, int *count)
{
	u16 status = 0;
	int i ;

	u8 RegAddr[] = {
		11, 12, 13, 22, 32, 43, 44, 53, 56, 59, 73,
		76, 77, 91, 134, 135, 137, 147,
		156, 166, 167, 168, 25 };

	*count = ARRAY_SIZE(RegAddr);

	status += MXL_BlockInit(fe);

	for (i = 0 ; i < *count; i++) {
		RegNum[i] = RegAddr[i];
		status += MXL_RegRead(fe, RegNum[i], &RegVal[i]);
	}

	return status;
}

static u16 MXL_GetCHRegister(struct dvb_frontend *fe, u8 *RegNum, u8 *RegVal,
	int *count)
{
	u16 status = 0;
	int i ;

/* add 77, 166, 167, 168 register for 2.6.12 */
#ifdef _MXL_PRODUCTION
	u8 RegAddr[] = {14, 15, 16, 17, 22, 43, 65, 68, 69, 70, 73, 92, 93, 106,
	   107, 108, 109, 110, 111, 112, 136, 138, 149, 77, 166, 167, 168 } ;
#else
	u8 RegAddr[] = {14, 15, 16, 17, 22, 43, 68, 69, 70, 73, 92, 93, 106,
	   107, 108, 109, 110, 111, 112, 136, 138, 149, 77, 166, 167, 168 } ;
	/*
	u8 RegAddr[171];
	for (i = 0; i <= 170; i++)
		RegAddr[i] = i;
	*/
#endif

	*count = ARRAY_SIZE(RegAddr);

	for (i = 0 ; i < *count; i++) {
		RegNum[i] = RegAddr[i];
		status += MXL_RegRead(fe, RegNum[i], &RegVal[i]);
	}

	return status;
}

static u16 MXL_GetCHRegister_ZeroIF(struct dvb_frontend *fe, u8 *RegNum,
	u8 *RegVal, int *count)
{
	u16 status = 0;
	int i;

	u8 RegAddr[] = {43, 136};

	*count = ARRAY_SIZE(RegAddr);

	for (i = 0; i < *count; i++) {
		RegNum[i] = RegAddr[i];
		status += MXL_RegRead(fe, RegNum[i], &RegVal[i]);
	}

	return status;
}

static u16 MXL_GetMasterControl(u8 *MasterReg, int state)
{
	if (state == 1) /* Load_Start */
		*MasterReg = 0xF3;
	if (state == 2) /* Power_Down */
		*MasterReg = 0x41;
	if (state == 3) /* Synth_Reset */
		*MasterReg = 0xB1;
	if (state == 4) /* Seq_Off */
		*MasterReg = 0xF1;

	return 0;
}

#ifdef _MXL_PRODUCTION
static u16 MXL_VCORange_Test(struct dvb_frontend *fe, int VCO_Range)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u16 status = 0 ;

	if (VCO_Range == 1) {
		status += MXL_ControlWrite(fe, RFSYN_EN_DIV, 1);
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX, 0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM, 0);
		status += MXL_ControlWrite(fe, RFSYN_DIVM, 1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT, 1);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS, 1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ, 0);
		if (state->Mode == 0 && state->IF_Mode == 1) {
			/* Analog Low IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 56);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 180224);
		}
		if (state->Mode == 0 && state->IF_Mode == 0) {
			/* Analog Zero IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 56);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 222822);
		}
		if (state->Mode == 1) /* Digital Mode */ {
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 56);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 229376);
		}
	}

	if (VCO_Range == 2) {
		status += MXL_ControlWrite(fe, RFSYN_EN_DIV, 1);
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX, 0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM, 0);
		status += MXL_ControlWrite(fe, RFSYN_DIVM, 1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT, 1);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS, 1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ, 0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
		status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 40);
		status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 41);
		if (state->Mode == 0 && state->IF_Mode == 1) {
			/* Analog Low IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 40);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 42);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 206438);
		}
		if (state->Mode == 0 && state->IF_Mode == 0) {
			/* Analog Zero IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 40);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 42);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 206438);
		}
		if (state->Mode == 1) /* Digital Mode */ {
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 1);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 40);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 41);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 16384);
		}
	}

	if (VCO_Range == 3) {
		status += MXL_ControlWrite(fe, RFSYN_EN_DIV, 1);
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX, 0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM, 0);
		status += MXL_ControlWrite(fe, RFSYN_DIVM, 1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT, 1);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS, 1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ, 0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 0);
		status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
		status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 42);
		if (state->Mode == 0 && state->IF_Mode == 1) {
			/* Analog Low IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 0);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
			status += MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, 44);
			status += MXL_ControlWrite(fe,
				CHCAL_FRAC_MOD_RF, 173670);
		}
		if (state->Mode == 0 && state->IF_Mode == 0) {
			/* Analog Zero IF Mode */
			status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI, 0);
			status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, 8);
			status += MXL_ControlWrite(f