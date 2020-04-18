// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_editor.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_unittest_helper.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/cocoa/cocoa_base_utils.h"

using ::testing::A;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::StrictMock;
using ::testing::_;

namespace {

class MockDecoration : public LocationBarDecoration {
 public:
  virtual CGFloat GetWidthForSpace(CGFloat width) { return 20.0; }

  virtual void DrawInFrame(NSRect frame, NSView* control_view) { ; }
  MOCK_METHOD0(AcceptsMousePress, AcceptsPress());
  MOCK_METHOD2(OnMousePressed, bool(NSRect frame, NSPoint location));
  MOCK_METHOD0(GetMenu, NSMenu*());
};

// Mock up an incrementing event number.
NSUInteger eventNumber = 0;

// Create an event of the indicated |type| at |point| within |view|.
// TODO(shess): Would be nice to have a MockApplication which provided
// nifty accessors to create these things and inject them.  It could
// even provide functions for "Click and drag mouse from point A to
// point B".
NSEvent* Event(NSView* view, const NSPoint point, const NSEventType type,
               const NSUInteger clickCount) {
  NSWindow* window([view window]);
  const NSPoint locationInWindow([view convertPoint:point toView:nil]);
  const NSPoint location =
      ui::ConvertPointFromWindowToScreen(window, locationInWindow);
  return [NSEvent mouseEventWithType:type
                            location:location
                       modifierFlags:0
                           timestamp:0
                        windowNumber:[window windowNumber]
                             context:nil
                         eventNumber:eventNumber++
                          clickCount:clickCount
                            pressure:0.0];
}
NSEvent* Event(NSView* view, const NSPoint point, const NSEventType type) {
  return Event(view, point, type, 1);
}

// Width of the field so that we don't have to ask |field_| for it all
// the time.
static const CGFloat kWidth(300.0);

class AutocompleteTextFieldTest : public CocoaTest {
 public:
  AutocompleteTextFieldTest() {
    // Make sure this is wide enough to play games with the cell
    // decorations.
    NSRect frame = NSMakeRect(0, 0, kWidth, 30);
    base::scoped_nsobject<AutocompleteTextField> field(
        [[AutocompleteTextField alloc] initWithFrame:frame]);
    field_ = field.get();
    [field_ setStringValue:@"Test test"];
    [[test_window() contentView] addSubview:field_];

    AutocompleteTextFieldCell* cell = [field_ cell];
    [cell clearDecorations];

    mock_leading_decoration_.SetVisible(false);
    [cell addLeadingDecoration:&mock_leading_decoration_];

    mock_trailing_decoration_.SetVisible(false);
    [cell addTrailingDecoration:&mock_trailing_decoration_];

    window_delegate_.reset(
        [[AutocompleteTextFieldWindowTestDelegate alloc] init]);
    [test_window() setDelegate:window_delegate_.get()];
  }

  NSEvent* KeyDownEventWithFlags(NSUInteger flags) {
    return [NSEvent keyEventWithType:NSKeyDown
                            location:NSZeroPoint
                       modifierFlags:flags
                           timestamp:0.0
                        windowNumber:[test_window() windowNumber]
                             context:nil
                          characters:@"a"
         charactersIgnoringModifiers:@"a"
                           isARepeat:NO
                             keyCode:'a'];
  }

  // Helper to return the field-editor frame being used w/in |field_|.
  NSRect EditorFrame() {
    EXPECT_TRUE([field_ currentEditor]);
    EXPECT_EQ([[field_ subviews] count], 1U);
    if ([[field_ subviews] count] > 0) {
      return [[[field_ subviews] objectAtIndex:0] frame];
    } else {
      // Return something which won't work so the caller can soldier
      // on.
      return NSZeroRect;
    }
  }

  AutocompleteTextFieldEditor* FieldEditor() {
    return base::mac::ObjCCastStrict<AutocompleteTextFieldEditor>(
        [field_ currentEditor]);
  }

  AutocompleteTextField* field_;
  MockDecoration mock_leading_decoration_;
  MockDecoration mock_trailing_decoration_;
  base::scoped_nsobject<AutocompleteTextFieldWindowTestDelegate>
      window_delegate_;
};

TEST_VIEW(AutocompleteTextFieldTest, field_);

// Base class for testing AutocompleteTextFieldObserver messages.
class AutocompleteTextFieldObserverTest : public AutocompleteTextFieldTest {
 public:
  virtual void SetUp() {
    AutocompleteTextFieldTest::SetUp();
    [field_ setObserver:&field_observer_];
  }

  virtual void TearDown() {
    // Clear the observer so that we don't show output for
    // uninteresting messages to the mock (for instance, if |field_| has
    // focus at the end of the test).
    [field_ setObserver:NULL];

    AutocompleteTextFieldTest::TearDown();
  }

  // Returns the center point of the decoration.
  NSPoint ClickLocationForDecoration(LocationBarDecoration* decoration) {
    AutocompleteTextFieldCell* cell = [field_ cell];
    NSRect decoration_rect =
        [cell frameForDecoration:decoration inFrame:[field_ bounds]];
    EXPECT_FALSE(NSIsEmptyRect(decoration_rect));
    return NSMakePoint(NSMidX(decoration_rect), NSMidY(decoration_rect));
  }

