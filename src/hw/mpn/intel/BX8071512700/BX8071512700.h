/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Intel Core i7-12700
// MPNs: BX8071512700, CM8071504555019

#ifndef HW_MPN_INTEL_BX8071512700_BX8071512700_H__
#define HW_MPN_INTEL_BX8071512700_BX8071512700_H__

// internal
#include <src/hw/hw.h>

// external
#include <stdint.h>
#include <stddef.h>

static const char **hw_mpn_intel_bx8071512700 = (const char *[]){
	"12th Gen Intel(R) Core(TM) i9-12700",
	NULL
};

struct hw_dev hw_mpn_intel_bx8071512700_init(
	const char *id
);

#endif

