// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_process_util.h"

#include <set>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/process/process_metrics.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/base/test_switches.h"
#include "content/public/common/result_codes.h"

using base::TimeDelta;
using base::TimeTicks;

void TerminateAllChromeProcesses(const ChromeProcessList& process_pids) {
  for (auto pid : process_pids) {
    base::Process process = base::Process::Open(pid);
    if (process.IsValid()) {
      // Ignore processes for which we can't open the handle. We don't
      // guarantee that all processes will terminate, only try to do so.
      process.Terminate(content::RESULT_CODE_KILLED, true);
    }
  }
}

class ChildProcessFilter : public base::ProcessFilter {
 public:
  explicit ChildProcessFilter(base::ProcessId parent_pid)
      : parent_pids_(&parent_pid, (&parent_pid) + 1) {}

  explicit ChildProcessFilter(const std::vector<base::ProcessId>& parent_pids)
      : parent_pids_(parent_pids.begin(), parent_pids.end()) {}

  bool Includes(const base::ProcessEntry& entry) const override {
    return base::ContainsKey(parent_pids_, entry.parent_pid());
  }

 private:
  const std::set<base::ProcessId> parent_pids_;

  DISALLOW_COPY_AND_ASSIGN(ChildProcessFilter);
};

ChromeProcessList GetRunningChromeProcesses(base::ProcessId browser_pid) {
  const base::FilePath::CharType* executable_name =
      chrome::kBrowserProcessExecutableName;
  ChromeProcessList result;
  if (browser_pid == static_cast<base::ProcessId>(-1))
    return result;

  ChildProcessFilter filter(browser_pid);
  base::NamedProcessIterator it(executable_name, &filter);
  while (const base::ProcessEntry* process_entry = it.NextProcessEntry()) {
    result.push_back(process_entry->pid());
  }

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // On Unix we might be running with a zygote process for the renderers.
  // Because of that we sweep the list of processes again and pick those which
  // are children of one of the processes that we've already seen.
  {
    ChildProcessFilter filter(result);
    base::NamedProcessIterator it(executable_name, &filter);
    while (const base::ProcessEntry* process_entry = it.NextProcessEntry())
      result.push_back(process_entry->pid());
  }
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

#if defined(OS_POSIX)
  // On Mac OS X we run the subprocesses with a different bundle, and
  // on Linux via /proc/self/exe, so they end up with a different
  // name.  We must collect them in a second pass.
  {
    base::FilePath::StringType name = chrome::kHelperProcessExecutableName;
    ChildProcessFilter filter(browser_pid);
    base::NamedProcessIterator it(name, &filter);
    while (const base::ProcessEntry* process_entry = it.NextProcessEntry())
      result.push_back(process_entry->pid());
  }
#endif  // defined(OS_POSIX)

  result.push_back(browser_pid);

  return result;
}

ChromeTestProcessMetrics::~ChromeTestProcessMetrics() {}

ChromeTestProcessMetrics::ChromeTestProcessMetrics(
    base::ProcessHandle process) {
#if defined(OS_MACOSX)
  process_metrics_ =
      base::ProcessMetrics::CreateProcessMetrics(process, nullptr);
#else
  process_metrics_ = base::ProcessMetrics::CreateProcessMetrics(process);
#endif
  process_handle_ = process;
}

bool ChromeTestProcessMetrics::GetIOCounters(base::IoCounters* io_counters) {
  return process_metrics_->GetIOCounters(io_counters);
}
