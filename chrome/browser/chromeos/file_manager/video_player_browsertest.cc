// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/file_manager_browsertest_base.h"

#include "chromeos/chromeos_switches.h"

namespace file_manager {

template <GuestMode MODE>
class VideoPlayerBrowserTestBase : public FileManagerBrowserTestBase {
 public:
  VideoPlayerBrowserTestBase() = default;

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        chromeos::switches::kEnableVideoPlayerChromecastSupport);

    FileManagerBrowserTestBase::SetUpCommandLine(command_line);
  }

  GuestMode GetGuestMode() const override { return MODE; }

  const char* GetTestCaseName() const override {
    return test_case_name_.c_str();
  }

  const char* GetTestExtensionManifestName() const override {
    return "video_player_test_manifest.json";
  }

  void set_test_case_name(const std::string& name) { test_case_name_ = name; }

 private:
  std::string test_case_name_;

  DISALLOW_COPY_AND_ASSIGN(VideoPlayerBrowserTestBase);
};

typedef VideoPlayerBrowserTestBase<NOT_IN_GUEST_MODE> VideoPlayerBrowserTest;
typedef VideoPlayerBrowserTestBase<IN_GUEST_MODE>
    VideoPlayerBrowserTestInGuestMode;

IN_PROC_BROWSER_TEST_F(VideoPlayerBrowserTest, OpenSingleVideoOnDownloads) {
  set_test_case_name("openSingleVideoOnDownloads");
  StartTest();
}

IN_PROC_BROWSER_TEST_F(VideoPlayerBrowserTestInGuestMode,
                       OpenSingleVideoOnDownloads) {
  set_test_case_name("openSingleVideoOnDownloads");
  StartTest();
}

IN_PROC_BROWSER_TEST_F(VideoPlayerBrowserTest, OpenSingleVideoOnDrive) {
  set_test_case_name("openSingleVideoOnDrive");
  StartTest();
}

IN_PROC_BROWSER_TEST_F(VideoPlayerBrowserTest, CheckInitialElements) {
  set_test_case_name("checkInitialElements");
  StartTest();
}

IN_PROC_BROWSER_TEST_F(VideoPlayerBrowserTest, ClickControlButtons) {
  set_test_case_name("clickControlButtons");
  StartTest();
}

}  // namespace file_manager
