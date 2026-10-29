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
#include LIB_STD
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

static struct ut_gbuf nics = { 0 };
struct nic {
	struct ut_gbuf links;
};
struct link {
	int fdr;
	int fdw;
	int64_t bw_max;
	int64_t prev;
};

static int configure(struct grp_group *grp)
{
	if (nics.size == 0) {
		if (ut_gbuf_ialloc(&nics))
			goto fail;
		if (ut_gbuf_init(&nics)) {
			ut_gbuf_ifree(&nics);
			goto fail;
		}
	}

	char path[256];
	int fd;
	DIR *dir;
	struct nic *nentry;
	struct link *lentry;
	char buffer[256];

	grp->counter = (int64_t)nics.len;
	for (uint32_t i = 0; i < grp->hw.len; i++) {
		struct hw_dev *dev = grp->hw.ptr[i];
		nentry = CFG_CALLOC(1, sizeof(struct nic));
		if (!nentry)
			goto fail;
		if (ut_gbuf_ialloc(&nentry->links))
			goto fail_nic;
		if (ut_gbuf_init(&nentry->links))
			goto fail_links;

		CFG_STRNCPY(path, dev->id, sizeof(path) - 1);
		CFG_STRNCAT(path, "/net/", sizeof(path) - 1);
		dir = opendir(path);
		if (!dir)
			goto fail_links;
		struct dirent *dentry;
		// find each link
		while ((dentry = readdir(dir)) != NULL) {
			if (!(dentry->d_name[0] == '.' &&
			      (dentry->d_name[1] == '\0' ||
			       (dentry->d_name[1] == '.' &&
				dentry->d_name[2] == '\0')))) {
				// get max bandwidth
				CFG_STRNCPY(path, dev->id, sizeof(path) - 1);
				CFG_STRNCAT(path, "/net/", sizeof(path) - 1);
				CFG_STRNCAT(path, dentry->d_name,
					    sizeof(path) - 1);
				CFG_STRNCAT(path, "/speed", sizeof(path) - 1);
				fd = open(path, O_RDONLY);
				if (fd < 0)
					goto fail_dir;
				int64_t bw;
				ssize_t bytes =
					read(fd, buffer, sizeof(buffer) - 1);
				close(fd);
				if (bytes < 0)
					goto fail_dir;
				if (sscanf(buffer, "%ld", &bw) != 1)
					goto fail_dir;
				bw *= (1000 * 1000) / 8;
				lentry = CFG_CALLOC(1, sizeof(struct link));
				if (!lentry)
					goto fail_dir;
				lentry->bw_max = bw; // bps

				// construct read/write paths
				CFG_STRNCPY(path, dev->id, sizeof(path) - 1);
				CFG_STRNCAT(path, "/net/", sizeof(path) - 1);
				CFG_STRNCAT(path, dentry->d_name,
					    sizeof(path) - 1);
				CFG_STRNCAT(path, "/statistics/tx_bytes",
					    sizeof(path) - 1);
				fd = open(path, O_RDONLY);
				if (fd < 0)
					goto fail_lentry;
				lentry->fdw = fd;
				CFG_STRNCPY(path, dev->id, sizeof(path) - 1);
				CFG_STRNCAT(path, "/net/", sizeof(path) - 1);
				CFG_STRNCAT(path, dentry->d_name,
					    sizeof(path) - 1);
				CFG_STRNCAT(path, "/statistics/rx_bytes",
					    sizeof(path) - 1);
				fd = open(path, O_RDONLY);
				if (fd < 0)
					goto fail_fdw;
				lentry->fdr = fd;
				ut_gbuf_push(&nentry->links, lentry);
			}
		}
		closedir(dir);
		ut_gbuf_push(&nics, nentry);
	}

	return 0;

fail_fdw:
	close(lentry->fdw);
fail_lentry:
	CFG_FREE(lentry);
fail_dir:
	closedir(dir);
fail_links:
	for (uint32_t i = 0; i < nentry->links.len; i++) {
		CFG_FREE(nentry->links.ptr[i]);
	}
	ut_gbuf_ifree(&nentry->links);
fail_nic:
	CFG_FREE(nentry);
fail:
	return -1;
}

int mdl_gen_nic_sysfs_bw(struct grp_group *grp)
{
	char buffer[256];
	int64_t ptime = grp->jw.uj_ts_ns;
	grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
	grp->jw.uw = 0;

	if (!ptime)
		if (configure(grp))
			goto fail;

	for (uint32_t i = (uint32_t)grp->counter;
	     i < grp->hw.len + (uint32_t)grp->counter; i++) {
		struct nic *nentry = nics.ptr[i];
		struct hw_dev *dev = grp->hw.ptr[i - grp->counter];
		struct hw_props props = hw_props_best(dev);
		for (uint32_t j = 0; j < nentry->links.len; j++) {
			struct link *lentry = nentry->links.ptr[j];
			int64_t total = 0;
			int64_t count = 0;
			lseek(lentry->fdr, 0, SEEK_SET);
			ssize_t bytes =
				read(lentry->fdr, buffer, sizeof(buffer) - 1);
			if (bytes < 0)
				goto fail;
			if (sscanf(buffer, "%ld", &count) != 1)
				goto fail;
			total += count;
			lseek(lentry->fdw, 0, SEEK_SET);
			bytes = read(lentry->fdw, buffer, sizeof(buffer) - 1);
			if (bytes < 0)
				goto fail;
			if (sscanf(buffer, "%ld", &count) != 1)
				goto fail;
			total += count;

			if (!ptime)
				continue;
			int64_t bw =
				((total - lentry->prev) * 1000 * 1000 * 1000) /
				(grp->jw.uj_ts_ns - ptime);
			lentry->prev = total;
			if (bw > lentry->bw_max)
				lentry->bw_max = bw;
			grp->jw.uw +=
				props.uw_min / (int64_t)nentry->links.len +
				(props.uw_max * bw) /
					(int64_t)nentry->links.len /
					lentry->bw_max;
		}
	}

	if (grp->jw.uw)
		grp->jw.uj += (grp->jw.uw * (grp->jw.uj_ts_ns - ptime)) /
			      (1000 * 1000 * 1000);

	return 0;

fail:
	return -1;
}
