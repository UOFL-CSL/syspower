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
	"@(#) The ommission of any identifier, license, or otherwise pertinent information regarding intellectual property for any individual or collective part herein does not renounce or alter any SPDX licensure, ownership, or rights. That is regardless of order and includes any derivative work of any depth. These statements are intended as a binary disclaimer but also apply to the entirety of the work they reasonably sit with (i.e. the file tree from the project root, a VCS structure or repository, or correlated work via any data structure or intent)."
	"\n";

// tricks the compiler into never optimizing identifier out
static const volatile void *volatile spdx_disclaimer_trick = &spdx_disclaimer;

#endif
