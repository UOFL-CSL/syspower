# Per-process power in `top` (the PWR column)

This guide shows how to add a **PWR** column to `top`. It shows how many watts each process is using, estimated by splitting syspower's whole-system power by each process's share of CPU time. It follows the steps it took to build it the first time and the problems that came up along the way, so you can rebuild it, change it, or move it to a new machine.

```
    PID USER      PR  NI    VIRT    RES  %CPU    PWR  %MEM     TIME+ S COMMAND
 360798 sesith01  20   0 1446.3m  35.4m 169.3  3.012   0.2   0:17.51 S fio ...
 360799 sesith01  20   0  384.5m  32.6m  15.8  0.251   0.2   0:01.94 S fio ...
```

**Where the code lives**

| What | Where |
|---|---|
| Modified `top`, with syspower's code copied in under `src/top/syspower/` | [`top-power-column.patch`](top-power-column.patch), next to this guide. It's a patch against [procps-ng](https://gitlab.com/procps-ng/procps) tag `v4.0.4`. |
| `hw.c` bug fix, `tools/fio_dual.sh` test workload, this guide | this repo (syspower), branch `proportional` |

## Quick start: use the finished code

If you just want the working PWR column, apply the patch instead of making every change in sections 4–7 by hand. Install the packages from [section 2](#2-before-you-start) first.

```bash
cd ~/Documents/lab
git clone -b v4.0.4 https://gitlab.com/procps-ng/procps.git
cd procps
git switch -c power-column
git am ~/Documents/lab/syspower/docs/top-power-column.patch   # adds the PWR commit

./autogen.sh
./configure --disable-nls
make -j8 src/top/top
sudo ./src/top/top
```

`git am` applies the patch as a real commit, keeping the author and message. Then continue with [section 8](#8-build-run-install-test) to install and test it.

**After you change `top`**, regenerate the patch so the repo stays current. From the procps folder:
```bash
git format-patch v4.0.4..HEAD --stdout > ~/Documents/lab/syspower/docs/top-power-column.patch
```
This puts **all** your commits since v4.0.4 into the one file. Commit the updated patch to syspower.

The rest of this guide explains what the patch changes and why.

---

## 1. How it works

### The formula

Each time `top` refreshes (every 3 s by default):

1. syspower's code measures **whole-system power** in watts: CPU, memory, storage, motherboard and so on, added together.
2. For every process, `top` already knows how much CPU time it used since the last refresh.
3. Each process gets the matching fraction of the total:

```
share of machine = process CPU time ÷ (CPU time available on ALL cores)
PWR (watts)      = share of machine × whole-system watts
```

**Example** on a 14-core laptop drawing 25 W: fio at 170% CPU gets 170 ÷ 1400 × 25 ≈ **3 W**.

The time when CPUs sit idle isn't given to any process, so PWR summed over all processes is less than the total.

> **Watch out:** `top`'s `%CPU` is **% of one core** by default, so it can go up to 100 × (number of cores). The code converts it to a share of the whole machine. Press `Shift+I` in `top` to switch modes: `%CPU` changes, but PWR stays the same.

### Why syspower is *inside* `top`

An earlier version ran syspower as a separate program that wrote its total to a file, which `top` then read. Compiling syspower's measuring code directly into `top` is simpler:

- one program instead of two
- no file to keep in sync
- measurements happen exactly when `top` refreshes, so the power and CPU numbers cover the same time window

The trade-offs:
- `top` must run as root (`sudo top`) to read the power counters.
- The copied syspower code doesn't update itself when syspower changes (see [§10](#10-limitations-and-ideas-for-next-steps)).

### The pieces

```
top.c ── pwr_refresh() ──> sp_top.c (sp_init / sp_sample)
                                 │
                                 └──> syspower's code (copied):
                                        grp/  find hardware, group it, grp_step()
                                        hw/   known parts and their power ranges
                                        mdl/  how to estimate each group's power
```

`top.c` only ever includes `sp_top.h`, a tiny header with two functions. syspower's own headers define lots of macros (`LIB_IO`, `CFG_*`, `NAME`, ...), and keeping them out of `top.c` avoids name clashes.

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

`top` is **not** part of the Linux kernel. It's a normal program from the **procps-ng** package. Only `top` gets rebuilt; there's no kernel compile and no reboot.

```bash
cd ~/Documents/lab
git clone -b v4.0.4 https://gitlab.com/procps-ng/procps.git
cd procps
git switch -c power-column

./autogen.sh                 # generates ./configure
./configure --disable-nls    # --disable-nls: skip translations (not needed)
make -j8 src/top/top         # build just top, plus the library it needs
./src/top/top                # run it from the build folder; q to quit
```

Build the unmodified version first. If this fails, the problem is your setup, not your changes.

> `src/top/top` is a small **wrapper script** made by libtool, and the real program is `src/top/.libs/top`. Always run or install it through the wrapper or libtool, as shown below.

---

## 4. Copy syspower's measuring code into `top`

Copy syspower's `src/` folder into `procps/src/top/syspower/src/`, **keeping its folder layout**, because syspower's files include each other as `<src/...>`.

```bash
SRC=~/Documents/lab/syspower/src
DST=~/Documents/lab/procps/src/top/syspower
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

**`src/top/syspower/src/grp/grp.c`**: add a function that measures every group **once**. syspower's own `grp_poll()` is an endless print-and-sleep loop, which `top` can't use. Add this above `grp_strip()`:

```c
// top: measure every group once, like one pass of grp_poll() without
// printing. *uw = whole-system microwatts (after power-supply efficiency)
int grp_step(int64_t *uw)
{
	int err = 0;
	int64_t total = 0;

	// step every group even if one fails, so each keeps its own timing
	for (uint32_t i = 0; i < grp_groups.len; i++) {
		struct grp_group *group = grp_groups.ptr[i];
		if (mdl_step(group)) {
			err = -1;
			continue;
		}
		total += group->jw.uw * 100 / CFG_EFFICIENCY;
	}

	*uw = total;
	return err;
}
```
and declare it in **`grp/grp.h`** next to `grp_strap`/`grp_strip`:
```c
int grp_step(int64_t *uw); // top: measure all groups once
```
- It has to live in `grp.c`, because the list of groups (`grp_groups`) is `static`, meaning private to that file.
- `* 100 / CFG_EFFICIENCY` is the same power-supply adjustment syspower applies to its own output: 92% efficient by default.

**`src/top/syspower/src/dconf.h`**: let the error-printing macro be replaced, so syspower's error messages don't get printed on top of `top`'s screen:
```c
// top: overridden in ../sp_config.h so errors don't scramble top's screen
#ifndef CFG_PRINTF_ERR
#define CFG_PRINTF_ERR(...) fprintf(stderr, __VA_ARGS__)
#endif
```

---

## 5. Write the connector files

All four go in `procps/src/top/syspower/`.

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

// find the hardware once; 0 = ok, -1 = unavailable (e.g. not run as root)
int sp_init(void);

// measure once; sets *watts to whole-system power averaged since the
// previous call. 0 = ok, -1 = a measurement failed (see SP_LOG_PATH)
int sp_sample(float *watts);

#define SP_LOG_PATH "/var/tmp/top-syspower.log"

#endif
```

**`sp_top.c`**:
```c
#include <src/dconf.h> // syspower's headers need this first
#include <src/grp/grp.h>
#include "sp_top.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int sp_init(void)
{
	// the CPU power counter and memory counters are root-only
	if (geteuid() != 0) {
		sp_log("top-syspower: not root, PWR column disabled\n");
		return -1;
	}
	return grp_strap();
}

int sp_sample(float *watts)
{
	int64_t uw = 0;
	int err = grp_step(&uw);

	*watts = (float)uw / 1000000.0f;
	// run as: sudo env TOP_SYSPOWER_DEBUG=1 top  -> logs every measurement
	if (getenv("TOP_SYSPOWER_DEBUG"))
		sp_log("top-syspower: total=%.3f W err=%d\n", *watts, err);
	return err;
}
```
> **`#include <src/dconf.h>` must come first.** syspower's headers (e.g. `ut/gbuf.h`) use macros like `#include LIB_INT` that are defined in `dconf.h`. Without it, the build fails with `#include expects "FILENAME" or <FILENAME>`.

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

In **`procps/Makefile.am`**, right after the `src_top_top_LDADD = ...` line:

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

All in `procps/src/top/`. Line numbers are approximate (v4.0.4).

> **`top.h` is saved in the old ISO-8859 text encoding, not UTF-8.** Some editors quietly convert it when saving, which corrupts a few special characters in it. If your editor asks about the encoding, keep ISO-8859-1. Or make the `top.h` edits with `sed`.

### 7a. Register a new column: `top.h`, `top.c`, `top_nls.c`

`top` gives every column an ID number from a list called `enum pflag`. A comment there warns that it **must stay in step with the `Fieldstab[]` table** in `top.c`.

**`top.h`** (~line 228): add `EU_PWR` after the last real column:
```c
   EU_NS7, EU_NS8,
   EU_PWR,
```

**`top.c`** (~line 2004): add a matching table row, and move `eu_LAST`:
```c
   {    10,     -1,  A_right,  PIDS_NS_TIME        },  // ul_int   EU_NS8
   {     6,     -1,  A_right,  PIDS_TICS_ALL_DELTA }   // u_int    EU_PWR
#define eu_LAST        EU_PWR
```
- Don't forget the new comma at the end of the `EU_NS8` line.
- `6` is the column width, enough for `12.345`.
- `PIDS_TICS_ALL_DELTA` is **the same data `%CPU` uses**: each process's CPU time since the last refresh. Two columns sharing one data item is normal (`TIME` and `TIME+` do it too). It also means sorting by PWR gives the same order as sorting by `%CPU`.

**`top_nls.c`** (~line 389): add the column title and its description in the `f` (fields) menu:
```c
   Head_nlstab[EU_PWR] = _("PWR");
   Desc_nlstab[EU_PWR] = _("Power, W (share of syspower)");
```

### 7b. Show PWR by default, right after %CPU: `top.h`

`DEF_FIELDS` (~line 567, the `#else` copy, *not* the `ORIG_TOPDEFS` one) lists every column as a number:

```
number = (column ID + 37) × 2 + 1   if shown
number = (column ID + 37) × 2       if hidden
```
(37 is the character code of `%`, the `FLD_OFFSET` in `top.h`.)

`EU_PWR` is column **78**, so PWR is `231` when shown and `230` when hidden. `%CPU` is `111`. Two edits:
- put `231` right after `111` on the first line
- **delete `230`** from the fourth line

Each column must appear exactly once, or `top` rejects its own defaults.

### 7c. Start as a flat list, not a tree: `top.h`

Upstream procps starts `top` in **tree view** (`Show_FOREST` in `DEF_WINFLGS`, ~line 305). Ubuntu's `top` doesn't. Remove `Show_FOREST` from that line:
```c
#define DEF_WINFLGS ( View_LOADAV | View_STATES | View_MEMORY | Show_CMDLIN \
   | Show_COLORS | Show_HIROWS | Show_IDLEPS | Show_JRNUMS | Show_TASKON \
   | Qsrt_NORMAL )
```
**Why this matters:** in tree view, a busy process like fio sits deep under `systemd → … → your terminal → bash`. It ends up far down the list below idle system processes. In batch mode (`-b`), the tree indentation even pushes its name off the end of the line. You can still press `V` to turn tree view on.

### 7d. Measure power each refresh: `top.c`

Near the top, with the other includes (~line 61):
```c
#include "syspower/sp_top.h"
```

Before `tasks_refresh()` (~line 2800):
```c
        /*
         * Power column (PWR): syspower's measuring code (src/top/syspower)
         * runs inside top. Each refresh it measures whole-system watts,
         * then each task gets the share of those watts matching its
         * share of all cpu time (see EU_PWR in task_show). */
static float Pwr_watts;               // whole-system watts since the last frame
static int   Pwr_ok;                  // 1 when this frame's measurement worked

static void pwr_refresh (void) {
   static int ready = -1;             // -1 = not tried yet, 0 = no, 1 = yes

   if (ready < 0)
      ready = (sp_init() == 0);
   Pwr_ok = ready && sp_sample(&Pwr_watts) == 0;
} // end: pwr_refresh
```

Inside `tasks_refresh()`, right after the line that sets `Frame_etscale` (~line 2841):
```c
      Frame_etscale = 100.0f / ((float)Hertz * (float)et * (Rc.mode_irixps ? 1 : Cpu_cnt));
      pwr_refresh();
```
This is where `top` works out the time since the last refresh. Measuring power here means power and CPU time cover the same window.

### 7e. Display each process's share: `top.c`, `task_show()`

Add a new `case` just before `case EU_CPU:` (~line 6942):
```c
   /* u_int, this task's share of syspower's watts (see pwr_refresh) */
         case EU_PWR:        // PIDS_TICS_ALL_DELTA
         {  float pct = (float)rSv(EU_PWR, u_int) * Frame_etscale;
            if (Restrict_some || !Pwr_ok) {
               cp = justify_pad("-", W, Jn);
               break;
            }
            // irix mode is % of one cpu, so make it % of the whole machine
            if (Rc.mode_irixps)
               pct /= (float)Cpu_cnt;
            // last arg 1 = up to 3 decimals, since most tasks use well under 1 W
            cp = scale_pcnt(pct / 100.0f * Pwr_watts, W, Jn, 1);
         }
            break;
```
- `rSv(EU_PWR, u_int)` is this process's CPU ticks since the last refresh.
- `× Frame_etscale` turns ticks into a percentage. That's % of one core in Irix mode (the default) or % of the whole machine in Solaris mode (`Shift+I`).
- In Irix mode, dividing by `Cpu_cnt` (number of cores) makes it a share of the whole machine.
- `-` is shown when not running as root, or when a measurement failed.

---

## 8. Build, run, install, test

### Build and run from the build folder
```bash
cd ~/Documents/lab/procps
make -j8 src/top/top
./src/top/top              # not root -> PWR shows "-"
sudo ./src/top/top         # root     -> PWR shows watts after ~2 refreshes
```
The first one or two refreshes show `0.000`. The CPU power counter (RAPL) needs two readings before it can calculate a rate.

### Install so plain `top` / `sudo top` run your version
```bash
sudo ./libtool --mode=install install -m 755 src/top/top /usr/local/bin/top
hash -r                    # make bash forget it found top in /usr/bin before
which top                  # -> /usr/local/bin/top
```
- `/usr/local/bin` comes before `/usr/bin` in the search path, including `sudo`'s.
- Ubuntu's original stays in `/usr/bin/top`.
- **After every code change**, rebuild and repeat the install command.

Undo:
```bash
sudo rm /usr/local/bin/top && hash -r
```

### Test with fio
In the syspower repo:
```bash
tools/fio_dual.sh 60       # 60 s of a heavy and a light fio job, in parallel
```
In another terminal run `sudo top`. Both `fio` lines should rise to the top of the list (sorted by %CPU):

| Job | %CPU (14-core laptop) | PWR |
|---|---|---|
| heavy: 16 workers, 4k writes, io_uring, queue depth 64 | ~150–190 | a few W |
| light: 1 worker, 1M reads, queue depth 4 | ~15–25 | a few tenths of a W |

**Check:** the **PWR ratio should match the %CPU ratio** (e.g. 3 ÷ 0.25 = 12 ≈ 170 ÷ 15). The script uses fio's `thread` option, so each job is **one process** no matter how many workers it has.

To see the total watts behind each refresh:
```bash
sudo env TOP_SYSPOWER_DEBUG=1 top -b -n 5 -d 3 | grep -E "PID|fio"
sudo cat /var/tmp/top-syspower.log      # total=XX.XXX W err=0 per refresh
```

---

## 9. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| PWR always `-` | Not running as root | `sudo top`. The log says `not root, PWR column disabled`. |
| PWR `-` as root | A measurement failed | Check `/var/tmp/top-syspower.log`, or run with `TOP_SYSPOWER_DEBUG=1` |
| Log file stays empty under `sudo` | The log was first created by your normal user. Ubuntu's `fs.protected_regular=2` stops even root from writing to another user's file in shared folders like `/var/tmp`. | `rm /var/tmp/top-syspower.log` and let root create it |
| Busy process not visible / shows 0.0 | Tree view: it's deep in the tree and off screen | Press `V` for the flat list, then `Shift+P` to sort by CPU. The default should already be flat (step 7c). |
| Everything shows `0.000` | Idle processes really are ~0 W, or you're in the first two refreshes | Run a load (fio, or `yes > /dev/null &`) and wait ~6 s |
| `failed to run model(s) for a group!` and the program stops (standalone syspower) | A device with **no model name** (e.g. Intel WiFi) matched *every* known part in `hw_match()` and ended up as a Samsung SSD | `hw.c` fix (syspower commit `917ac7c`): `if (size == 0) return false;` |
| Standalone syspower: `libnvidia-ml.so.1: cannot open shared object file` | No NVIDIA driver on the machine | `sudo env LD_LIBRARY_PATH=$(ls -d /nix/store/*cuda_nvml_dev*-stubs/lib/stubs \| head -1) ./result/bin/syspower` (not needed for `top`, which uses `nvml_stub.c`) |
| Build: `(*__errno_location ()).h: No such file` | `#include LIB_ERRNO` after `<errno.h>` is already included: the word `errno` inside the macro gets replaced too | Write `#include <errno.h>` directly |
| Build: `#include expects "FILENAME" or <FILENAME>` | A file included a syspower header before `src/dconf.h` | Include `<src/dconf.h>` first |
| Plain `/usr/bin/top` complains about its settings | You pressed `W` (save) in the modified `top`. Its settings file has one extra column the original doesn't know. | Delete `~/.config/procps/toprc`, or don't save with `W` |
| `sudo cmd &` hangs forever | A background `sudo` can't ask for your password, so it waits (shows as state `T`) | Run `sudo -v` first. Remove stuck ones with `sudo kill -9 <pid>`. |

---

## 10. Limitations and ideas for next steps

- **CPU time only explains CPU power.** fio's disk power is in the total, but it's split by CPU time, so I/O-heavy processes get too little and CPU-heavy ones too much. A next step: split each syspower group by its own activity, e.g. storage power by each process's disk bytes (`/proc/<pid>/io`).
- **The total depends on how well syspower knows the hardware.** Laptops get desktop defaults from `dconf.h`. On the dev laptop, 8 LPDDR5 chips × the desktop DIMM range made the memory estimate far too high, so the total was inflated and jumped between ~9 and ~46 W. Adding the machine's parts under `hw/mpn/` fixes this. On **lamport** (the hardware syspower was written for) it should be accurate.
- **Idle power isn't assigned to anyone.** You could split it evenly between processes instead, or by memory use.
- **NVIDIA GPU power is stubbed out.** To support it, use the real `mdl/nvml/nvml.c`, add CUDA's `nvml.h` include path, and link `-lnvidia-ml`.
- **The copy doesn't follow syspower.** When syspower changes, re-run the copy from step 4 and re-apply the two edits in `grp.c`/`grp.h` and `dconf.h`. One improvement would be to add `grp_step()` and the `#ifndef CFG_PRINTF_ERR` to syspower itself, so the copy needs no edits.
