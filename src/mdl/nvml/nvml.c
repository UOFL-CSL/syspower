/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>

// external
#include LIB_IO
#include LIB_BOOL
#include LIB_INT
#include LIB_TIME
#include <nvml.h>

static bool strapped = false;

int mdl_nvml_gpu(struct grp_group *grp)
{
	nvmlDevice_t nvml_dev;
	unsigned int mw;

	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw = 0;

	if (!strapped) {
		if (nvmlInit() != NVML_SUCCESS)
			goto fail;
		strapped = true;
	}

	for (uint32_t i = 0; i < grp->hw.len; i++) {
		if (nvmlDeviceGetHandleByIndex(i, &nvml_dev) != NVML_SUCCESS)
			goto fail;
		if (nvmlDeviceGetPowerUsage(nvml_dev, &mw) != NVML_SUCCESS)
			goto fail;

		grp->jw.uw += mw * 1000;
	}

	if (ptime)
		grp->jw.uj += (grp->jw.uw * (grp->jw.uj_ts_ns - ptime)) /
			      (1000 * 1000 * 1000);

	return 0;

fail:
	return -1;
}
