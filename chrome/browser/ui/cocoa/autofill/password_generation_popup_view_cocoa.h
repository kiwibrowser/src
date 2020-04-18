// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/autofill/password_generation_popup_controller.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_popup_base_view_cocoa.h"
#import "ui/base/cocoa/tracking_area.h"

@class HyperlinkTextView;

// Draws the native password generation popup view on Mac.
@interface PasswordGenerationPopupViewCocoa
    : AutofillPopupBaseViewCocoa <NSTextViewDelegate> {
 @private
  // The cross-platform controller for this view.
  autofill::PasswordGenerationPopupController* controller_;  // weak

  base::scoped_nsobject<NSView> passwordSection_;
  base::scoped_nsobject<NSTextField> passwordField_;
  base::scoped_nsobject<NSTextField> passwordTitleField_;
  base::scoped_nsobject<NSImageView> keyIcon_;
  base::scoped_nsobject<NSBox> divider_;
  base::scoped_nsobject<HyperlinkTextView> helpTextView_;
  ui::ScopedCrTrackingArea helpTextTrackingArea_;
}

// Designated initializer.
- (id)initWithController:
    (autofill::PasswordGenerationPopupController*)controller
                   frame:(NSRect)frame;

// Determines whether |point| falls inside the password section of the popup.
// |point| needs to be in the popup's coordinate system.
- (BOOL)isPointInPasswordBounds:(NSPoint)point;

// Informs the view that its controller has been (or will imminently be)
// destroyed.
- (void)controllerDestroyed;

// The preferred size for the popup.
- (NSSize)preferredSize;

@end

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_COCOA_H_
