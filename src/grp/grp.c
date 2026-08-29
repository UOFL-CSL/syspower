/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <src/mdl/mdl.h>
#include <src/ut/gbuf.h>
#include <src/ut/error.h>
#include <src/hw/hw.h>

// external
#include <hwloc.h>
#include <libudev.h>
#include LIB_INT
#include LIB_STD
#include LIB_IO
#include LIB_TIME

static struct ut_gbuf grp_groups;

int grp_cpu_groups_push(void) {
	struct grp_group *cpu;
	char id[HW_DEV_ID_SIZE];
	int sockets;
	struct hw_dev *dev;
	hwloc_topology_t topology;
	hwloc_obj_t obj;
	
	if (hwloc_topology_init(&topology))
		goto fail;

	if (hwloc_topology_load(topology))
		goto fail_topology;
	
	sockets = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PACKAGE);
	for (int i = 0; i < sockets; i++) {
		cpu = grp_group_create();
		if (!cpu)
			goto fail_topology;
		dev = CFG_CALLOC(1, sizeof(struct hw_dev));
		if (!dev)
			goto fail_group;

		obj = hwloc_get_obj_by_type(
			topology,
			HWLOC_OBJ_PACKAGE,
			(unsigned int)i
		);
		const char *model = hwloc_obj_get_info_by_name(
			obj,
			"CPUModel"
		);
		const char *match[] = {
			model,
			NULL
		};
		
		snprintf(id, sizeof(id), "%d", i);
		
		if(hw_dev_init( // nothing matched
			dev,
			id,
			match,
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		)) {
				hw_dev_init( // so use a generic
					dev,
					id,
					(const char *[]){
						"generic_cpu",
						NULL
					},
					SYSPWR_HW_LOCALE_LOCAL,
					CFG_EFFICIENCY
				);
		}

		if (grp_group_push(cpu, dev))
			goto fail_dev;
		if (ut_gbuf_push(&grp_groups, cpu))
			goto fail_dev;
	}

	hwloc_topology_destroy(topology);
	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_group:
		grp_group_destroy(cpu);
	fail_topology:
		hwloc_topology_destroy(topology);
	fail:
		ut_error(-1, "failed to create cpu group!");
		return -1;
}

int grp_motherboard_groups_push(void) {
	hwloc_topology_t topology;
	hwloc_obj_t obj;
	struct grp_group *motherboard = grp_group_create();
	struct hw_dev *dev;
	char string[HW_DEV_MATCH_SIZE];

	if (!motherboard)
		goto fail;
	
	if (hwloc_topology_init(&topology))
		goto fail_group;

	if (hwloc_topology_load(topology))
		goto fail_topology;

	obj = hwloc_get_root_obj(topology);
	
	dev = CFG_CALLOC(1, sizeof(struct hw_dev));
	if (!dev)
		goto fail_topology;

	const char *vendor = hwloc_obj_get_info_by_name(obj, "DMIBoardVendor");
	const char *model = hwloc_obj_get_info_by_name(obj, "DMIBoardName");
	if (!vendor || !model)
		goto fail_topology;

	CFG_STRNCPY(string, vendor, sizeof(string));
	CFG_STRNCAT(string, " ", sizeof(string));
	CFG_STRNCAT(string, model, sizeof(string));

	const char *match[] = {
		string,
		NULL
	};

	if (hw_dev_init( // nothing found
		dev,
		"0",
		match,
		SYSPWR_HW_LOCALE_LOCAL,
		CFG_EFFICIENCY
	)) {
		hw_dev_init( // so use a generic
			dev,
			"0",
			(const char *[]){
				"generic_motherboard",
				NULL
			},
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		);
	}

	if (grp_group_push(motherboard, dev))
		goto fail_dev;
	if (ut_gbuf_push(&grp_groups, motherboard))
		goto fail_dev;

	hwloc_topology_destroy(topology);
	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_topology:
		hwloc_topology_destroy(topology);
	fail_group:
		grp_group_destroy(motherboard);
	fail:
		ut_error(-1, "failed to create motherboard group!");
		return -1;

}

