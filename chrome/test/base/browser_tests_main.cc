// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/launcher/test_launcher.h"
#include "build/build_config.h"
#include "chrome/test/base/chrome_test_launcher.h"
#include "chrome/test/base/chrome_test_suite.h"

#if defined(OS_WIN)
#include "base/test/test_switches.h"
#include "base/win/win_util.h"
#endif  // defined(OS_WIN)

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs == 0U) {
    return 1;
  } else if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }

#if defined(OS_WIN)
  // Enable high-DPI for interactive tests where the user is expected to
  // manually verify results.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTestLauncherInteractive)) {
    base::win::EnableHighDPISupport();
  }
#endif  // defined(OS_WIN)

  ChromeTestSuiteRunner runner;
  ChromeTestLauncherDelegate delegate(&runner);
  return LaunchChromeTests(parallel_jobs, &delegate, argc, argv);
}
