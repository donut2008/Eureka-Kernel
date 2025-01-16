tr,
			   char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "0x%016llx\n", be64_to_cpu(target->id_ext));
}

static ssize_t show_ioc_guid(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "0x%016llx\n", be64_to_cpu(target->ioc_guid));
}

static ssize_t show_service_id(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "0x%016llx\n", be64_to_cpu(target->service_id));
}

static ssize_t show_pkey(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "0x%04x\n", be16_to_cpu(target->pkey));
}

static ssize_t show_sgid(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%pI6\n", target->sgid.raw);
}

static ssize_t show_dgid(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));
	struct srp_rdma_ch *ch = &target->ch[0];

	return sprintf(buf, "%pI6\n", ch->path.dgid.raw);
}

static ssize_t show_orig_dgid(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%pI6\n", target->orig_dgid.raw);
}

static ssize_t show_req_lim(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));
	struct srp_rdma_ch *ch;
	int i, req_lim = INT_MAX;

	for (i = 0; i < target->ch_count; i++) {
		ch = &target->ch[i];
		req_lim = min(req_lim, ch->req_lim);
	}
	return sprintf(buf, "%d\n", req_lim);
}

static ssize_t show_zero_req_lim(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%d\n", target->zero_req_lim);
}

static ssize_t show_local_ib_port(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%d\n", target->srp_host->port);
}

static ssize_t show_local_ib_device(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%s\n", target->srp_host->srp_dev->dev->name);
}

static ssize_t show_ch_count(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%d\n", target->ch_count);
}

static ssize_t show_comp_vector(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct srp_target_port *target = host_to_target(class_to_shost(dev));

	return sprintf(buf, "%d\n", target->comp_vector);
}

static ssize_t show_tl_retry_count(st