# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: ${SRCDIR}/list.mk

MAKES ?= ${SRCDIR}/grp/grp.mk \
	${SRCDIR}/hw/hw.mk \
	${SRCDIR}/mdl/mdl.mk \

include ${MAKES}

