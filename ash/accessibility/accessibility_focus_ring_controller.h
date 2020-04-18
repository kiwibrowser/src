// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_CONTROLLER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_CONTROLLER_H_

#include <memory>
#include <vector>

#include "ash/accessibility/accessibility_focus_ring.h"
#include "ash/accessibility/accessibility_layer.h"
#include "ash/ash_export.h"
#include "ash/public/interfaces/accessibility_focus_ring_controller.mojom.h"
#include "base/bind.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

class AccessibilityCursorRingLayer;
class AccessibilityFocusRingLayer;
class AccessibilityHighlightLayer;

// AccessibilityFocusRingController handles drawing custom rings around
// the focused object, cursor, and/or caret for accessibility.
class ASH_EXPORT AccessibilityFocusRingController
    : public AccessibilityLayerDelegate,
      public mojom::AccessibilityFocusRingController {
 public:
  AccessibilityFocusRingController();
  ~AccessibilityFocusRingController() override;

  // Binds the mojom::AccessibilityFocusRingController interface to this object.
  void BindRequest(mojom::AccessibilityFocusRingControllerRequest request);

  // mojom::AccessibilityFocusRingController overrides:
  void SetFocusRingColor(SkColor color) override;
  void ResetFocusRingColor() override;
  void SetFocusRing(const std::vector<gfx::Rect>& rects,
                    mojom::FocusRingBehavior focus_ring_behavior) override;
  void HideFocusRing() override;
  void SetHighlights(const std::vector<gfx::Rect>& rects,
                     SkColor color) override;
  void HideHighlights() override;

  // Draw a ring around the mouse cursor. It fades out automatically.
  void SetCursorRing(const gfx::Point& location);
  void HideCursorRing();

  // Draw a ring around the text caret. It fades out automatically.
  void SetCaretRing(const gfx::Point& location);
  void HideCaretRing();

  // Don't fade in / out, for testing.
  void SetNoFadeForTesting();

  // Get accessibility layers, for testing.
  AccessibilityCursorRingLayer* cursor_layer_for_testing() {
    return cursor_layer_.get();
  }
  AccessibilityCursorRingLayer* caret_layer_for_testing() {
    return caret_layer_.get();
  }
  const std::vector<std::unique_ptr<AccessibilityFocusRingLayer>>&
  focus_ring_layers_for_testing() {
    return focus_layers_;
  }

 protected:
  // Given an unordered vector of bounding rectangles that cover everything
  // that currently has focus, populate a vector of one or more
  // AccessibilityFocusRings that surround the rectangles. Adjacent or
  // overlapping rectangles are combined first. This function is protected
  // so it can be unit-tested.
  void RectsToRings(const std::vector<gfx::Rect>& rects,
                    std::vector<AccessibilityFocusRing>* rings) const;

  virtual int GetMargin() const;

  // Breaks an SkColor into its opacity and color. If the opacity is
  // not set (or is 0xFF), uses the |default_opacity| instead.
  // Visible for testing.
  static void GetColorAndOpacityFromColor(SkColor color,
                                          float default_opacity,
                                          SkColor* result_color,
                                          float* result_opacity);

 private:
  // AccessibilityLayerDelegate overrides.
  void OnDeviceScaleFactorChanged() override;
  void OnAnimationStep(base::TimeTicks timestamp) override;

  void UpdateFocusRingsFromFocusRects();
  void UpdateHighlightFromHighlightRects();

  void AnimateFocusRings(base::TimeTicks timestamp);
  void AnimateCursorRing(base::TimeTicks timestamp);
  void AnimateCaretRing(base::TimeTicks timestamp);

  AccessibilityFocusRing RingFromSortedRects(
      const std::vector<gfx::Rect>& rects) const;
  void SplitIntoParagraphShape(const std::vector<gfx::Rect>& rects,
                               gfx::Rect* top,
                               gfx::Rect* middle,
                               gfx::Rect* bottom) const;
  bool Intersects(const gfx::Rect& r1, const gfx::Rect& r2) const;

  struct LayerAnimationInfo {
    base::TimeTicks start_time;
    base::TimeTicks change_time;
    base::TimeDelta fade_in_time;
    base::TimeDelta fade_out_time;
    float opacity = 0;
    bool smooth = false;
  };
  void OnLayerChange(LayerAnimationInfo* animation_info);
  void ComputeOpacity(LayerAnimationInfo* animation_info,
                      base::TimeTicks timestamp);

  // Binding for mojom::AccessibilityFocusRingController interface.
  mojo::Binding<mojom::AccessibilityFocusRingController> binding_;

  LayerAnimationInfo focus_animation_info_;
  std::vector<gfx::Rect> focus_rects_;
  std::vector<AccessibilityFocusRing> previous_focus_rings_;
  std::vector<AccessibilityFocusRing> focus_rings_;
  std::vector<std::unique_ptr<AccessibilityFocusRingLayer>> focus_layers_;
  mojom::FocusRingBehavior focus_ring_behavior_ =
      mojom::FocusRingBehavior::FADE_OUT_FOCUS_RING;
  base::Optional<SkColor> focus_ring_color_;

  LayerAnimationInfo cursor_animation_info_;
  gfx::Point cursor_location_;
  std::unique_ptr<AccessibilityCursorRingLayer> cursor_layer_;

  LayerAnimationInfo caret_animation_info_;
  gfx::Point caret_location_;
  std::unique_ptr<AccessibilityCursorRingLayer> caret_layer_;

  std::vector<gfx::Rect> highlight_rects_;
  std::unique_ptr<AccessibilityHighlightLayer> highlight_layer_;
  SkColor highlight_color_ = SK_ColorBLACK;
  float highlight_opacity_ = 0.f;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityFocusRingController);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_FOCUS_RING_CONTROLLER_H_
