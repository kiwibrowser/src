// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_CONTAINER_BEHAVIOR_H_
#define UI_KEYBOARD_CONTAINER_BEHAVIOR_H_

#include "ui/aura/window.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event.h"
#include "ui/keyboard/container_type.h"
#include "ui/keyboard/keyboard_export.h"
#include "ui/wm/core/window_animations.h"

namespace keyboard {

// Represents and encapsulates how the keyboard container should visually behave
// within the workspace window.
class KEYBOARD_EXPORT ContainerBehavior {
 public:
  virtual ~ContainerBehavior() {}

  // Apply changes to the animation settings to animate the keyboard container
  // showing.
  virtual void DoShowingAnimation(
      aura::Window* window,
      ui::ScopedLayerAnimationSettings* animation_settings) = 0;

  // Apply changes to the animation settings to animate the keyboard container
  // hiding.
  virtual void DoHidingAnimation(
      aura::Window* window,
      ::wm::ScopedHidingAnimationSettings* animation_settings) = 0;

  // Initialize the starting state of the keyboard container for the showing
  // animation.
  virtual void InitializeShowAnimationStartingState(
      aura::Window* container) = 0;

  // Used by the layout manager to intercept any bounds setting request to
  // adjust the request to different bounds, if necessary. This method gets
  // called at any time during the keyboard's life cycle. The bounds are in
  // global screen coordinates.
  virtual gfx::Rect AdjustSetBoundsRequest(
      const gfx::Rect& display_bounds,
      const gfx::Rect& requested_bounds_in_screen_coords) = 0;

  // Used to set the bounds to the default location. This is generally called
  // during initialization, but may also be have identical behavior to
  // AdjustSetBoundsRequest in the case of constant layouts such as the fixed
  // full-width keyboard.
  virtual void SetCanonicalBounds(aura::Window* container,
                                  const gfx::Rect& display_bounds) = 0;

  // A ContainerBehavior can choose to not allow overscroll to be used. It is
  // important to note that the word "Allowed" is used because whether or not
  // overscroll is "enabled" depends on multiple external factors.
  virtual bool IsOverscrollAllowed() const = 0;

  // Return whether the given coordinate is a drag handle.
  virtual bool IsDragHandle(const gfx::Vector2d& offset,
                            const gfx::Size& keyboard_size) const = 0;

  virtual void SavePosition(const gfx::Rect& keyboard_bounds,
                            const gfx::Size& screen_size) = 0;

  virtual bool HandlePointerEvent(const ui::LocatedEvent& event,
                                  const display::Display& current_display) = 0;

  virtual ContainerType GetType() const = 0;

  // Removing focus from a text field should cause the keyboard to be dismissed.
  virtual bool TextBlurHidesKeyboard() const = 0;

  // Gets the region of the screen that is occluded by the keyboard, or an empty
  // rectangle if nothing is occluded. The occluded region is considered to be
  // 'unusable', so the window manager or other system UI should respond to the
  // occluded bounds (e.g. by moving windows out of the occluded region).
  //
  // The occluded bounds must be completely contained in the visual bounds.
  virtual gfx::Rect GetOccludedBounds(
      const gfx::Rect& visual_bounds_in_screen) const = 0;

  // Any region of the screen that is occluded by the keyboard should cause the
  // workspace to change its layout.
  virtual bool OccludedBoundsAffectWorkspaceLayout() const = 0;

  // Sets floating keyboard drggable rect.
  virtual bool SetDraggableArea(const gfx::Rect& rect) = 0;

 protected:
  // The opacity of virtual keyboard container when show animation
  // starts or hide animation finishes. This cannot be zero because we
  // call Show() on the keyboard window before setting the opacity
  // back to 1.0. Since windows are not allowed to be shown with zero
  // opacity, we always animate to 0.01 instead.
  static constexpr float kAnimationStartOrAfterHideOpacity = 0.01f;
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_CONTAINER_BEHAVIOR_H_
