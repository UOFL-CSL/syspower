/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Intel I219-LM NIC
// MPNs: WGI219LM

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_intel_wgi219lm_init(struct hw_dev *dev, const char *id,
			       enum hw_locale locale, uint8_t efficiency)
{
	dev->mpn = SYSPWR_HW_MPN_INTEL_WGI219LM;
	CFG_STRNCPY(dev->name, "intel_WGI219LM", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_NIC_SYSFS_BW;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 0;
	dev->unverified_props.uw_max = 5 * 100 * 1000;
	return 0;
}
