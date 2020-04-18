// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_editor.h"

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "chrome/app/chrome_command_ids.h"  // IDC_*
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_unittest_helper.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/grit/generated_resources.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#import "third_party/ocmock/ocmock_extensions.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#import "ui/events/test/cocoa_test_event_utils.h"

using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::StrictMock;
using ::testing::A;

namespace {

NSMenu* MenuFromRightClick(AutocompleteTextFieldEditor* editor) {
  NSEvent* event = cocoa_test_event_utils::MouseEventAtPoint(
      NSZeroPoint, NSRightMouseDown, 0);
  return [editor menuForEvent:event];
}

// TODO(shess): Very similar to AutocompleteTextFieldTest.  Maybe
// those can be shared.

class AutocompleteTextFieldEditorTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<AutocompleteTextField> field(
        [[AutocompleteTextField alloc] initWithFrame:frame]);
    field_ = field.get();
    [field_ setStringValue:@"Testing"];
    [[test_window() contentView] addSubview:field_];

    // Arrange for |field_| to get the right field editor.
    window_delegate_.reset(
        [[AutocompleteTextFieldWindowTestDelegate alloc] init]);
    [test_window() setDelegate:window_delegate_.get()];

    // Get the field editor setup.
    [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
    editor_ = static_cast<AutocompleteTextFieldEditor*>([field_ currentEditor]);

    EXPECT_TRUE(editor_);
    EXPECT_TRUE([editor_ isKindOfClass:[AutocompleteTextFieldEditor class]]);
  }

  AutocompleteTextFieldEditor* editor_;
  AutocompleteTextField* field_;
  base::scoped_nsobject<AutocompleteTextFieldWindowTestDelegate>
      window_delegate_;
};

// Disabled because it crashes sometimes. http://crbug.com/49522
// Can't rename DISABLED_ because the TEST_VIEW macro prepends.
//     http://crbug.com/53621
#if 0
TEST_VIEW(AutocompleteTextFieldEditorTest, field_);
#endif

// Test that control characters are stripped from insertions.
TEST_F(AutocompleteTextFieldEditorTest, InsertStripsControlChars) {
  // Sets a string in the field.
  NSString* test_string = @"astring";
  [field_ setStringValue:test_string];
  [editor_ selectAll:nil];

  [editor_ insertText:@"t"];
  EXPECT_NSEQ(@"t", [field_ stringValue]);

  [editor_ insertText:@"h"];
  EXPECT_NSEQ(@"th", [field_ stringValue]);

  // TAB doesn't get inserted.
  [editor_ insertText:[NSString stringWithFormat:@"%c", 7]];
  EXPECT_NSEQ(@"th", [field_ stringValue]);

  // Newline doesn't get inserted.
  [editor_ insertText:[NSString stringWithFormat:@"%c", 12]];
  EXPECT_NSEQ(@"th", [field_ stringValue]);

  // Multi-character strings get through.
  [editor_ insertText:[NSString stringWithFormat:@"i%cs%c", 8, 127]];
  EXPECT_NSEQ(@"this", [field_ stringValue]);

  // Attempting to insert newline when everything is selected clears
  // the field.
  [editor_ selectAll:nil];
  [editor_ insertText:[NSString stringWithFormat:@"%c", 12]];
  EXPECT_NSEQ(@"", [field_ stringValue]);
}

