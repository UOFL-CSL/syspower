/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UT_PARENT_H__
#define UT_PARENT_H__

#include <stddef.h>

// taken from the type unsafe linux `container_of` macro
// takes a:
// - pointer to a parent struct's member
// - the type of the parent struct
// - name of the member
// and maps deterministically the struct's address
// be 100% certain of the type of the parent before using this macro
#define UT_PARENT(ptr, type, member) ({				\
	((type *)((char *)(ptr - offsetof(type, member)));	\
})

#endif

