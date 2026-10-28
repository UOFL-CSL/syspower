/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

/*
 * model based off:
 * RAPL: Memory Power Estimation and Capping
 * Howard David, Eugene Gorbatov, Ulf R. Hanebutte, Rahul Khanna, and Christan Le
 * https://dl.acm.org/doi/10.1145/1840845.1840883
 */

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <src/hw/hw.h>
#include <src/mdl/gen/dram/dram_model.h>
#include <src/ut/error.h>
#include <stdint.h>

// external
#include LIB_BOOL
#include LIB_DEF
#include LIB_INT
#include LIB_IO
#include LIB_STRING
#include LIB_TIME
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <math.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MAX_IMC_CHANNELS 32
#define MAX_CPUS 256
#define MAX_RAPL_DOMAINS 8
#define PATH_BUF_SIZE 1024

struct imc_channel {
	char name[256];
	int fdr;
	int fdw;
	int fda;
	uint64_t prev_r;
	uint64_t prev_w;
	uint64_t prev_a;
	int64_t scale_r;
	int64_t scale_w;
};

struct cpu_c6_state {
	char time_path[PATH_BUF_SIZE];
	uint64_t prev_time_us;
};

struct rapl_dram_domain {
	char energy_path[PATH_BUF_SIZE];
	uint64_t prev_energy_uj;
	uint64_t max_energy_uj;
};

static struct imc_channel g_channels[MAX_IMC_CHANNELS];
static int g_num_channels = 0;

static int g_fd_llc_miss = -1;
static uint64_t g_prev_llc_miss = 0;

static struct cpu_c6_state g_cpus[MAX_CPUS];
static int g_num_cpus = 0;

static struct rapl_dram_domain g_rapl[MAX_RAPL_DOMAINS];
static int g_num_rapl = 0;
static bool g_rapl_detected = false;

/* weights from David RAPL paper */
static dram_weights_t g_weights = {
	.w_act = 36.3e-9,
	.w_read = 22.8e-9,
	.w_write = 21.7e-9,
	.w_ckeh = 2.112,
	.k_baseline = 1.663,
};

static bool g_initialized = false;
static int64_t g_prev_step_ts_ns = 0;
static int g_dimm_count = 1;
static int g_socket_count = 1;

static long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
				int cpu, int group_fd, unsigned long flags)
{
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd,
		       flags);
}

static int parse_event_conf(const char *path, unsigned long *config)
{
	char buffer[64];
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (bytes < 0)
		return -1;
	buffer[bytes] = '\0';

	unsigned long event = 0, umask = 0;
	int found_event = 0;
	char *p;

	p = strstr(buffer, "event=");
	if (p && sscanf(p, "event=%lx", &event) == 1)
		found_event = 1;

	p = strstr(buffer, "umask=");
	if (p)
		sscanf(p, "umask=%lx", &umask);

	if (found_event) {
		*config = event | (umask << 8);
		return 0;
	}

	if (sscanf(buffer, "%lx", &event) == 1) {
		*config = event;
		return 0;
	}

	return -1;
}

static int64_t read_counter_scale(const char *path)
{
	char buffer[64];
	char npath[PATH_BUF_SIZE];
	double scale = 1.0;

	snprintf(npath, sizeof(npath), "%s.scale", path);
	int fd = open(npath, O_RDONLY);
	if (fd >= 0) {
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		close(fd);
		if (bytes > 0) {
			buffer[bytes] = '\0';
			sscanf(buffer, "%lf", &scale);
		}
	}

	snprintf(npath, sizeof(npath), "%s.unit", path);
	fd = open(npath, O_RDONLY);
	if (fd >= 0) {
		ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
		close(fd);
		if (bytes > 0) {
			buffer[bytes] = '\0';
			if (strncmp(buffer, "MiB", 3) == 0)
				scale *= 1024.0 * 1024.0;
			else if (strncmp(buffer, "GB", 2) == 0)
				scale *= 1024.0 * 1024.0 * 1024.0;
		}
	}

	return (int64_t)scale;
}

