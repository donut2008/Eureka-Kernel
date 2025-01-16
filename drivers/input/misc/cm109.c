inter(dev->grab, handle);

 out:
	mutex_unlock(&dev->mutex);
	return retval;
}
EXPORT_SYMBOL(input_grab_device);

static void __input_release_device(struct input_handle *handle)
{
	struct input_dev *dev = handle->dev;
	struct input_handle *grabber;

	grabber = rcu_dereference_protected(dev->grab,
					    lockdep_is_held(&dev->mutex));
	if (grabber == handle) {
		rcu_assign_pointer(dev->grab, NULL);
		/* Make sure input_pass_event() notices that grab is gone */
		synchronize_rcu();

		list_for_each_entry(handle, &dev->h_list, d_node)
			if (handle->open && handle->handler->start)
				handle->handler->start(handle);
	}
}

/**
 * input_release_device - release previously grabbed device
 * @handle: input handle that owns the device
 *
 * Releases previously grabbed device so that other input handles can
 * start receiving input events. Upon release all handlers attached
 * to the device have their start() method called so they have a change
 * to synchronize device state with the rest of the system.
 */
void input_release_device(struct input_handle *handle)
{
	struct input_dev *dev = handle->dev;

	mutex_lock(&dev->mutex);
	__input_release_device(handle);
	mutex_unlock(&dev->mutex);
}
EXPORT_SYMBOL(input_release_device);

/**
 * input_open_device - open input device
 * @handle: handle through which device is being accessed
 *
 * This function should be called by input handlers when they
 * want to start receive events from given input device.
 */
int input_open_device(struct input_handle *handle)
{
	struct input_dev *dev = handle->dev;
	int retval;

	retval = mutex_lock_interruptible(&dev->mutex);
	if (retval)
		return retval;

	if (dev->going_away) {
		retval = -ENODEV;
		goto out;
	}

	handle->open++;

	dev->users_private++;
	if (!dev->disabled && !dev->users++ && dev->open)
		retval = dev->open(dev);

	if (retval) {
		dev->users_private--;
		if (!dev->disabled)
		dev->users--;
		if (!--handle->open) {
			/*
			 * Make sure we are not delivering any more events
			 * through this handle
			 */
			synchronize_rcu();
		}
	}

 out:
	mutex_unlock(&dev->mutex);
	return retval;
}
EXPORT_SYMBOL(input_open_device);

int input_flush_device(struct input_handle *handle, struct file *file)
{
	struct input_dev *dev = handle->dev;
	int retval;

	retval = mutex_lock_interruptible(&dev->mutex);
	if (retval)
		return retval;

	if (dev->flush)
		retval = dev->flush(dev, file);

	mutex_unlock(&dev->mutex);
	return retval;
}
EXPORT_SYMBOL(input_flush_device);

/**
 * input_close_device - close input device
 * @handle: handle through which device is being accessed
 *
 * This function should be called by input handlers when they
 * want to stop receive events from given input device.
 */
void input_close_device(struct input_handle *handle)
{
	struct input_dev *dev = handle->dev;

	mutex_lock(&dev->mutex);

	__input_release_device(handle);

	--dev->users_private;
	if (!dev->disabled && !--dev->users && dev->close)
		dev->close(dev);

	if (!--handle->open) {
		/*
		 * synchronize_rcu() makes sure that input_pass_event()
		 * completed and that no more input events are delivered
		 * through this handle
		 */
		synchronize_rcu();
	}

	mutex_unlock(&dev->mutex);
}
EXPORT_SYMBOL(input_close_device);
int input_enable_device(struct input_dev *dev)
{
	int retval;

	retval = mutex_lock_interruptible(&dev->mutex);
	if (retval)
		return retval;

	if (!dev->disabled)
		goto out;

	if (dev->users_private && dev->open) {
		retval = dev->open(dev);
		if (retval)
			goto out;
	}
	dev->users = dev->users_private;
	dev->disabled = false;

out:
	mutex_unlock(&dev->mutex);

	return retval;
}

