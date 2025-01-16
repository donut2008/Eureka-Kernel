kobj, KOBJ_CHANGE, envp);
			free_page((unsigned long)prop_buf);
		} else {
			/* Unlock early before uevent */
			spin_unlock_irqrestore(&edev->lock, flags);

			dev_err(&edev->dev, "out of memory in extcon_set_state\n");
			kobject_uevent(&edev->dev.kobj, KOBJ_CHANGE);
		}
	} else {
		/* No changes */
		spin_unlock_irqrestore(&edev->lock, flags);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(extcon_update_state);

/**
 * extcon_set_state() - Set the cable attach states of the extcon device.
 * @edev:	the extcon device
 * @state:	new cable attach status for @edev
 *
 * Note that notifier provides which bits are changed in the state
 * variable with the val parameter (second) to the callback.
 */
int extcon_set_state(struct extcon_dev *edev, u32 state)
{
	if (!edev)
		return -EINVAL;

	return extcon_update_state(edev, 0xffffffff, state);
}
EXPORT_SYMBOL_GPL(extcon_set_state);

/**
 * extcon_get_cable_state_() - Get the status of a specific cable.
 * @edev:	the extcon device that has the cable.
 * @id:		the unique id of each external connector in extcon enumeration.
 */
int extcon_get_cable_state_(struct extcon_dev *edev, const unsigned int id)
{
	int index;

	if (!edev)
		return -EINVAL;

	index = find_cable_index_by_id(edev, id);
	if (index < 0)
		return index;

	if (edev->max_supported && edev->max_supported <= index)
		return -EINVAL;

	return !!(edev->state & (1 << index));
}
EXPORT_SYMBOL_GPL(extcon_get_cable_state_);

/**
 * extcon_get_cable_state() - Get the status of a specific cable.
 * @edev:	the extcon device that has the cable.
 * @cable_name:	cable name.
 *
 * Note that this is slower than extcon_get_cable_state_.
 */
int extcon_get_cable_state(struct extcon_dev *edev, const char *cable_name)
{
	int id;

	id = find_cable_id_by_name(edev, cable_name);
	if (id < 0)
		return id;

	return extcon_get_cable_state_(edev, id);
}
EXPORT_SYMBOL_GPL(extcon_get_cable_state);

/**
 * extcon_set_cable_state_() - Set the status of a specific cable.
 * @edev:		the extcon device that has the cable.
 * @id:			the unique id of each external connector
 *			in extcon enumeration.
 * @state:		the new cable status. The default semantics is
 *			true: attached / false: detached.
 */
int extcon_set_cable_state_(struct extcon_dev *edev, unsigned int id,
				bool cable_state)
{
	u32 state;
	int index;

	if (!edev)
		return -EINVAL;

	index = find_cable_index_by_id(edev, id);
	if (index < 0)
		return index;

	if (edev->max_supported && edev->max_supported <= index)
		return -EINVAL;

	state = cable_state ? (1 << index) : 0;
	return extcon_update_state(edev, 1 << index, state);
}
EXPORT_SYMBOL_GPL(extcon_set_cable_state_);

/**
 * extcon_set_cable_state() - Set the status of a specific cable.
 * @edev:		the extcon device that has the cable.
 * @cable_name:		cable name.
 * @cable_state:	the new cable status. The default semantics is
 *			true: attached / false: detached.
 *
 * Note that this is slower than extcon_set_cable_state_.
 */
int extcon_set_cable_state(struct extcon_dev *edev,
			const char *cable_name, bool cable_state)
{
	int id;

	id = find_cable_id_by_name(edev, cable_name);
	if (id < 0)
		return id;

	return extcon_set_cable_state_(edev, id, cable_state);
}
EXPORT_SYMBOL_GPL(extcon_set_cable_state);

/**
 * extcon_get_extcon_dev() - Get the extcon device instance from the name
 * @extcon_name:	The extcon name provided with extcon_dev_register()
 */
struct extcon_dev *extcon_get_extcon_dev(const char *extcon_name)
{
	struct extcon_dev *sd;

	if (!extcon_name)
		return ERR_PTR(-EINVAL);

	mutex_lock(&extcon_dev_list_lock);
	list_for_each_entry(sd, &extcon_dev_list, entry) {
		if (!strcmp(sd->name, extcon_name))
			goto out;
	}
	sd = NULL;
out:
	mutex_unlock(&extcon_dev_list_lock);
	return sd;
}
EXPORT_SYMBOL_GPL(extcon_get_extcon_dev);

/**
 * extcon_register_interest() - Register a notifier for a state change of a
 *				specific cable, not an entier set of cables of a
 *				extcon device.
 * @obj:		an empty extcon_specific_cable_nb object to be returned.
 * @extcon_name:	the name of extcon device.
 *			if NULL, extcon_register_interest will register
 *			every cable with the target cable_name given.
 * @cable_name:		the target cable name.
 * @nb:			the notifier block to get notified.
 *
 * Provide an empty extcon_specific_cable_nb. extcon_register_interest() sets
 * the struct for you.
 *
 * extcon_register_interest is a helper function for those who want to get
 * notification for a single specific cable's status change. If a user wants
 * to get notification for any changes of all cables of a extcon device,
 * he/she should use the general extcon_register_notifier().
 *
 * Note that the second parameter given to the callback of nb (val) is
 * "old_state", not the current state. The current state can be retrieved
 * by looking at the third pameter (edev pointer)'s state value.
 */
int extcon_register_interest(struct extcon_specific_cable_nb *obj,
			     const char *extcon_name, const char *cable_name,
			     struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	if (!obj || !cable_name || !nb)
		return -EINVAL;

	if (extcon_name) {
		obj->edev = extcon_get_extcon_dev(extcon_name);
		if (!obj->edev)
			return -ENODEV;

		obj->cable_index = find_cable_index_by_name(obj->edev,
							cable_name);
		if (obj->cable_index < 0)
			return obj->cable_index;

		obj->user_nb = nb;

		spin_lock_irqsave(&obj->edev->lock, flags);
		ret = raw_notifier_chain_register(
					&obj->edev->nh[obj->cable_index],
					obj->user_nb);
		spin_unlock_irqrestore(&obj->edev->lock, flags);
	} else {
		struct class_dev_iter iter;
		struct extcon_dev *extd;
		struct device *dev;

		if (!extcon_class)
			return -ENODEV;
		class_dev_iter_init(&iter, extcon_class, NULL, NULL);
		while ((dev = class_dev_iter_next(&iter))) {
			extd = dev_get_drvdata(dev);

			if (find_cable_index_by_name(extd, cable_name) < 0)
				continue;

			class_dev_iter_exit(&iter);
			return extcon_register_interest(obj, extd->name,
						cable_name, nb);
		}

		ret = -ENODEV;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(extcon_register_interest);

/**
 * extcon_unregister_interest() - Unregister the notifier registered by
 *				  extcon_register_interest().
 * @obj:	the extcon_specific_cable_nb object returned by
 *		extcon_register_interest().
 */
int extcon_unregister_interest(struct extcon_specific_cable_nb *obj)
{
	unsigned long flags;
	int ret;

	if (!obj)
		return -EINVAL;

	spin_lock_irqsave(&obj->edev->lock, flags);
	ret = raw_notifier_chain_unregister(
			&obj->edev->nh[obj->cable_index], obj->user_nb);
	spin_unlock_irqrestore(&obj->edev->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(extcon_unregister_interest);

/**
 * extcon_register_notifier() - Register a notifiee to get notified by
 *				any attach status changes from the extcon.
 * @edev:	the extcon device that has the external connecotr.
 * @id:		the unique id of each external connector in extcon enumeration.
 * @nb:		a notifier block to be registered.
 *
 * Note that the second parameter given to the callback of nb (val) is
 * "old_state", not the current state. The current state can be retrieved
 * by looking at the third pameter (edev pointer)'s state value.
 */
int extcon_register_notifier(struct extcon_dev *edev, unsigned int id,
			     struct notifier_block *nb)
{
	unsigned long flags;
	int ret, idx;

	if (!edev || !nb)
		return -EINVAL;

	idx = find_cable_index_by_id(edev, id);

	spin_lock_irqsave(&edev->lock, flags);
	ret = raw_notifier_chain_register(&edev->nh[idx], nb);
	spin_unlock_irqrestore(&edev->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(extcon_register_notifier);

/**
 * extcon_unregister_notifier() - Unregister a notifiee from the extcon device.
 * @edev:	the extcon device that has the external connecotr.
 * @id:		the unique id of each external connector in extcon enumeration.
 * @nb:		a notifier block to be registered.
 */
int extcon_unregister_notifier(struct extcon_dev *edev, unsigned int id,
				struct notifier_block *nb)
{
	unsigned long flags;
	int ret, idx;

	if (!edev || !nb)
		return -EINVAL;

	idx = find_cable_index_by_id(edev, id);

	spin_lock_irqsave(&edev->lock, flags);
	ret = raw_notifier_chain_unregister(&edev->nh[idx], nb);
	spin_unlock_irqrestore(&edev->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(extcon_unregister_notifier);

static struct attribute *extcon_attrs[] = {
	&dev_attr_state.attr,
	&dev_attr_name.attr,
	NULL,
};
ATTRIBUTE_GROUPS(extcon);

static int create_extcon_class(void)
{
	if (!extcon_class) {
		extcon_class = class_create(THIS_MODULE, "extcon");
		if (IS_ERR(extcon_class))
			return PTR_ERR(extcon_class);
		extcon_class->dev_groups = extcon_groups;

#if defined(CONFIG_ANDROID)
		switch_class = class_compat_register("switch");
		if (WARN(!switch_class, "cannot allocate"))
			return -ENOMEM;
#endif /* CONFIG_ANDROID */
	}

	return 0;
}

static void extcon_dev_release(struct device *dev)
{
}

static const char *muex_name = "mutually_exclusive";
static void dummy_sysfs_dev_release(struct device *dev)
{
}

/*
 * extcon_dev_allocate() - Allocate the memory of extcon device.
 * @supported_cable:	Array of supported extcon ending with EXTCON_NONE.
 *			If supported_cable is NULL, cable name related APIs
 *			are disabled.
 *
 * This function allocates the memory for extcon device without allocating
 * memory in each extcon provider driver and initialize default setting for
 * extcon device.
 *
 * Return the pointer of extcon device if success or ERR_PTR(err) if fail
 */
struct extcon_dev *extcon_dev_allocate(const unsigned int *supported_cable)
{
	struct extcon_dev *edev;

	if (!supported_cable)
		return ERR_PTR(-EINVAL);

	edev = kzalloc(sizeof(*edev), GFP_KERNEL);
	if (!edev)
		return ERR_PTR(-ENOMEM);

	edev->max_supported = 0;
	edev->supported_cable = supported_cable;

	return edev;
}

/*
 * extcon_dev_free() - Free the memory of extcon device.
 * @edev:	the extcon device to free
 */
void extcon_dev_free(struct extcon_dev *edev)
{
	kfree(edev);
}
EXPORT_SYMBOL_GPL(extcon_dev_free);

static int devm_extcon_dev_match(struct device *dev, void *res, void *data)
{
	struct extcon_dev **r = res;

	if (WARN_ON(!r || !*r))
		return 0;

	return *r == data;
}

static void devm_extcon_dev_release(struct device *dev, void *res)
{
	extcon_dev_free(*(struct extcon_dev **)res);
}

/**
 * devm_extcon_dev_allocate - Allocate managed extcon device
 * @dev:		device owning the extcon device being created
 * @supported_cable:	Array of supported extcon ending with EXTCON_NONE.
 *			If supported_cable is NULL, cable name related APIs
 *			are disabled.
 *
 * This function manages automatically the memory of extcon device using device
 * resource management and simplify the control of freeing the memory of extcon
 * device.
 *
 * Returns the pointer memory of allocated extcon_dev if success
 * or ERR_PTR(err) if fail
 */
struct extcon_dev *devm_extcon_dev_allocate(struct device *dev,
					const unsigned int *supported_cable)
{
	struct extcon_dev **ptr, *edev;

	ptr = devres_alloc(devm_extcon_dev_release, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	edev = extcon_dev_allocate(supported_cable);
	if (IS_ERR(edev)) {
		devres_free(ptr);
		return edev;
	}

	edev->dev.parent = dev;

	*ptr = edev;
	devres_add(dev, ptr);

	return edev;
}
EXPORT_SYMBOL_GPL(devm_extcon_dev_allocate);

void devm_extcon_dev_free(struct device *dev, struct extcon_dev *edev)
{
	WARN_ON(devres_release(dev, devm_extcon_dev_release,
			       devm_extcon_dev_match, edev));
}
EXPORT_SYMBOL_GPL(devm_extcon_dev_free);

/**
 * extcon_dev_register() - Register a new extcon device
 * @edev	: the new extcon device (should be allocated before calling)
 *
 * Among the members of edev struct, please set the "user initializing data"
 * in any case and set the "optional callbacks" if required. However, please
 * do not set the values of "internal data", which are initialized by
 * this function.
 */
int extcon_dev_register(struct extcon_dev *edev)
{
	int ret, index = 0;
	static atomic_t edev_no = ATOMIC_INIT(-1);

	if (!extcon_class) {
		ret = create_extcon_class();
		if (ret < 0)
			return ret;
	}

	if (!edev || !edev->supported_cable)
		return -EINVAL;

	for (; edev->supported_cable[index] != EXTCON_NONE; index++);

	edev->max_supported = index;
	if (index > SUPPORTED_CABLE_MAX) {
		dev_err(&edev->dev,
			"exceed the maximum number of supported cables\n");
		return -EINVAL;
	}

	edev->dev.class = extcon_class;
	edev->dev.release = extcon_dev_release;

	edev->name = dev_name(edev->dev.parent);
	if (IS_ERR_OR_NULL(edev->name)) {
		dev_err(&edev->dev,
			"extcon device name is null\n");
		return -EINVAL;
	}
	dev_set_name(&edev->dev, "extcon%lu",
			(unsigned long)atomic_inc_return(&edev_no));

	if (edev->max_supported) {
		char buf[10];
		char *str;
		struct extcon_cable *cable;

		edev->cables = kzalloc(sizeof(struct extcon_cable) *
				       edev->max_supported, GFP_KERNEL);
		if (!edev->cables) {
			ret = -ENOMEM;
			goto err_sysfs_alloc;
		}
		for (index = 0; index < edev->max_supported; index++) {
			cable = &edev->cables[index];

			snprintf(buf, 10, "cable.%d", index);
			str = kzalloc(sizeof(char) * (strlen(buf) + 1),
				      GFP_KERNEL);
			if (!str) {
				for (index--; index >= 0; index--) {
					cable = &edev->cables[index];
					kfree(cable->attr_g.name);
				}
				ret = -ENOMEM;

				goto err_alloc_cables;
			}
			strcpy(str, buf);

			cable->edev = edev;
			cable->cable_index = index;
			cable->attrs[0] = &cable->attr_name.attr;
			cable->attrs[1] = &cable->attr_state.attr;
			cable->attrs[2] = NULL;
			cable->attr_g.name = str;
			cable->attr_g.attrs = cable->attrs;

			sysfs_attr_init(&cable->attr_name.attr);
			cable->attr_name.attr.name = "name";
			cable->attr_name.attr.mode = 0444;
			cable->attr_name.show = cable_name_show;

			sysfs_attr_init(&cable->attr_state.attr);
			cable->attr_state.attr.name = "state";
			cable->attr_state.attr.mode = 0444;
			cable->attr_state.show = cable_state_show;
		}
	}

	if (edev->max_supported && edev->mutually_exclusive) {
		char buf[80];
		char *name;

		/* Count the size of mutually_exclusive array */
		for (index = 0; edev->mutually_exclusive[index]; index++)
			;

		edev->attrs_muex = kzalloc(sizeof(struct attribute *) *
					   (index + 1), GFP_KERNEL);
		if (!edev->attrs_muex) {
			ret = -ENOMEM;
			goto err_muex;
		}

		edev->d_attrs_muex = kzalloc(sizeof(struct device_attribute) *
					     index, GFP_KERNEL);
		if (!edev->d_attrs_muex) {
			ret = -ENOMEM;
			kfree(edev->attrs_muex);
			goto err_muex;
		}

		for (index = 0; edev->mutually_exclusive[index]; index++) {
			sprintf(buf, "0x%x", edev->mutually_exclusive[index]);
			name = kzalloc(sizeof(char) * (strlen(buf) + 1),
				       GFP_KERNEL);
			if (!name) {
				for (index--; index >= 0; index--) {
					kfree(edev->d_attrs_muex[index].attr.
					      name);
				}
				kfree(edev->d_attrs_muex);
				kfree(edev->attrs_muex);
				ret = -ENOMEM;
				goto err_muex;
			}
			strcpy(name, buf);
			sysfs_attr_init(&edev->d_attrs_muex[index].attr);
			edev->d_attrs_muex[index].attr.name = name;
			edev->d_attrs_muex[index].attr.mode = 0000;
			edev->attrs_muex[index] = &edev->d_attrs_muex[index]
							.attr;
		}
		edev->attr_g_muex.name = muex_name;
		edev->attr_g_muex.attrs = edev->attrs_muex;

	}

	if (edev->max_supported) {
		edev->extcon_dev_type.groups =
			kzalloc(sizeof(struct attribute_group *) *
				(edev->max_supported + 2), GFP_KERNEL);
		if (!edev->extcon_dev_type.groups) {
			ret = -ENOMEM;
			goto err_alloc_groups;
		}

		edev->extcon_dev_type.name = dev_name(&edev->dev);
		edev->extcon_dev_type.release = dummy_sysfs_dev_release;

		for (index = 0; index < edev->max_supported; index++)
			edev->extcon_dev_type.groups[index] =
				&edev->cables[index].attr_g;
		if (edev->mutually_exclusive)
			edev->extcon_dev_type.groups[index] =
				&edev->attr_g_muex;

		edev->dev.type = &edev->extcon_dev_type;
	}

	ret = device_register(&edev->dev);
	if (ret) {
		put_device(&edev->dev);
		goto err_dev;
	}
#if defined(CONFIG_ANDROID)
	if (switch_class)
		ret = class_compat_create_link(switch_class, &edev->dev, NULL);
#endif /* CONFIG_ANDROID */

	spin_lock_init(&edev->lock);

	edev->nh = devm_kzalloc(&edev->dev,
			sizeof(*edev->nh) * edev->max_supported, GFP_KERNEL);
	if (!edev->nh) {
		ret = -ENOMEM;
		device_unregister(&edev->dev);
		goto err_dev;
	}

	for (index = 0; index < edev->max_supported; index++)
		RAW_INIT_NOTIFIER_HEAD(&edev->nh[index]);

	dev_set_drvdata(&edev->dev, edev);
	edev->state = 0;

	mutex_lock(&extcon_dev_list_lock);
	list_add(&edev->entry, &extcon_dev_list);
	mutex_unlock(&extcon_dev_list_lock);

	return 0;

err_dev:
	if (edev->max_supported)
		kfree(edev->extcon_dev_type.groups);
err_alloc_groups:
	if (edev->max_supported && edev->mutually_exclusive) {
		for (index = 0; edev->mutually_exclusive[index]; index++)
			kfree(edev->d_attrs_muex[index].attr.name);
		kfree(edev->d_attrs_muex);
		kfree(edev->attrs_muex);
	}
err_muex:
	for (index = 0; index < edev->max_supported; index++)
		kfree(edev->cables[index].attr_g.name);
err_alloc_cables:
	if (edev->max_supported)
		kfree(edev->cables);
err_sysfs_alloc:
	return ret;
}
EXPORT_SYMBOL_GPL(extcon_dev_register);

/**
 * extcon_dev_unregister() - Unregister the extcon device.
 * @edev:	the extcon device instance to be unregistered.
 *
 * Note that this does not call kfree(edev) because edev was not allocated
 * by this class.
 */
void extcon_dev_unregister(struct extcon_dev *edev)
{
	int index;

	if (!edev)
		return;

	mutex_lock(&extcon_dev_list_lock);
	list_del(&edev->entry);
	mutex_unlock(&extcon_dev_list_lock);

	if (IS_ERR_OR_NULL(get_device(&edev->dev))) {
		dev_err(&edev->dev, "Failed to unregister extcon_dev (%s)\n",
				dev_name(&edev->dev));
		return;
	}

	device_unregister(&edev->dev);

	if (edev->mutually_exclusive && edev->max_supported) {
		for (index = 0; edev->mutually_exclusive[index];
				index++)
			kfree(edev->d_attrs_muex[index].attr.name);
		kfree(edev->d_attrs_muex);
		kfree(edev->attrs_muex);
	}

	for (index = 0; index < edev->max_supported; index++)
		kfree(edev->cables[index].attr_g.name);

	if (edev->max_supported) {
		kfree(edev->extcon_dev_type.groups);
		kfree(edev->cables);
	}

#if defined(CONFIG_ANDROID)
	if (switch_class)
		class_compat_remove_link(switch_class, &edev->dev, NULL);
#endif
	put_device(&edev->dev);
}
EXPORT_SYMBOL_GPL(extcon_dev_unregister);

static void devm_extcon_dev_unreg(struct device *dev, void *res)
{
	extcon_dev_unregister(*(struct extcon_dev **)res);
}

/**
 * devm_extcon_dev_register() - Resource-managed extcon_dev_register()
 * @dev:	device to allocate extcon device
 * @edev:	the new extcon device to register
 *
 * Managed extcon_dev_register() function. If extcon device is attached with
 * this function, that extcon device is automatically unregistered on driver
 * detach. Internally this function calls extcon_dev_register() function.
 * To get more information, refer that function.
 *
 * If extcon device is registered with this function and the device needs to be
 * unregistered separately, devm_extcon_dev_unregister() should be used.
 *
 * Returns 0 if success or negaive error number if failure.
 */
int devm_extcon_dev_register(struct device *dev, struct extcon_dev *edev)
{
	struct extcon_dev **ptr;
	int ret;

	ptr = devres_alloc(devm_extcon_dev_unreg, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	ret = extcon_dev_register(edev);
	if (ret) {
		devres_free(ptr);
		return ret;
	}

	*ptr = edev;
	devres_add(dev, ptr);

	return 0;
}
EXPORT_SYMBOL_GPL(devm_extcon_dev_register);

/**
 * devm_extcon_dev_unregister() - Resource-managed extcon_dev_unregister()
 * @dev:	device the extcon belongs to
 * @edev:	the extcon device to unregister
 *
 * Unregister extcon device that is registered with devm_extcon_dev_register()
 * function.
 */
void devm_extcon_dev_unregister(struct device *dev, struct extcon_dev *edev)
{
	WARN_ON(devres_release(dev, devm_extcon_dev_unreg,
			       devm_extcon_dev_match, edev));
}
EXPORT_SYMBOL_GPL(devm_extcon_dev_unregister);

#ifdef CONFIG_OF
/*
 * extcon_get_edev_by_phandle - Get the extcon device from devicetree
 * @dev - instance to the given device
 * @index - index into list of extcon_dev
 *
 * return the instance of extcon device
 */
struct extcon_dev *extcon_get_edev_by_phandle(struct device *dev, int index)
{
	struct device_node *node;
	struct extcon_dev *edev;

	if (!dev)
		return ERR_PTR(-EINVAL);

	if (!dev->of_node) {
		dev_err(dev, "device does not have a device node entry\n");
		return ERR_PTR(-EINVAL);
	}

	node = of_parse_phandle(dev->of_node, "extcon", index);
	if (!node) {
		dev_err(dev, "failed to get phandle in %s node\n",
			dev->of_node->full_name);
		return ERR_PTR(-ENODEV);
	}

	mutex_lock(&extcon_dev_list_lock);
	list_for_each_entry(edev, &extcon_dev_list, entry) {
		if (edev->dev.parent && edev->dev.parent->of_node == node) {
			mutex_unlock(&extcon_dev_list_lock);
			return edev;
		}
	}
	mutex_unlock(&extcon_dev_list_lock);

	return ERR_PTR(-EPROBE_DEFER);
}
#else
struct extcon_dev *extcon_get_edev_by_phandle(struct device *dev, int index)
{
	return ERR_PTR(-ENOSYS);
}
#endif /* CONFIG_OF */
EXPORT_SYMBOL_GPL(extcon_get_edev_by_phandle);

/**
 * extcon_get_edev_name() - Get the name of the extcon device.
 * @edev:	the extcon device
 */
const char *extcon_get_edev_name(struct extcon_dev *edev)
{
	return !edev ? NULL : edev->name;
}

static int __init extcon_class_init(void)
{
	return create_extcon_class();
}
module_init(extcon_class_init);

static void __exit extcon_class_exit(void)
{
#if defined(CONFIG_ANDROID)
	class_compat_unregister(switch_class);
#endif
	class_destroy(extcon_class);
}
module_exit(extcon_class_exit);

MODULE_AUTHOR("Chanwoo Choi <cw00.choi@samsung.com>");
MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_AUTHOR("Donggeun Kim <dg77.kim@samsung.com>");
MODULE_AUTHOR("MyungJoo Ham <myungjoo.ham@samsung.com>");
MODULE_DESCRIPTION("External connector (extcon) class driver");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              #
# Sensor  drivers configuration
#
menuconfig SENSORS_FINGERPRINT
	bool "Finger Print Sensor devices"
	help
	  Say Y here, and a list of sensors drivers will be displayed.
	  Everything that didn't fit into the other categories is here. This option
	  doesn't affect the kernel.
	  If unsure, say Y.

config SENSORS_VFS7XXX
	tristate "VFS7XXX fingerprint sensor support"
	default n
	help
	  If you say yes here you get support for Validity's
	  fingerprint sensor VFS7XXX.
config SENSORS_FPRINT_SECURE
	tristate "VFS61XX fingerprint sensor support"
	default n
	help
	  If you say yes here you get support for Validity's
	  fingerprint sensor enable secure zone.
config SENSORS_FINGERPRINT_32BITS_PLATFORM_ONLY
	tristate "Fingerprint sensor supports only 32bits platform"
	default n
	help
	  If you say yes here the non TZ device driver will only supports
	  32bits platform.
config SENSORS_ET320
	tristate "ET320 fingerprint sensor supprot"
	default n
	help
	  If you say yes here you get support for Egistec's
	  fingerprint sensor ET320.
config SENSORS_ET510
	tristate "ET510 fingerprint sensor supprot"
	default n
	help
	  If you say yes here you get support for Egistec's
	  fingerprint sensor ET510.
config SENSORS_ET5XX
	tristate "ET5XX fingerprint sensor supprot"
	default n
	help
	  If you say yes here you get support for Egistec's
	  fingerprint sensor ET5XX.
config SENSORS_GW32X
	tristate "generic goodix fingerprint driver"
	default n
	help
	  add support for goodix fingerprint driver.
config SENSORS_ET7XX
	tristate "ET7XX fingerprint sensor support"
	default n
	help
	  If you say yes here you get support for Egistec's
	  fingerprint sensor ET7XX.
config SENSORS_FINGERPRINT_DUALIZATION
	tristate "Fingerprint sensor supports dualization et320 and viper2"
	default n
	help
	  If you say yes here vendor pin check will be enabled.
config SENSORS_FP_LOCKSCREEN_MODE
	tristate "fingerprint sensor support fast wake up"
	default n
	help
	  If you say yes here you can use lockscreen mode for optimizing
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

ccflags-y := $(KBUILD_FP_SENSOR_CFLAGS)

obj-$(CONFIG_SENSORS_FINGERPRINT)		+= fingerprint_sysfs.o
obj-$(CONFIG_SENSORS_VFS7XXX)			+= vfs7xxx.o
obj-$(CONFIG_SENSORS_ET320)			+= et320-spi.o et320-spi_data_transfer.o
obj-$(CONFIG_SENSORS_ET510)			+= et510-spi.o et510-spi_data_transfer.o
obj-$(CONFIG_SENSORS_ET5XX)			+= et5xx-spi.o et5xx-spi_data_transfer.o
obj-$(CONFIG_SENSORS_ET7XX)			+= et7xx-spi.o et7xx-spi_data_transfer.o
obj-$(CONFIG_SENSORS_GW32X)			+= gf_common.o gf_platform.o                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "fingerprint.h"
#include "et5xx.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#ifdef ENABLE_SENSORS_FPRINT_SECURE
#include <linux/smc.h>
#endif
#include <linux/sysfs.h>

#include <linux/pinctrl/consumer.h>
#include "../pinctrl/core.h"

static DECLARE_BITMAP(minors, N_SPI_MINORS);

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

#ifdef ENABLE_SENSORS_FPRINT_SECURE
int fpsensor_goto_suspend =0;
#endif

static int gpio_irq;
static struct etspi_data *g_data;
static DECLARE_WAIT_QUEUE_HEAD(interrupt_waitq);
static unsigned int bufsiz = 1024;
module_param(bufsiz, uint, 0444);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

#if defined(ENABLE_SENSORS_FPRINT_SECURE)
int fps_resume_set(void) {
	int ret =0;

	if (fpsensor_goto_suspend) {
		fpsensor_goto_suspend = 0;
#if defined(CONFIG_TZDEV)
		if (!g_data->ldo_enabled) {
			ret = exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWEROFF, 0);
			pr_info("etspi %s: FP_SET_POWEROFF ret = %d\n", __func__, ret);
		} else {
			ret = exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWERON_INACTIVE, 0);
			pr_info("etspi %s: FP_SET_POWERON_INACTIVE ret = %d\n", __func__, ret);
		}
#else
		ret = exynos_smc(MC_FC_FP_PM_RESUME, 0, 0, 0);
		pr_info("etspi %s : smc ret = %d\n", __func__, ret);
#endif
	}
	return ret;
}
#endif

static irqreturn_t etspi_fingerprint_interrupt(int irq, void *dev_id)
{
	struct etspi_data *etspi = (struct etspi_data *)dev_id;

	etspi->int_count++;
	etspi->finger_on = 1;
	disable_irq_nosync(gpio_irq);
	wake_up_interruptible(&interrupt_waitq);
	wake_lock_timeout(&etspi->fp_signal_lock, 1 * HZ);
	pr_info("%s FPS triggered.int_count(%d) On(%d)\n", __func__,
		etspi->int_count, etspi->finger_on);
	etspi->interrupt_count++;
	return IRQ_HANDLED;
}

int etspi_Interrupt_Init(
		struct etspi_data *etspi,
		int int_ctrl,
		int detect_period,
		int detect_threshold)
{
	int status = 0;

	etspi->finger_on = 0;
	etspi->int_count = 0;
	pr_info("%s int_ctrl = %d detect_period = %d detect_threshold = %d\n",
				__func__,
				int_ctrl,
				detect_period,
				detect_threshold);

	etspi->detect_period = detect_period;
	etspi->detect_threshold = detect_threshold;
	gpio_irq = gpio_to_irq(etspi->drdyPin);

	if (gpio_irq < 0) {
		pr_err("%s gpio_to_irq failed\n", __func__);
		status = gpio_irq;
		goto done;
	}

	if (etspi->drdy_irq_flag == (DRDY_IRQ_DISABLE | IRQF_PERF_CRITICAL)) {
		if (request_irq
			(gpio_irq, etspi_fingerprint_interrupt
			, int_ctrl, "etspi_irq", etspi) < 0) {
			pr_err("%s drdy request_irq failed\n", __func__);
			status = -EBUSY;
			goto done;
		} else {
			enable_irq_wake(gpio_irq);
			etspi->drdy_irq_flag = DRDY_IRQ_ENABLE | IRQF_PERF_CRITICAL;
		}
	}
done:
	return status;
}

int etspi_Interrupt_Free(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);

	if (etspi != NULL) {
		if (etspi->drdy_irq_flag == (DRDY_IRQ_ENABLE | IRQF_PERF_CRITICAL)) {
			if (!etspi->int_count)
				disable_irq_nosync(gpio_irq);

			disable_irq_wake(gpio_irq);
			free_irq(gpio_irq, etspi);
			etspi->drdy_irq_flag = DRDY_IRQ_DISABLE | IRQF_PERF_CRITICAL;
		}
		etspi->finger_on = 0;
		etspi->int_count = 0;
	}
	return 0;
}

void etspi_Interrupt_Abort(struct etspi_data *etspi)
{
	wake_up_interruptible(&interrupt_waitq);
}

unsigned int etspi_fps_interrupt_poll(
		struct file *file,
		struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct etspi_data *etspi = file->private_data;

	pr_debug("%s FPS fps_interrupt_poll, finger_on(%d), int_count(%d)\n",
		__func__, etspi->finger_on, etspi->int_count);

	if (!etspi->finger_on)
		poll_wait(file, &interrupt_waitq, wait);

	if (etspi->finger_on) {
		mask |= POLLIN | POLLRDNORM;
		etspi->finger_on = 0;
	}
	return mask;
}

/*-------------------------------------------------------------------------*/

static void etspi_reset(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);

	gpio_set_value(etspi->sleepPin, 0);
	usleep_range(1050, 1100);
	gpio_set_value(etspi->sleepPin, 1);
	etspi->reset_count++;
}

static void etspi_pin_control(struct etspi_data *etspi,
	bool pin_set)
{
	int status = 0;

	etspi->p->state = NULL;
	if (pin_set) {
		if (!IS_ERR(etspi->pins_poweron)) {
			status = pinctrl_select_state(etspi->p,
				etspi->pins_poweron);
			if (status)
				pr_err("%s: can't set pin default state\n",
					__func__);
			pr_info("%s idle\n", __func__);
		}
	} else {
		if (!IS_ERR(etspi->pins_poweroff)) {
			status = pinctrl_select_state(etspi->p,
				etspi->pins_poweroff);
			if (status)
				pr_err("%s: can't set pin sleep state\n",
					__func__);
			pr_info("%s sleep\n", __func__);
		}
	}
}

static void etspi_power_control(struct etspi_data *etspi, int status)
{
	pr_info("%s status = %d\n", __func__, status);
	if (status == 1) {
		etspi_pin_control(etspi, 1);
		if (etspi->ldo_pin) {
			gpio_set_value(etspi->ldo_pin, 1);
#if defined(ENABLE_SENSORS_FPRINT_SECURE) && defined(CONFIG_TZDEV)
			if(!etspi->ldo_enabled)
				pr_info("%s: FP_SET_POWERON_INACTIVE ret = %d\n", __func__, 
					exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWERON_INACTIVE, 0));
#endif
			etspi->ldo_enabled = 1;
		}
		usleep_range(1100, 1150);
		if (etspi->sleepPin)
			gpio_set_value(etspi->sleepPin, 1);
		usleep_range(10000, 10050);
	} else if (status == 0) {
#if defined(ENABLE_SENSORS_FPRINT_SECURE)
#if defined(CONFIG_TZDEV)
		pr_info("%s: FP_SET_POWEROFF ret = %d\n", __func__, 
			exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWEROFF, 0));
#else
		pr_info("%s: cs_set smc ret = %d\n", __func__,
			exynos_smc(MC_FC_FP_CS_SET, 0, 0, 0));
#endif
#endif
		if (etspi->sleepPin)
			gpio_set_value(etspi->sleepPin, 0);
		if (etspi->ldo_pin) {
			gpio_set_value(etspi->ldo_pin, 0);
			etspi->ldo_enabled = 0;
		}

		etspi_pin_control(etspi, 0);
	} else {
		pr_err("%s can't support this value. %d\n", __func__, status);
	}
}

static ssize_t etspi_read(struct file *filp,
						char __user *buf,
						size_t count,
						loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}

static ssize_t etspi_write(struct file *filp,
						const char __user *buf,
						size_t count,
						loff_t *f_pos)
{
/*Implement by vendor if needed*/
	return 0;
}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
static int etspi_sec_spi_prepare(struct sec_spi_info *spi_info,
		struct spi_device *spi)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	struct clk *fp_spi_dma;
	int ret = 0;
#endif

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");
	if (IS_ERR(fp_spi_pclk)) {
		pr_err("%s Can't get fp_spi_pclk\n", __func__);
		return PTR_ERR(fp_spi_pclk);
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");
	if (IS_ERR(fp_spi_sclk)) {
		pr_err("%s Can't get fp_spi_sclk\n", __func__);
		return PTR_ERR(fp_spi_sclk);
	}
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	fp_spi_dma = clk_get(NULL, "apb_pclk");
	if (IS_ERR(fp_spi_dma)) {
		pr_err("%s Can't get apb_pclk\n", __func__);
		return PTR_ERR(fp_spi_dma);
	}
#endif
	clk_prepare_enable(fp_spi_pclk);
	clk_prepare_enable(fp_spi_sclk);
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	ret = clk_prepare_enable(fp_spi_dma);
	if (ret) {
		pr_err("%s clk_finger clk_prepare_enable failed %d\n",
			__func__, ret);
		return ret;
	}
#endif
	clk_set_rate(fp_spi_sclk, spi_info->speed * 2);

	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	clk_put(fp_spi_dma);
#endif
	return 0;
}

static int etspi_sec_spi_unprepare(struct sec_spi_info *spi_info,
		struct spi_device *spi)
{
	struct clk *fp_spi_pclk, *fp_spi_sclk;
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	struct clk *fp_spi_dma;
#endif

	fp_spi_pclk = clk_get(NULL, "fp-spi-pclk");
	if (IS_ERR(fp_spi_pclk)) {
		pr_err("%s Can't get fp_spi_pclk\n", __func__);
		return PTR_ERR(fp_spi_pclk);
	}

	fp_spi_sclk = clk_get(NULL, "fp-spi-sclk");
	if (IS_ERR(fp_spi_sclk)) {
		pr_err("%s Can't get fp_spi_sclk\n", __func__);
		return PTR_ERR(fp_spi_sclk);
	}
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	fp_spi_dma = clk_get(NULL, "apb_pclk");
	if (IS_ERR(fp_spi_dma)) {
		pr_err("%s Can't get apb_pclk\n", __func__);
		return PTR_ERR(fp_spi_dma);
	}
#endif
	clk_disable_unprepare(fp_spi_pclk);
	clk_disable_unprepare(fp_spi_sclk);
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	clk_disable_unprepare(fp_spi_dma);
#endif
	clk_put(fp_spi_pclk);
	clk_put(fp_spi_sclk);
#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS7880)
	clk_put(fp_spi_dma);
#endif

	return 0;
}

#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS7570) \
	&& !defined(CONFIG_SOC_EXYNOS7870) && !defined(CONFIG_SOC_EXYNOS7880) \
	&& !defined(CONFIG_SOC_EXYNOS7885)
static struct amba_device *adev_dma;
static int etspi_sec_dma_prepare(struct sec_spi_info *spi_info)
{
	struct device_node *np;

	for_each_compatible_node(np, NULL, "arm,pl330") {
		if (!of_device_is_available(np))
			continue;

		if (!of_dma_secure_mode(np))
			continue;

		adev_dma = of_find_amba_device_by_node(np);
		pr_info("[%s]device_name:%s\n",
			__func__, dev_name(&adev_dma->dev));
		break;
	}

	if (adev_dma == NULL)
		return -1;

	pm_runtime_get_sync(&adev_dma->dev);

	return 0;
}

static int etspi_sec_dma_unprepare(void)
{
	if (adev_dma == NULL)
		return -1;

	pm_runtime_put(&adev_dma->dev);

	return 0;
}
#endif
#endif

static long etspi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, retval = 0;
	struct etspi_data *etspi;
	struct spi_device *spi;
	u32 tmp;
	struct egis_ioc_transfer *ioc = NULL;
#ifdef CONFIG_SENSORS_FINGERPRINT_32BITS_PLATFORM_ONLY
	struct egis_ioc_transfer_32 *ioc_32 = NULL;
	u64 tx_buffer_64, rx_buffer_64;
#endif
	u8 *buf, *address, *result, *fr;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	struct sec_spi_info *spi_info = NULL;
#endif
	/* Check type and command number */
	if (_IOC_TYPE(cmd) != EGIS_IOC_MAGIC) {
		pr_err("%s _IOC_TYPE(cmd) != EGIS_IOC_MAGIC", __func__);
		return -ENOTTY;
	}

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err) {
		pr_err("%s err", __func__);
		return -EFAULT;
	}

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	etspi = filp->private_data;
	spin_lock_irq(&etspi->spi_lock);
	spi = spi_dev_get(etspi->spi);
	spin_unlock_irq(&etspi->spi_lock);

	if (spi == NULL) {
		pr_err("%s spi == NULL", __func__);
		return -ESHUTDOWN;
	}

	mutex_lock(&etspi->buf_lock);

	/* segmented and/or full-duplex I/O request */
	if ((_IOC_NR(cmd)) != (_IOC_NR(EGIS_IOC_MESSAGE(0))) \
					|| (_IOC_DIR(cmd)) != _IOC_WRITE) {
		retval = -ENOTTY;
		goto out;
	}

	/*
	 *	If platform is 32bit and kernel is 64bit
	 *	We will alloc egis_ioc_transfer for 64bit and 32bit
	 *	We use ioc_32(32bit) to get data from user mode.
	 *	Then copy the ioc_32 to ioc(64bit).
	 */
#ifdef CONFIG_SENSORS_FINGERPRINT_32BITS_PLATFORM_ONLY
	tmp = _IOC_SIZE(cmd);
	if ((tmp == 0) || (tmp % sizeof(struct egis_ioc_transfer_32)) != 0) {
		pr_err("%s ioc_32 size error\n", __func__);
		retval = -EINVAL;
		goto out;
	}
	ioc_32 = kmalloc(tmp, GFP_KERNEL);
	if (ioc_32 == NULL) {
		retval = -ENOMEM;
		pr_err("%s ioc_32 kmalloc error\n", __func__);
		goto out;
	}
	if (__copy_from_user(ioc_32, (void __user *)arg, tmp)) {
		retval = -EFAULT;
		pr_err("%s ioc_32 copy_from_user error\n", __func__);
		goto out;
	}
	ioc = kmalloc(sizeof(struct egis_ioc_transfer), GFP_KERNEL);
	if (ioc == NULL) {
		retval = -ENOMEM;
		pr_err("%s ioc kmalloc error\n", __func__);
		goto out;
	}
	tx_buffer_64 = (u64)ioc_32->tx_buf;
	rx_buffer_64 = (u64)ioc_32->rx_buf;
	ioc->tx_buf = (u8 *)tx_buffer_64;
	ioc->rx_buf = (u8 *)rx_buffer_64;
	ioc->len = ioc_32->len;
	ioc->speed_hz = ioc_32->speed_hz;
	ioc->delay_usecs = ioc_32->delay_usecs;
	ioc->bits_per_word = ioc_32->bits_per_word;
	ioc->cs_change = ioc_32->cs_change;
	ioc->opcode = ioc_32->opcode;
	memcpy(ioc->pad, ioc_32->pad, 3);
	kfree(ioc_32);
#else
	tmp = _IOC_SIZE(cmd);
	if ((tmp == 0) || (tmp % sizeof(struct egis_ioc_transfer)) != 0) {
		pr_err("%s ioc size error\n", __func__);
		retval = -EINVAL;
		goto out;
	}
	/* copy into scratch area */
	ioc = kmalloc(tmp, GFP_KERNEL);
	if (!ioc) {
		retval = -ENOMEM;
		goto out;
	}
	if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
		pr_err("%s __copy_from_user error\n", __func__);
		retval = -EFAULT;
		goto out;
	}
#endif

	switch (ioc->opcode) {
	/*
	 * Read register
	 * tx_buf include register address will be read
	 */
	case FP_REGISTER_READ:
		address = ioc->tx_buf;
		result = ioc->rx_buf;
		pr_debug("etspi FP_REGISTER_READ\n");

		retval = etspi_io_read_register(etspi, address, result);
		if (retval < 0)	{
			pr_err("%s FP_REGISTER_READ error retval = %d\n"
			, __func__, retval);
		}
		break;

	/*
	 * Write data to register
	 * tx_buf includes address and value will be wrote
	 */
	case FP_REGISTER_WRITE:
		buf = ioc->tx_buf;
		pr_debug("%s FP_REGISTER_WRITE\n", __func__);

		retval = etspi_io_write_register(etspi, buf);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_WRITE error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_MREAD:
		address = ioc->tx_buf;
		result = ioc->rx_buf;
		pr_debug("%s FP_REGISTER_MREAD\n", __func__);
		retval = etspi_io_read_registerex(etspi, address, result,
				ioc->len);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_MREAD error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BREAD:
		pr_debug("%s FP_REGISTER_BREAD\n", __func__);
		retval = etspi_io_burst_read_register(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BREAD error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BWRITE:
		pr_debug("%s FP_REGISTER_BWRITE\n", __func__);
		retval = etspi_io_burst_write_register(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BWRITE error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BREAD_BACKWARD:
		pr_debug("%s FP_REGISTER_BREAD_BACKWARD\n", __func__);
		retval = etspi_io_burst_read_register_backward(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BREAD_BACKWARD error retval = %d\n"
				, __func__, retval);
		}
		break;
	case FP_REGISTER_BWRITE_BACKWARD:
		pr_debug("%s FP_REGISTER_BWRITE_BACKWARD\n", __func__);
		retval = etspi_io_burst_write_register_backward(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BWRITE_BACKWARD error retval = %d\n"
				, __func__, retval);
		}
		break;
	case FP_NVM_READ:
		pr_debug("%s FP_NVM_READ, (%d)\n", __func__, spi->max_speed_hz);
		retval = etspi_io_nvm_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_READ error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_WRITE:
		pr_debug("%s FP_NVM_WRITE, (%d)\n", __func__,
				spi->max_speed_hz);
		retval = etspi_io_nvm_write(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_WRITE error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_WRITEEX:
		pr_debug("%s FP_NVM_WRITEEX, (%d)\n", __func__,
				spi->max_speed_hz);
		retval = etspi_io_nvm_writeex(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_WRITEEX error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_OFF:
		pr_debug("%s FP_NVM_OFF\n", __func__);
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_VDM_READ:
		pr_debug("%s FP_VDM_READ\n", __func__);
		retval = etspi_io_vdm_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_VDM_READ error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_VDM_READ finished.\n", __func__);
		}
		break;
	case FP_VDM_WRITE:
		pr_debug("%s FP_VDM_WRITE\n", __func__);
		retval = etspi_io_vdm_write(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_VDM_WRITE error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_VDM_WRTIE finished.\n", __func__);
		}
		break;
	/*
	 * Get one frame data from sensor
	 */
	case FP_GET_ONE_IMG:
		fr = ioc->rx_buf;
		pr_debug("%s FP_GET_ONE_IMG\n", __func__);

		retval = etspi_io_get_frame(etspi, fr, ioc->len);
		if (retval < 0) {
			pr_err("%s FP_GET_ONE_IMG error retval = %d\n"
			, __func__, retval);
		}
		break;

	case FP_SENSOR_RESET:
		pr_info("%s FP_SENSOR_RESET\n", __func__);
		etspi_reset(etspi);
		break;

	case FP_RESET_SET:
		break;


	case FP_POWER_CONTROL:
	case FP_POWER_CONTROL_ET5XX:
		pr_info("%s FP_POWER_CONTROL, status = %d\n", __func__,
				ioc->len);
		etspi_power_control(etspi, ioc->len);
		break;

	case FP_SET_SPI_CLOCK:
		pr_info("%s FP_SET_SPI_CLOCK, clock = %d\n", __func__,
				ioc->speed_hz);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		if (etspi->enabled_clk) {
			if (spi->max_speed_hz == ioc->speed_hz) {
				pr_info("%s already enabled same clock.\n",
					__func__);
				break;
			}
			pr_info("%s already enabled. DISABLE_SPI_CLOCK\n",
				__func__);
			retval = etspi_sec_spi_unprepare(spi_info, spi);
			if (retval < 0)
				pr_err("%s: couldn't disable spi clks\n",
					__func__);
#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS7570) \
	&& !defined(CONFIG_SOC_EXYNOS7870) && !defined(CONFIG_SOC_EXYNOS7880) \
	&& !defined(CONFIG_SOC_EXYNOS7885)
			retval = etspi_sec_dma_unprepare();
			if (retval < 0)
				pr_err("%s: couldn't disable spi dma\n",
					__func__);
#endif
#ifdef FEATURE_SPI_WAKELOCK
			wake_unlock(&etspi->fp_spi_lock);
#endif
			etspi->enabled_clk = false;
		}
		spi->max_speed_hz = ioc->speed_hz;
		spi_info = kmalloc(sizeof(struct sec_spi_info),
			GFP_KERNEL);
		if (spi_info != NULL) {
			pr_info("%s ENABLE_SPI_CLOCK\n", __func__);

			spi_info->speed = spi->max_speed_hz;
			retval = etspi_sec_spi_prepare(spi_info, spi);
			if (retval < 0)
				pr_err("%s: Unable to enable spi clk\n",
					__func__);
#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS7570) \
	&& !defined(CONFIG_SOC_EXYNOS7870) && !defined(CONFIG_SOC_EXYNOS7880) \
	&& !defined(CONFIG_SOC_EXYNOS7885)
			retval = etspi_sec_dma_prepare(spi_info);
			if (retval < 0)
				pr_err("%s: Unable to enable spi dma\n",
					__func__);
#endif
			kfree(spi_info);
#ifdef FEATURE_SPI_WAKELOCK
			wake_lock(&etspi->fp_spi_lock);
#endif
			etspi->enabled_clk = true;
		} else
			retval = -ENOMEM;
#else
		spi->max_speed_hz = ioc->speed_hz;
#endif
		break;

	/*
	 * Trigger initial routine
	 */
	case INT_TRIGGER_INIT:
		pr_debug("%s Trigger function init\n", __func__);
		retval = etspi_Interrupt_Init(
				etspi,
				(int)ioc->pad[0],
				(int)ioc->pad[1],
				(int)ioc->pad[2]);
		break;
	/* trigger */
	case INT_TRIGGER_CLOSE:
		pr_debug("%s Trigger function close\n", __func__);
		retval = etspi_Interrupt_Free(etspi);
		break;
	/* Poll Abort */
	case INT_TRIGGER_ABORT:
		pr_debug("%s Trigger function abort\n", __func__);
		etspi_Interrupt_Abort(etspi);
		break;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	case FP_DISABLE_SPI_CLOCK:
		pr_info("%s FP_DISABLE_SPI_CLOCK\n", __func__);

		if (etspi->enabled_clk) {
			pr_info("%s DISABLE_SPI_CLOCK\n", __func__);

			retval = etspi_sec_spi_unprepare(spi_info, spi);
			if (retval < 0)
				pr_err("%s: couldn't disable spi clks\n",
					__func__);
#if !defined(CONFIG_SOC_EXYNOS8890) && !defined(CONFIG_SOC_EXYNOS7570) \
	&& !defined(CONFIG_SOC_EXYNOS7870) && !defined(CONFIG_SOC_EXYNOS7880) \
	&& !defined(CONFIG_SOC_EXYNOS7885)
			retval = etspi_sec_dma_unprepare();
			if (retval < 0)
				pr_err("%s: couldn't disable spi dma\n",
					__func__);
#endif
#ifdef FEATURE_SPI_WAKELOCK
			wake_unlock(&etspi->fp_spi_lock);
#endif
			etspi->enabled_clk = false;
		}
		break;
	case FP_CPU_SPEEDUP:
		pr_info("%s FP_CPU_SPEEDUP\n", __func__);

		if (ioc->len) {
			u8 retry_cnt = 0;

			pr_info("%s FP_CPU_SPEEDUP ON:%d, retry: %d\n",
				__func__, ioc->len, retry_cnt);
#if defined(CONFIG_SECURE_OS_BOOSTER_API)
			do {
				retval = secos_booster_start(ioc->len - 1);
				retry_cnt++;
				if (retval) {
					pr_err("%s: booster start failed. (%d) retry: %d\n"
						, __func__, retval, retry_cnt);
					if (retry_cnt < 7)
						usleep_range(500, 510);
				}
			} while (retval && retry_cnt < 7);
#elif defined(CONFIG_TZDEV_BOOST)
			tz_boost_enable();
#endif
		} else {
			pr_info("%s FP_CPU_SPEEDUP OFF\n", __func__);
#if defined(CONFIG_SECURE_OS_BOOSTER_API)
			retval = secos_booster_stop();
			if (retval)
				pr_err("%s: booster stop failed. (%d)\n"
					, __func__, retval);
#elif defined(CONFIG_TZDEV_BOOST)
			tz_boost_disable();
#endif
		}
		break;
	case FP_SET_SENSOR_TYPE:
		if ((int)ioc->len >= SENSOR_OOO &&
				(int)ioc->len < SENSOR_MAXIMUM) {
			if ((int)ioc->len == SENSOR_OOO &&
					etspi->sensortype == SENSOR_FAILED) {
				pr_info("%s maintain type check from out of order :%s\n",
					__func__,
					sensor_status[g_data->sensortype + 2]);
			} else {
				etspi->sensortype = (int)ioc->len;
				pr_info("%s FP_SET_SENSOR_TYPE :%s\n",
					__func__,
					sensor_status[g_data->sensortype + 2]);
			}
		} else {
			pr_err("%s FP_SET_SENSOR_TYPE invalid value %d\n",
					__func__, (int)ioc->len);
			etspi->sensortype = SENSOR_UNKNOWN;
		}
		break;
	case FP_SET_LOCKSCREEN:
		pr_info("%s FP_SET_LOCKSCREEN\n", __func__);
		break;
	case FP_SET_WAKE_UP_SIGNAL:
		pr_info("%s FP_SET_WAKE_UP_SIGNAL\n", __func__);
		break;
#endif
	case FP_SENSOR_ORIENT:
		pr_info("%s: orient is %d", __func__, etspi->orient);

		retval = put_user(etspi->orient, (u8 __user *) (uintptr_t)ioc->rx_buf);
		if (retval != 0)
			pr_err("%s FP_SENSOR_ORIENT put_user fail: %d\n", __func__, retval);
		break;

	case FP_SPI_VALUE:
		etspi->spi_value = ioc->len;
		pr_info("%s spi_value: 0x%x\n", __func__,etspi->spi_value);
			break;
	case FP_IOCTL_RESERVED_01:
	case FP_IOCTL_RESERVED_02:
			break;
	default:
		retval = -EFAULT;
		break;

	}

out:
	if (ioc != NULL)
		kfree(ioc);

	mutex_unlock(&etspi->buf_lock);
	spi_dev_put(spi);
	if (retval < 0)
		pr_err("%s retval = %d\n", __func__, retval);
	return retval;
}

#ifdef CONFIG_COMPAT
static long etspi_compat_ioctl(struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	return etspi_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define etspi_compat_ioctl NULL
#endif
/* CONFIG_COMPAT */

static int etspi_open(struct inode *inode, struct file *filp)
{
	struct etspi_data *etspi;
	int	status = -ENXIO;

	pr_info("%s\n", __func__);
	mutex_lock(&device_list_lock);

	list_for_each_entry(etspi, &device_list, device_entry) {
		if (etspi->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (etspi->buf == NULL) {
			etspi->buf = kmalloc(bufsiz, GFP_KERNEL);
			if (etspi->buf == NULL) {
				dev_dbg(&etspi->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			etspi->users++;
			filp->private_data = etspi;
			nonseekable_open(inode, filp);
			etspi->bufsiz = bufsiz;
		}
	} else
		pr_debug("%s nothing for minor %d\n"
			, __func__, iminor(inode));

	mutex_unlock(&device_list_lock);
	return status;
}

static int etspi_release(struct inode *inode, struct file *filp)
{
	struct etspi_data *etspi;

	pr_info("%s\n", __func__);
	mutex_lock(&device_list_lock);
	etspi = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	etspi->users--;
	if (etspi->users == 0) {
		int	dofree;

		kfree(etspi->buf);
		etspi->buf = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&etspi->spi_lock);
		dofree = (etspi->spi == NULL);
		spin_unlock_irq(&etspi->spi_lock);

		if (dofree)
			kfree(etspi);
	}
	mutex_unlock(&device_list_lock);

	return 0;
}

int etspi_platformInit(struct etspi_data *etspi)
{
	int status = 0;

	pr_info("%s\n", __func__);
	/* gpio setting for ldo, ldo2, sleep, drdy pin */
	if (etspi != NULL) {
		etspi->drdy_irq_flag = DRDY_IRQ_DISABLE | IRQF_PERF_CRITICAL;

		if (etspi->ldo_pin) {
			status = gpio_request(etspi->ldo_pin, "etspi_ldo_en");
			if (status < 0) {
				pr_err("%s gpio_request etspi_ldo_en failed\n",
					__func__);
				goto etspi_platformInit_ldo_failed;
			}
			gpio_direction_output(etspi->ldo_pin, 0);
			etspi->ldo_enabled = 0;
		}
		status = gpio_request(etspi->sleepPin, "etspi_sleep");
		if (status < 0) {
			pr_err("%s gpio_requset etspi_sleep failed\n",
				__func__);
			goto etspi_platformInit_sleep_failed;
		}

		gpio_direction_output(etspi->sleepPin, 0);
		if (status < 0) {
			pr_err("%s gpio_direction_output SLEEP failed\n",
					__func__);
			status = -EBUSY;
			goto etspi_platformInit_sleep_failed;
		}

		status = gpio_request(etspi->drdyPin, "etspi_drdy");
		if (status < 0) {
			pr_err("%s gpio_request etspi_drdy failed\n",
				__func__);
			goto etspi_platformInit_drdy_failed;
		}

		status = gpio_direction_input(etspi->drdyPin);
		if (status < 0) {
			pr_err("%s gpio_direction_input DRDY failed\n",
				__func__);
			goto etspi_platformInit_gpio_init_failed;
		}

		pr_info("%s sleep value =%d\n"
				"%s ldo en value =%d\n",
				__func__, gpio_get_value(etspi->sleepPin),
				__func__, gpio_get_value(etspi->ldo_pin));
	} else {
		status = -EFAULT;
	}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
#ifdef FEATURE_SPI_WAKELOCK
	wake_lock_init(&etspi->fp_spi_lock,
		WAKE_LOCK_SUSPEND, "etspi_wake_lock");
#endif
#endif
	wake_lock_init(&etspi->fp_signal_lock,
				WAKE_LOCK_SUSPEND, "etspi_sigwake_lock");

	pr_info("%s successful status=%d\n", __func__, status);
	return status;
etspi_platformInit_gpio_init_failed:
	gpio_free(etspi->drdyPin);
etspi_platformInit_drdy_failed:
	gpio_free(etspi->sleepPin);
etspi_platformInit_sleep_failed:
	gpio_free(etspi->ldo_pin);
etspi_platformInit_ldo_failed:
	pr_err("%s is failed\n", __func__);
	return status;
}

void etspi_platformUninit(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);

	if (etspi != NULL) {
		disable_irq_wake(gpio_irq);
		disable_irq(gpio_irq);
		free_irq(gpio_irq, etspi);
		etspi->drdy_irq_flag = DRDY_IRQ_DISABLE | IRQF_PERF_CRITICAL;
		if (etspi->ldo_pin)
			gpio_free(etspi->ldo_pin);
		gpio_free(etspi->sleepPin);
		gpio_free(etspi->drdyPin);
#ifndef ENABLE_SENSORS_FPRINT_SECURE
#ifdef CONFIG_SOC_EXYNOS8890
		if (etspi->cs_gpio)
			gpio_free(etspi->cs_gpio);
#endif
#endif
#ifdef ENABLE_SENSORS_FPRINT_SECURE
#ifdef FEATURE_SPI_WAKELOCK
		wake_lock_destroy(&etspi->fp_spi_lock);
#endif
#endif
		wake_lock_destroy(&etspi->fp_signal_lock);
	}
}

static int etspi_parse_dt(struct device *dev,
	struct etspi_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;
	int gpio;

#ifndef ENABLE_SENSORS_FPRINT_SECURE
#ifdef CONFIG_SOC_EXYNOS8890
	gpio = of_get_named_gpio_flags(np, "etspi-csgpio",
		0, &flags);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get csgpio\n", __func__);
		goto dt_exit;
	} else {
		data->cs_gpio = gpio;
		pr_info("%s: cs_gpio=%d\n",
			__func__, data->cs_gpio);
	}
#endif
#endif
	gpio = of_get_named_gpio_flags(np, "etspi-sleepPin",
		0, &flags);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get sleepPin\n", __func__);
		goto dt_exit;
	} else {
		data->sleepPin = gpio;
		pr_info("%s: sleepPin=%d\n",
			__func__, data->sleepPin);
	}
	gpio = of_get_named_gpio_flags(np, "etspi-drdyPin",
		0, &flags);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get drdyPin\n", __func__);
		goto dt_exit;
	} else {
		data->drdyPin = gpio;
		pr_info("%s: drdyPin=%d\n",
			__func__, data->drdyPin);
	}
	gpio = of_get_named_gpio_flags(np, "etspi-ldoPin",
		0, &flags);
	if (gpio < 0) {
		data->ldo_pin = 0;
		pr_err("%s: fail to get ldo_pin\n", __func__);
	} else {
		data->ldo_pin = gpio;
		pr_info("%s: ldo_pin=%d\n",
			__func__, data->ldo_pin);
	}

	if (of_property_read_string_index(np, "etspi-chipid", 0,
			(const char **)&data->chipid)) {
		data->chipid = NULL;
	}
	pr_info("%s: chipid: %s\n", __func__, data->chipid);

	if (of_property_read_u32(np, "etspi-orient", &data->orient))
		data->orient = 0;
	pr_info("%s: orient: %d\n", __func__, data->orient);

	data->p = pinctrl_get_select_default(dev);
	if (IS_ERR(data->p)) {
		errorno = -EINVAL;
		pr_err("%s: failed pinctrl_get\n", __func__);
		goto dt_exit;
	}

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	data->pins_poweroff = pinctrl_lookup_state(data->p, "pins_poweroff");
#else
	data->pins_poweroff = pinctrl_lookup_state(data->p, "pins_poweroff_tz");
#endif
	if (IS_ERR(data->pins_poweroff)) {
		pr_err("%s : could not get pins sleep_state (%li)\n",
			__func__, PTR_ERR(data->pins_poweroff));
		goto fail_pinctrl_get;
	}

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	data->pins_poweron = pinctrl_lookup_state(data->p, "pins_poweron");
#else
	data->pins_poweron = pinctrl_lookup_state(data->p, "pins_poweron_tz");
#endif
	if (IS_ERR(data->pins_poweron)) {
		pr_err("%s : could not get pins idle_state (%li)\n",
			__func__, PTR_ERR(data->pins_poweron));
		goto fail_pinctrl_get;
	}

	pr_info("%s is successful\n", __func__);
	return errorno;
fail_pinctrl_get:
		pinctrl_put(data->p);
dt_exit:
	pr_err("%s is failed\n", __func__);
	return errorno;
}

static const struct file_operations etspi_fops = {
	.owner = THIS_MODULE,
	.write = etspi_write,
	.read = etspi_read,
	.unlocked_ioctl = etspi_ioctl,
	.compat_ioctl = etspi_compat_ioctl,
	.open = etspi_open,
	.release = etspi_release,
	.llseek = no_llseek,
	.poll = etspi_fps_interrupt_poll
};

#ifndef ENABLE_SENSORS_FPRINT_SECURE
static int etspi_type_check(struct etspi_data *etspi)
{
	u8 buf1, buf2, buf3, buf4, buf5, buf6, buf7;

	etspi_power_control(g_data, 1);

	msleep(20);
	etspi_read_register(etspi, 0x00, &buf1);
	if (buf1 != 0xAA) {
		etspi->sensortype = SENSOR_FAILED;
		pr_info("%s sensor not ready, status = %x\n", __func__, buf1);
		etspi_power_control(g_data, 0);
		return -ENODEV;
	}

	etspi_read_register(etspi, 0xFD, &buf1);
	etspi_read_register(etspi, 0xFE, &buf2);
	etspi_read_register(etspi, 0xFF, &buf3);

	etspi_read_register(etspi, 0x20, &buf4);
	etspi_read_register(etspi, 0x21, &buf5);
	etspi_read_register(etspi, 0x23, &buf6);
	etspi_read_register(etspi, 0x24, &buf7);

	etspi_power_control(g_data, 0);

	pr_info("%s buf1-7: %x, %x, %x, %x, %x, %x, %x\n",
		__func__, buf1, buf2, buf3, buf4, buf5, buf6, buf7);

	/*
	 * type check return value
	 * ET510C : 0X00 / 0X66 / 0X00 / 0X33
	 * ET510D : 0x03 / 0x0A / 0x05
	 * ET516B : 0x01 or 0x02 / 0x10 / 0x05
	 * ET520  : 0x03 / 0x14 / 0x05
	 * ET520E  : 0x04 / 0x14 / 0x05
	 * ET523  : 0x00 / 0x17 / 0x05
	 */
	if (((buf1 == 0x01) || (buf1 == 0x02))
		&& (buf2 == 0x10) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET516B sensor\n", __func__);
	} else if ((buf1 == 0x03) && (buf2 == 0x0A) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET510D sensor\n", __func__);
	} else  if ((buf1 == 0x03) && (buf2 == 0x14) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET520 sensor\n", __func__);
	} else if ((buf1 == 0x04) && (buf2 == 0x14) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET520E sensor\n", __func__);
	} else if((buf1 == 0x00) && (buf2 == 0x17) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET523 sensor\n", __func__);
	} else {
		if ((buf4 == 0x00) && (buf5 == 0x66)
				&& (buf6 == 0x00) && (buf7 == 0x33)) {
			etspi->sensortype = SENSOR_EGIS;
			pr_info("%s sensor type is EGIS ET510C sensor\n",
					__func__);
		} else {
			etspi->sensortype = SENSOR_FAILED;
			pr_info("%s sensor type is FAILED\n", __func__);
			return -ENODEV;
		}
	}
	return 0;
}
#endif

static ssize_t etspi_bfs_values_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			data->spi->max_speed_hz);
}

