// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/cursor_renderer_aura.h"

#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/cursors_aura.h"
#include "ui/events/event_utils.h"
#include "ui/wm/public/activation_client.h"

namespace content {

// static
std::unique_ptr<CursorRenderer> CursorRenderer::Create(
    CursorRenderer::CursorDisplaySetting display) {
  return std::make_unique<CursorRendererAura>(display);
}

CursorRendererAura::CursorRendererAura(CursorDisplaySetting display)
    : CursorRenderer(display) {}

CursorRendererAura::~CursorRendererAura() {
  SetTargetView(nullptr);
}

void CursorRendererAura::SetTargetView(aura::Window* window) {
  if (window_) {
    window_->RemoveObserver(this);
    window_->RemovePreTargetHandler(this);
  }
  window_ = window;
  OnMouseHasGoneIdle();
  if (window_) {
    window_->AddObserver(this);
    window_->AddPreTargetHandler(this);
  }
}

bool CursorRendererAura::IsCapturedViewActive() {
  if (!window_) {
    DVLOG(2) << "Skipping update with no window being tracked";
    return false;
  }

  // If we are sharing the root window, or namely the whole screen, we will
  // render the mouse cursor. For ordinary window, we only render the mouse
  // cursor when the window is active.
  if (!window_->IsRootWindow()) {
    wm::ActivationClient* activation_client =
        wm::GetActivationClient(window_->GetRootWindow());
    if (!activation_client) {
      DVLOG(2) << "Assume window inactive with invalid activation_client";
      return false;
    }
    aura::Window* active_window = activation_client->GetActiveWindow();
    if (!active_window) {
      DVLOG(2) << "Skipping update as there is no active window";
      return false;
    }
    if (!active_window->Contains(window_)) {
      // Return early if the target window is not active.
      DVLOG(2) << "Skipping update on an inactive window";
      return false;
    }
  }
  return true;
}

gfx::Size CursorRendererAura::GetCapturedViewSize() {
  if (!window_) {
    return gfx::Size();
  }

  const gfx::Rect window_bounds = window_->GetBoundsInScreen();
  const gfx::Size view_size(window_bounds.width(), window_bounds.height());
  return view_size;
}

gfx::Point CursorRendererAura::GetCursorPositionInView() {
  if (!window_) {
    return gfx::Point(-1, -1);
  }

  // Convert from screen coordinates to view coordinates.
  aura::Window* const root_window = window_->GetRootWindow();
  if (!root_window)
    return gfx::Point(-1, -1);
  aura::client::ScreenPositionClient* const client =
      aura::client::GetScreenPositionClient(root_window);
  if (!client)
    return gfx::Point(-1, -1);
  gfx::Point cursor_position = aura::Env::GetInstance()->last_mouse_location();
  client->ConvertPointFromScreen(window_, &cursor_position);
  return cursor_position;
}

gfx::NativeCursor CursorRendererAura::GetLastKnownCursor() {
  if (!window_) {
    return gfx::NativeCursor();
  }

  return window_->GetHost()->last_cursor();
}

SkBitmap CursorRendererAura::GetLastKnownCursorImage(gfx::Point* hot_point) {
  if (!window_) {
    return SkBitmap();
  }

  gfx::NativeCursor cursor = window_->GetHost()->last_cursor();
  *hot_point = cursor.GetHotspot();
  return cursor.GetBitmap();
}

void CursorRendererAura::OnMouseEvent(ui::MouseEvent* event) {
  gfx::Point mouse_location(event->x(), event->y());
  switch (event->type()) {
    case ui::ET_MOUSE_MOVED:
      OnMouseMoved(mouse_location);
      break;
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_MOUSE_RELEASED:
    case ui::ET_MOUSEWHEEL:
      OnMouseClicked(mouse_location);
      break;
    default:
      return;
  }
}

void CursorRendererAura::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window_, window);
  window_->RemovePreTargetHandler(this);
  window_->RemoveObserver(this);
  window_ = nullptr;
}

}  // namespace content
