// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_A11Y_UTIL_H
#define UI_BASE_COCOA_A11Y_UTIL_H

#import <Cocoa/Cocoa.h>

#include "ui/base/ui_base_export.h"

namespace ui {
namespace a11y_util {

// Hides the given |view| from the accessibility order for Voice Over. This
// should be used when the view provides no additional information with
// voice over (i.e., an icon next to a written description of the icon).
UI_BASE_EXPORT void HideImageFromAccessibilityOrder(NSImageView* view);

// Ask VoiceOver to play a sound for |object|, generally a view or window
// (undocumented). Built-in apps seem to use this to indicate that something
// interesting has happened, like a failed download or available completions.
UI_BASE_EXPORT void PlayElementUpdatedSound(id source);

}  // namespace a11y_util
}  // namespace ui

#endif  // UI_BASE_COCOA_A11Y_UTIL_H
