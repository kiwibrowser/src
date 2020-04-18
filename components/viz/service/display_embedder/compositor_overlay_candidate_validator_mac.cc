// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator_mac.h"

#include <stddef.h>

namespace viz {

CompositorOverlayCandidateValidatorMac::CompositorOverlayCandidateValidatorMac(
    bool ca_layer_disabled)
    : software_mirror_active_(false), ca_layer_disabled_(ca_layer_disabled) {}

CompositorOverlayCandidateValidatorMac::
    ~CompositorOverlayCandidateValidatorMac() {}

void CompositorOverlayCandidateValidatorMac::GetStrategies(
    OverlayProcessor::StrategyList* strategies) {}

bool CompositorOverlayCandidateValidatorMac::AllowCALayerOverlays() {
  return !ca_layer_disabled_ && !software_mirror_active_;
}

bool CompositorOverlayCandidateValidatorMac::AllowDCLayerOverlays() {
  return false;
}

void CompositorOverlayCandidateValidatorMac::CheckOverlaySupport(
    OverlayCandidateList* surfaces) {}

void CompositorOverlayCandidateValidatorMac::SetSoftwareMirrorMode(
    bool enabled) {
  software_mirror_active_ = enabled;
}

}  // namespace viz
