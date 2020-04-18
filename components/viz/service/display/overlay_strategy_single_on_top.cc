// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/overlay_strategy_single_on_top.h"

#include "components/viz/service/display/overlay_candidate_validator.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace viz {

OverlayStrategySingleOnTop::OverlayStrategySingleOnTop(
    OverlayCandidateValidator* capability_checker)
    : capability_checker_(capability_checker) {
  DCHECK(capability_checker);
}

OverlayStrategySingleOnTop::~OverlayStrategySingleOnTop() {}

bool OverlayStrategySingleOnTop::Attempt(
    const SkMatrix44& output_color_matrix,
    DisplayResourceProvider* resource_provider,
    RenderPass* render_pass,
    OverlayCandidateList* candidate_list,
    std::vector<gfx::Rect>* content_bounds) {
  QuadList* quad_list = &render_pass->quad_list;
  // Build a list of candidates with the associated quad.
  OverlayCandidate best_candidate;
  auto best_quad_it = quad_list->end();
  for (auto it = quad_list->begin(); it != quad_list->end(); ++it) {
    OverlayCandidate candidate;
    if (OverlayCandidate::FromDrawQuad(resource_provider, output_color_matrix,
                                       *it, &candidate) &&
        !OverlayCandidate::IsOccluded(candidate, quad_list->cbegin(), it)) {
      if (candidate.display_rect.size().GetArea() >
          best_candidate.display_rect.size().GetArea()) {
        best_candidate = candidate;
        best_quad_it = it;
      }
    }
  }
  if (best_quad_it == quad_list->end())
    return false;

  if (TryOverlay(quad_list, candidate_list, best_candidate, best_quad_it))
    return true;

  return false;
}

bool OverlayStrategySingleOnTop::TryOverlay(
    QuadList* quad_list,
    OverlayCandidateList* candidate_list,
    const OverlayCandidate& candidate,
    QuadList::Iterator candidate_iterator) {
  // Add the overlay.
  OverlayCandidateList new_candidate_list = *candidate_list;
  new_candidate_list.push_back(candidate);
  new_candidate_list.back().plane_z_order = 1;

  // Check for support.
  capability_checker_->CheckOverlaySupport(&new_candidate_list);

  const OverlayCandidate& overlay_candidate = new_candidate_list.back();
  // If the candidate can be handled by an overlay, create a pass for it.
  if (overlay_candidate.overlay_handled) {
    quad_list->EraseAndInvalidateAllPointers(candidate_iterator);
    candidate_list->swap(new_candidate_list);
    return true;
  }

  return false;
}

OverlayProcessor::StrategyType OverlayStrategySingleOnTop::GetUMAEnum() const {
  return OverlayProcessor::StrategyType::kSingleOnTop;
}

}  // namespace viz
