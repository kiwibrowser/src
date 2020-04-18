// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_DEFAULT_ACCESSIBILITY_DELEGATE_H_
#define ASH_ACCESSIBILITY_DEFAULT_ACCESSIBILITY_DELEGATE_H_

#include "ash/accessibility/accessibility_delegate.h"
#include "ash/ash_export.h"
#include "ash/public/cpp/accessibility_types.h"
#include "base/macros.h"

namespace ash {

class ASH_EXPORT DefaultAccessibilityDelegate : public AccessibilityDelegate {
 public:
  DefaultAccessibilityDelegate();
  ~DefaultAccessibilityDelegate() override;

  void SetMagnifierEnabled(bool enabled) override;
  bool IsMagnifierEnabled() const override;
  bool ShouldShowAccessibilityMenu() const override;
  void SaveScreenMagnifierScale(double scale) override;
  double GetSavedScreenMagnifierScale() override;

 private:
  bool screen_magnifier_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(DefaultAccessibilityDelegate);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_DEFAULT_ACCESSIBILITY_DELEGATE_H_
