/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/grp/grp.h>

// external
#include LIB_STRING
#include LIB_INT
#include LIB_TIME
#include LIB_STD

int mdl_gen_motherboard_flat(struct grp_group *grp)
{
	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();

	grp->jw.uw = 0;
	for (uint32_t i = 0; i < grp->hw.len; i++) {
		struct hw_dev *dev = grp->hw.ptr[i];
		struct hw_props props = hw_props_best(dev);

		grp->jw.uw += props.uw_max;
		if (ptime)
			grp->jw.uj +=
				((grp->jw.uj_ts_ns - ptime) * grp->jw.uw) /
				(1000 * 1000 * 1000);
	}

	return 0;
}
