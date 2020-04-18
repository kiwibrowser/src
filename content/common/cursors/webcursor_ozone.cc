// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cursors/webcursor.h"

#include "third_party/blink/public/platform/web_cursor_info.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/cursor_util.h"
#include "ui/ozone/public/cursor_factory_ozone.h"

namespace {
const int kDefaultMaxCursorWidth = 64;
const int kDefaultMaxCursorHeight = 64;
}

namespace content {

ui::PlatformCursor WebCursor::GetPlatformCursor() {
  if (platform_cursor_)
    return platform_cursor_;

  SkBitmap bitmap;
  ImageFromCustomData(&bitmap);
  gfx::Point hotspot = hotspot_;

  float scale = device_scale_factor_ / custom_scale_;
  DCHECK_LT(0, maximum_cursor_size_.width());
  DCHECK_LT(0, maximum_cursor_size_.height());
  scale = std::min(
      scale, static_cast<float>(maximum_cursor_size_.width()) / bitmap.width());
  scale = std::min(scale, static_cast<float>(maximum_cursor_size_.height()) /
                              bitmap.height());

  ui::ScaleAndRotateCursorBitmapAndHotpoint(scale, rotation_, &bitmap,
                                            &hotspot);

  platform_cursor_ = ui::CursorFactoryOzone::GetInstance()->CreateImageCursor(
      bitmap, hotspot, scale);
  return platform_cursor_;
}

void WebCursor::SetDisplayInfo(const display::Display& display) {
  if (rotation_ == display.rotation() &&
      device_scale_factor_ == display.device_scale_factor() &&
      maximum_cursor_size_ == display.maximum_cursor_size())
    return;
  device_scale_factor_ = display.device_scale_factor();
  rotation_ = display.rotation();
  maximum_cursor_size_ = display.maximum_cursor_size();
  // TODO(oshima): Identify if it's possible to remove this check here and move
  // the kDefaultMaxCursor{Width,Height} constants to a single place.
  // crbug.com/603512
  if (maximum_cursor_size_.width() == 0 || maximum_cursor_size_.height() == 0)
    maximum_cursor_size_ =
        gfx::Size(kDefaultMaxCursorWidth, kDefaultMaxCursorHeight);
  if (platform_cursor_)
    ui::CursorFactoryOzone::GetInstance()->UnrefImageCursor(platform_cursor_);
  platform_cursor_ = NULL;
  // It is not necessary to recreate platform_cursor_ yet, since it will be
  // recreated on demand when GetPlatformCursor is called.
}

void WebCursor::InitPlatformData() {
  platform_cursor_ = NULL;
  device_scale_factor_ = 1.f;
  rotation_ = display::Display::ROTATE_0;
  maximum_cursor_size_ =
      gfx::Size(kDefaultMaxCursorWidth, kDefaultMaxCursorHeight);
}

bool WebCursor::IsPlatformDataEqual(const WebCursor& other) const {
  return true;
}

void WebCursor::CleanupPlatformData() {
  if (platform_cursor_) {
    ui::CursorFactoryOzone::GetInstance()->UnrefImageCursor(platform_cursor_);
    platform_cursor_ = NULL;
  }
}

void WebCursor::CopyPlatformData(const WebCursor& other) {
  if (platform_cursor_)
    ui::CursorFactoryOzone::GetInstance()->UnrefImageCursor(platform_cursor_);
  platform_cursor_ = other.platform_cursor_;
  if (platform_cursor_)
    ui::CursorFactoryOzone::GetInstance()->RefImageCursor(platform_cursor_);

  device_scale_factor_ = other.device_scale_factor_;
  maximum_cursor_size_ = other.maximum_cursor_size_;
}

}  // namespace content
