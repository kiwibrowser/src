// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "chrome/browser/extensions/extension_apitest.h"

namespace {

class NetworkingConfigTest : public ExtensionApiTest {
 public:
  NetworkingConfigTest() {}
};

}  // namespace

IN_PROC_BROWSER_TEST_F(NetworkingConfigTest, ApiAvailability) {
  ASSERT_TRUE(RunExtensionSubtest("networking_config", "api_availability.html"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(NetworkingConfigTest, RegisterNetworks) {
  ASSERT_TRUE(
      RunExtensionSubtest("networking_config", "register_networks.html"))
      << message_;
}
