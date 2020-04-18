// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/cursor.h"

#include "ui/ozone/public/cursor_factory_ozone.h"

namespace ui {

void Cursor::RefCustomCursor() {
  if (platform_cursor_)
    CursorFactoryOzone::GetInstance()->RefImageCursor(platform_cursor_);
}

void Cursor::UnrefCustomCursor() {
  if (platform_cursor_)
    CursorFactoryOzone::GetInstance()->UnrefImageCursor(platform_cursor_);
}

}  // namespace ui
