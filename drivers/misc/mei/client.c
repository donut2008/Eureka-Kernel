>index;
	unsigned long val;
	int err;
	u16 speed;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	val = clamp_val(val, 0, 1350000U);
	speed = fan_to_reg(val, data->fan_div[nr]);

	mutex_lock(&data->update_lock);
	data->target_speed[nr] = speed;
	pwm_update_registers(data, nr);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_temp_tolerance(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	return sprintf(buf, "%d\n", data->temp_tolerance[index][nr] * 1000);
}

static ssize_t
store_temp_tolerance(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	/* Limit tolerance as needed */
	val = clamp_val(DIV_ROUND_CLOSEST(val, 1000), 0, data->tolerance_mask);

	mutex_lock(&data->update_lock);
	data->temp_tolerance[index][nr] = val;
	if (index)
		pwm_update_registers(data, nr);
	else
		nct6775_write_value(data,
				    data->REG_CRITICAL_TEMP_TOLERANCE[nr],
				    val);
	mutex_unlock(&data->update_lock);
	return count;
}

/*
 * Fan speed tolerance is a tricky beast, since the associated register is
 * a tick counter, but the value is reported and configured as rpm.
 * Compute resulting low and high rpm values and report the difference.
 */
static ssize_t
show_speed_tolerance(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	int low = data->target_speed[nr] - data->target_speed_tolerance[nr];
	int high = data->target_speed[nr] + data->target_speed_tolerance[nr];
	int tolerance;

	if (low <= 0)
		low = 1;
	if (high > 0xffff)
		high = 0xffff;
	if (high < low)
		high = low;

	tolerance = (fan_from_reg16(low, data->fan_div[nr])
		     - fan_from_reg16(high, data->fan_div[nr])) / 2;

	return sprintf(buf, "%d\n", tolerance);
}

static ssize_t
store_speed_tolerance(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int nr = sattr->index;
	unsigned long val;
	int err;
	int low, high;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	high = fan_from_reg16(data->target_speed[nr],
			      data->fan_div[nr]) + val;
	low = fan_from_reg16(data->target_speed[nr],
			     data->fan_div[nr]) - val;
	if (low <= 0)
		low = 1;
	if (high < low)
		high = low;

	val = (fan_to_reg(low, data->fan_div[nr]) -
	       fan_to_reg(high, data->fan_div[nr])) / 2;

	/* Limit tolerance as needed */
	val = clamp_val(val, 0, data->speed_tolerance_limit);

	mutex_lock(&data->update_lock);
	data->target_speed_tolerance[nr] = val;
	pwm_update_registers(data, nr);
	mutex_unlock(&data->update_lock);
	return count;
}

SENSOR_TEMPLATE_2(pwm, "pwm%d", S_IWUSR | S_IRUGO, show_pwm, store_pwm, 0, 0);
SENSOR_TEMPLATE(pwm_mode, "pwm%d_mode", S_IWUSR | S_IRUGO, show_pwm_mode,
		store_pwm_mode, 0);
SENSOR_TEMPLATE(pwm_enable, "pwm%d_enable", S_IWUSR | S_IRUGO, show_pwm_enable,
		store_pwm_enable, 0);
SENSOR_TEMPLATE(pwm_temp_sel, "pwm%d_temp_sel", S_IWUSR | S_IRUGO,
		show_pwm_temp_sel, store_pwm_temp_sel, 0);
SENSOR_TEMPLATE(pwm_target_temp, "pwm%d_target_temp", S_IWUSR | S_IRUGO,
		show_target_temp, store_target_temp, 0);
SENSOR_TEMPLATE(fan_target, "fan%d_target", S_IWUSR | S_IRUGO,
		show_target_speed, store_target_speed, 0);
SENSOR_TEMPLATE(fan_tolerance, "fan%d_tolerance", S_IWUSR | S_IRUGO,
		show_speed_tolerance, store_speed_tolerance, 0);

/* Smart Fan registers */

static ssize_t
show_weight_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	return sprintf(buf, "%d\n", data->weight_temp[index][nr] * 1000);
}

static ssize_t
store_weight_temp(struct device *dev, struct device_attribute *attr,
		  const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	val = clamp_val(DIV_ROUND_CLOSEST(val, 1000), 0, 255);

	mutex_lock(&data->update_lock);
	data->weight_temp[index][nr] = val;
	nct6775_write_value(data, data->REG_WEIGHT_TEMP[index][nr], val);
	mutex_unlock(&data->update_lock);
	return count;
}

SENSOR_TEMPLATE(pwm_weight_temp_sel, "pwm%d_weight_temp_sel", S_IWUSR | S_IRUGO,
		  show_pwm_weight_temp_sel, store_pwm_weight_temp_sel, 0);
SENSOR_TEMPLATE_2(pwm_weight_temp_step, "pwm%d_weight_temp_step",
		  S_IWUSR | S_IRUGO, show_weight_temp, store_weight_temp, 0, 0);
SENSOR_TEMPLATE_2(pwm_weight_temp_step_tol, "pwm%d_weight_temp_step_tol",
		  S_IWUSR | S_IRUGO, show_weight_temp, store_weight_temp, 0, 1);
SENSOR_TEMPLATE_2(pwm_weight_temp_step_base, "pwm%d_weight_temp_step_base",
		  S_IWUSR | S_IRUGO, show_weight_temp, store_weight_temp, 0, 2);
SENSOR_TEMPLATE_2(pwm_weight_duty_step, "pwm%d_weight_duty_step",
		  S_IWUSR | S_IRUGO, show_pwm, store_pwm, 0, 5);
SENSOR_TEMPLATE_2(pwm_weight_duty_base, "pwm%d_weight_duty_base",
		  S_IWUSR | S_IRUGO, show_pwm, store_pwm, 0, 6);

static ssize_t
show_fan_time(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	return sprintf(buf, "%d\n",
		       step_time_from_reg(data->fan_time[index][nr],
					  data->pwm_mode[nr]));
}

static ssize_t
store_fan_time(struct device *dev, struct device_attribute *attr,
	       const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	val = step_time_to_reg(val, data->pwm_mode[nr]);
	mutex_lock(&data->update_lock);
	data->fan_time[index][nr] = val;
	nct6775_write_value(data, data->REG_FAN_TIME[index][nr], val);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_auto_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	return sprintf(buf, "%d\n", data->auto_pwm[sattr->nr][sattr->index]);
}

