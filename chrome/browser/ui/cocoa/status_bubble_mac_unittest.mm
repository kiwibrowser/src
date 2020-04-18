// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/status_bubble_mac.h"

#include <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/bubble_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

using base::UTF8ToUTF16;

// The test delegate records all of the status bubble object's state
// transitions.
@interface StatusBubbleMacTestDelegate : NSObject {
 @private
  NSWindow* window_;  // Weak.
  NSPoint baseFrameOffset_;
  std::vector<StatusBubbleMac::StatusBubbleState> states_;
}
- (id)initWithWindow:(NSWindow*)window;
- (void)forceBaseFrameOffset:(NSPoint)baseFrameOffset;
- (NSRect)statusBubbleBaseFrame;
- (void)statusBubbleWillEnterState:(StatusBubbleMac::StatusBubbleState)state;
@end
@implementation StatusBubbleMacTestDelegate
- (id)initWithWindow:(NSWindow*)window {
  if ((self = [super init])) {
    window_ = window;
    baseFrameOffset_ = NSZeroPoint;
  }
  return self;
}
- (void)forceBaseFrameOffset:(NSPoint)baseFrameOffset {
  baseFrameOffset_ = baseFrameOffset;
}
- (NSRect)statusBubbleBaseFrame {
  NSView* contentView = [window_ contentView];
  NSRect baseFrame = [contentView convertRect:[contentView frame] toView:nil];
  if (baseFrameOffset_.x > 0 || baseFrameOffset_.y > 0) {
    baseFrame = NSOffsetRect(baseFrame, baseFrameOffset_.x, baseFrameOffset_.y);
    baseFrame.size.width -= baseFrameOffset_.x;
    baseFrame.size.height -= baseFrameOffset_.y;
  }
  return baseFrame;
}
- (void)statusBubbleWillEnterState:(StatusBubbleMac::StatusBubbleState)state {
  states_.push_back(state);
}
- (std::vector<StatusBubbleMac::StatusBubbleState>*)states {
  return &states_;
}
@end

// This class implements, for testing purposes, a subclass of |StatusBubbleMac|
// whose |MouseMoved()| method does nothing. This lets the tests fake the mouse
// position and avoid being affected by the true mouse position.
class StatusBubbleMacIgnoreMouseMoved : public StatusBubbleMac {
 public:
  StatusBubbleMacIgnoreMouseMoved(NSWindow* parent, id delegate)
      : StatusBubbleMac(parent, delegate), mouseLocation_(0, 0) {
    // Set the fake mouse position to the top right of the content area.
    NSRect contentBounds = [[parent contentView] bounds];
    mouseLocation_.SetPoint(NSMaxX(contentBounds), NSMaxY(contentBounds));
  }

  void MouseMoved(bool left_content) override {}

  gfx::Point GetMouseLocation() override { return mouseLocation_; }

  void SetMouseLocationForTesting(int x, int y) {
    mouseLocation_.SetPoint(x, y);
    StatusBubbleMac::MouseMovedAt(gfx::Point(x, y), false);
  }

 private:
  gfx::Point mouseLocation_;
};

class StatusBubbleMacTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    CocoaTestHelperWindow* window = test_window();
    EXPECT_TRUE(window);
    delegate_.reset(
        [[StatusBubbleMacTestDelegate alloc] initWithWindow: window]);
    EXPECT_TRUE(delegate_.get());
    bubble_ = new StatusBubbleMacIgnoreMouseMoved(window, delegate_);
    EXPECT_TRUE(bubble_);

    // Turn off delays and transitions for test mode.  This doesn't just speed
    // things along, it's actually required to get StatusBubbleMac to behave
    // synchronously, because the tests here don't know how to wait for
    // results.  This allows these tests to be much more complete with a
    // minimal loss of coverage and without any heinous rearchitecting.
    bubble_->immediate_ = true;

    EXPECT_TRUE(bubble_->window_);  // immediately creates window
  }

  void TearDown() override {
    // Not using a scoped_ptr because bubble must be deleted before calling
    // TearDown to get rid of bubble's window.
    delete bubble_;
    CocoaTest::TearDown();
  }

  bool IsVisible() {
    if (![bubble_->window_ isVisible])
      return false;
    return [bubble_->window_ alphaValue] > 0.0;
  }
  NSString* GetText() {
    return bubble_->status_text_;
  }
  NSString* GetURLText() {
    return bubble_->url_text_;
  }
  NSString* GetBubbleViewText() {
    BubbleView* bubbleView = [bubble_->window_ contentView];
    return [bubbleView content];
  }
  NSWindow* GetWindow() { return bubble_->GetWindow(); }
  NSWindow* parent() {
    return bubble_->parent_;
  }
  StatusBubbleMac::StatusBubbleState GetState() {
    return bubble_->state_;
  }
  void SetState(StatusBubbleMac::StatusBubbleState state) {
    bubble_->SetState(state);
  }
  std::vector<StatusBubbleMac::StatusBubbleState>* States() {
    return [delegate_ states];
  }
  StatusBubbleMac::StatusBubbleState StateAt(int index) {
    return (*States())[index];
  }

  bool IsPointInBubble(int x, int y) {
    return NSPointInRect(NSMakePoint(x, y), [GetWindow() frame]);
  }

  void SetMouseLocation(int relative_x, int relative_y) {
    // Convert to screen coordinates.
    NSRect window_frame = [test_window() frame];
    int x = relative_x + window_frame.origin.x;
    int y = relative_y + window_frame.origin.y;

    ((StatusBubbleMacIgnoreMouseMoved*)
      bubble_)->SetMouseLocationForTesting(x, y);
  }

  // Test helper for moving the fake mouse location, and checking that
  // the bubble avoids that location.
  // For convenience & clarity, coordinates are relative to the main window.
  bool CheckAvoidsMouse(int relative_x, int relative_y) {
    SetMouseLocation(relative_x, relative_y);
    return !IsPointInBubble(relative_x, relative_y);
  }

  base::MessageLoop message_loop_;
  base::scoped_nsobject<StatusBubbleMacTestDelegate> delegate_;
  StatusBubbleMac* bubble_;  // Strong.
};

TEST_F(StatusBubbleMacTest, SetStatus) {
  bubble_->SetStatus(base::string16());
  bubble_->SetStatus(UTF8ToUTF16("This is a test"));
  EXPECT_NSEQ(@"This is a test", GetText());
  EXPECT_TRUE(IsVisible());

  // Set the status to the exact same thing again
  bubble_->SetStatus(UTF8ToUTF16("This is a test"));
  EXPECT_NSEQ(@"This is a test", GetText());

  // Hide it
  bubble_->SetStatus(base::string16());
  EXPECT_FALSE(IsVisible());
}

