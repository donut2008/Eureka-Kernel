 bitmasks not mentioned in dev->evbit are clean. */
	input_cleanse_bitmasks(dev);

	packet_size = input_estimate_events_per_packet(dev);
	if (dev->hint_events_per_packet < packet_size)
		dev->hint_events_per_packet = packet_size;

	dev->max_vals = dev->hint_events_per_packet + 2;
	dev->vals = kcalloc(dev->max_vals, sizeof(*dev->vals), GFP_KERNEL);
	if (!dev->vals) {
		error = -ENOMEM;
		goto err_devres_free;
	}

	/*
	 * If delay and period are pre-set by the driver, then autorepeating
	 * is handled by the driver itself and we don't do it in input.c.
	 */
	if (!dev->rep[REP_DELAY] && !dev->rep[REP_PERIOD])
		input_enable_softrepeat(dev, 250, 33);

	if (!dev->getkeycode)
		dev->getkeycode = input_default_getkeycode;

	if (!dev->setkeycode)
		dev->setkeycode = input_default_setkeycode;

	error = device_add(&dev->dev);
	if (error)
		goto err_free_vals;

	path = kobject_get_path(&dev->dev.kobj, GFP_KERNEL);
	pr_info("%s as %s\n",
		dev->name ? dev->name : "Unspecified device",
		path ? path : "N/A");
	kfree(path);

	error = mutex_lock_interruptible(&input_mutex);
	if (error)
		goto err_device_del;

	list_add_tail(&dev->node, &input_dev_list);

	list_for_each_entry(handler, &input_handler_list, node)
		input_attach_handler(dev, handler);

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);

	if (dev->devres_managed) {
		dev_dbg(dev->dev.parent, "%s: registering %s with devres.\n",
			__func__, dev_name(&dev->dev));
		devres_add(dev->dev.parent, devres);
	}
	return 0;

err_device_del:
	device_del(&dev->dev);
err_free_vals:
	kfree(dev->vals);
	dev->vals = NULL;
err_devres_free:
	devres_free(devres);
	return error;
}
EXPORT_SYMBOL(input_register_device);

/**
 * input_unregister_device - unregister previously registered device
 * @dev: device to be unregistered
 *
 * This function unregisters an input device. Once device is unregistered
 * the caller should not try to access it as it may get freed at any moment.
 */
void input_unregister_device(struct input_dev *dev)
{
	if (dev->devres_managed) {
		WARN_ON(devres_destroy(dev->dev.parent,
					devm_input_device_unregister,
					devm_input_device_match,
					dev));
		__input_unregister_device(dev);
		/*
		 * We do not do input_put_device() here because it will be done
		 * when 2nd devres fires up.
		 */
	} else {
		__input_unregister_device(dev);
		input_put_device(dev);
	}
}
EXPORT_SYMBOL(input_unregister_device);

/**
 * input_register_handler - register a new input handler
 * @handler: handler to be registered
 *
 * This function registers a new input handler (interface) for input
 * devices in the system and attaches it to all input devices that
 * are compatible with the handler.
 */
int input_register_handler(struct input_handler *handler)
{
	struct input_dev *dev;
	int error;

	error = mutex_lock_interruptible(&input_mutex);
	if (error)
		return error;

	INIT_LIST_HEAD(&handler->h_list);

	list_add_tail(&handler->node, &input_handler_list);

	list_for_each_entry(dev, &input_dev_list, node)
		input_attach_handler(dev, handler);

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);
	return 0;
}
EXPORT_SYMBOL(input_register_handler);

/**
 * input_unregister_handler - unregisters an input handler
 * @handler: handler to be unregistered
 *
 * This function disconnects a handler from its input devices and
 * removes it from lists of known handlers.
 */
void input_unregister_handler(struct input_handler *handler)
{
	struct input_handle *handle, *next;

	mutex_lock(&input_mutex);

	list_for_each_entry_safe(handle, next, &handler->h_list, h_node)
		handler->disconnect(handle);
	WARN_ON(!list_empty(&handler->h_list));

	list_del_init(&handler->node);

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);
}
EXPORT_SYMBOL(input_unregister_handler);

/**
 * input_handler_for_each_handle - handle iterator
 * @handler: input handler to iterate
 * @data: data for the callback
 * @fn: function to be called for each handle
 *
 * Iterate over @bus's list of devices, and call @fn for each, passing
 * it @data and stop when @fn returns