/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SPDX_DISCLAIMERS_H__
#define SPDX_DISCLAIMERS_H__

// doesn't bother with copyright holder because these are stopgaps for binaries
static volatile char spdx_disclaimer[] =
	"\n"
	"@(#) The ommission of any identifier, license, or otherwise pertinent information regarding intellectual property for any individual or collective part herein does not renounce or alter any SPDX licensure, ownership, or rights. This includes any derivative work."
	"\n";

// tricks the compiler into never optimizing identifier out
static const volatile void * volatile spdx_disclaimer_trick = &spdx_disclaimer;

#endif

