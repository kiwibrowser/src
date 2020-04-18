// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_XR_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_TEST_XR_BROWSER_TEST_H_

#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"

namespace vr {

// XR-specific test base class.
class XrBrowserTestBase : public VrXrBrowserTestBase {
 public:
  // Returns true if the JavaScript in the given WebContents has found a WebXR
  // XRDevice, false otherwise
  static bool XrDeviceFound(content::WebContents* web_contents);

  void EnterPresentation(content::WebContents* web_contents) override;
  void EnterPresentationOrFail(content::WebContents* web_contents) override;
  void ExitPresentation(content::WebContents* web_contents) override;
  void ExitPresentationOrFail(content::WebContents* web_contents) override;
};

// Test class with standard features enabled: WebXR and OpenVR.
class XrBrowserTestStandard : public XrBrowserTestBase {
 public:
  XrBrowserTestStandard() {
    enable_features_.push_back(features::kOpenVR);
    enable_features_.push_back(features::kWebXr);
  }
};

// Test class with WebXR disabled.
class XrBrowserTestWebXrDisabled : public XrBrowserTestBase {
 public:
  XrBrowserTestWebXrDisabled() {
    enable_features_.push_back(features::kOpenVR);
  }
};

// Test class with OpenVR disabled.
class XrBrowserTestOpenVrDisabled : public XrBrowserTestBase {
 public:
  XrBrowserTestOpenVrDisabled() {
    enable_features_.push_back(features::kWebXr);
  }
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_XR_BROWSER_TEST_H_
