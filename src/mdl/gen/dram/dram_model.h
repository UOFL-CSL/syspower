/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MDL_GEN_DRAM_DRAM_MODEL_H__
#define MDL_GEN_DRAM_DRAM_MODEL_H__

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <stdint.h>

// external
#include LIB_BOOL
#include LIB_INT

/*
 * RAPL Memory Power formulation (David et al. ISLPED 2010), Equation (1):
 * Power_DRAM [W] = (Act * W_act) + (Read * W_read) + (Write * W_write) +
 *                  (CKE_high * W_CKEH) + K
 */
typedef struct {
	double w_act; /* Joules per row activate / precharge */
	double w_read; /* Joules per CAS read command */
	double w_write; /* Joules per CAS write command */
	double w_ckeh; /* Watts active standby power adder */
	double k_baseline; /* Watts quiescent leakage power in self-refresh/C6 */
} dram_weights_t;

typedef struct {
	uint64_t act_count;
	uint64_t read_count;
	uint64_t write_count;
	double act_rate; /* Commands per second */
	double read_rate; /* Commands per second */
	double write_rate; /* Commands per second */
	double cke_high; /* Fraction of sampling window (0.0 to 1.0) */
	double interval_sec; /* Duration of current measurement interval */
	double power_watts; /* Estimated DRAM power in Watts */
	double rapl_power_watts; /* Ground-truth RAPL DRAM power in Watts */
	bool rapl_available; /* True if ground-truth RAPL DRAM is present */
	double error_percent; /* Relative error percentage vs RAPL */
} dram_estimate_t;

/* Lifecycle and configuration API */
int dram_model_init(int num_dimms, int num_sockets);
void dram_model_cleanup(void);
dram_weights_t dram_model_get_weights(void);
void dram_model_set_weights(dram_weights_t weights);

/* Core estimation functions */
double estimate_dram_power(double interval_sec);
int dram_model_step(struct grp_group *grp, dram_estimate_t *out_est);

/* Automated calibration engine using Ordinary Least Squares (OLS) */
int dram_model_calibrate(int num_samples, int sample_interval_ms);

/* SysPower group model integration callbacks */
int mdl_gen_dram_activity(struct grp_group *grp);
int mdl_gen_dram_david10(struct grp_group *grp);

#endif
