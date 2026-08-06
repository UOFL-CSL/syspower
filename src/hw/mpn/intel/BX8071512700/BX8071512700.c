/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// Intel Core i7-12700
// MPNs: BX8071512700, CM8071504555019

// internal
#include <src/hw/hw.h>

// external
#include <stdint.h>
#include <stddef.h>

struct hw_dev hw_mpn_intel_bx8071512700_init(
	const char *id
) {
	 struct hw_dev dev = {
		.mpn = SYSPWR_HW_MPN_INTEL_BX8071512700,
		.id = id,
		.models = {
			SYSPWR_MDL_RAPL
		},
		.verified_props = {0},
		.unverified_props = {
			.uw_min = 10 * 1000 * 1000,
			.uw_max = 180 * 1000 * 1000,
		}
	};
	return dev;
}

