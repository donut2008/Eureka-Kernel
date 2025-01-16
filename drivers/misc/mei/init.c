el(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;

	return sprintf(buf, "%s\n", data->temp_label[data->temp_src[nr]]);
}

static ssize_t
show_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	return sprintf(buf, "%d\n", LM75_TEMP_FROM_REG(data->temp[index][nr]));
}

static ssize_t
store_temp(struct device *dev, struct device_attribute *attr, const char *buf,
	   size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	int err;
	long val;

	err = kstrtol(buf, 10, &val);
	if (err < 0)
		return err;

	mutex_lock(&data->update_lock);
	data->temp[index][nr] = LM75_TEMP_TO_REG(val);
	nct6775_write_temp(data, data->reg_temp[index][nr],
			   data->temp[index][nr]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_temp_offset(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);

	return sprintf(buf, "%d\n", data->temp_offset[sattr->index] * 1000);
}

static ssize_t
store_temp_offset(struct device *dev, struct device_attribute *attr,
		  const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err < 0)
		return err;

	val = clamp_val(DIV_ROUND_CLOSEST(val, 1000), -128, 127);

	mutex_lock(&data->update_lock);
	data->temp_offset[nr] = val;
	nct6775_write_value(data, data->REG_TEMP_OFFSET[nr], val);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t
show_temp_type(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;

	return sprintf(buf, "%d\n", (int)data->temp_type[nr]);
}

static ssize_t
store_temp_type(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	unsigned long val;
	int err;
	u8 vbat, diode, vbit, dbit;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	if (val != 1 && val != 3 && val != 4)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	data->temp_type[nr] = val;
	vbit = 0x02 << nr;
	dbit = data->DIODE_MASK << nr;
	vbat = nct6775_read_value(data, data->REG_VBAT) & ~vbit;
	diode = nct6775_read_value(data, data->REG_DIODE) & ~dbit;
	switch (val) {
	case 1:	/* CPU diode (diode, current mode) */
		vbat |= vbit;
		diode |= dbit;
		break;
	case 3: /* diode, voltage mode */
		vbat |= dbit;
		break;
	case 4:	/* thermistor */
		break;
	}
	nct6775_write_value(data, data->REG_VBAT, vbat);
	nct6775_write_value(data, data->REG_DIODE, diode);

	mutex_unlock(&data->update_lock);
	return count;
}

static umode_t nct6775_temp_is_visible(struct kobject *kobj,
				       struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int temp = index / 10;	/* temp index */
	int nr = index % 10;	/* attribute index */

	if (!(data->have_temp & (1 << temp)))
		return 0;

	if (nr == 2 && find_temp_source(data, temp, data->num_temp_alarms) < 0)
		return 0;				/* alarm */

	if (nr == 3 && find_temp_source(data, temp, data->num_temp_beeps) < 0)
		return 0;				/* beep */

	if (nr == 4 && !data->reg_temp[1][temp])	/* max */
		return 0;

	if (nr == 5 && !data->reg_temp[2][temp])	/* max_hyst */
		return 0;

	if (nr == 6 && !data->reg_temp[3][temp])	/* crit */
		return 0;

	if (nr == 7 && !data->reg_temp[4][temp])	/* lcrit */
		return 0;

	/* offset and type only apply to fixed sensors */
	if (nr > 7 && !(data->have_temp_fixed & (1 << temp)))
		return 0;

	return attr->mode;
}

SENSOR_TEMPLATE_2(temp_input, "temp%d_input", S_IRUGO, show_temp, NULL, 0, 0);
SENSOR_TEMPLATE(temp_label, "temp%d_label", S_IRUGO, show_temp_label, NULL, 0);
SENSOR_TEMPLATE_2(temp_max, "temp%d_max", S_IRUGO | S_IWUSR, show_temp,
		  store_temp, 0, 1);
SENSOR_TEMPLATE_2(temp_max_hyst, "temp%d_max_hyst", S_IRUGO | S_IWUSR,
		  show_temp, store_temp, 0, 2);
SENSOR_TEMPLATE_2(temp_crit, "temp%d_crit", S_IRUGO | S_IWUSR, show_temp,
		  store_temp, 0, 3);
SENSOR_TEMPLATE_2(temp_lcrit, "temp%d_lcrit", S_IRUGO | S_IWUSR, show_temp,
		  store_temp, 0, 4);
SENSOR_TEMPLATE(temp_offset, "temp%d_offset", S_IRUGO | S_IWUSR,
		show_temp_offset, store_temp_offset, 0);
SENSOR_TEMPLATE(temp_type, "temp%d_type", S_IRUGO | S_IWUSR, show_temp_type,
		store_temp_type, 0);
SENSOR_TEMPLATE(temp_alarm, "temp%d_alarm", S_IRUGO, show_temp_alarm, NULL, 0);
SENSOR_TEMPLATE(temp_beep, "temp%d_beep", S_IRUGO | S_IWUSR, show_temp_beep,
		store_temp_beep, 0);

/*
 * nct6775_temp_is_visible uses the index into the following array
 * to determine if attributes should be created or not.
 * Any change in order or content must be matched.
 */
static struct sensor_device_template *nct6775_attributes_temp_template[] = {
	&sensor_dev_template_temp_input,
	&sensor_dev_template_temp_label,
	&sensor_dev_template_temp_alarm,	/* 2 */
	&sensor_dev_template_temp_beep,		/* 3 */
	&sensor_dev_template_temp_max,		/* 4 */
	&sensor_dev_template_temp_max_hyst,	/* 5 */
	&sensor_dev_template_temp_crit,		/* 6 */
	&sensor_dev_template_temp_lcrit,	/* 7 */
	&sensor_dev_template_temp_offset,	/* 8 */
	&sensor_dev_template_temp_type,		/* 9 */
	NULL
};

static struct sensor_template_group nct6775_temp_template_group = {
	.templates = nct6775_attributes_temp_template,
	.is_visible = nct6775_temp_is_visible,
	.base = 1,
};

static ssize_t
show_pwm_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);

	return sprintf(buf, "%d\n", data->pwm_mode[sattr->index]);
}

