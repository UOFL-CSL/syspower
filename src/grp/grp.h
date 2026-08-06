/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GRP_H__
#define GRP_H__

struct grp_group {
};

void grp_init(void);
void grp_cpu_init(void);
void grp_platform_init(void);
void grp_dram_init(void);
void grp_storage_init(void);
void grp_gpu_init(void);
void grp_fan_init(void);

#endif

