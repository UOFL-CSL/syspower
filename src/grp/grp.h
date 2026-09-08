/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GRP_GRP_H__
#define GRP_GRP_H__

struct hw_dev;
struct mdl_jw;

// internal
#include <src/ut/gbuf.h>
#include <src/mdl/mdl.h>
#include <src/hw/hw.h>

// external
#include LIB_STD
#include LIB_INT

struct grp_jw {
	int64_t uj_ts_ns;
	int64_t uj;
	int64_t uw_ts_ns;
	int64_t uw;
};

struct grp_group {
	struct ut_gbuf hw;
	struct grp_jw jw;
	int model; //enum mdl_model
	int64_t counter;
};

static inline struct grp_group *grp_group_create(void)
{
	struct grp_group *group = CFG_CALLOC(1, sizeof(struct grp_group));
	if (!group)
		goto fail;
	if (ut_gbuf_ialloc(&group->hw))
		goto fail_group;

	if (ut_gbuf_init(&group->hw))
		goto fail_group;

	return group;

fail_group:
	CFG_FREE(group);
fail:
	return NULL;
}

static inline void grp_group_destroy(struct grp_group *group)
{
	for (uint32_t j = 0; j < group->hw.len; j++) {
		struct hw_dev *dev = group->hw.ptr[j];
		CFG_FREE(dev);
	}
	ut_gbuf_ifree(&group->hw);
	CFG_FREE(group);
	return;
}

static inline int grp_group_push(struct grp_group *group, struct hw_dev *dev)
{
	if (ut_gbuf_push(&group->hw, dev))
		return -1;

	return 0;
}

int grp_strap(void);
int grp_strip(void);
int grp_cpu_groups_push(void);
int grp_motherboard_groups_push(void);
int grp_dram_groups_push(void);
int grp_storage_groups_push(void);
int grp_gpu_groups_push(void);
int grp_nic_groups_push(void);
int grp_fan_groups_push(void);

int grp_poll();

#endif
