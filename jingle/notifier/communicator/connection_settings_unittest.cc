// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/communicator/connection_settings.h"

#include <stddef.h>

#include "jingle/notifier/base/server_information.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace notifier {

namespace {

class ConnectionSettingsTest : public ::testing::Test {
 protected:
  ConnectionSettingsTest() {
    servers_.push_back(
        ServerInformation(
            net::HostPortPair("supports_ssltcp.com", 100),
            SUPPORTS_SSLTCP));
    servers_.push_back(
        ServerInformation(
            net::HostPortPair("does_not_support_ssltcp.com", 200),
            DOES_NOT_SUPPORT_SSLTCP));
  }

  ServerList servers_;
};

// An empty server list should always map to an empty connection
// settings list.
TEST_F(ConnectionSettingsTest, Empty) {
  EXPECT_TRUE(MakeConnectionSettingsList(ServerList(), false).empty());
  EXPECT_TRUE(MakeConnectionSettingsList(ServerList(), true).empty());
}

// Make sure that servers that support SSLTCP get mapped to two
// settings entries (with the SSLTCP one coming last) whereas those
// that don't map to only one.
TEST_F(ConnectionSettingsTest, Basic) {
  const ConnectionSettingsList settings_list =
      MakeConnectionSettingsList(servers_, false /* try_ssltcp_first */);

  ConnectionSettingsList expected_settings_list;
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("supports_ssltcp.com", 100),
          DO_NOT_USE_SSLTCP,
          SUPPORTS_SSLTCP));
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("supports_ssltcp.com", 443),
          USE_SSLTCP,
          SUPPORTS_SSLTCP));
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("does_not_support_ssltcp.com", 200),
          DO_NOT_USE_SSLTCP,
          DOES_NOT_SUPPORT_SSLTCP));

  ASSERT_EQ(expected_settings_list.size(), settings_list.size());
  for (size_t i = 0; i < settings_list.size(); ++i) {
    EXPECT_TRUE(settings_list[i].Equals(expected_settings_list[i]));
  }
}

// Make sure that servers that support SSLTCP get mapped to two
// settings entries (with the SSLTCP one coming first) when
// try_ssltcp_first is set.
TEST_F(ConnectionSettingsTest, TrySslTcpFirst) {
  const ConnectionSettingsList settings_list =
      MakeConnectionSettingsList(servers_, true /* try_ssltcp_first */);

  ConnectionSettingsList expected_settings_list;
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("supports_ssltcp.com", 443),
          USE_SSLTCP,
          SUPPORTS_SSLTCP));
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("supports_ssltcp.com", 100),
          DO_NOT_USE_SSLTCP,
          SUPPORTS_SSLTCP));
  expected_settings_list.push_back(
      ConnectionSettings(
          rtc::SocketAddress("does_not_support_ssltcp.com", 200),
          DO_NOT_USE_SSLTCP,
          DOES_NOT_SUPPORT_SSLTCP));

  ASSERT_EQ(expected_settings_list.size(), settings_list.size());
  for (size_t i = 0; i < settings_list.size(); ++i) {
    EXPECT_TRUE(settings_list[i].Equals(expected_settings_list[i]));
  }
}

}  // namespace

}  // namespace notifier
