/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Intel SATA SSD DC S3520 120G
// MPNs: SSDSC2BB120G7R

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_intel_ssdsc2bb120g7r_init(struct hw_dev *dev, const char *id,
					 enum hw_locale locale,
					 uint8_t efficiency)
{
	dev->mpn = SYSPWR_HW_MPN_INTEL_SSDSC2BB120G7R;
	CFG_STRNCPY(dev->name, "intel_ssdsc2bb120g7r", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_STORAGE_SYSFS_BW;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 530 * 1000;
	dev->unverified_props.uw_max = 22 * 100 * 1000;
	return 0;
}
