/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Intel Core i7-12700
// MPNs: BX8071512700, CM8071504555019

// internal
#include <src/dconf.h>
#include <src/mdl/mdl.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_intel_bx8071512700_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
) {
	dev->mpn = SYSPWR_HW_MPN_INTEL_BX8071512700;
	CFG_STRNCPY(dev->name, "intel_BX8071512700", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_RAPL_CPU;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 10 * 1000 * 1000;
	dev->unverified_props.uw_max = 180 * 1000 * 1000;

	return 0;
}

