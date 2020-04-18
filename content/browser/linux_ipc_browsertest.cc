// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <set>
#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "content/browser/sandbox_ipc_linux.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "services/service_manager/sandbox/switches.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class LinuxIPCBrowserTest : public ContentBrowserTest,
                            public SandboxIPCHandler::TestObserver,
                            public testing::WithParamInterface<std::string> {
 public:
  LinuxIPCBrowserTest() {
    SandboxIPCHandler::SetObserverForTests(this);
  }
  ~LinuxIPCBrowserTest() override {}

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ContentBrowserTest::SetUpCommandLine(command_line);
    if (GetParam() == "no-zygote") {
      command_line->AppendSwitch(service_manager::switches::kNoSandbox);
      command_line->AppendSwitch(switches::kNoZygote);
    }
  }

  void OnFontOpen(int id) override {
    base::AutoLock lock(lock_);
    opened_fonts_.insert(font_names_[id]);
  }

  void OnGetFallbackFontForChar(UChar32 c, std::string name, int id) override {
    base::AutoLock lock(lock_);
    fallback_fonts_[c] = name;
    font_names_[id] = name;
  }

  std::string FallbackFontName(UChar32 c) {
    base::AutoLock lock(lock_);
    return fallback_fonts_[c];
  }

  std::set<std::string> OpenedFonts() {
    base::AutoLock lock(lock_);
    return opened_fonts_;
  }

  // These variables are accessed on the IPC thread and the test thread.
  // All accesses on the IPC thread should be before the renderer process
  // completes navigation, and all accesses on the test thread should be after.
  // However we still need a mutex for the accesses to be sequenced according to
  // the C++ memory model.
  base::Lock lock_;
  std::map<UChar32, std::string> fallback_fonts_;
  std::map<int, std::string> font_names_;
  std::set<std::string> opened_fonts_;

  DISALLOW_COPY_AND_ASSIGN(LinuxIPCBrowserTest);
};

// Tests that Linux IPC font fallback code runs round-trip when Zygote is
// disabled. It doesn't care what font is chosen, just that the IPC messages are
// flowing. This test assumes that U+65E5 (CJK "Sun" character) will trigger the
// font fallback codepath.
IN_PROC_BROWSER_TEST_P(LinuxIPCBrowserTest, FontFallbackIPCWorks) {
  GURL test_url = GetTestUrl("font", "font_fallback.html");
  EXPECT_TRUE(NavigateToURL(shell(), test_url));
  EXPECT_THAT(FallbackFontName(U'\U000065E5'), testing::Ne(""));
  EXPECT_THAT(OpenedFonts(),
              testing::Contains(FallbackFontName(U'\U000065E5')));
}

INSTANTIATE_TEST_CASE_P(LinuxIPCBrowserTest,
                        LinuxIPCBrowserTest,
                        ::testing::Values("zygote", "no-zygote"));

}  // namespace
