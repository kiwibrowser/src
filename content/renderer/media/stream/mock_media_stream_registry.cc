// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/mock_media_stream_registry.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/mock_media_stream_video_source.h"
#include "content/renderer/media/stream/video_track_adapter.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace content {

namespace {

const char kTestStreamLabel[] = "stream_label";

class MockCDQualityAudioSource : public MediaStreamAudioSource {
 public:
  MockCDQualityAudioSource() : MediaStreamAudioSource(true) {
    SetFormat(media::AudioParameters(
        media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
        media::CHANNEL_LAYOUT_STEREO,
        media::AudioParameters::kAudioCDSampleRate,
        media::AudioParameters::kAudioCDSampleRate / 100));
    SetDevice(MediaStreamDevice(
        MEDIA_DEVICE_AUDIO_CAPTURE, "mock_audio_device_id", "Mock audio device",
        media::AudioParameters::kAudioCDSampleRate,
        media::CHANNEL_LAYOUT_STEREO,
        media::AudioParameters::kAudioCDSampleRate / 100));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCDQualityAudioSource);
};

}  // namespace

MockMediaStreamRegistry::MockMediaStreamRegistry() {}

void MockMediaStreamRegistry::Init(const std::string& stream_url) {
  stream_url_ = stream_url;
  const blink::WebVector<blink::WebMediaStreamTrack> webkit_audio_tracks;
  const blink::WebVector<blink::WebMediaStreamTrack> webkit_video_tracks;
  const blink::WebString label(kTestStreamLabel);
  test_stream_.Initialize(label, webkit_audio_tracks, webkit_video_tracks);
}

void MockMediaStreamRegistry::AddVideoTrack(
    const std::string& track_id,
    const VideoTrackAdapterSettings& adapter_settings,
    const base::Optional<bool>& noise_reduction,
    bool is_screencast,
    double min_frame_rate) {
  blink::WebMediaStreamSource blink_source;
  blink_source.Initialize("mock video source id",
                          blink::WebMediaStreamSource::kTypeVideo,
                          "mock video source name", false /* remote */);
  MockMediaStreamVideoSource* native_source = new MockMediaStreamVideoSource();
  blink_source.SetExtraData(native_source);
  blink::WebMediaStreamTrack blink_track;
  blink_track.Initialize(blink::WebString::FromUTF8(track_id), blink_source);

  MediaStreamVideoTrack* native_track = new MediaStreamVideoTrack(
      native_source, adapter_settings, noise_reduction, is_screencast,
      min_frame_rate, MediaStreamVideoSource::ConstraintsCallback(),
      true /* enabled */);
  blink_track.SetTrackData(native_track);
  test_stream_.AddTrack(blink_track);
}

void MockMediaStreamRegistry::AddVideoTrack(const std::string& track_id) {
  AddVideoTrack(track_id, VideoTrackAdapterSettings(), base::Optional<bool>(),
                false /* is_screncast */, 0.0 /* min_frame_rate */);
}

void MockMediaStreamRegistry::AddAudioTrack(const std::string& track_id) {
  blink::WebMediaStreamSource blink_source;
  blink_source.Initialize("mock audio source id",
                          blink::WebMediaStreamSource::kTypeAudio,
                          "mock audio source name", false /* remote */);
  MediaStreamAudioSource* const source = new MockCDQualityAudioSource();
  blink_source.SetExtraData(source);  // Takes ownership.

  blink::WebMediaStreamTrack blink_track;
  blink_track.Initialize(blink_source);
  CHECK(source->ConnectToTrack(blink_track));

  test_stream_.AddTrack(blink_track);
}

blink::WebMediaStream MockMediaStreamRegistry::GetMediaStream(
    const std::string& url) {
  return (url != stream_url_) ? blink::WebMediaStream() : test_stream_;
}

}  // namespace content
