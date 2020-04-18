// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TEST_STYLED_TEXT_FIELD_TEST_HELPER_H_
#define CHROME_BROWSER_UI_COCOA_TEST_STYLED_TEXT_FIELD_TEST_HELPER_H_

#import <Cocoa/Cocoa.h>
#import "chrome/browser/ui/cocoa/styled_text_field_cell.h"

// Subclass of StyledTextFieldCell that allows you to slice off sections on the
// left and right of the cell.
@interface StyledTextFieldTestCell : StyledTextFieldCell {
  CGFloat leftMargin_;
  CGFloat rightMargin_;
}
@property(nonatomic, assign) CGFloat leftMargin;
@property(nonatomic, assign) CGFloat rightMargin;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TEST_STYLED_TEXT_FIELD_TEST_HELPER_H_
