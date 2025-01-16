riv;
	int ret = 0;

	u8 buf[2] = { 0xff, 0x00 };
	struct i2c_msg msg = { .addr = state->config->i2c_address, .flags = 0,
			       .buf = buf, .len = 2 };

	dprintk(2, "%s()\n", __func__);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	if (i2c_transfer(state->i2c, &msg, 1) != 1) {
		printk(KERN_WARNING "mxl5005s I2C reset failed\n");
		ret = -EREMOTEIO;
	}

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return ret;
}

/* Write a single byte to a single reg, latch the value if required by
 * following the transaction with the latch byte.
 */
static int mxl5005s_writereg(struct dvb_frontend *fe, u8 reg, u8 val, int latch)
{
	struct mxl5005s_state *state = fe->tuner_priv;
	u8 buf[3] = { reg, val, MXL5005S_LATCH_BYTE };
	struct i2c_msg msg = { .addr = state->config->i2c_address, .flags = 0,
			       .buf = buf, .len = 3 };

	if (latch == 0)
		msg.len = 2;

	dprintk(2, "%s(0x%x, 0x%x, 0x%x)\n", __func__, reg, val, msg.addr);

	if (i2c_transfer(state->i2c, &msg, 1) != 1) {
		printk(KERN_WARNING "mxl5005s I2C write failed\n");
		return -EREMOTEIO;
	}
	return 0;
}

static int mxl5005s_writeregs(struct dvb_frontend *fe, u8 *addrtable,
	u8 *datatable, u8 len)
{
	int ret = 0, i;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	for (i = 0 ; i < len-1; i++) {
		ret = mxl5005s_writereg(fe, addrtable[i], datatable[i], 0);
		if (ret < 0)
			break;
	}

	ret = mxl5005s_writereg(fe, addrtable[i], datatable[i], 1);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return ret;
}

static int mxl5005s_init(struct dvb_frontend *fe)
{
	struct mxl5005s_state *state = fe->tuner_priv;

	dprintk(1, "%s()\n", __func__);
	state->current_mode = MXL_QAM;
	return mxl5005s_reconfigure(fe, MXL_QAM, MXL5005S_BANDWIDTH_6MHZ);
}

static int mxl5005s_reconfigure(struct dvb_frontend *fe, u32 mod_type,
	u32 bandwidth)
{
	struct mxl5005s_state *state = fe->tuner_priv;

	u8 AddrTable[MXL5005S_REG_WRITING_TABLE_LEN_MAX];
	u8 ByteTable[MXL5005S_REG_WRITING_TABLE_LEN_MAX];
	int TableLen;

	dprintk(1, "%s(type=%d, bw=%d)\n", __func__, mod_type, bandwidth);

	mxl5005s_reset(fe);

	/* Tuner initialization stage 0 */
	MXL_GetMasterControl(ByteTable, MC_SYNTH_RESET);
	AddrTable[0] = MASTER_CONTROL_ADDR;
	ByteTable[0] |= state->config