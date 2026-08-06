<!--
SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond

SPDX-License-Identifier: GPL-2.0-or-later
-->

# syspower

`syspower` is a user-space application for simplistic but system-wide power
estimation.

This utility is product of the _Computer Systems Laboratory_ at the 
_University of Louisville_.

This is a living repository and is in flux.

- [getting started](#getting-started)
- [hardware](#hardware)
- [groups](#groups)
- [models](#models)
- [building](#building)
- [acknowledgements](#acknowledgements)
- [todo](#todo)

## getting started

To get useful results out of this program better defaults or specific device
definitions MUST be written. **`syspower` is in alpha and it is not yet
comprehensive.**

To get started, build `syspower` on your host machine according to
[building](#building).

`syspower` will attempt to enumerate your [hardware](#hardware) and default to its
current capabilities in [models](#models) and [groups](#groups) layout.

For more accurate results, please submit a PR with your device characteristics
in the appropriate files. Specifically:

    - create your devices' `src/hw/mpn/{manufacturer}/{MPN}.*` files
    - register the devices in `src/hw/hw.c` and `src/hw/hw.h`
    - if the devices are not covered by the current discovery logic or layout in
      `src/grp`, adjust the logic as necessary
    - if there are not appropriate models in `src/mdl` to wire the device to,
      create a rudimentary or advanced model, or shoehorn something

If you sufficiently describe your device and `syspower` can be molded
architecturally to handle it, we will do our best with the time and resources
we have.

---

Some additional notes.

Before using model estimators that infer minimal/maximal state based on real work
of a device, it is recommended to first run the utility under load.

This is particularly for generic device+model pairs that must use empirical
maximum bandwidth. Any model that requires this will emit a warning when loading
`syspower`.

Some models might eventually be written to specifically use empirical evidence.
After all:

    - manufacturer quality control produces variance
    - device performance degrades over time
    - throttling is a reality
    - interplay across device boundaries affects capabilities

It is the job of `syspower` and its ilk to acknowledge stateful properties.
This vein of utilities should provide methodology to estimate power specific to
the current, actual, real system. It should not base its estimate on
supposition about that specification.

The ultimate realization of that is on-board sensors. It is something our
compute architecture desparately needs.

`syspower` is simply a stopgap in lieu of that. It is also a practical, tangible,
demonstration of the severe importance of this real-time data.

## hardware

A hardware unit represents a potential device. Hardware is organized by
manufacturer part number (MPN) because it is both reliably unique and doesn't
describe anything.

`syspower` attempts to abstract the properties of a device away from an
identifier (or minimal set of identifiers) because it does not care if your
device is a CPU. It doesn't care about what it produced. It cares about what
it consumed.

It is your job to care about what it consumes.

The goal of `syspower`, and other tools that will grow from it, is to provide
tools that allow you to write logic that cares.

## groups

A group in `syspower` is a unit within a computer. Any device, or groups of
devices that are capable of performing work could be a potential group.

A RAID controller could be a group. An NVMe drive could be a group. The
CPU could be a group. GPUs could be groups. DIMMs, together, could form a group.
Fans could each be a group. Etc.

Boundaries for groups are important. Some devices are deeply
interdependent and require information from each other to accurately estimate
work. These are architectural constraints, and it is the responsibility of
`syspower` to make these considerations when delineating.

## models

The assumption of `syspower` is that every group is a unit capable of spending
energy to perform work. This derives the two core methodologies for this
utility:

    1. directly measure power from the sensors of the unit
    2. measure work/state of the unit -> estimate power of the unit

The second principle is the dynamic promise of `syspower` to this section. It is
simple, but it is effective.

To further generalize: given an arbitrary set of parameters, there is some
model that correlates to the energy needed to perform work.

The simple model that incepted this research makes a good example.

For most components, there is a direct "product" produced from the work that a
device performs. Although the product normally lacks descriptive nuance--i.e.
the bandiwdth a PCIe device is pushing--it is a bounded descriptor of the poles
of the unit.

This forms the foundation of `syspower`. The easiest guess we can make about
power is a linear interpolation of:

    `minimum_power + (product / potential) * (maximum_power - minimum_power)`

This provides a model based on a generalized parameter without intricate
consideration of device type or class.

This is simple, almost seemingly trite, because this has not been performed
consistently or comprehensively across system architecture before. It has never
been implemented in a way to give holisitc real-time power measurements.

Important notes about groups:
    - more complex models can be written for single devices or groups of devices
    - in `syspower`, models **strictly** map to groups
    - if a model is instantiated as part of a group **it is contractual that the
      group members will be able to gracefully attempt the model**
    - the above implies a **model** must be able to directly access generic
      hardware interfaces--these can be provided by `hw/` or externally

, `syspower` seeks to provide an architecture that enumerates device class
and provides the modularity for users to implement their own models.

## building

The build toolchain for `syspower` roughly looks like:

```
flake.lock -> flake.nix -> default.nix -> local.nix -> package.nix -> makefile |
                  foreign/                          ^                          |
                      {name*}/                      |                          |
                          package.nix --------------/                          |
                                                              ... <- dconf.h <-|
                                                  ... <- unit0.h <- unit0.mk <-|
                                                  ... <- unit1.h <- unit0.mk <-|

```

`Nix` is lazily evaluated, so this is fuzzy. This is especially in respect to
`local.nix`.

This build strategy is dubbed `Mix` and is ripped from `zenirta`.

`Mix` allows `syspower` to be built in several ways:

    - `make` assuming all dependencies are installed correctly
    - 'nix-shell -A x86_64-linux.default.shell` and `make` assuming `nixpkgs`
      is installed
    - `nix build x86_64-linux` assuming nixpkgs is installed
    - `nix build .#default` assuming nixpkgs and flakes are enabled

This allows for source controlled dependencies without adding them as a monorepo
or installing them on your host system.

The final three allow the entire dependency closure to be enumerated by the build
toolchain:

    - `Nix` will automatically install dependencies
    - those closures are fully dumped in `closures/`
    - the entire `source <--> target` matrix in `flake.nix` gains absolute and
      agnostic documentation of every dependency graph--anyone could
      deterministically piece together the software by hand if they had to

`Mix` has other benefits but they are irrelevant for this repository.

To view customizable options, look at `dconf.h`. The top block is overridable.
Obviously, you may also change the source code to do what you want.

Idiomatic customization involves setting environment variables. This can be done
by hand before running `make` or by changing `env` in `local.nix` and then
performing any `Nix` based build.

Using `local.nix` provides a very intuitive iterative development cycle.

If you choose to use `Nix` to build it famously rejects the FHS standard.
`syspower` is dynamically linked so it needs to know where your linker is. To
run your build artifact on a different, generic Linux computer, you will need
to run:

`patchelf --set-interpreter "${DYNAMIC_LINKER_PATH}" "${BINARY}"`

where:

    - `DYNAMIC_LINKER_PATH` is normally `/lib64/ld-linux-x86-64.so.2`
    - `BINARY` is the filename of the compiled binary

## acknowledgements

This kind of research would not be possible without student and faculty
participation at the University of Louisville's J.B. Speed School of
Engineering.

This is a living list of individuals who have made direct, significant
contribution to this project from the Computer Systems Laboratory:
- Wendell Conrad Richmond
- Soren Lera
- Benjamin Carey
- Rinku Deuja
- Nihat Altiparmak

## todo

- decouple the engine so it can be written to back `syspower`, but also to power
  a separate but directly analogous kernel framework
- write device init logic based on generic ACPI dumps, DMI, DTB, or the Linux
  Device Model--instead of probing readily available info cleverly
- memory architecture is highly sensitive--develop dynamic models that account
  for channels, generation, iMCs, heterogeneous configs, etc.
- make closure output generate automatically instead of manually
- create a comprehensive architecture document
- `getopt` for better optional parameters
- docs `pandoc` migration for comprehensive visualization, documentation, and
  publication

---
> The intent is to migrate this document to `pandoc` and create static mappings
to READMEs, documents, websites, papers, etc. This can be fully specified as
part of the build process.
