// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/message_center/arc/mock_arc_notification_surface.h"

#include "ui/aura/window.h"

namespace ash {

MockArcNotificationSurface::MockArcNotificationSurface(
    const std::string& notification_key)
    : notification_key_(notification_key),
      ax_tree_id_(-1),
      native_view_host_(nullptr),
      window_(new aura::Window(nullptr)),
      content_window_(new aura::Window(nullptr)) {
  window_->Init(ui::LAYER_NOT_DRAWN);
  content_window_->Init(ui::LAYER_NOT_DRAWN);
}

MockArcNotificationSurface::~MockArcNotificationSurface() = default;

gfx::Size MockArcNotificationSurface::GetSize() const {
  return gfx::Size();
}

aura::Window* MockArcNotificationSurface::GetWindow() const {
  return window_.get();
}

aura::Window* MockArcNotificationSurface::GetContentWindow() const {
  return content_window_.get();
}

const std::string& MockArcNotificationSurface::GetNotificationKey() const {
  return notification_key_;
}

void MockArcNotificationSurface::Attach(
    views::NativeViewHost* native_view_host) {
  native_view_host_ = native_view_host;
}

void MockArcNotificationSurface::Detach() {
  native_view_host_ = nullptr;
}

bool MockArcNotificationSurface::IsAttached() const {
  return native_view_host_ != nullptr;
}

views::NativeViewHost* MockArcNotificationSurface::GetAttachedHost() const {
  return native_view_host_;
}

void MockArcNotificationSurface::FocusSurfaceWindow() {}

void MockArcNotificationSurface::SetAXTreeId(int32_t ax_tree_id) {
  ax_tree_id_ = ax_tree_id;
}

int32_t MockArcNotificationSurface::GetAXTreeId() const {
  return ax_tree_id_;
}

}  // namespace ash
