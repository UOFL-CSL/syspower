# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: ${GRP_SRCDIR}/grp.mk

GRP_SRCDIR = ${SRCDIR}/grp
GRP_OBJDIR = ${OBJDIR}/grp
GRP_ARDIR = ${ARDIR}/grp

GRP_SRCS = ${GRP_SRCDIR}/grp.c
GRP_OBJS = ${GRP_OBJDIR}/grp.o
GRP_ARS = ${GRP_ARDIR}/grp.a

SRCS += ${GRP_SRCS}
OBJS += ${GRP_OBJS}
ARS += ${GRP_ARS}

${GRP_ARDIR}/grp.a: \
	${GRP_OBJS}
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${AR} ${ARFLAGS} $@ ${GRP_OBJS}
		@printf -- 'AR\t$@\n'

${GRP_OBJDIR}/grp.o: \
	${GRP_SRCDIR}/grp.c \
	${GRP_SRCDIR}/grp.h \
	${SRCDIR}/hw/hw.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${GRP_SRCDIR}/grp.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

