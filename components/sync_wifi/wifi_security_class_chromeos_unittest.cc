// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_security_class.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace sync_wifi {

TEST(WifiSecurityClassTest, FromShillSecurityToEnum) {
  EXPECT_EQ(SECURITY_CLASS_NONE,
            WifiSecurityClassFromShillSecurity(shill::kSecurityNone));
  EXPECT_EQ(SECURITY_CLASS_WEP,
            WifiSecurityClassFromShillSecurity(shill::kSecurityWep));
  EXPECT_EQ(SECURITY_CLASS_PSK,
            WifiSecurityClassFromShillSecurity(shill::kSecurityPsk));
  EXPECT_EQ(SECURITY_CLASS_802_1X,
            WifiSecurityClassFromShillSecurity(shill::kSecurity8021x));
  EXPECT_EQ(SECURITY_CLASS_INVALID,
            WifiSecurityClassFromShillSecurity("bogus-security-class"));
}

}  // namespace sync_wifi
