// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/test/shell_apitest.h"

namespace extensions {

IN_PROC_BROWSER_TEST_F(ShellApiTest,
                       ExtensionGeolocationShouldReturnPositionUnavailable) {
  // app_shell does not implement CreateAccessTokenStore() and the other
  // bits for proper Geolocation support. We make sure that clients of this
  // API will always get "permission denied" and won't crash.
  ASSERT_TRUE(RunAppTest("geolocation/always_position_unavailable"))
      << message_;
}

}  // namespace extensions
