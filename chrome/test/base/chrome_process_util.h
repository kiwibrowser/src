// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_CHROME_PROCESS_UTIL_H_
#define CHROME_TEST_BASE_CHROME_PROCESS_UTIL_H_

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics.h"
#include "build/build_config.h"

using ChromeProcessList = std::vector<base::ProcessId>;

// Returns a vector of PIDs of all chrome processes (main and renderers etc)
// based on |browser_pid|, the PID of the main browser process.
ChromeProcessList GetRunningChromeProcesses(base::ProcessId browser_pid);

// Attempts to terminate all chrome processes in |process_list|.
void TerminateAllChromeProcesses(const ChromeProcessList& process_list);

// A wrapper class for tests to use in fetching process metrics.
// Delegates everything we need to base::ProcessMetrics, except
// memory stats on Mac (which have to parse ps output due to privilege
// restrictions, behavior we don't want in base).  Long-term, if
// the production base::ProcessMetrics gets updated to return
// acceptable metrics on Mac, this class should disappear.
class ChromeTestProcessMetrics {
 public:
  static ChromeTestProcessMetrics* CreateProcessMetrics(
        base::ProcessHandle process) {
    return new ChromeTestProcessMetrics(process);
  }

  bool GetIOCounters(base::IoCounters* io_counters);

  base::ProcessHandle process_handle_;

  ~ChromeTestProcessMetrics();

 private:
  explicit ChromeTestProcessMetrics(base::ProcessHandle process);

  std::unique_ptr<base::ProcessMetrics> process_metrics_;

  DISALLOW_COPY_AND_ASSIGN(ChromeTestProcessMetrics);
};

#if defined(OS_MACOSX)

// These types and API are here to fetch the information about a set of running
// processes by ID on the Mac.  There are also APIs in base, but fetching the
// information for another process requires privileges that a normal executable
// does not have.  This API fetches the data by spawning ps (which is setuid so
// it has the needed privileges) and processing its output. The API is provided
// here because we don't want code spawning processes like this in base, where
// someone writing cross platform code might use it without realizing that it's
// a heavyweight call on the Mac.

struct MacChromeProcessInfo {
  base::ProcessId pid;
  int rsz_in_kb;
  int vsz_in_kb;
};

using MacChromeProcessInfoList = std::vector<MacChromeProcessInfo>;

// Any ProcessId that info can't be found for will be left out.
MacChromeProcessInfoList GetRunningMacProcessInfo(
    const ChromeProcessList& process_list);

#endif  // defined(OS_MACOSX)

#endif  // CHROME_TEST_BASE_CHROME_PROCESS_UTIL_H_
