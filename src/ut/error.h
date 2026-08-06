/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UT_ERROR__
#define UT_ERROR__

// internal
#include <src/dconf.h>

// external
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define ut_error(...) \
	ut_error_real( \
		__FILE__, \
		__LINE__, \
		__func__, \
		__VA_ARGS__ \
	)

void ut_error_real(
	const char *file,
	int line,
	const char *func,
	int code,
	const char *fmt,
	...
) {
	char string[1024];
	va_list args;

	va_start(args, fmt);
	
	vsnprintf(string, sizeof(string), fmt, args);
	fprintf(stderr, "%s: %s: %d: %s: error: %s\n", NAME, file, line, func, string);

	va_end(args);

	_exit(code);
}

#endif

