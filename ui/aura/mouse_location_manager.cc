// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mouse_location_manager.h"

#include "ui/gfx/geometry/point.h"

namespace aura {

MouseLocationManager::MouseLocationManager() {}

MouseLocationManager::~MouseLocationManager() {}

void MouseLocationManager::SetMouseLocation(const gfx::Point& point_in_dip) {
  current_mouse_location_ = static_cast<base::subtle::Atomic32>(
      (point_in_dip.x() & 0xFFFF) << 16 | (point_in_dip.y() & 0xFFFF));
  if (mouse_location_memory()) {
    base::subtle::NoBarrier_Store(mouse_location_memory(),
                                  current_mouse_location_);
  }
}

mojo::ScopedSharedBufferHandle MouseLocationManager::GetMouseLocationMemory() {
  if (!mouse_location_handle_.is_valid()) {
    // Create our shared memory segment to share the mouse state with our
    // window clients.
    mouse_location_handle_ =
        mojo::SharedBufferHandle::Create(sizeof(base::subtle::Atomic32));

    if (!mouse_location_handle_.is_valid())
      return mojo::ScopedSharedBufferHandle();

    mouse_location_mapping_ =
        mouse_location_handle_->Map(sizeof(base::subtle::Atomic32));
    if (!mouse_location_mapping_)
      return mojo::ScopedSharedBufferHandle();
    base::subtle::NoBarrier_Store(mouse_location_memory(),
                                  current_mouse_location_);
  }

  return mouse_location_handle_->Clone(
      mojo::SharedBufferHandle::AccessMode::READ_ONLY);
}

}  // namespace aura
