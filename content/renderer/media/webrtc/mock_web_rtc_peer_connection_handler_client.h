// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_WEB_RTC_PEER_CONNECTION_HANDLER_CLIENT_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_WEB_RTC_PEER_CONNECTION_HANDLER_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_rtc_ice_candidate.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler_client.h"
#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"

namespace content {

class MockWebRTCPeerConnectionHandlerClient
    : public blink::WebRTCPeerConnectionHandlerClient {
 public:
  MockWebRTCPeerConnectionHandlerClient();
  ~MockWebRTCPeerConnectionHandlerClient() override;

  // WebRTCPeerConnectionHandlerClient implementation.
  MOCK_METHOD0(NegotiationNeeded, void());
  MOCK_METHOD1(DidGenerateICECandidate,
               void(scoped_refptr<blink::WebRTCICECandidate> candidate));
  MOCK_METHOD1(DidChangeSignalingState, void(SignalingState state));
  MOCK_METHOD1(DidChangeICEGatheringState, void(ICEGatheringState state));
  MOCK_METHOD1(DidChangeICEConnectionState, void(ICEConnectionState state));
  void DidAddRemoteTrack(
      std::unique_ptr<blink::WebRTCRtpReceiver> web_rtp_receiver) override {
    DidAddRemoteTrackForMock(&web_rtp_receiver);
  }
  void DidRemoveRemoteTrack(
      std::unique_ptr<blink::WebRTCRtpReceiver> web_rtp_receiver) override {
    DidRemoveRemoteTrackForMock(&web_rtp_receiver);
  }
  MOCK_METHOD1(DidAddRemoteDataChannel, void(blink::WebRTCDataChannelHandler*));
  MOCK_METHOD0(ReleasePeerConnectionHandler, void());
  MOCK_METHOD0(GetOriginTrials, WebRTCOriginTrials());

  // Move-only arguments do not play nicely with MOCK, the workaround is to
  // EXPECT_CALL with these instead.
  MOCK_METHOD1(DidAddRemoteTrackForMock,
               void(std::unique_ptr<blink::WebRTCRtpReceiver>*));
  MOCK_METHOD1(DidRemoveRemoteTrackForMock,
               void(std::unique_ptr<blink::WebRTCRtpReceiver>*));

  void didGenerateICECandidateWorker(
      scoped_refptr<blink::WebRTCICECandidate> candidate);
  void didAddRemoteTrackWorker(
      std::unique_ptr<blink::WebRTCRtpReceiver>* stream_web_rtp_receivers);
  void didRemoveRemoteTrackWorker(
      std::unique_ptr<blink::WebRTCRtpReceiver>* stream_web_rtp_receivers);

  const std::string& candidate_sdp() const { return candidate_sdp_; }
  int candidate_mlineindex() const {
    return candidate_mline_index_;
  }
  const std::string& candidate_mid() const { return candidate_mid_ ; }
  const blink::WebMediaStream& remote_stream() const { return remote_stream_; }

 private:
  blink::WebMediaStream remote_stream_;
  std::string candidate_sdp_;
  int candidate_mline_index_;
  std::string candidate_mid_;

  DISALLOW_COPY_AND_ASSIGN(MockWebRTCPeerConnectionHandlerClient);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_WEB_RTC_PEER_CONNECTION_HANDLER_CLIENT_H_
