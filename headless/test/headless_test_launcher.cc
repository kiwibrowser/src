// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/test/launcher/test_launcher.h"
#include "content/public/test/content_test_suite_base.h"
#include "content/public/test/test_launcher.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/headless_content_main_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace headless {
namespace {

class HeadlessBrowserImplForTest : public HeadlessBrowserImpl {
 public:
  explicit HeadlessBrowserImplForTest(HeadlessBrowser::Options options)
      : HeadlessBrowserImpl(base::BindOnce(&HeadlessBrowserImplForTest::OnStart,
                                           base::Unretained(this)),
                            std::move(options)) {}

  void OnStart(HeadlessBrowser* browser) { EXPECT_EQ(this, browser); }

 private:
  DISALLOW_COPY_AND_ASSIGN(HeadlessBrowserImplForTest);
};

class HeadlessTestLauncherDelegate : public content::TestLauncherDelegate {
 public:
  HeadlessTestLauncherDelegate() = default;
  ~HeadlessTestLauncherDelegate() override = default;

  // content::TestLauncherDelegate implementation:
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
    // Use HeadlessBrowserTest::options() or HeadlessBrowserContextOptions to
    // modify these defaults.
    HeadlessBrowser::Options::Builder options_builder;
    std::unique_ptr<HeadlessBrowserImpl> browser(
        new HeadlessBrowserImplForTest(options_builder.Build()));
    return new HeadlessContentMainDelegate(std::move(browser));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HeadlessTestLauncherDelegate);
};

}  // namespace
}  // namespace headless

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs();
  if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }
  headless::HeadlessTestLauncherDelegate launcher_delegate;
  return LaunchTests(&launcher_delegate, parallel_jobs, argc, argv);
}
