// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include "base/mac/availability.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_popup_base_view_cocoa.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

namespace autofill {
class AutofillPopupController;
class AutofillPopupViewCocoaDelegate;
}  // namespace autofill

// Draws the native Autofill popup view on Mac.
@interface AutofillPopupViewCocoa : AutofillPopupBaseViewCocoa {
 @private
  // The cross-platform controller for this view.
  autofill::AutofillPopupController* controller_;  // weak
  // The delegate back to the AutofillPopupViewBridge.
  autofill::AutofillPopupViewCocoaDelegate* delegate_;  // weak
}

// Designated initializer.
- (id)initWithController:(autofill::AutofillPopupController*)controller
                   frame:(NSRect)frame
                delegate:(autofill::AutofillPopupViewCocoaDelegate*)delegate;

// Informs the view that its controller has been (or will imminently be)
// destroyed.
- (void)controllerDestroyed;

- (void)invalidateRow:(NSInteger)row;

@end

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_POPUP_VIEW_COCOA_H_