int grp_dram_groups_push(void) {
	char string[HW_DEV_MATCH_SIZE];
	int i = 0;
	char key[64];
	struct hw_dev *dev;
	struct grp_group *dram = grp_group_create();
	if (!dram)
		goto fail;
	
	struct udev *udev = udev_new();
	if (!udev)
		goto fail_group;

	struct udev_device *udev_dev = udev_device_new_from_syspath(
		udev,
		"/sys/devices/virtual/dmi/id"
	);

	while(1) {
		dev = CFG_CALLOC(1, sizeof(struct hw_dev));
		if (!dev)
			goto fail_udev;

		snprintf(key, sizeof(key), "MEMORY_DEVICE_%d_PART_NUMBER", i);
		const char *model = udev_device_get_property_value(
			udev_dev,
			key
		);

		snprintf(key, sizeof(key), "MEMORY_DEVICE_%d_MANUFACTURER", i);
		const char *vendor = udev_device_get_property_value(
			udev_dev,
			key
		);
		i++;
		
		if (!model || !vendor) {
			CFG_FREE(dev);
			break;
		}
		
		if (strncmp(model, "Not Specified", HW_DEV_MATCH_SIZE) == 0) {
			CFG_FREE(dev);
			continue;
		}

		CFG_STRNCPY(string, "", sizeof(string));
		CFG_STRNCAT(string, vendor, sizeof(string));
		CFG_STRNCAT(string, " ", sizeof(string));
		CFG_STRNCAT(string, model, sizeof(string));
		const char *match[] = {
			string,
			NULL
		};

		if(hw_dev_init( // nothing found
			dev,
			"0",
			match,
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		)) {
			hw_dev_init( // so use a generic
				dev,
				"0",
				(const char *[]){
					"generic_dram",
					NULL
				},
				SYSPWR_HW_LOCALE_LOCAL,
				CFG_EFFICIENCY
			);
		}

		if (grp_group_push(dram, dev))
			goto fail_dev;
	}

	if (ut_gbuf_push(&grp_groups, dram))
		goto fail_dev;

	udev_device_unref(udev_dev);
	udev_unref(udev);

	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_udev:
		udev_device_unref(udev_dev);
		udev_unref(udev);
	fail_group:
		grp_group_destroy(dram);
	fail:
		ut_error(-1, "failed to create dram group!");
		return -1;
}

