/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_GEN_CPU_H__
#define HW_GEN_CPU_H__

//internal
#include <src/hw/hw.h>

// there is no good generic description of a CPU
// expects to attempt specific models then fail gracefully
static const struct hw_dev hw_gen_cpu {
	.mpn = GENERIC_CPU,
	id = NULL,
	models = [
		SYSPWR_MDL_RAPL
	],
	.verified_props = NULL,
	.unverified_props NULL
}

#endif

