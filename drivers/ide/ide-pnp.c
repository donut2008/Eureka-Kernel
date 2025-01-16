 : 0,
		((*resize_coeff & 0x1FFF) * 10000L) / 8192L, *resize_coeff);

	return 0;
}

static enum ipu_color_space format_to_colorspace(enum pixel_fmt fmt)
{
	switch (fmt) {
	case IPU_PIX_FMT_RGB565:
	case IPU_PIX_FMT_BGR24:
	case IPU_PIX_FMT_RGB24:
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_RGB32:
		return IPU_COLORSPACE_RGB;
	default:
		return IPU_COLORSPACE_YCBCR;
	}
}

static int ipu_ic_init_prpenc(struct ipu *ipu,
			      union ipu_channel_param *params, bool src_is_csi)
{
	uint32_t reg, ic_conf;
	uint32_t downsize_coeff, resize_coeff;
	enum ipu_color_space in_fmt, out_fmt;

	/* Setup vertical resizing */
	calc_resize_coeffs(params->video.in_height,
			    params->video.out_height,
			    &resize_coeff, &downsize_coeff);
	reg = (downsize_coeff << 30) | (resize_coeff << 16);

	/* Setup horizontal resizing */
	calc_resize_coeffs(params->video.in_width,
			    params->video.out_width,
			    &resize_coeff, &downsize_coeff);
	reg |= (downsize_coeff << 14) | resize_coeff;

	/* Setup color space conversion */
	in_fmt = format_to_colorspace(params->video.in_pixel_fmt);
	out_fmt = format_to_colorspace(params->video.out_pixel_fmt);

	/*
	 * Colourspace conversion unsupported yet - see _init_csc() in
	 * Freescale sources
	 */
	if (in_fmt != out_fmt) {
		dev_err(ipu->dev, "Colourspace conversion unsupported!\n");
		return -EOPNOTSUPP;
	}

	idmac_write_icreg(ipu, reg, IC_PRP_ENC_RSC);

	ic_conf = idmac_read_icreg(ipu, IC_CONF);

	if (src_is_csi)
		ic_conf &= ~IC_CONF_RWS_EN;
	else
		ic_conf |= IC_CONF_RWS_EN;

	idmac_write_icreg(ipu, ic_conf, IC_CONF);

	return 0;
}

static uint32_t dma_param_addr(uint32_t dma_ch)
{
	/* Channel Parameter Memory */
	return 0x10000 | (dma_ch << 4);
}

static void ipu_channel_set_priority(struct ipu *ipu, enum ipu_channel channel,
				     bool prio)
{
	u32 reg = idmac_read_icreg(ipu, IDMAC_CHA_PRI);

	if (prio)
		reg |= 1UL << channel;
	else
		reg &= ~(1UL << channel);

	idmac_write_icreg(ipu, reg, IDMAC_CHA_PRI);

	dump_idmac_reg(ipu);
}

static uint32_t ipu_channel_conf_mask(enum ipu_channel channel)
{
	uint32_t mask;

	switch (channel) {
	case IDMAC_IC_0:
	case IDMAC_IC_7:
		mask = IPU_CONF_CSI_EN | IPU_CONF_IC_EN;
		break;
	case IDMAC_SDC_0:
	case IDMAC_SDC_1:
		mask = IPU_CONF_SDC_EN | IPU_CONF_DI_EN;
		break;
	default:
		mask = 0;
		break;
	}

	return mask;
}

/**
 * ipu_enable_channel() - enable an IPU channel.
 * @idmac:	IPU DMAC context.
 * @ichan:	IDMAC channel.
 * @return:	0 on success or negative error code on failure.
 */
static int ipu