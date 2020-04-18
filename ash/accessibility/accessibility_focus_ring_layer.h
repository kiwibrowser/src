// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_LAYER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_LAYER_H_

#include "ash/accessibility/accessibility_focus_ring.h"
#include "ash/accessibility/focus_ring_layer.h"
#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

// A subclass of FocusRingLayer intended for use by ChromeVox; it supports
// nonrectangular focus rings in order to highlight groups of elements or
// a range of text on a page.
class ASH_EXPORT AccessibilityFocusRingLayer : public FocusRingLayer {
 public:
  explicit AccessibilityFocusRingLayer(AccessibilityLayerDelegate* delegate);
  ~AccessibilityFocusRingLayer() override;

  // Create the layer and update its bounds and position in the hierarchy.
  void Set(const AccessibilityFocusRing& ring);

 private:
  // ui::LayerDelegate overrides:
  void OnPaintLayer(const ui::PaintContext& context) override;

  // The outline of the current focus ring.
  AccessibilityFocusRing ring_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityFocusRingLayer);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_LAYER_H_
