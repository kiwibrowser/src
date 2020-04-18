// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_uma_histograms.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace content {

class MockPerSessionWebRTCAPIMetrics : public PerSessionWebRTCAPIMetrics {
 public:
  MockPerSessionWebRTCAPIMetrics() {}

  using PerSessionWebRTCAPIMetrics::LogUsageOnlyOnce;

  MOCK_METHOD1(LogUsage, void(blink::WebRTCAPIName));
};

TEST(PerSessionWebRTCAPIMetrics, NoCallOngoingGetUserMedia) {
  MockPerSessionWebRTCAPIMetrics metrics;
  EXPECT_CALL(metrics, LogUsage(_)).Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kGetUserMedia);
}

TEST(PerSessionWebRTCAPIMetrics, CallOngoingGetUserMedia) {
  MockPerSessionWebRTCAPIMetrics metrics;
  metrics.IncrementStreamCounter();
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kGetUserMedia)).Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kGetUserMedia);
}

TEST(PerSessionWebRTCAPIMetrics, NoCallOngoingGetMediaDevices) {
  MockPerSessionWebRTCAPIMetrics metrics;
  EXPECT_CALL(metrics, LogUsage(_)).Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kEnumerateDevices);
}

TEST(PerSessionWebRTCAPIMetrics, CallOngoingGetMediaDevices) {
  MockPerSessionWebRTCAPIMetrics metrics;
  metrics.IncrementStreamCounter();
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kEnumerateDevices))
      .Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kEnumerateDevices);
}

TEST(PerSessionWebRTCAPIMetrics, NoCallOngoingRTCPeerConnection) {
  MockPerSessionWebRTCAPIMetrics metrics;
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kRTCPeerConnection));
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
}

TEST(PerSessionWebRTCAPIMetrics, NoCallOngoingMultiplePC) {
  MockPerSessionWebRTCAPIMetrics metrics;
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kRTCPeerConnection))
      .Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
}

TEST(PerSessionWebRTCAPIMetrics, BeforeAfterCallMultiplePC) {
  MockPerSessionWebRTCAPIMetrics metrics;
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kRTCPeerConnection))
      .Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.IncrementStreamCounter();
  metrics.IncrementStreamCounter();
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.DecrementStreamCounter();
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.DecrementStreamCounter();
  EXPECT_CALL(metrics, LogUsage(blink::WebRTCAPIName::kRTCPeerConnection))
      .Times(1);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
  metrics.LogUsageOnlyOnce(blink::WebRTCAPIName::kRTCPeerConnection);
}

}  // namespace content
