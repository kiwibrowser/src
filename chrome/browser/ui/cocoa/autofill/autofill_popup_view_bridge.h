// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_BRIDGE_H_

#include <stddef.h>

#include <vector>

#include "base/compiler_specific.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/cocoa/autofill/autofill_popup_view_cocoa.h"

@class AutofillPopupViewCocoa;
@class NSWindow;

namespace autofill {

class AutofillPopupViewCocoaDelegate {
 public:
  // Returns the bounds of the item at |index| in the popup, relative to
  // the top left of the popup.
  virtual gfx::Rect GetRowBounds(int index) = 0;

  // Gets the resource value for the given resource, returning -1 if the
  // resource isn't recognized.
  virtual int GetIconResourceID(const base::string16& resource_name) = 0;
};

// Mac implementation of the AutofillPopupView interface.
// Serves as a bridge to an instance of the Objective-C class which actually
// implements the view.
class AutofillPopupViewBridge : public AutofillPopupView,
                                public AutofillPopupViewCocoaDelegate {
 public:
  explicit AutofillPopupViewBridge(AutofillPopupController* controller);

  // AutofillPopupViewCocoaDelegate implementation.
  gfx::Rect GetRowBounds(int index) override;
  int GetIconResourceID(const base::string16& resource_name) override;

 private:
  ~AutofillPopupViewBridge() override;

  // AutofillPopupView implementation.
  void Hide() override;
  void Show() override;
  void OnSelectedRowChanged(base::Optional<int> previous_row_selection,
                            base::Optional<int> current_row_selection) override;
  void OnSuggestionsChanged() override;

  // Set the initial bounds of the popup, including its placement.
  void SetInitialBounds();

  // The native Cocoa view.
  base::scoped_nsobject<AutofillPopupViewCocoa> view_;

  AutofillPopupController* controller_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupViewBridge);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_BRIDGE_H_
