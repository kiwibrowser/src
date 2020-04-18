// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_DELEGATE_IMPL_H_
#define SERVICES_UI_WS2_WINDOW_DELEGATE_IMPL_H_

#include "ui/aura/window_delegate.h"

namespace ui {
namespace ws2 {

// The aura::WindowDelegate implementation used for non-top-level windows
// created by the WindowService.
// WindowDelegateImpl deletes itself when the associated window is deleted.
class WindowDelegateImpl : public aura::WindowDelegate {
 public:
  WindowDelegateImpl();

  // This must be set right after creating WindowDelegateImpl (it can't be
  // passed to the constructor because Window's constructor takes
  // WindowDelegate).
  void set_window(aura::Window* window) { window_ = window; }

  // aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override;
  gfx::NativeCursor GetCursor(const gfx::Point& point) override;
  int GetNonClientComponent(const gfx::Point& point) const override;
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override;
  bool CanFocus() override;
  void OnCaptureLost() override;
  void OnPaint(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowTargetVisibilityChanged(bool visible) override;
  bool HasHitTestMask() const override;
  void GetHitTestMask(gfx::Path* mask) const override;

 private:
  ~WindowDelegateImpl() override;

  aura::Window* window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WindowDelegateImpl);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_DELEGATE_IMPL_H_
