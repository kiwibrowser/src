// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_AURALINUX_H_
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_AURALINUX_H_

#include "base/macros.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"

namespace content {
class BrowserAccessibilityAuraLinux;

// Manages a tree of BrowserAccessibilityAuraLinux objects.
class CONTENT_EXPORT BrowserAccessibilityManagerAuraLinux
    : public BrowserAccessibilityManager {
 public:
  BrowserAccessibilityManagerAuraLinux(
      AtkObject* parent_object,
      const ui::AXTreeUpdate& initial_tree,
      BrowserAccessibilityDelegate* delegate,
      BrowserAccessibilityFactory* factory = new BrowserAccessibilityFactory());

  ~BrowserAccessibilityManagerAuraLinux() override;

  static ui::AXTreeUpdate GetEmptyDocument();

  // Implementation of BrowserAccessibilityManager methods.
  void FireFocusEvent(BrowserAccessibility* node) override;
  void FireBlinkEvent(ax::mojom::Event event_type,
                      BrowserAccessibility* node) override;
  void FireGeneratedEvent(AXEventGenerator::Event event_type,
                          BrowserAccessibility* node) override;

  AtkObject* parent_object() { return parent_object_; }

 private:
  AtkObject* parent_object_;

  // Give BrowserAccessibilityManager::Create access to our constructor.
  friend class BrowserAccessibilityManager;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityManagerAuraLinux);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_AURALINUX_H_
