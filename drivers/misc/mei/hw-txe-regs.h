ibute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int index = sattr->index;
	int nr = sattr->nr;

	return sprintf(buf, "%ld\n", in_from_reg(data->in[nr][index], nr));
}

static ssize_t
store_in_reg(struct device *dev, struct device_attribute *attr, const char *buf,
	     size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int index = sattr->index;
	int nr = sattr->nr;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	mutex_lock(&data->update_lock);
	data->in[nr][index] = in_to_reg(val, nr);
	nct6775_write_value(data, data->REG_IN_MINMAX[index - 1][nr],
			    data->in[nr][index]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_alarm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = data->ALARM_BITS[sattr->index];

	return sprintf(buf, "%u\n",
		       (unsigned int)((data->alarms >> nr) & 0x01));
}

static int find_temp_source(struct nct6775_data *data, int index, int count)
{
	int source = data->temp_src[index];
	int nr;

	for (nr = 0; nr < count; nr++) {
		int src;

		src = nct6775_read_value(data,
					 data->REG_TEMP_SOURCE[nr]) & 0x1f;
		if (src == source)
			return nr;
	}
	return -ENODEV;
}

static ssize_t
show_temp_alarm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct nct6775_data *data = nct6775_update_device(dev);
	unsigned int alarm = 0;
	int nr;

	/*
	 * For temperatures, there is no fixed mapping from registers to alarm
	 * bits. Alarm bits are determined by the temperature source mapping.
	 */
	nr = find_temp_source(data, sattr->index, data->num_temp_alarms);
	if (nr >= 0) {
		int bit = data->ALARM_BITS[nr + TEMP_ALARM_BASE];

		alarm = (data->alarms >> bit) & 0x01;
	}
	return sprintf(buf, "%u\n", alarm);
}

static ssize_t
show_beep(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct nct6775_data *data = nct6775_update_device(dev);
	int nr = data->BEEP_BITS[sattr->index];

	return sprintf(buf, "%u\n",
		       (unsigned int)((data->beeps >> nr) & 0x01));
}

