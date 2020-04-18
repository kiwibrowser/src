// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_H_

#include "base/macros.h"
#include "components/viz/service/display/overlay_candidate_validator.h"
#include "components/viz/service/viz_service_export.h"

namespace viz {

class VIZ_SERVICE_EXPORT CompositorOverlayCandidateValidator
    : public OverlayCandidateValidator {
 public:
  CompositorOverlayCandidateValidator() {}
  ~CompositorOverlayCandidateValidator() override {}

  virtual void SetSoftwareMirrorMode(bool enabled) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CompositorOverlayCandidateValidator);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_COMPOSITOR_OVERLAY_CANDIDATE_VALIDATOR_H_
