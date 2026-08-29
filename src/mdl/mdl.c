/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/mdl/mdl.h>
#include <src/hw/hw.h>
#include <src/grp/grp.h>
#include <src/ut/error.h>
#include <src/ut/gbuf.h>
#include <src/mdl/gen/dram/dram.h>
#include <src/mdl/gen/fan/fan.h>
#include <src/mdl/gen/nic/nic.h>
#include <src/mdl/nvml/nvml.h>
#include <src/mdl/gen/storage/storage.h>
#include <src/mdl/gen/motherboard/motherboard.h>
#include <src/mdl/rapl/rapl.h>

// external
#include LIB_DEF
#include LIB_STD

int mdl_run(
	struct grp_group *grp
) {
	int ret = -1;

	switch (grp->model) {
	case SYSPWR_MDL_GEN_FAN:
		ret = mdl_gen_fan(grp);
		break;
	case SYSPWR_MDL_NVML_GPU:
		ret = mdl_nvml_gpu(grp);
		break;
	case SYSPWR_MDL_GEN_STORAGE_SYSFS_BW:
		ret = mdl_gen_storage_sysfs_bw(grp);
		break;
	case SYSPWR_MDL_GEN_NIC_SYSFS_BW:
		ret = mdl_gen_nic_sysfs_bw(grp);
		break;
	case SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE:
		ret = mdl_gen_dram_intel_uncore_free(grp);
		break;
	case SYSPWR_MDL_GEN_MOTHERBOARD_FLAT:
		ret = mdl_gen_motherboard_flat(grp);
		break;
	case SYSPWR_MDL_RAPL_CPU:
		ret = mdl_rapl_cpu(grp);
		break;
	}

	if (ret)
		return -1;

	return 0;
}

int mdl_try(
	struct grp_group *grp
) {
	struct occurrences {
		enum mdl_model model;
		size_t n;
	};
	struct ut_gbuf models;
	if (ut_gbuf_ialloc(&models))
		goto fail;
	if (ut_gbuf_init(&models))
		goto fail_models;

	// build models list
	for (size_t i = 0; i < grp->hw.len; i++) {
		struct hw_dev *dev = grp->hw.ptr[i];
		for (size_t j = 0; j < HW_DEV_MODEL_COUNT; j++) {
			if (!dev->models[j])
				continue;

			int duplicate = 0;
			for (size_t k = 0; k < models.len; k++) {
				struct occurrences *entry = models.ptr[k];
				if (((enum mdl_model)dev->models[j]) == entry->model) {
					duplicate = 1;
					entry->n++;
				}
			}
			if (!duplicate) {
				struct occurrences *entry =
					CFG_CALLOC(1, sizeof(struct occurrences));
				if (!entry)
					goto fail_models;
				entry->model = dev->models[j];
				entry->n++;
				ut_gbuf_push(&models, entry);
			}

		}
	}

	// only try to run models present on all hardware in the group
	bool success = false;
	for (size_t i = 0; i < models.len; i++) {
		struct occurrences *entry = models.ptr[i];
		if (entry->n < grp->hw.len)
			continue;
		grp->model = entry->model;

		// go with the first one that works
		// implies models array is priority ordered
		if(!mdl_run(grp)) {
			success = true;
			break;
		}
	}

	if (success)
		return 0;

	fail_models:
		for (size_t i = 0; i < models.len; i++) {
			CFG_FREE(models.ptr[i]);
		}
		ut_gbuf_ifree(&models);
	fail:
		return -1;

}

int mdl_step(
	struct grp_group *grp
) {
	if (!grp->model) {
		if (mdl_try(grp)) // no models worked
			goto fail;
	} else {
		if (mdl_run(grp)) // assigned model erred
			goto fail;
	}

	return 0;

	fail:
		ut_error(-1, "failed to run model(s) for a group!");
		return -1;
}