int input_disable_device(struct input_dev *dev)
{
	int retval;

	retval = mutex_lock_interruptible(&dev->mutex);
	if (retval)
		return retval;

	if (!dev->disabled) {
		dev->disabled = true;
		if (dev->users && dev->close)
			dev->close(dev);
		dev->users = 0;
	}

	mutex_unlock(&dev->mutex);
	return 0;
}

/*
 * Simulate keyup events for all keys that are marked as pressed.
 * The function must be called with dev->event_lock held.
 */
static void input_dev_release_keys(struct input_dev *dev)
{
	bool need_sync = false;
	int code;

	if (is_event_supported(EV_KEY, dev->evbit, EV_MAX)) {
		for_each_set_bit(code, dev->key, KEY_CNT) {
			input_pass_event(dev, EV_KEY, code, 0);
			need_sync = true;
		}

		if (need_sync)
			input_pass_event(dev, EV_SYN, SYN_REPORT, 1);

		memset(dev->key, 0, sizeof(dev->key));
	}
}

/*
 * Prepare device for unregistering
 */
static void input_disconnect_device(struct input_dev *dev)
{
	struct input_handle *handle;

	/*
	 * Mark device as going away. Note that we take dev->mutex here
	 * not to protect access to dev->going_away but rather to ensure
	 * that there are no threads in the middle of input_open_device()
	 */
	mutex_lock(&dev->mutex);
	dev->going_away = true;
	mutex_unlock(&dev->mutex);

	spin_lock_irq(&dev->event_lock);

	/*
	 * Simulate keyup events for all pressed keys so that handlers
	 * are not left with "stuck" keys. The driver may continue
	 * generate events even after we done here but they will not
	 * reach any handlers.
	 */
	input_dev_release_keys(dev);

	list_for_each_entry(handle, &dev->h_list, d_node)
		handle->open = 0;

	spin_unlock_irq(&dev->event_lock);
}

/**
 * input_scancode_to_scalar() - converts scancode in &struct input_keymap_entry
 * @ke: keymap entry containing scancode to be converted.
 * @scancode: pointer to the location where converted scancode should
 *	be stored.
 *
 * This function is used to convert scancode stored in &struct keymap_entry
 * into scalar form understood by legacy keymap handling methods. These
 * methods expect scancodes to be represented as 'unsigned int'.
 */