static ssize_t etspi_type_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	int retry = 0;
	int status = 0;

	do {
		status = etspi_type_check(data);
		pr_info("%s type (%u), retry (%d)\n"
			, __func__, data->sensortype, retry);
	} while (!data->sensortype && ++retry < 3);

	if (status == -ENODEV)
		pr_info("%s type check fail\n", __func__);
#endif
	return snprintf(buf, PAGE_SIZE, "%d\n", data->sensortype);
}

static ssize_t etspi_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t etspi_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", g_data->chipid);
}

static ssize_t etspi_adm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", DETECT_ADM);
}

static ssize_t etspi_intcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->interrupt_count);
}

static ssize_t etspi_intcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct etspi_data *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		data->interrupt_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t etspi_resetcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->reset_count);
}

static ssize_t etspi_resetcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct etspi_data *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		data->reset_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static DEVICE_ATTR(bfs_values, 0444, etspi_bfs_values_show, NULL);
static DEVICE_ATTR(type_check, 0444, etspi_type_check_show, NULL);
static DEVICE_ATTR(vendor, 0444, etspi_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, etspi_name_show, NULL);
static DEVICE_ATTR(adm, 0444, etspi_adm_show, NULL);
static DEVICE_ATTR(intcnt, 0664, etspi_intcnt_show, etspi_intcnt_store);
static DEVICE_ATTR(resetcnt, 0664, etspi_resetcnt_show, etspi_resetcnt_store);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_intcnt,
	&dev_attr_resetcnt,
	NULL,
};

