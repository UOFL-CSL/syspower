/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Samsung SSD 990 Pro 2TB
// MPNs: MZ-V9P2T0GW, MZ-V9P2T0CW

#ifndef HW_MPN_SAMSUNG_MZV9P2T0GW_MZV9P2T0GW_H__
#define HW_MPN_SAMSUNG_MZV9P2T0GW_MZV9P2T0GW_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_samsung_mzv9p2t0gw =
	(const char *[]){ "Samsung SSD 990 PRO with Heatsink 2TB   ", NULL };

int hw_mpn_samsung_mzv9p2t0gw_init(struct hw_dev *dev, const char *id,
				   enum hw_locale locale, uint8_t efficiency);

#endif
