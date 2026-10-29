/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Samsung 990 Pro 2TB
// MPNs: MZ-V9P2T0GW, MZ-V9P2T0CW

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_samsung_mzv9p2t0gw_init(struct hw_dev *dev, const char *id,
				   enum hw_locale locale, uint8_t efficiency)
{
	dev->mpn = SYSPWR_HW_MPN_SAMSUNG_MZV9P2T0GW;
	CFG_STRNCPY(dev->name, "samsung_MZV9P2T0GW", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_STORAGE_SYSFS_BW;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 55 * 1000;
	dev->unverified_props.uw_max = 61 * 100 * 1000;
	return 0;
}