static void etspi_work_func_debug(struct work_struct *work)
{
	u8 ldo_value = 0;

	if (g_data->ldo_pin)
		ldo_value = gpio_get_value(g_data->ldo_pin);

	pr_info("%s ldo: %d, sleep: %d, tz: %d, spi_value: 0x%x, type: %s\n",
		__func__,
		ldo_value, gpio_get_value(g_data->sleepPin),
		g_data->tz_mode, g_data->spi_value,
		sensor_status[g_data->sensortype + 2]);
}

static void etspi_enable_debug_timer(void)
{
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static void etspi_disable_debug_timer(void)
{
	del_timer_sync(&g_data->dbg_timer);
	cancel_work_sync(&g_data->work_debug);
}

static void etspi_timer_func(unsigned long ptr)
{
	queue_work(g_data->wq_dbg, &g_data->work_debug);
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static int etspi_set_timer(struct etspi_data *etspi)
{
	int status = 0;

	setup_timer(&etspi->dbg_timer,
		etspi_timer_func, (unsigned long)etspi);
	etspi->wq_dbg =
		create_singlethread_workqueue("etspi_debug_wq");
	if (!etspi->wq_dbg) {
		status = -ENOMEM;
		pr_err("%s could not create workqueue\n", __func__);
		return status;
	}
	INIT_WORK(&etspi->work_debug, etspi_work_func_debug);
	return status;
}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
#ifdef CONFIG_SOC_EXYNOS8890
static int etspi_set_cs_gpio(struct etspi_data *etspi,
	struct s3c64xx_spi_csinfo *cs)
{
	int status = -1;

	pr_info("%s, spi auto cs mode(%d)\n", __func__, cs->cs_mode);

	if (etspi->cs_gpio) {
		cs->line = etspi->cs_gpio;
		if (!gpio_is_valid(cs->line))
			cs->line = 0;
	} else {
		cs->line = 0;
	}

	if (cs->line != 0) {
		status = gpio_request_one(cs->line, GPIOF_OUT_INIT_HIGH,
					dev_name(&etspi->spi->dev));
		if (status) {
			dev_err(&etspi->spi->dev,
				"Failed to get /CS gpio [%d]: %d\n",
				cs->line, status);
		}
	}

	return status;
}
#endif
#endif

/*-------------------------------------------------------------------------*/

static struct class *etspi_class;

/*-------------------------------------------------------------------------*/

static int etspi_probe(struct spi_device *spi)
{
	struct etspi_data *etspi;
	int status;
	unsigned long minor;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	int retry = 0;
#ifdef CONFIG_SOC_EXYNOS8890
	struct s3c64xx_spi_csinfo *cs;
#endif
#endif

	pr_info("%s\n", __func__);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	fpsensor_goto_suspend = 0;
#endif
	/* Allocate driver data */
	etspi = kzalloc(sizeof(*etspi), GFP_KERNEL);
	if (!etspi)
		return -ENOMEM;

	/* device tree call */
	if (spi->dev.of_node) {
		status = etspi_parse_dt(&spi->dev, etspi);
		if (status) {
			pr_err("%s - Failed to parse DT\n", __func__);
			goto etspi_probe_parse_dt_failed;
		}
	}

	/* Initialize the driver data */
	etspi->spi = spi;
	g_data = etspi;

	spin_lock_init(&etspi->spi_lock);
	mutex_init(&etspi->buf_lock);
	mutex_init(&device_list_lock);

	INIT_LIST_HEAD(&etspi->device_entry);

	/* platform init */
	status = etspi_platformInit(etspi);
	if (status != 0) {
		pr_err("%s platforminit failed\n", __func__);
		goto etspi_probe_platformInit_failed;
	}

	spi->bits_per_word = 8;
	spi->max_speed_hz = SLOW_BAUD_RATE;
	spi->mode = SPI_MODE_0;
	spi->chip_select = 0;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
#ifdef CONFIG_SOC_EXYNOS8890
	/* set cs pin in fp driver, use only Exynos8890 */
	/* for use auto cs mode with dualization fp sensor */
	cs = spi->controller_data;

	if (cs->cs_mode == 1)
		status = etspi_set_cs_gpio(etspi, cs);
	else
		pr_info("%s, spi manual mode(%d)\n", __func__, cs->cs_mode);
#endif

	status = spi_setup(spi);
	if (status != 0) {
		pr_err("%s spi_setup() is failed. status : %d\n",
			__func__, status);
		return status;
	}
#endif
	etspi->spi_value = 0;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	etspi->sensortype = SENSOR_UNKNOWN;
#else
	/* sensor hw type check */
	do {
		status = etspi_type_check(etspi);
		pr_info("%s type (%u), retry (%d)\n"
			, __func__, etspi->sensortype, retry);
	} while (!etspi->sensortype && ++retry < 3);

	if (status == -ENODEV)
		pr_info("%s type check fail\n", __func__);
#endif

#if defined(DISABLED_GPIO_PROTECTION)
	etspi_pin_control(etspi, 0);
#endif

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	etspi->tz_mode = true;
#endif
	etspi->reset_count = 0;
	etspi->interrupt_count = 0;
	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		etspi->devt = MKDEV(ET5XX_MAJOR, (unsigned int)minor);
		dev = device_create(etspi_class, &spi->dev,
				etspi->devt, etspi, "esfp0");
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&etspi->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	if (status == 0)
		spi_set_drvdata(spi, etspi);
	else
		goto etspi_create_failed;

	status = fingerprint_register(etspi->fp_device,
		etspi, fp_attrs, "fingerprint");
	if (status) {
		pr_err("%s sysfs register failed\n", __func__);
		goto etspi_register_failed;
	}

	status = etspi_set_timer(etspi);
	if (status)
		goto etspi_sysfs_failed;
	etspi_enable_debug_timer();
	pr_info("%s is successful\n", __func__);

	return status;

etspi_sysfs_failed:
	fingerprint_unregister(etspi->fp_device, fp_attrs);

etspi_register_failed:
	device_destroy(etspi_class, etspi->devt);
	class_destroy(etspi_class);
etspi_create_failed:
	etspi_platformUninit(etspi);
etspi_probe_platformInit_failed:
etspi_probe_parse_dt_failed:
	kfree(etspi);
	pr_err("%s is failed\n", __func__);

	return status;
}

static int etspi_remove(struct spi_device *spi)
{
	struct etspi_data *etspi = spi_get_drvdata(spi);

	pr_info("%s\n", __func__);

	if (etspi != NULL) {
		etspi_disable_debug_timer();
		etspi_platformUninit(etspi);

		/* make sure ops on existing fds can abort cleanly */
		spin_lock_irq(&etspi->spi_lock);
		etspi->spi = NULL;
		spi_set_drvdata(spi, NULL);
		spin_unlock_irq(&etspi->spi_lock);

		/* prevent new opens */
		mutex_lock(&device_list_lock);
		fingerprint_unregister(etspi->fp_device, fp_attrs);

		list_del(&etspi->device_entry);
		device_destroy(etspi_class, etspi->devt);
		clear_bit(MINOR(etspi->devt), minors);
		if (etspi->users == 0)
			kfree(etspi);
		mutex_unlock(&device_list_lock);
	}
	return 0;
}

static int etspi_pm_suspend(struct device *dev)
{
#if defined(ENABLE_SENSORS_FPRINT_SECURE)
	int ret = 0;
#endif

	pr_info("%s\n", __func__);

	if (g_data != NULL) {
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		fpsensor_goto_suspend = 1; /* used by pinctrl_samsung.c */
#endif
		etspi_disable_debug_timer();

		if (!g_data->ldo_enabled) {
#if defined(ENABLE_SENSORS_FPRINT_SECURE)
#if defined(CONFIG_TZDEV)
			ret = exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWEROFF, 0);
			pr_info("%s: FP_SET_POWEROFF ret = %d\n", __func__, ret);
#else
			ret = exynos_smc(MC_FC_FP_PM_SUSPEND, 0, 0, 0);
			pr_info("%s: suspend smc ret = %d\n", __func__, ret);
#endif
#endif
		} else {
#if defined(ENABLE_SENSORS_FPRINT_SECURE)
#if defined(CONFIG_TZDEV)
			ret = exynos_smc(FP_CSMC_HANDLER_ID, FP_HANDLER_MAIN, FP_SET_POWERON_INACTIVE, 0);
			pr_info("%s: FP_SET_POWERON_INACTIVE ret = %d\n", __func__, ret);
#else
			ret = exynos_smc(MC_FC_FP_PM_SUSPEND_CS_HIGH, 0, 0, 0);
			pr_info("%s: suspend_cs_high smc ret = %d\n",
				__func__, ret);
#endif
#endif
		}
	}
	return 0;
}

static int etspi_pm_resume(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (g_data != NULL) {
		etspi_enable_debug_timer();
#if defined(ENABLE_SENSORS_FPRINT_SECURE)
		if (fpsensor_goto_suspend) {
			fps_resume_set();
		}
#endif
	}
	return 0;
}

static const struct dev_pm_ops etspi_pm_ops = {
	.suspend = etspi_pm_suspend,
	.resume = etspi_pm_resume
};

static const struct of_device_id etspi_match_table[] = {
	{ .compatible = "etspi,et5xx",},
	{},
};

static struct spi_driver etspi_spi_driver = {
	.driver = {
		.name =	"egis_fingerprint",
		.owner = THIS_MODULE,
		.pm = &etspi_pm_ops,
		.of_match_table = etspi_match_table
	},
	.probe = etspi_probe,
	.remove = etspi_remove,
};

/*-------------------------------------------------------------------------*/

static int __init etspi_init(void)
{
	int status;

	pr_info("%s\n", __func__);

#if defined(CONFIG_SENSORS_FINGERPRINT_DUALIZATION) \
		&& defined(CONFIG_SENSORS_VFS7XXX)
	/* vendor check */
	pr_info("%s FP_CHECK value (%d)\n", __func__, FP_CHECK);
	if (FP_CHECK) {
		pr_err("%s It is not egis sensor\n", __func__);
		return -ENODEV;
	}
#endif
	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_