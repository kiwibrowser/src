// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_CANDIDATE_VALIDATOR_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_CANDIDATE_VALIDATOR_H_

#include <vector>

#include "components/viz/service/display/overlay_candidate.h"
#include "components/viz/service/display/overlay_processor.h"
#include "components/viz/service/viz_service_export.h"

namespace viz {

// This class that can be used to answer questions about possible overlay
// configurations for a particular output device.
class VIZ_SERVICE_EXPORT OverlayCandidateValidator {
 public:
  // Populates a list of strategies that may work with this validator.
  virtual void GetStrategies(OverlayProcessor::StrategyList* strategies) = 0;

  // Returns true if draw quads can be represented as CALayers (Mac only).
  virtual bool AllowCALayerOverlays() = 0;

  // Returns true if draw quads can be represented as Direct Composition
  // Visuals (Windows only).
  virtual bool AllowDCLayerOverlays() = 0;

  // A list of possible overlay candidates is presented to this function.
  // The expected result is that those candidates that can be in a separate
  // plane are marked with |overlay_handled| set to true, otherwise they are
  // to be traditionally composited. Candidates with |overlay_handled| set to
  // true must also have their |display_rect| converted to integer
  // coordinates if necessary.
  virtual void CheckOverlaySupport(OverlayCandidateList* surfaces) = 0;

  virtual ~OverlayCandidateValidator() {}
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_CANDIDATE_VALIDATOR_H_
