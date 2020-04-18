// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_media_stream_adapter.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "content/renderer/media/webrtc/peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace content {

// static
std::unique_ptr<WebRtcMediaStreamAdapter>
WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
    PeerConnectionDependencyFactory* factory,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
    const blink::WebMediaStream& web_stream) {
  return std::make_unique<LocalWebRtcMediaStreamAdapter>(
      factory, std::move(track_adapter_map), web_stream);
}

// static
std::unique_ptr<WebRtcMediaStreamAdapter>
WebRtcMediaStreamAdapter::CreateRemoteStreamAdapter(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
    scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream) {
  return std::make_unique<RemoteWebRtcMediaStreamAdapter>(
      std::move(main_thread), std::move(track_adapter_map),
      std::move(webrtc_stream));
}

WebRtcMediaStreamAdapter::WebRtcMediaStreamAdapter(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
    scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream,
    const blink::WebMediaStream& web_stream)
    : main_thread_(std::move(main_thread)),
      track_adapter_map_(std::move(track_adapter_map)),
      webrtc_stream_(std::move(webrtc_stream)),
      web_stream_(web_stream) {
  DCHECK(main_thread_);
  DCHECK(track_adapter_map_);
}

WebRtcMediaStreamAdapter::~WebRtcMediaStreamAdapter() {
  DCHECK(main_thread_->BelongsToCurrentThread());
}

LocalWebRtcMediaStreamAdapter::LocalWebRtcMediaStreamAdapter(
    PeerConnectionDependencyFactory* factory,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
    const blink::WebMediaStream& web_stream)
    : WebRtcMediaStreamAdapter(base::ThreadTaskRunnerHandle::Get(),
                               std::move(track_adapter_map),
                               nullptr,
                               web_stream),
      factory_(factory) {
  webrtc_stream_ = factory_->CreateLocalMediaStream(web_stream.Id().Utf8());

  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  web_stream_.AudioTracks(audio_tracks);
  for (blink::WebMediaStreamTrack& audio_track : audio_tracks)
    TrackAdded(audio_track);

  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  web_stream_.VideoTracks(video_tracks);
  for (blink::WebMediaStreamTrack& video_track : video_tracks)
    TrackAdded(video_track);

  web_stream_.AddObserver(this);
}

LocalWebRtcMediaStreamAdapter::~LocalWebRtcMediaStreamAdapter() {
  DCHECK(main_thread_->BelongsToCurrentThread());
  web_stream_.RemoveObserver(this);

  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  web_stream_.AudioTracks(audio_tracks);
  for (blink::WebMediaStreamTrack& audio_track : audio_tracks)
    TrackRemoved(audio_track);

  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  web_stream_.VideoTracks(video_tracks);
  for (blink::WebMediaStreamTrack& video_track : video_tracks)
    TrackRemoved(video_track);
}

bool LocalWebRtcMediaStreamAdapter::is_initialized() const {
  return true;
}

const scoped_refptr<webrtc::MediaStreamInterface>&
LocalWebRtcMediaStreamAdapter::webrtc_stream() const {
  return webrtc_stream_;
}

const blink::WebMediaStream& LocalWebRtcMediaStreamAdapter::web_stream() const {
  return web_stream_;
}

void LocalWebRtcMediaStreamAdapter::SetTracks(
    WebRtcMediaStreamAdapter::TrackAdapterRefs track_refs) {
  NOTIMPLEMENTED() << "Not supported for local stream adapters.";
}

void LocalWebRtcMediaStreamAdapter::TrackAdded(
    const blink::WebMediaStreamTrack& web_track) {
  DCHECK(adapter_refs_.find(web_track.UniqueId()) == adapter_refs_.end());
  bool is_audio_track =
      (web_track.Source().GetType() == blink::WebMediaStreamSource::kTypeAudio);
  if (is_audio_track && !MediaStreamAudioTrack::From(web_track)) {
    DLOG(ERROR) << "No native track for blink audio track.";
    return;
  }
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter_ref =
      track_adapter_map_->GetOrCreateLocalTrackAdapter(web_track);
  if (is_audio_track) {
    webrtc_stream_->AddTrack(
        static_cast<webrtc::AudioTrackInterface*>(adapter_ref->webrtc_track()));
  } else {
    webrtc_stream_->AddTrack(
        static_cast<webrtc::VideoTrackInterface*>(adapter_ref->webrtc_track()));
  }
  adapter_refs_.insert(
      std::make_pair(web_track.UniqueId(), std::move(adapter_ref)));
}

void LocalWebRtcMediaStreamAdapter::TrackRemoved(
    const blink::WebMediaStreamTrack& web_track) {
  auto it = adapter_refs_.find(web_track.UniqueId());
  if (it == adapter_refs_.end()) {
    // This can happen for audio tracks that don't have a source, these would
    // never be added in the first place.
    return;
  }
  if (web_track.Source().GetType() == blink::WebMediaStreamSource::kTypeAudio) {
    webrtc_stream_->RemoveTrack(
        static_cast<webrtc::AudioTrackInterface*>(it->second->webrtc_track()));
  } else {
    webrtc_stream_->RemoveTrack(
        static_cast<webrtc::VideoTrackInterface*>(it->second->webrtc_track()));
  }
  adapter_refs_.erase(it);
}

