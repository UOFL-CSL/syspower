/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MPN_SK_HYNIX_HFS001TEJ9X101N_HFS001TEJ9X101N_H__
#define HW_MPN_SK_HYNIX_HFS001TEJ9X101N_HFS001TEJ9X101N_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_mpn_sk_hynix_hfs001tej9x101n =
	(const char *[]){ "PC801 NVMe SK hynix 1TB                 ", NULL };

int hw_mpn_sk_hynix_hfs001tej9x101n_init(struct hw_dev *dev, const char *id,
					 enum hw_locale locale,
					 uint8_t efficiency);

#endif
