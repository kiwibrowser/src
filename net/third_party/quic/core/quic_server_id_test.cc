// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/core/quic_server_id.h"

#include "net/third_party/quic/platform/api/quic_estimate_memory_usage.h"
#include "net/third_party/quic/platform/api/quic_string.h"
#include "net/third_party/quic/platform/api/quic_test.h"

namespace net {

namespace {

class QuicServerIdTest : public QuicTest {};

TEST_F(QuicServerIdTest, ToString) {
  HostPortPair google_host_port_pair("google.com", 10);

  QuicServerId google_server_id(google_host_port_pair, PRIVACY_MODE_DISABLED);
  QuicString google_server_id_str = google_server_id.ToString();
  EXPECT_EQ("https://google.com:10", google_server_id_str);

  QuicServerId private_server_id(google_host_port_pair, PRIVACY_MODE_ENABLED);
  QuicString private_server_id_str = private_server_id.ToString();
  EXPECT_EQ("https://google.com:10/private", private_server_id_str);
}

TEST_F(QuicServerIdTest, HostPortPair) {
  HostPortPair google_host_port_pair("google.com", 10);

  QuicServerId google_server_id(google_host_port_pair, PRIVACY_MODE_DISABLED);
  EXPECT_TRUE(google_host_port_pair.Equals(google_server_id.host_port_pair()));

  QuicServerId private_server_id(google_host_port_pair, PRIVACY_MODE_ENABLED);
  EXPECT_TRUE(google_host_port_pair.Equals(private_server_id.host_port_pair()));
}

TEST_F(QuicServerIdTest, LessThan) {
  QuicServerId a_10_https(HostPortPair("a.com", 10), PRIVACY_MODE_DISABLED);
  QuicServerId a_11_https(HostPortPair("a.com", 11), PRIVACY_MODE_DISABLED);
  QuicServerId b_10_https(HostPortPair("b.com", 10), PRIVACY_MODE_DISABLED);
  QuicServerId b_11_https(HostPortPair("b.com", 11), PRIVACY_MODE_DISABLED);

  QuicServerId a_10_https_private(HostPortPair("a.com", 10),
                                  PRIVACY_MODE_ENABLED);
  QuicServerId a_11_https_private(HostPortPair("a.com", 11),
                                  PRIVACY_MODE_ENABLED);
  QuicServerId b_10_https_private(HostPortPair("b.com", 10),
                                  PRIVACY_MODE_ENABLED);
  QuicServerId b_11_https_private(HostPortPair("b.com", 11),
                                  PRIVACY_MODE_ENABLED);

  // Test combinations of host, port, and privacy being same on left and
  // right side of less than.
  EXPECT_FALSE(a_10_https < a_10_https);
  EXPECT_TRUE(a_10_https < a_10_https_private);
  EXPECT_FALSE(a_10_https_private < a_10_https);
  EXPECT_FALSE(a_10_https_private < a_10_https_private);

  // Test with either host, port or https being different on left and right side
  // of less than.
  PrivacyMode left_privacy;
  PrivacyMode right_privacy;
  for (int i = 0; i < 4; i++) {
    left_privacy = static_cast<PrivacyMode>(i / 2);
    right_privacy = static_cast<PrivacyMode>(i % 2);
    QuicServerId a_10_https_left_private(HostPortPair("a.com", 10),
                                         left_privacy);
    QuicServerId a_10_https_right_private(HostPortPair("a.com", 10),
                                          right_privacy);
    QuicServerId a_11_https_left_private(HostPortPair("a.com", 11),
                                         left_privacy);
    QuicServerId a_11_https_right_private(HostPortPair("a.com", 11),
                                          right_privacy);

    QuicServerId b_10_https_left_private(HostPortPair("b.com", 10),
                                         left_privacy);
    QuicServerId b_10_https_right_private(HostPortPair("b.com", 10),
                                          right_privacy);
    QuicServerId b_11_https_left_private(HostPortPair("b.com", 11),
                                         left_privacy);
    QuicServerId b_11_https_right_private(HostPortPair("b.com", 11),
                                          right_privacy);

    EXPECT_TRUE(a_10_https_left_private < a_11_https_right_private);
    EXPECT_TRUE(a_10_https_left_private < b_10_https_right_private);
    EXPECT_TRUE(a_10_https_left_private < b_11_https_right_private);
    EXPECT_FALSE(a_11_https_left_private < a_10_https_right_private);
    EXPECT_FALSE(a_11_https_left_private < b_10_https_right_private);
    EXPECT_TRUE(a_11_https_left_private < b_11_https_right_private);
    EXPECT_FALSE(b_10_https_left_private < a_10_https_right_private);
    EXPECT_TRUE(b_10_https_left_private < a_11_https_right_private);
    EXPECT_TRUE(b_10_https_left_private < b_11_https_right_private);
    EXPECT_FALSE(b_11_https_left_private < a_10_https_right_private);
    EXPECT_FALSE(b_11_https_left_private < a_11_https_right_private);
    EXPECT_FALSE(b_11_https_left_private < b_10_https_right_private);
  }
}

TEST_F(QuicServerIdTest, Equals) {
  PrivacyMode left_privacy;
  PrivacyMode right_privacy;
  for (int i = 0; i < 2; i++) {
    left_privacy = right_privacy = static_cast<PrivacyMode>(i);
    QuicServerId a_10_https_right_private(HostPortPair("a.com", 10),
                                          right_privacy);
    QuicServerId a_11_https_right_private(HostPortPair("a.com", 11),
                                          right_privacy);
    QuicServerId b_10_https_right_private(HostPortPair("b.com", 10),
                                          right_privacy);
    QuicServerId b_11_https_right_private(HostPortPair("b.com", 11),
                                          right_privacy);

    QuicServerId new_a_10_https_left_private(HostPortPair("a.com", 10),
                                             left_privacy);
    QuicServerId new_a_11_https_left_private(HostPortPair("a.com", 11),
                                             left_privacy);
    QuicServerId new_b_10_https_left_private(HostPortPair("b.com", 10),
                                             left_privacy);
    QuicServerId new_b_11_https_left_private(HostPortPair("b.com", 11),
                                             left_privacy);

    EXPECT_EQ(new_a_10_https_left_private, a_10_https_right_private);
    EXPECT_EQ(new_a_11_https_left_private, a_11_https_right_private);
    EXPECT_EQ(new_b_10_https_left_private, b_10_https_right_private);
    EXPECT_EQ(new_b_11_https_left_private, b_11_https_right_private);
  }

  for (int i = 0; i < 2; i++) {
    right_privacy = static_cast<PrivacyMode>(i);
    QuicServerId a_10_https_right_private(HostPortPair("a.com", 10),
                                          right_privacy);
    QuicServerId a_11_https_right_private(HostPortPair("a.com", 11),
                                          right_privacy);
    QuicServerId b_10_https_right_private(HostPortPair("b.com", 10),
                                          right_privacy);
    QuicServerId b_11_https_right_private(HostPortPair("b.com", 11),
                                          right_privacy);

    QuicServerId new_a_10_https_left_private(HostPortPair("a.com", 10),
                                             PRIVACY_MODE_DISABLED);

    EXPECT_FALSE(new_a_10_https_left_private == a_11_https_right_private);
    EXPECT_FALSE(new_a_10_https_left_private == b_10_https_right_private);
    EXPECT_FALSE(new_a_10_https_left_private == b_11_https_right_private);
  }
  QuicServerId a_10_https_private(HostPortPair("a.com", 10),
                                  PRIVACY_MODE_ENABLED);
  QuicServerId new_a_10_https_no_private(HostPortPair("a.com", 10),
                                         PRIVACY_MODE_DISABLED);
  EXPECT_FALSE(new_a_10_https_no_private == a_10_https_private);
}

TEST_F(QuicServerIdTest, EstimateMemoryUsage) {
  HostPortPair host_port_pair("this is a rather very quite long hostname", 10);
  QuicServerId server_id(host_port_pair, PRIVACY_MODE_ENABLED);
  EXPECT_EQ(QuicEstimateMemoryUsage(host_port_pair),
            QuicEstimateMemoryUsage(server_id));
}

}  // namespace

}  // namespace net
