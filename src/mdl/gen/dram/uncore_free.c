
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
#include LIB_STRING
#include LIB_TIME
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

static long perf_event_open(
	struct perf_event_attr *hw_event,
	pid_t pid,
	int cpu,
	int group_fd,
	unsigned long flags
) {
    return syscall(SYS_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static int init_conf(
	char *path,
	unsigned long *config
) {
	char buffer[64];
	int fd = open(path, O_RDONLY);
	if (fd < 0) goto fail;
	unsigned long event, umask;
	ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (bytes < 0) goto fail;

	int n = sscanf(buffer, "event=%lx,umask=%lx", &event, &umask);

	if (n == 2) {
		*config = event | (umask << 8);
		return 0;
	}

	fail:
		return -1;
}

static int64_t bytes_per_counter(
		char *path
) {
	char buffer[64];
	char npath[256];
	CFG_STRNCPY(npath, path, sizeof(npath));
	CFG_STRNCAT(npath, ".scale", sizeof(npath));
	int fd = open(npath, O_RDONLY);
	if (fd < 0) goto fail;
	ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (bytes < 0) goto fail;
	double scale;
	if (sscanf(buffer, "%lf", &scale) != 1) goto fail;
	
	CFG_STRNCPY(npath, path, sizeof(npath));
	CFG_STRNCAT(npath, ".unit", sizeof(npath));
	fd = open(npath, O_RDONLY);
	if (fd < 0) goto fail;
	bytes = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (bytes < 0) goto fail;

	if (strncmp(buffer, "MiB", 3) == 0) {
		scale *= 1024.0 * 1024.0;
	} else if (strncmp(buffer, "GB", 2) == 0) {
		scale *= 1024.0 * 1024.0 * 1024.0;
	} else {
		goto fail;
	}

	return (int64_t)scale;

	fail:
		return -1;
}

static struct ut_gbuf dimms = {0};
struct dimm {
	int fdr;
	int fdw;
	int64_t bpr;
	int64_t bpw;
	int64_t bw_max;
	int64_t prev;
};

static int configure(
	struct grp_group *grp
) {
	if (dimms.size == 0) {
		ut_gbuf_ialloc(&dimms);
		ut_gbuf_init(&dimms);
	}
	
	char base[128];
	char path[256];
	char buffer[64];
	int fdr;
	int fdw;

	grp->counter = (int64_t)dimms.len;
	for (uint32_t i = 0; i < grp->hw.len; i++) {
		// for this DIMM find a free running counter
		unsigned long rconf, wconf;
		snprintf(base, sizeof(base),
			"/sys/bus/event_source/devices/uncore_imc_free_running_%d", i);

		// get the data configs
		CFG_STRNCPY(path, base, sizeof(path));
		CFG_STRNCAT(path, "/events/data_read", sizeof(path));
		if (init_conf(path, &rconf)) goto fail;
		int64_t bpr = bytes_per_counter(path);
		CFG_STRNCPY(path, base, sizeof(path));
		CFG_STRNCAT(path, "/events/data_write", sizeof(path));
		if (init_conf(path, &wconf)) goto fail;
		int64_t bpw = bytes_per_counter(path);
		if (bpr < 0 || bpw < 0) goto fail;
		CFG_STRNCPY(path, base, sizeof(path));
		CFG_STRNCAT(path, "/type", sizeof(path));
		int64_t type;
		int fd = open(path, O_RDONLY);
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		CFG_STRTOLL(buffer, 10, &type);

		close(fd);
		if (bytes < 0) goto fail;


		// get the fd configs for the perf event
		struct perf_event_attr pe;
    		memset(&pe, 0, sizeof(pe));
		pe.type = (unsigned int)type;
		pe.size = sizeof(pe);
		pe.config = rconf;
		fdr = (int)perf_event_open(&pe, -1, 0, -1, 0);
		if (fdr < 0) goto fail;
    		memset(&pe, 0, sizeof(pe));
		pe.type = (unsigned int)type;
		pe.size = sizeof(pe);
		pe.config = wconf;
		fdw = (int)perf_event_open(&pe, -1, 0, -1, 0);
		if (fdw < 0) goto fail_read;

		// actually store the structs
		struct dimm *entry = CFG_CALLOC(1, sizeof(struct dimm));
		if (!entry) goto fail_write;
		entry->fdr = fdr;
		entry->bpr = bpr;
		entry->fdw = fdw;
		entry->bpw = bpw;
		ut_gbuf_push(&dimms, entry);
	}

	return 0;

	fail_write:
		close(fdw);
	fail_read:
		close(fdr);
	fail:
		return -1;
}


int mdl_gen_dram_intel_uncore_free(
	struct grp_group *grp
) {
	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw = 0;

	if (!ptime)
		if (configure(grp)) goto fail;

	for (uint32_t i = (uint32_t)grp->counter;
			i < grp->hw.len + (uint32_t)grp->counter; i++) {
		struct dimm *entry = dimms.ptr[i];
		struct hw_dev *dev = grp->hw.ptr[i - grp->counter];
		struct hw_props props = hw_props_best(dev);
		
		int64_t total = 0;
		uint64_t count = 0;

		lseek(entry->fdr, 0, SEEK_SET);
		ssize_t bytes = read(entry->fdr, &count, sizeof(count));
		total += (int64_t)count * entry->bpr;
		if (bytes < 0) goto fail;
		lseek(entry->fdw, 0, SEEK_SET);
		bytes = read(entry->fdw, &count, sizeof(count));
		if (bytes < 0) goto fail;
		total += (int64_t)count * entry->bpw;

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

