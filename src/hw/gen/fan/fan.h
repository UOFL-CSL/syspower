/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// generic_fan

#ifndef HW_GEN_FAN_FAN_H__
#define HW_GEN_FAN_FAN_H__

// internal
#include <src/dconf.h>
#include <src/hw/hw.h>

// external
#include LIB_INT
#include LIB_DEF

static const char **hw_gen_fan = (const char *[]){ "generic_fan", NULL };

int hw_gen_fan_init(struct hw_dev *dev, const char *id, enum hw_locale locale,
		    uint8_t efficiency);

#endif
