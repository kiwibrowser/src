// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/macros.h"
#include "base/sys_info.h"
#include "base/test/launcher/test_launcher.h"
#include "base/test/test_suite.h"
#include "chromecast/app/cast_main_delegate.h"
#include "content/public/test/test_launcher.h"
#include "ipc/ipc_channel.h"
#include "mojo/edk/embedder/embedder.h"

namespace chromecast {
namespace shell {

class CastTestLauncherDelegate : public content::TestLauncherDelegate {
 public:
  CastTestLauncherDelegate() {}
  ~CastTestLauncherDelegate() override {}

  int RunTestSuite(int argc, char** argv) override {
    return base::TestSuite(argc, argv).Run();
  }

  bool AdjustChildProcessCommandLine(
      base::CommandLine* command_line,
      const base::FilePath& temp_data_dir) override {
    return true;
  }

 protected:
  content::ContentMainDelegate* CreateContentMainDelegate() override {
    return new CastMainDelegate();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CastTestLauncherDelegate);
};

}  // namespace shell
}  // namespace chromecast

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }
  chromecast::shell::CastTestLauncherDelegate launcher_delegate;
  mojo::edk::Init();
  return content::LaunchTests(&launcher_delegate, parallel_jobs, argc, argv);
}
