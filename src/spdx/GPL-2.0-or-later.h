/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SPDX_GPL_2_0_OR_LATER_LICENSE_H__
#define SPDX_GPL_2_0_OR_LATER_LICENSE_H__

// doesn't bother with copyright holder because these are stopgaps for binaries
static volatile char spdx_gpl_2_0_or_later_identifier[] =
	"\n"
	"@(#) SPDX" "-License-Identifier: GPL-2.0-or-later"
	"\n";

// tricks the compiler into never optimizing identifier out
static const volatile void * volatile spdx_gpl_2_0_or_later_trick = &spdx_gpl_2_0_or_later_identifier;

#endif

