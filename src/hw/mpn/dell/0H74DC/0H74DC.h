/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Dell OEM Nvidia GeForce RTX 3060 12GB
// MPNs: 0H74DC

#ifndef HW_MPN_DELL_0H74DC_0H74DC_H__
#define HW_MPN_DELL_0H74DC_0H74DC_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_dell_0h74dc =
	(const char *[]){ "GA106 [GeForce RTX 3060 Lite Hash Rate]", "0x10de",
			  "0x1028", NULL };

int hw_mpn_dell_0h74dc_init(struct hw_dev *dev, const char *id,
			    enum hw_locale locale, uint8_t efficiency);

#endif
