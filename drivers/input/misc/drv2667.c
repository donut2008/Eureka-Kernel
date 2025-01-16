inary crc*/
		input_info(true, &info->client->dev,
				"%s: efs:%d, bin:%d\n",
				__func__, crc_efs, info->light_table_crc);
		if (crc_efs != info->light_table_crc)
			return false;
	}

	return true;
}

static int efs_write_light_table_crc(struct abov_tk_info *info, int crc_cal)
{
	struct file *temp_file = NULL;
	char crc[LIGHT_CRC_SIZE] = {0, };
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_CRC_PATH, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
		input_info(true, &info->client->dev,
				"%s: failed to open efs file %d\n", __func__, ret);
	} else {
		snprintf(crc, sizeof(crc), "%d", crc_cal);
		temp_file->f_op->write(temp_file, crc, sizeof(crc), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev, "%s: %s\n", __func__, crc);
	}
	set_fs(old_fs);
	return ret;
}

static int efs_write_light_table_version(struct abov_tk_info *info, char *full_version)
{
	struct file *temp_file = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_VERSION_PATH, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(temp_file)) {
		ret = -ENOENT;
	} else {
		temp_file->f_op->write(temp_file, full_version,
				LIGHT_VERSION_LEN, &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev, "%s: version = %s\n",
				__func__, full_version);
	}
	set_fs(old_fs);
	return ret;
}

static int efs_write_light_table(struct abov_tk_info *info, struct light_info table)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	char led_reg[LIGHT_DATA_SIZE] = {0, };

	snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN,
			"%s%d", LIGHT_TABLE_PATH, table.octa_id);
	snprintf(led_reg, sizeof(led_reg), "%d", table.led_reg);

	input_info(true, &info->client->dev, "%s: make %s\n", __func__, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path, O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		input_err(true, &info->client->dev, "%s: open fail :%d\n",
			__func__, ret);
		return ret;
	}

	type_filp->f_op->write(type_filp, led_reg, sizeof(led_reg), &type_filp->f_pos);
	filp_close(type_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int efs_write(struct abov_tk_info *info)
{
	int ret = 0;
	int i, crc_cal;

	ret = efs_write_light_table_version(info, info->light_version_full_bin);
	if (ret < 0)
		return ret;
	info->light_version_efs = info->pdata->dt_light_version;

	for (i = 0; i < info->pdata->dt_light_table; i++) {
		ret = efs_write_light_table(info, tkey_light_reg_table[i]);
		if (ret < 0)
			break;
	}
	if (ret < 0)
		return ret;

	crc_cal = efs_calculate_crc(info);
	if (crc_cal < 0)
		return crc_cal;

	ret = efs_write_light_table_crc(info, crc_cal);
	if (ret < 0)
		return ret;

	if (!check_light_table_crc(info))
		ret = -EIO;

	return ret;
}

static int pick_light_table_version(char* str)
{
	static char* str_addr;
	char* token = NULL;
	int ret = 0;
	
	if (str != NULL)
		str_addr = str;
	else if (str_addr == NULL)
		return 0;

	token = str_addr;
	while (true) {
		if (!(*str_addr)) {
			break;
 		} else if (*str_addr == 'T') {
			*str_addr = '0';
		} else if (*str_addr == '.') {
			*str_addr = '\0';
			str_addr = str_addr + 1;
			break;
		}
		str_addr++;
	}

	if (kstrtoint(token + 1, 0, &ret) < 0)
		return 0;

	return ret;
}

static int efs_read_light_table_version(struct abov_tk_info *info)
{
	struct file *temp_file = NULL;
	char version[LIGHT_VERSION_LEN] = {0, };
	mm_segment_t old_fs;
	int ret = 0;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	temp_file = filp_open(LIGHT_VERSION_PATH, O_RDONLY, 0440);
	if (IS_ERR(temp_file)) {
		ret = PTR_ERR(temp_file);
	} else {
		temp_file->f_op->read(temp_file, version, sizeof(version), &temp_file->f_pos);
		filp_close(temp_file, current->files);
		input_info(true, &info->client->dev,
				"%s: table full version = %s\n", __func__, version);
		snprintf(info->light_version_full_efs,
				sizeof(info->light_version_full_efs), version);
		info->light_version_efs = pick_light_table_version(version);
		input_info(true, &info->client->dev,
				"%s: table version = %d\n", __func__, info->light_version_efs);
	}
	set_fs(old_fs);

	return ret;
}

static int efs_read_light_table(struct abov_tk_info *info, int octa_id)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	char predefine_value_path[LIGHT_TABLE_PATH_LEN];
	char led_reg[LIGHT_DATA_SIZE] = {0, };
	int ret;

	snprintf(predefine_value_path, LIGHT_TABLE_PATH_LEN,
		"%s%d", LIGHT_TABLE_PATH, octa_id);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path, O_RDONLY, 0440);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		input_err(true, &info->client->dev,
				"%s: fail to open light data %d\n", __func__, ret);
		return ret;
	}

	type_filp->f_op->read(type_filp, led_reg, sizeof(led_reg), &type_filp->f_pos);
	filp_close(type_filp, current->files);
	set_fs(old_fs);

	if (kstrtoint(led_reg, 0, &ret) < 0)
		return -EIO;

	return ret;
}

