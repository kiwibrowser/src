// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/invisible_hit_target.h"

namespace vr {

InvisibleHitTarget::InvisibleHitTarget() {
  set_hit_testable(true);
}
InvisibleHitTarget::~InvisibleHitTarget() = default;

void InvisibleHitTarget::Render(UiElementRenderer* renderer,
                                const CameraModel& model) const {}

void InvisibleHitTarget::OnHoverEnter(const gfx::PointF& position) {
  UiElement::OnHoverEnter(position);
  hovered_ = true;
}

void InvisibleHitTarget::OnHoverLeave() {
  UiElement::OnHoverLeave();
  hovered_ = false;
}

}  // namespace vr
