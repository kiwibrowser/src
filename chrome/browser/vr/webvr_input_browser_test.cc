// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/browser/vr/test/xr_browser_test.h"

// Browser test equivalent of
// chrome/android/javatests/src/.../browser/vr_shell/WebVrInputTest.java.
// End-to-end tests for user input interaction with WebVR.

namespace vr {

// Test that focus is locked to the presenting display for the purposes of VR/XR
// input.
void TestPresentationLocksFocusImpl(VrXrBrowserTestBase* t,
                                    std::string filename) {
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->EnterPresentationOrFail(t->GetFirstTabWebContents());
  t->ExecuteStepAndWait("stepSetupFocusLoss()", t->GetFirstTabWebContents());
  t->EndTest(t->GetFirstTabWebContents());
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestStandard,
                       REQUIRES_GPU(TestPresentationLocksFocus)) {
  TestPresentationLocksFocusImpl(this, "test_presentation_locks_focus");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestPresentationLocksFocus)) {
  TestPresentationLocksFocusImpl(this, "webxr_test_presentation_locks_focus");
}

}  // namespace vr
