// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_DISPLAY_UTIL_H_
#define CONTENT_BROWSER_RENDERER_HOST_DISPLAY_UTIL_H_

#include "content/common/content_export.h"
#include "content/public/common/screen_info.h"
#include "ui/display/display.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

class CONTENT_EXPORT DisplayUtil {
 public:
  static void DisplayToScreenInfo(ScreenInfo* screen_info,
                                  const display::Display& display);

  static void GetNativeViewScreenInfo(ScreenInfo* screen_info,
                                      gfx::NativeView native_view);

  static void GetDefaultScreenInfo(ScreenInfo* screen_info);

  // Compute the orientation type of the display assuming it is a mobile device.
  static ScreenOrientationValues GetOrientationTypeForMobile(
      const display::Display& display);

  // Compute the orientation type of the display assuming it is a desktop.
  static ScreenOrientationValues GetOrientationTypeForDesktop(
      const display::Display& display);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_DISPLAY_UTIL_H_
