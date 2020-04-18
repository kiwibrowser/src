// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_PROCESSED_LOCAL_AUDIO_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_PROCESSED_LOCAL_AUDIO_SOURCE_H_

#include <string>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "content/renderer/media/stream/media_stream_audio_level_calculator.h"
#include "content/renderer/media/stream/media_stream_audio_processor.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "media/base/audio_capturer_source.h"
#include "third_party/blink/public/platform/web_media_constraints.h"

namespace media {
class AudioBus;
}

namespace content {

class PeerConnectionDependencyFactory;

// Represents a local source of audio data that is routed through the WebRTC
// audio pipeline for post-processing (e.g., for echo cancellation during a
// video conferencing call). Owns a media::AudioCapturerSource and the
// MediaStreamProcessor that modifies its audio. Modified audio is delivered to
// one or more MediaStreamAudioTracks.
class CONTENT_EXPORT ProcessedLocalAudioSource final
    : public MediaStreamAudioSource,
      public media::AudioCapturerSource::CaptureCallback {
 public:
  // |consumer_render_frame_id| references the RenderFrame that will consume the
  // audio data. Audio parameters and (optionally) a pre-existing audio session
  // ID are derived from |device_info|. |factory| must outlive this instance.
  ProcessedLocalAudioSource(
      int consumer_render_frame_id,
      const MediaStreamDevice& device,
      bool hotword_enabled,
      bool disable_local_echo,
      const AudioProcessingProperties& audio_processing_properties,
      const ConstraintsCallback& started_callback,
      PeerConnectionDependencyFactory* factory);

  ~ProcessedLocalAudioSource() final;

  // If |source| is an instance of ProcessedLocalAudioSource, return a
  // type-casted pointer to it. Otherwise, return null.
  static ProcessedLocalAudioSource* From(MediaStreamAudioSource* source);

  // Non-browser unit tests cannot provide RenderFrame implementations at
  // run-time. This is used to skip the otherwise mandatory check for a valid
  // render frame ID when the source is started.
  void SetAllowInvalidRenderFrameIdForTesting(bool allowed) {
    allow_invalid_render_frame_id_for_testing_ = allowed;
  }

  const AudioProcessingProperties& audio_processing_properties() const {
    return audio_processing_properties_;
  }

  // The following accessors are not valid until after the source is started
  // (when the first track is connected).
  const scoped_refptr<MediaStreamAudioProcessor>& audio_processor() const {
    return audio_processor_;
  }
  const scoped_refptr<MediaStreamAudioLevelCalculator::Level>& audio_level()
      const {
    return level_calculator_.level();
  }

  // Thread-safe volume accessors used by WebRtcAudioDeviceImpl.
  void SetVolume(int volume);
  int Volume() const;
  int MaxVolume() const;

  // Audio parameters utilized by the source of the audio capturer.
  // TODO(phoglund): Think over the implications of this accessor and if we can
  // remove it.
  media::AudioParameters GetInputFormat() const;

  void SetOutputDeviceForAec(const std::string& output_device_id);

 protected:
  // MediaStreamAudioSource implementation.
  void* GetClassIdentifier() const final;
  bool EnsureSourceIsStarted() final;
  void EnsureSourceIsStopped() final;

  // AudioCapturerSource::CaptureCallback implementation.
  // Called on the AudioCapturerSource audio thread.
  void OnCaptureStarted() override;
  void Capture(const media::AudioBus* audio_source,
               int audio_delay_milliseconds,
               double volume,
               bool key_pressed) override;
  void OnCaptureError(const std::string& message) override;
  void OnCaptureMuted(bool is_muted) override;

 private:
  // Helper function to get the source buffer size based on whether audio
  // processing will take place.
  int GetBufferSize(int sample_rate) const;

  // The RenderFrame that will consume the audio data. Used when creating
  // AudioCapturerSources.
  const int consumer_render_frame_id_;

  PeerConnectionDependencyFactory* const pc_factory_;

  AudioProcessingProperties audio_processing_properties_;

  // Callback that's called when the audio source has been initialized.
  ConstraintsCallback started_callback_;

  // Audio processor doing processing like FIFO, AGC, AEC and NS. Its output
  // data is in a unit of 10 ms data chunk.
  scoped_refptr<MediaStreamAudioProcessor> audio_processor_;

  // The device created by the AudioDeviceFactory in EnsureSourceIsStarted().
  scoped_refptr<media::AudioCapturerSource> source_;

  // Stores latest microphone volume received in a CaptureData() callback.
  // Range is [0, 255].
  base::subtle::Atomic32 volume_;

  // Used to calculate the signal level that shows in the UI.
  MediaStreamAudioLevelCalculator level_calculator_;

  bool allow_invalid_render_frame_id_for_testing_;

  // Provides weak pointers for tasks posted by this instance.
  base::WeakPtrFactory<ProcessedLocalAudioSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProcessedLocalAudioSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_PROCESSED_LOCAL_AUDIO_SOURCE_H_
