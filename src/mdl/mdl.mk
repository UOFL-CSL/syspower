# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: ${MDL_SRCDIR}/mdl.mk

MDL_SRCDIR = ${SRCDIR}/mdl
MDL_OBJDIR = ${OBJDIR}/mdl
MDL_ARDIR = ${ARDIR}/mdl

MDL_SRCS = ${MDL_SRCDIR}/mdl.c
MDL_OBJS = ${MDL_OBJDIR}/mdl.o
MDL_ARS = ${MDL_ARDIR}/mdl.a

SRCS += ${MDL_SRCS}
OBJS += ${MDL_OBJS}
ARS += ${MDL_ARS}

${MDL_ARDIR}/mdl.a: \
	${MDL_OBJS}
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${AR} ${ARFLAGS} $@ ${MDL_OBJS}
		@printf -- 'AR\t$@\n'

${MDL_OBJDIR}/mdl.o: \
	${MDL_SRCDIR}/mdl.c \
	${MDL_SRCDIR}/mdl.h \
	${SRCDIR}/hw/hw.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