static ssize_t
store_beep(struct device *dev, struct device_attribute *attr, const char *buf,
	   size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int nr = data->BEEP_BITS[sattr->index];
	int regindex = nr >> 3;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	if (val > 1)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	if (val)
		data->beeps |= (1ULL << nr);
	else
		data->beeps &= ~(1ULL << nr);
	nct6775_write_value(data, data->REG_BEEP[regindex],
			    (data->beeps >> (regindex << 3)) & 0xff);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_temp_beep(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct nct6775_data *data = nct6775_update_device(dev);
	unsigned int beep = 0;
	int nr;

	/*
	 * For temperatures, there is no fixed mapping from registers to beep
	 * enable bits. Beep enable bits are determined by the temperature
	 * source mapping.
	 */
	nr = find_temp_source(data, sattr->index, data->num_temp_beeps);
	if (nr >= 0) {
		int bit = data->BEEP_BITS[nr + TEMP_ALARM_BASE];

		beep = (data->beeps >> bit) & 0x01;
	}
	return sprintf(buf, "%u\n", beep);
}

static ssize_t
store_temp_beep(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int nr, bit, regindex;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	if (val > 1)
		return -EINVAL;

	nr = find_temp_source(data, sattr->index, data->num_temp_beeps);
	if (nr < 0)
		return nr;

	bit = data->BEEP_BITS[nr + TEMP_ALARM_BASE];
	regindex = bit >> 3;

	mutex_lock(&data->update_lock);
	if (val)
		data->beeps |= (1ULL << bit);
	else
		data->beeps &= ~(1ULL << bit);
	nct6775_write_value(data, data->REG_BEEP[regindex],
			    (data->beeps >> (regindex << 3)) & 0xff);
	mutex_unlock(&data->update_lock);

	return count;
}

static umode_t nct6775_in_is_visible(struct kobject *kobj,
				     struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int in = index / 5;	/* voltage index */

	if (!(data->have_in & (1 << in)))
		return 0;

	return attr->mode;
}

SENSOR_TEMPLATE_2(in_input, "in%d_input", S_IRUGO, show_in_reg, NULL, 0, 0);
SENSOR_TEMPLATE(in_alarm, "in%d_alarm", S_IRUGO, show_alarm, NULL, 0);
SENSOR_TEMPLATE(in_beep, "in%d_beep", S_IWUSR | S_IRUGO, show_beep, store_beep,
		0);
SENSOR_TEMPLATE_2(in_min, "in%d_min", S_IWUSR | S_IRUGO, show_in_reg,
		  store_in_reg, 0, 1);
SENSOR_TEMPLATE_2(in_max, "in%d_max", S_IWUSR | S_IRUGO, show_in_reg,
		  store_in_reg, 0, 2);

/*
 * nct6775_in_is_visible uses the index into the following array
 * to determine if attributes should be created or not.
 * Any change in order or content must be matched.
 */
static struct sensor_device_template *nct6775_attributes_in_template[] = {
	&sensor_dev_template_in_input,
	&sensor_dev_template_in_alarm,
	&sensor_dev_template_in_beep,
	&sensor_dev_template_in_min,
	&sensor_dev_template_in_max,
	NULL
};

static struct sensor_template_group nct6775_in_template_group = {
	.templates = nct6775_attributes_in_template,
	.is_visible = nct6775_in_is_visible,
};

static ssize_t
show_fan(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;

	return sprintf(buf, "%d\n", data->rpm[nr]);
}

static ssize_t
show_fan_min(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;

	return sprintf(buf, "%d\n",
		       data->fan_from_reg_min(data->fan_min[nr],
					      data->fan_div[nr]));
}

static ssize_t
show_fan_div(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;

	return sprintf(buf, "%u\n", div_from_reg(data->fan_div[nr]));
}

static ssize_t
store_fan_min(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	unsigned long val;
	unsigned int reg;
	u8 new_div;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	mutex_lock(&data->update_lock);
	if (!data->has_fan_div) {
		/* NCT6776F or NCT6779D; we know this is a 13 bit register */
		if (!val) {
			val = 0xff1f;
		} else {
			if (val > 1350000U)
				val = 135000U;
			val = 1350000U / val;
			val = (val & 0x1f) | ((val << 3) & 0xff00);
		}
		data->fan_min[nr] = val;
		goto write_min;	/* Leave fan divider alone */
	}
	if (!val) {
		/* No min limit, alarm disabled */
		data->fan_min[nr] = 255;
		new_div = data->fan_div[nr]; /* No change */
		dev_info(dev, "fan%u low limit and alarm disabled\n", nr + 1);
		goto write_div;
	}
	reg = 1350000U / val;
	if (reg >= 128 * 255) {
		/*
		 * Speed below this value cannot possibly be represented,
		 * even with the highest divider (128)
		 */
		data->fan_min[nr] = 254;
		new_div = 7; /* 128 == (1 << 7) */
		dev_warn(dev,
			 "fan%u low limit %lu below minimum %u, set to minimum\n",
			 nr + 1, val, data->fan_from_reg_min(254, 7));
	} else if (!reg) {
		/*
		 * Speed above this value cannot possibly be represented,
		 * even with the lowest divider (1)
		 */
		data->fan_min[nr] = 1;
		new_div = 0; /* 1 == (1 << 0) */
		dev_warn(dev,
			 "fan%u low limit %lu above maximum %u, set to maximum\n",
			 nr + 1, val, data->fan_from_reg_min(1, 0));
	} else {
		/*
		 * Automatically pick the best divider, i.e. the one such
		 * that the min limit will correspond to a register value
		 * in the 96..192 range
		 */
		new_div = 0;
		while (reg > 192 && new_div < 7) {
			reg >>= 1;
			new_div++;
		}
		data->fan_min[nr] = reg;
	}

write_div:
	/*
	 * Write both the fan clock divider (if it changed) and the new
	 * fan min (unconditionally)
	 */
	if (new_div != data->fan_div[nr]) {
		dev_dbg(dev, "fan%u clock divider changed from %u to %u\n",
			nr + 1, div_from_reg(data->fan_div[nr]),
			div_from_reg(new_div));
		data->fan_div[nr] = new_div;
		nct6775_write_fan_div_common(data, nr);
		/* Give the chip time to sample a new speed value */
		data->last_updated = jiffies;
	}

write_min:
	nct6775_write_value(data, data->REG_FAN_MIN[nr], data->fan_min[nr]);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t
show_fan_pulses(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int p = data->fan_pulses[sattr->index];

	return sprintf(buf, "%d\n", p ? : 4);
}

static ssize_t
store_fan_pulses(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	unsigned long val;
	int err;
	u8 reg;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	if (val > 4)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->fan_pulses[nr] = val & 3;
	reg = nct6775_read_value(data, data->REG_FAN_PULSES[nr]);
	reg &= ~(0x03 << data->FAN_PULSE_SHIFT[nr]);
	reg |= (val & 3) << data->FAN_PULSE_SHIFT[nr];
	nct6775_write_value(data, data->REG_FAN_PULSES[nr], reg);
	mutex_unlock(&data->update_lock);

	return count;
}

static umode_t nct6775_fan_is_visible(struct kobject *kobj,
				      struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int fan = index / 6;	/* fan index */
	int nr = index % 6;	/* attribute index */

	if (!(data->has_fan & (1 << fan)))
		return 0;

	if (nr == 1 && data->ALARM_BITS[FAN_ALARM_BASE + fan] == -1)
		return 0;
	if (nr == 2 && data->BEEP_BITS[FAN_ALARM_BASE + fan] == -1)
		return 0;
	if (nr == 4 && !(data->has_fan_min & (1 << fan)))
		return 0;
	if (nr == 5 && data->kind != nct6775)
		return 0;

	return attr->mode;
}

SENSOR_TEMPLATE(fan_input, "fan%d_input", S_IRUGO, show_fan, NULL, 0);
SENSOR_TEMPLA