// static
bool RemoteWebRtcMediaStreamAdapter::RemoteAdapterRefsContainsTrack(
    const RemoteAdapterRefs& adapter_refs,
    webrtc::MediaStreamTrackInterface* webrtc_track) {
  for (const auto& adapter_ref : adapter_refs) {
    if (adapter_ref->webrtc_track() == webrtc_track)
      return true;
  }
  return false;
}

// static
RemoteWebRtcMediaStreamAdapter::RemoteAdapterRefs
RemoteWebRtcMediaStreamAdapter::GetRemoteAdapterRefsFromWebRtcStream(
    const scoped_refptr<WebRtcMediaStreamTrackAdapterMap>& track_adapter_map,
    webrtc::MediaStreamInterface* webrtc_stream) {
  // TODO(hbos): When adapter's |webrtc_track| can be called from any thread so
  // can RemoteAdapterRefsContainsTrack and we can DCHECK here that we don't end
  // up with duplicate entries. https://crbug.com/756436
  RemoteAdapterRefs adapter_refs;
  for (auto& webrtc_audio_track : webrtc_stream->GetAudioTracks()) {
    adapter_refs.push_back(track_adapter_map->GetOrCreateRemoteTrackAdapter(
        webrtc_audio_track.get()));
  }
  for (auto& webrtc_video_track : webrtc_stream->GetVideoTracks()) {
    adapter_refs.push_back(track_adapter_map->GetOrCreateRemoteTrackAdapter(
        webrtc_video_track.get()));
  }
  return adapter_refs;
}

RemoteWebRtcMediaStreamAdapter::RemoteWebRtcMediaStreamAdapter(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
    scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream)
    : WebRtcMediaStreamAdapter(std::move(main_thread),
                               std::move(track_adapter_map),
                               std::move(webrtc_stream),
                               // "null" |WebMediaStream|
                               blink::WebMediaStream()),
      is_initialized_(false),
      weak_factory_(this) {
  CHECK(!main_thread_->BelongsToCurrentThread());
  CHECK(track_adapter_map_);

  RemoteAdapterRefs adapter_refs = GetRemoteAdapterRefsFromWebRtcStream(
      track_adapter_map_, webrtc_stream_.get());
  main_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(&RemoteWebRtcMediaStreamAdapter::InitializeOnMainThread,
                     weak_factory_.GetWeakPtr(), webrtc_stream_->id(),
                     std::move(adapter_refs),
                     webrtc_stream_->GetAudioTracks().size(),
                     webrtc_stream_->GetVideoTracks().size()));
}

RemoteWebRtcMediaStreamAdapter::~RemoteWebRtcMediaStreamAdapter() {
  DCHECK(main_thread_->BelongsToCurrentThread());
  SetTracks(RemoteAdapterRefs());
}

bool RemoteWebRtcMediaStreamAdapter::is_initialized() const {
  base::AutoLock scoped_lock(lock_);
  return is_initialized_;
}

const scoped_refptr<webrtc::MediaStreamInterface>&
RemoteWebRtcMediaStreamAdapter::webrtc_stream() const {
  base::AutoLock scoped_lock(lock_);
  DCHECK(is_initialized_);
  return webrtc_stream_;
}

const blink::WebMediaStream& RemoteWebRtcMediaStreamAdapter::web_stream()
    const {
  base::AutoLock scoped_lock(lock_);
  DCHECK(is_initialized_);
  return web_stream_;
}

void RemoteWebRtcMediaStreamAdapter::InitializeOnMainThread(
    const std::string& label,
    RemoteAdapterRefs adapter_refs,
    size_t audio_track_count,
    size_t video_track_count) {
  CHECK(main_thread_->BelongsToCurrentThread());
  CHECK_EQ(audio_track_count + video_track_count, adapter_refs.size());

  adapter_refs_ = std::move(adapter_refs);
  blink::WebVector<blink::WebMediaStreamTrack> web_audio_tracks(
      audio_track_count);
  blink::WebVector<blink::WebMediaStreamTrack> web_video_tracks(
      video_track_count);
  size_t audio_i = 0;
  size_t video_i = 0;
  for (const auto& adapter_ref : adapter_refs_) {
    const blink::WebMediaStreamTrack& web_track = adapter_ref->web_track();
    if (web_track.Source().GetType() == blink::WebMediaStreamSource::kTypeAudio)
      web_audio_tracks[audio_i++] = web_track;
    else
      web_video_tracks[video_i++] = web_track;
  }

  web_stream_.Initialize(blink::WebString::FromUTF8(label), web_audio_tracks,
                         web_video_tracks);
  CHECK(!web_stream_.IsNull());

  base::AutoLock scoped_lock(lock_);
  is_initialized_ = true;
}

void RemoteWebRtcMediaStreamAdapter::SetTracks(
    WebRtcMediaStreamAdapter::TrackAdapterRefs track_refs) {
  DCHECK(main_thread_->BelongsToCurrentThread());

  // Find removed tracks.
  base::EraseIf(
      adapter_refs_,
      [this, &track_refs](
          const std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>&
              track_ref) {
        bool erase = !RemoteAdapterRefsContainsTrack(track_refs,
                                                     track_ref->webrtc_track());
        if (erase)
          web_stream_.RemoveTrack(track_ref->web_track());
        return erase;
      });
  // Find added tracks.
  for (auto& track_ref : track_refs) {
    if (!RemoteAdapterRefsContainsTrack(adapter_refs_,
                                        track_ref->webrtc_track())) {
      web_stream_.AddTrack(track_ref->web_track());
      adapter_refs_.push_back(std::move(track_ref));
    }
  }
}

}  // namespace content
