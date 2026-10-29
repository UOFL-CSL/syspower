/*
 * Glue between top and syspower's measuring code (src/grp, src/hw, src/mdl).
 */
#include <src/dconf.h> // syspower's headers need this first
#include <src/grp/grp.h>
#include <src/mdl/mdl.h>
#include "sp_top.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// room for every syspower model number (enum mdl_model)
#define SP_MAX_MODELS 64

void sp_log(const char *fmt, ...)
{
	FILE *fp = fopen(SP_LOG_PATH, "a");
	va_list args;

	if (!fp)
		return;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	fclose(fp);
}

// which component each syspower model measures
static int sp_comp_of(int model)
{
	switch (model) {
	case SYSPWR_MDL_RAPL_CPU:
		return SP_CPU;
	case SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE:
		return SP_RAM;
	case SYSPWR_MDL_GEN_STORAGE_SYSFS_BW:
		return SP_DISK;
	case SYSPWR_MDL_GEN_NIC_SYSFS_BW:
		return SP_NIC;
	case SYSPWR_MDL_GEN_MOTHERBOARD_FLAT:
		return SP_BOARD;
	case SYSPWR_MDL_GEN_FAN:
		return SP_FAN;
	case SYSPWR_MDL_NVML_GPU:
		return SP_GPU;
	default:
		return -1; // a new model: add it above
	}
}

int sp_init(void)
{
	// the CPU power counter and memory counters are root-only
	if (geteuid() != 0) {
		sp_log("top-syspower: not root, power columns disabled\n");
		return -1;
	}
	return grp_strap();
}

int sp_sample(struct sp_power *p)
{
	int64_t uw[SP_MAX_MODELS] = { 0 };
	int n[SP_MAX_MODELS] = { 0 };
	int err = grp_step(uw, n, SP_MAX_MODELS);

	memset(p, 0, sizeof(*p));
	for (int m = 0; m < SP_MAX_MODELS; m++) {
		int c = sp_comp_of(m);
		if (c < 0 || !n[m])
			continue;
		p->watts[c] += (float)uw[m] / 1000000.0f;
		p->present[c] = 1;
		p->total += (float)uw[m] / 1000000.0f;
	}

	// run as: sudo env TOP_SYSPOWER_DEBUG=1 top  -> logs every measurement
	if (getenv("TOP_SYSPOWER_DEBUG"))
		sp_log("top-syspower: cpu=%.3f ram=%.3f disk=%.3f nic=%.3f "
		       "board=%.3f fan=%.3f gpu=%.3f total=%.3f W err=%d\n",
		       p->watts[SP_CPU], p->watts[SP_RAM], p->watts[SP_DISK],
		       p->watts[SP_NIC], p->watts[SP_BOARD], p->watts[SP_FAN],
		       p->watts[SP_GPU], p->total, err);
	return err;
}
