data->lock);

	ist40xx_print_info(data);

	ret = ist40xx_set_input_device(data);
	if (ret)
		goto err_get_info;

	ist40xx_read_cmd(data, eHCOM_GET_CAL_RESULT_S, &calib_msg);
	input_info(true, &data->client->dev, "SLF calib result: 0x%08X\n", calib_msg);
	ist40xx_read_cmd(data, eHCOM_GET_CAL_RESULT_M, &calib_msg);
	input_info(true, &data->client->dev, "MTL calib result: 0x%08X\n", calib_msg);
	ist40xx_read_cmd(data, eHCOM_GET_CAL_RESULT_P, &calib_msg);
	input_info(true, &data->client->dev, "PROX calib result: 0x%08X\n", calib_msg);

	ret = ist40xx_write_cmd(data, IST40XX_HIB_CMD,
			(eHCOM_FW_HOLD << 16) | (IST40XX_DISABLE & 0xFFFF));
	if (ret) {
		input_err(true, &data->client->dev, "%s: fail to disable hold\n",
			   __func__);
		mutex_lock(&data->lock);
		ist40xx_reset(data, false);
		mutex_unlock(&data->lock);
	}

	ist40xx_enable_irq(data);

	if (data->dt_data->support_fod) {
		ist40xx_read_sponge_reg(data, IST40XX_SPONGE_FOD_INFO,
						(u16*)&fod_info, 2, true);

		data->fod_tx = fod_info[0];
		data->fod_rx = fod_info[1];
		data->fod_vi_size = fod_info[2];

		input_info(true, &data->client->dev, "%s fod_tx[%d] fod_rx[%d] fod_vi_size[%d]\n",
					__func__, data->fod_tx, data->fod_rx, data->fod_vi_size); 
	}
	
	return 0;

err_get_info:

	return ret;
}

#define SPECIAL_MAGIC_STRING		(0x4170CF00)
#define SPECIAL_MAGIC_MASK		(0xFFFFFF00)
#define SPECIAL_MESSAGE_MASK		(~SPECIAL_MAGIC_MASK)
#define PARSE_SPECIAL_MESSAGE(n)	((n & SPECIAL_MAGIC_MASK) == SPECIAL_MAGIC_STRING ? \
						(n & SPECIAL_MESSAGE_MASK) : -EINVAL)
#define SPECIAL_START_MASK		(0x80)
#define SPECIAL_SUCCESS_MASK		(0x0F)
void ist40xx_special_cmd(struct ist40xx_data *data, int cmd)
{
	gesture_msg g_msg;

	if (cmd == 0) {
		ist40xx_burst_read(data->client, IST40XX_HIB_GESTURE_MSG, g_msg.full,
				sizeof(g_msg.full) / IST40XX_DATA_LEN, true);

		input_info(true, &data->client->dev, "g_msg %d, %d, %d\n",
					g_msg.b.eid, g_msg.b.gtype, g_msg.b.gid);

		if (g_msg.b.eid == EID_GESTURE) {
			switch (g_msg.b.gtype) {
			case G_TYPE_SWIPE:
				if (g_msg.b.gid == G_ID_SWIPE_UP) {
					data->scrub_id = SPONGE_EVENT_TYPE_SPAY;
					data->scrub_x = 0;
					data->scrub_y = 0;
					data->all_spay_count++;

					input_info(true, &data->client->dev, "Swipe Up Trigger\n");

					input_report_key(data->input_dev,
							 KEY_BLACK_UI_GESTURE,
							 true);
					input_sync(data->input_dev);
					input_report_key(data->input_dev,
							 KEY_BLACK_UI_GESTURE,
							 false);
					input_sync(data->input_dev);
				}
				break;
			case G_TYPE_DOUBLETAP:
				if (g_msg.b.gid == G_ID_AOD_DOUBLETAP) {
					data->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;
					data->scrub_x = (g_msg.b.gdata[0] << 4) |
									((g_msg.b.gdata[2] >> 4) & 0xF);
					data->scrub_y = (g_msg.b.gdata[1] << 4) |
									(g_msg.b.gdata[2] & 0xF);
					data->all_aod_tsp_count++;

					input_info(true, &data->client->dev,
						   "Double Tap Trigger (%d, %d)\n",
						   data->scrub_x, data->scrub_y);

					input_report_key(data->input_dev,
							 KEY_BLACK_UI_GESTURE,
							 true);
					input_sync(data->input_dev);
					input_report_key(data->input_dev,
							 KEY_BLACK_U