// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_UNITTEST_HELPER_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_UNITTEST_HELPER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field.h"
#include "testing/gmock/include/gmock/gmock.h"

@class AutocompleteTextFieldEditor;

// Return the right field editor for AutocompleteTextField instance.

@interface AutocompleteTextFieldWindowTestDelegate :
    NSObject<NSWindowDelegate> {
  base::scoped_nsobject<AutocompleteTextFieldEditor> editor_;
}
- (id)windowWillReturnFieldEditor:(NSWindow *)sender toObject:(id)anObject;
@end

// Allow monitoring calls into AutocompleteTextField's observer.
// Being in a .h file with an anonymous namespace is strange, but this
// is here so the mock interface doesn't have to change in multiple
// places.

// Any method you add here needs a unit test.  You knew that.

class MockAutocompleteTextFieldObserver : public AutocompleteTextFieldObserver {
 public:
  MockAutocompleteTextFieldObserver();
  ~MockAutocompleteTextFieldObserver();

  MOCK_METHOD1(SelectionRangeForProposedRange, NSRange(NSRange range));
  MOCK_METHOD1(OnControlKeyChanged, void(bool pressed));
  MOCK_METHOD0(CanCopy, bool());
  MOCK_METHOD0(CreatePasteboardItem, base::scoped_nsobject<NSPasteboardItem>());
  MOCK_METHOD1(CopyToPasteboard, void(NSPasteboard* pboard));
  MOCK_METHOD0(OnPaste, void());
  MOCK_METHOD0(CanPasteAndGo, bool());
  MOCK_METHOD0(GetPasteActionStringId, int());
  MOCK_METHOD0(OnPasteAndGo, void());
  MOCK_METHOD0(OnFrameChanged, void());
  MOCK_METHOD0(ClosePopup, void());
  MOCK_METHOD0(OnDidBeginEditing, void());
  MOCK_METHOD0(OnBeforeChange, void());
  MOCK_METHOD0(OnDidChange, void());
  MOCK_METHOD0(OnDidEndEditing, void());
  MOCK_METHOD0(OnInsertText, void());
  MOCK_METHOD0(OnBeforeDrawRect, void());
  MOCK_METHOD0(OnDidDrawRect, void());
  MOCK_METHOD1(OnDoCommandBySelector, bool(SEL cmd));
  MOCK_METHOD1(OnSetFocus, void(bool control_down));
  MOCK_METHOD0(OnKillFocus, void());
  MOCK_METHOD1(OnMouseDown, void(NSInteger button_number));
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_AUTOCOMPLETE_TEXT_FIELD_UNITTEST_HELPER_H_
