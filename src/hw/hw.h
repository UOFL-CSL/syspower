/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_HW_H__
#define HW_HW_H__

//internal
#include <src/dconf.h>

// external
#include LIB_INT
#include LIB_BOOL

enum hw_locale {
	SYSPWR_HW_LOCALE_LOCAL,
	SYSPWR_HW_LOCALE_EXTERNAL,
	SYSPWR_HW_LOCALE_REMOTE
};

enum hw_mpn {
	SYSPWR_HW_PLACEHOLDER,
	SYSPWR_HW_GEN_CPU,
	SYSPWR_HW_GEN_MOTHERBOARD,
	SYSPWR_HW_GEN_DRAM,
	SYSPWR_HW_GEN_STORAGE,
	SYSPWR_HW_GEN_GPU,
	SYSPWR_HW_GEN_NIC,
	SYSPWR_HW_GEN_FAN,
	SYSPWR_HW_MPN_DELL_0H74DC,
	SYSPWR_HW_MPN_DELL_0N0992,
	SYSPWR_HW_MPN_INTEL_BX8071512700,
	SYSPWR_HW_MPN_INTEL_WGI219LM,
	SYSPWR_HW_MPN_MICRON_MTC4C10163S1UC48BA1,
	SYSPWR_HW_MPN_SAMSUNG_MZV9P2T0GW,
	SYSPWR_HW_MPN_SK_HYNIX_HFS001TEJ9X101N
};

struct hw_props {
	int64_t uw_min;
	int64_t uw_max;
};

#define HW_DEV_MATCH_SIZE 256
#define HW_DEV_NAME_SIZE 128
#define HW_DEV_ID_SIZE 128
#define HW_DEV_MODEL_COUNT 128
struct hw_dev {
	enum hw_mpn mpn;
	char name[HW_DEV_NAME_SIZE];
	char id[HW_DEV_ID_SIZE];
	enum hw_locale locale;
	uint8_t efficiency;
	int models[HW_DEV_MODEL_COUNT]; // enum mdl_model
	struct hw_props verified_props;
	struct hw_props unverified_props;
};

bool hw_match(
	const char **dmatch,
	const char **match
);

int hw_dev_init(
	struct hw_dev *dev,
	const char *id,
	const char **match,
	enum hw_locale locale,
	uint8_t efficiency
);

struct hw_props hw_props_best(
	struct hw_dev *dev
);

#endif

