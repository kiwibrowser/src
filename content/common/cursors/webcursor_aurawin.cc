// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cursors/webcursor.h"

#include <windows.h>

#include "third_party/blink/public/platform/web_cursor_info.h"
#include "ui/gfx/icon_util.h"

namespace content {

ui::PlatformCursor WebCursor::GetPlatformCursor() {
  if (!IsCustom())
    return LoadCursor(NULL, IDC_ARROW);

  if (custom_cursor_)
    return custom_cursor_;

  SkBitmap bitmap;
  gfx::Point hotspot;
  CreateScaledBitmapAndHotspotFromCustomData(&bitmap, &hotspot);

  gfx::Size custom_size;
  std::vector<char> custom_data;
  CreateCustomData(bitmap, &custom_data, &custom_size);

  custom_cursor_ = IconUtil::CreateCursorFromDIB(
      custom_size,
      hotspot,
      !custom_data.empty() ? &custom_data[0] : NULL,
      custom_data.size())
      .release();
  return custom_cursor_;
}

void WebCursor::InitPlatformData() {
  custom_cursor_ = NULL;
  device_scale_factor_ = 1.f;
}

bool WebCursor::IsPlatformDataEqual(const WebCursor& other) const {
  return true;
}

void WebCursor::CleanupPlatformData() {
  if (custom_cursor_) {
    DestroyIcon(custom_cursor_);
    custom_cursor_ = NULL;
  }
}

void WebCursor::CopyPlatformData(const WebCursor& other) {
  device_scale_factor_ = other.device_scale_factor_;
}

}  // namespace content
