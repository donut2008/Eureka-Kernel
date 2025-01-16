 = (device_tree_infor + device_count);
			const u32 *plevels = NULL;

			// Geting label.
			dt_infor->label = of_get_property(cnp, "input_booster,label", NULL);
			printk("[Input Booster] %s   dt_infor->label : %s\n", __FUNCTION__, dt_infor->label);

			if (of_property_read_u32(cnp, "input_booster,type", &dt_infor->type)) {
				printk("Failed to get type property\n");
				break;
			}

			// Geting the count of levels.
			plevels = of_get_property(cnp, "input_booster,levels", &nlevels);

			if (plevels && nlevels) {
				dt_infor->nlevels = nlevels / sizeof(u32);
				printk("[Input Booster] %s   dt_infor->nlevels : %d\n", __FUNCTION__, dt_infor->nlevels);
			} else {
				printk("Failed to calculate number of frequency.\n");
				break;
			}

			// Allocation the param table.
			dt_infor->param_tables = kcalloc(ABS_CNT, sizeof(struct t_input_booster_device_tree_param) * dt_infor->nlevels, GFP_KERNEL);
			if (!dt_infor->param_tables) {
				printk("Failed to allocate memory of freq_table\n");
				break;
			}

			// fill the param table
			pr_debug("[Input Booster] device_type:%d, label :%s, type: 0x%02x, num_level[%d]\n",
				dt_infor->type, dt_infor->label, dt_infor->type, dt_infor->nlevels);

			for (i = 0; i < dt_infor->nlevels; i++) {
				u32 temp;
				int err = 0;

				err = of_property_read_u32_index(cnp, "input_booster,levels", i, &temp);  dt_infor->param_tables[i].ilevels = (u8)temp;
				err |= of_property_read_u32_index(cnp, "input_booster,cpu_freqs", i, &dt_infor->param_tables[i].cpu_freq);
				err |= of_property_read_u32_index(cnp, "input_booster,kfc_freqs", i, &dt_infor->param_tables[i].kfc_freq);
				err |= of_property_read_u32_index(cnp, "input_booster,mif_freqs", i, &dt_infor->param_tables[i].mif_freq);
				err |= of_property_read_u32_index(cnp, "input_booster,int_freqs", i, &dt_infor->param_tables[i].int_freq);
				err |= of_property_read_u32_index(cnp, "input_booster,hmp_boost", i, &temp); dt_infor->param_tables[i].hmp_boost = (u8)temp;
				err |= of_property_read_u32_index(cnp, "input_booster,head_times", i, &temp); dt_infor->param_tables[i].head_time = (u16)temp;
				err |= of_property_read_u32_index(cnp, "input_booster,tail_times", i, &temp); dt_infor->param_tables[i].tail_time = (u16)temp;
				err |= of_property_read_u32_index(cnp, "input_booster,phase_times", i, &temp); dt_infor->param_tables[i].phase_time = (u16)temp;
				if (err) {
					printk("Failed to get [%d] param table property\n", i);
				}

				printk("[Input Booster] Level %d : frequency[%d,%d,%d,%d] hmp_boost[%d] times[%d,%d,%d]\n", i,
					dt_infor->param_tables[i].cpu_freq,
					dt_infor->param_tables[i].kfc_freq,
					dt_infor->param_tables[i].mif_freq,
					dt_infor->param_tables[i].int_freq,
					dt_infor->param_tables[i].hmp_boost,
					dt_infor->param_tables[i].head_time,
					dt_infor->param_tables[i].tail_time,
					dt_infor->param_tables[i].phase_time);
			}

			device_count++;
		}
	}

	// ********** Initialize Buffer for Touch **********
	for(i=0;i<MAX_MULTI_TOUCH_EVENTS;i++) {
		TouchIDs[i] = -1;
	}

	// ********** Initialize Booster **********
	INIT_BOOSTER(touch)
	INIT_BOOSTER(multitouch)
	INIT_BOOSTER(key)
	INIT_BOOSTER(touchkey)
	INIT_BOOSTER(keyboard)
	INIT_BOOSTER(mouse)
	INIT_BOOSTER(mouse_wheel)
	INIT_BOOSTER(pen)
	INIT_BOOSTER(hover)
	INIT_BOOSTER(key_two)
	multitouch_booster.change_on_release = 1;

	// ********** Initialize Sysfs **********
	{
		struct class *sysfs_class;

		sysfs_class = class_create(THIS_MODULE, "input_booster");
		if (IS_ERR(sysfs_class)) {
			printk("[Input Booster] Failed to create class\n");
			return;
		}

		INIT_SYSFS_CLASS(debug_level)
		INIT_SYSFS_CLASS(head)
		INIT_SYSFS_CLASS(tail)
		INIT_SYSFS_CLASS(level)

		INIT_SYSFS_DEVICE(touch)
		INIT_SYSFS_DEVICE(multitouch)
		INIT_SYSFS_DEVICE(key)
		INIT_SYSFS_DEVICE(touchkey)
		INIT_SYSFS_DEVICE(keyboard)
		INIT_SYSFS_DEVICE(mouse)
		INIT_SYSFS_DEVICE(mouse_wheel)
		INIT_SYSFS_DEVICE(pen)
		INIT_SYSFS_DEVICE(hover)
		INIT_SYSFS_DEVICE(key_two)
	}
}
#endif  // Input Booster -

