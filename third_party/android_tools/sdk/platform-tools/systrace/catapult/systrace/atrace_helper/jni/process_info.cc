// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "process_info.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "file_utils.h"
#include "logging.h"

ProcessInfo::ProcessInfo(int pid) : memory_(pid), pid_(pid) {}

bool ProcessInfo::IsProcess(int pid) {
  char buf[256];
  ssize_t rsize = file_utils::ReadProcFile(pid, "status", buf, sizeof(buf));
  if (rsize <= 0)
    return false;
  const char kTgid[] = "\nTgid:";
  const char* tgid_line = strstr(buf, kTgid);
  CHECK(tgid_line);
  int tgid = 0;
  if (sscanf(tgid_line + strlen(kTgid), "%d", &tgid) != 1)
    CHECK(false);
  return tgid == pid;
}

bool ProcessInfo::ReadProcessName() {
  if (!file_utils::ReadProcFileTrimmed(pid_, "cmdline", name_, sizeof(name_)))
    return false;

  // Fallback on "comm" for kernel threads.
  if (strlen(name_) == 0) {
    if (!file_utils::ReadProcFileTrimmed(pid_, "comm", name_, sizeof(name_)))
      return false;
  }

  // Get also the exe path, to distinguish system vs java apps and bitness.
  char exe_path[64];
  sprintf(exe_path, "/proc/%d/exe", pid_);
  exe_[0] = '\0';
  ssize_t res = readlink(exe_path, exe_, sizeof(exe_) - 1);
  if (res >= 0)
    exe_[res] = '\0';

  return true;
}

bool ProcessInfo::ReadThreadNames() {
  char tasks_path[64];
  sprintf(tasks_path, "/proc/%d/task", pid_);
  CHECK(threads_.empty());
  ThreadInfoMap* threads = &threads_;
  const int pid = pid_;
  file_utils::ForEachPidInProcPath(tasks_path, [pid, threads](int tid) {
    char comm[64];
    std::unique_ptr<ThreadInfo> thread_info(new ThreadInfo());
    sprintf(comm, "task/%d/comm", tid);
    if (!file_utils::ReadProcFileTrimmed(pid, comm, thread_info->name,
                                         sizeof(thread_info->name))) {
      return;
    }
    (*threads)[tid] = std::move(thread_info);
  });
  return true;
}

bool ProcessInfo::ReadOOMStats() {
  char buf[512];
  if (file_utils::ReadProcFileTrimmed(pid_, "oom_adj", buf, sizeof(buf)))
    oom_adj_ = atoi(buf);
  else
    return false;

  if (file_utils::ReadProcFileTrimmed(pid_, "oom_score", buf, sizeof(buf)))
    oom_score_ = atoi(buf);
  else
    return false;

  if (file_utils::ReadProcFileTrimmed(pid_, "oom_score_adj", buf, sizeof(buf)))
    oom_score_adj_ = atoi(buf);
  else
    return false;

  return true;
}

bool ProcessInfo::ReadPageFaultsAndCPUTimeStats() {
  char buf[512];
  if (!file_utils::ReadProcFileTrimmed(pid_, "stat", buf, sizeof(buf)))
    return false;
  int ret = sscanf(buf,
                   "%*d (%*[^)]) %*c %*d %*d %*d %*d %*d %*u %lu %*lu "
                   "%lu %*lu %lu %lu %*ld %*ld %*ld %*ld %*ld %*ld %llu",
                   &minflt_, &majflt_, &utime_, &stime_, &start_time_);
  CHECK(ret == 5);
  return true;
}
