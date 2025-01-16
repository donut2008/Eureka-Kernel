low_ext_sg;
	target->tl_retry_count	= 7;
	target->queue_size	= SRP_DEFAULT_QUEUE_SIZE;

	/*
	 * Avoid that the SCSI host can be removed by srp_remove_target()
	 * before this function returns.
	 */
	scsi_host_get(target->scsi_host);

	mutex_lock(&host->add_target_mutex);

	ret = srp_parse_options(buf, target);
	if (ret)
		goto out;

	target->req_ring_size = target->queue_size - SRP_TSK_MGMT_SQ_SIZE;

	if (!srp_conn_unique(target->srp_host, target)) {
		shost_printk(KERN_INFO, target->scsi_host,
			     PFX "Already connected to target port with id_ext=%016llx;ioc_guid=%016llx;initiator_ext=%016llx\n",
			     be64_to_cpu(target->id_ext),
			     be64_to_cpu(target->ioc_guid),
			     be64_to_cpu(target->initiator_ext));
		ret = -EEXIST;
		goto out;
	}

	if (!srp_dev->has_fmr && !srp_dev->has_fr && !target->allow_ext_sg &&
	    target->cmd_sg_cnt < target->sg_tablesize) {
		pr_warn("No MR pool and no external indirect descriptors, limiting sg_tablesize to cmd_sg_cnt\n");
		target->sg_tablesize = target->cmd_sg_cnt;
	}

	target_host->sg_tablesize = target->sg_tablesize;
	target->indirect_size = target->sg_tablesize *
				sizeof (struct srp_direct_buf);
	target->max_iu_len = sizeof (struct srp_cmd) +
			     sizeof (struct srp_indirect_buf) +
			     target->cmd_sg_cnt * sizeof (struct srp_direct_buf);

	INIT_WORK(&target->tl_err_work, srp_tl_err_work);
	INIT_WORK(&target->remove_work, srp_remove_work);
	spin_lock_init(&target->lock);
	ret = ib_query_gid(ibdev, host->port, 0, &target->sgid, NULL);
	if (ret)
		goto out;

	ret = -ENOMEM;
	target->ch_count = max_t(unsigned, num_online_nodes(),
				 min(ch_count ? :
				     min(4 * num_online_nodes(),
					 ibdev->num_comp_vectors),
				     num_online_cpus()));
	target->ch = kcalloc(target->ch_count, sizeof(*target->ch),
			     GFP_KERNEL);
	if (!target->ch)
		goto out;

	node_idx = 0;
	for_each_online_node(node) {
		const int ch_start = (node_idx * target->ch_count /
				      num_online_nodes());
		const int ch_end = ((node_idx + 1) * target->ch_count /
				    num_online_nodes());
		const int cv_start = node_idx * ibdev->num_comp_vectors /
				     num_online_nodes();
		const int cv_end = (node_idx + 1) * ibdev->num_comp_vectors /
				   num_online_nodes();
		int cpu_idx = 0;

		for_each_online_cpu(cpu) {
			if (cpu_to_node(cpu) != node)
				continue;
			if (ch_start + cpu_idx >= ch_end)
				continue;
			ch = &target->ch[ch_start + cpu_idx];
			ch->target = target;
			ch->comp_vector = cv_start == cv_end ? cv_start :
				cv_start + cpu_idx % (cv_end - cv_start);
			spin_lock_init(&ch->lock);
			INIT_LIST_HEAD(&ch->free_tx);
			ret = srp_new_cm_id(ch);
			if (ret)
				goto err_disconnect;

			ret = srp_create_ch_ib(ch);
			if (ret)
				goto err_disconnect;

			ret = srp_alloc_req_data(ch);
			if (ret)
				goto err_disconnect;

			ret = srp_connect_ch(ch, multich);
			if (ret) {
				shost_printk(KERN_ERR, target->scsi_host,
					     PFX "Connection %d/%d failed\n",
					     ch_start + cpu_idx,
					     target->ch_count);
				if (node_idx == 0 && cpu_idx == 0) {
					goto err_disconnect;
				} else {
					srp_free_ch_ib(target, ch);
					srp_free_req_data(target, ch);
					target->ch_count = ch - target->ch;
					goto connected;
				}
			}

			multich = true;
			cpu_idx++;
		}
		node_idx++;
	}

connected:
	target->scsi_host->nr_hw_queues = target->ch_count;

	ret = srp_add_target(host, target);
	if (ret)
		goto err_disconnect;

	if (target->state != SRP_TARGET_REMOVED) {
		shost_printk(KERN_DEBUG, target->scsi_host, PFX
			     "new target: id_ext %016llx ioc_guid %016llx pkey %04x service_id %016llx sgid %pI6 dgid %pI6\n",
			     be64_to_cpu(target->id_ext),
			     be64_to_cpu(target->ioc_guid),
			     be16_to_cpu(target->pkey),
			     be64_to_cpu(target->service_id),
			     target->sgid.raw, target->orig_dgid.raw);
	}

	ret = count;

out:
	mutex_unlock(&host->add_target_mutex);

	scsi_host_put(target->scsi_host);
	if (ret < 0)
		scsi_host_put