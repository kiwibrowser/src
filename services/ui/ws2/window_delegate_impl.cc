// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_delegate_impl.h"

#include "services/ui/ws2/client_window.h"
#include "ui/aura/window.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {
namespace ws2 {

WindowDelegateImpl::WindowDelegateImpl() = default;

gfx::Size WindowDelegateImpl::GetMinimumSize() const {
  return gfx::Size();
}

gfx::Size WindowDelegateImpl::GetMaximumSize() const {
  return gfx::Size();
}

void WindowDelegateImpl::OnBoundsChanged(const gfx::Rect& old_bounds,
                                         const gfx::Rect& new_bounds) {}

gfx::NativeCursor WindowDelegateImpl::GetCursor(const gfx::Point& point) {
  return gfx::kNullCursor;
}

int WindowDelegateImpl::GetNonClientComponent(const gfx::Point& point) const {
  return HTNOWHERE;
}

bool WindowDelegateImpl::ShouldDescendIntoChildForEventHandling(
    aura::Window* child,
    const gfx::Point& location) {
  return true;
}

bool WindowDelegateImpl::CanFocus() {
  return ClientWindow::GetMayBeNull(window_)->can_focus();
}

void WindowDelegateImpl::OnCaptureLost() {}

void WindowDelegateImpl::OnPaint(const ui::PaintContext& context) {}

void WindowDelegateImpl::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {}

void WindowDelegateImpl::OnWindowDestroying(aura::Window* window) {}

void WindowDelegateImpl::OnWindowDestroyed(aura::Window* window) {
  delete this;
}

void WindowDelegateImpl::OnWindowTargetVisibilityChanged(bool visible) {}

bool WindowDelegateImpl::HasHitTestMask() const {
  return false;
}

void WindowDelegateImpl::GetHitTestMask(gfx::Path* mask) const {}

WindowDelegateImpl::~WindowDelegateImpl() = default;

}  // namespace ws2
}  // namespace ui
