/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DEFCONF_H__
#define DEFCONF_H__
	#ifndef NAME
		#define NAME			"syspower"
	#endif

	#undef 	CFG_DEFAULTS
	#define CFG_INTERVAL		1

	#undef 	CFG_CMP_DEFAULTS
	#define CMP_DEFAULT_WARNING 	1

	// in microwatts
	#define CMP_DISK_DEFAULT_IDLE	55000
	#define CMP_DISK_DEFAULT_ACTIVE	7000000

	#undef 	CFG_MDL_DEFAULTS

	// bytes per second
	#define MDL_DISK_BW_DEFAULT_MAX 3500000000
#endif