int input_scancode_to_scalar(const struct input_keymap_entry *ke,
			     unsigned int *scancode)
{
	switch (ke->len) {
	case 1:
		*scancode = *((u8 *)ke->scancode);
		break;

	case 2:
		*scancode = *((u16 *)ke->scancode);
		break;

	case 4:
		*scancode = *((u32 *)ke->scancode);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(input_scancode_to_scalar);

/*
 * Those routines handle the default case where no [gs]etkeycode() is
 * defined. In this case, an array indexed by the scancode is used.
 */

static unsigned int input_fetch_keycode(struct input_dev *dev,
					unsigned int index)
{
	switch (dev->keycodesize) {
	case 1:
		return ((u8 *)dev->keycode)[index];

	case 2:
		return ((u16 *)dev->keycode)[index];

	default:
		return ((u32 *)dev->keycode)[index];
	}
}

static int input_default_getkeycode(struct input_dev *dev,
				    struct input_keymap_entry *ke)
{
	unsigned int index;
	int error;

	if (!dev->keycodesize)
		return -EINVAL;

	if (ke->flags & INPUT_KEYMAP_BY_INDEX)
		index = ke->index;
	else {
		error = input_scancode_to_scalar(ke, &index);
		if (error)
			return error;
	}

	if (index >= dev->keycodemax)
		return -EINVAL;

	ke->keycode = input_fetch_keycode(dev, index);
	ke->index = index;
	ke->len = sizeof(index);
	memcpy(ke->scancode, &index, sizeof(index));

	return 0;
}

static int input_default_setkeycode(struct input_dev *dev,
				    const struct input_keymap_entry *ke,
				    unsigned int *old_keycode)
{
	unsigned int index;
	int error;
	int i;

	if (!dev->keycodesize)
		return -EINVAL;

	if (ke->flags & INPUT_KEYMAP_BY_INDEX) {
		index = ke->index;
	} else {
		error = input_scancode_to_scalar(ke, &index);
		if (error)
			return error;
	}

	if (index >= dev->keycodemax)
		return -EINVAL;

	if (dev->keycodesize < sizeof(ke->keycode) &&
			(ke->keycode >> (dev->keycodesize * 8)))
		return -EINVAL;

	switch (dev->keycodesize) {
		case 1: {
			u8 *k = (u8 *)dev->keycode;
			*old_keycode = k[index];
			k[index] = ke->keycode;
			break;
		}
		case 2: {
			u16 *k = (u16 *)dev->keycode;
			*old_keycode = k[index];
			k[index] = ke->keycode;
			break;
		}
		default: {
			u32 *k = (u32 *)dev->keycode;
			*old_keycode = k[index];
			k[index] = ke->keycode;
			break;
		}
	}

	if (*old_keycode <= KEY_MAX) {
		__clear_bit(*old_keycode, dev->keybit);
		for (i = 0; i < dev->keycodemax; i++) {
			if (input_fetch_keycode(dev, i) == *old_keycode) {
				__set_bit(*old_keycode, dev->keybit);
				/* Setting the bit twice is useless, so break */
				break;
			}
		}
	}

	__set_bit(ke->keycode, dev->keybit);
	return 0;
}

/**
 * input_get_keycode - retrieve keycode currently mapped to a given scancode
 * @dev: input device which keymap is being queried
 * @ke: keymap entry
 *
 * This function should be called by anyone interested in retrieving current
 * keymap. Presently evdev handlers use it.
 */
int input_get_keycode(struct input_dev *dev, struct input_keymap_entry *ke)
{
	unsigned long flags;
	int retval;

	spin_lock_irqsave(&dev->event_lock, flags);
	retval = dev->getkeycode(dev, ke);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	return retval;
}
EXPORT_SYMBOL(input_get_keycode);

/**
 * input_set_keycode - attribute a keycode to a given scancode
 * @dev: input device which keymap is being updated
 * @ke: new keymap entry
 *
 * This function should be called by anyone needing to update current
 * keymap. Presently keyboard and evdev handlers use it.
 */
int input_set_keycode(struct input_dev *dev,
		      const struct input_keymap_entry *ke)
{
	unsigned long flags;
	unsigned int old_keycode;
	int retval;

	if (ke->keycode > KEY_MAX)
		return -EINVAL;

	spin_lock_irqsave(&dev->event_lock, flags);

	retval = dev->setkeycode(dev, ke, &old_keycode);
	if (retval)
		goto out;

	/* Make sure KEY_RESERVED did not get enabled. */
	__clear_bit(KEY_RESERVED, dev->keybit);

	/*
	 * Simulate keyup event if keycode is not present
	 * in the keymap anymore
	 */
	if (old_keycode > KEY_MAX) {
		dev_warn(dev->dev.parent ?: &dev->dev,
			 "%s: got too big old keycode %#x\n",
			 __func__, old_keycode);
	} else if (test_bit(EV_KEY, dev->evbit) &&
		   !is_event_supported(old_keycode, dev->keybit, KEY_MAX) &&
		   __test_and_clear_bit(old_keycode, dev->key)) {
		struct input_value vals[] =  {
			{ EV_KEY, old_keycode, 0 },
			input_value_sync
		};

		input_pass_values(dev, vals, ARRAY_SIZE(vals));
	}

 out:
	spin_unlock_irqrestore(&dev->event_lock, flags);

	return retval;
}
EXPORT_SYMBOL(input_set_keycode);

static const struct input_device_id *input_match_device(struct input_handler *handler,
							struct input_dev *dev)
{
	const struct input_device_id *id;

	for (id = handler->id_table; id->flags || id->driver_info; id++) {

		if (id->flags & INPUT_DEVICE_ID_MATCH_BUS)
			if (id->bustype != dev->id.bustype)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_VENDOR)
			if (id->vendor != dev->id.vendor)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_PRODUCT)
			if (id->product != dev->id.product)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_VERSION)
			if (id->version != dev->id.version)
				continue;

		if (!bitmap_subset(id->evbit, dev->evbit, EV_MAX))
			continue;

		if (!bitmap_subset(id->keybit, dev->keybit, KEY_MAX))
			continue;

		if (!bitmap_subset(id->relbit, dev->relbit, REL_MAX))
			continue;

		if (!bitmap_subset(id->absbit, dev->absbit, ABS_MAX))
			continue;

		if (!bitmap_subset(id->mscbit, dev->mscbit, MSC_MAX))
			continue;

		if (!bitmap_subset(id->ledbit, dev->ledbit, LED_MAX))
			continue;

		if (!bitmap_subset(id->sndbit, dev->sndbit, SND_MAX))
			continue;

		if (!bitmap_subset(id->ffbit, dev->ffbit, FF_MAX))
			continue;

		if (!bitmap_subset(id->swbit, dev->swbit, SW_MAX))
			continue;

		if (!handler->match || handler->match(handler, dev))
			return id;
	}

	return NULL;
}

