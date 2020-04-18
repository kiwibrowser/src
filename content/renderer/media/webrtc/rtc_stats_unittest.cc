// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_stats.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/api/stats/rtcstats_objects.h"
#include "third_party/webrtc/api/stats/rtcstatsreport.h"
#include "third_party/webrtc/stats/test/rtcteststats.h"

namespace content {

TEST(RTCStatsTest, OnlyIncludeWhitelistedStats_GetStats) {
  const char* not_whitelisted_id = "NotWhitelistedId";
  const char* whitelisted_id = "WhitelistedId";

  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(42);
  webrtc_report->AddStats(std::unique_ptr<webrtc::RTCTestStats>(
      new webrtc::RTCTestStats(not_whitelisted_id, 42)));
  webrtc_report->AddStats(std::unique_ptr<webrtc::RTCPeerConnectionStats>(
      new webrtc::RTCPeerConnectionStats(whitelisted_id, 42)));

  RTCStatsReport report(webrtc_report.get());
  EXPECT_FALSE(report.GetStats(blink::WebString::FromUTF8(not_whitelisted_id)));
  EXPECT_TRUE(report.GetStats(blink::WebString::FromUTF8(whitelisted_id)));
}

TEST(RTCStatsTest, OnlyIncludeWhitelistedStats_Iteration) {
  const char* not_whitelisted_id = "NotWhitelistedId";
  const char* whitelisted_id = "WhitelistedId";

  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(42);
  webrtc_report->AddStats(std::unique_ptr<webrtc::RTCTestStats>(
      new webrtc::RTCTestStats(not_whitelisted_id, 42)));
  webrtc_report->AddStats(std::unique_ptr<webrtc::RTCPeerConnectionStats>(
      new webrtc::RTCPeerConnectionStats(whitelisted_id, 42)));

  RTCStatsReport report(webrtc_report.get());
  std::unique_ptr<blink::WebRTCStats> stats = report.Next();
  EXPECT_TRUE(stats);
  EXPECT_EQ(stats->Id(), whitelisted_id);
  EXPECT_FALSE(report.Next());
}

}  // namespace content
