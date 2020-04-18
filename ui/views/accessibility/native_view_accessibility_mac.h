// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_MAC_H_
#define UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_MAC_H_

#include "base/macros.h"
#include "ui/views/accessibility/native_view_accessibility_base.h"

namespace views {

// Mac-specific accessibility class for NativeViewAccessibility.
class NativeViewAccessibilityMac : public NativeViewAccessibilityBase {
 public:
  explicit NativeViewAccessibilityMac(View* view);

  // NativeViewAccessibilityBase:
  gfx::NativeViewAccessible GetParent() override;

  DISALLOW_COPY_AND_ASSIGN(NativeViewAccessibilityMac);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_MAC_H_
