/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// generic_cpu

#ifndef HW_GEN_CPU_CPU_H__
#define HW_GEN_CPU_CPU_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_gen_cpu = (const char *[]){
	"generic_cpu",
	NULL
};

int hw_gen_cpu_init(
	struct hw_dev *dev,
	const char *id,
	enum hw_locale locale,
	uint8_t efficiency
);

#endif