static ssize_t
store_pwm_mode(struct device *dev, struct device_attribute *attr,
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

	if (val > 1)
		return -EINVAL;

	/* Setting DC mode (0) is not supported for all chips/channels */
	if (data->REG_PWM_MODE[nr] == 0) {
		if (!val)
			return -EINVAL;
		return count;
	}

	mutex_lock(&data->update_lock);
	data->pwm_mode[nr] = val;
	reg = nct6775_read_value(data, data->REG_PWM_MODE[nr]);
	reg &= ~data->PWM_MODE_MASK[nr];
	if (!val)
		reg |= data->PWM_MODE_MASK[nr];
	nct6775_write_value(data, data->REG_PWM_MODE[nr], reg);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	int pwm;

	/*
	 * For automatic fan control modes, show current pwm readings.
	 * Otherwise, show the configured value.
	 */
	if (index == 0 && data->pwm_enable[nr] > manual)
		pwm = nct6775_read_value(data, data->REG_PWM_READ[nr]);
	else
		pwm = data->pwm[index][nr];

	return sprintf(buf, "%d\n", pwm);
}

static ssize_t
store_pwm(struct device *dev, struct device_attribute *attr, const char *buf,
	  size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	unsigned long val;
	int minval[7] = { 0, 1, 1, data->pwm[2][nr], 0, 0, 0 };
	int maxval[7]
	  = { 255, 255, data->pwm[3][nr] ? : 255, 255, 255, 255, 255 };
	int err;
	u8 reg;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	val = clamp_val(val, minval[index], maxval[index]);

	mutex_lock(&data->update_lock);
	data->pwm[index][nr] = val;
	nct6775_write_value(data, data->REG_PWM[index][nr], val);
	if (index == 2)	{ /* floor: disable if val == 0 */
		reg = nct6775_read_value(data, data->REG_TEMP_SEL[nr]);
		reg &= 0x7f;
		if (val)
			reg |= 0x80;
		nct6775_write_value(data, data->REG_TEMP_SEL[nr], reg);
	}
	mutex_unlock(&data->update_lock);
	return count;
}

/* Returns 0 if OK, -EINVAL otherwise */
static int check_trip_points(struct nct6775_data *data, int nr)
{
	int i;

	for (i = 0; i < data->auto_pwm_num - 1; i++) {
		if (data->auto_temp[nr][i] > data->auto_temp[nr][i + 1])
			return -EINVAL;
	}
	for (i = 0; i < data->auto_pwm_num - 1; i++) {
		if (data->auto_pwm[nr][i] > data->auto_pwm[nr][i + 1])
			return -EINVAL;
	}
	/* validate critical temperature and pwm if enabled (pwm > 0) */
	if (data->auto_pwm[nr][data->auto_pwm_num]) {
		if (data->auto_temp[nr][data->auto_pwm_num - 1] >
				data->auto_temp[nr][data->auto_pwm_num] ||
		    data->auto_pwm[nr][data->auto_pwm_num - 1] >
				data->auto_pwm[nr][data->auto_pwm_num])
			return -EINVAL;
	}
	return 0;
}

static void pwm_update_registers(struct nct6775_data *data, int nr)
{
	u8 reg;

	switch (data->pwm_enable[nr]) {
	case off:
	case manual:
		break;
	case speed_cruise:
		reg = nct6775_read_value(data, data->REG_FAN_MODE[nr]);
		reg = (reg & ~data->tolerance_mask) |
		  (data->target_speed_tolerance[nr] & data->tolerance_mask);
		nct6775_write_value(data, data->REG_FAN_MODE[nr], reg);
		nct6775_write_value(data, data->REG_TARGET[nr],
				    data->target_speed[nr] & 0xff);
		if (data->REG_TOLERANCE_H) {
			reg = (data->t