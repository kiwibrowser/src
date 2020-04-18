// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_OUTPUT_STREAM_H_
#define CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_OUTPUT_STREAM_H_

#include <memory>

#include "base/macros.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_parameters.h"

namespace chromecast {
namespace media {

class CastAudioManager;

class CastAudioOutputStream : public ::media::AudioOutputStream {
 public:
  CastAudioOutputStream(const ::media::AudioParameters& audio_params,
                        CastAudioManager* audio_manager);
  ~CastAudioOutputStream() override;

  // ::media::AudioOutputStream implementation.
  bool Open() override;
  void Close() override;
  void Start(AudioSourceCallback* source_callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;

 private:
  class Backend;

  const ::media::AudioParameters audio_params_;
  CastAudioManager* const audio_manager_;
  double volume_;

  // Only valid when the stream is open.
  std::unique_ptr<Backend> backend_;

  DISALLOW_COPY_AND_ASSIGN(CastAudioOutputStream);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_AUDIO_CAST_AUDIO_OUTPUT_STREAM_H_
