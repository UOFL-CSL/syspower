/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MPN_INTEL_SSDSC2BB120G7R_SSDSC2BB120G7R_H__
#define HW_MPN_INTEL_SSDSC2BB120G7R_SSDSC2BB120G7R_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_intel_ssdsc2bb120g7r =
	(const char *[]){ "ATA      SSDSC2BB120G7R  ", NULL };

int hw_mpn_intel_ssdsc2bb120g7r_init(struct hw_dev *dev, const char *id,
					 enum hw_locale locale,
					 uint8_t efficiency);

#endif