  void SendMouseClickToDecoration(LocationBarDecoration* decoration) {
    NSPoint point = ClickLocationForDecoration(decoration);
    NSEvent* downEvent = Event(field_, point, NSLeftMouseDown);
    NSEvent* upEvent = Event(field_, point, NSLeftMouseUp);

    // Can't just use -sendEvent:, since that doesn't populate -currentEvent.
    [NSApp postEvent:downEvent atStart:YES];
    [NSApp postEvent:upEvent atStart:NO];

    NSEvent* next_event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                             untilDate:nil
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES];
    [NSApp sendEvent:next_event];
  }

  StrictMock<MockAutocompleteTextFieldObserver> field_observer_;
};

// Test that we have the right cell class.
TEST_F(AutocompleteTextFieldTest, CellClass) {
  EXPECT_TRUE([[field_ cell] isKindOfClass:[AutocompleteTextFieldCell class]]);
}

// Test that becoming first responder sets things up correctly.
TEST_F(AutocompleteTextFieldTest, FirstResponder) {
  EXPECT_EQ(nil, [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 0U);
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_FALSE(nil == [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 1U);
  EXPECT_TRUE([[field_ currentEditor] isDescendantOf:field_]);

  // Check that the window delegate is providing the right editor.
  Class c = [AutocompleteTextFieldEditor class];
  EXPECT_TRUE([[field_ currentEditor] isKindOfClass:c]);
}

TEST_F(AutocompleteTextFieldTest, AvailableDecorationWidth) {
  // A fudge factor to account for how much space the border takes up.
  // The test shouldn't be too dependent on the field's internals, but
  // it also shouldn't let deranged cases fall through the cracks
  // (like nothing available with no text, or everything available
  // with some text).
  const CGFloat kBorderWidth = 20.0;

  // With no contents, almost the entire width is available for
  // decorations.
  [field_ setStringValue:@""];
  CGFloat availableWidth = [field_ availableDecorationWidth];
  EXPECT_LE(availableWidth, kWidth);
  EXPECT_GT(availableWidth, kWidth - kBorderWidth);

  // With minor contents, most of the remaining width is available for
  // decorations.
  NSDictionary* attributes =
      [NSDictionary dictionaryWithObject:[field_ font]
                                  forKey:NSFontAttributeName];
  NSString* string = @"Hello world";
  const NSSize size([string sizeWithAttributes:attributes]);
  [field_ setStringValue:string];
  availableWidth = [field_ availableDecorationWidth];
  EXPECT_LE(availableWidth, kWidth - size.width);
  EXPECT_GT(availableWidth, kWidth - size.width - kBorderWidth);

  // With huge contents, nothing at all is left for decorations.
  string = @"A long string which is surely wider than field_ can hold.";
  [field_ setStringValue:string];
  availableWidth = [field_ availableDecorationWidth];
  EXPECT_LT(availableWidth, 0.0);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(AutocompleteTextFieldTest, Display) {
  [field_ display];

  // Test focussed drawing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  [field_ display];
}

TEST_F(AutocompleteTextFieldObserverTest, FlagsChanged) {
  InSequence dummy;  // Call mock in exactly the order specified.

  // Test without Control key down, but some other modifier down.
  EXPECT_CALL(field_observer_, OnControlKeyChanged(false));
  [field_ flagsChanged:KeyDownEventWithFlags(NSShiftKeyMask)];

  // Test with Control key down.
  EXPECT_CALL(field_observer_, OnControlKeyChanged(true));
  [field_ flagsChanged:KeyDownEventWithFlags(NSControlKeyMask)];
}

// This test is here rather than in the editor's tests because the
// field catches -flagsChanged: because it's on the responder chain,
// the field editor doesn't implement it.
TEST_F(AutocompleteTextFieldObserverTest, FieldEditorFlagsChanged) {
  // Many of these methods try to change the selection.
  EXPECT_CALL(field_observer_, SelectionRangeForProposedRange(A<NSRange>()))
      .WillRepeatedly(ReturnArg<0>());

  InSequence dummy;  // Call mock in exactly the order specified.
  EXPECT_CALL(field_observer_, OnSetFocus(false));
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  NSResponder* firstResponder = [[field_ window] firstResponder];
  EXPECT_EQ(firstResponder, [field_ currentEditor]);

  // Test without Control key down, but some other modifier down.
  EXPECT_CALL(field_observer_, OnControlKeyChanged(false));
  [firstResponder flagsChanged:KeyDownEventWithFlags(NSShiftKeyMask)];

  // Test with Control key down.
  EXPECT_CALL(field_observer_, OnControlKeyChanged(true));
  [firstResponder flagsChanged:KeyDownEventWithFlags(NSControlKeyMask)];
}

// Frame size changes are propagated to |observer_|.
TEST_F(AutocompleteTextFieldObserverTest, FrameChanged) {
  EXPECT_CALL(field_observer_, OnFrameChanged());
  NSRect frame = [field_ frame];
  frame.size.width += 10.0;
  [field_ setFrame:frame];
}

// Test that the field editor gets the same bounds when focus is
// delivered by the standard focusing machinery, or by
// -resetFieldEditorFrameIfNeeded.
TEST_F(AutocompleteTextFieldTest, ResetFieldEditorBase) {
  // Capture the editor frame resulting from the standard focus
  // machinery.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame = EditorFrame();

  // A decoration should result in a strictly smaller editor frame.
  mock_leading_decoration_.SetVisible(true);
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
  EXPECT_TRUE(NSContainsRect(baseEditorFrame, EditorFrame()));

  // Removing the decoration and using -resetFieldEditorFrameIfNeeded
  // should result in the same frame as the standard focus machinery.
  mock_leading_decoration_.SetVisible(false);
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSEQ(baseEditorFrame, EditorFrame());
}

// Test that the field editor gets the same bounds when focus is
// delivered by the standard focusing machinery, or by
// -resetFieldEditorFrameIfNeeded, this time with a decoration
// pre-loaded.
TEST_F(AutocompleteTextFieldTest, ResetFieldEditorWithDecoration) {
  AutocompleteTextFieldCell* cell = [field_ cell];

  // Make sure decoration isn't already visible, then make it visible.
  EXPECT_TRUE(NSIsEmptyRect([cell frameForDecoration:&mock_leading_decoration_
                                             inFrame:[field_ bounds]]));
  mock_leading_decoration_.SetVisible(true);
  EXPECT_FALSE(NSIsEmptyRect([cell frameForDecoration:&mock_leading_decoration_
                                              inFrame:[field_ bounds]]));

  // Capture the editor frame resulting from the standard focus
  // machinery.

  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame = EditorFrame();

  // When the decoration is not visible the frame should be strictly larger.
  mock_leading_decoration_.SetVisible(false);
  EXPECT_TRUE(NSIsEmptyRect([cell frameForDecoration:&mock_leading_decoration_
                                             inFrame:[field_ bounds]]));
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
  EXPECT_TRUE(NSContainsRect(EditorFrame(), baseEditorFrame));

  // When the decoration is visible, -resetFieldEditorFrameIfNeeded
  // should result in the same frame as the standard focus machinery.
  mock_leading_decoration_.SetVisible(true);
  EXPECT_FALSE(NSIsEmptyRect([cell frameForDecoration:&mock_leading_decoration_
                                              inFrame:[field_ bounds]]));

  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSEQ(baseEditorFrame, EditorFrame());
}

// Test that resetting the field editor bounds does not cause untoward
// messages to the field's observer.
TEST_F(AutocompleteTextFieldObserverTest, ResetFieldEditorContinuesEditing) {
  // Many of these methods try to change the selection.
  EXPECT_CALL(field_observer_, SelectionRangeForProposedRange(A<NSRange>()))
      .WillRepeatedly(ReturnArg<0>());

  EXPECT_CALL(field_observer_, OnSetFocus(false));
  // Becoming first responder doesn't begin editing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame = EditorFrame();
  NSTextView* editor = static_cast<NSTextView*>([field_ currentEditor]);
  EXPECT_TRUE(nil != editor);

  // This should begin editing and indicate a change.
  EXPECT_CALL(field_observer_, OnDidBeginEditing());
  EXPECT_CALL(field_observer_, OnBeforeChange());
  EXPECT_CALL(field_observer_, OnDidChange());
  [editor shouldChangeTextInRange:NSMakeRange(0, 0) replacementString:@""];
  [editor didChangeText];

  // No messages to |field_observer_| when the frame actually changes.
  mock_leading_decoration_.SetVisible(true);
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
}

// Clicking in a right-hand decoration which does not handle the mouse
// puts the caret rightmost.
TEST_F(AutocompleteTextFieldTest, ClickRightDecorationPutsCaretRightmost) {
  // Decoration does not handle the mouse event, so the cell should
  // process it.  Called at least once.
  EXPECT_CALL(mock_trailing_decoration_, AcceptsMousePress())
      .WillOnce(Return(AcceptsPress::NEVER))
      .WillRepeatedly(Return(AcceptsPress::NEVER));

  // Set the decoration before becoming responder.
  EXPECT_FALSE([field_ currentEditor]);
  mock_trailing_decoration_.SetVisible(true);

  // Make first responder should select all.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_TRUE([field_ currentEditor]);
  const NSRange allRange = NSMakeRange(0, [[field_ stringValue] length]);
  EXPECT_TRUE(NSEqualRanges(allRange, [[field_ currentEditor] selectedRange]));

  // Generate a click on the decoration.
  AutocompleteTextFieldCell* cell = [field_ cell];
  const NSRect bounds = [field_ bounds];
  const NSRect iconFrame =
      [cell frameForDecoration:&mock_trailing_decoration_ inFrame:bounds];
  const NSPoint point = NSMakePoint(NSMidX(iconFrame), NSMidY(iconFrame));
  NSEvent* downEvent = Event(field_, point, NSLeftMouseDown);
  NSEvent* upEvent = Event(field_, point, NSLeftMouseUp);
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];

  // Selection should be a right-hand-side caret.
  EXPECT_TRUE(NSEqualRanges(NSMakeRange([[field_ stringValue] length], 0),
                            [[field_ currentEditor] selectedRange]));
}

// Clicking in a left-side decoration which doesn't handle the event
// puts the selection in the leftmost position.
TEST_F(AutocompleteTextFieldTest, ClickLeftDecorationPutsCaretLeftmost) {
  // Decoration does not handle the mouse event, so the cell should
  // process it.  Called at least once.
  EXPECT_CALL(mock_leading_decoration_, AcceptsMousePress())
      .WillOnce(Return(AcceptsPress::NEVER))
      .WillRepeatedly(Return(AcceptsPress::NEVER));

  // Set the decoration before becoming responder.
  EXPECT_FALSE([field_ currentEditor]);
  mock_leading_decoration_.SetVisible(true);

  // Make first responder should select all.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_TRUE([field_ currentEditor]);
  const NSRange allRange = NSMakeRange(0, [[field_ stringValue] length]);
  EXPECT_TRUE(NSEqualRanges(allRange, [[field_ currentEditor] selectedRange]));

  // Generate a click on the decoration.
  AutocompleteTextFieldCell* cell = [field_ cell];
  const NSRect bounds = [field_ bounds];
  const NSRect iconFrame =
      [cell frameForDecoration:&mock_leading_decoration_ inFrame:bounds];
  const NSPoint point = NSMakePoint(NSMidX(iconFrame), NSMidY(iconFrame));
  NSEvent* downEvent = Event(field_, point, NSLeftMouseDown);
  NSEvent* upEvent = Event(field_, point, NSLeftMouseUp);
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];

  // Selection should be a left-hand-side caret.
  EXPECT_TRUE(NSEqualRanges(NSMakeRange(0, 0),
                            [[field_ currentEditor] selectedRange]));
}

// Clicks not in the text area or the cell's decorations fall through
// to the editor.
TEST_F(AutocompleteTextFieldTest, ClickBorderSelectsAll) {
  // Can't rely on the window machinery to make us first responder,
  // here.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_TRUE([field_ currentEditor]);

  const NSPoint point(NSMakePoint(20.0, 1.0));
  NSEvent* downEvent(Event(field_, point, NSLeftMouseDown));
  NSEvent* upEvent(Event(field_, point, NSLeftMouseUp));
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];

  // Clicking in the narrow border area around a Cocoa NSTextField
  // does a select-all.  Regardless of whether this is a good call, it
  // works as a test that things get passed down to the editor.
  const NSRange selectedRange([[field_ currentEditor] selectedRange]);
  EXPECT_EQ(selectedRange.location, 0U);
  EXPECT_EQ(selectedRange.length, [[field_ stringValue] length]);
}

