/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UT_ERROR_H__
#define UT_ERROR_H__

//internal
#include <src/dconf.h>

// external
#include LIB_ERRNO
#include LIB_IO
#include LIB_VARARG

#define ut_error(...) \
	ut_error_real( \
		__FILE__, \
		__LINE__, \
		__func__, \
		__VA_ARGS__ \
	)

static inline void ut_error_real(
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
	CFG_PRINTF_ERR("%s,%s:%d,%s(...),error:%d,%s\n", NAME, file, line, func, code, string);

	va_end(args);

	return;
}

#endif

