# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
	nixpkgs ? import <nixpkgs> {},
	src ? fetchGit ./.,
	source ? "unknown_version",
	target ? "unknown_target"
}: (nixpkgs.callPackage ./package.nix {
	getopt = nixpkgs.callPackage ./foreign/getopt/package.nix {};
}).overrideAttrs (finalAttrs: oldAttrs: {
	# set to the local source
	src = src;
	# development version
	version = "local-${builtins.toString src.lastModifiedDate}-${builtins.substring 0 6 (builtins.convertHash {
		hash = src.narHash;
		toHashFormat = "base16";
	})}";
	# build defaults recommended for development
	env = (oldAttrs.env or {}) // {
		NAME = finalAttrs.pname;
		VERSION = finalAttrs.version;
		PREFIX = "$(out)";
		SOURCE = source;
		TARGET = target;
		CLOSURE = 0;
		# define units you want to (or don't want to) build
		MAKES = builtins.concatStringsSep " " (map
			(p: nixpkgs.lib.strings.removePrefix "${src}/" (builtins.toString p))
			(builtins.filter
				(filePath:
					(nixpkgs.lib.hasSuffix ".mk" "${filePath}")
					# i.e. exceptions here--see lists.mk for units
					&& ("${filePath}" != "${src}/src/list.mk")
				)
				(nixpkgs.lib.filesystem.listFilesRecursive "${src}")
			)
		);
	};
	
	# custom development shell
	passthru = (oldAttrs.passthru or {}) // {
		shell = nixpkgs.mkShell {
			name = "${finalAttrs.env.NAME}--session/version=${finalAttrs.env.VERSION}--source=${finalAttrs.env.SOURCE}--target=${finalAttrs.env.TARGET}";
			env = finalAttrs.env;
			meta = finalAttrs.meta;
			inputsFrom = [ finalAttrs.finalPackage ];
			buildInputs = [];
			nativeBuildInputs = [
				nixpkgs.buildPackages.qemu
				nixpkgs.buildPackages.gdb
			];
			shellHook = ''
				export PS1="\n\[\033[1;33m\][${finalAttrs.passthru.shell.name}:\w]\$\[\033[0m\] "
				export NIX_SHELL_PRESERVE_PROMPT=1

				export LANG=C.UTF-8
				export LC_ALL=C.UTF-8
				export QEMU_LD_PREFIX="${nixpkgs.stdenv.cc.libc}"
				export GUEST_LIBS="${nixpkgs.lib.makeLibraryPath finalAttrs.buildInputs}"
				export QEMU_SET_ENV="LD_LIBRARY_PATH=$GUEST_LIBS"

				printf -- 'Run `pseudo {utility}` to emulate in the target platform!\n'
				alias pseudo="qemu-${nixpkgs.stdenv.targetPlatform.qemuArch} -L \$QEMU_LD_PREFIX"
			'';
		};
	};
})

