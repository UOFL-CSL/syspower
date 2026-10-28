/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MDL_GEN_DRAM_DRAM_H__
#define MDL_GEN_DRAM_DRAM_H__

struct grp_group;

int mdl_gen_dram_intel_uncore_free(struct grp_group *grp);
int mdl_gen_dram_activity(struct grp_group *grp);
int mdl_gen_dram_david10(struct grp_group *grp);

#endif
