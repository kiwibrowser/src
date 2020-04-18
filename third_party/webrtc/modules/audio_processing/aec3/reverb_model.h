/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_AEC3_REVERB_MODEL_H_
#define MODULES_AUDIO_PROCESSING_AEC3_REVERB_MODEL_H_

#include "api/array_view.h"
#include "modules/audio_processing/aec3/aec3_common.h"

namespace webrtc {

// The ReverbModel class describes an exponential reverberant model
// that can be applied over power spectrums.
class ReverbModel {
 public:
  ReverbModel();
  ~ReverbModel();

  // Resets the state.
  void Reset();

  // Updates the reverberation contributions.
  void UpdateReverbContributions(rtc::ArrayView<const float> power_spectrum,
                                 float power_spectrum_scaling,
                                 float reverb_decay);

  // Adds the reverberation contributions to an input/output power spectrum.
  // - power_spectrum: Input to the exponential reverberation model.
  // - power_spectrum_scaling: A pre-scaling of the power_spectrum used
  // before applying the exponential reverberation model.
  // - reverb_decay: Parameter used by the expontial reververation model.
  void AddReverb(rtc::ArrayView<const float> power_spectrum,
                 float power_spectrum_scaling,
                 float reverb_decay,
                 rtc::ArrayView<float> reverb_power_spectrum);

  // Returns the current power spectrum reverberation contributions.
  const std::array<float, kFftLengthBy2Plus1>& GetPowerSpectrum() const {
    return reverb_;
  }

 private:
  std::array<float, kFftLengthBy2Plus1> reverb_;
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_PROCESSING_AEC3_REVERB_MODEL_H_