static ssize_t
store_auto_pwm(struct device *dev, struct device_attribute *attr,
	       const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int point = sattr->index;
	unsigned long val;
	int err;
	u8 reg;

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;
	if (val > 255)
		return -EINVAL;

	if (point == data->auto_pwm_num) {
		if (data->kind != nct6775 && !val)
			return -EINVAL;
		if (data->kind != nct6779 && val)
			val = 0xff;
	}

	mutex_lock(&data->update_lock);
	data->auto_pwm[nr][point] = val;
	if (point < data->auto_pwm_num) {
		nct6775_write_value(data,
				    NCT6775_AUTO_PWM(data, nr, point),
				    data->auto_pwm[nr][point]);
	} else {
		switch (data->kind) {
		case nct6775:
			/* disable if needed (pwm == 0) */
			reg = nct6775_read_value(data,
						 NCT6775_REG_CRITICAL_ENAB[nr]);
			if (val)
				reg |= 0x02;
			else
				reg &= ~0x02;
			nct6775_write_value(data, NCT6775_REG_CRITICAL_ENAB[nr],
					    reg);
			break;
		case nct6776:
			break; /* always enabled, nothing to do */
		case nct6106:
		case nct6779:
		case nct6791:
		case nct6792:
		case nct6793:
			nct6775_write_value(data, data->REG_CRITICAL_PWM[nr],
					    val);
			reg = nct6775_read_value(data,
					data->REG_CRITICAL_PWM_ENABLE[nr]);
			if (val == 255)
				reg &= ~data->CRITICAL_PWM_ENABLE_MASK;
			else
				reg |= data->CRITICAL_PWM_ENABLE_MASK;
			nct6775_write_value(data,
					    data->REG_CRITICAL_PWM_ENABLE[nr],
					    reg);
			break;
		}
	}
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t
show_auto_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = nct6775_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int point = sattr->index;

	/*
	 * We don't know for sure if the temperature is signed or unsigned.
	 * Assume it is unsigned.
	 */
	return sprintf(buf, "%d\n", data->auto_temp[nr][point] * 1000);
}

static ssize_t
store_auto_temp(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int point = sattr->index;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 10, &val);
	if (err)
		return err;
	if (val > 255000)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->auto_temp[nr][point] = DIV_ROUND_CLOSEST(val, 1000);
	if (point < data->auto_pwm_num) {
		nct6775_write_value(data,
				    NCT6775_AUTO_TEMP(data, nr, point),
				    data->auto_temp[nr][point]);
	} else {
		nct6775_write_value(data, data->REG_CRITICAL_TEMP[nr],
				    data->auto_temp[nr][point]);
	}
	mutex_unlock(&data->update_lock);
	return count;
}

static umode_t nct6775_pwm_is_visible(struct kobject *kobj,
				      struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct6775_data *data = dev_get_drvdata(dev);
	int pwm = index / 36;	/* pwm index */
	int nr = index % 36;	/* attribute index */

	if (!(data->has_pwm & (1 << pwm)))
		return 0;

	if ((nr >= 14 && nr <= 18) || nr == 21)   /* weight */
		if (!data->REG_WEIGHT_TEMP_SEL[pwm])
			return 0;
	if (nr == 19 && data->REG_PWM[3] == NULL) /* pwm_max */
		return 0;
	if (nr == 20 && data->REG_PWM[4] == NULL) /* pwm_step */
		return 0;
	if (nr == 21 && data->REG_PWM[6] == NULL) /* weight_duty_base */
		return 0;

	if (nr >= 22 && nr <= 35) {		/* auto point */
		int api = (nr - 22) / 2;	/* auto point index */

		if (api > data->auto_pwm_num)
			return 0;
	}
	return attr->mode;
}

SENSOR_TEMPLATE_2(pwm_stop_time, "pwm%d_stop_time", S_IWUSR | S_IRUGO,
		  show_fan_time, store_fan_time, 0, 0);
SENSOR_TEMPLATE_2(pwm_step_up_time, "pwm%d_step_up_time", S_IWUSR | S_IRUGO,
		  show_fan_time, store_fan_time, 0, 1);
SENSOR_TEMPLATE_2(pwm_step_down_time, "pwm%d_step_down_time", S_IWUSR | S_IRUGO,
		  show_fan_time, store_fan_time, 0, 2);
SENSOR_TEMPLATE_2(pwm_start, "pwm%d_start", S_IWUSR | S_IRUGO, show_pwm,
		  store_pwm, 0, 1);
SENSOR_TEMPLATE_2(pwm_floor, "pwm%d_floor", S_IWUSR | S_IRUGO, show_pwm,
		  store_pwm, 0, 2);
SENSOR_TEMPLATE_2(pwm_temp_tolerance, "pwm%d_temp_tolerance", S_IWUSR | S_IRUGO,
		  show_temp_tolerance, store_temp_tolerance, 0, 0);
SENSOR_TEMPLATE_2(pwm_crit_temp_tolerance, "pwm%d_crit_temp_tolerance",
		  S_IWUSR | S_IRUGO, show_temp_tolerance, store_temp_tolerance,
		  0, 1);

SENSOR_TEMPLATE_2(pwm_max, "pwm%d_max", S_IWUSR | S_IRUGO, show_pwm, store_pwm,
		  0, 3);

SENSOR_TEMPLATE_2(pwm_step, "pwm%d_step", S_IWUSR | S_IRUGO, show_pwm,
		  store_pwm, 0, 4);

