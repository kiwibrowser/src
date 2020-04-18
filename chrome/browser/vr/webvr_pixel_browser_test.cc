// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/environment.h"
#include "base/files/file.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/browser/vr/test/xr_browser_test.h"
#include "device/vr/openvr/test/fake_openvr_log.h"

#include <memory>

namespace vr {

// Pixel test for WebVR/WebXR - start presentation, submit frames, get data back
// out. Validates that a pixel was rendered with the expected color.
void TestPresentationPixelsImpl(VrXrBrowserTestBase* t, std::string filename) {
  // Set up environment variable to tell mock device to save pixel logs.
  std::unique_ptr<base::Environment> env = base::Environment::Create();
  base::ScopedTempDir temp_dir;
  base::FilePath log_path;

  {
    base::ScopedAllowBlockingForTesting allow_files;
    EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
    log_path = temp_dir.GetPath().Append(FILE_PATH_LITERAL("VRPixelTest.Log"));
    EXPECT_TRUE(log_path.MaybeAsASCII() != "") << "Temp dir is non-ascii";
    env->SetVar(GetVrPixelLogEnvVarName(), log_path.MaybeAsASCII());
  }

  // Load the test page, and enter presentation.
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->EnterPresentationOrFail(t->GetFirstTabWebContents());

  // Wait for javascript to submit at least one frame.
  EXPECT_TRUE(t->PollJavaScriptBoolean(
      "hasPresentedFrame", t->kPollTimeoutShort, t->GetFirstTabWebContents()))
      << "No frame submitted";

  // Tell javascript that it is done with the test.
  t->ExecuteStepAndWait("finishTest()", t->GetFirstTabWebContents());
  t->EndTest(t->GetFirstTabWebContents());

  // Try to open the log file.
  {
    base::ScopedAllowBlockingForTesting allow_files;
    std::unique_ptr<base::File> file;
    base::Time start = base::Time::Now();
    while (!file) {
      file = std::make_unique<base::File>(
          log_path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                        base::File::FLAG_DELETE_ON_CLOSE);
      if (!file->IsValid()) {
        file = nullptr;
      }

      if (base::Time::Now() - start > t->kPollTimeoutLong)
        break;
    }
    EXPECT_TRUE(file);

    // Now parse the log to validate that we ran correctly.
    VRSubmittedFrameEvent event;
    int read =
        file->ReadAtCurrentPos(reinterpret_cast<char*>(&event), sizeof(event));
    EXPECT_EQ(read, static_cast<int>(sizeof(event)));
    VRSubmittedFrameEvent::Color expected = {0, 0, 255, 255};
    EXPECT_EQ(expected.r, event.color.r);
    EXPECT_EQ(expected.g, event.color.g);
    EXPECT_EQ(expected.b, event.color.b);
    EXPECT_EQ(expected.a, event.color.a);

    file = nullptr;  // Make sure we destroy this before allow_files.
    EXPECT_TRUE(temp_dir.Delete());
  }
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestStandard,
                       REQUIRES_GPU(TestPresentationPixels)) {
  TestPresentationPixelsImpl(this, "test_webvr_pixels");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestPresentationPixels)) {
  TestPresentationPixelsImpl(this, "test_webxr_pixels");
}

}  // namespace vr
