// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_DIALOG_CONSTANTS_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_DIALOG_CONSTANTS_H_

// Constants governing layout of autofill dialog.
namespace autofill {

// Horizontal padding between text and other elements (in pixels).
const CGFloat kAroundTextPadding = 4;

// Spacing between buttons.
const CGFloat kButtonGap = 6;

// The space between the edges of a notification bar and the text within (in
// pixels).
const int kNotificationPadding = 17;

// Vertical spacing between legal text and details section.
const int kVerticalSpacing = 8;

// Padding between top bar and details section, as well as between the bottom of
// the details section and the button strip.
const int kDetailVerticalPadding = 10;

// Fixed width for a single input field.
const CGFloat kFieldWidth = 104;

// Horizontal padding between fields
const CGFloat kHorizontalFieldPadding = 8;

// Size of the information icon in the footer.
const CGFloat kInfoIconSize = 16;

}  // autofill

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_DIALOG_CONSTANTS_H_
