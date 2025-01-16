tate->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x8);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x2);
		divider_val = 12 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 150000000UL ;
	FmaxBin = 200000000UL ;
	if (state->TG_LO > FminBin && state->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x0);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x2);
		divider_val = 8 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 200000000UL ;
	FmaxBin = 300000000UL ;
	if (state->TG_LO > FminBin && state->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x8);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x3);
		divider_val = 6 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 300000000UL ;
	FmaxBin = 400000000UL ;
	if (state->TG_LO > FminBin && state->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x0);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x3);
		divider_val = 4 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 400000000UL ;
	FmaxBin = 600000000UL ;
	if (state->TG_LO > FminBin && state->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x8);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x7);
		divider_val = 3 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 600000000UL ;
	FmaxBin = 900000000UL ;
	if (state->TG_LO > FminBin && state->TG_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, TG_LO_DIVVAL,	0x0);
		status += MXL_ControlWrite(fe, TG_LO_SELVAL,	0x7);
		divider_val = 2 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}

	/* TG_DIV_VAL */
	tg_divval = (state->TG_LO*divider_val/100000) *
		(MXL_Ceiling(state->Fxtal, 1000000) * 100) /
		(state->Fxtal/1000);

	status += MXL_ControlWrite(fe, TG_DIV_VAL, tg_divval);

	if (state->TG_LO > 600000000UL)
		status += MXL_ControlWrite(fe, TG_DIV_VAL, tg_divval + 1);

	Fmax = 1800000000UL ;
	Fmin = 1200000000UL ;

	/* prevent overflow of 32 bit unsigned integer, use
	 * following equation. Edit for v2.6.4
	 */
	/* Fref_TF = Fref_TG * 1000 */
	Fref_TG = (state->Fxtal/1000) / MXL_Ceiling(state->Fxtal, 1000000);

	/* Fvco = Fvco/10 */
	Fvco = (state->TG_LO/10000) * divider_val * Fref_TG;

	tg_lo = (((Fmax/10 - Fvco)/100)*32) / ((Fmax-Fmin)/1000)+8;

	/* below equation is same as above but much harder to debug.
	 *
	 * static u32 MXL_GetXtalInt(u32 Xtal_Freq)
	 * {
	 *	if ((Xtal_Freq % 1000000) == 0)
	 *		return (Xtal_Freq / 10000);
	 *	else
	 *		return (((Xtal_Freq / 1000000) + 1)*100);
	 * }
	 *
	 * u32 Xtal_Int = MXL_GetXtalInt(state->Fxtal);
	 * tg_lo = ( ((Fmax/10000 * Xtal_Int)/100) -
	 * ((state->TG_LO/10000)*divider_val *
	 * (state->Fxtal/10000)/100) )*32/((Fmax-Fmin)/10000 *
	 * Xtal_Int/100) + 8;
	 */

	status += MXL_ControlWrite(fe, TG_VCO_BIAS , tg_lo);

	/* add for 2.6.5 Special setting for QAM */
	if (state->Mod_Type == MXL_QAM) {
		if (state->config->qam_gain != 0)
			status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN,
						   state->config->qam_gain);
		else if (state->RF_IN < 680000000)
			status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 3);
		else
			status += MXL_ControlWrite(fe, RFSYN_CHP_GAIN, 2);
	}

	/* Off Chip Tracking Filter Control */
	if (state->TF_Type == MXL_TF_OFF) {
		/* Tracking Filter Off State; turn off all the banks */
		status += MXL_ControlWrite(fe, DAC_A_ENABLE, 0);
		status += MXL_ControlWrite(fe, DAC_B_ENABLE, 0);
		status += MXL_SetGPIO(fe, 3, 1); /* Bank1 Off */
		status += MXL_SetGPIO(fe, 1, 1); /* Bank2 Off */
		status += MXL_SetGPIO(fe, 4, 1); /* Bank3 Off */
	}

	if (state->TF_Type == MXL_TF_C) /* Tracking Filter type C */ {
		status += MXL_ControlWrite(fe, DAC_B_ENABLE, 1);
		status += MXL_ControlWrite(fe, DAC_DIN_A, 0);

		if (state->RF_IN >= 43000000 && state->RF_IN < 150000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 0);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 0);
			status += MXL_SetGPIO(fe, 3, 0);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 1);
		}
		if (state->RF_IN >= 150000000 && state->RF_IN < 280000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 0);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 0);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 4, 1);
		}
		if (state->RF_IN >= 280000000 && state->RF_IN < 360000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 0);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 0);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 0);
			status += MXL_SetGPIO(fe, 4, 0);
		}
		if (state->RF_IN >= 360000000 && state->RF_IN < 560000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 0);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 0);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 0);
		}
		if (state->RF_IN >= 560000000 && state->RF_IN < 580000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 29);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 0);
		}
		if (state->RF_IN >= 580000000 && state->RF_IN < 630000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 0);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 0);
		}
		if (state->RF_IN >= 630000000 && state->RF_IN < 700000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 16);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 1);
		}
		if (state->RF_IN >= 700000000 && state->RF_IN < 760000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
			status += MXL_ControlWrite(fe, DAC_DIN_B, 7);
			status += MXL_SetGPIO(fe, 3, 1);
			status += MXL_SetGPIO(fe, 1, 1);
			status += MXL_SetGPIO(fe, 4, 1);
		}
		if (state->RF_IN >= 760000000 && state->RF_IN <= 900000000) {
			status += MXL_ControlWrite(fe, DAC_A_ENABLE, 1);
			status += MXL_ControlWrite(f