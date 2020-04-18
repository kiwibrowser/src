// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_PUBLIC_MEDIA_DIRECT_AUDIO_SOURCE_H_
#define CHROMECAST_PUBLIC_MEDIA_DIRECT_AUDIO_SOURCE_H_

#include <vector>

#include "media_pipeline_backend.h"

namespace chromecast {
namespace media {

// Direct audio source for the backend, used for low-latency audio output.
// All methods are called from a high-priority thread that is internal to the
// media backend, and therefore all methods should be threadsafe. Once a direct
// audio source has been added to the backend, it must not be deleted until
// after OnAudioPlaybackComplete() has been called on it.
class DirectAudioSource {
 public:
  using RenderingDelay = MediaPipelineBackend::AudioDecoder::RenderingDelay;

  // Returns the sample rate of audio provided by the source, in samples per
  // second.
  virtual int GetSampleRate() = 0;

  // Returns the number of audio channels provided by the source. This is the
  // number of channels that will be requested when FillAudioPlaybackFrames()
  // is called.
  virtual int GetNumChannels() = 0;

  // Returns the desired playback buffer size in frames. This is the desired
  // value for |num_frames| when FillAudioPlaybackFrames(); it affects the
  // playback latency (larger value = higher latency). The backend may choose a
  // different actual buffer size.
  virtual int GetDesiredFillSize() = 0;

  // Called when the source has been added to the backend, before any other
  // calls are made. The |read_size| is the number of frames that will be
  // requested for each call to FillAudioPlaybackFrames(). The
  // |initial_rendering_delay| is the rendering delay estimate for the first
  // call to FillAudioPlaybackFrames().
  virtual void InitializeAudioPlayback(
      int read_size,
      RenderingDelay initial_rendering_delay) = 0;

  // Called to read more audio data from the source. The source must fill in
  // the |channels| with up to |num_frames| of audio. Note that only planar
  // float format is supported.The |rendering_delay| indicates when the first
  // frame of the filled data will be played out.
  // Returns the number of frames filled.
  virtual int FillAudioPlaybackFrames(int num_frames,
                                      RenderingDelay rendering_delay,
                                      const std::vector<float*>& channels) = 0;

  // Called when an error occurs in audio playback. FillAudioPlaybackFrames()
  // will not be called after an error occurs.
  virtual void OnAudioPlaybackError() = 0;

  // Called when audio playback is complete for this source. The source can only
  // be safely deleted after OnAudioPlaybackComplete() has been called.
  virtual void OnAudioPlaybackComplete() = 0;

 protected:
  virtual ~DirectAudioSource() = default;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_PUBLIC_MEDIA_DIRECT_AUDIO_SOURCE_H_
