# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: ${HW_SRCDIR}/hw.mk

HW_SRCDIR = ${SRCDIR}/hw
HW_OBJDIR = ${OBJDIR}/hw
HW_ARDIR = ${ARDIR}/hw



HW_SRCS = ${HW_SRCDIR}/hw.c \
	${HW_SRCDIR}/gen/cpu/cpu.c \
	${HW_SRCDIR}/mpn/dell/0H74DC/0H74DC.c \
	${HW_SRCDIR}/mpn/intel/BX8071512700/BX8071512700.c \
	${HW_SRCDIR}/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.c \
	${HW_SRCDIR}/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.c \
	${HW_SRCDIR}/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.c
HW_OBJS = ${HW_OBJDIR}/hw.o \
	${HW_OBJDIR}/gen/cpu/cpu.o \
	${HW_OBJDIR}/mpn/dell/0H74DC/0H74DC.o \
	${HW_OBJDIR}/mpn/intel/BX8071512700/BX8071512700.o \
	${HW_OBJDIR}/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.o \
	${HW_OBJDIR}/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.o \
	${HW_OBJDIR}/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.o
HW_ARS = ${HW_ARDIR}/hw.a

SRCS += ${HW_SRCS}
OBJS += ${HW_OBJS}
ARS += ${HW_ARS}

${HW_ARDIR}/hw.a: \
	${HW_OBJS}
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${AR} ${ARFLAGS} $@ ${HW_OBJS}
		@printf -- 'AR\t$@\n'

${HW_OBJDIR}/hw.o: \
	${HW_SRCDIR}/hw.c \
	${HW_SRCDIR}/hw.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${HW_SRCDIR}/hw.mk
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/gen/cpu/cpu.o: \
	${HW_SRCDIR}/gen/cpu/cpu.c \
	${HW_SRCDIR}/gen/cpu/cpu.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/mpn/dell/0H74DC/0H74DC.o: \
	${HW_SRCDIR}/mpn/dell/0H74DC/0H74DC.c \
	${HW_SRCDIR}/mpn/dell/0H74DC/0H74DC.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/mpn/intel/BX8071512700/BX8071512700.o: \
	${HW_SRCDIR}/mpn/intel/BX8071512700/BX8071512700.c \
	${HW_SRCDIR}/mpn/intel/BX8071512700/BX8071512700.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.o: \
	${HW_SRCDIR}/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.c \
	${HW_SRCDIR}/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.o: \
	${HW_SRCDIR}/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.c \
	${HW_SRCDIR}/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

${HW_OBJDIR}/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.o: \
	${HW_SRCDIR}/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.c \
	${HW_SRCDIR}/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