// Single-click with no drag should setup a field editor and
// select all.
TEST_F(AutocompleteTextFieldTest, ClickSelectsAll) {
  EXPECT_FALSE([field_ currentEditor]);

  const NSPoint point = NSMakePoint(20.0, NSMidY([field_ bounds]));
  NSEvent* downEvent(Event(field_, point, NSLeftMouseDown));
  NSEvent* upEvent(Event(field_, point, NSLeftMouseUp));
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];
  EXPECT_TRUE([field_ currentEditor]);
  const NSRange selectedRange([[field_ currentEditor] selectedRange]);
  EXPECT_EQ(selectedRange.location, 0U);
  EXPECT_EQ(selectedRange.length, [[field_ stringValue] length]);
}

// Click-drag selects text, not select all.
TEST_F(AutocompleteTextFieldTest, ClickDragSelectsText) {
  EXPECT_FALSE([field_ currentEditor]);

  NSEvent* downEvent(Event(field_, NSMakePoint(20.0, 5.0), NSLeftMouseDown));
  NSEvent* upEvent(Event(field_, NSMakePoint(0.0, 5.0), NSLeftMouseUp));
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];
  EXPECT_TRUE([field_ currentEditor]);

  // Expect this to have selected a prefix of the content.  Mostly
  // just don't want the select-all behavior.
  const NSRange selectedRange([[field_ currentEditor] selectedRange]);
  EXPECT_EQ(selectedRange.location, 0U);
  EXPECT_LT(selectedRange.length, [[field_ stringValue] length]);
}

