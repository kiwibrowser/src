// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_TOOLTOP_MANAGER_MAC_H_
#define UI_VIEWS_COCOA_TOOLTOP_MANAGER_MAC_H_

#include "base/macros.h"
#include "ui/views/widget/tooltip_manager.h"

namespace views {
class BridgedNativeWidget;

// Manages native Cocoa tooltips for the given BridgedNativeWidget.
class TooltipManagerMac : public TooltipManager {
 public:
  explicit TooltipManagerMac(BridgedNativeWidget* widget);
  ~TooltipManagerMac() override;

  // TooltipManager:
  int GetMaxWidth(const gfx::Point& location) const override;
  const gfx::FontList& GetFontList() const override;
  void UpdateTooltip() override;
  void TooltipTextChanged(View* view) override;

 private:
  BridgedNativeWidget* widget_;  // Weak. Owns this.

  DISALLOW_COPY_AND_ASSIGN(TooltipManagerMac);
};

}  // namespace views

#endif  // UI_VIEWS_COCOA_TOOLTOP_MANAGER_MAC_H_