// Test that |delegate| can provide page action menus.
TEST_F(AutocompleteTextFieldEditorTest, PageActionMenus) {
  // The event just needs to be something the mock can recognize.
  NSEvent* event = cocoa_test_event_utils::MouseEventAtPoint(NSZeroPoint,
                                                             NSRightMouseDown,
                                                             0);

  // Trivial menu which we can recognize and which doesn't look like
  // the default editor context menu.
  base::scoped_nsobject<id> menu([[NSMenu alloc] initWithTitle:@"Menu"]);
  [menu addItemWithTitle:@"Go Fish"
                  action:@selector(goFish:)
           keyEquivalent:@""];

  // So that we don't have to mock the observer.
  [editor_ setEditable:NO];

  // The delegate's intercept point gets called, and results are
  // propagated back.
  {
    id delegate = [OCMockObject mockForClass:[AutocompleteTextField class]];
    [[[delegate stub] andReturnBool:YES]
      isKindOfClass:[AutocompleteTextField class]];
    [[[delegate expect] andReturn:menu.get()] decorationMenuForEvent:event];
    [[[delegate expect] andReturn:nil] undoManagerForTextView:editor_];
    [editor_ setDelegate:delegate];
    NSMenu* contextMenu = [editor_ menuForEvent:event];
    EXPECT_OCMOCK_VERIFY(delegate);
    [editor_ setDelegate:nil];

    EXPECT_EQ(contextMenu, menu.get());
  }

  // If the delegate does not return any menu, the default menu is
  // returned.
  {
    id delegate = [OCMockObject mockForClass:[AutocompleteTextField class]];
    [[[delegate stub] andReturnBool:YES]
      isKindOfClass:[AutocompleteTextField class]];
    [[[delegate expect] andReturn:nil] decorationMenuForEvent:event];
    [[[delegate expect] andReturn:nil] undoManagerForTextView:editor_];
    [editor_ setDelegate:delegate];
    NSMenu* contextMenu = [editor_ menuForEvent:event];
    EXPECT_OCMOCK_VERIFY(delegate);
    [editor_ setDelegate:nil];

    EXPECT_NE(contextMenu, menu.get());
    NSArray* items = [contextMenu itemArray];
    ASSERT_GT([items count], 0U);
    EXPECT_EQ(@selector(cut:), [[items objectAtIndex:0] action])
        << "action is: " << sel_getName([[items objectAtIndex:0] action]);
  }
}

// Base class for testing AutocompleteTextFieldObserver messages.
class AutocompleteTextFieldEditorObserverTest
    : public AutocompleteTextFieldEditorTest {
 public:
  virtual void SetUp() {
    AutocompleteTextFieldEditorTest::SetUp();
    [field_ setObserver:&field_observer_];
  }

  virtual void TearDown() {
    // Clear the observer so that we don't show output for
    // uninteresting messages to the mock (for instance, if |field_| has
    // focus at the end of the test).
    [field_ setObserver:NULL];

    AutocompleteTextFieldEditorTest::TearDown();
  }

  StrictMock<MockAutocompleteTextFieldObserver> field_observer_;
};

// Test that the field editor is linked in correctly.
TEST_F(AutocompleteTextFieldEditorTest, FirstResponder) {
  EXPECT_EQ(editor_, [field_ currentEditor]);
  EXPECT_TRUE([editor_ isDescendantOf:field_]);
  EXPECT_EQ([editor_ delegate], field_);
  EXPECT_EQ([editor_ observer], [field_ observer]);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(AutocompleteTextFieldEditorTest, Display) {
  [field_ display];
  [editor_ display];
}

// Tests that the Touch Bar is nothing.
TEST_F(AutocompleteTextFieldEditorTest, TouchBarTest) {
  if (@available(macOS 10.12.2, *)) {
    NSTouchBar* touch_bar = [field_ performSelector:@selector(touchBar)];
    EXPECT_FALSE(touch_bar);
  }
}

// Test that -paste: is correctly delegated to the observer.
TEST_F(AutocompleteTextFieldEditorObserverTest, Paste) {
  EXPECT_CALL(field_observer_, OnPaste());
  [editor_ paste:nil];
}

// Test that -copy: is correctly delegated to the observer.
TEST_F(AutocompleteTextFieldEditorObserverTest, Copy) {
  EXPECT_CALL(field_observer_, CanCopy())
      .WillOnce(Return(true));
  EXPECT_CALL(field_observer_, CopyToPasteboard(A<NSPasteboard*>()))
      .Times(1);
  [editor_ copy:nil];
}

// Test that -cut: is correctly delegated to the observer and clears
// the text field.
TEST_F(AutocompleteTextFieldEditorObserverTest, Cut) {
  // Sets a string in the field.
  NSString* test_string = @"astring";
  EXPECT_CALL(field_observer_, OnDidBeginEditing());
  EXPECT_CALL(field_observer_, OnBeforeChange());
  EXPECT_CALL(field_observer_, OnDidChange());
  EXPECT_CALL(field_observer_, SelectionRangeForProposedRange(A<NSRange>()))
      .WillRepeatedly(ReturnArg<0>());
  [editor_ setString:test_string];
  [editor_ selectAll:nil];

  // Calls cut.
  EXPECT_CALL(field_observer_, CanCopy())
      .WillOnce(Return(true));
  EXPECT_CALL(field_observer_, CopyToPasteboard(A<NSPasteboard*>()))
      .Times(1);
  [editor_ cut:nil];

  // Check if the field is cleared.
  ASSERT_EQ([[editor_ textStorage] length], 0U);
}

// Test that -pasteAndGo: is correctly delegated to the observer.
TEST_F(AutocompleteTextFieldEditorObserverTest, PasteAndGo) {
  EXPECT_CALL(field_observer_, OnPasteAndGo());
  [editor_ pasteAndGo:nil];
}

// Test that the menu is constructed correctly.
TEST_F(AutocompleteTextFieldEditorObserverTest, Menu) {
  EXPECT_CALL(field_observer_, CanPasteAndGo()).WillOnce(Return(true));
  EXPECT_CALL(field_observer_, GetPasteActionStringId()).
      WillOnce(Return(IDS_PASTE_AND_GO));

  NSMenu* menu = MenuFromRightClick(editor_);
  NSArray* items = [menu itemArray];
  ASSERT_EQ([items count], 6U);
  // TODO(shess): Check the titles, too?
  NSUInteger i = 0;  // Use an index to make future changes easier.
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(cut:));
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(copy:));
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(paste:));
  NSMenuItem* pasteAndGo = [items objectAtIndex:i++];
  EXPECT_EQ([pasteAndGo action], @selector(pasteAndGo:));
  EXPECT_TRUE([pasteAndGo isEnabled]);
  EXPECT_TRUE([[items objectAtIndex:i++] isSeparatorItem]);
  EXPECT_EQ([[items objectAtIndex:i] tag], IDC_EDIT_SEARCH_ENGINES);
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(commandDispatch:));
}

