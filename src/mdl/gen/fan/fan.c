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
#include <fcntl.h>
#include <unistd.h>

int mdl_gen_fan(
	struct grp_group *grp
) {
	int fd;

	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw = 0;
	for (uint32_t i = 0; i < grp->hw.len; i++) {
		struct hw_dev *dev = grp->hw.ptr[i];
		struct hw_props props = hw_props_best(dev);

		fd = open(dev->id, O_RDONLY);
		if (fd < 0) goto fail;
		char pwm_string[4];
		CFG_STRNCPY(pwm_string, "", sizeof(pwm_string));

		ssize_t bytes = read(fd, pwm_string, sizeof(pwm_string) - 1);
		if (bytes < 0) goto fail_file;

		int64_t pwm;
		CFG_STRTOLL(pwm_string, 10, &pwm);

		grp->jw.uw += props.uw_min + ((props.uw_max * pwm) / 255);
		if (ptime)
			grp->jw.uj += ((grp->jw.uj_ts_ns - ptime) * grp->jw.uw)
				/ (1000 * 1000 * 1000);

		close(fd);
	}

	return 0;

	fail_file:
		close(fd);
	fail:
		return -1;

}

