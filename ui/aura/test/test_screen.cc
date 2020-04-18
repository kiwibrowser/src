// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/test_screen.h"

#include <stdint.h>

#include "base/logging.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/native_widget_types.h"

namespace aura {

namespace {

bool IsRotationPortrait(display::Display::Rotation rotation) {
  return rotation == display::Display::ROTATE_90 ||
         rotation == display::Display::ROTATE_270;
}

}  // namespace

// static
TestScreen* TestScreen::Create(const gfx::Size& size,
                               WindowTreeClient* window_tree_client) {
  const gfx::Size kDefaultSize(800, 600);
  // Use (0,0) because the desktop aura tests are executed in
  // native environment where the display's origin is (0,0).
  return new TestScreen(gfx::Rect(size.IsEmpty() ? kDefaultSize : size),
                        window_tree_client);
}

TestScreen::~TestScreen() {
  delete host_;
}

WindowTreeHost* TestScreen::CreateHostForPrimaryDisplay() {
  DCHECK(!host_);
  if (window_tree_client_) {
    host_ = WindowTreeClientPrivate(window_tree_client_)
                .CallWmNewDisplayAdded(GetPrimaryDisplay());
  } else {
    host_ =
        WindowTreeHost::Create(gfx::Rect(GetPrimaryDisplay().GetSizeInPixel()));
  }
  // Some tests don't correctly manage window focus/activation states.
  // Makes sure InputMethod is default focused so that IME basics can work.
  host_->GetInputMethod()->OnFocus();
  host_->window()->AddObserver(this);
  // Other test code may have already initialized the compositor.
  if (!host_->compositor()->root_layer())
    host_->InitHost();
  host_->window()->Show();
  return host_;
}

void TestScreen::SetDeviceScaleFactor(float device_scale_factor) {
  display::Display display(GetPrimaryDisplay());
  gfx::Rect bounds_in_pixel(display.GetSizeInPixel());
  display.SetScaleAndBounds(device_scale_factor, bounds_in_pixel);
  display_list().UpdateDisplay(display);
  host_->OnHostResizedInPixels(bounds_in_pixel.size());
}

void TestScreen::SetColorSpace(const gfx::ColorSpace& color_space) {
  display::Display display(GetPrimaryDisplay());
  display.set_color_space(color_space);
  display_list().UpdateDisplay(display);
}

void TestScreen::SetDisplayRotation(display::Display::Rotation rotation) {
  display::Display display(GetPrimaryDisplay());
  gfx::Rect bounds_in_pixel(display.GetSizeInPixel());
  gfx::Rect new_bounds(bounds_in_pixel);
  if (IsRotationPortrait(rotation) != IsRotationPortrait(display.rotation())) {
    new_bounds.set_width(bounds_in_pixel.height());
    new_bounds.set_height(bounds_in_pixel.width());
  }
  display.set_rotation(rotation);
  display.SetScaleAndBounds(display.device_scale_factor(), new_bounds);
  display_list().UpdateDisplay(display);
  host_->SetRootTransform(GetRotationTransform() * GetUIScaleTransform());
}

void TestScreen::SetUIScale(float ui_scale) {
  ui_scale_ = ui_scale;
  display::Display display(GetPrimaryDisplay());
  gfx::Rect bounds_in_pixel(display.GetSizeInPixel());
  gfx::Rect new_bounds = gfx::ToNearestRect(
      gfx::ScaleRect(gfx::RectF(bounds_in_pixel), 1.0f / ui_scale));
  display.SetScaleAndBounds(display.device_scale_factor(), new_bounds);
  display_list().UpdateDisplay(display);
  host_->SetRootTransform(GetRotationTransform() * GetUIScaleTransform());
}

void TestScreen::SetWorkAreaInsets(const gfx::Insets& insets) {
  display::Display display(GetPrimaryDisplay());
  display.UpdateWorkAreaFromInsets(insets);
  display_list().UpdateDisplay(display);
}

gfx::Transform TestScreen::GetRotationTransform() const {
  gfx::Transform rotate;
  display::Display display(GetPrimaryDisplay());
  switch (display.rotation()) {
    case display::Display::ROTATE_0:
      break;
    case display::Display::ROTATE_90:
      rotate.Translate(display.bounds().height(), 0);
      rotate.Rotate(90);
      break;
    case display::Display::ROTATE_270:
      rotate.Translate(0, display.bounds().width());
      rotate.Rotate(270);
      break;
    case display::Display::ROTATE_180:
      rotate.Translate(display.bounds().width(), display.bounds().height());
      rotate.Rotate(180);
      break;
  }

  return rotate;
}

gfx::Transform TestScreen::GetUIScaleTransform() const {
  gfx::Transform ui_scale;
  ui_scale.Scale(1.0f / ui_scale_, 1.0f / ui_scale_);
  return ui_scale;
}

void TestScreen::OnWindowBoundsChanged(Window* window,
                                       const gfx::Rect& old_bounds,
                                       const gfx::Rect& new_bounds,
                                       ui::PropertyChangeReason reason) {
  DCHECK_EQ(host_->window(), window);
  display::Display display(GetPrimaryDisplay());
  display.SetSize(gfx::ScaleToFlooredSize(new_bounds.size(),
                                          display.device_scale_factor()));
  display_list().UpdateDisplay(display);
}

void TestScreen::OnWindowDestroying(Window* window) {
  if (host_->window() == window) {
    host_->window()->RemoveObserver(this);
    host_ = NULL;
  }
}

gfx::Point TestScreen::GetCursorScreenPoint() {
  return Env::GetInstance()->last_mouse_location();
}

bool TestScreen::IsWindowUnderCursor(gfx::NativeWindow window) {
  return GetWindowAtScreenPoint(GetCursorScreenPoint()) == window;
}

gfx::NativeWindow TestScreen::GetWindowAtScreenPoint(const gfx::Point& point) {
  if (!host_ || !host_->window())
    return nullptr;
  return host_->window()->GetEventHandlerForPoint(point);
}

display::Display TestScreen::GetDisplayNearestWindow(
    gfx::NativeWindow window) const {
  return GetPrimaryDisplay();
}

TestScreen::TestScreen(const gfx::Rect& screen_bounds,
                       WindowTreeClient* window_tree_client)
    : host_(nullptr), ui_scale_(1.0f), window_tree_client_(window_tree_client) {
  static int64_t synthesized_display_id = 2000;
  display::Display display(synthesized_display_id++);
  display.SetScaleAndBounds(1.0f, screen_bounds);
  ProcessDisplayChanged(display, true /* is_primary */);
}

}  // namespace aura
