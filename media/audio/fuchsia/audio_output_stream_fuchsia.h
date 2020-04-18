// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
#define MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_

#include <media/audio.h>

#include "base/timer/timer.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioManagerFuchsia;

class AudioOutputStreamFuchsia : public AudioOutputStream {
 public:
  // Caller must ensure that manager outlives the stream.
  AudioOutputStreamFuchsia(AudioManagerFuchsia* manager,
                           const std::string& device_id,
                           const AudioParameters& parameters);

  // AudioOutputStream interface.
  bool Open() override;
  void Start(AudioSourceCallback* callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;
  void Close() override;

 private:
  ~AudioOutputStreamFuchsia() override;

  base::TimeTicks GetCurrentStreamTime();

  // Updates |presentation_delay_ns_|.
  bool UpdatePresentationDelay();

  // Requests data from AudioSourceCallback, passes it to the mixer and
  // schedules |timer_| for the next call.
  void PumpSamples();

  AudioManagerFuchsia* manager_;
  std::string device_id_;
  AudioParameters parameters_;

  // These are used only in PumpSamples(). They are kept here to avoid
  // reallocating the memory every time.
  std::unique_ptr<AudioBus> audio_bus_;
  std::vector<float> buffer_;

  fuchsia_audio_output_stream* stream_ = nullptr;
  AudioSourceCallback* callback_ = nullptr;

  double volume_ = 1.0;

  base::TimeTicks started_time_;
  int64_t stream_position_samples_ = 0;

  // Total presentation delay for the stream. This value is returned by
  // fuchsia_audio_output_stream_get_min_delay()
  zx_duration_t presentation_delay_ns_ = 0;

  // Timer that's scheduled to call PumpSamples().
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(AudioOutputStreamFuchsia);
};

}  // namespace media

#endif  // MEDIA_AUDIO_FUCHSIA_AUDIO_OUTPUT_STREAM_FUCHSIA_H_
