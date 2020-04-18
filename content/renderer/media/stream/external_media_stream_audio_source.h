// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_EXTERNAL_MEDIA_STREAM_AUDIO_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_EXTERNAL_MEDIA_STREAM_AUDIO_SOURCE_H_

#include "content/renderer/media/stream/media_stream_audio_source.h"

#include "media/base/audio_capturer_source.h"

namespace content {

// Represents an externally-provided local or remote source of audio data. This
// allows users of the public content::MediaStreamApi to provide a
// media::AudioCapturerSource to be used as the source of audio data in the
// MediaStream framework. Audio data is transported directly to the tracks
// (i.e., there is no audio processing).
class CONTENT_EXPORT ExternalMediaStreamAudioSource final
    : public MediaStreamAudioSource,
      public media::AudioCapturerSource::CaptureCallback {
 public:
  ExternalMediaStreamAudioSource(
      scoped_refptr<media::AudioCapturerSource> source,
      int sample_rate,
      media::ChannelLayout channel_layout,
      int frames_per_buffer,
      bool is_remote);

  ~ExternalMediaStreamAudioSource() final;

 private:
  // MediaStreamAudioSource implementation.
  bool EnsureSourceIsStarted() final;
  void EnsureSourceIsStopped() final;

  // media::AudioCapturerSource::CaptureCallback implementation.
  void Capture(const media::AudioBus* audio_bus,
               int audio_delay_milliseconds,
               double volume,
               bool key_pressed) final;
  void OnCaptureError(const std::string& message) final;
  void OnCaptureMuted(bool is_muted) final;

  // The external source provided to the constructor.
  scoped_refptr<media::AudioCapturerSource> source_;

  // In debug builds, check that all methods that could cause object graph
  // or data flow changes are being called on the main thread.
  base::ThreadChecker thread_checker_;

  // True once the source has been started successfully.
  bool was_started_;

  DISALLOW_COPY_AND_ASSIGN(ExternalMediaStreamAudioSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_EXTERNAL_MEDIA_STREAM_AUDIO_SOURCE_H_