/**
 * input_event() - report new input event
 * @dev: device that generated the event
 * @type: type of the event
 * @code: event code
 * @value: value of the event
 *
 * This function should be used by drivers implementing various input
 * devices to report input events. See also input_inject_event().
 *
 * NOTE: input_event() may be safely used right after input device was
 * allocated with input_allocate_device(), even before it is registered
 * with input_register_device(), but the event will not reach any of the
 * input handlers. Such early invocation of input_event() may be used
 * to 'seed' initial state of a switch or initial position of absolute
 * axis, etc.
 */
void input_event(struct input_dev *dev,
		 unsigned int type, unsigned int code, int value)
{
	unsigned long flags;
	int idx;

	if (is_event_supported(type, dev->evbit, EV_MAX)) {

		spin_lock_irqsave(&dev->event_lock, flags);
		input_handle_event(dev, type, code, value);
		spin_unlock_irqrestore(&dev->event_lock, flags);

#if !defined(CONFIG_INPUT_BOOSTER) // Input Booster +
		if(device_tree_infor != NULL) {
			if (type == EV_SYN && input_count > 0) {
				pr_debug("[Input Booster1] ==============================================\n");
				input_booster(dev);
				input_count=0;
			} else if (input_count < MAX_EVENTS) {
				pr_debug("[Input Booster1] type = %x, code = %x, value =%x\n", type, code, value);
				idx = input_count;
				input_events[idx].type = type;
				input_events[idx].code = code;
				input_events[idx].value = value;
				if (idx < MAX_EVENTS) {
					input_count = idx + 1 ;
				}
			} else {
				pr_debug("[Input Booster1] type = %x, code = %x, value =%x   Booster Event Exceeded\n", type, code, value);
			}
		}
#endif  // Input Booster -
	}
}
EXPORT_SYMBOL(input_event);

/**
 * input_inject_event() - send input event from input handler
 * @handle: input handle to send event through
 * @type: type of the event
 * @code: event code
 * @value: value of the event
 *
 * Similar to input_event() but will ignore event if device is
 * "grabbed" and handle injecting event is not the one that owns
 * the device.
 */
void input_inject_event(struct input_handle *handle,
			unsigned int type, unsigned int code, int value)
{
	struct input_dev *dev = handle->dev;
	struct input_handle *grab;
	unsigned long flags;

	if (is_event_supported(type, dev->evbit, EV_MAX)) {
		spin_lock_irqsave(&dev->event_lock, flags);

		rcu_read_lock();
		grab = rcu_dereference(dev->grab);
		if (!grab || grab == handle)
			input_handle_event(dev, type, code, value);
		rcu_read_unlock();

		spin_unlock_irqre