static int open_pmu_event(int type, unsigned long config, int target_cpu)
{
	struct perf_event_attr pe;
	memset(&pe, 0, sizeof(pe));
	pe.type = (unsigned int)type;
	pe.size = sizeof(pe);
	pe.config = config;
	pe.disabled = 0;

	long fd = sys_perf_event_open(&pe, -1, target_cpu, -1, 0);
	return (int)fd;
}

static int discover_uncore_channels(void)
{
	DIR *dir = opendir("/sys/bus/event_source/devices");
	if (!dir)
		return -1;

	struct dirent *entry;
	g_num_channels = 0;

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "uncore_imc", 10) != 0)
			continue;
		if (g_num_channels >= MAX_IMC_CHANNELS)
			break;

		char base[512];
		char path[PATH_BUF_SIZE];
		char buffer[64];

		snprintf(base, sizeof(base), "/sys/bus/event_source/devices/%s",
			 entry->d_name);

		snprintf(path, sizeof(path), "%s/type", base);
		int tfd = open(path, O_RDONLY);
		if (tfd < 0)
			continue;
		ssize_t bytes = read(tfd, buffer, sizeof(buffer) - 1);
		close(tfd);
		if (bytes <= 0)
			continue;
		buffer[bytes] = '\0';
		int type = atoi(buffer);
		if (type <= 0)
			continue;

		int target_cpu = 0;
		snprintf(path, sizeof(path), "%s/cpumask", base);
		int cfd = open(path, O_RDONLY);
		if (cfd >= 0) {
			bytes = read(cfd, buffer, sizeof(buffer) - 1);
			close(cfd);
			if (bytes > 0) {
				buffer[bytes] = '\0';
				sscanf(buffer, "%d", &target_cpu);
			}
		}

		/* Find Read event */
		const char *read_names[] = { "cas_count_read", "data_reads",
					     "data_read" };
		unsigned long r_conf = 0;
		int found_r = 0;
		int64_t r_scale = 64;
		for (size_t i = 0;
		     i < sizeof(read_names) / sizeof(read_names[0]); i++) {
			snprintf(path, sizeof(path), "%s/events/%s", base,
				 read_names[i]);
			if (access(path, R_OK) == 0 &&
			    parse_event_conf(path, &r_conf) == 0) {
				found_r = 1;
				r_scale = read_counter_scale(path);
				break;
			}
		}

		/* Find Write event */
		const char *write_names[] = { "cas_count_write", "data_writes",
					      "data_write" };
		unsigned long w_conf = 0;
		int found_w = 0;
		int64_t w_scale = 64;
		for (size_t i = 0;
		     i < sizeof(write_names) / sizeof(write_names[0]); i++) {
			snprintf(path, sizeof(path), "%s/events/%s", base,
				 write_names[i]);
			if (access(path, R_OK) == 0 &&
			    parse_event_conf(path, &w_conf) == 0) {
				found_w = 1;
				w_scale = read_counter_scale(path);
				break;
			}
		}

		/* Find Activate event */
		const char *act_names[] = { "act_count", "act_counts", "act" };
		unsigned long a_conf = 0;
		int found_a = 0;
		for (size_t i = 0; i < sizeof(act_names) / sizeof(act_names[0]);
		     i++) {
			snprintf(path, sizeof(path), "%s/events/%s", base,
				 act_names[i]);
			if (access(path, R_OK) == 0 &&
			    parse_event_conf(path, &a_conf) == 0) {
				found_a = 1;
				break;
			}
		}
		if (!found_a)
			a_conf =
				0x0101; /* NOTE: Standard Intel Xeon UNC_M_ACT_COUNT */

		if (!found_r || !found_w)
			continue;

		int fdr = open_pmu_event(type, r_conf, target_cpu);
		int fdw = open_pmu_event(type, w_conf, target_cpu);
		int fda = open_pmu_event(type, a_conf, target_cpu);

		if (fdr < 0 || fdw < 0) {
			if (fdr >= 0)
				close(fdr);
			if (fdw >= 0)
				close(fdw);
			if (fda >= 0)
				close(fda);
			continue;
		}

		struct imc_channel *ch = &g_channels[g_num_channels++];
		snprintf(ch->name, sizeof(ch->name), "%s", entry->d_name);
		ch->fdr = fdr;
		ch->fdw = fdw;
		ch->fda = fda;
		ch->scale_r = r_scale > 0 ? r_scale : 64;
		ch->scale_w = w_scale > 0 ? w_scale : 64;
		ch->prev_r = 0;
		ch->prev_w = 0;
		ch->prev_a = 0;
	}

	closedir(dir);

	/* Check if activate counter is missing on all channels */
	bool has_act = false;
	for (int i = 0; i < g_num_channels; i++) {
		if (g_channels[i].fda >= 0) {
			has_act = true;
			break;
		}
	}

	/* Fallback: Core LLC read misses proxy */
	if (!has_act && g_fd_llc_miss < 0) {
		struct perf_event_attr pe;
		memset(&pe, 0, sizeof(pe));
		pe.type = PERF_TYPE_HW_CACHE;
		pe.size = sizeof(pe);
		pe.config =
			PERF_COUNT_HW_CACHE_LL |
			((unsigned long)PERF_COUNT_HW_CACHE_OP_READ << 8) |
			((unsigned long)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
		pe.disabled = 0;

		long fd = sys_perf_event_open(&pe, -1, 0, -1, 0);
		if (fd >= 0)
			g_fd_llc_miss = (int)fd;
	}

	return g_num_channels > 0 ? 0 : -1;
}

