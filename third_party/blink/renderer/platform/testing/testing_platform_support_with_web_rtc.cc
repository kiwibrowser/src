// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_web_rtc.h"

#include <memory>
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_rtc_dtmf_sender_handler.h"
#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"
#include "third_party/blink/public/platform/web_rtc_rtp_sender.h"
#include "third_party/blink/public/platform/web_rtc_session_description.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

namespace {

class DummyWebRTCRtpSender : public WebRTCRtpSender {
 private:
  static uintptr_t last_id_;

 public:
  DummyWebRTCRtpSender() : id_(++last_id_) {}
  ~DummyWebRTCRtpSender() override {}

  uintptr_t Id() const override { return id_; }
  WebMediaStreamTrack Track() const override { return WebMediaStreamTrack(); }
  void ReplaceTrack(WebMediaStreamTrack, WebRTCVoidRequest) override {}
  std::unique_ptr<WebRTCDTMFSenderHandler> GetDtmfSender() const override {
    return nullptr;
  }
  std::unique_ptr<webrtc::RtpParameters> GetParameters() const override {
    return std::unique_ptr<webrtc::RtpParameters>();
  }
  void SetParameters(blink::WebVector<webrtc::RtpEncodingParameters>,
                     webrtc::DegradationPreference,
                     WebRTCVoidRequest) override {}
  void GetStats(std::unique_ptr<blink::WebRTCStatsReportCallback>) override {}

 private:
  const uintptr_t id_;
};

uintptr_t DummyWebRTCRtpSender::last_id_ = 0;

}  // namespace

MockWebRTCPeerConnectionHandler::MockWebRTCPeerConnectionHandler() = default;

MockWebRTCPeerConnectionHandler::~MockWebRTCPeerConnectionHandler() = default;

bool MockWebRTCPeerConnectionHandler::Initialize(const WebRTCConfiguration&,
                                                 const WebMediaConstraints&) {
  return true;
}

void MockWebRTCPeerConnectionHandler::CreateOffer(
    const WebRTCSessionDescriptionRequest&,
    const WebMediaConstraints&) {}

void MockWebRTCPeerConnectionHandler::CreateOffer(
    const WebRTCSessionDescriptionRequest&,
    const WebRTCOfferOptions&) {}

void MockWebRTCPeerConnectionHandler::CreateAnswer(
    const WebRTCSessionDescriptionRequest&,
    const WebMediaConstraints&) {}

void MockWebRTCPeerConnectionHandler::CreateAnswer(
    const WebRTCSessionDescriptionRequest&,
    const WebRTCAnswerOptions&) {}

void MockWebRTCPeerConnectionHandler::SetLocalDescription(
    const WebRTCVoidRequest&,
    const WebRTCSessionDescription&) {}

void MockWebRTCPeerConnectionHandler::SetRemoteDescription(
    const WebRTCVoidRequest&,
    const WebRTCSessionDescription&) {}

WebRTCSessionDescription MockWebRTCPeerConnectionHandler::LocalDescription() {
  return WebRTCSessionDescription();
}

WebRTCSessionDescription MockWebRTCPeerConnectionHandler::RemoteDescription() {
  return WebRTCSessionDescription();
}

webrtc::RTCErrorType MockWebRTCPeerConnectionHandler::SetConfiguration(
    const WebRTCConfiguration&) {
  return webrtc::RTCErrorType::NONE;
}

void MockWebRTCPeerConnectionHandler::GetStats(const WebRTCStatsRequest&) {}

void MockWebRTCPeerConnectionHandler::GetStats(
    std::unique_ptr<WebRTCStatsReportCallback>) {}

std::unique_ptr<WebRTCRtpSender> MockWebRTCPeerConnectionHandler::AddTrack(
    const WebMediaStreamTrack&,
    const WebVector<WebMediaStream>&) {
  return std::make_unique<DummyWebRTCRtpSender>();
}

bool MockWebRTCPeerConnectionHandler::RemoveTrack(WebRTCRtpSender*) {
  return true;
}

WebRTCDataChannelHandler* MockWebRTCPeerConnectionHandler::CreateDataChannel(
    const WebString& label,
    const WebRTCDataChannelInit&) {
  return nullptr;
}

void MockWebRTCPeerConnectionHandler::Stop() {}

WebString MockWebRTCPeerConnectionHandler::Id() const {
  return WebString();
}

std::unique_ptr<WebRTCPeerConnectionHandler>
TestingPlatformSupportWithWebRTC::CreateRTCPeerConnectionHandler(
    WebRTCPeerConnectionHandlerClient*,
    scoped_refptr<base::SingleThreadTaskRunner>) {
  return std::make_unique<MockWebRTCPeerConnectionHandler>();
}

}  // namespace blink
