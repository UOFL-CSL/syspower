# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
	stdenv,
	lib,
	src ? ./.
}: stdenv.mkDerivation {
	pname = "getopt";
	version = "1.2.6";
	
	getoptLongC = fetchurl {
		url = "https://git.musl-libc.org/cgit/musl/plain/src/misc/getopt_long.c?id=v1.2.6";
		sha256 = "04i2l9fj0s1w6ah4a0mb4px8l3zsg9rmbac6icmr0z6f8wvpnbxp";
	};

	getoptC = fetchurl {
		url = "https://git.musl-libc.org/cgit/musl/plain/src/misc/getopt.c?id=v1.2.6";
		sha256 = "1qpjsi6647yiqih6j6gb8361cdicas28zci6nvl5i3wmai07s0kb";
	};
	getoptH = fetchurl {
		url = "https://git.musl-libc.org/cgit/musl/plain/include/getopt.h?id=v1.2.6";
		sha256 = "1r5lzdw0s4gq547hnbaibq4vfd53mzyki0qhl0672qx6zcay5l7i";
	};
	
	# need to write the actual build steps and utilize getopt in program
}