static void discover_cpuidle_c6(void)
{
	g_num_cpus = 0;
	for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
		char path[PATH_BUF_SIZE];
		char name_buf[64];
		snprintf(path, sizeof(path),
			 "/sys/devices/system/cpu/cpu%d/cpuidle", cpu);
		if (access(path, R_OK) != 0)
			break;

		int c6_state = -1;
		int deepest_state = -1;

		for (int s = 0; s < 10; s++) {
			snprintf(
				path, sizeof(path),
				"/sys/devices/system/cpu/cpu%d/cpuidle/state%d/name",
				cpu, s);
			int fd = open(path, O_RDONLY);
			if (fd < 0)
				break;
			ssize_t bytes =
				read(fd, name_buf, sizeof(name_buf) - 1);
			close(fd);
			if (bytes <= 0)
				break;
			name_buf[bytes] = '\0';

			deepest_state = s;
			if (strstr(name_buf, "C6") != NULL ||
			    strstr(name_buf, "c6") != NULL) {
				c6_state = s;
				break;
			}
		}

		int target_state = (c6_state >= 0) ? c6_state : deepest_state;
		if (target_state >= 0) {
			struct cpu_c6_state *st = &g_cpus[g_num_cpus++];
			snprintf(
				st->time_path, sizeof(st->time_path),
				"/sys/devices/system/cpu/cpu%d/cpuidle/state%d/time",
				cpu, target_state);
			st->prev_time_us = 0;
		}
	}
}

