t(args, &token) || token < 1) {
				pr_warn("bad queue_size parameter '%s'\n", p);
				goto out;
			}
			target->scsi_host->can_queue = token;
			target->queue_size = token + SRP_RSP_SQ_SIZE +
					     SRP_TSK_MGMT_SQ_SIZE;
			if (!(opt_mask & SRP_OPT_MAX_CMD_PER_LUN))
				target->scsi_host->cmd_per_lun = token;
			break;

		case SRP_OPT_MAX_CMD_PER_LUN:
			if (match_int(args, &token) || token < 1) {
				pr_warn("bad max cmd_per_lun parameter '%s'\n",
					p);
				goto out;
			}
			target->scsi_host->cmd_per_lun = token;
			break;

		case SRP_OPT_IO_CLASS:
			if (match_hex(args, &token)) {
				pr_warn("bad IO class parameter '%s'\n", p);
				goto out;
			}
			if (token != SRP_REV10_IB_IO_CLASS &&
			    token != SRP_REV16A_IB_IO_CLASS) {
				pr_warn("unknown IO class parameter value %x specified (use %x or %x).\n",
					token, SRP_REV10_IB_IO_CLASS,
					SRP_REV16A_IB_IO_CLASS);
				goto out;
			}
			target->io_class = token;
			break;

		case SRP_OPT_INITIATOR_EXT:
			p = match_strdup(args);
			if (!p) {
				ret = -ENOMEM;
				goto out;
			}
			target->initiator_ext = cpu_to_be64(simple_strtoull(p, NULL, 16));
			kfree(p);
			break;

		case SRP_OPT_CMD_SG_ENTRIES:
			if (match_int(args, &token) || token < 1 || token > 255) {
				pr_warn("bad max cmd_sg_entries parameter '%s'\n",
					p);
				goto out;
			}
			target->cmd_sg_cnt = token;
			break;

		case SRP_OPT_ALLOW_EXT_SG:
			if (match_int(args, &token)) {
				pr_warn("bad allow_ext_sg parameter '%s'\n", p);
				goto out;
			}
			target->allow_ext_sg = !!token;
			break;

		case SRP_OPT_SG_TABLESIZE:
			if (match_int(args, &token) || token < 1 ||
					token > SCSI_MAX_SG_CHAIN_SEGMENTS) {
				pr_warn("bad max sg_tablesize parameter '%s'\n",
					p);
				goto out;
			}
			target->sg_tablesize = token;
			break;

		case SRP_OPT_COMP_VECTOR:
			if (match_int(args, &token) || token < 0) {
				pr_warn("bad comp_vector parameter '%s'\n", p);
				goto out;
			}
			target->comp_vector = token;
			break;

		case SRP_OPT_TL_RETRY_COUNT:
			if (match_int(args, &token) || token < 2 || token > 7) {
				pr_warn("bad tl_retry_count parameter '%s' (must be a number between 2 and 7)\n",
					p);
				goto out;
			}
			target->tl_retry_count = token;
			break;

		default:
			pr_warn("unknown parameter or missing value '%s' in target creation request\n",
				p);
			goto out;
		}
	}

	if ((opt_mask & SRP_OPT_ALL) == SRP_OPT_ALL)
		ret = 0;
	else
		for (i = 0; i < ARRAY_SIZE(srp_opt_tokens); ++i)
			if ((srp_opt_tokens[i].token & SRP_OPT_ALL) &&
			    !(srp_opt_tokens[i].token & opt_mask))
				pr_warn("target creation request is missing parameter '%s'\n",
					srp_opt_tokens[i].pattern);

	if (target->scsi_host->cmd_per_lun > target->scsi_host->can_queue
	    && (opt_mask & SRP_OPT_MAX_CMD_PER_LUN))
		pr_warn("cmd_per_lun = %d > queue_size = %d\n",
			target->scsi_host->cmd_per_lun,
			target->scsi_host->can_queue);

out:
	kfree(options);
	return ret;
}

static ssize_t srp_create_target(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct srp_host *host =
		container_of(dev, struct srp_host, dev);
	struct Scsi_Host *target_host;
	struct srp_target_port *target;
	struct srp_rdma_ch *ch;
	struct srp_d