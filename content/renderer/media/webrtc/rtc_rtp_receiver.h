// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_RECEIVER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_RECEIVER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"
#include "third_party/webrtc/api/mediastreaminterface.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"
#include "third_party/webrtc/api/rtpreceiverinterface.h"

namespace content {

// Used to surface |webrtc::RtpReceiverInterface| to blink. Multiple
// |RTCRtpReceiver|s could reference the same webrtc receiver; |id| is the value
// of the pointer to the webrtc receiver.
class CONTENT_EXPORT RTCRtpReceiver : public blink::WebRTCRtpReceiver {
 public:
  static uintptr_t getId(
      const webrtc::RtpReceiverInterface* webrtc_rtp_receiver);

  RTCRtpReceiver(
      scoped_refptr<webrtc::PeerConnectionInterface> native_peer_connection,
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<base::SingleThreadTaskRunner> signaling_thread,
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> webrtc_receiver,
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>
          track_adapter,
      std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
          stream_adapter_refs);
  RTCRtpReceiver(const RTCRtpReceiver& other);
  ~RTCRtpReceiver() override;

  RTCRtpReceiver& operator=(const RTCRtpReceiver& other);

  // Creates a shallow copy of the receiver, representing the same underlying
  // webrtc receiver as the original.
  std::unique_ptr<RTCRtpReceiver> ShallowCopy() const;

  uintptr_t Id() const override;
  const blink::WebMediaStreamTrack& Track() const override;
  blink::WebVector<blink::WebMediaStream> Streams() const override;
  blink::WebVector<std::unique_ptr<blink::WebRTCRtpContributingSource>>
  GetSources() override;
  void GetStats(std::unique_ptr<blink::WebRTCStatsReportCallback>) override;

  webrtc::RtpReceiverInterface* webrtc_receiver() const;
  const webrtc::MediaStreamTrackInterface& webrtc_track() const;
  bool HasStream(const webrtc::MediaStreamInterface* webrtc_stream) const;
  std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
  StreamAdapterRefs() const;

 private:
  class RTCRtpReceiverInternal;
  struct RTCRtpReceiverInternalTraits;

  scoped_refptr<RTCRtpReceiverInternal> internal_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_RECEIVER_H_
