// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code if governed by a BSD-style license that can be
// found in LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/scheduler/public/page_scheduler.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_web_rtc.h"

using testing::_;

namespace blink {

class ActiveConnectionThrottlingTest : public SimTest {};

TEST_F(ActiveConnectionThrottlingTest, WebSocketStopsThrottling) {
  SimRequest main_resource("https://example.com/", "text/html");

  LoadURL("https://example.com/");

  EXPECT_FALSE(WebView().Scheduler()->HasActiveConnectionForTest());

  main_resource.Complete(
      "(<script>"
      "  var socket = new WebSocket(\"ws://www.example.com/websocket\");"
      "</script>)");

  EXPECT_TRUE(WebView().Scheduler()->HasActiveConnectionForTest());

  MainFrame().ExecuteScript(WebString("socket.close();"));

  EXPECT_FALSE(WebView().Scheduler()->HasActiveConnectionForTest());
}

TEST_F(ActiveConnectionThrottlingTest, WebRTCStopsThrottling) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithWebRTC> platform;

  SimRequest main_resource("https://example.com/", "text/html");

  LoadURL("https://example.com/");

  EXPECT_FALSE(WebView().Scheduler()->HasActiveConnectionForTest());

  main_resource.Complete(
      "(<script>"
      "  var data_channel = new RTCPeerConnection();"
      "</script>)");

  EXPECT_TRUE(WebView().Scheduler()->HasActiveConnectionForTest());

  MainFrame().ExecuteScript(WebString("data_channel.close();"));

  EXPECT_FALSE(WebView().Scheduler()->HasActiveConnectionForTest());
}

}  // namespace blink