static int efs_read_light_table_with_default(struct abov_tk_info *info, int octa_id)
{
	bool set_default = false;
	int ret;

retry:
	if (set_default)
		octa_id = WINDOW_COLOR_DEFAULT;

	ret = efs_read_light_table(info, octa_id);
	if (ret < 0) {
		if (!set_default) {
			set_default = true;
			goto retry;
		}
	}

	return ret;
}


static bool need_update_light_table(struct abov_tk_info *info)
{
	/* Check version file exist*/
	if (efs_read_light_table_version(info) < 0) {
		return true;
	}

	/* Compare version */
	input_info(true, &info->client->dev,
			"%s: efs:%d, bin:%d\n", __func__,
			info->light_version_efs, info->pdata->dt_light_version);
	if (info->light_version_efs < info->pdata->dt_light_version)
		return true;

	/* Check CRC */
	if (!check_light_table_crc(info)) {
		input_info(true, &info->client->dev,
				"%s: crc is diffrent\n", __func__);
		return true;
	}

	return false;
}

static void touchkey_efs_open_work(struct work_struct *work)
{
	struct abov_tk_info *info =
			container_of(work, struct abov_tk_info, efs_open_work.work);
	int window_type;
	static int count = 0;
	int led_reg;

	if (need_update_light_table(info)) {
		if (efs_write(info) < 0)
			goto out;
	}

	window_type = read_window_type();
	if (window_type < 0)
		goto out;

	led_reg = efs_read_light_table_with_default(info, window_type);
	if ((led_reg >= LIGHT_REG_MIN_VAL) && (led_reg <= LIGHT_REG_MAX_VAL)) {
		change_touch_key_led_brightness(&info->client->dev, led_reg);
		input_info(true, &info->client->dev,
				"%s: read done for window_type=%d\n", __func__, window_type);
	} else {
		input_err(true, &info->client->dev,
				"%s: fail. key led brightness reg is %d\n", __func__, led_reg);
	}
	return;

out:
	if (count < 50) {
		schedule_delayed_work(&info->efs_open_work, msecs_to_jiffies(2000));
		count++;
 	} else {
		input_err(true, &info->client->dev,
				"%s: retry %d times but can't check efs\n", __func__, count);
 	}
}
#endif

#ifdef CONFIG_TOUCHKEY_GRIP
static void abov_sar_only_mode(struct abov_tk_info *info, int on)
{
	struct i2c_client *client = info->client;
	int retry =3;
	int ret;
	u8 cmd;
	u8 r_buf;
	int mode_retry = 5;

	if(info->sar_mode == on){
		input_info(true, &client->dev, "[TK] %s : skip already %s\n", __func__, (on==1)? "sar only mode":"normal mode");
		return;
	}

	if(on == 1)	cmd = 0x20;
	else	cmd = 0x10;

	input_info(true, &client->dev, "[TK] %s : %s, cmd=%x\n", __func__, (on==1)? "sar only mode":"normal mode", cmd);
sar_mode:
	while(retry>0){
		ret = abov_tk_i2c_write(info->client, CMD_SAR_MODE, &cmd, 1);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s fail(%d), retry %d\n", __func__, ret, retry);
			retry--;
			abov_delay(20);
			continue;
		}
		break;
	}

	abov_delay(40);

	ret = abov_tk_i2c_read(info->client, CMD_SAR_MODE, &r_buf, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}
	input_info(true, &client->dev, "%s read reg = %x\n", __func__,r_buf);

	if((r_buf != cmd) && (mode_retry > 0)){
		input_info(true, &info->client->dev, "%s change fail retry\n", __func__);
		mode_retry--;

		if(mode_retry == 0){
			abov_tk_reset(info);
		}
		goto sar_mode;
	}

	if(r_buf == 0x20)
		info->sar_mode = 1;
	else if(r_buf == 0x10)
		info->sar_mode = 0;
}

static void touchkey_sar_sensing(struct abov_tk_info *info, int on)
{
	struct i2c_client *client = info->client;
	int ret;
	u8 cmd;

	if(on==1)	cmd = CMD_ON;
	else	cmd = CMD_OFF;

	input_info(true, &client->dev, "[TK] %s : %s\n", __func__, (on)? "on":"off");

	ret = abov_tk_i2c_write(info->client, CMD_SAR_SENSING, &cmd, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s fail(%d)\n", __func__, ret);
	}
	info->sar_sensing = on;
}
#endif

