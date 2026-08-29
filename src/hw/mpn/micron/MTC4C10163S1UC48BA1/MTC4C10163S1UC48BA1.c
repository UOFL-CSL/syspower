/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Micron 8GB 4800MHz DDR5 UDIMM
// MPNs: MTC4C10163S1UC48B, MTC4C10163S1UC48BA1

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/mdl/mdl.h>

// external
#include LIB_INT
#include LIB_STRING

int hw_mpn_micron_mtc4c10163s1uc48ba1_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
) {
	dev->mpn = SYSPWR_HW_MPN_MICRON_MTC4C10163S1UC48BA1;
	CFG_STRNCPY(dev->name, "micron_MTC4C10163S1UC48BA1", HW_DEV_NAME_SIZE);
	CFG_STRNCPY(dev->id, id, HW_DEV_ID_SIZE);
	dev->locale = locale;
	dev->efficiency = efficiency;
	dev->models[0] = SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE;
	dev->verified_props.uw_min = 0;
	dev->verified_props.uw_max = 0;
	dev->unverified_props.uw_min = 155 * 1000;
	dev->unverified_props.uw_max = 317 * 10 * 1000;

	return 0;
}

