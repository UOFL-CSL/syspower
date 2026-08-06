/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_HW_H__
#define HW_HW_H__

//internal
#include <src/mdl/mdl.h>

// external
#include <stdbool.h>
#include <stdint.h>

enum hw_mpn {
	SYSPWR_HW_PLACEHOLDER,
	SYSPWR_HW_MPN_INTEL_BX8071512700
};

struct hw_props {
	uint64_t uw_min;
	uint64_t uw_max;
};

struct hw_dev {
	enum hw_mpn mpn;
	const char *id;
	enum mdl_model *models[8];
	struct hw_props verified_props;
	struct hw_props unverified_props;
};

bool hw_match(
	const char **dmatch,
	const char **match
);

struct hw_dev hw_dev_init(
	const char *id,
	const char **match
);

struct hw_props hw_props_best(
	struct hw_dev *dev
);


#endif