TEST_F(StatusBubbleMacTest, SetURL) {
  bubble_->SetURL(GURL());
  EXPECT_FALSE(IsVisible());
  bubble_->SetURL(GURL("bad url"));
  EXPECT_FALSE(IsVisible());
  bubble_->SetURL(GURL("http://"));
  EXPECT_FALSE(IsVisible());
  bubble_->SetURL(GURL("about:blank"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"about:blank", GetURLText());
  bubble_->SetURL(GURL("foopy://"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"foopy://", GetURLText());
  bubble_->SetURL(GURL("http://www.cnn.com"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"www.cnn.com", GetURLText());
}

// Test hiding bubble that's already hidden.
TEST_F(StatusBubbleMacTest, Hides) {
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  EXPECT_TRUE(IsVisible());
  bubble_->Hide();
  EXPECT_FALSE(IsVisible());
  bubble_->Hide();
  EXPECT_FALSE(IsVisible());
}

// Test the "main"/"backup" behavior in StatusBubbleMac::SetText().
TEST_F(StatusBubbleMacTest, SetStatusAndURL) {
  EXPECT_FALSE(IsVisible());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"Status", GetBubbleViewText());
  bubble_->SetURL(GURL("http://www.nytimes.com"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"www.nytimes.com", GetBubbleViewText());
  bubble_->SetURL(GURL());
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"Status", GetBubbleViewText());
  bubble_->SetStatus(base::string16());
  EXPECT_FALSE(IsVisible());
  bubble_->SetURL(GURL("http://www.nytimes.com"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"www.nytimes.com", GetBubbleViewText());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"Status", GetBubbleViewText());
  bubble_->SetStatus(base::string16());
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"www.nytimes.com", GetBubbleViewText());
  bubble_->SetURL(GURL());
  EXPECT_FALSE(IsVisible());
}

// Test that the status bubble goes through the correct delay and fade states.
// The delay and fade duration are simulated and not actually experienced
// during the test because StatusBubbleMacTest sets immediate_ mode.
TEST_F(StatusBubbleMacTest, StateTransitions) {
  // First, some sanity

  EXPECT_FALSE(IsVisible());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());

  States()->clear();
  EXPECT_TRUE(States()->empty());

  bubble_->SetStatus(base::string16());
  EXPECT_FALSE(IsVisible());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_TRUE(States()->empty());  // no change from initial kBubbleHidden state

  // Next, a few ordinary cases

  // Test StartShowing from kBubbleHidden
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_TRUE(IsVisible());
  // Check GetState before checking States to make sure that all state
  // transitions have been flushed to States.
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_EQ(3u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleShowingTimer, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleShowingFadeIn, StateAt(1));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, StateAt(2));

  // Test StartShowing from kBubbleShown with the same message
  States()->clear();
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_TRUE(IsVisible());
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_TRUE(States()->empty());

  // Test StartShowing from kBubbleShown with a different message
  bubble_->SetStatus(UTF8ToUTF16("New Status"));
  EXPECT_TRUE(IsVisible());
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_TRUE(States()->empty());

  // Test StartHiding from kBubbleShown
  bubble_->SetStatus(base::string16());
  EXPECT_FALSE(IsVisible());
  // Check GetState before checking States to make sure that all state
  // transitions have been flushed to States.
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(3u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidingTimer, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleHidingFadeOut, StateAt(1));
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(2));

  // Test StartHiding from kBubbleHidden
  States()->clear();
  bubble_->SetStatus(base::string16());
  EXPECT_FALSE(IsVisible());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_TRUE(States()->empty());

  // Now, the edge cases

  // Test StartShowing from kBubbleShowingTimer
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingTimer);
  [GetWindow() setAlphaValue:0.0];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_EQ(2u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleShowingFadeIn, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, StateAt(1));

  // Test StartShowing from kBubbleShowingFadeIn
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingFadeIn);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  // The actual state values can't be tested in immediate_ mode because
  // the window wasn't actually fading in.  Without immediate_ mode,
  // expect kBubbleShown.
  bubble_->SetStatus(base::string16());  // Go back to a deterministic state.

  // Test StartShowing from kBubbleHidingTimer
  bubble_->SetStatus(base::string16());
  SetState(StatusBubbleMac::kBubbleHidingTimer);
  [GetWindow() setAlphaValue:1.0];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, StateAt(0));

  // Test StartShowing from kBubbleHidingFadeOut
  bubble_->SetStatus(base::string16());
  SetState(StatusBubbleMac::kBubbleHidingFadeOut);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, GetState());
  EXPECT_EQ(2u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleShowingFadeIn, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleShown, StateAt(1));

  // Test StartHiding from kBubbleShowingTimer
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingTimer);
  [GetWindow() setAlphaValue:0.0];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(base::string16());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(0));

  // Test StartHiding from kBubbleShowingFadeIn
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingFadeIn);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(base::string16());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(2u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidingFadeOut, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(1));

  // Test StartHiding from kBubbleHidingTimer
  bubble_->SetStatus(base::string16());
  SetState(StatusBubbleMac::kBubbleHidingTimer);
  [GetWindow() setAlphaValue:1.0];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(base::string16());
  // The actual state values can't be tested in immediate_ mode because
  // the timer wasn't actually running.  Without immediate_ mode, expect
  // kBubbleHidingFadeOut and kBubbleHidden.
  // Go back to a deterministic state.
  bubble_->SetStatus(UTF8ToUTF16("Status"));

  // Test StartHiding from kBubbleHidingFadeOut
  bubble_->SetStatus(base::string16());
  SetState(StatusBubbleMac::kBubbleHidingFadeOut);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->SetStatus(base::string16());
  // The actual state values can't be tested in immediate_ mode because
  // the window wasn't actually fading out.  Without immediate_ mode, expect
  // kBubbleHidden.
  // Go back to a deterministic state.
  bubble_->SetStatus(UTF8ToUTF16("Status"));

  // Test Hide from kBubbleHidden
  bubble_->SetStatus(base::string16());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_TRUE(States()->empty());

  // Test Hide from kBubbleShowingTimer
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingTimer);
  [GetWindow() setAlphaValue:0.0];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(0));

  // Test Hide from kBubbleShowingFadeIn
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleShowingFadeIn);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(2u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidingFadeOut, StateAt(0));
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(1));

  // Test Hide from kBubbleShown
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(0));

  // Test Hide from kBubbleHidingTimer
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleHidingTimer);
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(0));

  // Test Hide from kBubbleHidingFadeOut
  bubble_->SetStatus(UTF8ToUTF16("Status"));
  SetState(StatusBubbleMac::kBubbleHidingFadeOut);
  [GetWindow() setAlphaValue:0.5];
  States()->clear();
  EXPECT_TRUE(States()->empty());
  bubble_->Hide();
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, GetState());
  EXPECT_EQ(1u, States()->size());
  EXPECT_EQ(StatusBubbleMac::kBubbleHidden, StateAt(0));
}

