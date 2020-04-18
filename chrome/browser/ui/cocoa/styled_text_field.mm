// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/styled_text_field.h"

#include "base/logging.h"
#import "chrome/browser/ui/cocoa/styled_text_field_cell.h"

@implementation StyledTextField

- (StyledTextFieldCell*)styledTextFieldCell {
  DCHECK([[self cell] isKindOfClass:[StyledTextFieldCell class]]);
  return static_cast<StyledTextFieldCell*>([self cell]);
}

// Cocoa text fields are edited by placing an NSTextView as subview,
// positioned by the cell's -editWithFrame:inView:... method.  Using
// the standard -makeFirstResponder: machinery to reposition the field
// editor results in resetting the field editor's editing state, which
// OmniboxViewMac monitors.  This causes problems because
// editing can require the field editor to be repositioned, which
// could disrupt editing.  This code repositions the subview directly,
// which causes no editing-state changes.
- (void)resetFieldEditorFrameIfNeeded {
  // No action if not editing.
  NSText* editor = [self currentEditor];
  if (!editor) {
    return;
  }

  // When editing, we should have exactly one subview, which is a
  // clipview containing the editor (for purposes of scrolling).
  NSArray* subviews = [self subviews];
  DCHECK_EQ([subviews count], 1U);
  DCHECK([editor isDescendantOf:self]);
  if ([subviews count] == 0) {
    return;
  }

  // If the frame is already right, don't make any visible changes.
  StyledTextFieldCell* cell = [self styledTextFieldCell];
  const NSRect frame([cell drawingRectForBounds:[self bounds]]);
  NSView* subview = [subviews objectAtIndex:0];
  if (NSEqualRects(frame, [subview frame])) {
    return;
  }

  [subview setFrame:frame];

  // Make sure the selection remains visible.
  [editor scrollRangeToVisible:[editor selectedRange]];
}

- (CGFloat)availableDecorationWidth {
  NSAttributedString* as = [self attributedStringValue];
  const NSSize size([as size]);
  const NSRect bounds([self bounds]);
  return NSWidth(bounds) - size.width;
}

- (NSFocusRingType)focusRingType {
  // This is taken care of by the custom drawing code in the cell.
  return NSFocusRingTypeNone;
}

// Due to theming, parts of the field are transparent.
- (BOOL)isOpaque {
  return NO;
}

@end
