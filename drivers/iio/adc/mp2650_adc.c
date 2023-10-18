// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MP2650 Driver for ADC
 *
 * Copyright 2020 Monolithic Power Systems, Inc
 *
 * Author: Saravanan Sekar <sravanhome@gmail.com>
 */

#include <linux/iio/driver.h>
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/mfd/mp2650.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define	MP2650_REG_ADC_CTRL		0x03
#define	MP2650_REG_BATT_VOLT		0x0e
#define	MP2650_REG_SYSTEM_VOLT		0x0f
#define	MP2650_REG_INPUT_VOLT		0x11
#define	MP2650_REG_BATT_CURRENT		0x12
#define	MP2650_REG_INPUT_CURRENT	0x13

#define	MP2650_ADC_START		BIT(7)
#define	MP2650_ADC_CONTINUOUS		BIT(6)

#define MP2650_MAP(_mp, _mpc) IIO_MAP(#_mp, "mp2650_charger", "mp2650-"_mpc)

#define MP2650_ADC_CHAN(_ch, _type) {				\
	.type = _type,						\
	.indexed = 1,						\
	.datasheet_name = #_ch,					\
	.channel = MP2650_ ## _ch,				\
	.address = MP2650_REG_ ## _ch,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
}

struct mp2650_adc {
	struct regmap *regmap;
	struct device *dev;
};

static struct iio_chan_spec mp2650_channels[] = {
	MP2650_ADC_CHAN(BATT_VOLT, IIO_VOLTAGE),
	MP2650_ADC_CHAN(SYSTEM_VOLT, IIO_VOLTAGE),
	MP2650_ADC_CHAN(INPUT_VOLT, IIO_VOLTAGE),
	MP2650_ADC_CHAN(BATT_CURRENT, IIO_CURRENT),
	MP2650_ADC_CHAN(INPUT_CURRENT, IIO_CURRENT)
};

static struct iio_map mp2650_adc_maps[] = {
	MP2650_MAP(BATT_VOLT, "batt-volt"),
	MP2650_MAP(SYSTEM_VOLT, "system-volt"),
	MP2650_MAP(INPUT_VOLT, "input-volt"),
	MP2650_MAP(BATT_CURRENT, "batt-current"),
	MP2650_MAP(INPUT_CURRENT, "input-current"),
	{ }
};

static int mp2650_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	struct mp2650_adc *info = iio_priv(indio_dev);
	struct mp2650_data *ddata = dev_get_drvdata(info->dev);
	unsigned int rval;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = regmap_read(info->regmap, chan->address, &rval);
		if (ret)
			return ret;

		if (chan->channel == MP2650_INPUT_VOLT &&
		    ddata->chip_id == CHIP_ID_MP2650)
			rval &= GENMASK(6, 0);
		*val = rval;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		switch (chan->channel) {
		case MP2650_BATT_VOLT:
		case MP2650_SYSTEM_VOLT:
			*val = 20;
			return IIO_VAL_INT;

		case MP2650_INPUT_VOLT:
			*val = 60;
			return IIO_VAL_INT;

		case MP2650_BATT_CURRENT:
			*val = 175;
			*val2 = 10;
			return IIO_VAL_FRACTIONAL;

		case MP2650_INPUT_CURRENT:
			*val = 133;
			*val2 = 10;
			return IIO_VAL_FRACTIONAL;

		default:
			return -EINVAL;
		}

	default:
		return -EINVAL;
	}
}

static const struct iio_info mp2650_adc_info = {
	.read_raw = &mp2650_read_raw,
};

static int mp2650_adc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mp2650_data *ddata = dev_get_drvdata(dev->parent);
	struct mp2650_adc *info;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*info));
	if (!indio_dev)
		return -ENOMEM;

	info = iio_priv(indio_dev);
	info->regmap = ddata->regmap;
	info->dev = dev;
	platform_set_drvdata(pdev, indio_dev);

	ret = regmap_update_bits(info->regmap, MP2650_REG_ADC_CTRL,
				MP2650_ADC_START | MP2650_ADC_CONTINUOUS,
				MP2650_ADC_START | MP2650_ADC_CONTINUOUS);
	if (ret) {
		dev_err(dev, "adc enable fail: %d\n", ret);
		return ret;
	}

	ret = iio_map_array_register(indio_dev, mp2650_adc_maps);
	if (ret) {
		dev_err(dev, "IIO maps register fail: %d\n", ret);
		goto fail_disable;
	}

	indio_dev->name = "mp2650-adc";
	indio_dev->channels = mp2650_channels;
	indio_dev->num_channels = ARRAY_SIZE(mp2650_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &mp2650_adc_info;

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "IIO device register fail: %d\n", ret);
		goto fail_map_unregister;
	}

	return 0;

fail_map_unregister:
	iio_map_array_unregister(indio_dev);

fail_disable:
	regmap_update_bits(info->regmap, MP2650_REG_ADC_CTRL,
					 MP2650_ADC_CONTINUOUS, 0);
	regmap_update_bits(info->regmap, MP2650_REG_ADC_CTRL,
					 MP2650_ADC_START, 0);

	return ret;
}

static int mp2650_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct mp2650_adc *info = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);

	iio_map_array_unregister(indio_dev);

	regmap_update_bits(info->regmap, MP2650_REG_ADC_CTRL,
					 MP2650_ADC_CONTINUOUS, 0);
	regmap_update_bits(info->regmap, MP2650_REG_ADC_CTRL,
					 MP2650_ADC_START, 0);

	return 0;
}

static const struct of_device_id mp2650_adc_of_match[] = {
	{ .compatible = "mps,mp2650_adc"},
	{}
};
MODULE_DEVICE_TABLE(of, mp2650_adc_of_match);

static struct platform_driver mp2650_adc_driver = {
	.driver = {
		.name = "mp2650_adc",
		.of_match_table = mp2650_adc_of_match,
	},
	.probe		= mp2650_adc_probe,
	.remove		= mp2650_adc_remove,
};
module_platform_driver(mp2650_adc_driver);

MODULE_AUTHOR("Saravanan Sekar <sravanhome@gmail.com>");
MODULE_DESCRIPTION("MP2650 ADC driver");
MODULE_LICENSE("GPL");
