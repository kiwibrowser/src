// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_STYLED_TEXT_FIELD_H_
#define CHROME_BROWSER_UI_COCOA_STYLED_TEXT_FIELD_H_

#import <Cocoa/Cocoa.h>

@class StyledTextFieldCell;

// An implementation of NSTextField that is designed to work with
// StyledTextFieldCell.  Provides methods to redraw the field when cell
// decorations have changed and overrides |mouseDown:| to properly handle clicks
// in sections of the cell with decorations.
@interface StyledTextField : NSTextField {
}

// Repositions and redraws the field editor.  Call this method when the cell's
// text frame has changed (whenever changing cell decorations).
- (void)resetFieldEditorFrameIfNeeded;

// Returns the amount of the field's width which is not being taken up
// by the text contents.  May be negative if the contents are large
// enough to scroll.
- (CGFloat)availableDecorationWidth;

@end

@interface StyledTextField (ExposedForTesting)
- (StyledTextFieldCell*)styledTextFieldCell;
@end

#endif  // CHROME_BROWSER_UI_COCOA_STYLED_TEXT_FIELD_H_