static int input_attach_handler(struct input_dev *dev, struct input_handler *handler)
{
	const struct input_device_id *id;
	int error;

	id = input_match_device(handler, dev);
	if (!id)
		return -ENODEV;

	error = handler->connect(handler, dev, id);
	if (error && error != -ENODEV)
		pr_err("failed to attach handler %s to device %s, error: %d\n",
		       handler->name, kobject_name(&dev->dev.kobj), error);

	return error;
}

#ifdef CONFIG_COMPAT

static int input_bits_to_string(char *buf, int buf_size,
				unsigned long bits, bool skip_empty)
{
	int len = 0;

	if (INPUT_COMPAT_TEST) {
		u32 dword = bits >> 32;
		if (dword || !skip_empty)
			len += snprintf(buf, buf_size, "%x ", dword);

		dword = bits & 0xffffffffUL;
		if (dword || !skip_empty || len)
			len += snprintf(buf + len, max(buf_size - len, 0),
					"%x", dword);
	} else {
		if (bits || !skip_empty)
			len += snprintf(buf, buf_size, "%lx", bits);
	}

	return len;
}

#else /* !CONFIG_COMPAT */

static int input_bits_to_string(char *buf, int buf_size,
				unsigned long bits, bool skip_empty)
{
	return bits || !skip_empty ?
		snprintf(buf, buf_size, "%lx", bits) : 0;
}

#endif

#ifdef CONFIG_PROC_FS

static struct proc_dir_entry *proc_bus_input_dir;
static DECLARE_WAIT_QUEUE_HEAD(input_devices_poll_wait);
static int input_devices_state;

static inline void input_wakeup_procfs_readers(void)
{
	input_devices_state++;
	wake_up(&input_devices_poll_wait);
}

static unsigned int input_proc_devices_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &input_devices_poll_wait, wait);
	if (file->f_version != input_devices_state) {
		file->f_version = input_devices_state;
		return POLLIN | POLLRDNORM;
	}

	return 0;
}

union input_seq_state {
	struct {
		unsigned short pos;
		bool mutex_acquired;
	};
	void *p;
};

static void *input_devices_seq_start(struct seq_file *seq, loff_t *pos)
{
	union input_seq_state *state = (union input_seq_state *)&seq->private;
	int error;

	/* We need to fit into seq->private pointer */
	BUILD_BUG_ON(sizeof(union input_seq_state) != sizeof(seq->private));

	error = mutex_lock_interruptible(&input_mutex);
	if (error) {
		state->mutex_acquired = false;
		return ERR_PTR(error);
	}

	state->mutex_acquired = true;

	return seq_list_start(&input_dev_list, *pos);
}