int grp_storage_groups_push(void) {
	struct grp_group *storage;
	char string[HW_DEV_MATCH_SIZE];
	struct hw_dev *dev;
	struct udev_device *udev_dev;
	struct udev *udev = udev_new();
	if (!udev)
		goto fail;

	struct udev_enumerate *udev_enum = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(udev_enum, "block");
	udev_enumerate_scan_devices(udev_enum);

	struct udev_list_entry *udev_devices = udev_enumerate_get_list_entry(
		udev_enum
	);
	struct udev_list_entry *udev_entry;

	udev_list_entry_foreach(udev_entry, udev_devices) {
		storage = grp_group_create();
		if (!storage)
			goto fail_udev;
		
		const char *udev_path = udev_list_entry_get_name(udev_entry);
		udev_dev = udev_device_new_from_syspath(udev, udev_path);
		if (!udev_dev) {
			grp_group_destroy(storage);
			continue;
		}

		dev = CFG_CALLOC(1, sizeof(struct hw_dev));
		if (!dev)
			goto fail_group;

		const char *udev_type = udev_device_get_property_value(udev_dev, "DEVTYPE");
		if (!udev_type || strncmp(udev_type, "disk", 4) != 0) {
			grp_group_destroy(storage);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			continue;
		}
		
		const char *udev_name = udev_device_get_property_value(udev_dev, "DEVNAME");
		if (
			!udev_name
			|| strncmp(udev_name, "/dev/loop", 9) == 0
			|| strncmp(udev_name, "/dev/ram", 8) == 0
		) {
			grp_group_destroy(storage);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			continue;
		}

		struct udev_device *usb = udev_device_get_parent_with_subsystem_devtype(
			udev_dev,
			"usb",
			NULL
		);
		if (usb) {
			grp_group_destroy(storage);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			continue;
		}
			
		CFG_STRNCPY(string, "", sizeof(string));
		const char *vendor = udev_device_get_sysattr_value(udev_dev, "device/vendor");
		if (vendor)
			CFG_STRNCAT(string, vendor, sizeof(string));
		const char *model = udev_device_get_sysattr_value(udev_dev, "device/model");
		if (vendor && model)
			CFG_STRNCAT(string, " ", sizeof(string));
		if (model)
			CFG_STRNCAT(string, model, sizeof(string));

		const char *match[] = {
			string,
			NULL
		};

		if (hw_dev_init( // nothing found
			dev,
			udev_path,
			match,
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		)) {
			hw_dev_init( // so use a generic
				dev,
				"0",
				(const char *[]){
					"generic_storage",
					NULL
				},
				SYSPWR_HW_LOCALE_LOCAL,
				CFG_EFFICIENCY
			);
		}

		if (dev->mpn == 0) {
			grp_group_destroy(storage);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			//hw_gen_dram_init();
			continue;
		}

		if (grp_group_push(storage, dev))
			goto fail_dev;
		if (ut_gbuf_push(&grp_groups, storage))
			goto fail_dev;

		udev_device_unref(udev_dev);
	}

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);

	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_group:
		grp_group_destroy(storage);
	fail_udev:
		udev_device_unref(udev_dev);
		udev_enumerate_unref(udev_enum);
		udev_unref(udev);
	fail:
		ut_error(-1, "failed to initialize storage group!");
		return -1;
}

int grp_gpu_groups_push(void) {
	struct grp_group *gpu;
	char id[HW_DEV_ID_SIZE];
	struct hw_dev *dev;
	struct udev_device *udev_dev;
	struct udev *udev = udev_new();
	if (!udev)
		goto fail;

	struct udev_enumerate *udev_enum = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(udev_enum, "pci");
	udev_enumerate_scan_devices(udev_enum);

	struct udev_list_entry *udev_devices = udev_enumerate_get_list_entry(
		udev_enum
	);
	struct udev_list_entry *udev_entry;

	udev_list_entry_foreach(udev_entry, udev_devices) {
		gpu = grp_group_create();
		if (!gpu)
			goto fail_udev;
		
		const char *udev_path = udev_list_entry_get_name(udev_entry);
		udev_dev = udev_device_new_from_syspath(udev, udev_path);
		if (!udev_dev) {
			grp_group_destroy(gpu);
			continue;
		}

		dev = CFG_CALLOC(1, sizeof(struct hw_dev));
		if (!dev)
			goto fail_group;

		const char *udev_class = udev_device_get_sysattr_value(udev_dev, "class");
		if (
			!udev_class
			|| !( // not a GPU
				strncmp(udev_class, "0x03", 4) == 0 // general
				|| strncmp(udev_class, "0x12", 4) == 0 // AI accelerator
			)
		) {
			grp_group_destroy(gpu);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			continue;
		}

		const char *board_vendor = udev_device_get_sysattr_value(
			udev_dev,
			"subsystem_vendor"
		);
		const char *silicon_vendor = udev_device_get_sysattr_value(
			udev_dev,
			"vendor"
		);
		const char *model = udev_device_get_property_value(
			udev_dev,
			"ID_MODEL_FROM_DATABASE"
		);
		snprintf(id, sizeof(id), "%d", 0);

		const char *match[] = {
			model,
			silicon_vendor,
			board_vendor,
			NULL
		};

		hw_dev_init(
			dev,
			id,
			match,
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		);

		if (dev->mpn == 0) {
			grp_group_destroy(gpu);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			//hw_gen_dram_init();
			continue;
		}

		if (grp_group_push(gpu, dev))
			goto fail_dev;
		if (ut_gbuf_push(&grp_groups, gpu))
			goto fail_dev;

		udev_device_unref(udev_dev);
	}

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);

	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_group:
		grp_group_destroy(gpu);
	fail_udev:
		udev_device_unref(udev_dev);
		udev_enumerate_unref(udev_enum);
		udev_unref(udev);
	fail:
		ut_error(-1, "failed to initialize gpu group!");
		return -1;
}

