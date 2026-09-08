/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DEFCONF_H__
#define DEFCONF_H__

// interval to collect metrics
#ifndef CFG_INTERVAL
#define CFG_INTERVAL 1
#endif

// print override warning if user didn't flip overriden
#ifndef CFG_OVERRIDDEN
#define CFG_OVERRIDDEN 0
#endif

// generic overrides AND defaults
// defaults are currently based on CSL's testbed
#ifndef CFG_EFFICIENCY
#define CFG_EFFICIENCY 92
#endif
// Linux lies about fan hardware in hwmon, so manually specify your hwmons
// space-delimited
#ifndef CFG_HWMONS_FANS
#define CFG_HWMONS_FANS "/sys/class/hwmon/hwmon6"
#endif
#ifndef CFG_GENERIC_DRAM_UW_MIN_OVERRIDE
#define CFG_GENERIC_DRAM_UW_MIN_OVERRIDE 155000
#endif
#ifndef CFG_GENERIC_DRAM_UW_MAX_OVERRIDE
#define CFG_GENERIC_DRAM_UW_MAX_OVERRIDE 3170000
#endif
#ifndef CFG_GENERIC_STORAGE_UW_MIN_OVERRIDE
#define CFG_GENERIC_STORAGE_UW_MIN_OVERRIDE 5 * 10 * 1000
#endif
#ifndef CFG_GENERIC_STORAGE_UW_MAX_OVERRIDE
#define CFG_GENERIC_STORAGE_UW_MAX_OVERRIDE 7 * 1000 * 1000
#endif
#ifndef CFG_GENERIC_NIC_UW_MIN_OVERRIDE
#define CFG_GENERIC_NIC_UW_MIN_OVERRIDE 0
#endif
#ifndef CFG_GENERIC_NIC_UW_MAX_OVERRIDE
#define CFG_GENERIC_NIC_UW_MAX_OVERRIDE 5 * 100 * 1000
#endif

#ifndef CFG_GENERIC_FAN_UW_MIN_OVERRIDE
#define CFG_GENERIC_FAN_UW_MIN_OVERRIDE 0
#endif
#ifndef CFG_GENERIC_FAN_UW_MAX_OVERRIDE
#define CFG_GENERIC_FAN_UW_MAX_OVERRIDE 18 * 1000 * 1000
#endif

// in microwatts
#define CMP_DISK_DEFAULT_IDLE 55000
#define CMP_DISK_DEFAULT_ACTIVE 7000000

#undef CFG_MDL_DEFAULTS

// bytes per second
#define MDL_DISK_BW_DEFAULT_MAX 3500000000

// libraries
#define LIB_BOOL <stdbool.h>
#define LIB_DEF <stddef.h>
#define LIB_ERRNO <errno.h>

#define LIB_INT <stdint.h>

#define LIB_STRING <string.h>
#define CFG_STRNCPY(dest, src, size) strlcpy(dest, src, size)
#define CFG_STRNCAT(dest, src, size) strncat(dest, src, size - strlen(dest) - 1)
#define CFG_STRTOLL(s, base, res)              \
	({                                     \
		*res = strtoll(s, NULL, base); \
		0;                             \
	})

#define LIB_TIME <time.h>
#define CFG_TIME_MONOTONIC_NS()                                       \
	({                                                            \
		struct timespec _ts;                                  \
		clock_gettime(CLOCK_MONOTONIC, &_ts);                 \
		((long long)_ts.tv_sec * 1000000000LL) + _ts.tv_nsec; \
	})
#define CFG_TIME_REAL_NS()                                            \
	({                                                            \
		struct timespec _ts;                                  \
		clock_gettime(CLOCK_REALTIME, &_ts);                  \
		((long long)_ts.tv_sec * 1000000000LL) + _ts.tv_nsec; \
	})
#define CFG_SLEEP_NS(ns)                         \
	({                                       \
		struct timespec _ts;             \
		_ts.tv_sec = ns / 1000000000LL;  \
		_ts.tv_nsec = ns % 1000000000LL; \
		nanosleep(&_ts, NULL);           \
	})

#define LIB_VARARG <stdarg.h>

#define LIB_IO <stdio.h>
#define CFG_PRINTF_OUT(...) fprintf(stdout, __VA_ARGS__)
#define CFG_PRINTF_ERR(...) fprintf(stderr, __VA_ARGS__)

#define LIB_STD <stdlib.h>
#define CFG_MALLOC(bytes) malloc(bytes)
#define CFG_CALLOC(n, bytes) calloc(n, bytes)
#define CFG_REALLOC(ptr, bytes) realloc(ptr, bytes)
#define CFG_FREE(ptr) free(ptr)
#define CFG_DIE(status) _Exit(status)
#endif
