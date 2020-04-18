// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/remote_media_stream_track_adapter.h"

#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/webrtc/media_stream_remote_video_source.h"
#include "content/renderer/media/webrtc/peer_connection_remote_audio_source.h"
#include "content/renderer/media/webrtc/track_observer.h"

namespace content {

RemoteVideoTrackAdapter::RemoteVideoTrackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
    webrtc::VideoTrackInterface* webrtc_track)
    : RemoteMediaStreamTrackAdapter(main_thread, webrtc_track) {
  std::unique_ptr<TrackObserver> observer(
      new TrackObserver(main_thread, observed_track().get()));
  // Here, we use base::Unretained() to avoid a circular reference.
  web_initialize_ = base::Bind(
      &RemoteVideoTrackAdapter::InitializeWebVideoTrack, base::Unretained(this),
      base::Passed(&observer), observed_track()->enabled());
}

RemoteVideoTrackAdapter::~RemoteVideoTrackAdapter() {
  DCHECK(main_thread_->BelongsToCurrentThread());
  if (initialized()) {
    static_cast<MediaStreamRemoteVideoSource*>(
        web_track()->Source().GetExtraData())
        ->OnSourceTerminated();
  }
}

void RemoteVideoTrackAdapter::InitializeWebVideoTrack(
    std::unique_ptr<TrackObserver> observer,
    bool enabled) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  std::unique_ptr<MediaStreamRemoteVideoSource> video_source(
      new MediaStreamRemoteVideoSource(std::move(observer)));
  InitializeWebTrack(blink::WebMediaStreamSource::kTypeVideo);
  web_track()->Source().SetExtraData(video_source.get());

  blink::WebMediaStreamSource::Capabilities capabilities;
  capabilities.device_id = blink::WebString::FromUTF8(id());
  web_track()->Source().SetCapabilities(capabilities);

  MediaStreamVideoTrack* media_stream_track = new MediaStreamVideoTrack(
      video_source.release(), MediaStreamVideoSource::ConstraintsCallback(),
      enabled);
  web_track()->SetTrackData(media_stream_track);
}

RemoteAudioTrackAdapter::RemoteAudioTrackAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
    webrtc::AudioTrackInterface* webrtc_track)
    : RemoteMediaStreamTrackAdapter(main_thread, webrtc_track),
#if DCHECK_IS_ON()
      unregistered_(false),
#endif
      state_(observed_track()->state()) {
  // TODO(tommi): Use TrackObserver instead.
  observed_track()->RegisterObserver(this);
  // Here, we use base::Unretained() to avoid a circular reference.
  web_initialize_ =
      base::Bind(&RemoteAudioTrackAdapter::InitializeWebAudioTrack,
                 base::Unretained(this));
}

RemoteAudioTrackAdapter::~RemoteAudioTrackAdapter() {
#if DCHECK_IS_ON()
  DCHECK(unregistered_);
#endif
}

void RemoteAudioTrackAdapter::Unregister() {
#if DCHECK_IS_ON()
  DCHECK(!unregistered_);
  unregistered_ = true;
#endif
  observed_track()->UnregisterObserver(this);
}

void RemoteAudioTrackAdapter::InitializeWebAudioTrack() {
  InitializeWebTrack(blink::WebMediaStreamSource::kTypeAudio);

  MediaStreamAudioSource* const source =
      new PeerConnectionRemoteAudioSource(observed_track().get());
  web_track()->Source().SetExtraData(source);  // Takes ownership.

  blink::WebMediaStreamSource::Capabilities capabilities;
  capabilities.device_id = blink::WebString::FromUTF8(id());
  capabilities.echo_cancellation = std::vector<bool>({false});
  capabilities.auto_gain_control = std::vector<bool>({false});
  capabilities.noise_suppression = std::vector<bool>({false});
  web_track()->Source().SetCapabilities(capabilities);

  source->ConnectToTrack(*(web_track()));
}

void RemoteAudioTrackAdapter::OnChanged() {
  main_thread_->PostTask(
      FROM_HERE, base::BindOnce(&RemoteAudioTrackAdapter::OnChangedOnMainThread,
                                this, observed_track()->state()));
}

void RemoteAudioTrackAdapter::OnChangedOnMainThread(
    webrtc::MediaStreamTrackInterface::TrackState state) {
  DCHECK(main_thread_->BelongsToCurrentThread());

  if (state == state_ || !initialized())
    return;

  state_ = state;

  switch (state) {
    case webrtc::MediaStreamTrackInterface::kLive:
      web_track()->Source().SetReadyState(
          blink::WebMediaStreamSource::kReadyStateLive);
      break;
    case webrtc::MediaStreamTrackInterface::kEnded:
      web_track()->Source().SetReadyState(
          blink::WebMediaStreamSource::kReadyStateEnded);
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace content