int grp_nic_groups_push(void) {
	struct grp_group *nic;
	struct hw_dev *dev;
	struct udev_device *udev_dev;
	struct udev *udev = udev_new();
	if (!udev)
		goto fail;

	struct udev_enumerate *udev_enum = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(udev_enum, "pci");
	udev_enumerate_scan_devices(udev_enum);

	struct udev_list_entry *udev_devices = udev_enumerate_get_list_entry(
		udev_enum
	);
	struct udev_list_entry *udev_entry;

	udev_list_entry_foreach(udev_entry, udev_devices) {
		nic = grp_group_create();
		if (!nic)
			goto fail_udev;
		
		const char *udev_path = udev_list_entry_get_name(udev_entry);
		udev_dev = udev_device_new_from_syspath(udev, udev_path);
		if (!udev_dev) {
			grp_group_destroy(nic);
			continue;
		}

		dev = CFG_CALLOC(1, sizeof(struct hw_dev));
		if (!dev)
			goto fail_group;

		const char *udev_class = udev_device_get_sysattr_value(udev_dev, "class");
		if (
			!udev_class
			|| !( // not a NIC
				strncmp(udev_class, "0x02", 4) == 0 // general
			)
		) {
			grp_group_destroy(nic);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			continue;
		}

		const char *board_vendor = udev_device_get_sysattr_value(
			udev_dev,
			"subsystem_vendor"
		);
		const char *silicon_vendor = udev_device_get_sysattr_value(
			udev_dev,
			"vendor"
		);
		const char *model = udev_device_get_property_value(
			udev_dev,
			"ID_MODEL_FROM_DATABASE"
		);

		const char *match[] = {
			model,
			silicon_vendor,
			board_vendor,
			NULL
		};

		hw_dev_init(
			dev,
			udev_path,
			match,
			SYSPWR_HW_LOCALE_LOCAL,
			CFG_EFFICIENCY
		);

		if (dev->mpn == 0) {
			grp_group_destroy(nic);
			CFG_FREE(dev);
			udev_device_unref(udev_dev);
			//hw_gen_dram_init();
			continue;
		}

		if (grp_group_push(nic, dev))
			goto fail_dev;
		if (ut_gbuf_push(&grp_groups, nic))
			goto fail_dev;

		udev_device_unref(udev_dev);
	}

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);

	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_group:
		grp_group_destroy(nic);
	fail_udev:
		udev_device_unref(udev_dev);
		udev_enumerate_unref(udev_enum);
		udev_unref(udev);
	fail:
		ut_error(-1, "failed to initialize nic group!");
		return -1;
}