TEST_F(StatusBubbleMacTest, Delete) {
  NSWindow* window = test_window();
  // Create and delete immediately.
  StatusBubbleMac* bubble = new StatusBubbleMac(window, nil);
  delete bubble;

  // Create then delete while visible.
  bubble = new StatusBubbleMac(window, nil);
  bubble->SetStatus(UTF8ToUTF16("showing"));
  delete bubble;
}

TEST_F(StatusBubbleMacTest, UpdateSizeAndPosition) {
  // Test |UpdateSizeAndPosition()| when status bubble does not exist (shouldn't
  // crash; shouldn't create window).
  EXPECT_TRUE(GetWindow());
  bubble_->UpdateSizeAndPosition();
  EXPECT_TRUE(GetWindow());

  // Create a status bubble (with contents) and call resize (without actually
  // resizing); the frame size shouldn't change.
  bubble_->SetStatus(UTF8ToUTF16("UpdateSizeAndPosition test"));
  ASSERT_TRUE(GetWindow());
  NSRect rect_before = [GetWindow() frame];
  bubble_->UpdateSizeAndPosition();
  NSRect rect_after = [GetWindow() frame];
  EXPECT_NSEQ(rect_before, rect_after);

  // Move the window and call resize; only the origin should change.
  NSWindow* window = test_window();
  ASSERT_TRUE(window);
  NSRect frame = [window frame];
  rect_before = [GetWindow() frame];
  frame.origin.x += 10.0;  // (fairly arbitrary nonzero value)
  frame.origin.y += 10.0;  // (fairly arbitrary nonzero value)
  [window setFrame:frame display:YES];
  bubble_->UpdateSizeAndPosition();
  rect_after = [GetWindow() frame];
  EXPECT_NE(rect_before.origin.x, rect_after.origin.x);
  EXPECT_NE(rect_before.origin.y, rect_after.origin.y);
  EXPECT_EQ(rect_before.size.width, rect_after.size.width);
  EXPECT_EQ(rect_before.size.height, rect_after.size.height);

  // Resize the window (without moving). The origin shouldn't change. The width
  // should change (in the current implementation), but not the height.
  frame = [window frame];
  rect_before = [GetWindow() frame];
  frame.size.width += 50.0;   // (fairly arbitrary nonzero value)
  frame.size.height += 50.0;  // (fairly arbitrary nonzero value)
  [window setFrame:frame display:YES];
  bubble_->UpdateSizeAndPosition();
  rect_after = [GetWindow() frame];
  EXPECT_EQ(rect_before.origin.x, rect_after.origin.x);
  EXPECT_EQ(rect_before.origin.y, rect_after.origin.y);
  EXPECT_NE(rect_before.size.width, rect_after.size.width);
  EXPECT_EQ(rect_before.size.height, rect_after.size.height);
}

