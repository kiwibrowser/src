// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field_cell.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"

@implementation FindBarTextField

+ (Class)cellClass {
  return [FindBarTextFieldCell class];
}

- (void)awakeFromNib {
  DCHECK([[self cell] isKindOfClass:[FindBarTextFieldCell class]]);

  [self registerForDraggedTypes:
          [NSArray arrayWithObjects:NSStringPboardType, nil]];
}

- (FindBarTextFieldCell*)findBarTextFieldCell {
  DCHECK([[self cell] isKindOfClass:[FindBarTextFieldCell class]]);
  return static_cast<FindBarTextFieldCell*>([self cell]);
}

- (ViewID)viewID {
  return VIEW_ID_FIND_IN_PAGE_TEXT_FIELD;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  // When a drag enters the text field, focus the field.  This will swap in the
  // field editor, which will then handle the drag itself.
  [[self window] makeFirstResponder:self];
  return NSDragOperationNone;
}

// Disable default automated replacements, see <http://crbug.com/173405>.
- (void)textDidBeginEditing:(NSNotification*)aNotification {
  // NSTextDidBeginEditingNotification is from NSText, but this only
  // applies to NSTextView instances.
  NSTextView* textView =
      base::mac::ObjCCast<NSTextView>([aNotification object]);
  NSTextCheckingTypes checkingTypes = [textView enabledTextCheckingTypes];
  checkingTypes &= ~NSTextCheckingTypeReplacement;
  checkingTypes &= ~NSTextCheckingTypeCorrection;
  [textView setEnabledTextCheckingTypes:checkingTypes];
}

// Implemented to allow the findbar to respond to "Paste and Match Style" menu
// commands.
- (void)pasteAndMatchStyle:(id)sender {
  [[self currentEditor] paste:sender];
}

@end
