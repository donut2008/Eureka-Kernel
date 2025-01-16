state)
{
	if (!(dev->flags & OCRDMA_FLAGS_LINK_STATUS_INIT)) {
		dev->flags |= OCRDMA_FLAGS_LINK_STATUS_INIT;
		if (!lstate)
			return;
	}

	if (!lstate)
		ocrdma_dispatch_port_error(dev);
	else
		ocrdma_dispatch_port_active(dev);
}

static struct ocrdma_driver ocrdma_drv = {
	.name			= "ocrdma_driver",
	.add			= ocrdma_add,
	.remove			= ocrdma_remove,
	.state_change_handler	= ocrdma_event_handler,
	.be_abi_version		= OCRDMA_BE_ROCE_ABI_VERSION,
};

static int __init ocrdma_init_module(void)
{
	int status;

	ocrdma_init_debugfs();

	status = be_roce_register_driver(&ocrdma_drv);
	if (status)
		goto err_be_reg;

	return 0;

err_be_reg:

	return status;
}

static void __exit ocrdma_exit_module(void)
{
	be_roce_unregister_driver(&ocrdma_drv);
	ocrdma_rem_