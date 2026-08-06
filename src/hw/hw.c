/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/hw/hw.h>
#include <src/hw/mpn/intel/BX8071512700/BX8071512700.h>

// external
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <stdio.h>

bool hw_match(
	const char **dmatch,
	const char **match
) {
	int match_size;
	int dmatch_size;
	int i;

	i = 0;
	while (match[i] != NULL)
		i++;
	match_size = i;

	i = 0;
	while (dmatch[i] != NULL)
		i++;
	dmatch_size = i;
	
	if (match_size != dmatch_size)
		return false;

	for (i = 0; i < match_size; i++) {
		if (strcmp(match[i], dmatch[i]) == 0)
			return false;
	}

	return true;
}

struct hw_dev hw_dev_init(
	const char *id,
	const char **match
) {
	struct hw_dev dev = {0};

	dev = hw_match(hw_mpn_intel_bx8071512700, match) ?
		hw_mpn_intel_bx8071512700_init(id) : dev;

	return dev;
}

struct hw_props hw_props_best(
	struct hw_dev *dev
) {
	struct hw_props props;
	struct hw_props ver = dev->verified_props;
	struct hw_props unver = dev->unverified_props;

	props.uw_min = ver.uw_min ? ver.uw_min : unver.uw_min;
	props.uw_max = ver.uw_max ? ver.uw_max : unver.uw_max;

	return props;
}