static void discover_rapl_dram(void)
{
	g_num_rapl = 0;
	g_rapl_detected = false;

	for (int pkg = 0; pkg < 8; pkg++) {
		for (int sub = 0; sub < 8; sub++) {
			char path[PATH_BUF_SIZE];
			char buf[64];

			snprintf(
				path, sizeof(path),
				"/sys/class/powercap/intel-rapl:%d/intel-rapl:%d:%d/name",
				pkg, pkg, sub);
			int fd = open(path, O_RDONLY);
			if (fd < 0) {
				snprintf(
					path, sizeof(path),
					"/sys/class/powercap/intel-rapl/intel-rapl:%d/intel-rapl:%d:%d/name",
					pkg, pkg, sub);
				fd = open(path, O_RDONLY);
			}
			if (fd < 0)
				continue;

			ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
			close(fd);
			if (bytes <= 0)
				continue;
			buf[bytes] = '\0';

			if (strncmp(buf, "dram", 4) == 0 &&
			    g_num_rapl < MAX_RAPL_DOMAINS) {
				struct rapl_dram_domain *dom =
					&g_rapl[g_num_rapl++];
				snprintf(
					dom->energy_path,
					sizeof(dom->energy_path),
					"/sys/class/powercap/intel-rapl:%d/intel-rapl:%d:%d/energy_uj",
					pkg, pkg, sub);
				if (access(dom->energy_path, R_OK) != 0) {
					snprintf(
						dom->energy_path,
						sizeof(dom->energy_path),
						"/sys/class/powercap/intel-rapl/intel-rapl:%d/intel-rapl:%d:%d/energy_uj",
						pkg, pkg, sub);
				}

				dom->max_energy_uj = 0;
				char max_path[PATH_BUF_SIZE];
				snprintf(
					max_path, sizeof(max_path),
					"/sys/class/powercap/intel-rapl:%d/intel-rapl:%d:%d/max_energy_range_uj",
					pkg, pkg, sub);
				fd = open(max_path, O_RDONLY);
				if (fd >= 0) {
					bytes = read(fd, buf, sizeof(buf) - 1);
					close(fd);
					if (bytes > 0) {
						buf[bytes] = '\0';
						dom->max_energy_uj =
							strtoull(buf, NULL, 10);
					}
				}
				dom->prev_energy_uj = 0;
				g_rapl_detected = true;
			}
		}
	}
}

int dram_model_init(int num_dimms, int num_sockets)
{
	if (g_initialized)
		return 0;

	g_dimm_count = (num_dimms > 0) ? num_dimms : 1;
	g_socket_count = (num_sockets > 0) ? num_sockets : 1;

	/* Scale standby and idle baseline weights by DIMM count */
	g_weights.w_ckeh = 2.112 * (double)g_dimm_count;
	g_weights.k_baseline = 1.663 * (double)g_dimm_count;

	if (discover_uncore_channels() != 0)
		return -1;

	discover_cpuidle_c6();
	discover_rapl_dram();

	g_initialized = true;
	g_prev_step_ts_ns = 0;

	bool do_cal = (CFG_DRAM_CALIBRATE != 0);
	const char *cal_env = getenv("SYSPWR_DRAM_CALIBRATE");
	if (cal_env) {
		if (strcmp(cal_env, "0") == 0 || strcmp(cal_env, "false") == 0)
			do_cal = false;
		else if (strcmp(cal_env, "1") == 0 ||
			 strcmp(cal_env, "true") == 0)
			do_cal = true;
	}

	if (do_cal) {
		if (g_rapl_detected) {
			dram_model_calibrate(30, 100);
		} else {
			CFG_PRINTF_ERR(
				"[dram_model] RAPL DRAM not detected; skipping calibration and using default weights.\n");
		}
	}

	return 0;
}

void dram_model_cleanup(void)
{
	for (int i = 0; i < g_num_channels; i++) {
		if (g_channels[i].fdr >= 0)
			close(g_channels[i].fdr);
		if (g_channels[i].fdw >= 0)
			close(g_channels[i].fdw);
		if (g_channels[i].fda >= 0)
			close(g_channels[i].fda);
	}
	g_num_channels = 0;

	if (g_fd_llc_miss >= 0) {
		close(g_fd_llc_miss);
		g_fd_llc_miss = -1;
	}

	g_initialized = false;
}

dram_weights_t dram_model_get_weights(void)
{
	return g_weights;
}

void dram_model_set_weights(dram_weights_t weights)
{
	g_weights = weights;
}

