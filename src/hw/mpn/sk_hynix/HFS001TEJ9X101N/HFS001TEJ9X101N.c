/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// SK Hynix PC801 NVMe 1TB SSD
// MPNs: HFS001TEJ9X101N

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_sk_hynix_hfs001tej9x101n_init(struct hw_dev *dev, const char *id,
					 enum hw_locale locale,
					 uint8_t efficiency)
{
	dev->mpn = SYSPWR_HW_MPN_SK_HYNIX_HFS001TEJ9X101N;
	CFG_STRNCPY(dev->name, "sk_hynix_HFS001TEJ9X101N", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_STORAGE_SYSFS_BW;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 5 * 10 * 1000;
	dev->unverified_props.uw_max = 7 * 1000 * 1000;
	return 0;
}