static void *input_devices_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	return seq_list_next(v, &input_dev_list, pos);
}

static void input_seq_stop(struct seq_file *seq, void *v)
{
	union input_seq_state *state = (union input_seq_state *)&seq->private;

	if (state->mutex_acquired)
		mutex_unlock(&input_mutex);
}

static void input_seq_print_bitmap(struct seq_file *seq, const char *name,
				   unsigned long *bitmap, int max)
{
	int i;
	bool skip_empty = true;
	char buf[18];

	seq_printf(seq, "B: %s=", name);

	for (i = BITS_TO_LONGS(max) - 1; i >= 0; i--) {
		if (input_bits_to_string(buf, sizeof(buf),
					 bitmap[i], skip_empty)) {
			skip_empty = false;
			seq_printf(seq, "%s%s", buf, i > 0 ? " " : "");
		}
	}

	/*
	 * If no output was produced print a single 0.
	 */
	if (skip_empty)
		seq_puts(seq, "0");

	seq_putc(seq, '\n');
}

static int input_devices_seq_show(struct seq_file *seq, void *v)
{
	struct input_dev *dev = container_of(v, struct input_dev, node);
	const char *path = kobject_get_path(&dev->dev.kobj, GFP_KERNEL);
	struct input_handle *handle;

	seq_printf(seq, "I: Bus=%04x Vendor=%04x Product=%04x Version=%04x\n",
		   dev->id.bustype, dev->id.vendor, dev->id.product, dev->id.version);

	seq_printf(seq, "N: Name=\"%s\"\n", dev->name ? dev->name : "");
	seq_printf(seq, "P: Phys=%s\n", dev->phys ? dev->phys : "");
	seq_printf(seq, "S: Sysfs=%s\n", path ? path : "");
	seq_printf(seq, "U: Uniq=%s\n", dev->uniq ? dev->uniq : "");
	seq_printf(seq, "H: Handlers=");

	list_for_each_entry(handle, &dev->h_list, d_node)
		seq_printf(seq, "%s ", handle->name);
	seq_putc(seq, '\n');

	input_seq_print_bitmap(seq, "PROP", dev->propbit, INPUT_PROP_MAX);

	input_seq_print_bitmap(seq, "EV", dev->evbit, EV_MAX);
	if (test_bit(EV_KEY, dev->evbit))
		input_seq_print_bitmap(seq, "KEY", dev->keybit, KEY_MAX);
	if (test_bit(EV_REL, dev->evbit))
		input_seq_print_bitmap(seq, "REL", dev->relbit, REL_MAX);
	if (test_bit(EV_ABS, dev->evbit))
		input_seq_print_bitmap(seq, "ABS", dev->absbit, ABS_MAX);
	if (test_bit(EV_MSC, dev->evbit))
		input_seq_print_bitmap(seq, "MSC", dev->mscbit, MSC_MAX);
	if (test_bit(EV_LED, dev->evbit))
		input_seq_print_bitmap(seq, "LED", dev->ledbit, LED_MAX);
	if (test_bit(EV_SND, dev->evbit))
		input_seq_print_bitmap(seq, "SND", dev->sndbit, SND_MAX);
	if (test_bit(EV_FF, dev->evbit))
		input_seq_print_bitmap(seq, "FF", dev->ffbit, FF_MAX);
	if (test_bit(EV_SW, dev->evbit))
		input_seq_print_bitmap(seq, "SW", dev->swbit, SW_MAX);

	seq_putc(seq, '\n');

	kfree(path);
	return 0;
}

static const struct seq_operations input_devices_seq_ops = {
	.start	= input_devices_seq_start,
	.next	= input_devices_seq_next,
	.stop	= input_seq_stop,
	.show	= input_devices_seq_show,
};

static int input_proc_devices_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &input_devices_seq_ops);
}

