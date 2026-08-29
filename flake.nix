# SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
	inputs.nixpkgs.url = "github:NixOs/nixpkgs/nixos-26.05";

	outputs = {
		self,
		nixpkgs
	}: let
		builder = import ./default.nix;

		# https://github.com/NixOS/nixpkgs/blob/master/lib/systems/flake-systems.nix
		sources = [
			"x86_64-linux"
		];
		# https://github.com/NixOS/nixpkgs/blob/master/lib/systems/examples.nix
		targets = [
			"x86_64-unknown-linux-gnu"
		];
	in {
		packages = (builder {
			inherit nixpkgs sources targets;
			src = self;
		});
	};
}

