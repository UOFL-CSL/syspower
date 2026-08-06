/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/ut/error.h>
#include <src/hw/hw.h>

// external
#include <hwloc.h>
#include <errno.h>

void grp_cpu_init(void) {
	int sockets;
	struct hw_dev dev;
	hwloc_topology_t topology;
	hwloc_obj_t obj;


	if (hwloc_topology_init(&topology))
		goto err;

	if (hwloc_topology_load(topology))
		goto clean;

	
	sockets = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PACKAGE);
	for (int i = 0; i < sockets; i++) {
		obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PACKAGE, (unsigned int)i);
		const char *model = hwloc_obj_get_info_by_name(obj, "CPUModel");
		const char *match[] = {
			model,
			"test",
			NULL
		};
		
		dev = hw_dev_init("0", match);
		if (dev.mpn == 0)
			continue;
			//dev = hw_gen_cpu_init();
	}

	hwloc_topology_destroy(topology);
	return;

	clean:
		hwloc_topology_destroy(topology);
	err:
		ut_error(errno, "failed to initialize CPU group!");
}

void grp_motherboard_init(void) {
	return;
}

void grp_dram_init(void) {
	return;
}

void grp_storage_init(void) {
	return;
}

void grp_gpu_init(void) {
	return;
}

void grp_fan_init(void) {
	return;
}

// grps need to be tracked and query their models every second for stats
void grp_init() {
	grp_cpu_init();
	grp_motherboard_init();
	grp_dram_init();
	grp_storage_init();
	grp_gpu_init();
	grp_fan_init();

	return;
}

