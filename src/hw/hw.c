/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>
#include <src/hw/gen/cpu/cpu.h>
#include <src/hw/gen/motherboard/motherboard.h>
#include <src/hw/gen/dram/dram.h>
#include <src/hw/gen/storage/storage.h>
#include <src/hw/gen/nic/nic.h>
#include <src/hw/gen/fan/fan.h>
#include <src/hw/mpn/dell/0H74DC/0H74DC.h>
#include <src/hw/mpn/dell/0N0992/0N0992.h>
#include <src/hw/mpn/intel/BX8071512700/BX8071512700.h>
#include <src/hw/mpn/intel/WGI219LM/WGI219LM.h>
#include <src/hw/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.h>
#include <src/hw/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.h>
#include <src/hw/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.h>

// external
#include LIB_STRING
#include LIB_BOOL
#include LIB_DEF
#include LIB_INT
#include LIB_IO

bool hw_match(const char **dmatch, const char **match)
{
	int match_size;
	int dmatch_size;
	int size;
	int i;

	i = 0;
	while (match[i] != NULL)
		i++;
	match_size = i;

	i = 0;
	while (dmatch[i] != NULL)
		i++;
	dmatch_size = i;

	if (match_size <= dmatch_size)
		size = match_size;
	else
		size = dmatch_size;

	int matches = 0;
	for (i = 0; i < size; i++) {
		if (strncmp(match[i], dmatch[i], HW_DEV_MATCH_SIZE) == 0)
			matches++;
	}

	if (matches == i)
		return true;
	else
		return false;
}

int hw_dev_init(struct hw_dev *dev, const char *id, const char **match,
		enum hw_locale locale, uint8_t efficiency)
{
	int found = 0;

	found += hw_match(hw_gen_cpu, match) ?
			 !hw_gen_cpu_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_gen_dram, match) ?
			 !hw_gen_dram_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_gen_motherboard, match) ?
			 !hw_gen_motherboard_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_gen_storage, match) ?
			 !hw_gen_storage_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_gen_nic, match) ?
			 !hw_gen_nic_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_gen_fan, match) ?
			 !hw_gen_fan_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_mpn_dell_0h74dc, match) ?
			 !hw_mpn_dell_0h74dc_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_mpn_dell_0n0992, match) ?
			 !hw_mpn_dell_0n0992_init(dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_mpn_intel_bx8071512700, match) ?
			 !hw_mpn_intel_bx8071512700_init(dev, id, locale,
							 efficiency) :
			 0;
	found += hw_match(hw_mpn_intel_wgi219lm, match) ?
			 !hw_mpn_intel_wgi219lm_init(dev, id, locale,
						     efficiency) :
			 0;
	found += hw_match(hw_mpn_micron_mtc4c10163s1uc48ba1, match) ?
			 !hw_mpn_micron_mtc4c10163s1uc48ba1_init(
				 dev, id, locale, efficiency) :
			 0;
	found += hw_match(hw_mpn_sk_hynix_hfs001tej9x101n, match) ?
			 !hw_mpn_sk_hynix_hfs001tej9x101n_init(dev, id, locale,
							       efficiency) :
			 0;
	found += hw_match(hw_mpn_samsung_mzv9p2t0gw, match) ?
			 !hw_mpn_samsung_mzv9p2t0gw_init(dev, id, locale,
							 efficiency) :
			 0;

	if (found > 1 || !found)
		return -1;

	return 0;
}

struct hw_props hw_props_best(struct hw_dev *dev)
{
	struct hw_props props;
	struct hw_props ver = dev->verified_props;
	struct hw_props unver = dev->unverified_props;

	props.uw_min = ver.uw_min ? ver.uw_min : unver.uw_min;
	props.uw_max = ver.uw_max ? ver.uw_max : unver.uw_max;

	return props;
}
