/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/disclaimer.h>
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <src/hw/hw.h>

// external
#include LIB_IO
#include LIB_STD
#include <signal.h>

void exit_signal(int signal) {
	CFG_PRINTF_OUT("signal: %d\n", signal);
	CFG_DIE(128 + signal);
}

int main(void) {
	int err;

	struct sigaction sa = {
		.sa_handler = exit_signal
	};
	
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);


	if ((err = grp_strap()))
		CFG_DIE(err);

	while (!grp_poll()) {}

	if ((err = grp_strip()))
		CFG_DIE(err);

	return 0;
}