// TODO(shess): Test that click/pause/click allows cursor placement.
// In this case the first click goes to the field, but the second
// click goes to the field editor, so the current testing pattern
// can't work.  What really needs to happen is to push through the
// NSWindow event machinery so that we can say "two independent clicks
// at the same location have the right effect".  Once that is done, it
// might make sense to revise the other tests to use the same
// machinery.

// Double-click selects word, not select all.
TEST_F(AutocompleteTextFieldTest, DoubleClickSelectsWord) {
  EXPECT_FALSE([field_ currentEditor]);

  const NSPoint point = NSMakePoint(20.0, NSMidY([field_ bounds]));
  NSEvent* downEvent(Event(field_, point, NSLeftMouseDown, 1));
  NSEvent* upEvent(Event(field_, point, NSLeftMouseUp, 1));
  NSEvent* downEvent2(Event(field_, point, NSLeftMouseDown, 2));
  NSEvent* upEvent2(Event(field_, point, NSLeftMouseUp, 2));
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];
  [NSApp postEvent:upEvent2 atStart:YES];
  [field_ mouseDown:downEvent2];
  EXPECT_TRUE([field_ currentEditor]);

  // Selected the first word.
  const NSRange selectedRange([[field_ currentEditor] selectedRange]);
  const NSRange spaceRange([[field_ stringValue] rangeOfString:@" "]);
  EXPECT_GT(spaceRange.location, 0U);
  EXPECT_LT(spaceRange.length, [[field_ stringValue] length]);
  EXPECT_EQ(selectedRange.location, 0U);
  EXPECT_EQ(selectedRange.length, spaceRange.location);
}

