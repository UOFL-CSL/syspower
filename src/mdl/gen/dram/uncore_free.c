
/*
 * SPDX-FileCopyrightText: 2026 Wendell Conrad Richmond
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <src/spdx/GPL-2.0-or-later.h>

// internal
#include <src/dconf.h>
#include <src/grp/grp.h>
#include <src/ut/error.h>

// external
#include LIB_IO
#include LIB_INT
#include LIB_STRING
#include LIB_TIME
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
  return syscall(SYS_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static int init_conf(char *path, unsigned long *config) {
  char buffer[64];
  int fd = open(path, O_RDONLY);
  if (fd < 0)
    goto fail;
  ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytes < 0)
    goto fail;
  buffer[bytes] = '\0';

  unsigned long event = 0, umask = 0;
  int found_event = 0;
  char *p;

  p = strstr(buffer, "event=");
  if (p) {
    if (sscanf(p, "event=%lx", &event) == 1)
      found_event = 1;
  }

  p = strstr(buffer, "umask=");
  if (p) {
    sscanf(p, "umask=%lx", &umask);
  }

  if (found_event) {
    *config = event | (umask << 8);
    return 0;
  }

  if (sscanf(buffer, "%lx", &event) == 1) {
    *config = event;
    return 0;
  }

fail:
  return -1;
}

static int64_t bytes_per_counter(char *path) {
  char buffer[64];
  char npath[256];
  CFG_STRNCPY(npath, path, sizeof(npath));
  CFG_STRNCAT(npath, ".scale", sizeof(npath));
  int fd = open(npath, O_RDONLY);
  if (fd < 0)
    goto fallback;
  ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytes < 0)
    goto fallback;
  buffer[bytes] = '\0';
  double scale;
  if (sscanf(buffer, "%lf", &scale) != 1)
    goto fallback;

  CFG_STRNCPY(npath, path, sizeof(npath));
  CFG_STRNCAT(npath, ".unit", sizeof(npath));
  fd = open(npath, O_RDONLY);
  if (fd < 0)
    goto fallback;
  bytes = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytes < 0)
    goto fallback;
  buffer[bytes] = '\0';

  if (strncmp(buffer, "MiB", 3) == 0) {
    scale *= 1024.0 * 1024.0;
  } else if (strncmp(buffer, "GB", 2) == 0) {
    scale *= 1024.0 * 1024.0 * 1024.0;
  } else if (strncmp(buffer, "Bytes", 5) == 0 || strncmp(buffer, "B", 1) == 0) {
    scale *= 1.0;
  } else {
    goto fallback;
  }

  return (int64_t)scale;

fallback:
  return 64;
}

static struct ut_gbuf dimms = {0};
struct dimm {
  int fdr;
  int fdw;
  int64_t bpr;
  int64_t bpw;
  int64_t bw_max;
  int64_t prev;
};

enum imc_type {
  IMC_TYPE_NONE = 0,
  IMC_TYPE_FREE_RUNNING_INDEXED,
  IMC_TYPE_INDEXED,
  IMC_TYPE_SINGLE,
  IMC_TYPE_FREE_RUNNING_SINGLE,
};

static int configure(struct grp_group *grp) {
  if (dimms.size == 0) {
    ut_gbuf_ialloc(&dimms);
    ut_gbuf_init(&dimms);
  }

  char base[128];
  char path[256];
  char buffer[64];
  int fdr;
  int fdw;

  enum imc_type type_detected = IMC_TYPE_NONE;
  int num_imcs = 0;
  char test_path[256];

  snprintf(test_path, sizeof(test_path),
           "/sys/bus/event_source/devices/uncore_imc_free_running_0/type");
  if (access(test_path, R_OK) == 0) {
    type_detected = IMC_TYPE_FREE_RUNNING_INDEXED;
    while (1) {
      snprintf(test_path, sizeof(test_path),
               "/sys/bus/event_source/devices/uncore_imc_free_running_%d/type",
               num_imcs);
      if (access(test_path, R_OK) == 0)
        num_imcs++;
      else
        break;
    }
  } else {
    snprintf(test_path, sizeof(test_path),
             "/sys/bus/event_source/devices/uncore_imc_0/type");
    if (access(test_path, R_OK) == 0) {
      type_detected = IMC_TYPE_INDEXED;
      while (1) {
        snprintf(test_path, sizeof(test_path),
                 "/sys/bus/event_source/devices/uncore_imc_%d/type",
                 num_imcs);
        if (access(test_path, R_OK) == 0)
          num_imcs++;
        else
          break;
      }
    } else {
      snprintf(test_path, sizeof(test_path),
               "/sys/bus/event_source/devices/uncore_imc/type");
      if (access(test_path, R_OK) == 0) {
        type_detected = IMC_TYPE_SINGLE;
        num_imcs = 1;
      } else {
        snprintf(test_path, sizeof(test_path),
                 "/sys/bus/event_source/devices/uncore_imc_free_running/type");
        if (access(test_path, R_OK) == 0) {
          type_detected = IMC_TYPE_FREE_RUNNING_SINGLE;
          num_imcs = 1;
        }
      }
    }
  }

  if (type_detected == IMC_TYPE_NONE || num_imcs <= 0)
    goto fail;

  grp->counter = (int64_t)dimms.len;
  for (uint32_t i = 0; i < grp->hw.len; i++) {
    unsigned long rconf, wconf;
    int imc_idx = (int)(i % (uint32_t)num_imcs);

    switch (type_detected) {
    case IMC_TYPE_FREE_RUNNING_INDEXED:
      snprintf(base, sizeof(base),
               "/sys/bus/event_source/devices/uncore_imc_free_running_%d",
               imc_idx);
      break;
    case IMC_TYPE_INDEXED:
      snprintf(base, sizeof(base),
               "/sys/bus/event_source/devices/uncore_imc_%d",
               imc_idx);
      break;
    case IMC_TYPE_SINGLE:
      snprintf(base, sizeof(base),
               "/sys/bus/event_source/devices/uncore_imc");
      break;
    case IMC_TYPE_FREE_RUNNING_SINGLE:
      snprintf(base, sizeof(base),
               "/sys/bus/event_source/devices/uncore_imc_free_running");
      break;
    default:
      goto fail;
    }

    const char *read_events[] = {"data_read", "data_reads", "cas_count_read"};
    char rpath[256];
    int found_r = 0;
    for (size_t e = 0; e < sizeof(read_events) / sizeof(read_events[0]); e++) {
      snprintf(rpath, sizeof(rpath), "%s/events/%s", base, read_events[e]);
      if (access(rpath, R_OK) == 0) {
        found_r = 1;
        break;
      }
    }
    if (!found_r)
      goto fail;

    const char *write_events[] = {"data_write", "data_writes", "cas_count_write"};
    char wpath[256];
    int found_w = 0;
    for (size_t e = 0; e < sizeof(write_events) / sizeof(write_events[0]); e++) {
      snprintf(wpath, sizeof(wpath), "%s/events/%s", base, write_events[e]);
      if (access(wpath, R_OK) == 0) {
        found_w = 1;
        break;
      }
    }
    if (!found_w)
      goto fail;

    if (init_conf(rpath, &rconf))
      goto fail;
    int64_t bpr = bytes_per_counter(rpath);

    if (init_conf(wpath, &wconf))
      goto fail;
    int64_t bpw = bytes_per_counter(wpath);

    if (bpr < 0 || bpw < 0)
      goto fail;

    CFG_STRNCPY(path, base, sizeof(path));
    CFG_STRNCAT(path, "/type", sizeof(path));
    int64_t type;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
      goto fail;
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes < 0)
      goto fail;
    buffer[bytes] = '\0';
    CFG_STRTOLL(buffer, 10, &type);

    int target_cpu = 0;
    CFG_STRNCPY(path, base, sizeof(path));
    CFG_STRNCAT(path, "/cpumask", sizeof(path));
    int cfd = open(path, O_RDONLY);
    if (cfd >= 0) {
      char cbuf[32];
      ssize_t cb = read(cfd, cbuf, sizeof(cbuf) - 1);
      close(cfd);
      if (cb > 0) {
        cbuf[cb] = '\0';
        sscanf(cbuf, "%d", &target_cpu);
      }
    }

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = (unsigned int)type;
    pe.size = sizeof(pe);
    pe.config = rconf;
    fdr = (int)perf_event_open(&pe, -1, target_cpu, -1, 0);
    if (fdr < 0)
      goto fail;
    memset(&pe, 0, sizeof(pe));
    pe.type = (unsigned int)type;
    pe.size = sizeof(pe);
    pe.config = wconf;
    fdw = (int)perf_event_open(&pe, -1, target_cpu, -1, 0);
    if (fdw < 0)
      goto fail_read;

    struct dimm *entry = CFG_CALLOC(1, sizeof(struct dimm));
    if (!entry)
      goto fail_write;
    entry->fdr = fdr;
    entry->bpr = bpr;
    entry->fdw = fdw;
    entry->bpw = bpw;
    ut_gbuf_push(&dimms, entry);
  }

  return 0;

fail_write:
  close(fdw);
fail_read:
  close(fdr);
fail:
  return -1;
}

int mdl_gen_dram_intel_uncore_free(struct grp_group *grp) {
  int64_t ptime = grp->jw.uj_ts_ns;
  grp->jw.uw_ts_ns = CFG_TIME_MONOTONIC_NS();
  grp->jw.uj_ts_ns = CFG_TIME_MONOTONIC_NS();
  grp->jw.uw = 0;

  if (!ptime)
    if (configure(grp))
      goto fail;

  for (uint32_t i = (uint32_t)grp->counter;
       i < grp->hw.len + (uint32_t)grp->counter; i++) {
    struct dimm *entry = dimms.ptr[i];
    struct hw_dev *dev = grp->hw.ptr[i - grp->counter];
    struct hw_props props = hw_props_best(dev);

    int64_t total = 0;
    uint64_t count = 0;

    lseek(entry->fdr, 0, SEEK_SET);
    ssize_t bytes = read(entry->fdr, &count, sizeof(count));
    total += (int64_t)count * entry->bpr;
    if (bytes < 0)
      goto fail;
    lseek(entry->fdw, 0, SEEK_SET);
    bytes = read(entry->fdw, &count, sizeof(count));
    if (bytes < 0)
      goto fail;
    total += (int64_t)count * entry->bpw;

    if (!ptime)
      continue;
    int64_t bw = ((total - entry->prev) * 1000 * 1000 * 1000) /
                 (grp->jw.uj_ts_ns - ptime);
    entry->prev = total;
    if (bw > entry->bw_max)
      entry->bw_max = bw;
    grp->jw.uw += props.uw_min + (props.uw_max * bw) / entry->bw_max;
  }

  if (grp->jw.uw)
    grp->jw.uj +=
        (grp->jw.uw * (grp->jw.uj_ts_ns - ptime)) / (1000 * 1000 * 1000);

  return 0;

fail:
  return -1;
}
