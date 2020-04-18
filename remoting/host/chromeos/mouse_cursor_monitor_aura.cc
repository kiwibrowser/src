// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/chromeos/mouse_cursor_monitor_aura.h"

#include <utility>

#include "ash/shell.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "remoting/host/chromeos/skia_bitmap_desktop_frame.h"
#include "third_party/webrtc/modules/desktop_capture/mouse_cursor.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/cursors_aura.h"

namespace {

// Creates an empty webrtc::MouseCursor. The caller is responsible for
// destroying the returned cursor.
webrtc::MouseCursor* CreateEmptyMouseCursor() {
  return new webrtc::MouseCursor(
      new webrtc::BasicDesktopFrame(webrtc::DesktopSize(0, 0)),
      webrtc::DesktopVector(0, 0));
}

}  // namespace

namespace remoting {

MouseCursorMonitorAura::MouseCursorMonitorAura()
    : callback_(nullptr),
      mode_(SHAPE_AND_POSITION) {
}

void MouseCursorMonitorAura::Init(Callback* callback, Mode mode) {
  DCHECK(!callback_);
  DCHECK(callback);

  callback_ = callback;
  mode_ = mode;
}

void MouseCursorMonitorAura::Capture() {
  // Check if the cursor is different.
  gfx::NativeCursor cursor =
      ash::Shell::GetPrimaryRootWindow()->GetHost()->last_cursor();

  if (cursor != last_cursor_) {
    last_cursor_ = cursor;
    NotifyCursorChanged(cursor);
  }

  // Check if we need to update the location.
  if (mode_ == SHAPE_AND_POSITION) {
    gfx::Point position = aura::Env::GetInstance()->last_mouse_location();
    if (position != last_mouse_location_) {
      last_mouse_location_ = position;
      callback_->OnMouseCursorPosition(
          INSIDE, webrtc::DesktopVector(position.x(), position.y()));
    }
  }
}

void MouseCursorMonitorAura::NotifyCursorChanged(const ui::Cursor& cursor) {
  if (cursor.native_type() == ui::CursorType::kNone) {
    callback_->OnMouseCursor(CreateEmptyMouseCursor());
    return;
  }

  std::unique_ptr<SkBitmap> cursor_bitmap =
      std::make_unique<SkBitmap>(cursor.GetBitmap());
  gfx::Point cursor_hotspot = cursor.GetHotspot();

  if (cursor_bitmap->isNull()) {
    LOG(ERROR) << "Failed to load bitmap for cursor type:"
               << static_cast<int>(cursor.native_type());
    callback_->OnMouseCursor(CreateEmptyMouseCursor());
    return;
  }

  // There is a bug (crbug.com/436993) in aura::GetCursorBitmap() such that it
  // it would return a scale-factor-100 bitmap with a scale-factor-200 hotspot.
  // This causes the hotspot to go out of range.  As a result, we would need to
  // manually downscale the hotspot.
  float scale_factor = cursor.device_scale_factor();
  cursor_hotspot.SetPoint(cursor_hotspot.x() / scale_factor,
                          cursor_hotspot.y() / scale_factor);

  if (cursor_hotspot.x() >= cursor_bitmap->width() ||
      cursor_hotspot.y() >= cursor_bitmap->height()) {
    LOG(WARNING) << "Cursor hotspot is out of bounds for type: "
                 << static_cast<int>(cursor.native_type())
                 << ".  Setting to (0,0) instead";
    cursor_hotspot.SetPoint(0, 0);
  }

  std::unique_ptr<webrtc::DesktopFrame> image(
      SkiaBitmapDesktopFrame::Create(std::move(cursor_bitmap)));
  std::unique_ptr<webrtc::MouseCursor> cursor_shape(new webrtc::MouseCursor(
      image.release(),
      webrtc::DesktopVector(cursor_hotspot.x(), cursor_hotspot.y())));

  callback_->OnMouseCursor(cursor_shape.release());
}

}  // namespace remoting