TEST_F(AutocompleteTextFieldTest, TripleClickSelectsAll) {
  EXPECT_FALSE([field_ currentEditor]);

  const NSPoint point(NSMakePoint(20.0, 5.0));
  NSEvent* downEvent(Event(field_, point, NSLeftMouseDown, 1));
  NSEvent* upEvent(Event(field_, point, NSLeftMouseUp, 1));
  NSEvent* downEvent2(Event(field_, point, NSLeftMouseDown, 2));
  NSEvent* upEvent2(Event(field_, point, NSLeftMouseUp, 2));
  NSEvent* downEvent3(Event(field_, point, NSLeftMouseDown, 3));
  NSEvent* upEvent3(Event(field_, point, NSLeftMouseUp, 3));
  [NSApp postEvent:upEvent atStart:YES];
  [field_ mouseDown:downEvent];
  [NSApp postEvent:upEvent2 atStart:YES];
  [field_ mouseDown:downEvent2];
  [NSApp postEvent:upEvent3 atStart:YES];
  [field_ mouseDown:downEvent3];
  EXPECT_TRUE([field_ currentEditor]);

  // Selected the first word.
  const NSRange selectedRange([[field_ currentEditor] selectedRange]);
  EXPECT_EQ(selectedRange.location, 0U);
  EXPECT_EQ(selectedRange.length, [[field_ stringValue] length]);
}

// Clicking a decoration should call decoration's OnMousePressed.
TEST_F(AutocompleteTextFieldTest, LeftDecorationMouseDown) {
  // At this point, not focussed.
  EXPECT_FALSE([field_ currentEditor]);

  mock_leading_decoration_.SetVisible(true);
  EXPECT_CALL(mock_leading_decoration_, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));

  AutocompleteTextFieldCell* cell = [field_ cell];
  [cell updateMouseTrackingAndToolTipsInRect:[field_ frame] ofView:field_];

  const NSRect iconFrame = [cell frameForDecoration:&mock_leading_decoration_
                                            inFrame:[field_ bounds]];
  const NSPoint location = NSMakePoint(NSMidX(iconFrame), NSMidY(iconFrame));
  NSEvent* downEvent = Event(field_, location, NSLeftMouseDown, 1);
  NSEvent* upEvent = Event(field_, location, NSLeftMouseUp, 1);

  // Since decorations can be dragged, the mouse-press is sent on
  // mouse-up.
  [NSApp postEvent:upEvent atStart:YES];

  EXPECT_CALL(mock_leading_decoration_, OnMousePressed(_, _))
      .WillOnce(Return(true));
  [field_ mouseDown:downEvent];

  // Focus the field and test that handled clicks don't affect selection.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_TRUE([field_ currentEditor]);
  const NSRange allRange = NSMakeRange(0, [[field_ stringValue] length]);
  EXPECT_TRUE(NSEqualRanges(allRange, [[field_ currentEditor] selectedRange]));

  // Generate another click on the decoration.
  downEvent = Event(field_, location, NSLeftMouseDown, 1);
  upEvent = Event(field_, location, NSLeftMouseUp, 1);
  [NSApp postEvent:upEvent atStart:YES];
  EXPECT_CALL(mock_leading_decoration_, OnMousePressed(_, _))
      .WillOnce(Return(true));
  [field_ mouseDown:downEvent];

  // The selection should not have changed.
  EXPECT_TRUE(NSEqualRanges(allRange, [[field_ currentEditor] selectedRange]));

  // TODO(shess): Test that mouse drags are initiated if the next
  // event is a drag, or if the mouse-up takes too long to arrive.
  // IDEA: mock decoration to return a pasteboard which a mock
  // AutocompleteTextField notes in -dragImage:*.
}

// Clicking a decoration should call decoration's OnMousePressed.
TEST_F(AutocompleteTextFieldTest, RightDecorationMouseDown) {
  // At this point, not focussed.
  EXPECT_FALSE([field_ currentEditor]);

  mock_trailing_decoration_.SetVisible(true);
  EXPECT_CALL(mock_trailing_decoration_, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));

  AutocompleteTextFieldCell* cell = [field_ cell];
  [cell updateMouseTrackingAndToolTipsInRect:[field_ frame] ofView:field_];

  const NSRect bounds = [field_ bounds];
  const NSRect iconFrame =
      [cell frameForDecoration:&mock_trailing_decoration_ inFrame:bounds];
  const NSPoint location = NSMakePoint(NSMidX(iconFrame), NSMidY(iconFrame));
  NSEvent* downEvent = Event(field_, location, NSLeftMouseDown, 1);
  NSEvent* upEvent = Event(field_, location, NSLeftMouseUp, 1);

  // Since decorations can be dragged, the mouse-press is sent on
  // mouse-up.
  [NSApp postEvent:upEvent atStart:YES];

  EXPECT_CALL(mock_trailing_decoration_, OnMousePressed(_, _))
      .WillOnce(Return(true));
  [field_ mouseDown:downEvent];
}

