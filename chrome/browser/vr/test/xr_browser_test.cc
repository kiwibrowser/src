// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "chrome/browser/vr/test/xr_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"

namespace vr {

bool XrBrowserTestBase::XrDeviceFound(content::WebContents* web_contents) {
  return RunJavaScriptAndExtractBoolOrFail("xrDevice != null", web_contents);
}

void XrBrowserTestBase::EnterPresentation(content::WebContents* web_contents) {
  // ExecuteScript runs with a user gesture, so we can just directly call
  // requestSession instead of having to do the hacky workaround the
  // instrumentation tests use of actually sending a click event to the canvas.
  EXPECT_TRUE(content::ExecuteScript(web_contents, "onRequestSession()"));
}

void XrBrowserTestBase::EnterPresentationOrFail(
    content::WebContents* web_contents) {
  EnterPresentation(web_contents);
  EXPECT_TRUE(PollJavaScriptBoolean("exclusiveSession!= null", kPollTimeoutLong,
                                    web_contents));
}

void XrBrowserTestBase::ExitPresentation(content::WebContents* web_contents) {
  EXPECT_TRUE(content::ExecuteScript(web_contents, "exclusiveSession.end()"));
}

void XrBrowserTestBase::ExitPresentationOrFail(
    content::WebContents* web_contents) {
  ExitPresentation(web_contents);
  EXPECT_TRUE(PollJavaScriptBoolean("exclusiveSession == null",
                                    kPollTimeoutLong, web_contents));
}

}  // namespace vr