static void sample_hardware_metrics(uint64_t *out_act, uint64_t *out_read,
				    uint64_t *out_write, double *out_cke_high,
				    double *out_rapl_watts,
				    bool *out_rapl_avail, double interval_sec,
				    bool first_sample)
{
	uint64_t total_act = 0;
	uint64_t total_read = 0;
	uint64_t total_write = 0;

	for (int i = 0; i < g_num_channels; i++) {
		struct imc_channel *ch = &g_channels[i];
		uint64_t val = 0;

		lseek(ch->fdr, 0, SEEK_SET);
		if (read(ch->fdr, &val, sizeof(val)) == sizeof(val)) {
			if (!first_sample && val >= ch->prev_r) {
				uint64_t d = val - ch->prev_r;
				total_read +=
					(d * (uint64_t)ch->scale_r) / 64ULL;
			}
			ch->prev_r = val;
		}

		lseek(ch->fdw, 0, SEEK_SET);
		if (read(ch->fdw, &val, sizeof(val)) == sizeof(val)) {
			if (!first_sample && val >= ch->prev_w) {
				uint64_t d = val - ch->prev_w;
				total_write +=
					(d * (uint64_t)ch->scale_w) / 64ULL;
			}
			ch->prev_w = val;
		}

		if (ch->fda >= 0) {
			lseek(ch->fda, 0, SEEK_SET);
			if (read(ch->fda, &val, sizeof(val)) == sizeof(val)) {
				if (!first_sample && val >= ch->prev_a) {
					total_act += (val - ch->prev_a);
				}
				ch->prev_a = val;
			}
		}
	}

	/* Fallback proxy for ACT if not supported on uncore IMCs */
	if (g_fd_llc_miss >= 0) {
		uint64_t val = 0;
		lseek(g_fd_llc_miss, 0, SEEK_SET);
		if (read(g_fd_llc_miss, &val, sizeof(val)) == sizeof(val)) {
			if (!first_sample && val >= g_prev_llc_miss) {
				total_act += (val - g_prev_llc_miss);
			}
			g_prev_llc_miss = val;
		}
	}

	/* CKE / C6 residency */
	uint64_t total_c6_delta_us = 0;
	for (int i = 0; i < g_num_cpus; i++) {
		int fd = open(g_cpus[i].time_path, O_RDONLY);
		if (fd >= 0) {
			char buf[32];
			ssize_t n = read(fd, buf, sizeof(buf) - 1);
			close(fd);
			if (n > 0) {
				buf[n] = '\0';
				uint64_t cur_us = strtoull(buf, NULL, 10);
				if (!first_sample &&
				    cur_us >= g_cpus[i].prev_time_us) {
					total_c6_delta_us +=
						(cur_us -
						 g_cpus[i].prev_time_us);
				}
				g_cpus[i].prev_time_us = cur_us;
			}
		}
	}

	double cke_high = 1.0;
	if (interval_sec > 0.0 && g_num_cpus > 0) {
		double c6_sec = (double)total_c6_delta_us / 1000000.0;
		double c6_frac = c6_sec / (interval_sec * (double)g_num_cpus);
		cke_high = 1.0 - c6_frac;
		if (cke_high < 0.0)
			cke_high = 0.0;
		if (cke_high > 1.0)
			cke_high = 1.0;
	}

	/* RAPL DRAM Ground Truth */
	double rapl_watts = 0.0;
	bool rapl_avail = false;
	if (g_rapl_detected && interval_sec > 0.0) {
		uint64_t total_energy_uj = 0;
		for (int i = 0; i < g_num_rapl; i++) {
			int fd = open(g_rapl[i].energy_path, O_RDONLY);
			if (fd >= 0) {
				char buf[32];
				ssize_t n = read(fd, buf, sizeof(buf) - 1);
				close(fd);
				if (n > 0) {
					buf[n] = '\0';
					uint64_t cur_uj =
						strtoull(buf, NULL, 10);
					if (!first_sample) {
						uint64_t d_uj = 0;
						if (cur_uj >=
						    g_rapl[i].prev_energy_uj) {
							d_uj = cur_uj -
							       g_rapl[i]
								       .prev_energy_uj;
						} else {
							uint64_t wrap =
								g_rapl[i].max_energy_uj >
										0 ?
									g_rapl[i]
										.max_energy_uj :
									0x100000000ULL;
							d_uj = (wrap -
								g_rapl[i]
									.prev_energy_uj) +
							       cur_uj;
						}
						total_energy_uj += d_uj;
					}
					g_rapl[i].prev_energy_uj = cur_uj;
					rapl_avail = true;
				}
			}
		}
		if (!first_sample && rapl_avail) {
			rapl_watts = (double)total_energy_uj /
				     (interval_sec * 1000000.0);
		}
	}

	*out_act = total_act;
	*out_read = total_read;
	*out_write = total_write;
	*out_cke_high = cke_high;
	*out_rapl_watts = rapl_watts;
	*out_rapl_avail = rapl_avail;
}

