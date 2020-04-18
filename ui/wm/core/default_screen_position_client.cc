// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/default_screen_position_client.h"

#include "ui/aura/window_tree_host.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"

namespace wm {

DefaultScreenPositionClient::DefaultScreenPositionClient() {
}

DefaultScreenPositionClient::~DefaultScreenPositionClient() {
}

gfx::Point DefaultScreenPositionClient::GetOriginInScreen(
    const aura::Window* root_window) {
  aura::Window* window = const_cast<aura::Window*>(root_window);
  display::Screen* screen = display::Screen::GetScreen();
  gfx::Rect screen_bounds = root_window->GetHost()->GetBoundsInPixels();
  gfx::Rect dip_bounds = screen->ScreenToDIPRectInWindow(window, screen_bounds);
  return dip_bounds.origin();
}

void DefaultScreenPositionClient::ConvertPointToScreen(
    const aura::Window* window,
    gfx::PointF* point) {
  const aura::Window* root_window = window->GetRootWindow();
  aura::Window::ConvertPointToTarget(window, root_window, point);
  gfx::Point origin = GetOriginInScreen(root_window);
  point->Offset(origin.x(), origin.y());
}

void DefaultScreenPositionClient::ConvertPointFromScreen(
    const aura::Window* window,
    gfx::PointF* point) {
  const aura::Window* root_window = window->GetRootWindow();
  gfx::Point origin = GetOriginInScreen(root_window);
  point->Offset(-origin.x(), -origin.y());
  aura::Window::ConvertPointToTarget(root_window, window, point);
}

void DefaultScreenPositionClient::ConvertHostPointToScreen(aura::Window* window,
                                                           gfx::Point* point) {
  aura::Window* root_window = window->GetRootWindow();
  aura::client::ScreenPositionClient::ConvertPointToScreen(root_window, point);
}

void DefaultScreenPositionClient::SetBounds(aura::Window* window,
                                            const gfx::Rect& bounds,
                                            const display::Display& display) {
  window->SetBounds(bounds);
}

}  // namespace wm
