// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/client_hints/client_hints.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

class ClientHintsTest : public testing::Test {
 public:
  ClientHintsTest() {}

  ~ClientHintsTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ClientHintsTest);
};

TEST_F(ClientHintsTest, RttRoundedOff) {
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(1023)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(6787)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(12)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "foo.com", base::TimeDelta::FromMilliseconds(1023)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "foo.com", base::TimeDelta::FromMilliseconds(1193)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "foo.com", base::TimeDelta::FromMilliseconds(12)) %
                    50);
}

TEST_F(ClientHintsTest, DownlinkRoundedOff) {
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 102) * 1000) %
                   50);
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 12) * 1000) %
                   50);
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 2102) * 1000) %
                   50);

  EXPECT_GE(
      1, static_cast<int>(
             client_hints::internal::RoundKbpsToMbps("foo.com", 102) * 1000) %
             50);
  EXPECT_GE(1,
            static_cast<int>(
                client_hints::internal::RoundKbpsToMbps("foo.com", 12) * 1000) %
                50);
  EXPECT_GE(
      1, static_cast<int>(
             client_hints::internal::RoundKbpsToMbps("foo.com", 2102) * 1000) %
             50);
  EXPECT_GE(
      1, static_cast<int>(
             client_hints::internal::RoundKbpsToMbps("foo.com", 12102) * 1000) %
             50);
}

// Verify that the value of RTT after adding noise is within approximately 10%
// of the original value. Note that the difference between the final value of
// RTT and the original value may be slightly more than 10% due to rounding off.
// To handle that, the maximum absolute difference allowed is set to a value
// slightly larger than 10% of the original metric value.
TEST_F(ClientHintsTest, FinalRttWithin10PercentValue) {
  EXPECT_NEAR(98,
              client_hints::internal::RoundRtt(
                  "", base::TimeDelta::FromMilliseconds(98)),
              100);
  EXPECT_NEAR(1023,
              client_hints::internal::RoundRtt(
                  "", base::TimeDelta::FromMilliseconds(1023)),
              200);
  EXPECT_NEAR(1193,
              client_hints::internal::RoundRtt(
                  "", base::TimeDelta::FromMilliseconds(1193)),
              200);
  EXPECT_NEAR(2750,
              client_hints::internal::RoundRtt(
                  "", base::TimeDelta::FromMilliseconds(2750)),
              400);
}

// Verify that the value of downlink after adding noise is within approximately
// 10% of the original value. Note that the difference between the final value
// of downlink and the original value may be slightly more than 10% due to
// rounding off. To handle that, the maximum absolute difference allowed is set
// to a value slightly larger than 10% of the original metric value.
TEST_F(ClientHintsTest, FinalDownlinkWithin10PercentValue) {
  EXPECT_NEAR(0.098, client_hints::internal::RoundKbpsToMbps("", 98), 0.1);
  EXPECT_NEAR(1.023, client_hints::internal::RoundKbpsToMbps("", 1023), 0.2);
  EXPECT_NEAR(1.193, client_hints::internal::RoundKbpsToMbps("", 1193), 0.2);
  EXPECT_NEAR(7.523, client_hints::internal::RoundKbpsToMbps("", 7523), 0.9);
  EXPECT_NEAR(9.999, client_hints::internal::RoundKbpsToMbps("", 9999), 1.2);
}

TEST_F(ClientHintsTest, RttMaxValue) {
  EXPECT_GE(3000u, client_hints::internal::RoundRtt(
                       "", base::TimeDelta::FromMilliseconds(1023)));
  EXPECT_GE(3000u, client_hints::internal::RoundRtt(
                       "", base::TimeDelta::FromMilliseconds(2789)));
  EXPECT_GE(3000u, client_hints::internal::RoundRtt(
                       "", base::TimeDelta::FromMilliseconds(6023)));
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(1023)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(2789)) %
                    50);
  EXPECT_EQ(0u, client_hints::internal::RoundRtt(
                    "", base::TimeDelta::FromMilliseconds(6023)) %
                    50);
}

TEST_F(ClientHintsTest, DownlinkMaxValue) {
  EXPECT_GE(10.0, client_hints::internal::RoundKbpsToMbps("", 102));
  EXPECT_GE(10.0, client_hints::internal::RoundKbpsToMbps("", 2102));
  EXPECT_GE(10.0, client_hints::internal::RoundKbpsToMbps("", 100102));
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 102) * 1000) %
                   50);
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 2102) * 1000) %
                   50);
  EXPECT_GE(1, static_cast<int>(
                   client_hints::internal::RoundKbpsToMbps("", 100102) * 1000) %
                   50);
}

TEST_F(ClientHintsTest, RttRandomized) {
  const int initial_value = client_hints::internal::RoundRtt(
      "example.com", base::TimeDelta::FromMilliseconds(1023));
  bool network_quality_randomized_by_host = false;
  // There is a 1/20 chance that the same random noise is selected for two
  // different hosts. Run this test across 20 hosts to reduce the chances of
  // test failing to (1/20)^20.
  for (size_t i = 0; i < 20; ++i) {
    int value = client_hints::internal::RoundRtt(
        base::IntToString(i), base::TimeDelta::FromMilliseconds(1023));
    // If |value| is different than |initial_value|, it implies that RTT is
    // randomized by host. This verifies the behavior, and test can be ended.
    if (value != initial_value)
      network_quality_randomized_by_host = true;
  }
  EXPECT_TRUE(network_quality_randomized_by_host);

  // Calling RoundRtt for same host should return the same result.
  for (size_t i = 0; i < 20; ++i) {
    int value = client_hints::internal::RoundRtt(
        "example.com", base::TimeDelta::FromMilliseconds(1023));
    EXPECT_EQ(initial_value, value);
  }
}

TEST_F(ClientHintsTest, DownlinkRandomized) {
  const int initial_value =
      client_hints::internal::RoundKbpsToMbps("example.com", 1023);
  bool network_quality_randomized_by_host = false;
  // There is a 1/20 chance that the same random noise is selected for two
  // different hosts. Run this test across 20 hosts to reduce the chances of
  // test failing to (1/20)^20.
  for (size_t i = 0; i < 20; ++i) {
    int value =
        client_hints::internal::RoundKbpsToMbps(base::IntToString(i), 1023);
    // If |value| is different than |initial_value|, it implies that downlink is
    // randomized by host. This verifies the behavior, and test can be ended.
    if (value != initial_value)
      network_quality_randomized_by_host = true;
  }
  EXPECT_TRUE(network_quality_randomized_by_host);

  // Calling RoundMbps for same host should return the same result.
  for (size_t i = 0; i < 20; ++i) {
    int value = client_hints::internal::RoundKbpsToMbps("example.com", 1023);
    EXPECT_EQ(initial_value, value);
  }
}
