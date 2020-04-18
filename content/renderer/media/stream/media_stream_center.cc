// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_center.h"

#include <stddef.h>

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/media_stream_audio_sink.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/webaudio_media_stream_source.h"
#include "content/renderer/media/webrtc_local_audio_source_provider.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_center_client.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_frame.h"

using blink::WebFrame;
using blink::WebView;

namespace content {

namespace {

void CreateNativeAudioMediaStreamTrack(
    const blink::WebMediaStreamTrack& track) {
  blink::WebMediaStreamSource source = track.Source();
  MediaStreamAudioSource* media_stream_source =
      MediaStreamAudioSource::From(source);

  // At this point, a MediaStreamAudioSource instance must exist. The one
  // exception is when a WebAudio destination node is acting as a source of
  // audio.
  //
  // TODO(miu): This needs to be moved to an appropriate location. A WebAudio
  // source should have been created before this method was called so that this
  // special case code isn't needed here.
  if (!media_stream_source && source.RequiresAudioConsumer()) {
    DVLOG(1) << "Creating WebAudio media stream source.";
    media_stream_source = new WebAudioMediaStreamSource(&source);
    source.SetExtraData(media_stream_source);  // Takes ownership.

    blink::WebMediaStreamSource::Capabilities capabilities;
    capabilities.device_id = source.Id();
    capabilities.echo_cancellation = std::vector<bool>({false});
    capabilities.auto_gain_control = std::vector<bool>({false});
    capabilities.noise_suppression = std::vector<bool>({false});
    source.SetCapabilities(capabilities);
  }

  if (media_stream_source)
    media_stream_source->ConnectToTrack(track);
  else
    LOG(DFATAL) << "WebMediaStreamSource missing its MediaStreamAudioSource.";
}

void CreateNativeVideoMediaStreamTrack(blink::WebMediaStreamTrack track) {
  DCHECK(track.GetTrackData() == nullptr);
  blink::WebMediaStreamSource source = track.Source();
  DCHECK_EQ(source.GetType(), blink::WebMediaStreamSource::kTypeVideo);
  MediaStreamVideoSource* native_source =
      MediaStreamVideoSource::GetVideoSource(source);
  DCHECK(native_source);
  track.SetTrackData(new MediaStreamVideoTrack(
      native_source, MediaStreamVideoSource::ConstraintsCallback(),
      track.IsEnabled()));
}

void CloneNativeVideoMediaStreamTrack(
    const blink::WebMediaStreamTrack& original,
    blink::WebMediaStreamTrack clone) {
  DCHECK(!clone.GetTrackData());
  blink::WebMediaStreamSource source = clone.Source();
  DCHECK_EQ(source.GetType(), blink::WebMediaStreamSource::kTypeVideo);
  MediaStreamVideoSource* native_source =
      MediaStreamVideoSource::GetVideoSource(source);
  DCHECK(native_source);
  MediaStreamVideoTrack* original_track =
      MediaStreamVideoTrack::GetVideoTrack(original);
  DCHECK(original_track);
  clone.SetTrackData(new MediaStreamVideoTrack(
      native_source, original_track->adapter_settings(),
      original_track->noise_reduction(), original_track->is_screencast(),
      original_track->min_frame_rate(),
      MediaStreamVideoSource::ConstraintsCallback(), clone.IsEnabled()));
}

}  // namespace

MediaStreamCenter::MediaStreamCenter(
    blink::WebMediaStreamCenterClient* client,
    PeerConnectionDependencyFactory* factory) {}

MediaStreamCenter::~MediaStreamCenter() {}

void MediaStreamCenter::DidCreateMediaStreamTrack(
    const blink::WebMediaStreamTrack& track) {
  DVLOG(1) << "MediaStreamCenter::didCreateMediaStreamTrack";
  DCHECK(!track.IsNull() && !track.GetTrackData());
  DCHECK(!track.Source().IsNull());

  switch (track.Source().GetType()) {
    case blink::WebMediaStreamSource::kTypeAudio:
      CreateNativeAudioMediaStreamTrack(track);
      break;
    case blink::WebMediaStreamSource::kTypeVideo:
      CreateNativeVideoMediaStreamTrack(track);
      break;
  }
}

void MediaStreamCenter::DidCloneMediaStreamTrack(
    const blink::WebMediaStreamTrack& original,
    const blink::WebMediaStreamTrack& clone) {
  DCHECK(!clone.IsNull());
  DCHECK(!clone.GetTrackData());
  DCHECK(!clone.Source().IsNull());

  switch (clone.Source().GetType()) {
    case blink::WebMediaStreamSource::kTypeAudio:
      CreateNativeAudioMediaStreamTrack(clone);
      break;
    case blink::WebMediaStreamSource::kTypeVideo:
      CloneNativeVideoMediaStreamTrack(original, clone);
      break;
  }
}

void MediaStreamCenter::DidSetContentHint(
    const blink::WebMediaStreamTrack& track) {
  MediaStreamTrack* native_track = MediaStreamTrack::GetTrack(track);
  if (native_track)
    native_track->SetContentHint(track.ContentHint());
}

void MediaStreamCenter::DidEnableMediaStreamTrack(
    const blink::WebMediaStreamTrack& track) {
  MediaStreamTrack* native_track =
      MediaStreamTrack::GetTrack(track);
  if (native_track)
    native_track->SetEnabled(true);
}

void MediaStreamCenter::DidDisableMediaStreamTrack(
    const blink::WebMediaStreamTrack& track) {
  MediaStreamTrack* native_track =
      MediaStreamTrack::GetTrack(track);
  if (native_track)
    native_track->SetEnabled(false);
}

blink::WebAudioSourceProvider*
MediaStreamCenter::CreateWebAudioSourceFromMediaStreamTrack(
    const blink::WebMediaStreamTrack& track) {
  DVLOG(1) << "MediaStreamCenter::createWebAudioSourceFromMediaStreamTrack";
  MediaStreamTrack* media_stream_track =
      static_cast<MediaStreamTrack*>(track.GetTrackData());
  if (!media_stream_track) {
    DLOG(ERROR) << "Native track missing for webaudio source.";
    return nullptr;
  }

  blink::WebMediaStreamSource source = track.Source();
  DCHECK_EQ(source.GetType(), blink::WebMediaStreamSource::kTypeAudio);

  // TODO(tommi): Rename WebRtcLocalAudioSourceProvider to
  // WebAudioMediaStreamSink since it's not specific to any particular source.
  // http://crbug.com/577874
  return new WebRtcLocalAudioSourceProvider(track);
}

void MediaStreamCenter::DidStopMediaStreamSource(
    const blink::WebMediaStreamSource& web_source) {
  if (web_source.IsNull())
    return;
  MediaStreamSource* const source =
      static_cast<MediaStreamSource*>(web_source.GetExtraData());
  DCHECK(source);
  source->StopSource();
}

}  // namespace content
