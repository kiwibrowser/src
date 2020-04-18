// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_

#include "chrome/browser/vr/test/vr_xr_browser_test.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"

namespace vr {

// VR-specific test base class.
class VrBrowserTestBase : public VrXrBrowserTestBase {
 public:
  // Returns true if the JavaScript in the given WebContents has found a
  // WebVR VRDisplay, false otherwise.
  static bool VrDisplayFound(content::WebContents* web_contents);

  void EnterPresentation(content::WebContents* web_contents) override;
  void EnterPresentationOrFail(content::WebContents* web_contents) override;
  void ExitPresentation(content::WebContents* web_contents) override;
  void ExitPresentationOrFail(content::WebContents* web_contents) override;
};

// Test class with standard features enabled: WebVR, OpenVR support, and the
// Gamepad API.
class VrBrowserTestStandard : public VrBrowserTestBase {
 public:
  VrBrowserTestStandard() {
    append_switches_.push_back(switches::kEnableWebVR);
    enable_features_.push_back(features::kOpenVR);
    enable_features_.push_back(features::kGamepadExtensions);
  }
};

// Test class with WebVR disabled.
class VrBrowserTestWebVrDisabled : public VrBrowserTestBase {
 public:
  VrBrowserTestWebVrDisabled() {
    enable_features_.push_back(features::kOpenVR);
    enable_features_.push_back(features::kGamepadExtensions);
  }
};

// Test class with OpenVR support disabled.
class VrBrowserTestOpenVrDisabled : public VrBrowserTestBase {
 public:
  VrBrowserTestOpenVrDisabled() {
    append_switches_.push_back(switches::kEnableWebVR);
    enable_features_.push_back(features::kGamepadExtensions);
  }
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
