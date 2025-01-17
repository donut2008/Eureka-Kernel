/*
 * Defines interfaces for interacting wtih the Raspberry Pi firmware's
 * property channel.
 *
 * Copyright © 2015 Broadcom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/dma-mapping.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <soc/bcm2835/raspberrypi-firmware.h>

#define MBOX_MSG(chan, data28)		(((data28) & ~0xf) | ((chan) & 0xf))
#define MBOX_CHAN(msg)			((msg) & 0xf)
#define MBOX_DATA28(msg)		((msg) & ~0xf)
#define MBOX_CHAN_PROPERTY		8

struct rpi_firmware {
	struct mbox_client cl;
	struct mbox_chan *chan; /* The property channel. */
	struct completion c;
	u32 enabled;
};

static DEFINE_MUTEX(transaction_lock);

static void response_callback(struct mbox_client *cl, void *msg)
{
	struct rpi_firmware *fw = container_of(cl, struct rpi_firmware, cl);
	complete(&fw->c);
}

/*
 * Sends a request to the firmware through the BCM2835 mailbox driver,
 * and synchronously waits for the reply.
 */
static int
rpi_firmware_transaction(struct rpi_firmware *fw, u32 chan, u32 data)
{
	u32 message = MBOX_MSG(chan, data);
	int ret;

	WARN_ON(data & 0xf);

	mutex_lock(&transaction_lock);
	reinit_completion(&fw->c);
	ret = mbox_send_message(fw->chan, &message);
	if (ret >= 0) {
		wait_for_completion(&fw->c);
		ret = 0;
	} else {
		dev_err(fw->cl.dev, "mbox_send_message returned %d\n", ret);
	}
	mutex_unlock(&transaction_lock);

	return ret;
}

/**
 * rpi_firmware_property_list - Submit firmware property list
 * @fw:		Pointer to firmware structure from rpi_firmware_get().
 * @data:	Buffer holding tags.
 * @tag_size:	Size of tags buffer.
 *
 * Submits a set of concatenated tags to the VPU firmware through the
 * mailbox property interface.
 *
 * The buffer header and the ending tag are added by this function and
 * don't need to be supplied, just the actual tags for your operation.
 * See struct rpi_firmware_property_tag_header for the per-tag
 * structure.
 */
int rpi_firmware_property_list(struct rpi_firmware *fw,
			       void *data, size_t tag_size)
{
	size_t size = tag_size + 12;
	u32 *buf;
	dma_addr_t bus_addr;
	int ret;

	/* Packets are processed a dword at a time. */
	if (size & 3)
		return -EINVAL;

	buf = dma_alloc_coherent(fw->cl.dev, PAGE_ALIGN(size), &bus_addr,
				 GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	/* The firmware will error out without parsing in this case. */
	WARN_ON(size >= 1024 * 1024);

	buf[0] = size;
	buf[1] = RPI_FIRMWARE_STATUS_REQUEST;
	memcpy(&buf[2], data, tag_size);
	buf[size / 4 - 1] = RPI_FIRMWARE_PROPERTY_END;
	wmb();

	ret = rpi_firmware_transaction(fw, MBOX_CHAN_PROPERTY, bus_addr);

	rmb();
	memcpy(data, &buf[2], tag_size);
	if (ret == 0 && buf[1] != RPI_FIRMWARE_STATUS_SUCCESS) {
		/*
		 * The tag name here might not be the one causing the
		 * error, if there were multiple tags in the request.
		 * But single-tag is the most common, so go with it.
		 */
		dev_err(fw->cl.dev, "Request 0x%08x returned status 0x%08x\n",
			buf[2], buf[1]);
		ret = -EINVAL;
	}

	dma_free_coherent(fw->cl.dev, PAGE_ALIGN(size), buf, bus_addr);

	return ret;
}
EXPORT_SYMBOL_GPL(rpi_firmware_property_list);

/**
 * rpi_firmware_property - Submit single firmware property
 * @fw:		Pointer to firmware structure from rpi_firmware_get().
 * @tag:	One of enum_mbox_property_tag.
 * @tag_data:	Tag data buffer.
 * @buf_size:	Buffer size.
 *
 * Submits a single tag to the VPU firmware through the mailbox
 * property interface.
 *
 * This is a convenience wrapper around
 * rpi_firmware_property_list() to avoid some of the
 * boilerplate in property calls.
 */
int rpi_firmware_property(struct rpi_firmware *fw,
			  u32 tag, void *tag_data, size_t buf_size)
{
	/* Single tags are very small (generally 8 bytes), so the
	 * stack should be safe.
	 */
	u8 data[buf_size + sizeof(struct rpi_firmware_property_tag_header)];
	struct rpi_firmware_property_tag_header *header =
		(struct rpi_firmware_property_tag_header *)data;
	int ret;

	header->tag = tag;
	header->buf_size = buf_size;
	header->req_resp_size = 0;
	memcpy(data + sizeof(struct rpi_firmware_property_tag_header),
	       tag_data, buf_size);

	ret = rpi_firmware_property_list(fw, &data, sizeof(data));
	memcpy(tag_data,
	       data + sizeof(struct rpi_firmware_property_tag_header),
	       buf_size);

	return ret;
}
EXPORT_SYMBOL_GPL(rpi_firmware_property);

static void
rpi_firmware_print_firmware_revision(struct rpi_firmware *fw)
{
	u32 packet;
	int ret = rpi_firmware_property(fw,
					RPI_FIRMWARE_GET_FIRMWARE_REVISION,
					&packet, sizeof(packet));

	if (ret == 0) {
		struct tm tm;

		time_to_tm(packet, 0, &tm);

		dev_info(fw->cl.dev,
			 "Attached to firmware from %04ld-%02d-%02d %02d:%02d\n",
			 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			 tm.tm_hour, tm.tm_min);
	}
}

static int rpi_firmware_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rpi_firmware *fw;

	fw = devm_kzalloc(dev, sizeof(*fw), GFP_KERNEL);
	if (!fw)
		return -ENOMEM;

	fw->cl.dev = dev;
	fw->cl.rx_callback = response_callback;
	fw->cl.tx_block = true;

	fw->chan = mbox_request_channel(&fw->cl, 0);
	if (IS_ERR(fw->chan)) {
		int ret = PTR_ERR(fw->chan);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get mbox channel: %d\n", ret);
		return ret;
	}

	init_completion(&fw->c);

	platform_set_drvdata(pdev, fw);

	rpi_firmware_print_firmware_revision(fw);

	return 0;
}

