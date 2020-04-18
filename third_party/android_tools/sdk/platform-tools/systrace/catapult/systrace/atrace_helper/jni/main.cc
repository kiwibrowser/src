// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>

#include <limits>
#include <memory>

#include "file_utils.h"
#include "logging.h"
#include "process_info.h"

namespace {

using ProcessMap = std::map<int, std::unique_ptr<ProcessInfo>>;

int g_timer;

std::unique_ptr<ProcessMap> CollectStatsForAllProcs(bool full_mem_stats) {
  std::unique_ptr<ProcessMap> procs(new ProcessMap());
  file_utils::ForEachPidInProcPath("/proc", [&procs, full_mem_stats](int pid) {
    if (!ProcessInfo::IsProcess(pid))
      return;
    CHECK(procs->count(pid) == 0);
    std::unique_ptr<ProcessInfo> pinfo(new ProcessInfo(pid));
    if (!(pinfo->ReadProcessName() && pinfo->ReadThreadNames() &&
          pinfo->ReadOOMStats() && pinfo->ReadPageFaultsAndCPUTimeStats()))
      return;

    if (full_mem_stats) {
      if (!pinfo->memory()->ReadFullStats())
        return;
    } else {
      if (!pinfo->memory()->ReadLightStats())
        return;
    }
    (*procs)[pid] = std::move(pinfo);
  });
  return procs;
}

void SerializeSnapshot(const ProcessMap& procs,
                       FILE* stream,
                       bool full_mem_stats) {
  struct timespec ts = {};
  CHECK(clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0);
  fprintf(stream, "{\n");
  fprintf(stream, "  \"ts\": %lu,\n",
          (ts.tv_sec * 1000 + ts.tv_nsec / 1000000ul));
  fprintf(stream, "  \"processes\": [\n");
  for (auto it = procs.begin(); it != procs.end();) {
    int pid = it->first;
    const ProcessInfo& pinfo = *it->second;
    fprintf(stream, "    {\"pid\": %d, \"name\": \"%s\", \"exe\": \"%s\"", pid,
            pinfo.name(), pinfo.exe());
    fprintf(stream, ", \"threads\": [");
    for (auto t = pinfo.threads()->begin(); t != pinfo.threads()->end();) {
      fprintf(stream, "{\"tid\": %d, \"name\":\"%s\"", t->first,
              t->second->name);
      t++;
      fprintf(stream, t != pinfo.threads()->end() ? "}, " : "}");
    }
    fprintf(stream, "]");

    const ProcessMemoryStats* mem_info = pinfo.memory();
    fprintf(stream, ", \"mem\": {\"vm\": %llu, \"rss\": %llu",
            mem_info->virt_kb(), mem_info->rss_kb());
    if (full_mem_stats) {
      fprintf(stream,
              ", \"pss\": %llu, \"swp\": %llu, \"pc\": %llu, \"pd\": %llu, "
              "\"sc\": %llu, \"sd\": %llu",
              mem_info->rss_kb(), mem_info->swapped_kb(),
              mem_info->private_clean_kb(), mem_info->private_dirty_kb(),
              mem_info->shared_clean_kb(), mem_info->shared_dirty_kb());
    }
    fprintf(stream, "}");

    fprintf(stream,
            ", \"oom\": {\"adj\": %d, \"score_adj\": %d, \"score\": %d}",
            pinfo.oom_adj(), pinfo.oom_score_adj(), pinfo.oom_score());
    fprintf(stream,
            ", \"stat\": {\"minflt\": %lu, \"majflt\": %lu, "
            "\"utime\": %lu, \"stime\": %lu }",
            pinfo.minflt(), pinfo.majflt(), pinfo.utime(), pinfo.stime());
    fprintf(stream, "}");
    it++;
    fprintf(stream, it != procs.end() ? ",\n" : "\n");
  }
  fprintf(stream, "  ]\n");
  fprintf(stream, "}\n");
}

}  // namespace

int main(int argc, char** argv) {
  bool background = false;
  int dump_interval_ms = 5000;
  char out_file[PATH_MAX] = {};
  bool dump_to_file = false;
  bool full_mem_stats = false;
  int count = std::numeric_limits<int>::max();
  int opt;
  while ((opt = getopt(argc, argv, "bmt:o:c:")) != -1) {
    switch (opt) {
      case 'b':
        background = true;
        break;
      case 'm':
        full_mem_stats = true;
        break;
      case 't':
        dump_interval_ms = atoi(optarg);
        CHECK(dump_interval_ms > 0);
        break;
      case 'c':
        count = atoi(optarg);
        CHECK(count > 0);
        break;
      case 'o':
        strncpy(out_file, optarg, sizeof(out_file));
        dump_to_file = true;
        break;
      default:
        fprintf(stderr,
                "Usage: %s [-b] [-t dump_interval_ms] [-c dumps_count] "
                "[-o out.json]\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (geteuid()) {
    fprintf(stderr, "Must run as root\n");
    exit(EXIT_FAILURE);
  }

  FILE* out_stream = stdout;
  char tmp_file[PATH_MAX];
  if (dump_to_file) {
    unlink(out_file);
    sprintf(tmp_file, "%s.tmp", out_file);
    out_stream = fopen(tmp_file, "w");
    CHECK(out_stream);
  }

  if (background) {
    if (!dump_to_file) {
      fprintf(stderr, "-b requires -o for output dump path\n");
      exit(EXIT_FAILURE);
    }
    printf("Continuing in background. kill -TERM to terminate the daemon.\n");
    CHECK(daemon(0 /* nochdir */, 0 /* noclose */) == 0);
  }

  g_timer = timerfd_create(CLOCK_MONOTONIC, 0);
  CHECK(g_timer >= 0);
  struct itimerspec ts = {};
  ts.it_value.tv_nsec = 1;  // Get the first snapshot immediately.
  ts.it_interval.tv_nsec = (dump_interval_ms % 1000) * 1000000ul;
  ts.it_interval.tv_sec = dump_interval_ms / 1000;
  CHECK(timerfd_settime(g_timer, 0, &ts, nullptr) == 0);

  // Closing the g_timer fd on SIGINT/SIGTERM will cause the read() below to
  // unblock and fail with EBADF, hence allowing the loop below to finalize
  // the file and exit.
  auto on_exit = [](int) { close(g_timer); };
  signal(SIGINT, on_exit);
  signal(SIGTERM, on_exit);

  fprintf(out_stream, "{\"snapshots\": [\n");
  bool is_first_snapshot = true;
  for (; count > 0; count--) {
    uint64_t missed = 0;
    int res = read(g_timer, &missed, sizeof(missed));
    if (res < 0 && errno == EBADF)
      break;  // Received SIGINT/SIGTERM signal.
    CHECK(res > 0);
    if (!is_first_snapshot)
      fprintf(out_stream, ",");
    is_first_snapshot = false;

    std::unique_ptr<ProcessMap> procs = CollectStatsForAllProcs(full_mem_stats);
    SerializeSnapshot(*procs, out_stream, full_mem_stats);
    fflush(out_stream);
  }
  fprintf(out_stream, "]}\n");
  fclose(out_stream);
  if (dump_to_file)
    rename(tmp_file, out_file);
}
