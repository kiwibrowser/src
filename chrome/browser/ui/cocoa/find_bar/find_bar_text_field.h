// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_TEXT_FIELD_H_
#define CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_TEXT_FIELD_H_

#import <Cocoa/Cocoa.h>
#import "chrome/browser/ui/cocoa/styled_text_field.h"

@class FindBarTextFieldCell;

// TODO(rohitrao): This class may not need to exist, since it does not really
// add any functionality over StyledTextField.  See if we can change the nib
// file to put a FindBarTextFieldCell into a StyledTextField.

// Extends StyledTextField to use a custom cell class (FindBarTextFieldCell).
@interface FindBarTextField : StyledTextField {
}

// Convenience method to return the cell, casted appropriately.
- (FindBarTextFieldCell*)findBarTextFieldCell;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_TEXT_FIELD_H_
