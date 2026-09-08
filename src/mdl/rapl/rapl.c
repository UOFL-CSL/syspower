/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <src/ut/error.h>

// external
#include LIB_IO
#include LIB_INT
#include LIB_BOOL
#include LIB_STRING
#include LIB_TIME
#include <fcntl.h>
#include <unistd.h>

int mdl_rapl_cpu(struct grp_group *grp)
{
	int fd;
	char base[64];
	char path[128];
	char buffer[64];

	// is psys there?
	bool offset = false;
	CFG_STRNCPY(base, "/sys/class/powercap/intel-rapl:0", sizeof(base));
	CFG_STRNCPY(path, base, sizeof(path));
	CFG_STRNCAT(path, "/name", sizeof(path));
	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto fail;
	ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
	if (bytes < 0)
		goto fail_file;
	close(fd);
	// then offset j
	if (strncmp(buffer, "package", 7) != 0)
		offset = true;

	int64_t puj = grp->jw.uj;
	grp->jw.uj = 0;
	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	for (uint64_t i = 0; i < grp->hw.len; i++) {
		struct hw_dev *dev = grp->hw.ptr[i];
		int64_t j;
		CFG_STRTOLL(dev->id, 10, &j);
		if (offset)
			j++;

		// is it a package?
		snprintf(base, sizeof(base),
			 "/sys/class/powercap/intel-rapl:%ld", j);
		CFG_STRNCPY(path, base, sizeof(path));
		CFG_STRNCAT(path, "/name", sizeof(path));
		fd = open(path, O_RDONLY);
		if (fd < 0)
			goto fail;
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes < 0)
			goto fail_file;
		close(fd);
		if (strncmp(buffer, "package", 7) != 0)
			continue;

		// then get the energy
		CFG_STRNCPY(path, base, sizeof(path));
		CFG_STRNCAT(path, "/energy_uj", sizeof(path));
		fd = open(path, O_RDONLY);
		if (fd < 0)
			goto fail;
		bytes = read(fd, buffer, sizeof(buffer) - 1);
		if (bytes < 0)
			goto fail_file;
		close(fd);
		int64_t uj;
		CFG_STRTOLL(buffer, 10, &uj);
		// initial measurement
		if (!ptime) {
			grp->counter += uj;
			continue;
		}

		grp->jw.uj += uj;
	}

	if (ptime)
		grp->jw.uj -= grp->counter;
	if (puj)
		grp->jw.uw = ((grp->jw.uj - puj) * (1000 * 1000 * 1000)) /
			     (grp->jw.uw_ts_ns - ptime);

	return 0;

fail_file:
	close(fd);
fail:
	return -1;
}