static int rpi_firmware_remove(struct platform_device *pdev)
{
	struct rpi_firmware *fw = platform_get_drvdata(pdev);

	mbox_free_channel(fw->chan);

	return 0;
}

/**
 * rpi_firmware_get - Get pointer to rpi_firmware structure.
 * @firmware_node:    Pointer to the firmware Device Tree node.
 *
 * Returns NULL is the firmware device is not ready.
 */
struct rpi_firmware *rpi_firmware_get(struct device_node *firmware_node)
{
	struct platform_device *pdev = of_find_device_by_node(firmware_node);

	if (!pdev)
		return NULL;

	return platform_get_drvdata(pdev);
}
EXPORT_SYMBOL_GPL(rpi_firmware_get);

static const struct of_device_id rpi_firmware_of_match[] = {
	{ .compatible = "raspberrypi,bcm2835-firmware", },
	{},
};
MODULE_DEVICE_TABLE(of, rpi_firmware_of_match);

static struct platform_driver rpi_firmware_driver = {
	.driver = {
		.name = "raspberrypi-firmware",
		.of_match_table = rpi_firmware_of_match,
	},
	.probe		= rpi_firmware_probe,
	.remove		= rpi_firmware_remove,
};
module_platform_driver(rpi_firmware_driver);

MODULE_AUTHOR("Eric Anholt <eric@anholt.net>");
MODULE_DESCRIPTION("Raspberry Pi firmware driver");
MODULE_LICENSE("GPL v2");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   #
# FMC (ANSI-VITA 57.1) bus support
#

menuconfig FMC
	tristate "FMC support"
	help

	  FMC (FPGA Mezzanine Carrier) is a mechanical and electrical
	  standard for mezzanine cards that plug into a carrier board.
	  This kernel subsystem supports the matching between carrier
	  and mezzanine based on identifiers stored in the internal I2C
	  EEPROM, as well as having carrier-independent drivers.

	  The framework was born outside of the kernel and at this time
	  the off-tree code base is more complete.  Code and documentation
	  is at git://ohwr.org/fmc-projects/fmc-bus.git .

if FMC

config FMC_FAKEDEV
	tristate "FMC fake device (software testing)"
	help
	  This is a fake carrier, bringing a default EEPROM content
	  that can be rewritten at run time and usef for matching
	  mezzanines.

config FMC_TRIVIAL
	tristate "FMC trivial mezzanine driver (software testing)"
	help
	  This is a fake mezzanine driver, to show how FMC works and test it.
	  The driver also handles interrupts (we used it with a real carrier
	  before the mezzanines were produced)

config FMC_WRITE_EEPROM
	tristate "FMC mezzanine driver to write I2C EEPROM"
	help
	  This driver matches every mezzanine device and can write the
	  internal EEPROM of the PCB, using the firmware loader to get
	  its binary and the function carrier->reprogram to actually do it.
	  It is useful when the mezzanines are produced.

config FMC_CHARDEV
	tristate "FMC mezzanine driver that registers a char device"
	help
	  This driver matches every mezzanine device and allows user
	  space to read and write registers using a char device. It
	  can be used to write user-space drivers, or just get
	  acquainted with a mezzanine before writing its specific driver.

endif # FMC
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         