double estimate_dram_power(double interval_sec)
{
	if (!g_initialized) {
		if (dram_model_init(0, 0) != 0)
			return 0.0;
	}

	if (interval_sec <= 0.0)
		interval_sec = 1.0;

	uint64_t act, read, write;
	double cke_high, rapl_watts;
	bool rapl_avail;

	sample_hardware_metrics(&act, &read, &write, &cke_high, &rapl_watts,
				&rapl_avail, interval_sec, false);

	double act_rate = (double)act / interval_sec;
	double read_rate = (double)read / interval_sec;
	double write_rate = (double)write / interval_sec;

	double power = (act_rate * g_weights.w_act) +
		       (read_rate * g_weights.w_read) +
		       (write_rate * g_weights.w_write) +
		       (cke_high * g_weights.w_ckeh) + g_weights.k_baseline;

	return power;
}

int dram_model_step(struct grp_group *grp, dram_estimate_t *out_est)
{
	int64_t now_ns = CFG_TIME_MONOTONIC_NS();
	bool first_sample = (g_prev_step_ts_ns == 0);
	double interval_sec = 1.0;

	if (!first_sample && now_ns > g_prev_step_ts_ns) {
		interval_sec = (double)(now_ns - g_prev_step_ts_ns) / 1e9;
	}
	g_prev_step_ts_ns = now_ns;

	uint64_t act, read, write;
	double cke_high, rapl_watts;
	bool rapl_avail;

	sample_hardware_metrics(&act, &read, &write, &cke_high, &rapl_watts,
				&rapl_avail, interval_sec, first_sample);

	double act_rate = (double)act / interval_sec;
	double read_rate = (double)read / interval_sec;
	double write_rate = (double)write / interval_sec;

	double power = (act_rate * g_weights.w_act) +
		       (read_rate * g_weights.w_read) +
		       (write_rate * g_weights.w_write) +
		       (cke_high * g_weights.w_ckeh) + g_weights.k_baseline;

	if (out_est) {
		out_est->act_count = act;
		out_est->read_count = read;
		out_est->write_count = write;
		out_est->act_rate = act_rate;
		out_est->read_rate = read_rate;
		out_est->write_rate = write_rate;
		out_est->cke_high = cke_high;
		out_est->interval_sec = interval_sec;
		out_est->power_watts = power;
		out_est->rapl_power_watts = rapl_watts;
		out_est->rapl_available = rapl_avail;
		out_est->error_percent = 0.0;
		if (rapl_avail && rapl_watts > 0.0) {
			out_est->error_percent =
				fabs(power - rapl_watts) / rapl_watts * 100.0;
		}
	}

	/* Expose power in microwatts and microjoules */
	grp->jw.uw_ts_ns = now_ns;
	grp->jw.uj_ts_ns = now_ns;
	/*FIX:
	 * rapl measures the DC poewr on the motehrboard rails
	 * the paper/model predicts the physical energy of dram activity
	 * grp_poll() in syspower applies CFG_EFFICIENCY (in dconf) to give true estimated wall power
	 *
	 * the model accurary is dependent on comparing with RAPL, so don't multiply by CFG_EFFICIENCY
	 * */
	/* grp->jw.uw = (int64_t)(power * 1000000.0) * CFG_EFFICIENCY; */
	grp->jw.uw = (int64_t)(power * 1000000.0);
	if (!first_sample) {
		grp->jw.uj += (int64_t)(power * interval_sec * 1000000.0);
	}

	return 0;
}

