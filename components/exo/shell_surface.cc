// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace exo {
namespace {

// Maximum amount of time to wait for contents after a change to maximize,
// fullscreen or pinned state.
constexpr int kMaximizedOrFullscreenOrPinnedLockTimeoutMs = 100;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// ShellSurface, ScopedAnimationsDisabled:

// Helper class used to temporarily disable animations. Restores the
// animations disabled property when instance is destroyed.
class ShellSurface::ScopedAnimationsDisabled {
 public:
  explicit ScopedAnimationsDisabled(ShellSurface* shell_surface);
  ~ScopedAnimationsDisabled();

 private:
  ShellSurface* const shell_surface_;
  bool saved_animations_disabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(ScopedAnimationsDisabled);
};

ShellSurface::ScopedAnimationsDisabled::ScopedAnimationsDisabled(
    ShellSurface* shell_surface)
    : shell_surface_(shell_surface) {
  if (shell_surface_->widget_) {
    aura::Window* window = shell_surface_->widget_->GetNativeWindow();
    saved_animations_disabled_ =
        window->GetProperty(aura::client::kAnimationsDisabledKey);
    window->SetProperty(aura::client::kAnimationsDisabledKey, true);
  }
}

ShellSurface::ScopedAnimationsDisabled::~ScopedAnimationsDisabled() {
  if (shell_surface_->widget_) {
    aura::Window* window = shell_surface_->widget_->GetNativeWindow();
    DCHECK_EQ(window->GetProperty(aura::client::kAnimationsDisabledKey), true);
    window->SetProperty(aura::client::kAnimationsDisabledKey,
                        saved_animations_disabled_);
  }
}

////////////////////////////////////////////////////////////////////////////////
// ShellSurface, public:

ShellSurface::ShellSurface(Surface* surface,
                           const gfx::Point& origin,
                           bool activatable,
                           bool can_minimize,
                           int container)
    : ShellSurfaceBase(surface, origin, activatable, can_minimize, container) {}

ShellSurface::ShellSurface(Surface* surface)
    : ShellSurfaceBase(surface,
                       gfx::Point(),
                       true,
                       true,
                       ash::kShellWindowId_DefaultContainer) {}

ShellSurface::~ShellSurface() {
  if (widget_)
    ash::wm::GetWindowState(widget_->GetNativeWindow())->RemoveObserver(this);
}

void ShellSurface::SetParent(ShellSurface* parent) {
  TRACE_EVENT1("exo", "ShellSurface::SetParent", "parent",
               parent ? base::UTF16ToASCII(parent->title_) : "null");

  SetParentWindow(parent ? parent->GetWidget()->GetNativeWindow() : nullptr);
}

void ShellSurface::Maximize() {
  TRACE_EVENT0("exo", "ShellSurface::Maximize");

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_MAXIMIZED);

  // Note: This will ask client to configure its surface even if already
  // maximized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Maximize();
}

void ShellSurface::Minimize() {
  TRACE_EVENT0("exo", "ShellSurface::Minimize");

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_MINIMIZED);

  // Note: This will ask client to configure its surface even if already
  // minimized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Minimize();
}

void ShellSurface::Restore() {
  TRACE_EVENT0("exo", "ShellSurface::Restore");

  if (!widget_)
    return;

  // Note: This will ask client to configure its surface even if not already
  // maximized or minimized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Restore();
}

void ShellSurface::SetFullscreen(bool fullscreen) {
  TRACE_EVENT1("exo", "ShellSurface::SetFullscreen", "fullscreen", fullscreen);

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_FULLSCREEN);

  // Note: This will ask client to configure its surface even if fullscreen
  // state doesn't change.
  ScopedConfigure scoped_configure(this, true);
  widget_->SetFullscreen(fullscreen);
}

void ShellSurface::Resize(int component) {
  TRACE_EVENT1("exo", "ShellSurface::Resize", "component", component);

  if (!widget_)
    return;

  AttemptToStartDrag(component);
}

void ShellSurface::InitializeWindowState(ash::wm::WindowState* window_state) {
  window_state->AddObserver(this);
  window_state->set_allow_set_bounds_direct(false);
  widget_->set_movement_disabled(movement_disabled_);
  window_state->set_ignore_keyboard_bounds_change(movement_disabled_);
}

////////////////////////////////////////////////////////////////////////////////
// ash::wm::WindowStateObserver overrides:

void ShellSurface::OnPreWindowStateTypeChange(
    ash::wm::WindowState* window_state,
    ash::mojom::WindowStateType old_type) {
  ash::mojom::WindowStateType new_type = window_state->GetStateType();
  if (ash::IsMinimizedWindowStateType(old_type) ||
      ash::IsMinimizedWindowStateType(new_type)) {
    return;
  }

  if (ash::IsMaximizedOrFullscreenOrPinnedWindowStateType(old_type) ||
      ash::IsMaximizedOrFullscreenOrPinnedWindowStateType(new_type)) {
    if (!widget_)
      return;
    // When transitioning in/out of maximized or fullscreen mode, we need to
    // make sure we have a configure callback before we allow the default
    // cross-fade animations. The configure callback provides a mechanism for
    // the client to inform us that a frame has taken the state change into
    // account, and without this cross-fade animations are unreliable.
    if (!configure_callback_.is_null()) {
      // Give client a chance to produce a frame that takes state change into
      // account by acquiring a compositor lock.
      ui::Compositor* compositor =
          widget_->GetNativeWindow()->layer()->GetCompositor();
      configure_compositor_lock_ = compositor->GetCompositorLock(
          nullptr, base::TimeDelta::FromMilliseconds(
                       kMaximizedOrFullscreenOrPinnedLockTimeoutMs));
    } else {
      scoped_animations_disabled_ =
          std::make_unique<ScopedAnimationsDisabled>(this);
    }
  }
}

void ShellSurface::OnPostWindowStateTypeChange(
    ash::wm::WindowState* window_state,
    ash::mojom::WindowStateType old_type) {
  ash::mojom::WindowStateType new_type = window_state->GetStateType();
  if (ash::IsMaximizedOrFullscreenOrPinnedWindowStateType(new_type)) {
    Configure();
  }

  if (widget_) {
    UpdateWidgetBounds();
    UpdateShadow();
  }

  // Re-enable animations if they were disabled in pre state change handler.
  scoped_animations_disabled_.reset();
}

}  // namespace exo
