# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
	stdenv,
	lib,
	fetchFromGitHub,
	getopt,
	hwloc,
	udev
}: stdenv.mkDerivation (finalAttrs: {
	pname = "syspower";
	version = "unknown_version";
	src = fetchFromGitHub {
		owner = "UOFL-CSL";
		repo = "syspower";
		rev = "unknown";
		hash = "---";
	};

	env = {
		NAME = finalAttrs.pname;
		VERSION = finalAttrs.version;
		PREFIX = "$(out)";
		SOURCE = "unknown_source";
		TARGET = "unknown_target";
		CLOSURE = 0;
	};

	# libhwloc
	# libudev
	nativeBuildInputs = [];
	buildInputs = [
		hwloc
		udev
	];

	# unit tests
	# doCheck = true;
	# checkPhase = ''
	# 	runHook preCheck
	# 	command
	# 	runHook postCheck
	# '';
	
	# passthru = {
	#	# integration tests
	#	tests = ...;
	# };

	meta = {
		description = "A user space application for simplistic but comprehensive power estimation.";
		license = lib.licenses.gpl2Plus;
		# doesn't use meta.platforms
		# expects manual transfer of flake logic to all-packages.nix
		# meta.platforms is too ambiguous
		homepage = "https://github.com/UOFL-CSL/syspower";
		mainProgram = finalAttrs.pname;
		maintainers = [
			{
				name = "Wendell Conrad Richmond";
				email = "wendell.richmond@louisville.edu";
				github = "zenirta";
				personal = "zenirta@posteo.com";
				sourcehut = "~zenirta";
			}
			{
				name = "Nihat Altiparmak";
				email = "nihat.altiparmak@louisville.edu";
			}
		];
	};
})