TEST_F(StatusBubbleMacTest, MovingWindowUpdatesPosition) {
  NSWindow* window = test_window();

  // Show the bubble and make sure it has the same origin as |window|.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  NSWindow* child = GetWindow();
  EXPECT_NSEQ([window frame].origin, [child frame].origin);

  // Hide the bubble, move the window, and show it again.
  bubble_->Hide();
  NSRect frame = [window frame];
  frame.origin.x += 50;
  [window setFrame:frame display:YES];
  bubble_->SetStatus(UTF8ToUTF16("Reshowing"));

  // The bubble should reattach in the correct location.
  child = GetWindow();
  EXPECT_NSEQ([window frame].origin, [child frame].origin);
}

TEST_F(StatusBubbleMacTest, StatuBubbleRespectsBaseFrameLimits) {
  NSWindow* window = test_window();

  // Show the bubble and make sure it has the same origin as |window|.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  NSWindow* child = GetWindow();
  EXPECT_NSEQ([window frame].origin, [child frame].origin);

  // Hide the bubble, change base frame offset, and show it again.
  bubble_->Hide();

  NSPoint baseFrameOffset = NSMakePoint(0, [window frame].size.height / 3);
  EXPECT_GT(baseFrameOffset.y, 0);
  [delegate_ forceBaseFrameOffset:baseFrameOffset];

  bubble_->SetStatus(UTF8ToUTF16("Reshowing"));

  // The bubble should reattach in the correct location.
  child = GetWindow();
  NSPoint expectedOrigin = [window frame].origin;
  expectedOrigin.x += baseFrameOffset.x;
  expectedOrigin.y += baseFrameOffset.y;
  EXPECT_NSEQ(expectedOrigin, [child frame].origin);
}

TEST_F(StatusBubbleMacTest, ExpandBubble) {
  NSWindow* window = test_window();

  // The system font changes between OSX 10.9 and OSX 10.10. Use the system
  // font from OSX 10.9 for this test.
  id mockContentView =
      [OCMockObject partialMockForObject:[GetWindow() contentView]];
  [[[mockContentView stub]
      andReturn:[NSFont fontWithName:@"Lucida Grande" size:11]] font];

  ASSERT_TRUE(window);
  NSRect window_frame = [window frame];
  window_frame.size.width = 600.0;
  [window setFrame:window_frame display:YES];

  // Check basic expansion
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  EXPECT_TRUE(IsVisible());
  bubble_->SetURL(GURL("http://www.battersbox.com/peter_paul_and_mary.html"));
  EXPECT_TRUE([GetURLText() hasSuffix:@"\u2026"]);
  bubble_->ExpandBubble();
  EXPECT_TRUE(IsVisible());
  EXPECT_NSEQ(@"www.battersbox.com/peter_paul_and_mary.html", GetURLText());
  bubble_->Hide();

  // Make sure bubble resets after hide.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  bubble_->SetURL(GURL("http://www.snickersnee.com/pioneer_fishstix.html"));
  EXPECT_TRUE([GetURLText() hasSuffix:@"\u2026"]);
  // ...and that it expands again properly.
  bubble_->ExpandBubble();
  EXPECT_NSEQ(@"www.snickersnee.com/pioneer_fishstix.html", GetURLText());
  // ...again, again!
  bubble_->SetURL(GURL("http://www.battersbox.com/peter_paul_and_mary.html"));
  bubble_->ExpandBubble();
  EXPECT_NSEQ(@"www.battersbox.com/peter_paul_and_mary.html", GetURLText());
  bubble_->Hide();

  window_frame = [window frame];
  window_frame.size.width = 300.0;
  [window setFrame:window_frame display:YES];

  // Very long URL's will be cut off even in the expanded state.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  const char veryLongUrl[] =
      "http://www.diewahrscheinlichlaengstepralinederwelt.com/duuuuplo.html";
  bubble_->SetURL(GURL(veryLongUrl));
  EXPECT_TRUE([GetURLText() hasSuffix:@"\u2026"]);
  bubble_->ExpandBubble();
  EXPECT_TRUE([GetURLText() hasSuffix:@"\u2026"]);
}

