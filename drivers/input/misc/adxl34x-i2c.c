ut_dev_proximity);
				}
				break;
			case G_TYPE_SINGLETAP:
				if (g_msg.b.gid == G_ID_SINGLETAP) {
					data->scrub_id = SPONGE_EVENT_TYPE_SINGLETAP;
					data->scrub_x = (g_msg.b.gdata[0] << 4) |
									((g_msg.b.gdata[2] >> 4) & 0xF);
					data->scrub_y = (g_msg.b.gdata[1] << 4) |
									(g_msg.b.gdata[2] & 0xF);
					data->all_singletap_count++;

					input_info(true, &data->client->dev,
						   "Single Tap Trigger (%d, %d)\n",
						   data->scrub_x, data->scrub_y);

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
			case G_TYPE_PRESSURE:
				break;
			case G_TYPE_PRESS:
				if (g_msg.b.gid == G_ID_FOD_LONG || g_msg.b.gid == G_ID_FOD_NORMAL) {
					data->scrub_id = SPONGE_EVENT_TYPE_FOD;
					input_info(true, &data->client->dev,
					   "FOD %s\n", g_msg.b.gid ? "normal" : "long");
				} else if (g_msg.b.gid == G_ID_FOD_RELEASE) {
					data->scrub_id = SPONGE_EVENT_TYPE_FOD_RELEASE;
					input_info(true, &data->client->dev,
					   "FOD release\n");
				} else if (g_msg.b.gid == G_ID_FOD_OUT) {
					data->scrub_id = SPONGE_EVENT_TYPE_FOD_OUT;
					input_info(true, &data->client->dev,
						"FOD out\n");
				}
				input_report_key(data->input_dev,
						 KEY_BLACK_UI_GESTURE,
						 true);
				input_sync(data->input_dev);
				input_report_key(data->input_dev,
						 KEY_BLACK_UI_GESTURE,
						 false);
				input_sync(data->input_dev);
				break;
			default:
				input_err(true, &data->client->dev,
					  "Not support gesture type\n");
				break;
			}
		} else {
			input_err(true, &data->client->dev, "Not Gesture Msg\n");
		}

		return;
	}

	input_err(true, &data->client->dev, "Not support gesture cmd: 0x%02X\n", cmd);
}

static void location_detect(struct ist40xx_data *data, char *loc, int x, int y)
{
	if (x < data->dt_data->area_edge)
		strncat(loc, "E.", 2);
	else if (x < (data->tsp_info.width - data->dt_data->area_edge))
		strncat(loc, "C.", 2);
	else
		strncat(loc, "e.", 2);

	if (y < data->dt_data->area_indicator)
		strncat(loc, "S", 1);
	else if (y < (data->tsp_info.height - data->dt_data->area_navigation))
		strncat(loc, "C", 1);
	else
		strncat(loc, "N", 1);
}

static void release_finger(struct ist40xx_data *data, int id)
{
	input_mt_slot(data->input_dev, id);
	input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
	input_report_key(data->input_dev, BTN_TOOL_FINGER, 0);
	input_sync(data->input_dev);

	input_info(true, &data->client->dev, "forced touch release: %d\n", id);

	data->tsp_touched[id] = false;
}

void clear_input_data(struct ist40xx_data *data)
{
	int i = 0;

	for (i = 0; i < IST40XX_MAX_FINGER_ID; i++) {
		if (data->tsp_touched[i] == true)
			release_finger(data, i);
	}

	data->touch_pressed_num = 0;
	data->check_multi = 0;
}

#define TOUCH_DOWN_MESSAGE		("p")
#define TOUCH_UP_MESSAGE		("r")
#define TOUCH_MOVE_MESSAGE		(" ")
static void report_input_data(struct ist40xx_data *data)
{
	int ret;
	int i, id;
	int idx = 0;
	bool press = false;
	finger_info *fingers = (finger_info *)data->fingers;
	u32 status;
	u8 mode = TOUCH_STATUS_NORMAL_MODE;
	char location[4] = { 0 };

	ret = ist40xx_read_reg(data->client, IST40XX_HIB_TOUCH_STATUS, &status);
	if (!ret) {
		if ((status & TOUCH_STATUS_MASK) == TOUCH_STATUS_MAGIC) {
			if (GET_NOISE_MODE(status))
				mode |= TOUCH_STATUS_NOISE_MODE;
			if (GET_WET_MODE(status))
				mode |= TOUCH_STATUS_WET_MODE;
		}
	}

	for (i = 0; i < IST40XX_MAX_FINGER_ID; i++) {
		memset(location, 0, sizeof(location));
		id = fingers[idx].b.id - 1;
		if (i == id) {
			if (fingers[idx].b.status == TOUCH_STA_NONE) {
				idx++;
				continue;
			} else if (fingers[idx].b.status == TOUCH_STA_CSV) {
				// No Report CSV Touch
				if (data->tsp_touched[id] == true) {
					data->move_count[id]++;
					/*for getting coordinate of the last point of move event*/
					data->r_x[id] = fingers[idx].b.x;
					data->r_y[id] = f