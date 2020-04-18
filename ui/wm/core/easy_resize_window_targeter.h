// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_EASY_RESIZE_WINDOW_TARGETER_H_
#define UI_WM_CORE_EASY_RESIZE_WINDOW_TARGETER_H_

#include "base/macros.h"
#include "ui/aura/window_targeter.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

// An EventTargeter for a container window that uses a slightly larger
// hit-target region for easier resize.
// TODO(sky): make this class final.
class WM_CORE_EXPORT EasyResizeWindowTargeter : public aura::WindowTargeter {
 public:
  // |container| window is the owner of this targeter.
  // NOTE: the insets must be negative.
  EasyResizeWindowTargeter(aura::Window* container,
                           const gfx::Insets& mouse_extend,
                           const gfx::Insets& touch_extend);

  ~EasyResizeWindowTargeter() override;

 protected:
  // aura::WindowTargeter:
  void OnSetInsets(const gfx::Insets& last_mouse_extend,
                   const gfx::Insets& last_touch_extend) override;

 private:
  class HitMaskSetter;

  // aura::WindowTargeter:
  // Delegates to WindowTargeter's impl and prevents overriding in subclasses.
  bool EventLocationInsideBounds(aura::Window* target,
                                 const ui::LocatedEvent& event) const final;

  // Returns true if the hit testing (GetHitTestRects()) should use the
  // extended bounds.
  bool ShouldUseExtendedBounds(const aura::Window* window) const override;

  aura::Window* container_;

  std::unique_ptr<HitMaskSetter> hit_mask_setter_;

  DISALLOW_COPY_AND_ASSIGN(EasyResizeWindowTargeter);
};

}  // namespace wm

#endif  // UI_WM_CORE_EASY_RESIZE_WINDOW_TARGETER_H_
