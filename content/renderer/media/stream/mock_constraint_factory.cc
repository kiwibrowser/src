// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/media/stream/media_stream_audio_processor_options.h"
#include "content/renderer/media/stream/mock_constraint_factory.h"
#include "third_party/webrtc/api/mediaconstraintsinterface.h"

namespace content {

MockConstraintFactory::MockConstraintFactory() {}

MockConstraintFactory::~MockConstraintFactory() {}

blink::WebMediaTrackConstraintSet& MockConstraintFactory::AddAdvanced() {
  advanced_.emplace_back();
  return advanced_.back();
}

blink::WebMediaConstraints MockConstraintFactory::CreateWebMediaConstraints()
    const {
  blink::WebMediaConstraints constraints;
  constraints.Initialize(basic_, advanced_);
  return constraints;
}

void MockConstraintFactory::DisableDefaultAudioConstraints() {
  basic_.goog_echo_cancellation.SetExact(false);
  basic_.goog_experimental_echo_cancellation.SetExact(false);
  basic_.goog_auto_gain_control.SetExact(false);
  basic_.goog_experimental_auto_gain_control.SetExact(false);
  basic_.goog_noise_suppression.SetExact(false);
  basic_.goog_noise_suppression.SetExact(false);
  basic_.goog_highpass_filter.SetExact(false);
  basic_.goog_typing_noise_detection.SetExact(false);
  basic_.goog_experimental_noise_suppression.SetExact(false);
  basic_.goog_beamforming.SetExact(false);
}

void MockConstraintFactory::DisableAecAudioConstraints() {
  basic_.goog_echo_cancellation.SetExact(false);
}

void MockConstraintFactory::Reset() {
  basic_ = blink::WebMediaTrackConstraintSet();
  advanced_.clear();
}

}  // namespace content
