// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/ice_config.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {
namespace protocol {

TEST(IceConfigTest, ParseValid) {
  const char kTestConfigJson[] =
      "{"
      "  \"lifetimeDuration\": \"43200.000s\","
      "  \"iceServers\": ["
      "    {"
      "      \"urls\": ["
      "        \"turn:8.8.8.8:19234\","
      "        \"turn:[2001:4860:4860::8888]:333\","
      "        \"turn:[2001:4860:4860::8888]\","
      "        \"turn:[2001:4860:4860::8888]:333?transport=tcp\","
      "        \"turns:the_server.com\","
      "        \"turns:the_server.com?transport=udp\""
      "      ],"
      "      \"username\": \"123\","
      "      \"credential\": \"abc\""
      "    },"
      "    {"
      "      \"urls\": ["
      "        \"stun:stun_server.com:18344\","
      "        \"stun:1.2.3.4\""
      "      ]"
      "    }"
      "  ]"
      "}";

  IceConfig config = IceConfig::Parse(kTestConfigJson);

  // lifetimeDuration in the config is set to 12 hours. Verify that the
  // resulting expiration time is within 20 seconds before 12 hours after now.
  EXPECT_TRUE(base::Time::Now() + base::TimeDelta::FromHours(12) -
                  base::TimeDelta::FromSeconds(20) <
              config.expiration_time);
  EXPECT_TRUE(config.expiration_time <
              base::Time::Now() + base::TimeDelta::FromHours(12));

  EXPECT_EQ(6U, config.turn_servers.size());
  EXPECT_TRUE(cricket::RelayServerConfig("8.8.8.8", 19234, "123", "abc",
                                         cricket::PROTO_UDP,
                                         false) == config.turn_servers[0]);
  EXPECT_TRUE(cricket::RelayServerConfig("2001:4860:4860::8888", 333, "123",
                                         "abc", cricket::PROTO_UDP,
                                         false) == config.turn_servers[1]);
  EXPECT_TRUE(cricket::RelayServerConfig("2001:4860:4860::8888", 3478, "123",
                                         "abc", cricket::PROTO_UDP,
                                         false) == config.turn_servers[2]);
  EXPECT_TRUE(cricket::RelayServerConfig("2001:4860:4860::8888", 333, "123",
                                         "abc", cricket::PROTO_TCP,
                                         false) == config.turn_servers[3]);
  EXPECT_TRUE(cricket::RelayServerConfig("the_server.com", 5349, "123", "abc",
                                         cricket::PROTO_TCP,
                                         true) == config.turn_servers[4]);
  EXPECT_TRUE(cricket::RelayServerConfig("the_server.com", 5349, "123", "abc",
                                         cricket::PROTO_UDP,
                                         true) == config.turn_servers[5]);

  EXPECT_EQ(2U, config.stun_servers.size());
  EXPECT_EQ(rtc::SocketAddress("stun_server.com", 18344),
            config.stun_servers[0]);
  EXPECT_EQ(rtc::SocketAddress("1.2.3.4", 3478), config.stun_servers[1]);
}

TEST(IceConfigTest, ParseDataEnvelope) {
  const char kTestConfigJson[] =
      "{\"data\":{"
      "  \"lifetimeDuration\": \"43200.000s\","
      "  \"iceServers\": ["
      "    {"
      "      \"urls\": ["
      "        \"stun:1.2.3.4\""
      "      ]"
      "    }"
      "  ]"
      "}}";

  IceConfig config = IceConfig::Parse(kTestConfigJson);

  EXPECT_EQ(1U, config.stun_servers.size());
  EXPECT_EQ(rtc::SocketAddress("1.2.3.4", 3478), config.stun_servers[0]);
}

// Verify that we can still proceed if some servers cannot be parsed.
TEST(IceConfigTest, ParsePartiallyInvalid) {
  const char kTestConfigJson[] =
      "{"
      "  \"lifetimeDuration\": \"43200.000s\","
      "  \"iceServers\": ["
      "    {"
      "      \"urls\": ["
      "        \"InvalidURL\","
      "        \"turn:[2001:4860:4860::8888]:333\""
      "      ],"
      "      \"username\": \"123\","
      "      \"credential\": \"abc\""
      "    },"
      "    \"42\""
      "  ]"
      "}";

  IceConfig config = IceConfig::Parse(kTestConfigJson);

  // Config should be already expired because it couldn't be parsed.
  EXPECT_TRUE(config.expiration_time <= base::Time::Now());

  EXPECT_EQ(1U, config.turn_servers.size());
  EXPECT_TRUE(cricket::RelayServerConfig("2001:4860:4860::8888", 333, "123",
                                         "abc", cricket::PROTO_UDP,
                                         false) == config.turn_servers[0]);
}

TEST(IceConfigTest, NotParseable) {
  IceConfig config = IceConfig::Parse("Invalid Ice Config");
  EXPECT_TRUE(config.is_null());
}

}  // namespace protocol
}  // namespace remoting
