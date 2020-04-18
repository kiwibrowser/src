// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/launcher/test_launcher.h"
#include "extensions/shell/test/shell_test_launcher_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }
  extensions::AppShellTestLauncherDelegate launcher_delegate;
  return content::LaunchTests(&launcher_delegate, parallel_jobs, argc, argv);
}
