// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/cursor_location_manager.h"

#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws {

CursorLocationManager::CursorLocationManager() {}

CursorLocationManager::~CursorLocationManager() {}

void CursorLocationManager::OnMouseCursorLocationChanged(
    const gfx::Point& point_in_dip) {
  current_cursor_location_ = static_cast<base::subtle::Atomic32>(
      (point_in_dip.x() & 0xFFFF) << 16 | (point_in_dip.y() & 0xFFFF));
  if (cursor_location_memory()) {
    base::subtle::NoBarrier_Store(cursor_location_memory(),
                                  current_cursor_location_);
  }
}

mojo::ScopedSharedBufferHandle
CursorLocationManager::GetCursorLocationMemory() {
  if (!cursor_location_handle_.is_valid()) {
    // Create our shared memory segment to share the cursor state with our
    // window clients.
    cursor_location_handle_ =
        mojo::SharedBufferHandle::Create(sizeof(base::subtle::Atomic32));

    if (!cursor_location_handle_.is_valid())
      return mojo::ScopedSharedBufferHandle();

    cursor_location_mapping_ =
        cursor_location_handle_->Map(sizeof(base::subtle::Atomic32));
    if (!cursor_location_mapping_)
      return mojo::ScopedSharedBufferHandle();
    base::subtle::NoBarrier_Store(cursor_location_memory(),
                                  current_cursor_location_);
  }

  return cursor_location_handle_->Clone(
      mojo::SharedBufferHandle::AccessMode::READ_ONLY);
}

}  // namespace ws
}  // namespace ui