// Test that page action menus are properly returned.
// TODO(shess): Really, this should test that things are forwarded to
// the cell, and the cell tests should test that the right things are
// selected.  It's easier to mock the event here, though.  This code's
// event-mockers might be worth promoting to |cocoa_test_event_utils.h| or
// |cocoa_test_helper.h|.
TEST_F(AutocompleteTextFieldTest, DecorationMenu) {
  AutocompleteTextFieldCell* cell = [field_ cell];
  const NSRect bounds([field_ bounds]);

  const CGFloat edge = NSHeight(bounds) - 4.0;
  const NSSize size = NSMakeSize(edge, edge);
  base::scoped_nsobject<NSImage> image([[NSImage alloc] initWithSize:size]);

  base::scoped_nsobject<NSMenu> menu([[NSMenu alloc] initWithTitle:@"Menu"]);

  mock_leading_decoration_.SetVisible(true);
  mock_trailing_decoration_.SetVisible(true);

  // The item with a menu returns it.
  NSRect actionFrame =
      [cell frameForDecoration:&mock_trailing_decoration_ inFrame:bounds];
  NSPoint location = NSMakePoint(NSMidX(actionFrame), NSMidY(actionFrame));
  NSEvent* event = Event(field_, location, NSRightMouseDown, 1);

  // Check that the decoration is called, and the field returns the
  // menu.
  EXPECT_CALL(mock_trailing_decoration_, GetMenu())
      .WillOnce(Return(menu.get()));
  NSMenu *decorationMenu = [field_ decorationMenuForEvent:event];
  EXPECT_EQ(decorationMenu, menu);

  // The item without a menu returns nil.
  EXPECT_CALL(mock_leading_decoration_, GetMenu())
      .WillOnce(Return(static_cast<NSMenu*>(nil)));
  actionFrame =
      [cell frameForDecoration:&mock_leading_decoration_ inFrame:bounds];
  location = NSMakePoint(NSMidX(actionFrame), NSMidY(actionFrame));
  event = Event(field_, location, NSRightMouseDown, 1);
  EXPECT_FALSE([field_ decorationMenuForEvent:event]);

  // Something not in an action returns nil.
  location = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
  event = Event(field_, location, NSRightMouseDown, 1);
  EXPECT_FALSE([field_ decorationMenuForEvent:event]);
}

// Verify that -setAttributedStringValue: works as expected when
// focussed or when not focussed.  Our code mostly depends on about
// whether -stringValue works right.
TEST_F(AutocompleteTextFieldTest, SetAttributedStringBaseline) {
  EXPECT_EQ(nil, [field_ currentEditor]);

  // So that we can set rich text.
  [field_ setAllowsEditingTextAttributes:YES];

  // Set an attribute different from the field's default so we can
  // tell we got the same string out as we put in.
  NSFont* font = [NSFont fontWithDescriptor:[[field_ font] fontDescriptor]
                                       size:[[field_ font] pointSize] + 2];
  NSDictionary* attributes =
      [NSDictionary dictionaryWithObject:font
                                  forKey:NSFontAttributeName];
  NSString* const kString = @"This is a test";
  base::scoped_nsobject<NSAttributedString> attributedString(
      [[NSAttributedString alloc] initWithString:kString
                                      attributes:attributes]);

  // Check that what we get back looks like what we put in.
  EXPECT_NSNE(kString, [field_ stringValue]);
  [field_ setAttributedStringValue:attributedString];
  EXPECT_TRUE([[field_ attributedStringValue]
                isEqualToAttributedString:attributedString]);
  EXPECT_NSEQ(kString, [field_ stringValue]);

  // Try that again with focus.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];

  EXPECT_TRUE([field_ currentEditor]);

  // Check that what we get back looks like what we put in.
  [field_ setStringValue:@""];
  EXPECT_NSNE(kString, [field_ stringValue]);
  [field_ setAttributedStringValue:attributedString];
  EXPECT_TRUE([[field_ attributedStringValue]
                isEqualToAttributedString:attributedString]);
  EXPECT_NSEQ(kString, [field_ stringValue]);
}

// -setAttributedStringValue: shouldn't reset the undo state if things
// are being editted.
TEST_F(AutocompleteTextFieldTest, SetAttributedStringUndo) {
  NSColor* redColor = [NSColor redColor];
  NSDictionary* attributes =
      [NSDictionary dictionaryWithObject:redColor
                                  forKey:NSForegroundColorAttributeName];
  NSString* const kString = @"This is a test";
  base::scoped_nsobject<NSAttributedString> attributedString(
      [[NSAttributedString alloc] initWithString:kString
                                      attributes:attributes]);
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_TRUE([field_ currentEditor]);
  NSTextView* editor = static_cast<NSTextView*>([field_ currentEditor]);
  NSUndoManager* undoManager = [editor undoManager];
  EXPECT_TRUE(undoManager);

  // Nothing to undo, yet.
  EXPECT_FALSE([undoManager canUndo]);

  // Starting an editing action creates an undoable item.
  [editor shouldChangeTextInRange:NSMakeRange(0, 0) replacementString:@""];
  [editor didChangeText];
  EXPECT_TRUE([undoManager canUndo]);

  // -setStringValue: resets the editor's undo chain.
  [field_ setStringValue:kString];
  EXPECT_FALSE([undoManager canUndo]);

  // Verify that -setAttributedStringValue: does not reset the
  // editor's undo chain.
  [field_ setStringValue:@""];
  [editor shouldChangeTextInRange:NSMakeRange(0, 0) replacementString:@""];
  [editor didChangeText];
  EXPECT_TRUE([undoManager canUndo]);
  [field_ setAttributedStringValue:attributedString];
  EXPECT_TRUE([undoManager canUndo]);

  // Verify that calling -clearUndoChain clears the undo chain.
  [field_ clearUndoChain];
  EXPECT_FALSE([undoManager canUndo]);
}

