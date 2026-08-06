/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>
#include <src/spdx/disclaimer.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>

// external
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>


void exit_signal(int signal) {
	fprintf(stderr, "signal: %d: %s!\n", signal, strsignal(signal));
	_exit(128 + signal);
}

int main(void) {
	struct sigaction sa = {
		.sa_handler = exit_signal
	};

	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	grp_init();

	return 0;
}

