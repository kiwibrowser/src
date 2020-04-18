// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/browser/vr/test/xr_browser_test.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// Browser test equivalent of
// chrome/android/javatests/src/.../browser/vr_shell/WebVrTabTest.java.
// End-to-end tests for testing WebVR's interaction with multiple tabss.

namespace vr {

// Tests that non-focused tabs cannot get pose information from WebVR/WebXR
void TestPoseDataUnfocusedTabImpl(VrXrBrowserTestBase* t,
                                  std::string filename) {
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->ExecuteStepAndWait("stepCheckFrameDataWhileFocusedTab()",
                        t->GetFirstTabWebContents());
  chrome::AddTabAt(t->browser(), GURL(url::kAboutBlankURL),
                   -1 /* index, append to end */, true /* foreground */);
  t->ExecuteStepAndWait("stepCheckFrameDataWhileNonFocusedTab()",
                        t->GetFirstTabWebContents());
  t->EndTest(t->GetFirstTabWebContents());
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestStandard,
                       REQUIRES_GPU(TestPoseDataUnfocusedTab)) {
  TestPoseDataUnfocusedTabImpl(this, "test_pose_data_unfocused_tab");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestPoseDataUnfocusedTab)) {
  TestPoseDataUnfocusedTabImpl(this, "webxr_test_pose_data_unfocused_tab");
}

}  // namespace vr