TEST_F(AutocompleteTextFieldTest, EditorGetsCorrectUndoManager) {
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];

  NSTextView* editor = static_cast<NSTextView*>([field_ currentEditor]);
  EXPECT_TRUE(editor);
  EXPECT_EQ([field_ undoManagerForTextView:editor], [editor undoManager]);
}

// Verify that hideFocusState correctly hides the focus ring and insertion
// pointer.
TEST_F(AutocompleteTextFieldTest, HideFocusState) {
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  [[field_ cell] setShowsFirstResponder:YES];

  EXPECT_TRUE([[field_ cell] showsFirstResponder]);
  EXPECT_TRUE([FieldEditor() shouldDrawInsertionPoint]);

  [[field_ cell] setHideFocusState:YES
                            ofView:field_];
  EXPECT_FALSE([[field_ cell] showsFirstResponder]);
  EXPECT_FALSE([FieldEditor() shouldDrawInsertionPoint]);

  [[field_ cell] setHideFocusState:NO
                            ofView:field_];
  EXPECT_TRUE([[field_ cell] showsFirstResponder]);
  EXPECT_TRUE([FieldEditor() shouldDrawInsertionPoint]);
}

// Verify that the tracking areas are added properly.
TEST_F(AutocompleteTextFieldTest, UpdateTrackingAreas) {
  AutocompleteTextFieldCell* cell = [field_ cell];

  mock_leading_decoration_.SetVisible(true);
  mock_trailing_decoration_.SetVisible(true);

  EXPECT_CALL(mock_leading_decoration_, AcceptsMousePress())
      .WillOnce(Return(AcceptsPress::ALWAYS))
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));
  EXPECT_CALL(mock_trailing_decoration_, AcceptsMousePress())
      .WillOnce(Return(AcceptsPress::NEVER))
      .WillRepeatedly(Return(AcceptsPress::NEVER));
  [cell updateMouseTrackingAndToolTipsInRect:[field_ bounds] ofView:field_];

  EXPECT_EQ([cell mouseTrackingDecorations].size(), 1.0);

  [cell clearTrackingArea];
  EXPECT_TRUE([cell mouseTrackingDecorations].empty());

  EXPECT_CALL(mock_trailing_decoration_, AcceptsMousePress())
      .WillOnce(Return(AcceptsPress::ALWAYS))
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));

  [cell updateMouseTrackingAndToolTipsInRect:[field_ bounds] ofView:field_];
  EXPECT_EQ([cell mouseTrackingDecorations].size(), 2.0);
}

// Verify that clicking a decoration that accepts mouse clicks does not focus
// the Omnibox.
TEST_F(AutocompleteTextFieldObserverTest,
       ClickingDecorationDoesNotFocusOmnibox) {
  AutocompleteTextFieldCell* cell = [field_ cell];

  // Set up a non-interactive decoration.
  MockDecoration noninteractive_decoration;
  noninteractive_decoration.SetVisible(true);
  EXPECT_CALL(noninteractive_decoration, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::NEVER));
  [cell addLeadingDecoration:&noninteractive_decoration];

  // Set up an interactive decoration.
  MockDecoration interactive_decoration;
  EXPECT_CALL(interactive_decoration, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));
  interactive_decoration.SetVisible(true);
  [cell addLeadingDecoration:&interactive_decoration];
  [cell updateMouseTrackingAndToolTipsInRect:[field_ frame] ofView:field_];
  EXPECT_CALL(interactive_decoration, OnMousePressed(_, _))
      .WillRepeatedly(testing::Return(true));

  // Ignore incidental calls. The exact frequency of these calls doesn't matter
  // as they are auxiliary.
  EXPECT_CALL(field_observer_, SelectionRangeForProposedRange(_))
      .WillRepeatedly(testing::Return(NSMakeRange(0, 0)));
  EXPECT_CALL(field_observer_, OnMouseDown(_)).Times(testing::AnyNumber());
  EXPECT_CALL(field_observer_, OnSetFocus(false)).Times(testing::AnyNumber());
  EXPECT_CALL(field_observer_, OnKillFocus()).Times(testing::AnyNumber());
  EXPECT_CALL(field_observer_, OnDidEndEditing()).Times(testing::AnyNumber());
  EXPECT_CALL(field_observer_, OnBeforeDrawRect()).Times(testing::AnyNumber());
  EXPECT_CALL(field_observer_, OnDidDrawRect()).Times(testing::AnyNumber());

  // Ensure the field is currently not first responder.
  [test_window() makePretendKeyWindowAndSetFirstResponder:nil];
  NSResponder* firstResponder = [[field_ window] firstResponder];
  EXPECT_FALSE(
      [base::mac::ObjCCast<NSView>(firstResponder) isDescendantOf:field_]);

  // Clicking an interactive decoration doesn't change the first responder.
  SendMouseClickToDecoration(&interactive_decoration);
  EXPECT_NSEQ(firstResponder, [[field_ window] firstResponder]);

  // Clicking a non-interactive decoration focuses the Omnibox.
  SendMouseClickToDecoration(&noninteractive_decoration);
  firstResponder = [[field_ window] firstResponder];
  EXPECT_TRUE(
      [base::mac::ObjCCast<NSView>(firstResponder) isDescendantOf:field_]);

  // Clicking an interactive decoration doesn't change the first responder.
  SendMouseClickToDecoration(&interactive_decoration);
  EXPECT_NSEQ(firstResponder, [[field_ window] firstResponder]);
}

