/*
 * The only part of syspower that top.c sees.
 * Kept free of syspower's own headers so their macros can't clash with top's.
 */
#ifndef SP_TOP_H__
#define SP_TOP_H__

// hardware components, in the order top shows them
enum sp_comp {
	SP_CPU,
	SP_RAM,
	SP_DISK,
	SP_NIC,
	SP_BOARD,
	SP_FAN,
	SP_GPU,
	SP_NCOMP // number of components
};

struct sp_power {
	float watts[SP_NCOMP]; // power per component, averaged since last call
	int present[SP_NCOMP]; // 1 if this machine has it and it was measured
	float total;           // sum of all components
};

// find the hardware once; 0 = ok, -1 = unavailable (e.g. not run as root)
int sp_init(void);

// measure once. 0 = ok, -1 = a measurement failed (see SP_LOG_PATH)
int sp_sample(struct sp_power *p);

#define SP_LOG_PATH "/var/tmp/top-syspower.log"

#endif
