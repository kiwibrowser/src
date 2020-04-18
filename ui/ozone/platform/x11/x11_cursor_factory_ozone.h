// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_X11_X11_CURSOR_FACTORY_OZONE_H_
#define UI_OZONE_PLATFORM_X11_X11_CURSOR_FACTORY_OZONE_H_

#include <map>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/x/x11.h"
#include "ui/ozone/platform/x11/x11_cursor_ozone.h"
#include "ui/ozone/public/cursor_factory_ozone.h"

namespace ui {

// CursorFactoryOzone implementation for X11 cursors.
class X11CursorFactoryOzone : public CursorFactoryOzone {
 public:
  X11CursorFactoryOzone();
  ~X11CursorFactoryOzone() override;

  // CursorFactoryOzone:
  PlatformCursor GetDefaultCursor(CursorType type) override;
  PlatformCursor CreateImageCursor(const SkBitmap& bitmap,
                                   const gfx::Point& hotspot,
                                   float bitmap_dpi) override;
  PlatformCursor CreateAnimatedCursor(const std::vector<SkBitmap>& bitmaps,
                                      const gfx::Point& hotspot,
                                      int frame_delay_ms,
                                      float bitmap_dpi) override;
  void RefImageCursor(PlatformCursor cursor) override;
  void UnrefImageCursor(PlatformCursor cursor) override;

 private:
  // Loads/caches default cursor or returns cached version.
  scoped_refptr<X11CursorOzone> GetDefaultCursorInternal(CursorType type);

  // Holds a single instance of the invisible cursor. X11 has no way to hide
  // the cursor so an invisible cursor mimics that.
  scoped_refptr<X11CursorOzone> invisible_cursor_;

  std::map<CursorType, scoped_refptr<X11CursorOzone>> default_cursors_;

  DISALLOW_COPY_AND_ASSIGN(X11CursorFactoryOzone);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_X11_X11_CURSOR_FACTORY_OZONE_H_
