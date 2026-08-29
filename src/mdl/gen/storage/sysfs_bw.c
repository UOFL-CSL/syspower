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
#include LIB_INT
#include LIB_STRING
#include LIB_TIME
#include <fcntl.h>
#include <unistd.h>

static struct ut_gbuf drives = {0};
struct drive {
	int fd;
	int64_t bw_max;
	int64_t prev;
};

static int configure(
	struct grp_group *grp
) {
	if (drives.size == 0) {
		ut_gbuf_ialloc(&drives);
		ut_gbuf_init(&drives);
	}
	
	char path[256];
	int fd;

	grp->counter = (int64_t)drives.len;
	for (uint32_t i = 0; i < grp->hw.len; i++) {
		// for this DIMM find a free running counter
		struct hw_dev *dev = grp->hw.ptr[i];
		// get the data configs
		CFG_STRNCPY(path, dev->id, sizeof(path));
		CFG_STRNCAT(path, "/stat", sizeof(path));
		fd = open(path, O_RDONLY);
		if (fd < 0) goto fail;
		
		struct drive *entry = CFG_CALLOC(1, sizeof(struct drive));
		entry->fd = fd;
		if (!entry) goto fail_close;
		ut_gbuf_push(&drives, entry);
	}

	return 0;

	fail_close:
		close(fd);
	fail:
		return -1;
}


int mdl_gen_storage_sysfs_bw(
	struct grp_group *grp
) {
	char buffer[256];
	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw = 0;
	
	if (!ptime)
		if (configure(grp)) goto fail;

	for (uint32_t i = (uint32_t)grp->counter;
			i < grp->hw.len + (uint32_t)grp->counter; i++) {
		struct drive *entry = drives.ptr[i];
		struct hw_dev *dev = grp->hw.ptr[i - grp->counter];
		struct hw_props props = hw_props_best(dev);
		
		int64_t total = 0;
		uint64_t bread, bwritten;
		lseek(entry->fd, 0, SEEK_SET);
		ssize_t bytes = read(entry->fd, buffer, sizeof(buffer) - 1);
		if (bytes < 0) goto fail;

		if (sscanf(buffer, "%*u %*u %lu %*u %*u %*u %lu", &bread, &bwritten) != 2) {
			goto fail;
		}

		total += (int64_t)bread;
		total += (int64_t)bwritten;
		
		
		if (!ptime)
			continue;
		int64_t bw = ((total - entry->prev) * 1000 * 1000 * 1000)
			/ (grp->jw.uj_ts_ns - ptime);
		entry->prev = total;
		if (bw > entry->bw_max)
			entry->bw_max = bw;
		grp->jw.uw += props.uw_min + (props.uw_max * bw) / entry->bw_max;

	}

	if (grp->jw.uw)
		grp->jw.uj += (grp->jw.uw * (grp->jw.uj_ts_ns - ptime))
			/ (1000 * 1000 * 1000);

	return 0;
	
	fail:
		return -1;
}

