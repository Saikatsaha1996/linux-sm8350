/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 Monolithic Power Systems, Inc
 */

#ifndef __MP2629_H__
#define __MP2629_H__

#include <linux/device.h>
#include <linux/regmap.h>

enum mp2xx_chip_id {
	CHIP_ID_MP2629,
	CHIP_ID_MP2733,
};

struct mp2629_data {
	struct device *dev;
	struct regmap *regmap;
	enum mp2xx_chip_id chip_id;
};

enum mp2629_adc_chan {
	MP2629_BATT_VOLT,
	MP2629_SYSTEM_VOLT,
	MP2629_INPUT_VOLT,
	MP2629_BATT_CURRENT,
	MP2629_INPUT_CURRENT,
	MP2629_ADC_CHAN_END
};

#endif
