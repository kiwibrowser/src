// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_INVISIBLE_HIT_TARGET_H_
#define CHROME_BROWSER_VR_ELEMENTS_INVISIBLE_HIT_TARGET_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"

namespace vr {

// A hit target does not render, but may affect reticle positioning.
class InvisibleHitTarget : public UiElement {
 public:
  InvisibleHitTarget();
  ~InvisibleHitTarget() override;

  void Render(UiElementRenderer* renderer,
              const CameraModel& model) const final;

  void OnHoverEnter(const gfx::PointF& position) override;
  void OnHoverLeave() override;

  bool hovered() const { return hovered_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(InvisibleHitTarget);
  bool hovered_ = false;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_INVISIBLE_HIT_TARGET_H_
