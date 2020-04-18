// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_H_
#define UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_H_

#include <memory>

#include "base/macros.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"

namespace views {

class View;

// Abstract base for that allows native platform accessibility toolkits to
// interface with a View.
class VIEWS_EXPORT NativeViewAccessibility {
 public:
  static std::unique_ptr<NativeViewAccessibility> Create(View* view);

  virtual ~NativeViewAccessibility() {}

  virtual gfx::NativeViewAccessible GetNativeObject() = 0;
  virtual void NotifyAccessibilityEvent(ax::mojom::Event event_type) = 0;

 protected:
  NativeViewAccessibility() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeViewAccessibility);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_NATIVE_VIEW_ACCESSIBILITY_H_
