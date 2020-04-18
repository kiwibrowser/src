// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_BRIDGE_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/ui/autofill/password_generation_popup_view.h"

@class PasswordGenerationPopupViewCocoa;

namespace autofill {

// Mac implementation for PasswordGenerationPopupView interface.
// Serves as a bridge to an instance of the Objective-C class which actually
// implements the view.
class PasswordGenerationPopupViewBridge : public PasswordGenerationPopupView {
 public:
  explicit PasswordGenerationPopupViewBridge(
      PasswordGenerationPopupController* controller);

 private:
  virtual ~PasswordGenerationPopupViewBridge();

  // PasswordGenerationPopupView implementation.
  void Hide() override;
  void Show() override;
  gfx::Size GetPreferredSizeOfPasswordView() override;
  void UpdateBoundsAndRedrawPopup() override;
  void PasswordSelectionUpdated() override;
  bool IsPointInPasswordBounds(const gfx::Point& point) override;

  // The native Cocoa view.
  base::scoped_nsobject<PasswordGenerationPopupViewCocoa> view_;

  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationPopupViewBridge);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_BRIDGE_H_