SENSOR_TEMPLATE_2(pwm_auto_point1_pwm, "pwm%d_auto_point1_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 0);
SENSOR_TEMPLATE_2(pwm_auto_point1_temp, "pwm%d_auto_point1_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 0);

SENSOR_TEMPLATE_2(pwm_auto_point2_pwm, "pwm%d_auto_point2_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 1);
SENSOR_TEMPLATE_2(pwm_auto_point2_temp, "pwm%d_auto_point2_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 1);

SENSOR_TEMPLATE_2(pwm_auto_point3_pwm, "pwm%d_auto_point3_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 2);
SENSOR_TEMPLATE_2(pwm_auto_point3_temp, "pwm%d_auto_point3_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 2);

SENSOR_TEMPLATE_2(pwm_auto_point4_pwm, "pwm%d_auto_point4_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 3);
SENSOR_TEMPLATE_2(pwm_auto_point4_temp, "pwm%d_auto_point4_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 3);

SENSOR_TEMPLATE_2(pwm_auto_point5_pwm, "pwm%d_auto_point5_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 4);
SENSOR_TEMPLATE_2(pwm_auto_point5_temp, "pwm%d_auto_point5_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 4);

SENSOR_TEMPLATE_2(pwm_auto_point6_pwm, "pwm%d_auto_point6_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 5);
SENSOR_TEMPLATE_2(pwm_auto_point6_temp, "pwm%d_auto_point6_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 5);

SENSOR_TEMPLATE_2(pwm_auto_point7_pwm, "pwm%d_auto_point7_pwm",
		  S_IWUSR | S_IRUGO, show_auto_pwm, store_auto_pwm, 0, 6);
SENSOR_TEMPLATE_2(pwm_auto_point7_temp, "pwm%d_auto_point7_temp",
		  S_IWUSR | S_IRUGO, show_auto_temp, store_auto_temp, 0, 6);

/*
 * nct6775_pwm_is_visible uses the index into the following array
 * to determine if attributes should be created or not.
 * Any change in order or content must be matched.
 */
static struct sensor_device_template *nct6775_attributes_pwm_template[] = {
	&sensor_dev_template_pwm,
	&sensor_dev_template_pwm_mode,
	&sensor_dev_template_pwm_enable,
	&sensor_dev_template_pwm_temp_sel,
	&sensor_dev_template_pwm_temp_tolerance,
	&sensor_dev_template_pwm_crit_temp_tolerance,
	&sensor_dev_template_pwm_target_temp,
	&sensor_dev_template_fan_target,
	&sensor_dev_template_fan_tolerance,
	&sensor_dev_template_pwm_stop_time,
	&sensor_dev_template_pwm_step_up_time,
	&sensor_dev_template_pwm_step_down_time,
	&sensor_dev_template_pwm_start,
	&sensor_dev_template_pwm_floor,
	&sensor_dev_template_pwm_weight_temp_sel,	/* 14 */
	&sensor_dev_template_pwm_weight_temp_step,
	&sensor_dev_template_pwm_weight_temp_step_tol,
	&sensor_dev_template_pwm_weight_temp_step_base,
	&sensor_dev_template_pwm_weight_duty_step,	/* 18 */
	&sensor_dev_template_pwm_max,			/* 19 */
	&sensor_dev_template_pwm_step,			/* 20 */
	&sensor_dev_template_pwm_weight_duty_base,	/* 21 */
	&sensor_dev_template_pwm_auto_point1_pwm,	/* 22 */
	&sensor_dev_template_pwm_auto_point1_temp,
	&sensor_dev_template_pwm_auto_point2_pwm,
	&sensor_dev_template_pwm_auto_point2_temp,
	&sensor_dev_template_pwm_auto_point3_pwm,
	&sensor_dev_template_pwm_auto_point3_temp,
	&sensor_dev_template_pwm_auto_point4_pwm,
	&sensor_dev_template_pwm_auto_point4_temp,
	&sensor_dev_template_pwm_auto_point5_pwm,
	&sensor_dev_template_pwm_auto_point5_temp,
	&sensor_dev_template_pwm_auto_point6_pwm,
	&sensor_dev_template_pwm_auto_point6_temp,
	&sensor_dev_template_pwm_auto_point7_pwm,
	&sensor_dev_template_pwm_auto_point7_temp,	/* 35 */

	NULL
};

static struct sensor_template_group nct6775_pwm_template_group = {
	.templates = nct6775_attributes_pwm_template,
	.is_visible = nct6775_pwm_is_visible,
	.base = 1,
};

static ssize_t
show_vid(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct nct6775_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", vid_from_reg(data->vid, data->vrm));
}

static DEVICE_ATTR(cpu0_vid, S_IRUGO, show_vid, NULL);

/* Case open detection */

static ssize_t
clear_caseopen(struct device *dev, struct device_attribute *attr,
	       const char *buf, size_t count)
{
	struct nct6775_data *data = dev_get_drvdata(dev);
	int nr = to_sensor_dev_attr(attr)->index - INTRUSION_ALARM_BASE;
	unsigned long val;
	u8 reg;
	int ret;

	if (kstrtoul(buf, 10, &val) || val != 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	/*
	 * Use CR registers to clear caseopen status.
	 * The CR registers are the same for all chips, and not all chips
	 * support clearing the caseopen status through "regular" registers.
	 */
	ret = superio_enter(data->sioreg);
	if (ret) {
		count = ret;
		goto error;
	}

	superio_select(data->sioreg, NCT6775_LD_ACPI);
	reg = superio_inb(data->sioreg, NCT6775_REG_CR_CASEOPEN_CLR[nr]);
	reg |= NCT6775_CR_CASEOPEN_CLR_MASK[nr];
	superio_outb(data->sioreg, NCT6775_REG_CR_CASEOPEN_CLR[nr], reg);
	reg &= ~NCT6775_CR_CASEOPEN_CLR_MASK[nr];
	superio_outb(data->sioreg, NCT6775_REG_CR_CASEOPEN_CLR[nr], reg);
	superio_exit(data->sioreg);

	data->valid = false;	/* Force cache refresh */
error:
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR(intrusion0_alarm, S_IWUSR | S_IRUGO, show_alarm,
			  clear_caseopen, INTRUSION_ALARM_BASE);
static SENSOR_DEVICE_ATTR(intrusion1_alarm, S_IWUSR | S_IRUGO, show_alarm,
			  clear_caseopen, INTRUSION_ALARM_BASE + 1);
static SENSOR_DEVICE_ATTR(intrusion0_beep, S_IWUSR | S_IRUGO, show_beep,
			  store_beep, INTRUSION_ALARM_BASE);
static SENSOR_DEVICE_ATTR(intrusion1_beep, S_IWUSR | S_IRUGO, show_beep,
			  store_beep, INTRUSION_ALARM_BASE + 1);
static SENSOR_DEVICE_ATTR(beep_enable, S_IWUSR | S_IRUGO, show_beep,
			  store_beep, BEEP_ENABLE_BASE);

static umode_t nct6775_other_is_visible(struct kobject *kobj,
					struct attribute *attr, int index)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct nct6775_data *data = dev_get_drvdata(dev);

	if (index == 0 && !data->have_vid)
		return 0;

	if (index == 1 || index == 2) {
		if (data->ALARM_BITS[INTRUSION_ALARM_BASE + index - 1] < 0)
			return 0;
	}

	if (index == 3 || index == 4) {
		if (data->BEEP_BITS[INTRUSION_ALARM_BASE + index - 3] < 0)
			return 0;
	}

	return attr->mode;
}

/*
 * nct6775_other_is_visible uses the index into the following array
 * to determine if attributes should be created or not.
 * Any change in order or content must be matched.
 */
static struct attribute *nct6775_attributes_other[] = {
	&dev_attr_cpu0_vid.attr,				/* 0 */
	&sensor_dev_attr_intrusion0_alarm.dev_attr.attr,	/* 1 */
	&sensor_dev_attr_intrusion1_alarm.dev_attr.attr,	/* 2 */
	&sensor_dev_attr_intrusion0_beep.dev_attr.attr,		/* 3 */
	&sensor_dev_attr_intrusion1_beep.dev_attr.attr,		/* 4 */
	&sensor_dev_attr_beep_enable.dev_attr.attr,		/* 5 */

	NULL
};

static const struct attribute_group nct6775_group_other = {
	.attrs = nct6775_attributes_other,
	.is_visible = nct6775_other_is_visible,
};

static inline void nct6775_init_device(struct nct6775_data *data)
{
	int i;
	u8 tmp, diode;

	/* Start monitoring if needed */
	if (data->REG_CONFIG) {
		tmp = nct6775_read_value(data, data->REG_CONFIG);
		if (!(tmp & 0x01))
			nct6775_write_value(data, data->REG_CONFIG, tmp | 0x01);
	}

	/* Enable temperature sensors if needed */
	for (i = 0; i < NUM_TEMP; i++) {
		if (!(data->have_temp & (1 << i)))
			continue;
		if (!data->reg_temp_config[i])
			continue;
		tmp = nct6775_read_value(data, data->reg_temp_config[i]);
		if (tmp & 0x01)
			nct6775_write_value(data, data->reg_temp_config[i],
					    tmp & 0xfe);
	}

	/* Enable VBAT monitoring if needed */
	tmp = nct6775_read_value(data, data->REG_VBAT);
	if (!(tmp & 0x01))
		nct6775_write_value(data, data->REG_VBAT, tmp | 0x01);

	diode = nct6775_read_value(data, data->REG_DIODE);

	for (i = 0; i < data->temp_fixed_num; i++) {
		if (!(data->have_temp_fixed & (1 << i)))
			continue;
		if ((tmp & (data->DIODE_MASK << i)))	/* diode */
			data->temp_type[i]
			  = 3 - ((diode >> i) & data->DIODE_MASK);
		else				/* thermistor */
			data->temp_type[i] = 4;
	}
}

static void
nct6775_check_fan_inputs(struct nct6775_data *data)
{
	bool fan3pin, fan4pin, fan4min, fan5pin, fan6pin;
	bool pwm3pin, pwm4pin, pwm5pin, pwm6pin;
	int sioreg = data->sioreg;
	int regval;

	/* Store SIO_REG_ENABLE for use during resume */
	superio_select(sioreg, NCT6775_LD_HWM);
	data->sio_reg_enable = superio_inb(sioreg, SIO_REG_ENABLE);

	/* fan4 and fan5 share some pins with the GPIO and serial flash */
	if (data->kind == nct6775) {
		regval = superio_inb(sioreg, 0x2c);

		fan3pin = regval & (1 << 6);
		pwm3pin = regval & (1 << 7);

		/* On NCT6775, fan4 shares pins with the fdc interface */
		fan4pin = !(superio_inb(sioreg, 0x2A) & 0x80);
		fan4min = false;
		fan5pin = false;
		fan6pin = false;
		pwm4pin = false;
		pwm5pin = false;
		pwm6pin = false;
	} else if (data->kind == nct6776) {
		bool gpok = superio_inb(sioreg, 0x27) & 0x80;
		const char *board_vendor, *board_name;

		board_vendor = dmi_get_system_info(DMI_BOARD_VENDOR);
		board_name = dmi_get_system_info(DMI_BOARD_NAME);

		if (board_name && board_vendor &&
		    !strcmp(board_vendor, "ASRock")) {
			/*
			 * Auxiliary fan monitoring is not enabled on ASRock
			 * Z77 Pro4-M if booted in UEFI Ultra-FastBoot mode.
			 * Observed with BIOS version 2.00.
			 */
			if (!strcmp(board_name, "Z77 Pro4-M")) {
				if ((data->sio_reg_enable & 0xe0) != 0xe0) {
					data->sio_reg_enable |= 0xe0;
					superio_outb(sioreg, SIO_REG_ENABLE,
						     data->sio_reg_enable);
				}
			}
		}

		if (data->sio_reg_enable & 0x80)
			fan3pin = gpok;
		else
			fan3pin = !(superio_inb(sioreg, 0x24) & 0x40);

		if (data->sio_reg_enable & 0x40)
			fan4pin = gpok;
		else
			fan4pin = superio_inb(sioreg, 0x1C) & 0x01;

		if (data->sio_reg_enable & 0x20)
			fan5pin = gpok;
		else
			fan5pin = superio_inb(sioreg, 0x1C) & 0x02;

		fan4min = fan4pin;
		fan6pin = false;
		pwm3pin = fan3pin;
		pwm4pin = false;
		pwm5pin = false;
		pwm6pin = false;
	} else if (data->kind == nct6106) {
		regval = superio_inb(sioreg, 0x24);
		fan3pin = !(regval & 0x80);
		pwm3pin = regval & 0x08;

		fan4pin = false;
		fan4min = false;
		fan5pin = false;
		fan6pin = false;
		pwm4pin = false;
		pwm5pin = false;
		pwm6pin = false;
	} else {	/* NCT6779D, NCT6791D, NCT6792D, or NCT6793D */
		regval = superio_inb(sioreg, 0x1c);

		fan3pin = !(regval & (1 << 5));
		fan4pin = !(regval & (1 << 6));
		fan5pin = !(regval & (1 << 7));

		pwm3pin = !(regval & (1 << 0));
		pwm4pin = !(regval & (1 << 1));
		pwm5pin = !(regval & (1 << 2));

		fan4min = fan4pin;

		if (data->kind == nct6791 || data->kind == nct6792 ||
		    data->kind == nct6793) {
			regval = superio_inb(sioreg, 0x2d);
			fan6pin = (regval & (1 << 1));
			pwm6pin = (regval & (1 << 0));
		} else {	/* NCT6779D */
			fan6pin = false;
			pwm6pin = false;
		}
	}

	/* fan 1 and 2 (0x03) are always present */
	data->has_fan = 0x03 | (fan3pin << 2) | (fan4pin << 3) |
		(fan5pin << 4) | (fan6pin << 5);
	data->has_fan_min = 0x03 | (fan3pin << 2) | (fan4min << 3) |
		(fan5pin << 4);
	data->has_pwm = 0x03 | (pwm3pin << 2) | (pwm4pin << 3) |
		(pwm5pin << 4) | (pwm6pin << 5);
}

static void add_temp_sensors(struct nct6775_data *data, const u16 *regp,
			     int *available, int *mask)
{
	int i;
	u8 src;

	for (i = 0; i < data->pwm_num && *available; i++) {
		int index;

		if (!regp[i])
			continue;
		src = nct6775_read_value(data, regp[i]);
		src &= 0x1f;
		if (!src || (*mask & (1 << src)))
			continue;
		if (src >= data->temp_label_num ||
		    !strlen(data->temp_label[src]))
			continue;

		index = __ffs(*available);
		nct6775_write_value(data, data->REG_TEMP_SOURCE[index], src);
		*available &= ~(1 << index);
		*mask |= 1 << src;
	}
}

static int nct6775_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nct6775_sio_data *sio_data = dev_get_platdata(dev);
	struct nct6775_data *data;
	struct resource *res;
	int i, s, err = 0;
	int src, mask, available;
	const u16 *reg_temp, *reg_temp_over, *reg_temp_hyst, *reg_temp_config;
	const u16 *reg_temp_mon, *reg_temp_alternate, *reg_temp_crit;
	const u16 *reg_temp_crit_l = NULL, *reg_temp_crit_h = NULL;
	int num_reg_temp, num_reg_temp_mon;
	u8 cr2a;
	struct attribute_group *group;
	struct device *hwmon_dev;
	int num_attr_groups = 0;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!devm_request_region(&pdev->dev, res->start, IOREGION_LENGTH,
				 DRVNAME))
		return -EBUSY;

	data = devm_kzalloc(&pdev->dev, sizeof(struct nct6775_data),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->kind = sio_data->kind;
	data->sioreg = sio_data->sioreg;
	data->addr = res->start;
	mutex_init(&data->update_lock);
	data->name = nct6775_device_names[data->kind];
	data->bank = 0xff;		/* Force initial bank selection */
	platform_set_drvdata(pdev, data);

	switch (data->kind) {
	case nct6106:
		data->in_num = 9;
		data->pwm_num = 3;
		data->auto_pwm_num = 4;
		data->temp_fixed_num = 3;
		data->num_temp_alarms = 6;
		data->num_temp_beeps = 6;

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;

		data->temp_label = nct6776_temp_label;
		data->temp_label_num = ARRAY_SIZE(nct6776_temp_label);

		data->REG_VBAT = NCT6106_REG_VBAT;
		data->REG_DIODE = NCT6106_REG_DIODE;
		data->DIODE_MASK = NCT6106_DIODE_MASK;
		data->REG_VIN = NCT6106_REG_IN;
		data->REG_IN_MINMAX[0] = NCT6106_REG_IN_MIN;
		data->REG_IN_MINMAX[1] = NCT6106_REG_IN_MAX;
		data->REG_TARGET = NCT6106_REG_TARGET;
		data->REG_FAN = NCT6106_REG_FAN;
		data->REG_FAN_MODE = NCT6106_REG_FAN_MODE;
		data->REG_FAN_MIN = NCT6106_REG_FAN_MIN;
		data->REG_FAN_PULSES = NCT6106_REG_FAN_PULSES;
		data->FAN_PULSE_SHIFT = NCT6106_FAN_PULSE_SHIFT;
		data->REG_FAN_TIME[0] = NCT6106_REG_FAN_STOP_TIME;
		data->REG_FAN_TIME[1] = NCT6106_REG_FAN_STEP_UP_TIME;
		data->REG_FAN_TIME[2] = NCT6106_REG_FAN_STEP_DOWN_TIME;
		data->REG_TOLERANCE_H = NCT6106_REG_TOLERANCE_H;
		data->REG_PWM[0] = NCT6106_REG_PWM;
		data->REG_PWM[1] = NCT6106_REG_FAN_START_OUTPUT;
		data->REG_PWM[2] = NCT6106_REG_FAN_STOP_OUTPUT;
		data->REG_PWM[5] = NCT6106_REG_WEIGHT_DUTY_STEP;
		data->REG_PWM[6] = NCT6106_REG_WEIGHT_DUTY_BASE;
		data->REG_PWM_READ = NCT6106_REG_PWM_READ;
		data->REG_PWM_MODE = NCT6106_REG_PWM_MODE;
		data->PWM_MODE_MASK = NCT6106_PWM_MODE_MASK;
		data->REG_AUTO_TEMP = NCT6106_REG_AUTO_TEMP;
		data->REG_AUTO_PWM = NCT6106_REG_AUTO_PWM;
		data->REG_CRITICAL_TEMP = NCT6106_REG_CRITICAL_TEMP;
		data->REG_CRITICAL_TEMP_TOLERANCE
		  = NCT6106_REG_CRITICAL_TEMP_TOLERANCE;
		data->REG_CRITICAL_PWM_ENABLE = NCT6106_REG_CRITICAL_PWM_ENABLE;
		data->CRITICAL_PWM_ENABLE_MASK
		  = NCT6106_CRITICAL_PWM_ENABLE_MASK;
		data->REG_CRITICAL_PWM = NCT6106_REG_CRITICAL_PWM;
		data->REG_TEMP_OFFSET = NCT6106_REG_TEMP_OFFSET;
		data->REG_TEMP_SOURCE = NCT6106_REG_TEMP_SOURCE;
		data->REG_TEMP_SEL = NCT6106_REG_TEMP_SEL;
		data->REG_WEIGHT_TEMP_SEL = NCT6106_REG_WEIGHT_TEMP_SEL;
		data->REG_WEIGHT_TEMP[0] = NCT6106_REG_WEIGHT_TEMP_STEP;
		data->REG_WEIGHT_TEMP[1] = NCT6106_REG_WEIGHT_TEMP_STEP_TOL;
		data->REG_WEIGHT_TEMP[2] = NCT6106_REG_WEIGHT_TEMP_BASE;
		data->REG_ALARM = NCT6106_REG_ALARM;
		data->ALARM_BITS = NCT6106_ALARM_BITS;
		data->REG_BEEP = NCT6106_REG_BEEP;
		data->BEEP_BITS = NCT6106_BEEP_BITS;

		reg_temp = NCT6106_REG_TEMP;
		reg_temp_mon = NCT6106_REG_TEMP_MON;
		num_reg_temp = ARRAY_SIZE(NCT6106_REG_TEMP);
		num_reg_temp_mon = ARRAY_SIZE(NCT6106_REG_TEMP_MON);
		reg_temp_over = NCT6106_REG_TEMP_OVER;
		reg_temp_hyst = NCT6106_REG_TEMP_HYST;
		reg_temp_config = NCT6106_REG_TEMP_CONFIG;
		reg_temp_alternate = NCT6106_REG_TEMP_ALTERNATE;
		reg_temp_crit = NCT6106_REG_TEMP_CRIT;
		reg_temp_crit_l = NCT6106_REG_TEMP_CRIT_L;
		reg_temp_crit_h = NCT6106_REG_TEMP_CRIT_H;

		break;
	case nct6775:
		data->in_num = 9;
		data->pwm_num = 3;
		data->auto_pwm_num = 6;
		data->has_fan_div = true;
		data->temp_fixed_num = 3;
		data->num_temp_alarms = 3;
		data->num_temp_beeps = 3;

		data->ALARM_BITS = NCT6775_ALARM_BITS;
		data->BEEP_BITS = NCT6775_BEEP_BITS;

		data->fan_from_reg = fan_from_reg16;
		data->fan_from_reg_min = fan_from_reg8;
		data->target_temp_mask = 0x7f;
		data->tolerance_mask = 0x0f;
		data->speed_tolerance_limit = 15;

		data->temp_label = nct6775_temp_label;
		data->temp_label_num = ARRAY_SIZE(nct6775_temp_label);

		data->REG_CONFIG = NCT6775_REG_CONFIG;
		data->REG_VBAT = NCT6775_REG_VBAT;
		data->REG_DIODE = NCT6775_REG_DIODE;
		data->DIODE_MASK = NCT6775_DIODE_MASK;
		data->REG_VIN = NCT6775_REG_IN;
		data->REG_IN_MINMAX[0] = NCT6775_REG_IN_MIN;
		data->REG_IN_MINMAX[1] = NCT6775_REG_IN_MAX;
		data->REG_TARGET = NCT6775_REG_TARGET;
		data->REG_FAN = NCT6775_REG_FAN;
		data->REG_FAN_MODE = NCT6775_REG_FAN_MODE;
		data->REG_FAN_MIN = NCT6775_REG_FAN_MIN;
		data->REG_FAN_PULSES = NCT6775_REG_FAN_PULSES;
		data->FAN_PULSE_SHIFT = NCT6775_FAN_PULSE_SHIFT;
		data->REG_FAN_TIME[0] = NCT6775_REG_FAN_STOP_TIME;
		data->REG_FAN_TIME[1] = NCT6775_REG_FAN_STEP_UP_TIME;
		data->REG_FAN_TIME[2] = NCT6775_REG_FAN_STEP_DOWN_TIME;
		data->REG_PWM[0] = NCT6775_REG_PWM;
		data->REG_PWM[1] = NCT6775_REG_FAN_START_OUTPUT;
		data->REG_PWM[2] = NCT6775_REG_FAN_STOP_OUTPUT;
		data->REG_PWM[3] = NCT6775_REG_FAN_MAX_OUTPUT;
		data->REG_PWM[4] = NCT6775_REG_FAN_STEP_OUTPUT;
		data->REG_PWM[5] = NCT6775_REG_WEIGHT_DUTY_STEP;
		data->REG_PWM_READ = NCT6775_REG_PWM_READ;
		data->REG_PWM_MODE = NCT6775_REG_PWM_MODE;
		data->PWM_MODE_MASK = NCT6775_PWM_MODE_MASK;
		data->REG_AUTO_TEMP = NCT6775_REG_AUTO_TEMP;
		data->REG_AUTO_PWM = NCT6775_REG_AUTO_PWM;
		data->REG_CRITICAL_TEMP = NCT6775_REG_CRITICAL_TEMP;
		data->REG_CRITICAL_TEMP_TOLERANCE
		  = NCT6775_REG_CRITICAL_TEMP_TOLERANCE;
		data->REG_TEMP_OFFSET = NCT6775_REG_TEMP_OFFSET;
		data->REG_TEMP_SOURCE = NCT6775_REG_TEMP_SOURCE;
		data->REG_TEMP_SEL = NCT6775_REG_TEMP_SEL;
		data->REG_WEIGHT_TEMP_SEL = NCT6775_REG_WEIGHT_TEMP_SEL;
		data->REG_WEIGHT_TEMP[0] = NCT6775_REG_WEIGHT_TEMP_STEP;
		data->REG_WEIGHT_TEMP[1] = NCT6775_REG_WEIGHT_TEMP_STEP_TOL;
		data->REG_WEIGHT_TEMP[2] = NCT6775_REG_WEIGHT_TEMP_BASE;
		data->REG_ALARM = NCT6775_REG_ALARM;
		data->REG_BEEP = NCT6775_REG_BEEP;

		reg_temp = NCT6775_REG_TEMP;
		reg_temp_mon = NCT6775_REG_TEMP_MON;
		num_reg_temp = ARRAY_SIZE(NCT6775_REG_TEMP);
		num_reg_temp_mon = ARRAY_SIZE(NCT6775_REG_TEMP_MON);
		reg_temp_over = NCT6775_REG_TEMP_OVER;
		reg_temp_hyst = NCT6775_REG_TEMP_HYST;
		reg_temp_config = NCT6775_REG_TEMP_CONFIG;
		reg_temp_alternate = NCT6775_REG_TEMP_ALTERNATE;
		reg_temp_crit = NCT6775_REG_TEMP_CRIT;

		break;
	case nct6776:
		data->in_num = 9;
		data->pwm_num = 3;
		data->auto_pwm_num = 4;
		data->has_fan_div = false;
		data->temp_fixed_num = 3;
		data->num_temp_alarms = 3;
		data->num_temp_beeps = 6;

		data->ALARM_BITS = NCT6776_ALARM_BITS;
		data->BEEP_BITS = NCT6776_BEEP_BITS;

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;
		data->target_temp_mask = 0xff;
		data->tolerance_mask = 0x07;
		data->speed_tolerance_limit = 63;

		data->temp_label = nct6776_temp_label;
		data->temp_label_num = ARRAY_SIZE(nct6776_temp_label);

		data->REG_CONFIG = NCT6775_REG_CONFIG;
		data->REG_VBAT = NCT6775_REG_VBAT;
		data->REG_DIODE = NCT6775_REG_DIODE;
		data->DIODE_MASK = NCT6775_DIODE_MASK;
		data->REG_VIN = NCT6775_REG_IN;
		data->REG_IN_MINMAX[0] = NCT6775_REG_IN_MIN;
		data->REG_IN_MINMAX[1] = NCT6775_REG_IN_MAX;
		data->REG_TARGET = NCT6775_REG_TARGET;
		data->REG_FAN = NCT6775_REG_FAN;
		data->REG_FAN_MODE = NCT6775_REG_FAN_MODE;
		data->REG_FAN_MIN = NCT6776_REG_FAN_MIN;
		data->REG_FAN_PULSES = NCT6776_REG_FAN_PULSES;
		data->FAN_PULSE_SHIFT = NCT6775_FAN_PULSE_SHIFT;
		data->REG_FAN_TIME[0] = NCT6775_REG_FAN_STOP_TIME;
		data->REG_FAN_TIME[1] = NCT6776_REG_FAN_STEP_UP_TIME;
		data->REG_FAN_TIME[2] = NCT6776_REG_FAN_STEP_DOWN_TIME;
		data->REG_TOLERANCE_H = NCT6776_REG_TOLERANCE_H;
		data->REG_PWM[0] = NCT6775_REG_PWM;
		data->REG_PWM[1] = NCT6775_REG_FAN_START_OUTPUT;
		data->REG_PWM[2] = NCT6775_REG_FAN_STOP_OUTPUT;
		data->REG_PWM[5] = NCT6775_REG_WEIGHT_DUTY_STEP;
		data->REG_PWM[6] = NCT6776_REG_WEIGHT_DUTY_BASE;
		data->REG_PWM_READ = NCT6775_REG_PWM_READ;
		data->REG_PWM_MODE = NCT6776_REG_PWM_MODE;
		data->PWM_MODE_MASK = NCT6776_PWM_MODE_MASK;
		data->REG_AUTO_TEMP = NCT6775_REG_AUTO_TEMP;
		data->REG_AUTO_PWM = NCT6775_REG_AUTO_PWM;
		data->REG_CRITICAL_TEMP = NCT6775_REG_CRITICAL_TEMP;
		data->REG_CRITICAL_TEMP_TOLERANCE
		  = NCT6775_REG_CRITICAL_TEMP_TOLERANCE;
		data->REG_TEMP_OFFSET = NCT6775_REG_TEMP_OFFSET;
		data->REG_TEMP_SOURCE = NCT6775_REG_TEMP_SOURCE;
		data->REG_TEMP_SEL = NCT6775_REG_TEMP_SEL;
		data->REG_WEIGHT_TEMP_SEL = NCT6775_REG_WEIGHT_TEMP_SEL;
		data->REG_WEIGHT_TEMP[0] = NCT6775_REG_WEIGHT_TEMP_STEP;
		data->REG_WEIGHT_TEMP[1] = NCT6775_REG_WEIGHT_TEMP_STEP_TOL;
		data->REG_WEIGHT_TEMP[2] = NCT6775_REG_WEIGHT_TEMP_BASE;
		data->REG_ALARM = NCT6775_REG_ALARM;
		data->REG_BEEP = NCT6776_REG_BEEP;

		reg_temp = NCT6775_REG_TEMP;
		reg_temp_mon = NCT6775_REG_TEMP_MON;
		num_reg_temp = ARRAY_SIZE(NCT6775_REG_TEMP);
		num_reg_temp_mon = ARRAY_SIZE(NCT6775_REG_TEMP_MON);
		reg_temp_over = NCT6775_REG_TEMP_OVER;
		reg_temp_hyst = NCT6775_REG_TEMP_HYST;
		reg_temp_config = NCT6776_REG_TEMP_CONFIG;
		reg_temp_alternate = NCT6776_REG_TEMP_ALTERNATE;
		reg_temp_crit = NCT6776_REG_TEMP_CRIT;

		break;
	case nct6779:
		data->in_num = 15;
		data->pwm_num = 5;
		data->auto_pwm_num = 4;
		data->has_fan_div = false;
		data->temp_fixed_num = 6;
		data->num_temp_alarms = 2;
		data->num_temp_beeps = 2;

		data->ALARM_BITS = NCT6779_ALARM_BITS;
		data->BEEP_BITS = NCT6779_BEEP_BITS;

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;
		data->target_temp_mask = 0xff;
		data->tolerance_mask = 0x07;
		data->speed_tolerance_limit = 63;

		data->temp_label = nct6779_temp_label;
		data->temp_label_num = NCT6779_NUM_LABELS;

		data->REG_CONFIG = NCT6775_REG_CONFIG;
		data->REG_VBAT = NCT6775_REG_VBAT;
		data->REG_DIODE = NCT6775_REG_DIODE;
		data->DIODE_MASK = NCT6775_DIODE_MASK;
		data->REG_VIN = NCT6779_REG_IN;
		data->REG_IN_MINMAX[0] = NCT6775_REG_IN_MIN;
		data->REG_IN_MINMAX[1] = NCT6775_REG_IN_MAX;
		data->REG_TARGET = NCT6775_REG_TARGET;
		data->REG_FAN = NCT6779_REG_FAN;
		data->REG_FAN_MODE = NCT6775_REG_FAN_MODE;
		data->REG_FAN_MIN = NCT6776_REG_FAN_MIN;
		data->REG_FAN_PULSES = NCT6779_REG_FAN_PULSES;
		data->FAN_PULSE_SHIFT = NCT6775_FAN_PULSE_SHIFT;
		data->REG_FAN_TIME[0] = NCT6775_REG_FAN_STOP_TIME;
		data->REG_FAN_TIME[1] = NCT6776_REG_FAN_STEP_UP_TIME;
		data->REG_FAN_TIME[2] = NCT6776_REG_FAN_STEP_DOWN_TIME;
		data->REG_TOLERANCE_H = NCT6776_REG_TOLERANCE_H;
		data->REG_PWM[0] = NCT6775_REG_PWM;
		data->REG_PWM[1] = NCT6775_REG_FAN_START_OUTPUT;
		data->REG_PWM[2] = NCT6775_REG_FAN_STOP_OUTPUT;
		data->REG_PWM[5] = NCT6775_REG_WEIGHT_DUTY_STEP;
		data->REG_PWM[6] = NCT6776_REG_WEIGHT_DUTY_BASE;
		data->REG_PWM_READ = NCT6775_REG_PWM_READ;
		data->REG_PWM_MODE = NCT6776_REG_PWM_MODE;
		data->PWM_MODE_MASK = NCT6776_PWM_MODE_MASK;
		data->REG_AUTO_TEMP = NCT6775_REG_AUTO_TEMP;
		data->REG_AUTO_PWM = NCT6775_REG_AUTO_PWM;
		data->REG_CRITICAL_TEMP = NCT6775_REG_CRITICAL_TEMP;
		data->REG_CRITICAL_TEMP_TOLERANCE
		  = NCT6775_REG_CRITICAL_TEMP_TOLERANCE;
		data->REG_CRITICAL_PWM_ENABLE = NCT6779_REG_CRITICAL_PWM_ENABLE;
		data->CRITICAL_PWM_ENABLE_MASK
		  = NCT6779_CRITICAL_PWM_ENABLE_MASK;
		data->REG_CRITICAL_PWM = NCT6779_REG_CRITICAL_PWM;
		data->REG_TEMP_OFFSET = NCT6779_REG_TEMP_OFFSET;
		data->REG_TEMP_SOURCE = NCT6775_REG_TEMP_SOURCE;
		data->REG_TEMP_SEL = NCT6775_REG_TEMP_SEL;
		data->REG_WEIGHT_TEMP_SEL = NCT6775_REG_WEIGHT_TEMP_SEL;
		data->REG_WEIGHT_TEMP[0] = NCT6775_REG_WEIGHT_TEMP_STEP;
		data->REG_WEIGHT_TEMP[1] = NCT6775_REG_WEIGHT_TEMP_STEP_TOL;
		data->REG_WEIGHT_TEMP[2] = NCT6775_REG_WEIGHT_TEMP_BASE;
		data->REG_ALARM = NCT6779_REG_ALARM;
		data->REG_BEEP = NCT6776_REG_BEEP;

		reg_temp = NCT6779_REG_TEMP;
		reg_temp_mon = NCT6779_REG_TEMP_MON;
		num_reg_temp = ARRAY_SIZE(NCT6779_REG_TEMP);
		num_reg_temp_mon = ARRAY_SIZE(NCT6779_REG_TEMP_MON);
		reg_temp_over = NCT6779_REG_TEMP_OVER;
		reg_temp_hyst = NCT6779_REG_TEMP_HYST;
		reg_temp_config = NCT6779_REG_TEMP_CONFIG;
		reg_temp_alternate = NCT6779_REG_TEMP_ALTERNATE;
		reg_temp_crit = NCT6779_REG_TEMP_CRIT;

		break;
	case nct6791:
	case nct6792:
	case nct6793:
		data->in_num = 15;
		data->pwm_num = 6;
		data->auto_pwm_num = 4;
		data->has_fan_div = false;
		data->temp_fixed_num = 6;
		data->num_temp_alarms = 2;
		data->num_temp_beeps = 2;

		data->ALARM_BITS = NCT6791_ALARM_BITS;
		data->BEEP_BITS = NCT6779_BEEP_BITS;

		data->fan_from_reg = fan_from_reg13;
		data->fan_from_reg_min = fan_from_reg13;
		data->target_temp_mask = 0xff;
		data->tolerance_mask = 0x07;
		data->speed_tolerance_limit = 63;

		switch (data->kind) {
		default:
		case nct6791:
			data->temp_label = nct6779_temp_label;
			break;
		case nct6792:
			data->temp_label = nct6792_temp_label;
			break;
		case nct6793:
			data->temp_label = nct6793_temp_label;
			break;
		}
		data->temp_label_num = NCT6791_NUM_LABELS;

		data->REG_CONFIG = NCT6775_REG_CONFIG;
		data->REG_VBAT = NCT6775_REG_VBAT;
		data->REG_DIODE = NCT6775_REG_DIODE;
		data->DIODE_MASK = NCT6775_DIODE_MASK;
		data->REG_VIN = NCT6779_REG_IN;
		data->REG_IN_MINMAX[0] = NCT6775_REG_IN_MIN;
		data->REG_IN_MINMAX[1] = NCT6775_REG_IN_MAX;
		data->REG_TARGET = NCT6775_REG_TARGET;
		data->REG_FAN = NCT6779_REG_FAN;
		data->REG_FAN_MODE = NCT6775_REG_FAN_MODE;
		data->REG_FAN_MIN = NCT6776_REG_FAN_MIN;
		data->REG_FAN_PULSES = NCT6779_REG_FAN_PULSES;
		data->FAN_PULSE_SHIFT = NCT6775_FAN_PULSE_SHIFT;
		data->REG_FAN_TIME[0] = NCT6775_REG_FAN_STOP_TIME;
		data->REG_FAN_TIME[1] = NCT6776_REG_FAN_STEP_UP_TIME;
		data->REG_FAN_TIME[2] = NCT6776_REG_FAN_STEP_DOWN_TIME;
		data->REG_TOLERANCE_H = NCT6776_REG_TOLERANCE_H;
		data->REG_PWM[0] = NCT6775_REG_PWM;
		data->REG_PWM[1] = NCT6775_REG_FAN_START_OUTPUT;
		data->REG_PWM[2] = NCT6775_REG_FAN_STOP_OUTPUT;
		data->REG_PWM[5] = NCT6791_REG_WEIGHT_DUTY_STEP;
		data->REG_PWM[6] = NCT6791_REG_WEIGHT_DUTY_BASE;
		data->REG_PWM_READ = NCT6775_REG_PWM_READ;
		data->REG_PWM_MODE = NCT6776_REG_PWM_MODE;
		data->PWM_MODE_MASK = NCT6776_PWM_MODE_MASK;
		data->REG_AUTO_TEMP = NCT6775_REG_AUTO_TEMP;
		data->REG_AUTO_PWM = NCT6775_REG_AUTO_PWM;
		data->REG_CRITICAL_TEMP = NCT6775_REG_CRITICAL_TEMP;
		data->REG_CRITICAL_TEMP_TOLERANCE
		  = NCT6775_REG_CRITICAL_TEMP_TOLERANCE;
		data->REG_C