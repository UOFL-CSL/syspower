# syspower inside top

Power-measuring code from [syspower](https://github.com/UOFL-CSL/syspower), compiled into `top` to fill the power columns (`CPU_W`, `RAM_W`, `DSK_W`, `TOT_W`, ...) and the `Watts` summary line.

**Source:** syspower commit `c129213` plus the blank-name fix in `src/hw/hw.c`. That fix stops `hw_match()` from matching every part when a device has no model name.

## What's here

| Path | What it is |
|---|---|
| `src/` | Copied from syspower's `src/`, keeping its folder layout so its `#include <src/...>` lines still work. Left out: `main.c`, the makefiles, and `mdl/nvml/nvml.c`. |
| `sp_top.h` | The only file `top.c` includes. It declares `sp_init()` and `sp_sample()`. |
| `sp_top.c` | Connects top to syspower: sets up the hardware groups, measures them, and logs errors. |
| `sp_config.h` | Force-included before every syspower file (see `Makefile.am`). It defines `NAME` and sends syspower's error messages to `/var/tmp/top-syspower.log`. |
| `nvml_stub.c` | Stand-in for the NVIDIA GPU model, which needs CUDA's `nvml.h`. |

## Changes made to the copied syspower files

- `src/grp/grp.c`, `src/grp/grp.h`: added `grp_step()`, which measures every group once without printing and reports watts per syspower model (`sp_top.c` maps each model to a component: CPU, RAM, disk, ...).
- `src/dconf.h`: wrapped `CFG_PRINTF_ERR` in `#ifndef` so `sp_config.h` can redirect it.
