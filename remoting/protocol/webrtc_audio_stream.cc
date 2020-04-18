// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_audio_stream.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "remoting/base/constants.h"
#include "remoting/protocol/audio_source.h"
#include "remoting/protocol/webrtc_audio_source_adapter.h"
#include "remoting/protocol/webrtc_transport.h"
#include "third_party/webrtc/api/mediastreaminterface.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"
#include "third_party/webrtc/rtc_base/refcount.h"

namespace remoting {
namespace protocol {

const char kAudioStreamLabel[] = "audio_stream";
const char kAudioTrackLabel[] = "system_audio";

WebrtcAudioStream::WebrtcAudioStream() = default;

WebrtcAudioStream::~WebrtcAudioStream() {
  if (stream_) {
    for (const auto& track : stream_->GetAudioTracks()) {
      stream_->RemoveTrack(track.get());
    }
    peer_connection_->RemoveStream(stream_.get());
  }
}

void WebrtcAudioStream::Start(
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<AudioSource> audio_source,
    WebrtcTransport* webrtc_transport) {
  DCHECK(webrtc_transport);

  source_adapter_ =
      new rtc::RefCountedObject<WebrtcAudioSourceAdapter>(audio_task_runner);
  source_adapter_->Start(std::move(audio_source));

  scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory(
      webrtc_transport->peer_connection_factory());
  peer_connection_ = webrtc_transport->peer_connection();

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track =
      peer_connection_factory->CreateAudioTrack(kAudioTrackLabel,
                                                source_adapter_.get());

  stream_ = peer_connection_factory->CreateLocalMediaStream(kAudioStreamLabel);

  // AddTrack() may fail only if there is another track with the same name,
  // which is impossible because it's a brand new stream.
  bool result = stream_->AddTrack(audio_track.get());
  DCHECK(result);

  // AddStream() may fail if there is another stream with the same name or when
  // the PeerConnection is closed, neither is expected.
  result = peer_connection_->AddStream(stream_.get());
  DCHECK(result);
}

void WebrtcAudioStream::Pause(bool pause) {
  source_adapter_->Pause(pause);
}

}  // namespace protocol
}  // namespace remoting
