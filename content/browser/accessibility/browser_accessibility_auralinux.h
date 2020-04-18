// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_AURALINUX_H_
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_AURALINUX_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/common/content_export.h"

namespace ui {
class AXPlatformNodeAuraLinux;
}

namespace content {

class BrowserAccessibilityAuraLinux : public BrowserAccessibility {
 public:
  BrowserAccessibilityAuraLinux();

  ~BrowserAccessibilityAuraLinux() override;

  ui::AXPlatformNodeAuraLinux* GetNode() const;

  // BrowserAccessibility methods.
  void OnDataChanged() override;
  bool IsNative() const override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;

 private:
  // Give BrowserAccessibility::Create access to our constructor.
  friend class BrowserAccessibility;

  ui::AXPlatformNodeAuraLinux* node_;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityAuraLinux);
};

CONTENT_EXPORT BrowserAccessibilityAuraLinux* ToBrowserAccessibilityAuraLinux(
    BrowserAccessibility* obj);

}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_AURALINUX_H_
