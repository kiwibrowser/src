// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_CURSOR_INFO_H_
#define CONTENT_PUBLIC_COMMON_CURSOR_INFO_H_

#include "build/build_config.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_cursor_info.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

// This struct represents the data sufficient to create a cross-platform cursor:
// either a predefined cursor type (from WebCursorInfo) or custom image.
struct CONTENT_EXPORT CursorInfo {
  explicit CursorInfo(blink::WebCursorInfo::Type cursor_type)
      : type(cursor_type), image_scale_factor(1) {}

  CursorInfo()
      : type(blink::WebCursorInfo::kTypePointer), image_scale_factor(1) {}

  // One of the predefined cursors.
  blink::WebCursorInfo::Type type;

  // Custom cursor image.
  SkBitmap custom_image;

  // Hotspot in custom image in pixels.
  gfx::Point hotspot;

  // The scale factor of custom image, used to possibly re-scale the image
  // for a different density display.
  float image_scale_factor;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_CURSOR_INFO_H_
