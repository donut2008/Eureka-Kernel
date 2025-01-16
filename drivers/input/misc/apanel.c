MD_RSP_SENT
		    && state != SRPT_STATE_MGMT_RSP_SENT
		    && state != SRPT_STATE_DONE))
		pr_debug("state = %d\n", state);

	if (state != SRPT_STATE_DONE) {
		srpt_unmap_sg_to_ib_sge(ch, ioctx);
		transport_generic_free_cmd(&ioctx->cmd, 0);
	} else {
		pr_err("IB completion has been received too late for"
		       " wr_id = %u.\n", ioctx->ioctx.index);
	}
}

/**
 * srpt_handle_rdma_comp() - Process an IB RDMA completion notification.
 *
 * XXX: what is now target_execute_cmd used to be asynchronous, and unmapping
 * the data that has been transferred via IB RDMA had to be postponed until the
 * check_stop_free() callback.  None of this is necessary anymore and needs to
 * be cleaned up.
 */
static void srpt_handle_rdma_comp(struct srpt_rdma_ch *ch,
				  struct srpt_send_ioctx *ioctx,
				  enum srpt_opcode opcode)
{
	WARN_ON(ioctx->n_rdma <= 0);
	atomic_add(ioctx->n_rdma, &ch->sq_wr_avail);

	if (opcode == SRPT_RDMA_READ_LAST) {
		if (srpt_test_and_set_cmd_state(ioctx, SRPT_STATE_NEED_DATA,
						SRPT_STATE_DATA_IN))
			target_execute_cmd(&ioctx->cmd);
		else
			pr_err("%s[%d]: wrong state = %d\n", __func__,
			       __LINE__, srpt_get_cmd_state(ioctx));
	} else if (opcode == SRPT_RDMA_ABORT) {
		ioctx->rdma_aborted = true;
	} else {
		WARN(true, "unexpected opcode %d\n", opcode);
	}
}

/**
 * srpt_handle_rdma_err_comp() - Process an IB RDMA error completion.
 */
static void srpt_handle_rdma_err_comp(struct srpt_rdma_ch *ch,
				      struct srpt_send_ioctx *ioctx,
				      enum srpt_opcode opcode)
{
	enum srpt_command_state state;

	state = srpt_get_cmd_state(ioctx);
	switch (opcode) {
	case SRPT_RDMA_READ_LAST:
		if (ioctx->n_rdma <= 0) {
			pr_err("Received invalid RDMA read"
			       " error completion with idx %d\n",
			       ioctx->ioctx.index);
			break;
		}
		atomic_add(ioctx->n_rdma, &ch->sq_wr_avail);
		if (state == SRPT_STATE_NEED_DATA)
			srpt_abort_cmd(ioctx);
		else
			pr_err("%s[%d]: wrong state = %d\n",
			       __func__, __LINE__, state);
		break;
	case SRPT_RDMA_WRITE_LAST:
		break;
	default:
		pr_err("%s[%d]: opcode = %u\n", __func__, __LINE__, opcode);
		break;
	}
}

/**
 * srpt_build_cmd_rsp() - Build an SRP_RSP response.
 * @ch: RDMA channel through which the request has been received.
 * @ioctx: I/O context associated with the SRP_CMD request. The response will
 *   be built in the buffer ioctx->buf points at and hence this function will
 *   overwrite the request data.
 * @tag: tag of the request for which this response is being generated.
 * @status: value for the STATUS field of the SRP_RSP information unit.
 *
 * Returns the size in bytes of the SRP_RSP response.
 *
 * An SRP_RSP response contains a SCSI status or service response. See also
 * section 6.9 in the SRP r16a document for the format of an SRP_RSP
 * response. See also SPC-2 for more information about sense data.
 */
static int srpt_build_cmd_rsp(struct srpt_rdma_ch *ch,
			      struct srpt_send_ioctx *ioctx, u64 tag,
			      int status)
{
	struct se_cmd *cmd = &ioctx->cmd;
	struct srp_rsp *srp_rsp;
	const u8 *sense_data;
	int sense_data_len, max_sense_len;
	u32 resid = cmd->residual_count;

	/*
	 * The lowest bit of all SAM-3 status codes is zero (see also
	 * paragraph 5.3 in SAM-3).
	 */
	WARN_ON(status & 1);

	srp_rsp = ioctx->ioctx.buf;
	BUG_ON(!srp_rsp);

	sense_data = ioctx->sense_data;
	sense_data_len = ioctx->cmd.scsi_sense_length;
	WARN_ON(sense_data_len > sizeof(ioctx->sense_data));

	memset(srp_rsp, 0, sizeof *srp_rsp);
	srp_rsp->opcode = SRP_RSP;
	srp_rsp->req_lim_delta =
		cpu_to_be32(1 + atomic_xchg(&ch->req_lim_delta, 0));
	srp_rsp->tag = tag;
	srp_rsp->status = status;

	if (cmd->se_cmd_flags & SCF_UNDERFLOW_BIT) {
		if (cmd->data_direction == DMA_TO_DEVICE) {
			/* residual data from an underflow write */
			srp_rsp->flags = SRP_RSP_FLAG_DOUNDER;
			srp_rsp->data_out_res_cnt = cpu_to_be32(resid);
		} else if (cmd->data_direction == DMA_FROM_DEVICE) {
			/* residual data from an underflow read */
			srp_rsp->flags = SRP_RSP_FLAG_DIUNDER;
			srp_rsp->data_in_res_cnt = cpu_to_be32(resid);
		}
	} else if (cmd->se_cmd_flags & SCF_OVERFLOW_BIT) {
		if (cmd->data_direction == DMA_TO_DEVICE) {
			/* residual data from an overflow write */
			srp_rsp->flags = SRP_RSP_FLAG_DOOVER;
			srp_rsp->data_out_res_cnt = cpu_to_be32(resid);
		} else if (cmd->data_direction == DMA_FROM_DEVICE) {
			/* residual data from an overflow read */
			srp_rsp->flags = SRP_RSP_FLAG_DIOVER;
			srp_rsp->data_in_res_cnt = cpu_to_be32(resid);
		}
	}

	if (sense_data_len) {
		BUILD_BUG_ON(MIN_MAX_RSP_SIZE <= sizeof(*srp_rsp));
		max_sense_len = ch->max_ti_iu_len - sizeof(*srp_rsp);
		if (sense_data_len > max_sense_len) {
			pr_warn("truncated sense data from %d to %d"
				" bytes\n", sense_data_len, max_sense_len);
			sense_data_len = max_sense_len;
		}

		srp_rsp->flags |= SRP_RSP_FLAG_SNSVALID;
		srp_rsp->sense_data_len = cpu_to_be32(sense_data_len);
		memcpy(srp_rsp + 1, sense_data, sense_data_len);
	}

	return sizeof(*srp_rsp) + sense_data_len;
}

