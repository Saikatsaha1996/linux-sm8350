/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 Monolithic Power Systems, Inc
 */

#ifndef __MP2650_H__
#define __MP2650_H__

#include <linux/device.h>
#include <linux/regmap.h>

enum mp2xx_chip_id {
	CHIP_ID_MP2650,
	CHIP_ID_MP2733,
};

struct mp2650_data {
	struct device *dev;
	struct regmap *regmap;
	enum mp2xx_chip_id chip_id;
};

enum mp2650_adc_chan {
	MP2650_BATT_VOLT,
	MP2650_SYSTEM_VOLT,
	MP2650_INPUT_VOLT,
	MP2650_BATT_CURRENT,
	MP2650_INPUT_CURRENT,
	MP2650_ADC_CHAN_END
};

#endif