TEST_F(StatusBubbleMacTest, BubbleAvoidsMouse) {
  NSWindow* window = test_window();

  // All coordinates here are relative to the window origin.

  // Initially, the bubble should appear in the bottom left.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  EXPECT_TRUE(IsPointInBubble(0, 0));
  bubble_->Hide();

  // Check that the bubble doesn't appear in the left corner if the
  // mouse is currently located there.
  SetMouseLocation(0, 0);
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  EXPECT_FALSE(IsPointInBubble(0, 0));

  // Leave the bubble visible, and try moving the mouse around.
  int smallValue = NSHeight([GetWindow() frame]) / 2;
  EXPECT_TRUE(CheckAvoidsMouse(0, 0));
  EXPECT_TRUE(CheckAvoidsMouse(smallValue, 0));
  EXPECT_TRUE(CheckAvoidsMouse(0, smallValue));
  EXPECT_TRUE(CheckAvoidsMouse(smallValue, smallValue));

  // Simulate moving the mouse down from the top of the window.
  for (int y = NSHeight([window frame]); y >= 0; y -= smallValue) {
    ASSERT_TRUE(CheckAvoidsMouse(smallValue, y));
  }

  // Simulate moving the mouse from left to right.
  int windowWidth = NSWidth([window frame]);
  for (int x = 0; x < windowWidth; x += smallValue) {
    ASSERT_TRUE(CheckAvoidsMouse(x, smallValue));
  }

  // Simulate moving the mouse from right to left.
  for (int x = windowWidth; x >= 0; x -= smallValue) {
    ASSERT_TRUE(CheckAvoidsMouse(x, smallValue));
  }
}

TEST_F(StatusBubbleMacTest, ReparentBubble) {
  // The second window is borderless, like the window used in fullscreen mode.
  base::scoped_nsobject<NSWindow> fullscreenParent(
      [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);

  // Switch parents with the bubble hidden.
  bubble_->SwitchParentWindow(fullscreenParent);

  // Switch back to the original parent with the bubble showing.
  bubble_->SetStatus(UTF8ToUTF16("Showing"));
  bubble_->SwitchParentWindow(test_window());
}

TEST_F(StatusBubbleMacTest, WaitsUntilVisible) {
  [test_window() orderOut:nil];
  bubble_->SetStatus(UTF8ToUTF16("Show soon"));
  EXPECT_NSEQ(nil, GetWindow().parentWindow);

  [test_window() orderBack:nil];
  EXPECT_NSNE(nil, GetWindow().parentWindow);
}

TEST_F(StatusBubbleMacTest, WaitsUntilOnActiveSpace) {
  test_window().pretendIsOnActiveSpace = NO;
  bubble_->SetStatus(UTF8ToUTF16("Show soon"));
  EXPECT_NSEQ(nil, GetWindow().parentWindow);

  test_window().pretendIsOnActiveSpace = YES;
  EXPECT_NSNE(nil, GetWindow().parentWindow);
}
