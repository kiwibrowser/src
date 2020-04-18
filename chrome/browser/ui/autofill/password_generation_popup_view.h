// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_H_
#define CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_H_

#include "build/build_config.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class Point;
class Size;
}

namespace autofill {

class PasswordGenerationPopupController;

// Interface for creating and controlling a platform dependent view.
class PasswordGenerationPopupView {
 public:
  // Number of pixels added in between lines of the help section.
  static const int kHelpSectionAdditionalSpacing = 3;

  // Display the popup.
  virtual void Show() = 0;

  // This will cause the popup to be deleted.
  virtual void Hide() = 0;

  // Get desired size of the popup.
  virtual gfx::Size GetPreferredSizeOfPasswordView() = 0;

  // Updates layout information from the controller.
  virtual void UpdateBoundsAndRedrawPopup() = 0;

  // Called when the password selection state has changed.
  virtual void PasswordSelectionUpdated() = 0;

  virtual bool IsPointInPasswordBounds(const gfx::Point& point) = 0;

  // Note that PasswordGenerationPopupView owns itself, and will only be deleted
  // when Hide() is called.
  static PasswordGenerationPopupView* Create(
      PasswordGenerationPopupController* controller);
#if defined(OS_MACOSX)
  // Temporary shim for Polychrome. See bottom of first comment in
  // https://crbug.com/804950 for details
  static PasswordGenerationPopupView* CreateCocoa(
      PasswordGenerationPopupController* controller);
#endif

  static const SkColor kPasswordTextColor;
  static const SkColor kExplanatoryTextBackgroundColor;
  static const SkColor kExplanatoryTextColor;
  static const SkColor kDividerColor;
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_H_
