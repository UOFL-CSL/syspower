# `sptop`: per-process power in `top`, by component

This guide shows how to add **power columns** to `top`. They show how many watts each process uses for each hardware component (CPU, RAM, disk) plus its total, along with a summary line of whole-machine power per component. The modified program is installed under its own name, **`sptop`** (syspower top), so Ubuntu's normal `top` stays untouched. It follows the steps it took to build it the first time and the problems that came up along the way, so you can rebuild it, change it, or move it to a new machine.

```
Watts : CPU 6.10, RAM 12.27, Disk 1.85, NIC -, Board 8.17, Fan -, GPU -,  Total 28.39

    PID USER  ...  %CPU  CPU_W  RAM_W  DSK_W  NIC_W  BRD_W  FAN_W  TOT_W  %MEM     TIME+ S COMMAND
 360798 sesi  ... 169.3  0.738  0.029  1.712      -      -      -  2.479   0.2   0:17.51 S fio ...
 360799 sesi  ...  15.8  0.069  0.026  0.138      -      -      -  0.233   0.2   0:01.94 S fio ...
```
(Example values to show the layout.)

**Where the code lives**

Everything is in this repo (syspower), branch `proportional`, under `tools/`:

| Path | What |
|---|---|
| [`tools/procps/`](../procps/) | `top`'s source code: [procps-ng](https://gitlab.com/procps-ng/procps) **v4.0.4** with the power changes, and syspower's code copied in under `src/top/syspower/` |
| [`tools/docs/`](.) | this guide |
| [`tools/fio_dual.sh`](../fio_dual.sh) | test workload: a heavy and a light fio job in parallel |
| `src/hw/hw.c` | syspower's blank-name bug fix (commit `917ac7c`), also in the copy |

`tools/procps/` is plain files tracked by syspower's git, not a separate repo. procps's own history isn't included; see the official GitLab for that. `tools/procps/.tarball-version` holds `4.0.4`, because without git history procps's build can't work out its version and would call itself "UNKNOWN".

## Quick start: build and run it

