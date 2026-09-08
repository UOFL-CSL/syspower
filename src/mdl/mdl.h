/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MDL_MDL_H__
#define MDL_MDL_H__

struct grp_group;

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/grp/grp.h>

enum mdl_model {
	SYSPWR_MDL_PLACEHOLDER,
	SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE,
	SYSPWR_MDL_GEN_FAN,
	SYSPWR_MDL_NVML_GPU,
	SYSPWR_MDL_GEN_NIC_SYSFS_BW,
	SYSPWR_MDL_GEN_STORAGE_SYSFS_BW,
	SYSPWR_MDL_GEN_MOTHERBOARD_FLAT,
	SYSPWR_MDL_RAPL_CPU,
};

int mdl_step(struct grp_group *grp);

#endif
