// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "device/geolocation/public/cpp/scoped_geolocation_overrider.h"

class GeolocationApiTest : public extensions::ExtensionApiTest {
 public:
  GeolocationApiTest() {
  }

  // InProcessBrowserTest
  void SetUpOnMainThread() override {
    geolocation_overrider_ =
        std::make_unique<device::ScopedGeolocationOverrider>(0, 0);
  }

 private:
  std::unique_ptr<device::ScopedGeolocationOverrider> geolocation_overrider_;
};

// http://crbug.com/68287
IN_PROC_BROWSER_TEST_F(GeolocationApiTest,
                       DISABLED_ExtensionGeolocationAccessFail) {
  // Test that geolocation cannot be accessed from extension without permission.
  ASSERT_TRUE(RunExtensionTest("geolocation/no_permission")) << message_;
}

// Timing out. http://crbug.com/128412
IN_PROC_BROWSER_TEST_F(GeolocationApiTest,
                       DISABLED_ExtensionGeolocationAccessPass) {
  // Test that geolocation can be accessed from extension with permission.
  ASSERT_TRUE(RunExtensionTest("geolocation/has_permission")) << message_;
}
