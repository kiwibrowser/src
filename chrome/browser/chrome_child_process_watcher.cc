// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_child_process_watcher.h"

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/common/chrome_result_codes.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/child_process_termination_info.h"

namespace {

void AnalyzeCrash(int exit_code) {
  if (exit_code == chrome::RESULT_CODE_INVALID_SANDBOX_STATE) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    const bool no_startup_window =
        command_line->HasSwitch(switches::kNoStartupWindow);
    UMA_HISTOGRAM_BOOLEAN(
        "ChildProcess.InvalidSandboxStateCrash.NoStartupWindow",
        no_startup_window);
  }
}

}  // namespace

ChromeChildProcessWatcher::ChromeChildProcessWatcher() {
  BrowserChildProcessObserver::Add(this);
}

ChromeChildProcessWatcher::~ChromeChildProcessWatcher() {
  BrowserChildProcessObserver::Remove(this);
}

void ChromeChildProcessWatcher::BrowserChildProcessCrashed(
    const content::ChildProcessData& data,
    const content::ChildProcessTerminationInfo& info) {
  AnalyzeCrash(info.exit_code);
}