/**
 * srpt_build_tskmgmt_rsp() - Build a task management response.
 * @ch:       RDMA channel through which the request has been received.
 * @ioctx:    I/O context in which the SRP_RSP response will be built.
 * @rsp_code: RSP_CODE that will be stored in the response.
 * @tag:      Tag of the request for which this response is being generated.
 *
 * Returns the size in bytes of the SRP_RSP response.
 *
 * An SRP_RSP response contains a SCSI status or service response. See also
 * section 6.9 in the SRP r16a document for the format of an SRP_RSP
 * response.
 */
static int srpt_build_tskmgmt_rsp(struct srpt_rdma_ch *ch,
				  struct srpt_send_ioctx *ioctx,
				  u8 rsp_code, u64 tag)
{
	struct srp_rsp *srp_rsp;
	int resp_data_len;
	int resp_len;

	resp_data_len = 4;
	resp_len = sizeof(*srp_rsp) + resp_data_len;

	srp_rsp = ioctx->ioctx.buf;
	BUG_ON(!srp_rsp);
	memset(srp_rsp, 0, sizeof *srp_rsp);

	srp_rsp->opcode = SRP_RSP;
	srp_rsp->req_lim_delta =
		cpu_to_be32(1 + atomic_xchg(&ch->req_lim_delta, 0));
	srp_rsp->tag = tag;

	srp_rsp->flags |= SRP_RSP_FLAG_RSPVALID;
	srp_rsp->resp_data_len = cpu_to_be32(resp_data_len);
	srp_rsp->data[3] = rsp_code;

	return resp_len;
}

#define NO_SUCH_LUN ((uint64_t)-1LL)

/*
 * SCSI LUN addressing method. See also SAM-2 and the section about
 * eight byte LUNs.
 */
enum scsi_lun_addr_method {
	SCSI_LUN_ADDR_METHOD_PERIPHERAL   = 0,
	SCSI_LUN_ADDR_METHOD_FLAT         = 1,
	SCSI_LUN_ADDR_METHOD_LUN          = 2,
	SCSI_LUN_ADDR_METHOD_EXTENDED_LUN = 3,
};

/*
 * srpt_unpack_lun() - Convert from network LUN to linear LUN.
 *
 * Convert an 2-byte, 4-byte, 6-byte or 8-byte LUN structure in network byte
 * order (big endian) to a linear LUN. Supports three LUN addressing methods:
 * peripheral, flat and logical unit. See also SAM-2, section 4.9.4 (page 40).
 */
static uint64_t srpt_unpack_lun(const uint8_t *lun, int len)
{
	uint64_t res = NO_SUCH_LUN;
	int addressing_method;

	if (unlikely(len < 2)) {
		pr_err("Illegal LUN length %d, expected 2 bytes or more\n",
		       len);
		goto out;
	}

	switch (len) {
	case 8:
		if ((*((__be64 *)lun) &
		     cpu_to_be64(0x0000FFFFFFFFFFFFLL)) != 0)
			goto out_err;
		break;
	case 4:
		if (*((__be16 *)&lun[2]) != 0)
			goto out_err;
		break;
	case 6:
		if (*((__be32 *)&lun[2]) != 0)
			goto out_err;
		break;
	case 2:
		break;
	default:
		goto out_err;
	}

	addressing_method = (*lun) >> 6; /* highest two bits of byte 0 */
	switch (addressing_method) {
	case SCSI_LUN_ADDR_METHOD_PERIPHERAL:
	case SCSI_LUN_ADDR_METHOD_FLAT:
	case SCSI_LUN_ADDR_METHOD_LUN:
		res = *(lun + 1) | (((*lun) & 0x3f) << 8);
		break;

	case SCSI_LUN_ADDR_METHOD_EXTENDED_LUN:
	default:
		pr_err("Unimplemented LUN addressing method %u\n",
		       addressing_method);
		break;
	}

out:
	return res;

out_err:
	pr_err("Support for multi-level LUNs has not yet been implemented\n");
	goto out;
}

static int srpt_check_stop_free(struct se_cmd *cmd)
{
	struct srpt_send_ioctx *ioctx = container_of(cmd,
				struct srpt_send_ioctx, cmd);
