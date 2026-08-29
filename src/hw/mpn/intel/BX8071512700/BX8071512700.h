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
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_intel_bx8071512700 = (const char *[]){
	"12th Gen Intel(R) Core(TM) i7-12700",
	NULL
};

int hw_mpn_intel_bx8071512700_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
);

#endif

