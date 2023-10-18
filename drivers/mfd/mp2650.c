// SPDX-License-Identifier: GPL-2.0+
/*
 * MP2650 parent driver for ADC and battery charger
 *
 * Copyright 2020 Monolithic Power Systems, Inc
 *
 * Author: Saravanan Sekar <sravanhome@gmail.com>
 */

#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mp2650.h>
#include <linux/property.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>

static const struct mfd_cell mp2650_cell[] = {
	{
		.name = "mp2650_adc",
		.of_compatible = "mps,mp2650_adc",
	},
	{
		.name = "mp2650_charger",
		.of_compatible = "mps,mp2650_charger",
	}
};

static const struct regmap_config mp2650_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x17,
};

static int mp2650_probe(struct i2c_client *client)
{
	struct mp2650_data *ddata;
	int ret;

	ddata = devm_kzalloc(&client->dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	ddata->dev = &client->dev;
	ddata->chip_id = (uintptr_t)device_get_match_data(&client->dev);
	i2c_set_clientdata(client, ddata);

	ddata->regmap = devm_regmap_init_i2c(client, &mp2650_regmap_config);
	if (IS_ERR(ddata->regmap)) {
		dev_err(ddata->dev, "Failed to allocate regmap\n");
		return PTR_ERR(ddata->regmap);
	}

	ret = devm_mfd_add_devices(ddata->dev, PLATFORM_DEVID_NONE, mp2650_cell,
				   ARRAY_SIZE(mp2650_cell), NULL, 0, NULL);
	if (ret)
		dev_err(ddata->dev, "Failed to register sub-devices %d\n", ret);

	return ret;
}

static const struct of_device_id mp2650_of_match[] = {
	{ .compatible = "mps,mp2650", .data = (void *)CHIP_ID_MP2650 },
	{ .compatible = "mps,mp2733", .data = (void *)CHIP_ID_MP2733 },
	{ }
};
MODULE_DEVICE_TABLE(of, mp2650_of_match);

static struct i2c_driver mp2650_driver = {
	.driver = {
		.name = "mp2650",
		.of_match_table = mp2650_of_match,
	},
	.probe		= mp2650_probe,
};
module_i2c_driver(mp2650_driver);

MODULE_AUTHOR("Saravanan Sekar <sravanhome@gmail.com>");
MODULE_DESCRIPTION("MP2650 Battery charger parent driver");
MODULE_LICENSE("GPL");
