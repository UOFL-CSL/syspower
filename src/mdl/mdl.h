/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MDL_MDL_H__
#define MDL_MDL_H__

// internal
#include <src/grp/grp.h>

// external
#include <stdint.h>

enum mdl_model {
	SYSPWR_MDL_RAPL,
	SYSPWR_MDL_GEN_INTERPOLATE
};

struct mdl_jw {
	uint64_t uj_ts_ns;
	uint64_t uj;
	uint64_t uw_ts_ns;
	uint64_t uw;
};

void mdl_step(
	struct grp_group *grp
);

#endif