static const struct file_operations input_devices_fileops = {
	.owner		= THIS_MODULE,
	.open		= input_proc_devices_open,
	.poll		= input_proc_devices_poll,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static void *input_handlers_seq_start(struct seq_file *seq, loff_t *pos)
{
	union input_seq_state *state = (union input_seq_state *)&seq->private;
	int error;

	/* We need to fit into seq->private pointer */
	BUILD_BUG_ON(sizeof(union input_seq_state) != sizeof(seq->private));

	error = mutex_lock_interruptible(&input_mutex);
	if (error) {
		state->mutex_acquired = false;
		return ERR_PTR(error);
	}

	state->mutex_acquired = true;
	state->pos = *pos;

	return seq_list_start(&input_handler_list, *pos);
}

static void *input_handlers_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	union input_seq_state *state = (union input_seq_state *)&seq->private;

	state->pos = *pos + 1;
	return seq_list_next(v, &input_handler_list, pos);
}

static int input_handlers_seq_show(struct seq_file *seq, void *v)
{
	struct input_handler *handler = container_of(v, struct input_handler, node);
	union input_seq_state *state = (union input_seq_state *)&seq->private;

	seq_printf(seq, "N: Number=%u Name=%s", state->pos, handler->name);
	if (handler->filter)
		seq_puts(seq, " (filter)");
	if (handler->legacy_minors)
		seq_printf(seq, " Minor=%d", handler->minor);
	seq_putc(seq, '\n');

	return 0;
}

static const struct seq_operations input_handlers_seq_ops = {
	.start	= input_handlers_seq_start,
	.next	= input_handlers_seq_next,
	.stop	= input_seq_stop,
	.show	= input_handlers_seq_show,
};

static int input_proc_handlers_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &input_handlers_seq_ops);
}

