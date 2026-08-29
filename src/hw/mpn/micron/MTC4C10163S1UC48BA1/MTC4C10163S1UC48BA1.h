/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MPN_INTEL_MTC4C10163S1UC48BA1_MTC4C10163S1UC48BA1_H__
#define HW_MPN_INTEL_MTC4C10163S1UC48BA1_MTC4C10163S1UC48BA1_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_micron_mtc4c10163s1uc48ba1 = (const char *[]){
	"802C0000802C MTC4C10163S1UC48BA1",
	NULL
};

int hw_mpn_micron_mtc4c10163s1uc48ba1_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
);

#endif