int grp_fan_groups_push(void) {
	struct grp_group *fan;
	struct hw_dev *dev;
	char id[HW_DEV_ID_SIZE];
	struct udev *udev = udev_new();
	if (!udev)
		goto fail;
	struct udev_device *udev_dev;
	struct udev_list_entry *udev_devices;
	char *udev_path;

	char *hwmons = CFG_HWMONS_FANS;
	while ((udev_path = strsep(&hwmons, " ")) != NULL) {
		udev_dev = udev_device_new_from_syspath(udev, udev_path);
		struct udev_list_entry *udev_entry;
		udev_devices = udev_device_get_sysattr_list_entry(udev_dev);

		udev_list_entry_foreach(udev_entry, udev_devices) {
			const char *name = udev_list_entry_get_name(udev_entry);
			if (strchr(name, '/') != NULL) continue;
			size_t len = strlen(name);

			if (!( // not a fan
				len > 6
				&& strncmp(name, "pwm", 3) == 0
			))
				continue;
			

			const char *pwm_string = udev_device_get_sysattr_value(udev_dev, name);
			if (!pwm_string)
				continue;
			long long int pwm;
			if(CFG_STRTOLL(pwm_string, 10, &pwm))
				continue;
			
			const char *match[] = {
				"generic_fan",
				NULL
			};
			
			fan = grp_group_create();
			if (!fan)
				goto fail_udev;

			dev = CFG_CALLOC(1, sizeof(struct hw_dev));
			if (!dev)
				goto fail_group;

			CFG_STRNCPY(id, udev_path, sizeof(id));
			CFG_STRNCAT(id, "/", sizeof(id));
			CFG_STRNCAT(id, name, sizeof(id));

			hw_dev_init(
				dev,
				id,
				match,
				SYSPWR_HW_LOCALE_LOCAL,
				CFG_EFFICIENCY
			);

			if (grp_group_push(fan, dev))
				goto fail_dev;
			if (ut_gbuf_push(&grp_groups, fan))
				goto fail_dev;
		}
		udev_device_unref(udev_dev);
	}

	udev_unref(udev);
	return 0;

	fail_dev:
		CFG_FREE(dev);
	fail_group:
		grp_group_destroy(fan);
	fail_udev:
		udev_device_unref(udev_dev);
		udev_unref(udev);
	fail:
		ut_error(-1, "failed to initialize nic group!");
		return -1;
}

int grp_strap(void) {
	int err = 0;

	if (ut_gbuf_ialloc(&grp_groups))
		goto fail;
	err += ut_gbuf_init(&grp_groups);
	err += grp_cpu_groups_push();
	err += grp_motherboard_groups_push();
	err += grp_dram_groups_push();
	err += grp_storage_groups_push();
	err += grp_gpu_groups_push();
	err += grp_nic_groups_push();
	err += grp_fan_groups_push();
	
	if (err)
		goto fail_groups;

	return 0;

	fail_groups:
		grp_strip();
	fail:
		ut_error(-1, "failed to initialize groups!");
		return -1;
}

int grp_poll(void) {
	CFG_PRINTF_OUT("timestamp_ns,group,hardware,uw,uj\n");
	while (1) {
		long long int time = CFG_TIME_REAL_NS();
		for (uint32_t i = 0; i < grp_groups.len; i++) {
			CFG_PRINTF_OUT("%llu,%u,", time, i);
			struct grp_group *group = grp_groups.ptr[i];
			if (mdl_step(group))
				goto fail;
			for (uint32_t j = 0; j < group->hw.len; j++) {
				struct hw_dev *dev = group->hw.ptr[j];
				if (j > 0)
					CFG_PRINTF_OUT(";%s", dev->name);
				else
					CFG_PRINTF_OUT("%s", dev->name);
			}
			CFG_PRINTF_OUT(",%ld,%ld\n",
				group->jw.uw * 100 / CFG_EFFICIENCY,
				group->jw.uj * 100 / CFG_EFFICIENCY);
		}
		CFG_SLEEP_NS(1000 * 1000 * 1000);
	}
	
	return 0;

	fail:
		return -1;
}

int grp_strip(void) {
	for (uint32_t i = 0; i < grp_groups.len; i++) {
		struct grp_group *group = grp_groups.ptr[i];
		grp_group_destroy(group);
	}
	ut_gbuf_ifree(&grp_groups);
	return 0;
}

