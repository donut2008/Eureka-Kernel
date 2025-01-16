ice_attribute a1;
		struct sensor_device_attribute_2 a2;
	} u;
	char name[32];
};

#define __TEMPLATE_ATTR(_template, _mode, _show, _store) {	\
	.attr = {.name = _template, .mode = _mode },		\
	.show	= _show,					\
	.store	= _store,					\
}

#define SENSOR_DEVICE_TEMPLATE(_template, _mode, _show, _store, _index)	\
	{ .dev_attr = __TEMPLATE_ATTR(_template, _mode, _show, _store),	\
	  .u.index = _index,						\
	  .s2 = false }

#define SENSOR_DEVICE_TEMPLATE_2(_template, _mode, _show, _store,	\
				 _nr, _index)				\
	{ .dev_attr = __TEMPLATE_ATTR(_template, _mode, _show, _store),	\
	  .u.s.index = _index,						\
	  .u.s.nr = _nr,						\
	  .s2 = true }

#define SENSOR_TEMPLATE(_name, _template, _mode, _show, _store, _index)	\
static struct sensor_device_template sensor_dev_template_##_name	\
	= SENSOR_DEVICE_TEMPLATE(_template, _mode, _show, _store,	\
				 _index)

#define SENSOR_TEMPLATE_2(_name, _template, _mode, _show, _store,	\
			  _nr, _index)					\
static struct sensor_device_template sensor_dev_template_##_name	\
	= SENSOR_DEVICE_TEMPLATE_2(_template, _mode, _show, _store,	\
				 _nr, _index)

struct sensor_template_group {
	struct sensor_device_template **templates;
	umode_t (*is_visible)(struct kobject *, struct attribute *, int);
	int base;
};

static struct attribute_group *
nct6775_create_attr_group(struct device *dev, struct sensor_template_group *tg,
			  int repeat)
{
	struct attribute_group *group;
	struct sensor_device_attr_u *su;
	struct sensor_device_attribute *a;
	struct sensor_device_attribute_2 *a2;
	struct attribute **attrs;
	struct sensor_device_template **t;
	int i, count;

	if (repeat <= 0)
		return ERR_PTR(-EINVAL);

	t = tg->templates;
	for (count = 0; *t; t++, count++)
		;

	if (count == 0)
		return ERR_PTR(-EINVAL);

	group = devm_kzalloc(dev, sizeof(*group), GFP_KERNEL);
	if (group == NULL)
		return ERR_PTR(-ENOMEM);

	attrs = devm_kzalloc(dev, sizeof(*attrs) * (repeat * count + 1),
			     GFP_KERNEL);
	if (attrs == NULL)
		return ERR_PTR(-ENOMEM);

	su = devm_kzalloc(dev, sizeof(*su) * repeat * co