static void release_all_fingers(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
	int i;

	input_info(true, &client->dev, "%s called (touchkey_count=%d)\n", __func__,info->touchkey_count);

	for (i = 1; i < info->touchkey_count; i++) {
		input_report_key(info->input_dev,
			touchkey_keycode[i], 0);
	}
	input_sync(info->input_dev);
}

static int abov_tk_reset_for_bootmode(struct abov_tk_info *info)
{
	int ret=0;

	info->pdata->power(info, false);
	abov_delay(50);
	info->pdata->power(info, true);

	return ret;

}

static void abov_tk_reset(struct abov_tk_info *info)
{
	struct i2c_client *client = info->client;
#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	int ret;
#endif

	if (info->enabled == false)
		return;

	input_info(true,&client->dev, "%s start\n", __func__);
	disable_irq_nosync(info->irq);

	info->enabled = false;

	release_all_fingers(info);

	abov_tk_reset_for_bootmode(info);
	abov_delay(ABOV_RESET_DELAY);

#ifdef CONFIG_TOUCHKEY_LIGHT_EFS
	/*led dimming */
	ret = abov_tk_i2c_write(info->client, ABOV_LED_BACK, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming back key write fail(%d)\n", __func__, ret);
	}

	ret = abov_tk_i2c_write(info->client, ABOV_LED_RECENT, &info->light_reg, 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s led dimming recent key write fail(%d)\n", __func__, ret);
	}
#endif
#ifdef CONFIG_TOUCHKEY_GRIP
	if(info->sar_enable)
		abov_mode_enable(client, CMD_SAR_ENABLE, CMD_ON);

	if(info->sar_sensing != 1)
		touchkey_sar_sensing(info, 0);
#else
	if (info->pdata->ta_notifier && g_ta_connected) {
		abov_set_ta_status(info);
	}
	if (info->flip_mode){
		abov_mode_enable(client, ABOV_FLIP, CMD_FLIP_ON);
	} else {
		if (info->glovemode)
			abov_mode_enable(client, ABOV_GLOVE, CMD_GLOVE_ON);
	}
	if (info->keyboard_mode)
		abov_mode_enable(client, ABOV_KEYBOARD, CMD_MOBILE_KBD_ON);
#endif
	info->enabled = true;

	enable_irq(info->irq);
	input_info(true,&client->dev, "%s end\n", __func__);
}

static irqreturn_t abov_tk_interrupt(int irq, void *dev_id)
{
	struct abov_tk_info *info = dev_id;
	struct i2c_client *client = info->client;
	int ret, retry;
	u8 buf;
#ifdef CONFIG_TOUCHKEY_GRIP
	int grip_data;
	u8 grip_press = 0;

	wake_lock(&info->touckey_wake_lock);
#endif

	ret = abov_tk_i2c_read(client, ABOV_BTNSTATUS_NEW, &buf, 1);
	if (ret < 0) {
		retry = 3;
		while (retry--) {
			input_err(true, &client->dev, "%s read fail(%d)\n",
				__func__, retry);
			ret = abov_tk_i2c_read(client, ABOV_BTNSTATUS_NEW, &buf, 1);
			if (ret == 0)
				break;
			else
				abov_delay(10);
		}
		if (retry == 0) {
			abov_tk_reset(info);
#ifdef CONFIG_TOUCHKEY_GRIP
			wake_unlock(&info->touckey_wake_lock);
#endif
			return IRQ_HANDLED;
		}
	}

	input_info(true, &client->dev, " %s buf = 0x%02x\n",__func__, buf);

	{
		int menu_data = buf & 0x03;
		int back_data = (buf >> 2) & 0x03;
		u8 menu_press = !(menu_data % 2);
		u8 back_press = !(back_data % 2);
#ifdef CONFIG_TOUCHKEY_GRIP
		grip_data = (buf >> 4) & 0x03;
		grip_press = !(grip_data % 2);
#endif
		if (menu_data)
			input_report_key(info->input_dev,
				touchkey_keycode[1], menu_press);
		if (back_data)
			input_report_key(info->input_dev,
				touchkey_keycode[2], back_press);
#ifdef CONFIG_TOUCHKEY_GRIP
		if (grip_data){
			input_report_key(info->input_dev,
				touchkey_keycode[3], grip_press);
			info->grip_event =  grip_press;
		}
#endif
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &client->dev,
			"keycode : %s%s ver 0x%02x\n",
			menu_data ? (menu_press ? "P" : "R") : "",
			back_data ? (bac