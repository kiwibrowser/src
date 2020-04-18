// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_TOUCH_TOUCH_DEVICE_H_
#define UI_BASE_TOUCH_TOUCH_DEVICE_H_

#include <tuple>

#include "build/build_config.h"
#include "ui/base/ui_base_export.h"

#if defined(OS_ANDROID)
#include <jni.h>
#endif

// TODO(mustaq@chromium.org): This covers more than just touches. Rename to
// input_device? crbug.com/438794

namespace ui {

enum class TouchScreensAvailability {
  NONE,      // No touch screens are present.
  ENABLED,   // Touch screens are present and enabled.
  DISABLED,  // Touch screens are present and disabled.
};

UI_BASE_EXPORT TouchScreensAvailability GetTouchScreensAvailability();

// Returns the maximum number of simultaneous touch contacts supported
// by the device. In the case of devices with multiple digitizers (e.g.
// multiple touchscreens), the value MUST be the maximum of the set of
// maximum supported contacts by each individual digitizer.
// For example, suppose a device has 3 touchscreens, which support 2, 5,
// and 10 simultaneous touch contacts, respectively. This returns 10.
// http://www.w3.org/TR/pointerevents/#widl-Navigator-maxTouchPoints
UI_BASE_EXPORT int MaxTouchPoints();

// Bit field values indicating available pointer types. Identical to
// blink::PointerType enums, enforced by compile-time assertions in
// content/public/common/web_preferences.cc .
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.ui.base
// GENERATED_JAVA_PREFIX_TO_STRIP: POINTER_TYPE_
enum PointerType {
  POINTER_TYPE_NONE = 1 << 0,
  POINTER_TYPE_FIRST = POINTER_TYPE_NONE,
  POINTER_TYPE_COARSE = 1 << 1,
  POINTER_TYPE_FINE = 1 << 2,
  POINTER_TYPE_LAST = POINTER_TYPE_FINE
};

// Bit field values indicating available hover types. Identical to
// blink::HoverType enums, enforced by compile-time assertions in
// content/public/common/web_preferences.cc .
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.ui.base
// GENERATED_JAVA_PREFIX_TO_STRIP: HOVER_TYPE_
enum HoverType {
  HOVER_TYPE_NONE = 1 << 0,
  HOVER_TYPE_FIRST = HOVER_TYPE_NONE,
  HOVER_TYPE_HOVER = 1 << 1,
  HOVER_TYPE_LAST = HOVER_TYPE_HOVER
};

int GetAvailablePointerTypes();
int GetAvailableHoverTypes();
UI_BASE_EXPORT std::pair<int, int> GetAvailablePointerAndHoverTypes();
UI_BASE_EXPORT void SetAvailablePointerAndHoverTypesForTesting(
    int available_pointer_types,
    int available_hover_types);
UI_BASE_EXPORT PointerType GetPrimaryPointerType(int available_pointer_types);
UI_BASE_EXPORT HoverType GetPrimaryHoverType(int available_hover_types);

}  // namespace ui

#endif  // UI_BASE_TOUCH_TOUCH_DEVICE_H_
