// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TESTING_PLATFORM_SUPPORT_WITH_WEB_RTC_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TESTING_PLATFORM_SUPPORT_WITH_WEB_RTC_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"

namespace blink {

class MockWebRTCPeerConnectionHandler : public WebRTCPeerConnectionHandler {
 public:
  MockWebRTCPeerConnectionHandler();
  ~MockWebRTCPeerConnectionHandler() override;

  bool Initialize(const WebRTCConfiguration&,
                  const WebMediaConstraints&) override;

  void CreateOffer(const WebRTCSessionDescriptionRequest&,
                   const WebMediaConstraints&) override;
  void CreateOffer(const WebRTCSessionDescriptionRequest&,
                   const WebRTCOfferOptions&) override;
  void CreateAnswer(const WebRTCSessionDescriptionRequest&,
                    const WebMediaConstraints&) override;
  void CreateAnswer(const WebRTCSessionDescriptionRequest&,
                    const WebRTCAnswerOptions&) override;
  void SetLocalDescription(const WebRTCVoidRequest&,
                           const WebRTCSessionDescription&) override;
  void SetRemoteDescription(const WebRTCVoidRequest&,
                            const WebRTCSessionDescription&) override;
  WebRTCSessionDescription LocalDescription() override;
  WebRTCSessionDescription RemoteDescription() override;
  webrtc::RTCErrorType SetConfiguration(const WebRTCConfiguration&) override;
  void GetStats(const WebRTCStatsRequest&) override;
  void GetStats(std::unique_ptr<WebRTCStatsReportCallback>) override;
  std::unique_ptr<WebRTCRtpSender> AddTrack(
      const WebMediaStreamTrack&,
      const WebVector<WebMediaStream>&) override;
  bool RemoveTrack(WebRTCRtpSender*) override;
  WebRTCDataChannelHandler* CreateDataChannel(
      const WebString& label,
      const WebRTCDataChannelInit&) override;
  void Stop() override;
  WebString Id() const override;
};

class TestingPlatformSupportWithWebRTC : public TestingPlatformSupport {
 public:
  std::unique_ptr<WebRTCPeerConnectionHandler> CreateRTCPeerConnectionHandler(
      WebRTCPeerConnectionHandlerClient*,
      scoped_refptr<base::SingleThreadTaskRunner>) override;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TESTING_PLATFORM_SUPPORT_WITH_WEB_RTC_H_
