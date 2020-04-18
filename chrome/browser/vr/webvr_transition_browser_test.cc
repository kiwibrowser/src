// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/browser/vr/test/xr_browser_test.h"
#include "content/public/test/browser_test_utils.h"

// Browser test equivalent of
// chrome/android/javatests/src/.../browser/vr_shell/WebVrTransitionTest.java.
// End-to-end tests for transitioning between WebVR's magic window and
// presentation modes.

namespace vr {

// Tests that a successful requestPresent or requestSession call enters
// presentation.
void TestPresentationEntryImpl(VrXrBrowserTestBase* t, std::string filename) {
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->EnterPresentationOrFail(t->GetFirstTabWebContents());
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestStandard,
                       REQUIRES_GPU(TestRequestPresentEntersVr)) {
  TestPresentationEntryImpl(this, "generic_webvr_page");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestRequestSessionEntersVr)) {
  TestPresentationEntryImpl(this, "generic_webxr_page");
}

// Tests that window.requestAnimationFrame continues to fire while in
// WebVR/WebXR presentation since the tab is still visible.
void TestWindowRafFiresWhilePresentingImpl(VrXrBrowserTestBase* t,
                                           std::string filename) {
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->ExecuteStepAndWait("stepVerifyBeforePresent()",
                        t->GetFirstTabWebContents());
  t->EnterPresentationOrFail(t->GetFirstTabWebContents());
  t->ExecuteStepAndWait("stepVerifyDuringPresent()",
                        t->GetFirstTabWebContents());
  t->ExitPresentationOrFail(t->GetFirstTabWebContents());
  t->ExecuteStepAndWait("stepVerifyAfterPresent()",
                        t->GetFirstTabWebContents());
  t->EndTest(t->GetFirstTabWebContents());
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestStandard,
                       REQUIRES_GPU(TestWindowRafFiresWhilePresenting)) {
  TestWindowRafFiresWhilePresentingImpl(
      this, "test_window_raf_fires_while_presenting");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestWindowRafFiresWhilePresenting)) {
  TestWindowRafFiresWhilePresentingImpl(
      this, "webxr_test_window_raf_fires_while_presenting");
}

// Tests that WebVR/WebXR is not exposed if the flag is not on and the page does
// not have an origin trial token. Since the API isn't actually used, we can
// remove the GPU requirement.
void TestApiDisabledWithoutFlagSetImpl(VrXrBrowserTestBase* t,
                                       std::string filename) {
  t->LoadUrlAndAwaitInitialization(t->GetHtmlTestFile(filename));
  t->WaitOnJavaScriptStep(t->GetFirstTabWebContents());
  t->EndTest(t->GetFirstTabWebContents());
}

IN_PROC_BROWSER_TEST_F(VrBrowserTestWebVrDisabled,
                       TestWebVrDisabledWithoutFlagSet) {
  TestApiDisabledWithoutFlagSetImpl(this,
                                    "test_webvr_disabled_without_flag_set");
}
IN_PROC_BROWSER_TEST_F(XrBrowserTestWebXrDisabled,
                       TestWebXrDisabledWithoutFlagSet) {
  TestApiDisabledWithoutFlagSetImpl(this,
                                    "test_webxr_disabled_without_flag_set");
}

// Tests that WebVR does not return any devices if OpenVR support is disabled.
// Since WebVR isn't actually used, we can remove the GPU requirement.
IN_PROC_BROWSER_TEST_F(VrBrowserTestOpenVrDisabled,
                       TestWebVrNoDevicesWithoutOpenVr) {
  LoadUrlAndAwaitInitialization(GetHtmlTestFile("generic_webvr_page"));
  EXPECT_FALSE(VrDisplayFound(GetFirstTabWebContents()));
}

// Tests that WebXR does not return any devices if OpenVR support is disabled.
// Since WebXR isn't actually used, we can remove the GPU requirement.
IN_PROC_BROWSER_TEST_F(XrBrowserTestOpenVrDisabled,
                       TestWebXrNoDevicesWithoutOpenVr) {
  LoadUrlAndAwaitInitialization(
      GetHtmlTestFile("test_webxr_does_not_return_device"));
  WaitOnJavaScriptStep(GetFirstTabWebContents());
  EndTest(GetFirstTabWebContents());
}

// Tests that window.requestAnimationFrame continues to fire when we have a
// non-exclusive WebXR session
IN_PROC_BROWSER_TEST_F(
    XrBrowserTestStandard,
    REQUIRES_GPU(TestWindowRafFiresDuringNonExclusiveSession)) {
  LoadUrlAndAwaitInitialization(
      GetHtmlTestFile("test_window_raf_fires_during_non_exclusive_session"));
  WaitOnJavaScriptStep(GetFirstTabWebContents());
  EndTest(GetFirstTabWebContents());
}

// Tests that non-exclusive sessions stop receiving rAFs during an exclusive
// session, but resume once the exclusive session ends.
IN_PROC_BROWSER_TEST_F(XrBrowserTestStandard,
                       REQUIRES_GPU(TestNonExclusiveStopsDuringExclusive)) {
  LoadUrlAndAwaitInitialization(
      GetHtmlTestFile("test_non_exclusive_stops_during_exclusive"));
  ExecuteStepAndWait("stepBeforeExclusive()", GetFirstTabWebContents());
  EnterPresentationOrFail(GetFirstTabWebContents());
  ExecuteStepAndWait("stepDuringExclusive()", GetFirstTabWebContents());
  ExitPresentationOrFail(GetFirstTabWebContents());
  ExecuteStepAndWait("stepAfterExclusive()", GetFirstTabWebContents());
  EndTest(GetFirstTabWebContents());
}

}  // namespace vr
