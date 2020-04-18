// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_WIN_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_WIN_H_

#include "base/macros.h"
#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator.h"
#include "components/viz/service/viz_service_export.h"

namespace viz {

// This is a simple overlay candidate validator that promotes everything
// possible to an overlay.
class VIZ_SERVICE_EXPORT CompositorOverlayCandidateValidatorWin
    : public CompositorOverlayCandidateValidator {
 public:
  CompositorOverlayCandidateValidatorWin();
  ~CompositorOverlayCandidateValidatorWin() override;

  void GetStrategies(OverlayProcessor::StrategyList* strategies) override;
  void CheckOverlaySupport(OverlayCandidateList* surfaces) override;
  bool AllowCALayerOverlays() override;
  bool AllowDCLayerOverlays() override;

  void SetSoftwareMirrorMode(bool enabled) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CompositorOverlayCandidateValidatorWin);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_WIN_H_
