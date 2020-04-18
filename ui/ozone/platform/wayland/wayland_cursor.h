// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_CURSOR_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_CURSOR_H_

#include <wayland-client.h>
#include <vector>

#include "base/macros.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/geometry/size.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace base {
class SharedMemory;
}

namespace gfx {
class Point;
}

namespace ui {

class WaylandConnection;

// The WaylandCursor class wraps the actual visual representation
// (what users see drawn) of a wl_pointer.
//
// 'pointer' is the Wayland terminology for mouse/mice.
class WaylandCursor {
 public:
  WaylandCursor();
  ~WaylandCursor();

  void Init(wl_pointer* pointer, WaylandConnection* connection);

  // Updates wl_pointer's visual representation with the given bitmap
  // image set, at the hotspot specified by 'location'.
  void UpdateBitmap(const std::vector<SkBitmap>& bitmaps,
                    const gfx::Point& location,
                    uint32_t serial);

 private:
  bool CreateSHMBuffer(const gfx::Size& size);
  void HideCursor(uint32_t serial);

  wl_shm* shm_ = nullptr;                // Owned by WaylandConnection.
  wl_pointer* input_pointer_ = nullptr;  // Owned by WaylandPointer.

  wl::Object<wl_buffer> buffer_;
  wl::Object<wl_surface> pointer_surface_;

  std::unique_ptr<base::SharedMemory> shared_memory_;
  sk_sp<SkSurface> sk_surface_;

  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(WaylandCursor);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_CURSOR_H_