/* some linear algebra bs */
/* Guass-Jordan elimination and Tikhonov/Ridge Regularization */
static int solve_ols_5(double A[5][5], double b[5], double out[5])
{
	double M[5][6];
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++)
			M[i][j] = A[i][j];
		M[i][5] = b[i];
	}

	for (int p = 0; p < 5; p++) {
		int max_r = p;
		double max_v = fabs(M[p][p]);
		for (int r = p + 1; r < 5; r++) {
			if (fabs(M[r][p]) > max_v) {
				max_v = fabs(M[r][p]);
				max_r = r;
			}
		}
		if (max_v < 1e-12)
			return -1;

		if (max_r != p) {
			for (int j = 0; j < 6; j++) {
				double tmp = M[p][j];
				M[p][j] = M[max_r][j];
				M[max_r][j] = tmp;
			}
		}

		double pivot = M[p][p];
		for (int j = p; j < 6; j++)
			M[p][j] /= pivot;

		for (int i = 0; i < 5; i++) {
			if (i != p) {
				double factor = M[i][p];
				for (int j = p; j < 6; j++)
					M[i][j] -= factor * M[p][j];
			}
		}
	}

	for (int i = 0; i < 5; i++)
		out[i] = M[i][5];

	return 0;
}

int dram_model_calibrate(int num_samples, int sample_interval_ms)
{
	if (!g_initialized) {
		if (dram_model_init(0, 0) != 0)
			return -1;
	}

	if (!g_rapl_detected) {
		CFG_PRINTF_ERR(
			"[dram_model] Error: Intel RAPL DRAM telemetry unavailable for calibration.\n");
		return -1;
	}

	if (num_samples < 10)
		num_samples = 30;
	if (sample_interval_ms <= 0)
		sample_interval_ms = 100;

	size_t buf_size = 64 * 1024 * 1024;
	char *synth_buf = CFG_CALLOC(1, buf_size);
	if (!synth_buf)
		return -1;

	double A[5][5] = { { 0 } };
	double b[5] = { 0 };

	/* Initial warmup sample */
	uint64_t act, read, write;
	double cke_high, rapl_watts;
	bool rapl_avail;
	sample_hardware_metrics(&act, &read, &write, &cke_high, &rapl_watts,
				&rapl_avail, 0.1, true);

	for (int s = 0; s < num_samples; s++) {
		int phase = s % 4;
		int64_t t_start = CFG_TIME_MONOTONIC_NS();

		if (phase == 0) {
			/* Idle */
			usleep((useconds_t)sample_interval_ms * 1000);
		} else if (phase == 1) {
			/* Sequential read */
			volatile char sum = 0;
			int64_t deadline =
				t_start +
				(int64_t)sample_interval_ms * 1000000LL;
			while (CFG_TIME_MONOTONIC_NS() < deadline) {
				for (size_t i = 0; i < buf_size; i += 64)
					sum = (char)(sum + synth_buf[i]);
			}
		} else if (phase == 2) {
			/* Sequential write */
			int64_t deadline =
				t_start +
				(int64_t)sample_interval_ms * 1000000LL;
			int val = s;
			while (CFG_TIME_MONOTONIC_NS() < deadline) {
				for (size_t i = 0; i < buf_size; i += 64)
					synth_buf[i] = (char)(val++);
			}
		} else {
			/* Strided read/write activating new rows */
			volatile char sum = 0;
			int64_t deadline =
				t_start +
				(int64_t)sample_interval_ms * 1000000LL;
			size_t stride = 4096;
			while (CFG_TIME_MONOTONIC_NS() < deadline) {
				for (size_t i = 0; i < buf_size; i += stride) {
					synth_buf[i] = (char)(synth_buf[i] + 1);
					sum = (char)(sum + synth_buf[i]);
				}
			}
		}

		int64_t t_end = CFG_TIME_MONOTONIC_NS();
		double dt = (double)(t_end - t_start) / 1e9;
		if (dt <= 0.0)
			dt = (double)sample_interval_ms / 1000.0;

		sample_hardware_metrics(&act, &read, &write, &cke_high,
					&rapl_watts, &rapl_avail, dt, false);

		if (!rapl_avail || rapl_watts <= 0.0)
			continue;

		double x[5];
		x[0] = (double)act / dt;
		x[1] = (double)read / dt;
		x[2] = (double)write / dt;
		x[3] = cke_high;
		x[4] = 1.0;

		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++)
				A[i][j] += x[i] * x[j];
			b[i] += x[i] * rapl_watts;
		}
	}

	CFG_FREE(synth_buf);

	/* Tikhonov / Ridge regularization */
	for (int i = 0; i < 5; i++)
		A[i][i] += 1e-6;

	double solved[5];
	if (solve_ols_5(A, b, solved) != 0) {
		CFG_PRINTF_ERR(
			"[dram_model] Calibration error: OLS matrix inversion failed.\n");
		return -1;
	}

	/* Ensure non-negative physical bounds */
	if (solved[0] <= 0.0)
		solved[0] = 36.3e-9;
	if (solved[1] <= 0.0)
		solved[1] = 22.8e-9;
	if (solved[2] <= 0.0)
		solved[2] = 21.7e-9;
	if (solved[3] < 0.0) {
		solved[4] += solved[3];
		solved[3] = 0.0;
	}
	if (solved[4] < 0.0) {
		solved[3] += solved[4];
		solved[4] = 0.0;
	}
	if (solved[3] + solved[4] <= 0.0) {
		solved[3] = 2.112 * (double)g_dimm_count;
		solved[4] = 1.663 * (double)g_dimm_count;
	}

	g_weights.w_act = solved[0];
	g_weights.w_read = solved[1];
	g_weights.w_write = solved[2];
	g_weights.w_ckeh = solved[3];
	g_weights.k_baseline = solved[4];

	CFG_PRINTF_OUT("[dram_model] OLS Calibration successful:\n");
	CFG_PRINTF_OUT("  w_act:      %.3e J/act\n", g_weights.w_act);
	CFG_PRINTF_OUT("  w_read:     %.3e J/read\n", g_weights.w_read);
	CFG_PRINTF_OUT("  w_write:    %.3e J/write\n", g_weights.w_write);
	CFG_PRINTF_OUT("  w_ckeh:     %.4f W\n", g_weights.w_ckeh);
	CFG_PRINTF_OUT("  k_baseline: %.4f W\n", g_weights.k_baseline);
	return 0;
}

int mdl_gen_dram_activity(struct grp_group *grp)
{
	if (!g_initialized) {
		int dimms = (int)grp->hw.len;
		if (dram_model_init(dimms, 1) != 0)
			return -1;
	}

	dram_estimate_t est;
	if (dram_model_step(grp, &est) != 0)
		return -1;

	if (est.rapl_available && est.rapl_power_watts > 0.0) {
		CFG_PRINTF_ERR(
			"[dram_model] RAPL: %.3f W | Model: %.3f W (error: %.2f%%) | Act/s: %.0f Read/s: %.0f Write/s: %.0f CKE: %.2f\n",
			est.rapl_power_watts, est.power_watts,
			est.error_percent, est.act_rate, est.read_rate,
			est.write_rate, est.cke_high);
	}

	return 0;
}

int mdl_gen_dram_david10(struct grp_group *grp)
{
	return mdl_gen_dram_activity(grp);
}