// Verify the behavior of AcceptsPress::WHEN_ACTIVATED.
TEST_F(AutocompleteTextFieldObserverTest, ReceivePressOnlyWhenActivated) {
  AutocompleteTextFieldCell* cell = [field_ cell];

  // Have the mock use AcceptsPress::WHEN_ACTIVATED.
  MockDecoration decoration;
  EXPECT_CALL(decoration, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::WHEN_ACTIVATED));

  decoration.SetVisible(true);
  [cell addLeadingDecoration:&decoration];
  [cell updateMouseTrackingAndToolTipsInRect:[field_ frame] ofView:field_];

  EXPECT_CALL(field_observer_, OnMouseDown(_)).Times(testing::AnyNumber());

  // Expect a call to OnMousePressed(), since the decoration is not active yet.
  EXPECT_CALL(decoration, OnMousePressed(_, _)).WillOnce(Return(true));
  SendMouseClickToDecoration(&decoration);
  decoration.SetActive(true);

  // Click again: should be no additional calls, since the decoration is active.
  EXPECT_CALL(decoration, OnMousePressed(_, _)).Times(0);
  SendMouseClickToDecoration(&decoration);

  // Most popups will call SetActive(false) due to a click occurring outside the
  // bubble. (But there is defensive code that makes this unnecessary). Test
  // what a popup _should_ do.
  decoration.SetActive(false);
  EXPECT_CALL(decoration, OnMousePressed(_, _)).WillOnce(Return(true));
  SendMouseClickToDecoration(&decoration);

  // And test a misbehaving bubble that forgets to call SetActive(false). It
  // should still see the second click.
  decoration.SetActive(true);
  EXPECT_CALL(decoration, OnMousePressed(_, _)).Times(0);
  SendMouseClickToDecoration(&decoration);
  EXPECT_CALL(decoration, OnMousePressed(_, _)).WillOnce(Return(true));
  SendMouseClickToDecoration(&decoration);

  // Verify that AcceptsPress::ALWAYS receives presses when active.
  EXPECT_CALL(decoration, AcceptsMousePress())
      .WillRepeatedly(Return(AcceptsPress::ALWAYS));
  decoration.SetActive(true);
  EXPECT_CALL(decoration, OnMousePressed(_, _)).WillOnce(Return(true));
  SendMouseClickToDecoration(&decoration);
}

TEST_F(AutocompleteTextFieldObserverTest, SendsEditingMessages) {
  // Many of these methods try to change the selection.
  EXPECT_CALL(field_observer_, SelectionRangeForProposedRange(A<NSRange>()))
      .WillRepeatedly(ReturnArg<0>());

  EXPECT_CALL(field_observer_, OnSetFocus(false));
  // Becoming first responder doesn't begin editing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  NSTextView* editor = static_cast<NSTextView*>([field_ currentEditor]);
  EXPECT_TRUE(nil != editor);

  // This should begin editing and indicate a change.
  EXPECT_CALL(field_observer_, OnDidBeginEditing());
  EXPECT_CALL(field_observer_, OnBeforeChange());
  EXPECT_CALL(field_observer_, OnDidChange());
  [editor shouldChangeTextInRange:NSMakeRange(0, 0) replacementString:@""];
  [editor didChangeText];

  // Further changes don't send the begin message.
  EXPECT_CALL(field_observer_, OnBeforeChange());
  EXPECT_CALL(field_observer_, OnDidChange());
  [editor shouldChangeTextInRange:NSMakeRange(0, 0) replacementString:@""];
  [editor didChangeText];

  // -doCommandBySelector: should forward to observer via |field_|.
  // TODO(shess): Test with a fake arrow-key event?
  const SEL cmd = @selector(moveDown:);
  EXPECT_CALL(field_observer_, OnDoCommandBySelector(cmd))
      .WillOnce(Return(true));
  [editor doCommandBySelector:cmd];

  // Finished with the changes.
  EXPECT_CALL(field_observer_, OnKillFocus());
  EXPECT_CALL(field_observer_, OnDidEndEditing());
  [test_window() clearPretendKeyWindowAndFirstResponder];
}

// Test that the resign-key notification is forwarded right, and that
// the notification is registered and unregistered when the view moves
// in and out of the window.
// TODO(shess): Should this test the key window for realz?  That would
// be really annoying to whoever is running the tests.
TEST_F(AutocompleteTextFieldObserverTest, ClosePopupOnResignKey) {
  EXPECT_CALL(field_observer_, ClosePopup());
  [test_window() resignKeyWindow];

  base::scoped_nsobject<AutocompleteTextField> pin([field_ retain]);
  [field_ removeFromSuperview];
  [test_window() resignKeyWindow];

  [[test_window() contentView] addSubview:field_];
  EXPECT_CALL(field_observer_, ClosePopup());
  [test_window() resignKeyWindow];
}

}  // namespace
