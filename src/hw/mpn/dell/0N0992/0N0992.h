/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Dell Precision 3360 Tower OEM Motherboard 
// MPNs: 0N0992

#ifndef HW_MPN_DELL_0N0992_0N0992_H__
#define HW_MPN_DELL_0N0992_0N0992_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_dell_0n0992 = (const char *[]){
	"Dell Inc. 0N0992",
	NULL
};

int hw_mpn_dell_0n0992_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
);

#endif

