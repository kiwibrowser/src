// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_STEREO_PANNER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_STEREO_PANNER_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class AudioBus;

// Implement the equal-power panning algorithm for mono or stereo input. See:
// https://webaudio.github.io/web-audio-api/#Spatialzation-equal-power-panning

class PLATFORM_EXPORT StereoPanner {
  USING_FAST_MALLOC(StereoPanner);
  WTF_MAKE_NONCOPYABLE(StereoPanner);

 public:
  static std::unique_ptr<StereoPanner> Create(float sample_rate);
  ~StereoPanner() = default;
  ;

  void PanWithSampleAccurateValues(const AudioBus* input_bus,
                                   AudioBus* output_bus,
                                   const float* pan_values,
                                   size_t frames_to_process);
  void PanToTargetValue(const AudioBus* input_bus,
                        AudioBus* output_bus,
                        float pan_value,
                        size_t frames_to_process);

 private:
  explicit StereoPanner(float sample_rate);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_AUDIO_STEREO_PANNER_H_
