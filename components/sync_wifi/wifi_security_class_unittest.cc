// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_security_class.h"

#include "components/onc/onc_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_wifi {

TEST(WifiSecurityClassTest, WifiSecurityClassSupportsPassphrases) {
  EXPECT_FALSE(WifiSecurityClassSupportsPassphrases(SECURITY_CLASS_NONE));
  EXPECT_TRUE(WifiSecurityClassSupportsPassphrases(SECURITY_CLASS_WEP));
  EXPECT_TRUE(WifiSecurityClassSupportsPassphrases(SECURITY_CLASS_PSK));
  EXPECT_TRUE(WifiSecurityClassSupportsPassphrases(SECURITY_CLASS_802_1X));
  EXPECT_FALSE(WifiSecurityClassSupportsPassphrases(SECURITY_CLASS_INVALID));
}

TEST(WifiSecurityClassTest, WifiSecurityClassFromSyncSecurityClass) {
  EXPECT_EQ(SECURITY_CLASS_NONE,
            WifiSecurityClassFromSyncSecurityClass(
                sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_NONE));
  EXPECT_EQ(SECURITY_CLASS_WEP,
            WifiSecurityClassFromSyncSecurityClass(
                sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_WEP));
  EXPECT_EQ(SECURITY_CLASS_PSK,
            WifiSecurityClassFromSyncSecurityClass(
                sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_PSK));
  EXPECT_EQ(SECURITY_CLASS_INVALID,
            WifiSecurityClassFromSyncSecurityClass(
                sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_INVALID));
}

TEST(WifiSecurityClassTest, WifiSecurityClassToSyncSecurityClass) {
  EXPECT_EQ(sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_NONE,
            WifiSecurityClassToSyncSecurityClass(SECURITY_CLASS_NONE));
  EXPECT_EQ(sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_WEP,
            WifiSecurityClassToSyncSecurityClass(SECURITY_CLASS_WEP));
  EXPECT_EQ(sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_PSK,
            WifiSecurityClassToSyncSecurityClass(SECURITY_CLASS_PSK));
  EXPECT_EQ(sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_INVALID,
            WifiSecurityClassToSyncSecurityClass(SECURITY_CLASS_802_1X));
  EXPECT_EQ(sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_INVALID,
            WifiSecurityClassToSyncSecurityClass(SECURITY_CLASS_INVALID));
}

TEST(WifiSecurityClassTest, WifiSecurityClassToOncSecurityString) {
  std::string security_string;

  EXPECT_TRUE(WifiSecurityClassToOncSecurityString(SECURITY_CLASS_NONE,
                                                   &security_string));
  EXPECT_EQ(onc::wifi::kSecurityNone, security_string);

  EXPECT_TRUE(WifiSecurityClassToOncSecurityString(SECURITY_CLASS_WEP,
                                                   &security_string));
  EXPECT_EQ(onc::wifi::kWEP_PSK, security_string);

  EXPECT_TRUE(WifiSecurityClassToOncSecurityString(SECURITY_CLASS_PSK,
                                                   &security_string));
  EXPECT_EQ(onc::wifi::kWPA_PSK, security_string);

  EXPECT_TRUE(WifiSecurityClassToOncSecurityString(SECURITY_CLASS_802_1X,
                                                   &security_string));
  EXPECT_EQ(onc::wifi::kWPA_EAP, security_string);

  EXPECT_FALSE(WifiSecurityClassToOncSecurityString(SECURITY_CLASS_INVALID,
                                                    &security_string));
}

}  // namespace sync_wifi