Install the packages from [section 2](#2-before-you-start) first. Then, from the syspower repo:

```bash
cd tools/procps
./autogen.sh                 # generates ./configure (only needed once)
./configure --disable-nls    # (only needed once)
make -j8 src/top/top
sudo ./src/top/top
```

Then continue with [section 8](#8-build-run-install-test) to install it as `sptop`, so `sudo sptop` works from anywhere.

**The rest of this guide explains every change made to `top`, and why.** It's written so you could redo them on a fresh copy of procps, e.g. to move to a newer version (sections 3–7).

---

## 1. How it works

### Step 1: measure each component (the `Watts` line)

Each time `top` refreshes (every 3 s by default), syspower's code measures each hardware component. Every value is divided by **0.92**, the assumed power-supply efficiency (`CFG_EFFICIENCY` in `dconf.h`):

| Component | How syspower gets it | |
|---|---|---|
| **CPU** | change in the CPU's energy counter (RAPL) ÷ time since last refresh | measured |
| **RAM** | Σ ( P_min + P_max × BW / BW_max ) over memory channels, where BW = memory traffic | estimated |
| **Disk** | Σ ( P_min + P_max × BW / BW_max ) over disks, where BW = bytes/s from disk statistics | estimated |
| **Board** (motherboard) | a fixed value (7.52 W): chipset, voltage regulators, USB/audio, … | fixed guess |
| **NIC**, **Fan** | network traffic and fan speed (PWM), when syspower finds supported hardware | estimated |
| **GPU** | NVIDIA's library. Stubbed out in `top` (see [§10](#10-limitations-and-ideas-for-next-steps)). | — |

A component the machine doesn't have, or that syspower doesn't recognize, shows `-`.

`BW_max` starts from the hardware's rated speed and goes up whenever higher traffic is seen, so the RAM and disk estimates settle down after the machine has been under load once.

### Step 2: split each component between processes (the columns)

Each component is split by the activity that drives it. For process *i*, over the time Δt since the last refresh:

```
CPU_W = ticks_i / (Hz × Δt × N_cores)        × P_CPU     share of all CPU time on the machine
RAM_W = RES_i / MemTotal                      × P_RAM     share of physical memory (same ratio as %MEM)
DSK_W = Δbytes_i / Σ Δbytes (all processes)   × P_Disk    share of disk bytes moved this refresh
TOT_W = CPU_W + RAM_W + DSK_W
NIC_W, BRD_W, FAN_W, GPU_W = "-"   (no per-process split yet; their totals are on the Watts line)
```
- `ticks_i`: CPU time the process used since the last refresh (Hz = 100 ticks per second per core)
- `RES_i`: physical memory the process holds
- `Δbytes_i`: (read_bytes + write_bytes) now, minus the same at the last refresh, from `/proc/<pid>/io`

**Example** on a 14-core laptop with P_CPU = 6 W: fio at 170% CPU gets 170 ÷ 1400 × 6 ≈ **0.73 W** of CPU power.

**How the splits differ:**

| | Divided by | Do all processes add up to 100%? |
|---|---|---|
| CPU | the whole machine's CPU capacity | no: idle CPU time is left over |
| RAM | total installed memory | no: free and kernel memory are left over (shared memory can be counted twice) |
| Disk | only what processes actually moved | **yes**, whenever any I/O happened (0 for everyone if nobody touched the disk) |

> **Watch out:** `top`'s `%CPU` is **% of one core** by default, so it can go up to 100 × (number of cores). The code converts it to a share of the whole machine. Press `Shift+I` in `top` to switch modes: `%CPU` changes, but `CPU_W` stays the same.

### Why syspower is *inside* `top`

An earlier version ran syspower as a separate program that wrote its total to a file, which `top` then read. Compiling syspower's measuring code directly into `top` is simpler:

- one program instead of two
- no file to keep in sync
- measurements happen exactly when `top` refreshes, so power and process activity cover the same time window

The trade-offs:
- It must run as root (`sudo sptop`) to read the power counters and other processes' disk statistics.
- The copied syspower code doesn't update itself when syspower changes (see [§10](#10-limitations-and-ideas-for-next-steps)).

### The pieces

```
top.c ── pwr_refresh() ──> sp_top.c (sp_init / sp_sample: watts per component)
     │                           │
     │                           └──> syspower's code (copied):
     │                                  grp/  find hardware, group it, grp_step()
     │                                  hw/   known parts and their power ranges
     │                                  mdl/  how to estimate each group's power
     ├── pwr_io_refresh() : disk bytes per process since last refresh
     ├── task_show()      : per-process columns
     └── summary_show()   : the Watts line
```

`top.c` only ever includes `sp_top.h`, a tiny header. syspower's own headers define lots of macros (`LIB_IO`, `CFG_*`, `NAME`, ...), and keeping them out of `top.c` avoids name clashes.

---

## 2. Before you start

**Packages:**
```bash
sudo apt install fio autoconf automake libtool autopoint gettext pkg-config \
                 libncurses-dev libhwloc-dev libudev-dev
```

| Package | Why |
|---|---|
| `autoconf automake libtool autopoint gettext pkg-config` | procps's build system (autotools) |
| `libncurses-dev` | `top`'s screen drawing |
| `libhwloc-dev` | syspower uses hwloc to find CPU sockets and the motherboard |
| `libudev-dev` | syspower uses udev to find memory, disks and network cards |
| `fio` | test workloads |

**Check your `top` version.** Match procps to the version your distro ships, so the modified `top` works with the system's `libproc2` library:
```bash
apt-cache policy procps     # Ubuntu 24.04: 2:4.0.4-...  -> use tag v4.0.4
```

---

## 3. Get `top`'s source and build it unmodified

> **Already done in this repo:** `tools/procps/` contains the finished result of sections 3–7. Follow these sections only to redo the work on a fresh copy of procps, e.g. a newer version.

`top` is **not** part of the Linux kernel. It's a normal program from the **procps-ng** package. Only `top` gets rebuilt; there's no kernel compile and no reboot.

How the procps code got into this repo, starting from the official release:

```bash
cd /tmp
git clone -b v4.0.4 https://gitlab.com/procps-ng/procps.git
cd ~/Documents/lab/syspower
mkdir -p tools/procps
git -C /tmp/procps archive --format=tar HEAD | tar -x -C tools/procps   # source files only, no .git
echo 4.0.4 > tools/procps/.tarball-version                              # version, since there's no git history

cd tools/procps
./autogen.sh                 # generates ./configure
./configure --disable-nls    # --disable-nls: skip translations (not needed)
make -j8 src/top/top         # build just top, plus the library it needs
./src/top/top                # run it from the build folder; q to quit
```
- **`git archive`** exports only the tracked source files. Copying a folder that still has its own `.git` inside would make syspower's git treat it as a separate embedded repo and skip its contents.
- The build creates hundreds of generated files (`configure`, `Makefile`, `*.o`, …). procps's own `.gitignore` (inside `tools/procps/`) keeps them out of commits.

Build the unmodified version first. If this fails, the problem is your setup, not your changes.

> `src/top/top` is a small **wrapper script** made by libtool, and the real program is `src/top/.libs/top`. Always run or install it through the wrapper or libtool, as shown below.

---

## 4. Copy syspower's measuring code into `top`

Copy syspower's `src/` folder into `procps/src/top/syspower/src/`, **keeping its folder layout**, because syspower's files include each other as `<src/...>`.

```bash
SRC=~/Documents/lab/syspower/src
DST=~/Documents/lab/syspower/tools/procps/src/top/syspower
mkdir -p $DST/src
cd $SRC
find dconf.h grp hw mdl ut spdx -type f \( -name "*.c" -o -name "*.h" \) \
     ! -path "mdl/nvml/nvml.c" |
while read f; do
	mkdir -p "$DST/src/$(dirname "$f")"
	cp "$f" "$DST/src/$f"
done
```

Left out on purpose:

| File | Why |
|---|---|
| `main.c` | `top` has its own `main()` |
| `*.mk`, `list.mk` | syspower's makefiles; procps uses its own build |
| `mdl/nvml/nvml.c` | the NVIDIA GPU model needs `nvml.h` from CUDA, which isn't on a normal system. It's replaced by a stub in step 5. |

> **Make sure the `hw.c` fix is in what you copy** (syspower commit `917ac7c`). Without it, any device with no model name (like many Intel WiFi cards) gets set up as the wrong part, and every measurement fails. See [Troubleshooting](#9-troubleshooting).

### Two small changes to the copied files

**`src/top/syspower/src/grp/grp.c`**: add a function that measures every group **once** and reports the watts per **model**. syspower's own `grp_poll()` is an endless print-and-sleep loop, which `top` can't use. Each group's model (`enum mdl_model` in `mdl/mdl.h`) says what kind of hardware it is, so `sp_top.c` can later sort the watts into components. Add this above `grp_strip()`:

```c
// top: measure every group once, like one pass of grp_poll() without
// printing. Results are added up per model (enum mdl_model), since the
// model says what kind of hardware a group is (cpu, dram, storage, ...):
//   uw[model] = microwatts (after power-supply efficiency)
//   n[model]  = number of groups measured with that model
// Both arrays must hold nmodels entries and start zeroed.
int grp_step(int64_t *uw, int *n, int nmodels)
{
	int err = 0;

	// step every group even if one fails, so each keeps its own timing
	for (uint32_t i = 0; i < grp_groups.len; i++) {
		struct grp_group *group = grp_groups.ptr[i];
		if (mdl_step(group)) {
			err = -1;
			continue;
		}
		if (group->model <= 0 || group->model >= nmodels)
			continue;
		uw[group->model] += group->jw.uw * 100 / CFG_EFFICIENCY;
		n[group->model]++;
	}

	return err;
}
```
and declare it in **`grp/grp.h`** next to `grp_strap`/`grp_strip`:
```c
int grp_step(int64_t *uw, int *n, int nmodels); // top: measure all groups once
```
- It has to live in `grp.c`, because the list of groups (`grp_groups`) is `static`, meaning private to that file.
- `* 100 / CFG_EFFICIENCY` is the same power-supply adjustment syspower applies to its own output: 92% efficient by default.
- `group->model` is only set after a group's first successful measurement, so a group that has never worked is skipped.

**`src/top/syspower/src/dconf.h`**: let the error-printing macro be replaced, so syspower's error messages don't get printed on top of `top`'s screen:
```c
// top: overridden in ../sp_config.h so errors don't scramble top's screen
#ifndef CFG_PRINTF_ERR
#define CFG_PRINTF_ERR(...) fprintf(stderr, __VA_ARGS__)
#endif
```

---

## 5. Write the connector files

All four go in `tools/procps/src/top/syspower/`.

**`sp_config.h`**: automatically included before every syspower file (see step 6):
```c
#ifndef SP_CONFIG_H__
#define SP_CONFIG_H__

// program name used in syspower's error messages
#define NAME "top-syspower"

// send syspower's error messages to a log file instead of the terminal,
// where they would scramble top's screen
void sp_log(const char *fmt, ...);
#define CFG_PRINTF_ERR(...) sp_log(__VA_ARGS__)

#endif
```
syspower's makefile normally passes `NAME` on the compiler command line, so we define it here instead.

**`sp_top.h`**: the only syspower header `top.c` sees:
```c
#ifndef SP_TOP_H__
#define SP_TOP_H__

// hardware components, in the order top shows them
enum sp_comp {
	SP_CPU,
	SP_RAM,
	SP_DISK,
	SP_NIC,
	SP_BOARD,
	SP_FAN,
	SP_GPU,
	SP_NCOMP // number of components
};

struct sp_power {
	float watts[SP_NCOMP]; // power per component, averaged since last call
	int present[SP_NCOMP]; // 1 if this machine has it and it was measured
	float total;           // sum of all components
};

// find the hardware once; 0 = ok, -1 = unavailable (e.g. not run as root)
int sp_init(void);

// measure once. 0 = ok, -1 = a measurement failed (see SP_LOG_PATH)
int sp_sample(struct sp_power *p);

#define SP_LOG_PATH "/var/tmp/top-syspower.log"

#endif
```

**`sp_top.c`**: measures, then sorts each syspower model's watts into a component:
```c
#include <src/dconf.h> // syspower's headers need this first
#include <src/grp/grp.h>
#include <src/mdl/mdl.h>
#include "sp_top.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// room for every syspower model number (enum mdl_model)
#define SP_MAX_MODELS 64

void sp_log(const char *fmt, ...)
{
	FILE *fp = fopen(SP_LOG_PATH, "a");
	va_list args;

	if (!fp)
		return;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	fclose(fp);
}

// which component each syspower model measures
static int sp_comp_of(int model)
{
	switch (model) {
	case SYSPWR_MDL_RAPL_CPU:
		return SP_CPU;
	case SYSPWR_MDL_GEN_DRAM_INTEL_UNCORE_FREE:
		return SP_RAM;
	case SYSPWR_MDL_GEN_STORAGE_SYSFS_BW:
		return SP_DISK;
	case SYSPWR_MDL_GEN_NIC_SYSFS_BW:
		return SP_NIC;
	case SYSPWR_MDL_GEN_MOTHERBOARD_FLAT:
		return SP_BOARD;
	case SYSPWR_MDL_GEN_FAN:
		return SP_FAN;
	case SYSPWR_MDL_NVML_GPU:
		return SP_GPU;
	default:
		return -1; // a new model: add it above
	}
}

int sp_init(void)
{
	// the CPU power counter and memory counters are root-only
	if (geteuid() != 0) {
		sp_log("top-syspower: not root, power columns disabled\n");
		return -1;
	}
	return grp_strap();
}

int sp_sample(struct sp_power *p)
{
	int64_t uw[SP_MAX_MODELS] = { 0 };
	int n[SP_MAX_MODELS] = { 0 };
	int err = grp_step(uw, n, SP_MAX_MODELS);

	memset(p, 0, sizeof(*p));
	for (int m = 0; m < SP_MAX_MODELS; m++) {
		int c = sp_comp_of(m);
		if (c < 0 || !n[m])
			continue;
		p->watts[c] += (float)uw[m] / 1000000.0f;
		p->present[c] = 1;
		p->total += (float)uw[m] / 1000000.0f;
	}

	// run as: sudo env TOP_SYSPOWER_DEBUG=1 top  -> logs every measurement
	if (getenv("TOP_SYSPOWER_DEBUG"))
		sp_log("top-syspower: cpu=%.3f ram=%.3f disk=%.3f nic=%.3f "
		       "board=%.3f fan=%.3f gpu=%.3f total=%.3f W err=%d\n",
		       p->watts[SP_CPU], p->watts[SP_RAM], p->watts[SP_DISK],
		       p->watts[SP_NIC], p->watts[SP_BOARD], p->watts[SP_FAN],
		       p->watts[SP_GPU], p->total, err);
	return err;
}
```
> **`#include <src/dconf.h>` must come first.** syspower's headers (e.g. `ut/gbuf.h`) use macros like `#include LIB_INT` that are defined in `dconf.h`. Without it, the build fails with `#include expects "FILENAME" or <FILENAME>`.

> **If syspower gets a new model** (a new `SYSPWR_MDL_...` in `mdl/mdl.h`), add it to `sp_comp_of()`, or its watts won't be counted anywhere.

**`nvml_stub.c`**: stands in for the NVIDIA GPU model:
```c
#include <src/dconf.h> // syspower's headers need this first
#include <src/grp/grp.h>
#include <src/mdl/nvml/nvml.h>

int mdl_nvml_gpu(struct grp_group *grp)
{
	(void)grp;
	return -1;
}
```
Returning `-1` means "this model can't measure". GPU groups are only created for NVIDIA parts listed in `hw/`, and none are listed yet, so this normally never runs.

**`.gitignore`**: one line, `libsyspower.la`, so the build output doesn't get committed.

---

## 6. Build syspower's code as a library linked into `top`

In **`tools/procps/Makefile.am`**, right after the `src_top_top_LDADD = ...` line:

```make
# syspower's power-measuring code, built as a small library linked into top
# (feeds the PWR column; see src/top/syspower/README.md)
noinst_LTLIBRARIES = src/top/syspower/libsyspower.la
src_top_syspower_libsyspower_la_SOURCES = \
	src/top/syspower/sp_top.c \
	src/top/syspower/nvml_stub.c \
	src/top/syspower/src/grp/grp.c \
	src/top/syspower/src/hw/hw.c \
	src/top/syspower/src/hw/gen/cpu/cpu.c \
	src/top/syspower/src/hw/gen/dram/dram.c \
	src/top/syspower/src/hw/gen/fan/fan.c \
	src/top/syspower/src/hw/gen/motherboard/motherboard.c \
	src/top/syspower/src/hw/gen/nic/nic.c \
	src/top/syspower/src/hw/gen/storage/storage.c \
	src/top/syspower/src/hw/mpn/dell/0H74DC/0H74DC.c \
	src/top/syspower/src/hw/mpn/dell/0N0992/0N0992.c \
	src/top/syspower/src/hw/mpn/intel/BX8071512700/BX8071512700.c \
	src/top/syspower/src/hw/mpn/intel/WGI219LM/WGI219LM.c \
	src/top/syspower/src/hw/mpn/micron/MTC4C10163S1UC48BA1/MTC4C10163S1UC48BA1.c \
	src/top/syspower/src/hw/mpn/samsung/MZ-V9P2T0GW/MZ-V9P2T0GW.c \
	src/top/syspower/src/hw/mpn/sk_hynix/HFS001TEJ9X101N/HFS001TEJ9X101N.c \
	src/top/syspower/src/mdl/mdl.c \
	src/top/syspower/src/mdl/rapl/rapl.c \
	src/top/syspower/src/mdl/gen/dram/uncore_free.c \
	src/top/syspower/src/mdl/gen/fan/fan.c \
	src/top/syspower/src/mdl/gen/motherboard/flat.c \
	src/top/syspower/src/mdl/gen/nic/sysfs_bw.c \
	src/top/syspower/src/mdl/gen/storage/sysfs_bw.c
src_top_syspower_libsyspower_la_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/top/syspower \
	-include $(top_srcdir)/src/top/syspower/sp_config.h
src_top_top_LDADD += src/top/syspower/libsyspower.la -lhwloc -ludev
```

- **Why a separate library:** syspower's files get their own compiler flags (`CPPFLAGS`) without those flags touching `top.c`.
- **`-I.../syspower`** makes `#include <src/grp/grp.h>` find the copied files.
- **`-include sp_config.h`** acts as if every syspower file started with `#include "sp_config.h"`.
- **`-lhwloc -ludev`** link the two libraries syspower needs.
- **If you add a part to `hw/mpn/`** (a new CPU, SSD, ...), add its `.c` file to this list too.

`make` notices that `Makefile.am` changed and re-runs automake by itself.

---

## 7. Changes to `top` itself

All in `tools/procps/src/top/`. Line numbers are approximate (v4.0.4).

> **`top.h` is saved in the old ISO-8859 text encoding, not UTF-8.** Some editors quietly convert it when saving, which corrupts a few special characters in it. If your editor asks about the encoding, keep ISO-8859-1. Or make the `top.h` edits with `sed` or a short Python script.

### 7a. Register the new columns: `top.h`, `top.c`, `top_nls.c`

`top` gives every column an ID number from a list called `enum pflag`. A comment there warns that it **must stay in step with the `Fieldstab[]` table** in `top.c`.

**`top.h`** (~line 229): add eight IDs after the last real column:
```c
   EU_NS7, EU_NS8,
   EU_PWR, EU_PWC, EU_PWM, EU_PWD, EU_PWN, EU_PWB, EU_PWF, EU_PWG,
```
(`PWR` = total, then CPU, Memory, Disk, NIC, Board, Fan, GPU.)

**`top.c`** (~line 2004): add matching table rows, in the **same order**, and move `eu_LAST`:
```c
   {    10,     -1,  A_right,  PIDS_NS_TIME        },  // ul_int   EU_NS8
   {     6,     -1,  A_right,  PIDS_TICS_ALL_DELTA },  // u_int    EU_PWR  (total)
   {     6,     -1,  A_right,  PIDS_TICS_ALL_DELTA },  // u_int    EU_PWC  (cpu)
   {     6,     -1,  A_right,  PIDS_MEM_RES        },  // ul_int   EU_PWM  (ram)
   {     6,     -1,  A_right,  PIDS_ID_PID         },  // s_int    EU_PWD  (disk)
   {     6,     -1,  A_right,  PIDS_ID_PID         },  // s_int    EU_PWN  (nic, placeholder)
   {     6,     -1,  A_right,  PIDS_ID_PID         },  // s_int    EU_PWB  (board, placeholder)
   {     6,     -1,  A_right,  PIDS_ID_PID         },  // s_int    EU_PWF  (fan, placeholder)
   {     6,     -1,  A_right,  PIDS_ID_PID         }   // s_int    EU_PWG  (gpu, placeholder)
#define eu_LAST        EU_PWG
```
- Don't forget the new comma at the end of the `EU_NS8` line.
- `6` is the column width, enough for `12.345`.
- The last field is the **data item** `top`'s library fetches for that column, and it's also what the column sorts by:
  - `PIDS_TICS_ALL_DELTA` (CPU time since last refresh) is the same data `%CPU` uses. Two columns sharing one item is normal (`TIME` and `TIME+` do it too).
  - `PIDS_MEM_RES` is the same data `RES` and `%MEM` use.
  - Disk and the placeholders use `PIDS_ID_PID`. Disk deltas are calculated separately (7d), and the PID is harmless to fetch or sort by.

**`top_nls.c`** (~line 389): column titles, and their descriptions in the `f` (fields) menu:
```c
   Head_nlstab[EU_PWR] = _("TOT_W");
   Desc_nlstab[EU_PWR] = _("Power W, total: CPU+RAM+DSK");
   Head_nlstab[EU_PWC] = _("CPU_W");
   Desc_nlstab[EU_PWC] = _("Power W, CPU (by cpu time)");
   Head_nlstab[EU_PWM] = _("RAM_W");
   Desc_nlstab[EU_PWM] = _("Power W, RAM (by RES)");
   Head_nlstab[EU_PWD] = _("DSK_W");
   Desc_nlstab[EU_PWD] = _("Power W, disk (by bytes r+w)");
   Head_nlstab[EU_PWN] = _("NIC_W");
   Desc_nlstab[EU_PWN] = _("Power W, network (not yet)");
   Head_nlstab[EU_PWB] = _("BRD_W");
   Desc_nlstab[EU_PWB] = _("Power W, motherboard (not yet)");
   Head_nlstab[EU_PWF] = _("FAN_W");
   Desc_nlstab[EU_PWF] = _("Power W, fans (not yet)");
   Head_nlstab[EU_PWG] = _("GPU_W");
   Desc_nlstab[EU_PWG] = _("Power W, GPU (not yet)");
```

### 7b. Show the columns by default, right after %CPU: `top.h`

`DEF_FIELDS` (~line 567, the `#else` copy, *not* the `ORIG_TOPDEFS` one) lists every column as a number:

```
number = (column ID + 37) × 2 + 1   if shown
number = (column ID + 37) × 2       if hidden
```
(37 is the character code of `%`, the `FLD_OFFSET` in `top.h`.)

| Column | ID | Shown | Hidden |
|---|---|---|---|
| `%CPU` (EU_CPU) | 18 | 111 | 110 |
| `TOT_W` (EU_PWR) | 78 | 231 | 230 |
| `CPU_W` (EU_PWC) | 79 | 233 | 232 |
| `RAM_W` (EU_PWM) | 80 | 235 | 234 |
| `DSK_W` (EU_PWD) | 81 | 237 | 236 |
| `NIC_W` (EU_PWN) | 82 | 239 | 238 |
| `BRD_W` (EU_PWB) | 83 | 241 | 240 |
| `FAN_W` (EU_PWF) | 84 | 243 | 242 |
| `GPU_W` (EU_PWG) | 85 | 245 | 244 |

The edits:
- On the first line, right after `111`, insert `233, 235, 237, 239, 241, 243, 231,` (the parts, then the total).
- Further down, **delete** `230, 232, 234, 236, 238, 240, 242` (their "hidden" versions).
- Leave `244`: GPU stays hidden by default (turn it on with `f`).

The list must keep **exactly 100 numbers**, with each column appearing once, or `top` rejects its own defaults.

### 7c. Start as a flat list, not a tree: `top.h`

Upstream procps starts `top` in **tree view** (`Show_FOREST` in `DEF_WINFLGS`, ~line 305). Ubuntu's `top` doesn't. Remove `Show_FOREST` from that line:
```c
#define DEF_WINFLGS ( View_LOADAV | View_STATES | View_MEMORY | Show_CMDLIN \
   | Show_COLORS | Show_HIROWS | Show_IDLEPS | Show_JRNUMS | Show_TASKON \
   | Qsrt_NORMAL )
```
**Why this matters:** in tree view, a busy process like fio sits deep under `systemd → … → your terminal → bash`. It ends up far down the list below idle system processes. In batch mode (`-b`), the tree indentation even pushes its name off the end of the line. You can still press `V` to turn tree view on.

### 7d. Measure power and disk bytes each refresh: `top.c`

Near the top, with the other includes (~line 61):
```c
#include "syspower/sp_top.h"
```

Before `tasks_refresh()` (~line 2800), add the power state, the disk-byte tracking, and one helper per component:
```c
        /*
         * Power columns: syspower's measuring code (src/top/syspower) runs
         * inside top. Each refresh it measures watts per component (cpu,
         * ram, disk, ...). Each task then gets a share of each component:
         *    cpu  - by its share of all cpu time on the machine
         *    ram  - by its share of all physical memory (RES, like %MEM)
         *    disk - by its share of all bytes read + written this refresh
         * nic, board, fan and gpu have no per-task split yet ('-'). */
static struct sp_power Pwr;           // watts per component since last frame
static int Pwr_ready = -1;            // -1 = not tried yet, 0 = no, 1 = yes
static int Pwr_ok;                    // 1 when this frame's measurement worked

static void pwr_refresh (void) {
   if (Pwr_ready < 0)
      Pwr_ready = (sp_init() == 0);
   Pwr_ok = Pwr_ready && sp_sample(&Pwr) == 0;
} // end: pwr_refresh


        /*
         * Disk bytes per task. The library only gives running totals since
         * each task started, so we remember last frame's total per pid
         * (a small hash table) to get the bytes moved during this frame. */
#define PWR_IO_CAP  65536             // max tasks tracked (power of 2)
struct pwr_io { int pid; unsigned long long bytes, delta; };
static struct pwr_io *Pwr_io_cur, *Pwr_io_prv;
static unsigned long long Pwr_io_total; // all tasks' bytes this frame

static unsigned pwr_io_slot (const struct pwr_io *t, int pid) {
   unsigned h = ((unsigned)pid * 2654435761u) & (PWR_IO_CAP - 1);

   while (t[h].pid && t[h].pid != pid)
      h = (h + 1) & (PWR_IO_CAP - 1);
   return h;
} // end: pwr_io_slot

static void pwr_io_refresh (struct pids_fetch *reap) {
   struct pwr_io *tmp;
   int i;

   Pwr_io_total = 0;
   // only when a disk or total power column asked for the io items
   if (Pids_itms[EU_IRB] != PIDS_IO_READ_BYTES
   || Pids_itms[EU_IWB] != PIDS_IO_WRITE_BYTES)
      return;
   if (!Pwr_io_cur) {
      Pwr_io_cur = alloc_c(PWR_IO_CAP * sizeof(struct pwr_io));
      Pwr_io_prv = alloc_c(PWR_IO_CAP * sizeof(struct pwr_io));
   }
   // last frame's table becomes 'previous', then refill 'current'
   tmp = Pwr_io_prv; Pwr_io_prv = Pwr_io_cur; Pwr_io_cur = tmp;
   memset(Pwr_io_cur, 0, PWR_IO_CAP * sizeof(struct pwr_io));
   if (reap->counts->total > PWR_IO_CAP / 2)
      return;                         // too many tasks, table would overflow

   for (i = 0; i < reap->counts->total; i++) {
      struct pids_stack *p = reap->stacks[i];
      int pid = PID_VAL(EU_PID, s_int, p);
      unsigned long long bytes = PID_VAL(EU_IRB, ul_int, p) + PID_VAL(EU_IWB, ul_int, p);
      unsigned c = pwr_io_slot(Pwr_io_cur, pid);
      unsigned o = pwr_io_slot(Pwr_io_prv, pid);

      if (pid <= 0) continue;
      Pwr_io_cur[c].pid = pid;
      Pwr_io_cur[c].bytes = bytes;
      // a new task (or a reused pid) has no previous total, so no delta
      if (Pwr_io_prv[o].pid == pid && bytes >= Pwr_io_prv[o].bytes)
         Pwr_io_cur[c].delta = bytes - Pwr_io_prv[o].bytes;
      Pwr_io_total += Pwr_io_cur[c].delta;
   }
} // end: pwr_io_refresh

static unsigned long long pwr_io_delta (int pid) {
   unsigned c;

   if (!Pwr_io_cur) return 0;
   c = pwr_io_slot(Pwr_io_cur, pid);
   return Pwr_io_cur[c].pid == pid ? Pwr_io_cur[c].delta : 0;
} // end: pwr_io_delta


        /*
         * One task's watts for each component it can be tied to. */
static float pwr_task_cpu (unsigned ticks) {
   float pct = (float)ticks * Frame_etscale;
   // irix mode is % of one cpu, so make it % of the whole machine
   if (Rc.mode_irixps)
      pct /= (float)Cpu_cnt;
   return pct / 100.0f * Pwr.watts[SP_CPU];
} // end: pwr_task_cpu

static float pwr_task_ram (unsigned long res_kib) {
   unsigned long tot = MEM_VAL(mem_TOT);
   return tot ? (float)res_kib / (float)tot * Pwr.watts[SP_RAM] : 0.0f;
} // end: pwr_task_ram

static float pwr_task_disk (int pid) {
   if (!Pwr_io_total) return 0.0f;
   return (float)pwr_io_delta(pid) / (float)Pwr_io_total * Pwr.watts[SP_DISK];
} // end: pwr_task_disk
```
About the disk table:
- **Why a hash table:** PIDs can go up to about 4 million, so an array indexed by PID would be huge. The table maps PID → last total in a fixed 65,536 slots.
- **Two tables, swapped each refresh:** "current" is rebuilt from scratch, so processes that exited disappear on their own.

Inside `tasks_refresh()`, measure power right after the line that sets `Frame_etscale` (~line 2841):
```c
      Frame_etscale = 100.0f / ((float)Hertz * (float)et * (Rc.mode_irixps ? 1 : Cpu_cnt));
      pwr_refresh();
```
This is where `top` works out the time since the last refresh, so power and activity cover the same window.

Then track disk bytes right after `top` collects the process list (~line 2860):
```c
      if (!Pids_reap)
         error_exit(fmtmk(N_fmt(LIB_errorpid_fmt), __LINE__, strerror(errno)));
      pwr_io_refresh(Pids_reap);
```

### 7e. Ask the library for the extra data: `top.c`, `build_headers()`

The library only fetches data for columns that are on screen. The disk and total columns need every process's read and write bytes, and the total also needs RES. In the `switch (f)` inside `build_headers()` (~line 2207), after `case EU_CPU:`, add:
```c
               case EU_PWR:
               // total power adds cpu (own item), ram and disk shares
                  ckITEM(EU_MEM);
               // fall through
               case EU_PWD:
               // disk power needs each task's bytes read + written
                  ckITEM(EU_IRB);
                  ckITEM(EU_IWB);
                  break;
```
`ckITEM(X)` means "also fetch the data that column X uses". `EU_IRB`/`EU_IWB` are `top`'s existing read/write-bytes columns, and `EU_MEM` is `%MEM` (RES).

### 7f. Display each process's watts: `top.c`, `task_show()`

Add these cases just before `case EU_CPU:` (~line 7045):
```c
         case EU_PWC:        // PIDS_TICS_ALL_DELTA -> cpu watts
         case EU_PWM:        // PIDS_MEM_RES        -> ram watts
         case EU_PWD:        // bytes via pwr_io    -> disk watts
         case EU_PWR:        // PIDS_TICS_ALL_DELTA -> cpu + ram + disk
         {  float w = 0.0f;
            if (Restrict_some || !Pwr_ok) {
               cp = justify_pad("-", W, Jn);
               break;
            }
            if (i == EU_PWC || i == EU_PWR)
               w += pwr_task_cpu(rSv(i, u_int));
            if (i == EU_PWM)
               w += pwr_task_ram(rSv(EU_PWM, ul_int));
            if (i == EU_PWR)
               w += pwr_task_ram(rSv(EU_MEM, ul_int));
            if (i == EU_PWD || i == EU_PWR)
               w += pwr_task_disk(rSv(EU_PID, s_int));
            // last arg 1 = up to 3 decimals, since most tasks use well under 1 W
            cp = scale_pcnt(w, W, Jn, 1);
         }
            break;
         case EU_PWN:        // nic   \  no way yet to tie these to one
         case EU_PWB:        // board  > task, so placeholders for now;
         case EU_PWF:        // fan   |  their totals are in the summary
         case EU_PWG:        // gpu   /  'Watts' line at the top
            cp = justify_pad("-", W, Jn);
            break;
```
- `i` is the column being drawn, and `rSv(X, type)` reads this process's value for column X's data item.
- All columns show `-` when not running as root, or when a measurement failed.

### 7g. The `Watts` summary line: `top.c`, `summary_show()`

Add this function just before `summary_show()` (~line 6810):
```c
        /*
         * The 'Watts' summary line: whole-machine power per component,
         * '-' for components this machine doesn't have (or couldn't measure). */
static const char *pwr_summary_line (void) {
   static const char *names[SP_NCOMP] = { "CPU", "RAM", "Disk", "NIC", "Board", "Fan", "GPU" };
   static char buf[SMLBUFSIZ];
   int c, n;

   if (!Pwr_ready)
      return fmtmk("Watts : unavailable, run as root (sudo %s)\n", Myname);
   if (!Pwr_ok)
      return "Watts : measurement failed, see " SP_LOG_PATH "\n";

   n = snprintf(buf, sizeof(buf), "Watts :");
   for (c = 0; c < SP_NCOMP && n < (int)sizeof(buf); c++) {
      if (Pwr.present[c])
         n += snprintf(buf + n, sizeof(buf) - n, " %s %.2f,", names[c], Pwr.watts[c]);
      else
         n += snprintf(buf + n, sizeof(buf) - n, " %s -,", names[c]);
   }
   if (n < (int)sizeof(buf))
      snprintf(buf + n, sizeof(buf) - n, "  Total %.2f\n", Pwr.total);
   return buf;
} // end: pwr_summary_line
```
And at the end of `summary_show()`, after the memory lines and before `#undef isROOM`:
```c
   // Display whole-machine power per component (syspower)
   if (Msg_row + 1 < SCREEN_ROWS - 1) {
      show_special(0, pwr_summary_line());
      Msg_row += 1;
   }
```
- `show_special()` is how `top` prints its summary lines. Each line must end in `\n`.
- `Msg_row` counts summary lines, so the process list starts one row lower.

---

## 8. Build, run, install, test

### Build and run from the build folder
```bash
cd ~/Documents/lab/syspower/tools/procps
make -j8 src/top/top
./src/top/top              # not root -> power columns "-", Watts line says run as root
sudo ./src/top/top         # root     -> watts after ~2 refreshes
```
The first one or two refreshes show `0.000`. The CPU energy counter needs two readings before it can calculate a rate, and disk bytes need a previous total to compare against.

### Install as `sptop`, a command that works from anywhere
```bash
sudo ./libtool --mode=install install -m 755 src/top/top /usr/local/bin/sptop
hash -r                    # make bash re-check where commands live
which sptop                # -> /usr/local/bin/sptop
sudo sptop
```
- `/usr/local/bin` is in the command search path (`PATH`), including `sudo`'s, so `sptop` works from any folder.
- The build still produces a program called `top`. Only the **installed copy** is named `sptop`.
- `top` takes its name from how it was started, so the header line, the root message and its **settings file** all say `sptop` automatically. Settings saved with `W` go to `~/.config/procps/sptoprc`, separate from normal `top`'s `toprc`.
- **Plain `top` is still Ubuntu's original** (`/usr/bin/top`), so you can compare the two side by side.
- **After every code change**, rebuild and repeat the install command. The installed copy doesn't update itself.
- Test with `sudo ./src/top/top` first and install only once it works, so a half-finished change never replaces the `sptop` others use.
- `sptop` was chosen because it isn't taken by any Ubuntu package. (`powertop` is Intel's PowerTOP, and `ptop` is an existing package.)

Undo:
```bash
sudo rm /usr/local/bin/sptop && hash -r
```

### Test with fio
In the syspower repo:
```bash
tools/fio_dual.sh 60       # 60 s of a heavy and a light fio job, in parallel
```
In another terminal run `sudo sptop`. Both `fio` lines should rise to the top of the list (sorted by %CPU):

| Job | %CPU (14-core laptop) | What to expect |
|---|---|---|
| heavy: 16 workers, 4k writes, io_uring, queue depth 64 | ~150–190 | largest `CPU_W` and by far the largest `DSK_W` |
| light: 1 worker, 1M reads, queue depth 4 | ~15–25 | small `CPU_W`, smaller `DSK_W` |

**Checks:**
- The **`CPU_W` ratio should match the `%CPU` ratio** (e.g. 170 ÷ 15 ≈ 11).
- The **`DSK_W` ratio should match the ratio of bytes moved** (compare `fio_dual1.out` and `fio_dual2.out`).
- `TOT_W` = `CPU_W + RAM_W + DSK_W` on every row.
- On the Watts line, Total = the sum of the parts.
- `RAM_W` is non-zero for every process holding memory, idle ones included, because it follows memory held, not activity.

The script uses fio's `thread` option, so each job is **one process** no matter how many workers it has.

To see the measured watts behind each refresh:
```bash
sudo env TOP_SYSPOWER_DEBUG=1 top -b -n 5 -d 3 | grep -E "Watts|PID|fio"
sudo cat /var/tmp/top-syspower.log      # cpu=.. ram=.. disk=.. ... total=.. W err=0 per refresh
```

---

## 9. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Power columns always `-`, Watts line says "run as root" | Not running as root | `sudo sptop`. The log says `not root, power columns disabled`. |
| Columns `-` as root, Watts line says "measurement failed" | A syspower measurement failed | Check `/var/tmp/top-syspower.log`, or run with `TOP_SYSPOWER_DEBUG=1` |
| Watts line shows `-` for NIC or Fan | syspower found no supported hardware. **NIC:** only network cards listed in `hw/mpn/` get a group (there's no generic fallback), so WiFi is skipped. **Fan:** `CFG_HWMONS_FANS` in `dconf.h` points at the lab machine's sensor (`hwmon6`), and many laptops don't expose fans to Linux at all. | Add the card under `hw/mpn/` (or a generic NIC fallback). Set `CFG_HWMONS_FANS` to the sensor whose folder has `pwm*` files. |
| `DSK_W` is 0 for everything | Nobody read or wrote the disk this refresh, or the first refresh | Run a disk load (`tools/fio_dual.sh`) and wait ~6 s |
| Log file stays empty under `sudo` | The log was first created by your normal user. Ubuntu's `fs.protected_regular=2` stops even root from writing to another user's file in shared folders like `/var/tmp`. | `rm /var/tmp/top-syspower.log` and let root create it |
| Busy process not visible / shows 0 | Tree view: it's deep in the tree and off screen | Press `V` for the flat list, then `Shift+P` to sort by CPU. The default should already be flat (step 7c). |
| Rows cut off on the right | The power columns add ~50 characters | Widen the terminal, or press `f` to hide columns |
| `failed to run model(s) for a group!` and the program stops (standalone syspower) | A device with **no model name** (e.g. Intel WiFi) matched *every* known part in `hw_match()` and ended up as a Samsung SSD | `hw.c` fix (syspower commit `917ac7c`): `if (size == 0) return false;` |
| Standalone syspower: `libnvidia-ml.so.1: cannot open shared object file` | No NVIDIA driver on the machine | `sudo env LD_LIBRARY_PATH=$(ls -d /nix/store/*cuda_nvml_dev*-stubs/lib/stubs \| head -1) ./result/bin/syspower` (not needed for `top`, which uses `nvml_stub.c`) |
| Build: `(*__errno_location ()).h: No such file` | `#include LIB_ERRNO` after `<errno.h>` is already included: the word `errno` inside the macro gets replaced too | Write `#include <errno.h>` directly |
| Build: `#include expects "FILENAME" or <FILENAME>` | A file included a syspower header before `src/dconf.h` | Include `<src/dconf.h>` first |
| Normal `top` complains about its settings | An older install put the modified program at `/usr/local/bin/top`, and `W` saved its settings into normal `top`'s `toprc` | `sudo rm /usr/local/bin/top`, then delete `~/.config/procps/toprc`. Installed as `sptop`, it uses its own `sptoprc`. |
| `sptop: command not found` | Not installed yet, or the terminal hasn't re-checked its command list | Install it (section 8), then `hash -r` or open a new terminal. `type sptop` should say `/usr/local/bin/sptop`. |
| `sudo cmd &` hangs forever | A background `sudo` can't ask for your password, so it waits (shows as state `T`) | Run `sudo -v` first. Remove stuck ones with `sudo kill -9 <pid>`. |

---

## 10. Limitations and ideas for next steps

- **NIC, board, fan and GPU have no per-process split yet.** They show `-` per process and only appear on the Watts line. Ideas:
  - **NIC:** per-process network bytes (Linux doesn't track these in `/proc`; it needs eBPF or per-connection accounting)
  - **Board and fan:** split by each process's share of total power, or evenly
- **RAM is split by memory held (RES), not memory traffic.** DRAM power mostly follows traffic, which is hard to measure per process without hardware performance counters. An idle process holding lots of memory gets a large `RAM_W`.
- **Disk power is split by activity, idle part included.** When nobody uses the disk, its idle power isn't given to anyone. When one process does all the I/O, it also gets the disk's idle power.
- **The totals depend on how well syspower knows the hardware.** Laptops get desktop defaults from `dconf.h`. On the dev laptop:
  - 8 LPDDR5 chips × the desktop memory range made RAM far too high (~12 W), and it jumped between refreshes.
  - The fixed 7.52 W board value was chosen for the lab's desktop motherboard.

  Adding the machine's parts under `hw/mpn/` fixes this. On **lamport** (the hardware syspower was written for) it should be accurate.
- **Possible bug in syspower's RAM and disk formula.** The code uses `P_min + P_max × BW/BW_max`, which gives `P_min + P_max` at full load, slightly more than the maximum. syspower's README describes `P_min + (BW/BW_max) × (P_max − P_min)`, which tops out at `P_max`. Worth checking with Wendell.
- **NVIDIA GPU power is stubbed out.** To support it, use the real `mdl/nvml/nvml.c`, add CUDA's `nvml.h` include path, and link `-lnvidia-ml`.
- **The copy doesn't follow syspower.** When syspower changes:
  - re-run the copy from step 4
  - re-apply the edits in `grp.c`/`grp.h` and `dconf.h`
  - check `sp_comp_of()` covers any new models

  One improvement would be to add `grp_step()` and the `#ifndef CFG_PRINTF_ERR` to syspower itself, so the copy needs no edits.