// Test that the menu is constructed correctly when field isn't
// editable.
TEST_F(AutocompleteTextFieldEditorObserverTest, CanPasteAndGoMenuNotEditable) {
  [field_ setEditable:NO];
  [editor_ setEditable:NO];

  // Never call this when not editable.
  EXPECT_CALL(field_observer_, GetPasteActionStringId()).Times(0);

  NSMenu* menu = MenuFromRightClick(editor_);
  NSArray* items = [menu itemArray];
  ASSERT_EQ([items count], 3U);
  // TODO(shess): Check the titles, too?
  NSUInteger i = 0;  // Use an index to make future changes easier.
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(cut:));
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(copy:));
  EXPECT_EQ([[items objectAtIndex:i++] action], @selector(paste:));
}

// Test that the menu validation works as expected when CanPasteAndGo().
TEST_F(AutocompleteTextFieldEditorObserverTest, CanPasteAndGoValidate) {
  EXPECT_CALL(field_observer_, GetPasteActionStringId())
      .WillOnce(Return(IDS_PASTE_AND_GO));
  EXPECT_CALL(field_observer_, CanPasteAndGo()).WillOnce(Return(true));

  NSMenu* menu = MenuFromRightClick(editor_);
  NSArray* items = [menu itemArray];
  ASSERT_EQ([items count], 6U);
  for (NSUInteger i = 0; i < [items count]; ++i) {
    NSMenuItem* item = [items objectAtIndex:i];
    if ([item action] == @selector(pasteAndGo:)) {
      EXPECT_TRUE([item isEnabled]);
      EXPECT_TRUE([editor_ validateMenuItem:item]);
      break;
    }
  }
}

// Test that the GetPasteActionStringId() is not called when !CanPasteAndGo().
TEST_F(AutocompleteTextFieldEditorObserverTest, CannotPasteAndGoValidate) {
  EXPECT_CALL(field_observer_, GetPasteActionStringId()).Times(0);
  EXPECT_CALL(field_observer_, CanPasteAndGo()).WillOnce(Return(false));

  NSMenu* menu = MenuFromRightClick(editor_);
  NSArray* items = [menu itemArray];
  ASSERT_EQ([items count], 6U);
  for (NSUInteger i = 0; i < [items count]; ++i) {
    NSMenuItem* item = [items objectAtIndex:i];
    if ([item action] == @selector(pasteAndGo:)) {
      EXPECT_FALSE([item isEnabled]);
      EXPECT_FALSE([editor_ validateMenuItem:item]);
      break;
    }
  }
}

}  // namespace
