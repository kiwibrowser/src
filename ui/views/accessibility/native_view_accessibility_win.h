// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_WIN_H_
#define UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_WIN_H_

#include "base/macros.h"
#include "ui/views/accessibility/native_view_accessibility_base.h"
#include "ui/views/view.h"

namespace views {

class NativeViewAccessibilityWin : public NativeViewAccessibilityBase {
 public:
  NativeViewAccessibilityWin(View* view);
  ~NativeViewAccessibilityWin() override;

  // NativeViewAccessibilityBase:
  gfx::NativeViewAccessible GetParent() override;
  gfx::AcceleratedWidget GetTargetForNativeAccessibilityEvent() override;
  gfx::Rect GetClippedScreenBoundsRect() const override;
  gfx::Rect GetUnclippedScreenBoundsRect() const override;

  DISALLOW_COPY_AND_ASSIGN(NativeViewAccessibilityWin);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_WIN_H_
