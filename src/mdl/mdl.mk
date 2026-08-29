# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: ${MDL_SRCDIR}/mdl.mk

MDL_SRCDIR = ${SRCDIR}/mdl
MDL_OBJDIR = ${OBJDIR}/mdl
MDL_ARDIR = ${ARDIR}/mdl

MDL_SRCS = ${MDL_SRCDIR}/mdl.c \
	${MDL_SRCDIR}/nvml/nvml.c \
	${MDL_SRCDIR}/rapl/rapl.c \
	${MDL_SRCDIR}/gen/dram/uncore_free.c \
	${MDL_SRCDIR}/gen/motherboard/flat.c \
	${MDL_SRCDIR}/gen/fan/fan.c \
	${MDL_SRCDIR}/gen/nic/sysfs_bw.c \
	${MDL_SRCDIR}/gen/storage/sysfs_bw.c
MDL_OBJS = ${MDL_OBJDIR}/mdl.o \
	${MDL_OBJDIR}/nvml/nvml.o \
	${MDL_OBJDIR}/rapl/rapl.o \
	${MDL_OBJDIR}/gen/dram/uncore_free.o \
	${MDL_OBJDIR}/gen/motherboard/flat.o \
	${MDL_OBJDIR}/gen/fan/fan.o \
	${MDL_OBJDIR}/gen/nic/sysfs_bw.o \
	${MDL_OBJDIR}/gen/storage/sysfs_bw.o
MDL_ARS = ${MDL_ARDIR}/mdl.a

SRCS += ${MDL_SRCS}
OBJS += ${MDL_OBJS}
ARS += ${MDL_ARS}

${MDL_SRCDIR}/mdl.mk: \
	${SRCDIR}/grp/grp.mk \
	${SRCDIR}/hw/hw.mk \
	${SRCDIR}/dconf.h \
	${SRCDIR}/ut/error.h
		@touch $@


${MDL_ARDIR}/mdl.a: \
	${MDL_SRCDIR}/mdl.mk \
	${MDL_OBJS}
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${AR} ${ARFLAGS} $@ ${MDL_OBJS}
		@printf -- 'AR\t$@\n'

${MDL_OBJDIR}/mdl.o: \
	${MDL_SRCDIR}/mdl.c \
	${SRCDIR}/hw/hw.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/gen/dram/uncore_free.o: \
	${MDL_SRCDIR}/gen/dram/uncore_free.c \
	${MDL_SRCDIR}/gen/dram/dram.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/gen/motherboard/flat.o: \
	${MDL_SRCDIR}/gen/motherboard/flat.c \
	${MDL_SRCDIR}/gen/motherboard/motherboard.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/gen/storage/sysfs_bw.o: \
	${MDL_SRCDIR}/gen/storage/sysfs_bw.c \
	${MDL_SRCDIR}/gen/storage/storage.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/gen/nic/sysfs_bw.o: \
	${MDL_SRCDIR}/gen/nic/sysfs_bw.c \
	${MDL_SRCDIR}/gen/nic/nic.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/nvml/nvml.o: \
	${MDL_SRCDIR}/nvml/nvml.c \
	${MDL_SRCDIR}/nvml/nvml.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/rapl/rapl.o: \
	${MDL_SRCDIR}/rapl/rapl.c \
	${MDL_SRCDIR}/rapl/rapl.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${MDL_OBJDIR}/gen/fan/fan.o: \
	${MDL_SRCDIR}/gen/fan/fan.c \
	${MDL_SRCDIR}/gen/fan/fan.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${MDL_SRCDIR}/mdl.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