static const struct file_operations input_handlers_fileops = {
	.owner		= THIS_MODULE,
	.open		= input_proc_handlers_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init input_proc_init(void)
{
	struct proc_dir_entry *entry;

	proc_bus_input_dir = proc_mkdir("bus/input", NULL);
	if (!proc_bus_input_dir)
		return -ENOMEM;

	entry = proc_create("devices", 0, proc_bus_input_dir,
			    &input_devices_fileops);
	if (!entry)
		goto fail1;

	entry = proc_create("handlers", 0, proc_bus_input_dir,
			    &input_handlers_fileops);
	if (!entry)
		goto fail2;

	return 0;

 fail2:	remove_proc_entry("devices", proc_bus_input_dir);
 fail1: remove_proc_entry("bus/input", NULL);
	return -ENOMEM;
}

static void input_proc_exit(void)
{
	remove_proc_entry("devices", proc_bus_input_dir);
	remove_proc_entry("handlers", proc_bus_input_dir);
	remove_proc_entry("bus/input", NULL);
}

#else /* !CONFIG_PROC_FS */
static inline void input_wakeup_procfs_readers(void) { }
static inline int input_proc_init(void) { return 0; }
static inline void input_proc_exit(void) { }
#endif

#define INPUT_DEV_STRING_ATTR_SHOW(name)				\
static ssize_t input_dev_show_##name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     char *buf)				\
{									\
	struct input_dev *input_dev = to_input_dev(dev);		\
									\
	return scnprintf(buf, PAGE_SIZE, "%s\n",			\
			 input_dev->name ? input_dev->name : "");	\
}									\
static DEVICE_ATTR(name, S_IRUGO, input_dev_show_##name, NULL)

INPUT_DEV_STRING_ATTR_SHOW(name);
INPUT_DEV_STRING_ATTR_SHOW(phys);
INPUT_DEV_STRING_ATTR_SHOW(uniq);

static int input_print_modalias_bits(char *buf, int size,
				     char name, unsigned long *bm,
				     unsigned int min_bit, unsigned int max_bit)
{
	int len = 0, i;

	len += snprintf(buf, max(size, 0), "%c", name);
	for (i = min_bit; i < max_bit; i++)
		if (bm[BIT_WORD(i)] & BIT_MASK(i))
			len += snprintf(buf + len, max(size - len, 0), "%X,", i);
	return len;
}

static int input_print_modalias(char *buf, int size, struct input_dev *id,
				int add_cr)
{
	int len;

	len = snprintf(buf, max(size, 0),
		       "input:b%04Xv%04Xp%04Xe%04X-",
		       id->id.bustype, id->id.vendor,
		       id->id.product, id->id.version);

	len += input_print_modalias_bits(buf + len, size - len,
				'e', id->evbit, 0, EV_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'k', id->keybit, KEY_MIN_INTERESTING, KEY_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'r', id->relbit, 0, REL_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'a', id->absbit, 0, ABS_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'm', id->mscbit, 0, MSC_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'l', id->ledbit, 0, LED_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				's', id->sndbit, 0, SND_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'f', id->ffbit, 0, FF_MAX);
	len += input_print_modalias_bits(buf + len, size - len,
				'w', id->swbit, 0, SW_MAX);

	if (add_cr)
		len += snprintf(buf + len, max(size - len, 0), "\n");

	return len;
}

static ssize_t input_dev_show_modalias(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct input_dev *id = to_input_dev(dev);
	ssize_t len;

	len = input_print_modalias(buf, PAGE_SIZE, id, 1);

	return min_t(int, len, PAGE_SIZE);
}
static DEVICE_ATTR(modalias, S_IRUGO, input_dev_show_modalias, NULL);

static int input_print_bitmap(char *buf, int buf_size, unsigned long *bitmap,
			      int max, int add_cr);

static ssize_t input_dev_show_properties(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	int len = input_print_bitmap(buf, PAGE_SIZE, input_dev->propbit,
				     INPUT_PROP_MAX, true);
	return min_t(int, len, PAGE_SIZE);
}
static DEVICE_ATTR(properties, S_IRUGO, input_dev_show_properties, NULL);

static ssize_t input_dev_show_enabled(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	return scnprintf(buf, PAGE_SIZE, "%d\n", !input_dev->disabled);
}

static ssize_t input_dev_store_enabled(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	int ret;
	bool enable;
	struct input_dev *input_dev = to_input_dev(dev);

	ret = strtobool(buf, &enable);
	if (ret)
		return ret;

	if (enable)
		ret = input_enable_device(input_dev);
	else
		ret = input_disable_device(input_dev);
	if (ret)
		return ret;

	return size;
}

static DEVICE_ATTR(enabled, S_IRUGO | S_IWUSR,
		   input_dev_show_enabled, input_dev_store_enabled);

static struct attribute *input_dev_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_phys.attr,
	&dev_attr_uniq.attr,
	&dev_attr_modalias.attr,
	&dev_attr_properties.attr,
	&dev_attr_enabled.attr,
	NULL
};

static struct attribute_group input_dev_attr_group = {
	.attrs	= input_dev_attrs,
};

#define INPUT_DEV_ID_ATTR(name)						\
static ssize_t input_dev_show_id_##name(struct device *dev,		\
					struct device_attribute *attr,	\
					char *buf)			\
{									\
	struct input_dev *input_dev = to_input_dev(dev);		\
	return scnprintf(buf, PAGE_SIZE, "%04x\n", input_dev->id.name);	\
}									\
static DEVICE_ATTR(name, S_IRUGO, input_dev_show_id_##name, NULL)

INPUT_DEV_ID_ATTR(bustype);
INPUT_DEV_ID_ATTR(vendor);
INPUT_DEV_ID_ATTR(product);
INPUT_DEV_ID_ATTR(version);

static struct attribute *input_dev_id_attrs[] = {
	&dev_attr_bustype.attr,
	&dev_attr_vendor.attr,
	&dev_attr_product.attr,
	&dev_attr_version.attr,
	NULL
};

static struct attribute_group input_dev_id_attr_group = {
	.name	= "id",
	.attrs	= input_dev_id_attrs,
};

static int input_print_bitmap(char *buf, int buf_size, unsigned long *bitmap,
			      int max, int add_cr)
{
	int i;
	int len = 0;
	bool skip_empty = true;

	for (i = BITS_T