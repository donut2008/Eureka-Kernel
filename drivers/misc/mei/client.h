fe, DN_IQTNBUF_AMP,       10);
		status += MXL_ControlWrite(fe, DN_IQTNGNBFBIAS_BST,  1);
	}

	/*
	 * Set RF Synth and LO Path Control
	 *
	 * Look-Up table implementation for:
	 *	RFSYN_EN_OUTMUX
	 *	RFSYN_SEL_VCO_OUT
	 *	RFSYN_SEL_VCO_HI
	 *  RFSYN_SEL_DIVM
	 *	RFSYN_RF_DIV_BIAS
	 *	DN_SEL_FREQ
	 *
	 * Set divider_val, Fmax, Fmix to use in Equations
	 */
	FminBin = 28000000UL ;
	FmaxBin = 42500000UL ;
	if (state->RF_LO >= 40000000UL && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         1);
		divider_val = 64 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 42500000UL ;
	FmaxBin = 56000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         1);
		divider_val = 64 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 56000000UL ;
	FmaxBin = 85000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         1);
		divider_val = 32 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 85000000UL ;
	FmaxBin = 112000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         1);
		divider_val = 32 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 112000000UL ;
	FmaxBin = 170000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         2);
		divider_val = 16 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 170000000UL ;
	FmaxBin = 225000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         2);
		divider_val = 16 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 225000000UL ;
	FmaxBin = 300000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         4);
		divider_val = 8 ;
		Fmax = 340000000UL ;
		Fmin = FminBin ;
	}
	FminBin = 300000000UL ;
	FmaxBin = 340000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         0);
		divider_val = 8 ;
		Fmax = FmaxBin ;
		Fmin = 225000000UL ;
	}
	FminBin = 340000000UL ;
	FmaxBin = 450000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      0);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   2);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         0);
		divider_val = 8 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 450000000UL ;
	FmaxBin = 680000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      1);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         0);
		divider_val = 4 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}
	FminBin = 680000000UL ;
	FmaxBin = 900000000UL ;
	if (state->RF_LO > FminBin && state->RF_LO <= FmaxBin) {
		status += MXL_ControlWrite(fe, RFSYN_EN_OUTMUX,     0);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_OUT,   1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_VCO_HI,    1);
		status += MXL_ControlWrite(fe, RFSYN_SEL_DIVM,      1);
		status += MXL_ControlWrite(fe, RFSYN_RF_DIV_BIAS,   1);
		status += MXL_ControlWrite(fe, DN_SEL_FREQ,         0);
		divider_val = 4 ;
		Fmax = FmaxBin ;
		Fmin = FminBin ;
	}

	/*	CHCAL_INT_MOD_RF
	 *	CHCAL_FRAC_MOD_RF
	 *	RFSYN_LPF_R
	 *	CHCAL_EN_INT_RF
	 */
	/* Equation E3 RFSYN_VCO_BIAS */
	E3 = (((Fmax-state->RF_LO)/1000)*32)/((Fmax-Fmin)/1000) + 8 ;
	status += MXL_ControlWrite(fe, RFSYN_VCO_BIAS, E3);

	/* Equation E4 CHCAL_INT_MOD_RF */
	E4 = (state->RF_LO*divider_val/1000)/(2*state->Fxtal*Kdbl_RF/1000);
	MXL_ControlWrite(fe, CHCAL_INT_MOD_RF, E4);

	/* Equation E5 CHCAL_FRAC_MOD_RF CHCAL_EN_INT_RF */
	E5 = ((2<<17)*(state->RF_LO/10000*divider_val -
		(E4*(2*state->Fxtal*Kdbl_RF)/10000))) /
		(2*state->Fxtal*Kdbl_RF/10000);

	status += MXL_ControlWrite(fe, CHCAL_FRAC_MOD_RF, E5);

	/* Equation E5A RFSYN_LPF_R */
	E5A = (((Fmax - state->RF_LO)/1000)*4/((Fmax-Fmin)/1000)) + 1 ;
	status += MXL_ControlWrite(fe, RFSYN_LPF_R, E5A);

	/* Euqation E5B CHCAL_EN_INIT_RF */
	status += MXL_ControlWrite(fe, CHCAL_EN_INT_RF, ((E5 == 0) ? 1 : 0));
	/*if (E5 == 0)
	 *	status += MXL_ControlWrite(fe, CHCAL_EN_INT_RF, 1);
	 *else
	 *	status += MXL_ControlWrite(fe, CHCAL_FRAC_MOD_RF, E5);
	 */

	/*
	 * Set TG Synth
	 *
	 * Look-Up table implementation for:
	 *	TG_LO_DIVVAL
	 *	TG_LO_SELVAL
	 *
	 * Set divider_val, Fmax, Fmix to use in Equations
	 */
	if 