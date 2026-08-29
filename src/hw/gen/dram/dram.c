/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// generic_dram

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_gen_dram_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
) {
	dev->mpn = SYSPWR_HW_GEN_DRAM;
	CFG_STRNCPY(dev->name, "generic_dram", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = CFG_GENERIC_DRAM_UW_MIN_OVERRIDE;
	dev->unverified_props.uw_max = CFG_GENERIC_DRAM_UW_MAX_OVERRIDE;
	return 0;
}

