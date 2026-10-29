
/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Intel I219-LM NIC
// MPNs: WGI219LM

#ifndef HW_MPN_INTEL_WGI219LM_WGI219LM_H__
#define HW_MPN_INTEL_WGI219LM_WGI219LM_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_intel_wgi219lm =
	(const char *[]){ "Ethernet Connection (17) I219-LM", "0x8086", NULL };

int hw_mpn_intel_wgi219lm_init(struct hw_dev *dev, const char *id,
			       enum hw_locale locale, uint8_t efficiency);

#endif
