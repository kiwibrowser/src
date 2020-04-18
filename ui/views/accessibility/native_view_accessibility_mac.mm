// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/native_view_accessibility_mac.h"

#include <memory>

#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

// static
std::unique_ptr<ViewAccessibility> ViewAccessibility::Create(View* view) {
  return std::make_unique<NativeViewAccessibilityMac>(view);
}

NativeViewAccessibilityMac::NativeViewAccessibilityMac(View* view)
    : NativeViewAccessibilityBase(view) {}

gfx::NativeViewAccessible NativeViewAccessibilityMac::GetParent() {
  if (view()->parent())
    return view()->parent()->GetNativeViewAccessible();

  if (view()->GetWidget())
    return view()->GetWidget()->GetNativeView();

  return nullptr;
}

}  // namespace views
