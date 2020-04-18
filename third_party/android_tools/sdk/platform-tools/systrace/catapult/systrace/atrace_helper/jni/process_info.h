// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROCESS_INFO_H_
#define PROCESS_INFO_H_

#include <map>
#include <memory>

#include "process_memory_stats.h"

// Reads various process stats and details from /proc/pid/.
class ProcessInfo {
 public:
  struct ThreadInfo {
    char name[128] = {};
  };
  using ThreadInfoMap = std::map<int, std::unique_ptr<ThreadInfo>>;

  // Returns true if |pid| is a process (|pid| == TGID), false if it's just a
  // thread of another process, or if |pid| doesn't exist at all.
  static bool IsProcess(int pid);

  explicit ProcessInfo(int pid);

  bool ReadProcessName();
  bool ReadThreadNames();
  bool ReadOOMStats();
  bool ReadPageFaultsAndCPUTimeStats();

  ProcessMemoryStats* memory() { return &memory_; }
  const ProcessMemoryStats* memory() const { return &memory_; }
  const ThreadInfoMap* threads() const { return &threads_; }
  const char* name() const { return name_; }
  const char* exe() const { return exe_; }

  int oom_adj() const { return oom_adj_; }
  int oom_score_adj() const { return oom_score_adj_; }
  int oom_score() const { return oom_score_; }

  unsigned long minflt() const { return minflt_; }
  unsigned long majflt() const { return majflt_; }
  unsigned long utime() const { return utime_; }
  unsigned long stime() const { return stime_; }
  unsigned long long start_time() const { return start_time_; }

 private:
  ProcessInfo(const ProcessInfo&) = delete;
  void operator=(const ProcessInfo&) = delete;

  ProcessMemoryStats memory_;

  ThreadInfoMap threads_;
  char name_[128] = {};
  char exe_[128] = {};

  int oom_adj_ = 0;
  int oom_score_adj_ = 0;
  int oom_score_ = 0;

  unsigned long minflt_ = 0;
  unsigned long majflt_ = 0;
  unsigned long utime_ = 0;            // CPU time in user mode.
  unsigned long stime_ = 0;            // CPU time in kernel mode.
  unsigned long long start_time_ = 0;  // CPU time in kernel mode.

  const int pid_;
};

#endif  // PROCESS_INFO_H_
