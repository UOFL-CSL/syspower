# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
	nixpkgs ? import <nixpkgs> {},
	src ? fetchGit ./.,
	sources ? [],
	targets ? []
}: let
	# real defaults that fall back to current system if hermetic flake input isn't provided
	sources' = if sources == [] then [builtins.currentSystem] else sources;
	targets' = if targets == [] then [nixpkgs.stdenv.buildPlatform.config] else targets;

	# mapper that runs functions over a list and combines set outputs together
	mapper = function: list:
		builtins.listToAttrs (builtins.map
			(item: {
				name = item;
				value = (function item).${item};
			})
		list);
	
	# gets every target for a source properly and calls package.nix
	targeter = pkgs: src: source: target: let
			# find the proper pkgsCross string
			crossTarget = pkgs.lib.findFirst
				(key: (pkgs.pkgsCross.${key}.stdenv.hostPlatform.config or "") == target)
				null
				(builtins.attrNames pkgs.pkgsCross);
			# use pkgsCross on the already routed nixpkgs localSystem to cross compile
			pkgs' = if (crossTarget == null || !(pkgs.pkgsCross ? ${crossTarget})) then
					pkgs
				else
					pkgs.pkgsCross.${crossTarget};

		# actually call the build with specific config options
		in {
			${target} = import ./local.nix {
				nixpkgs = pkgs';
				src = src;
				source = source;
				target = target;
			};
		};

	# sets up a mapping from all sources to the targeter
	sourcer = pkgs: src: targets: source: let
			# if legacyPackages is available, use it
			# otherwise fall back on the already insantiated nixpkgs
			pkgs' = if pkgs ? legacyPackages then
				pkgs.legacyPackages.${source}
			else
				pkgs;

			stargeter = targeter pkgs' src source;
			derivations = mapper stargeter targets;
			default = targeter pkgs' src source pkgs'.stdenv.buildPlatform.config;
		in {
			# actually construct the derivations with the source -> target pairs
			${source} = derivations // {
				# keep a default pair whereby the source is the current system
				default = default.${builtins.head (builtins.attrNames default)};
			};
		};

	# a builder wrapper that takes a list of sources and targets
	builder = pkgs: src: sources: targets: let
			tsourcer = sourcer pkgs src targets;
		in
			mapper tsourcer sources;
# actually run everything and provide a set of build targets
in builder nixpkgs src sources' targets'

