empty = false;
			if (i > 0)
				len += snprintf(buf + len, max(buf_size - len, 0), " ");
		}
	}

	/*
	 * If no output was produced print a single 0.
	 */
	if (len == 0)
		len = snprintf(buf, buf_size, "%d", 0);

	if (add_cr)
		len += snprintf(buf + len, max(buf_size - len, 0), "\n");

	return len;
}

#define INPUT_DEV_CAP_ATTR(ev, bm)					\
static ssize_t input_dev_show_cap_##bm(struct device *dev,		\
				       struct device_attribute *attr,	\
				       char *buf)			\
{									\
	struct input_dev *input_dev = to_input_dev(dev);		\
	int len = input_print_bitmap(buf, PAGE_SIZE,			\
				     input_dev->bm##bit, ev##_MAX,	\
				     true);				\
	return min_t(int, len, PAGE_SIZE);				\
}									\
static DEVICE_ATTR(bm, S_IRUGO, input_dev_show_cap_##bm, NULL)

INPUT_DEV_CAP_ATTR(EV, ev);
INPUT_DEV_CAP_ATTR(KEY, key);
INPUT_DEV_CAP_ATTR(REL, rel);
INPUT_DEV_CAP_ATTR(ABS, abs);
INPUT_DEV_CAP_ATTR(MSC, msc);
INPUT_DEV_CAP_ATTR(LED, led);
INPUT_DEV_CAP_ATTR(SND, snd);
INPUT_DEV_CAP_ATTR(FF, ff);
INPUT_DEV_CAP_ATTR(SW, sw);

static struct attribute *input_dev_caps_attrs[] = {
	&dev_attr_ev.attr,
	&dev_attr_key.attr,
	&dev_attr_rel.attr,
	&dev_attr_abs.attr,
	&dev_attr_msc.attr,
	&dev_attr_led.attr,
	&dev_attr_snd.attr,
	&dev_attr_ff.attr,
	&dev_attr_sw.attr,
	NULL
};

static struct attribute_group input_dev_caps_attr_group = {
	.name	= "capabilities",
	.attrs	= input_dev_caps_attrs,
};

static const struct attribute_group *input_dev_attr_groups[] = {
	&input_dev_attr_group,
	&input_dev_id_attr_group,
	&input_dev_caps_attr_group,
	NULL
};

static void input_dev_release(struct device *device)
{
	struct input_dev *dev = to_input_dev(device);

	input_ff_destroy(dev);
	input_mt_destroy_slots(dev);
	kfree(dev->absinfo);
	kfree(dev->vals);
	kfree(dev);

	module_put(THIS_MODULE);
}

/*
 * Input uevent interface - loading event handlers based on
 * device bitfields.
 */
static int input_add_uevent_bm_var(struct kobj_uevent_env *env,
				   const char *name, unsigned long *bitmap, int max)
{
	int len;

	if (add_uevent_var(env, "%s", name))
		return -ENOMEM;

	len = input_print_bitmap(&env->buf[env->buflen - 1],
				 sizeof(env->buf) - env->buflen,
				 bitmap, max, false);
	if (len >= (sizeof(env->buf) - env->buflen))
		return -ENOMEM;

	env->buflen += len;
	return 0;
}

static int input_add_uevent_modalias_var(struct kobj_uevent_env *env,
					 struct input_dev *dev)
{
	int len;

	if (add_uevent_var(env, "MODALIAS="))
		return -ENOMEM;

	len = input_print_modalias(&env->buf[env->buflen - 1],
				   sizeof(env->buf) - env->buflen,
				   dev, 0);
	if (len >= (sizeof(env->buf) - env->buflen))
		return -ENOMEM;

	env->buflen += len;
	return 0;
}

#define INPUT_ADD_HOTPLUG_VAR(fmt, val...)				\
	do {								\
		int err = add_uevent_var(env, fmt, val);		\
		if (err)						\
			return err;					\
	} while (0)

#define INPUT_ADD_HOTPLUG_BM_VAR(name, bm, max)				\
	do {								\
		int err = input_add_uevent_bm_var(env, name, bm, max);	\
		if (err)						\
			return err;					\
	} while (0)

#define INPUT_ADD_HOTPLUG_MODALIAS_VAR(dev)				\
	do {								\
		int err = input_add_uevent_modalias_var(env, dev);	\
		if (err)						\
			return err;					\
	} while (0)

static int input_dev_uevent(struct device *device, struct kobj_uevent_env *env)
{
	struct input_dev *dev = to_input_dev(device);

	INPUT_ADD_HOTPLUG_VAR("PRODUCT=%x/%x/%x/%x",
				dev->id.bustype, dev->id.vendor,
				dev->id.product, dev->id.version);
	if (dev->name)
		INPUT_ADD_HOTPLUG_VAR("NAME=\"%s\"", dev->name);
	if (dev->phys)
		INPUT_ADD_HOTPLUG_VAR("PHYS=\"%s\"", dev->phys);
	if (dev->uniq)
		INPUT_ADD_HOTPLUG_VAR("UNIQ=\"%s\"", dev->uniq);

	INPUT_ADD_HOTPLUG_BM_VAR("PROP=", dev->propbit, INPUT_PROP_MAX);

	INPUT_ADD_HOTPLUG_BM_VAR("EV=", dev->evbit, EV_MAX);
	if (test_bit(EV_KEY, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("KEY=", dev->keybit, KEY_MAX);
	if (test_bit(EV_REL, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("REL=", dev->relbit, REL_MAX);
	if (test_bit(EV_ABS, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("ABS=", dev->absbit, ABS_MAX);
	if (test_bit(EV_MSC, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("MSC=", dev->mscbit, MSC_MAX);
	if (test_bit(EV_LED, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("LED=", dev->ledbit, LED_MAX);
	if (test_bit(EV_SND, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("SND=", dev->sndbit, SND_MAX);
	if (test_bit(EV_FF, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("FF=", dev->ffbit, FF_MAX);
	if (test_bit(EV_SW, dev->evbit))
		INPUT_ADD_HOTPLUG_BM_VAR("SW=", dev->swbit, SW_MAX);

	INPUT_ADD_HOTPLUG_MODALIAS_VAR(dev);

	return 0;
}

#define INPUT_DO_TOGGLE(dev, type, bits, on)				\
	do {								\
		int i;							\
		bool active;						\
									\
		if (!test_bit(EV_##type, dev->evbit))			\
			break;						\
									\
		for_each_set_bit(i, dev->bits##bit, type##_CNT) {	\
			active = test_bit(i, dev->bits);		\
			if (!active && !on)				\
				continue;				\
									\
			dev->event(dev, EV_##type, i, on ? active : 0);	\
		}							\
	} while (0)

static void input_dev_toggle(struct input_dev *dev, bool activate)
{
	if (!dev->event)
		return;

	INPUT_DO_TOGGLE(dev, LED, led, activate);
	INPUT_DO_TOGGLE(dev, SND, snd, activate);

	if (activate && test_bit(EV_REP, dev->evbit)) {
		dev->event(dev, EV_REP, REP_PERIOD, dev->rep[REP_PERIOD]);
		dev->event(dev, EV_REP, REP_DELAY, dev->rep[REP_DELAY]);
	}
}

/**
 * input_reset_device() - reset/restore the state of input device
 * @dev: input device whose state needs to be reset
 *
 * This function tries to reset the state of an opened input device and
 * bring internal state and state if the hardware in sync with each other.
 * We mark all keys as released, restore LED state, repeat rate, etc.
 */
void input_reset_device(struct input_dev *dev)
{
	mutex_lock(&dev->mutex);

	if (dev->users) {
		input_dev_toggle(dev, true);

		/*
		 * Keys that have been pressed at suspend time are unlikely
		 * to be still pressed when we resume.
		 */
		spin_lock_irq(&dev->event_lock);
//		input_dev_release_keys(dev);
		spin_unlock_irq(&dev->event_lock);
	}

	mutex_unlock(&dev->mutex);
}
EXPORT_SYMBOL(input_reset_device);

#ifdef CONFIG_PM
static int input_dev_suspend(struct device *dev)
{
	struct input_dev *input_dev = to_input_dev(dev);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		input_dev_toggle(input_dev, false);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int input_dev_resume(struct device *dev)
{
	struct input_dev *input_dev = to_input_dev(dev);

	input_reset_device(input_dev);

	return 0;
}

static const struct dev_pm_ops input_dev_pm_ops = {
	.suspend	= input_dev_suspend,
	.resume		= input_dev_resume,
	.poweroff	= input_dev_suspend,
	.restore	= input_dev_resume,
};
#endif /* CONFIG_PM */

static struct device_type input_dev_type = {
	.groups		= input_dev_attr_groups,
	.release	= input_dev_release,
	.uevent		= input_dev_uevent,
#ifdef CONFIG_PM
	.pm		= &input_dev_pm_ops,
#endif
};

static char *input_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "input/%s", dev_name(dev));
}

struct class input_class = {
	.name		= "input",
	.devnode	= input_devnode,
};
EXPORT_SYMBOL_GPL(input_class);

/**
 * input_allocate_device - allocate memory for new input device
 *
 * Returns prepared struct input_dev or %NULL.
 *
 * NOTE: Use input_free_device() to free devices that have not been
 * registered; input_unregister_device() should be used for already
 * registered devices.
 */
struct input_dev *input_allocate_device(void)
{
	static atomic_t input_no = ATOMIC_INIT(-1);
	struct input_dev *dev;

	dev = kzalloc(sizeof(struct input_dev), GFP_KERNEL);
	if (dev) {
		dev->dev.type = &input_dev_type;
		dev->dev.class = &input_class;
		device_initialize(&dev->dev);
		mutex_init(&dev->mutex);
		spin_lock_init(&dev->event_lock);
		init_timer(&dev->timer);
		INIT_LIST_HEAD(&dev->h_list);
		INIT_LIST_HEAD(&dev->node);

		dev_set_name(&dev->dev, "input%lu",
			     (unsigned long)atomic_inc_return(&input_no));

		__module_get(THIS_MODULE);
	}

	return dev;
}
EXPORT_SYMBOL(input_allocate_device);

struct input_devres {
	struct input_dev *input;
};

static int devm_input_device_match(struct device *dev, void *res, void *data)
{
	struct input_devres *devres = res;

	return devres->input == data;
}

static void devm_input_device_release(struct device *dev, void *res)
{
	struct input_devres *devres = res;
	struct input_dev *input = devres->input;

	dev_dbg(dev, "%s: dropping reference to %s\n",
		__func__, dev_name(&input->dev));
	input_put_device(input);
}

/**
 * devm_input_allocate_device - allocate managed input device
 * @dev: device owning the input device being created
 *
 * Returns prepared struct input_dev or %NULL.
 *
 * Managed input devices do not need to be explicitly unregistered or
 * freed as it will be done automatically when owner device unbinds from
 * its driver (or binding fails). Once managed input device is allocated,
 * it is ready to be set up and registered in the same fashion as regular
 * input device. There are no special devm_input_device_[un]register()
 * variants, regular ones work with both managed and unmanaged devices,
 * should you need them. In most cases however, managed input device need
 * not be explicitly unregistered or freed.
 *
 * NOTE: the owner device is set up as parent of input device and users
 * should not override it.
 */
struct input_dev *devm_input_allocate_device(struct device *dev)
{
	struct input_dev *input;
	struct input_devres *devres;

	devres = devres_alloc(devm_input_device_release,
			      sizeof(struct input_devres), GFP_KERNEL);
	if (!devres)
		return NULL;

	input = input_allocate_device();
	if (!input) {
		devres_free(devres);
		return NULL;
	}

	input->dev.parent = dev;
	input->devres_managed = true;

	devres->input = input;
	devres_add(dev, devres);

	return input;
}
EXPORT_SYMBOL(devm_input_allocate_device);

/**
 * input_free_device - free memory occupied by input_dev structure
 * @dev: input device to free
 *
 * This function should only be used if input_register_device()
 * was not called yet or if it failed. Once device was registered
 * use input_unregister_device() and memory will be freed once last
 * reference to the device is dropped.
 *
 * Device should be allocated by input_allocate_device().
 *
 * NOTE: If there are references to the input device then memory
 * will not be freed until last reference is dropped.
 */
void input_free_device(struct input_dev *dev)
{
	if (dev) {
		if (dev->devres_managed)
			WARN_ON(devres_destroy(dev->dev.parent,
						devm_input_device_release,
						devm_input_device_match,
						dev));
		input_put_device(dev);
	}
}
EXPORT_SYMBOL(input_free_device);

/**
 * input_set_capability - mark device as capable of a certain event
 * @dev: device that is capable of emitting or accepting event
 * @type: type of the event (EV_KEY, EV_REL, etc...)
 * @code: event code
 *
 * In addition to setting up corresponding bit in appropriate capability
 * bitmap the function also adjusts dev->evbit.
 */
void input_set_capability(struct input_dev *dev, unsigned int type, unsigned int code)
{
	if (type < EV_CNT && input_max_code[type] &&
	    code > input_max_code[type]) {
		pr_err("%s: invalid code %u for type %u\n", __func__, code,
		       type);
		dump_stack();
		return;
	}

	switch (type) {
	case EV_KEY:
		__set_bit(code, dev->keybit);
		break;

	case EV_REL:
		__set_bit(code, dev->relbit);
		break;

	case EV_ABS:
		input_alloc_absinfo(dev);
		if (!dev->absinfo)
			return;

		__set_bit(code, dev->absbit);
		break;

	case EV_MSC:
		__set_bit(code, dev->mscbit);
		break;

	case EV_SW:
		__set_bit(code, dev->swbit);
		break;

	case EV_LED:
		__set_bit(code, dev->ledbit);
		break;

	case EV_SND:
		__set_bit(code, dev->sndbit);
		break;

	case EV_FF:
		__set_bit(code, dev->ffbit);
		break;

	case EV_PWR:
		/* do nothing */
		break;

	default:
		pr_err("input_set_capability: unknown type %u (code %u)\n",
		       type, code);
		dump_stack();
		return;
	}

	__set_bit(type, dev->evbit);
}
EXPORT_SYMBOL(input_set_capability);

static unsigned int input_estimate_events_per_packet(struct input_dev *dev)
{
	int mt_slots;
	int i;
	unsigned int events;

	if (dev->mt) {
		mt_slots = dev->mt->num_slots;
	} else if (test_bit(ABS_MT_TRACKING_ID, dev->absbit)) {
		mt_slots = dev->absinfo[ABS_MT_TRACKING_ID].maximum -
			   dev->absinfo[ABS_MT_TRACKING_ID].minimum + 1,
		mt_slots = clamp(mt_slots, 2, 32);
	} else if (test_bit(ABS_MT_POSITION_X, dev->absbit)) {
		mt_slots = 2;
	} else {
		mt_slots = 0;
	}

	events = mt_slots + 1; /* count SYN_MT_REPORT and SYN_REPORT */

	if (test_bit(EV_ABS, dev->evbit))
		for_each_set_bit(i, dev->absbit, ABS_CNT)
			events += input_is_mt_axis(i) ? mt_slots : 1;

	if (test_bit(EV_REL, dev->evbit))
		events += bitmap_weight(dev->relbit, REL_CNT);

	/* Make room for KEY and MSC events */
	events += 7;

	return events;
}

#define INPUT_CLEANSE_BITMASK(dev, type, bits)				\
	do {								\
		if (!test_bit(EV_##type, dev->evbit))			\
			memset(dev->bits##bit, 0,			\
				sizeof(dev->bits##bit));		\
	} while (0)

static void input_cleanse_bitmasks(struct input_dev *dev)
{
	INPUT_CLEANSE_BITMASK(dev, KEY, key);
	INPUT_CLEANSE_BITMASK(dev, REL, rel);
	INPUT_CLEANSE_BITMASK(dev, ABS, abs);
	INPUT_CLEANSE_BITMASK(dev, MSC, msc);
	INPUT_CLEANSE_BITMASK(dev, LED, led);
	INPUT_CLEANSE_BITMASK(dev, SND, snd);
	INPUT_CLEANSE_BITMASK(dev, FF, ff);
	INPUT_CLEANSE_BITMASK(dev, SW, sw);
}

static void __input_unregister_device(struct input_dev *dev)
{
	struct input_handle *handle, *next;

	input_disconnect_device(dev);

	mutex_lock(&input_mutex);

	list_for_each_entry_safe(handle, next, &dev->h_list, d_node)
		handle->handler->disconnect(handle);
	WARN_ON(!list_empty(&dev->h_list));

	del_timer_sync(&dev->timer);
	list_del_init(&dev->node);

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);

	device_del(&dev->dev);
}

static void devm_input_device_unregister(struct device *dev, void *res)
{
	struct input_devres *devres = res;
	struct input_dev *input = devres->input;

	dev_dbg(dev, "%s: unregistering device %s\n",
		__func__, dev_name(&input->dev));
	__input_unregister_device(input);
}

/**
 * input_enable_softrepeat - enable software autorepeat
 * @dev: input device
 * @delay: repeat delay
 * @period: repeat period
 *
 * Enable software autorepeat on the input device.
 */
void input_enable_softrepeat(struct input_dev *dev, int delay, int period)
{
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = input_repeat_key;
	dev->rep[REP_DELAY] = delay;
	dev->rep[REP_PERIOD] = period;
}
EXPORT_SYMBOL(input_enable_softrepeat);

/**
 * input_register_device - register device with input core
 * @dev: device to be registered
 *
 * This function registers device with input core. The device must be
 * allocated with input_allocate_device() and all it's capabilities
 * set up before registering.
 * If function fails the device must be freed with input_free_device().
 * Once device has been successfully registered it can be unregistered
 * with input_unregister_device(); input_free_device() should not be
 * called in this case.
 *
 * Note that this function is also used to register managed input devices
 * (ones allocated with devm_input_allocate_device()). Such managed input
 * devices need not be explicitly unregistered or freed, their tear down
 * is controlled by the devres infrastructure. It is also worth noting
 * that tear down of managed input devices is internally a 2-step process:
 * registered managed input device is first unregistered, but stays in
 * memory and can still handle input_event() calls (although events will
 * not be delivered anywhere). The freeing of managed input device will
 * happen later, when devres stack is unwound to the point where device
 * allocation was made.
 */
int input_register_device(struct input_dev *dev)
{
	struct input_devres *devres = NULL;
	struct input_handler *handler;
	unsigned int packet_size;
	const char *path;
	int error;

	if (dev->devres_managed) {
		devres = devres_alloc(devm_input_device_unregister,
				      sizeof(struct input_devres), GFP_KERNEL);
		if (!devres)
			return -ENOMEM;

		devres->input = dev;
	}

	/* Every input device generates EV_SYN/SYN_REPORT events. */
	__set_bit(EV_SYN, dev->evbit);

	/* KEY_RESERVED is no