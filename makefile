# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

.POSIX:
.PRECIOUS: makefile
.PHONY: __default __always all clean

# configurable default
__default: \
	all
		@:

NAME ?= syspower
# based on commit date
VERSION ?= unknown_version
# system app is being built for
TARGET ?= unknown_target
# rebuild the closure
CLOSURE ?= 0

PREFIX ?= /usr/local

SRCDIR = src
BUILDDIR = build/${VERSION}/${TARGET}
BINDIR = ${BUILDDIR}/bin
OBJDIR = ${BUILDDIR}/obj
ARDIR = ${BUILDDIR}/ar

INCLUDES = -I. -lhwloc -ludev
DEFINES = -DNAME=\"${NAME}\" \
	-DVERSION=\"${VERSION}\" \
	-DTARGET=\"${TARGET}\"

CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
	${DEFINES} \
	-O2 \
	-fhardened \
	-Wall -Wextra \
	-Wformat -Wformat=2 \
	-Wconversion -Wsign-conversion \
	-Wtrampolines \
	-Wimplicit-fallthrough \
	-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 \
	-D_GLIBCXX_ASSERTIONS \
	-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST \
	-fstrict-flex-arrays=3 \
	-fstack-clash-protection -fstack-protector-strong \
	-Wl,-z,nodlopen \
	-Wl,-z,noexecstack \
	-Wl,-z,relro \
	-Wl,-z,now \
	-Wl,--as-needed \
	-Wl,--no-copy-dt-needed-entries \
	-Wno-hardened \
	-Werror=format-security \
#	-Werror

AR = ar
ARFLAGS = rcs

SRCS = ${SRCDIR}/main.c
OBJS = ${OBJDIR}/main.o
ARS = ${ARDIR}/main.a

# MAKES defined here
include ${SRCDIR}/list.mk

# rules that run no matter what for blanket targets--clean, all, etc.
__always:
		@:

all: \
	__always \
	${BUILDDIR}/.timestamp \
	${BINDIR}/${NAME}-${VERSION} \
	closure/${SOURCE}.${TARGET}.txt
		@:

closure/${SOURCE}.${TARGET}.txt: \
	${BUILDDIR}/.timestamp
		@if [ "${CLOSURE}" -gt 0 ]; then \
			DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}; \
			printf -- 'CLOSURE\t$@\n'; \
			command -v nix-store > /dev/null \
			&& command -v nix-instantiate > /dev/null \
			&& command -v nix-build > /dev/null \
			&& CLOSURE=0 nix-store -q --tree $$( \
				nix-instantiate -A ${SOURCE}.${TARGET} \
			) $$( \
				nix-build -A ${SOURCE}.${TARGET} \
			) | sed -E 's|/nix/store/([a-z0-9]{32})-(.*)$$|\2 (\1)|g; s/(\[...\]) (\([a-z0-9]{32}\))/\2 \1/g' \
			> $@; \
		fi


${BUILDDIR}/.timestamp: \
	makefile \
	${SRCDIR}/list.mk \
	${MAKES}
		@${MAKE} -s clean
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@touch $@

${BINDIR}/${NAME}-${VERSION}: \
	${ARS}
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ $< ${ARS} ${CFLAGS} ${INCLUDES}
		@printf -- 'LD\t$@\n'

${ARDIR}/main.a: \
	${OBJDIR}/main.o
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${AR} ${ARFLAGS} $@ $<
		@printf -- 'AR\t$@\n'


${OBJDIR}/main.o: \
	${SRCDIR}/main.c \
	${SRCDIR}/spdx/disclaimer.h \
	${SRCDIR}/spdx/GPL-2.0-or-later.h \
	${SRCDIR}/dconf.h
		@DIR=$@ && DIR=$${DIR%/*} && mkdir -p $${DIR}
		@${CC} -o $@ -c $< ${CFLAGS} ${INCLUDES}
		@printf -- 'CC\t$@\n'

clean: \
	__always
		@rm -rf ${BUILDDIR}
		@printf -- 'CLEAN\t${BINDIR}/${NAME}-${VERSION}\n'

install: \
	__always
		@mkdir -p ${PREFIX}/bin
		@cp ${BINDIR}/${NAME}-${VERSION} ${PREFIX}/bin/${NAME}
		@printf -- 'INSTALL\t${BINDIR}/${NAME}-${VERSION} -> ${PREFIX}/${NAME}\n'

uninstall: \
	__always
		@rm -f ${PREFIX}/bin/${NAME}
		@printf -- 'UNINSTALL\t${PREFIX}/${NAME}\n'

