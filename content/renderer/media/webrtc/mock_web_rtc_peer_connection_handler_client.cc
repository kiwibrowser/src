// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/renderer/media/webrtc/mock_web_rtc_peer_connection_handler_client.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"
#include "third_party/blink/public/platform/web_string.h"

using testing::_;

namespace content {

MockWebRTCPeerConnectionHandlerClient::
MockWebRTCPeerConnectionHandlerClient()
    : candidate_mline_index_(-1) {
  ON_CALL(*this, DidGenerateICECandidate(_))
      .WillByDefault(
          testing::Invoke(this, &MockWebRTCPeerConnectionHandlerClient::
                                    didGenerateICECandidateWorker));
  ON_CALL(*this, DidAddRemoteTrackForMock(_))
      .WillByDefault(testing::Invoke(
          this,
          &MockWebRTCPeerConnectionHandlerClient::didAddRemoteTrackWorker));
  ON_CALL(*this, DidRemoveRemoteTrackForMock(_))
      .WillByDefault(testing::Invoke(
          this,
          &MockWebRTCPeerConnectionHandlerClient::didRemoveRemoteTrackWorker));
}

MockWebRTCPeerConnectionHandlerClient::
~MockWebRTCPeerConnectionHandlerClient() {}

void MockWebRTCPeerConnectionHandlerClient::didGenerateICECandidateWorker(
    scoped_refptr<blink::WebRTCICECandidate> candidate) {
  candidate_sdp_ = candidate->Candidate().Utf8();
  candidate_mline_index_ = candidate->SdpMLineIndex();
  candidate_mid_ = candidate->SdpMid().Utf8();
}

void MockWebRTCPeerConnectionHandlerClient::didAddRemoteTrackWorker(
    std::unique_ptr<blink::WebRTCRtpReceiver>* web_rtp_receiver) {
  blink::WebVector<blink::WebMediaStream> web_streams =
      (*web_rtp_receiver)->Streams();
  DCHECK_EQ(1u, web_streams.size());
  remote_stream_ = web_streams[0];
}

void MockWebRTCPeerConnectionHandlerClient::didRemoveRemoteTrackWorker(
    std::unique_ptr<blink::WebRTCRtpReceiver>* web_rtp_receiver) {
  remote_stream_.Reset();
}

}  // namespace content
