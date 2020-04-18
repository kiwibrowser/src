// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/command_line.h"
#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/extensions/extension_action_test_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/load_error_reporter.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/translate_decoration.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/scoped_force_rtl_mac.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_view_cocoa.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

// An NSView that fakes out hitTest:.
@interface HitView : NSView {
  id hitTestReturn_;
}
@end

@implementation HitView

- (void)setHitTestReturn:(id)rtn {
  hitTestReturn_ = rtn;
}

- (NSView *)hitTest:(NSPoint)aPoint {
  return hitTestReturn_;
}

@end

// Records the last command id and enabled state it has received so it can be
// queried by the tests to see if we got a notification or not.
@interface TestToolbarController : ToolbarController {
 @private
  int lastCommand_;  // Id of last received state change.
  bool lastState_;   // State of last received state change.
}
@property(nonatomic, readonly) int lastCommand;
@property(nonatomic, readonly) bool lastState;
@end

@implementation TestToolbarController
@synthesize lastCommand = lastCommand_;
@synthesize lastState = lastState_;
- (void)enabledStateChangedForCommand:(int)command enabled:(bool)enabled {
  [super enabledStateChangedForCommand:command enabled:enabled];
  lastCommand_ = command;
  lastState_ = enabled;
}
@end

namespace {

class ToolbarControllerTest : public CocoaProfileTest {
 public:

  // Indexes that match the ordering returned by the private ToolbarController
  // |-toolbarViews| method.
  enum SubviewIndex {
    kBackIndex,
    kForwardIndex,
    kReloadIndex,
    kHomeIndex,
    kLocationIndex,
    kBrowserActionContainerViewIndex,
    kAppMenuIndex
  };

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    // Add an extension so the browser action container view
    // is visible and has a real size/position.
    extensions::TestExtensionSystem* extension_system =
        static_cast<extensions::TestExtensionSystem*>(
            extensions::ExtensionSystem::Get(profile()));
    extension_system->CreateExtensionService(
        base::CommandLine::ForCurrentProcess(), base::FilePath(), false);
    scoped_refptr<const extensions::Extension> extension =
        extensions::ExtensionBuilder("ABC")
            .SetAction(extensions::ExtensionBuilder::ActionType::BROWSER_ACTION)
            .Build();
    extensions::ExtensionSystem::Get(profile())
        ->extension_service()
        ->AddExtension(extension.get());
    extensions::LoadErrorReporter::Init(false);
    ToolbarActionsModel* model =
        extensions::extension_action_test_util::CreateToolbarModelForProfile(
            profile());
    model->SetVisibleIconCount(1);

    resizeDelegate_.reset([[ViewResizerPong alloc] init]);

    CommandUpdater* updater = browser()->command_controller();
    // The default state for the commands is true, set a couple to false to
    // ensure they get picked up correct on initialization
    updater->UpdateCommandEnabled(IDC_BACK, false);
    updater->UpdateCommandEnabled(IDC_FORWARD, false);
    bar_.reset([[TestToolbarController alloc]
        initWithCommands:browser()->command_controller()
                 profile:profile()
                 browser:browser()]);
    [[bar_ toolbarView] setResizeDelegate:resizeDelegate_.get()];
    EXPECT_TRUE([bar_ view]);
    NSView* parent = [test_window() contentView];
    [parent addSubview:[bar_ view]];

    // Nudge a few things to ensure the browser actions container gets
    // laid out.
    [bar_ createBrowserActionButtons];
    [[bar_ browserActionsController] update];
    [bar_ toolbarFrameChanged];
  }

  void TearDown() override {
    // Releasing ToolbarController doesn't actually free it at this point, since
    // the NSViewController retains a reference to it from the nib loading.
    // As browser() is released in the superclass TearDown, call
    // -[ToolbarController browserWillBeDestroyed] to prevent a use after free
    // issue on the |browser_| pointer in LocationBarViewMac when
    // ToolbarController is actually freed (some time after this method is run).
    [bar_ browserWillBeDestroyed];
    bar_.reset();
    CocoaProfileTest::TearDown();
  }

  // Make sure the enabled state of the view is the same as the corresponding
  // command in the updater. The views are in the declaration order of outlets.
  void CompareState(CommandUpdater* updater, NSArray* views) {
    EXPECT_EQ(updater->IsCommandEnabled(IDC_BACK),
              [[views objectAtIndex:kBackIndex] isEnabled] ? true : false);
    EXPECT_EQ(updater->IsCommandEnabled(IDC_FORWARD),
              [[views objectAtIndex:kForwardIndex] isEnabled] ? true : false);
    EXPECT_EQ(updater->IsCommandEnabled(IDC_RELOAD),
              [[views objectAtIndex:kReloadIndex] isEnabled] ? true : false);
    EXPECT_EQ(updater->IsCommandEnabled(IDC_HOME),
              [[views objectAtIndex:kHomeIndex] isEnabled] ? true : false);
  }

  NSView* GetSubviewAt(SubviewIndex index) {
    return [[bar_ toolbarViews] objectAtIndex:index];
  }

  base::scoped_nsobject<ViewResizerPong> resizeDelegate_;
  base::scoped_nsobject<TestToolbarController> bar_;
};

TEST_VIEW(ToolbarControllerTest, [bar_ view])

// Test the initial state that everything is sync'd up
TEST_F(ToolbarControllerTest, InitialState) {
  CommandUpdater* updater = browser()->command_controller();
  CompareState(updater, [bar_ toolbarViews]);
}

// Make sure a "titlebar only" toolbar with location bar works.
TEST_F(ToolbarControllerTest, TitlebarOnly) {
  NSView* view = [bar_ view];

  [bar_ setHasToolbar:NO hasLocationBar:YES];
  EXPECT_EQ(view, [bar_ view]);

  // Simulate a popup going fullscreen and back by performing the reparenting
  // that happens during fullscreen transitions
  NSView* superview = [view superview];
  [view removeFromSuperview];
  [superview addSubview:view];

  EXPECT_EQ(view, [bar_ view]);
}

// Test updateVisibility with location bar only; this method is used by bookmark
// apps, and should never be called when the toolbar is enabled. Ensure that the
// buttons remain in the correct state.
TEST_F(ToolbarControllerTest, UpdateVisibility) {
  NSView* view = [bar_ view];

  // Test the escapable states first.
  [bar_ setHasToolbar:YES hasLocationBar:YES];
  EXPECT_GT([[bar_ view] frame].size.height, 0);
  EXPECT_GT([[bar_ view] frame].size.height,
            [GetSubviewAt(kLocationIndex) frame].size.height);
  EXPECT_GT([[bar_ view] frame].size.width,
            [GetSubviewAt(kLocationIndex) frame].size.width);
  EXPECT_FALSE([view isHidden]);
  EXPECT_FALSE([GetSubviewAt(kLocationIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kBackIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kForwardIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kReloadIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kAppMenuIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kHomeIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kBrowserActionContainerViewIndex) isHidden]);

  // For NO/NO, only the top level toolbar view is hidden.
  [bar_ setHasToolbar:NO hasLocationBar:NO];
  EXPECT_TRUE([view isHidden]);
  EXPECT_FALSE([GetSubviewAt(kLocationIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kBackIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kForwardIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kReloadIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kAppMenuIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kHomeIndex) isHidden]);
  EXPECT_FALSE([GetSubviewAt(kBrowserActionContainerViewIndex) isHidden]);

  // Now test the inescapable state.
  [bar_ setHasToolbar:NO hasLocationBar:YES];
  EXPECT_GT([[bar_ view] frame].size.height, 0);
  EXPECT_EQ([[bar_ view] frame].size.height,
            [GetSubviewAt(kLocationIndex) frame].size.height);
  EXPECT_EQ([[bar_ view] frame].size.width,
            [GetSubviewAt(kLocationIndex) frame].size.width);
  EXPECT_FALSE([view isHidden]);
  EXPECT_FALSE([GetSubviewAt(kLocationIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBackIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kForwardIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kReloadIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kAppMenuIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kHomeIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBrowserActionContainerViewIndex) isHidden]);

  // Maintain visible state.
  [bar_ updateVisibility:YES withAnimation:NO];
  EXPECT_GT([[bar_ view] frame].size.height, 0);
  EXPECT_EQ([[bar_ view] frame].size.height,
            [GetSubviewAt(kLocationIndex) frame].size.height);
  EXPECT_EQ([[bar_ view] frame].size.width,
            [GetSubviewAt(kLocationIndex) frame].size.width);
  EXPECT_FALSE([view isHidden]);
  EXPECT_FALSE([GetSubviewAt(kLocationIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBackIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kForwardIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kReloadIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kAppMenuIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kHomeIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBrowserActionContainerViewIndex) isHidden]);

  // Hide the toolbar and ensure it has height 0.
  [bar_ updateVisibility:NO withAnimation:NO];
  EXPECT_FALSE([view isHidden]);
  EXPECT_EQ(0, [resizeDelegate_ height]);
  EXPECT_EQ(0, [[bar_ view] frame].size.height);

  // Try to show the home button.
  [bar_ showOptionalHomeButton];

  // Re-show the bar. Buttons should remain hidden, including the home button.
  [bar_ updateVisibility:YES withAnimation:NO];
  EXPECT_GT([resizeDelegate_ height], 0);
  EXPECT_GT([[bar_ view] frame].size.height, 0);
  EXPECT_EQ([[bar_ view] frame].size.height,
            [GetSubviewAt(kLocationIndex) frame].size.height);
  EXPECT_EQ([[bar_ view] frame].size.width,
            [GetSubviewAt(kLocationIndex) frame].size.width);
  EXPECT_FALSE([view isHidden]);
  EXPECT_FALSE([GetSubviewAt(kLocationIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBackIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kForwardIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kReloadIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kAppMenuIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kHomeIndex) isHidden]);
  EXPECT_TRUE([GetSubviewAt(kBrowserActionContainerViewIndex) isHidden]);
}

// Make sure it works in the completely undecorated case.
TEST_F(ToolbarControllerTest, NoLocationBar) {
  NSView* view = [bar_ view];

  [bar_ setHasToolbar:NO hasLocationBar:NO];
  EXPECT_TRUE([[bar_ view] isHidden]);

  // Simulate a popup going fullscreen and back by performing the reparenting
  // that happens during fullscreen transitions
  NSView* superview = [view superview];
  [view removeFromSuperview];
  [superview addSubview:view];
}

// Make some changes to the enabled state of a few of the buttons and ensure
// that we're still in sync.
TEST_F(ToolbarControllerTest, UpdateEnabledState) {
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_BACK));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_FORWARD));
  chrome::UpdateCommandEnabled(browser(), IDC_BACK, true);
  chrome::UpdateCommandEnabled(browser(), IDC_FORWARD, true);
  CommandUpdater* updater = browser()->command_controller();
  CompareState(updater, [bar_ toolbarViews]);

  // Change an unwatched command and ensure the last state does not change.
  updater->UpdateCommandEnabled(IDC_MinimumLabelValue, false);
  EXPECT_EQ([bar_ lastCommand], IDC_FORWARD);
  EXPECT_EQ([bar_ lastState], true);
}

// Focus the location bar and make sure that it's the first responder.
TEST_F(ToolbarControllerTest, FocusLocation) {
  NSWindow* window = test_window();
  [window makeFirstResponder:[window contentView]];
  EXPECT_EQ([window firstResponder], [window contentView]);
  [bar_ focusLocationBar:YES];
  EXPECT_NE([window firstResponder], [window contentView]);
  NSView* locationBar = [[bar_ toolbarViews] objectAtIndex:kLocationIndex];
  EXPECT_EQ([window firstResponder], [(id)locationBar currentEditor]);
}

TEST_F(ToolbarControllerTest, LoadingState) {
  // In its initial state, the reload button has a tag of
  // IDC_RELOAD. When loading, it should be IDC_STOP.
  NSButton* reload = [[bar_ toolbarViews] objectAtIndex:kReloadIndex];
  EXPECT_EQ([reload tag], IDC_RELOAD);
  [bar_ setIsLoading:YES force:YES];
  EXPECT_EQ([reload tag], IDC_STOP);
  [bar_ setIsLoading:NO force:YES];
  EXPECT_EQ([reload tag], IDC_RELOAD);
}

// Check that toggling the state of the home button changes the visible
// state of the home button and moves the other items accordingly.
TEST_F(ToolbarControllerTest, ToggleHome) {
  PrefService* prefs = profile()->GetPrefs();
  bool showHome = prefs->GetBoolean(prefs::kShowHomeButton);
  NSView* homeButton = [[bar_ toolbarViews] objectAtIndex:kHomeIndex];
  EXPECT_EQ(showHome, ![homeButton isHidden]);

  NSView* locationBar = [[bar_ toolbarViews] objectAtIndex:kLocationIndex];
  NSRect originalLocationBarFrame = [locationBar frame];

  // Toggle the pref and make sure the button changed state and the other
  // views moved.
  prefs->SetBoolean(prefs::kShowHomeButton, !showHome);
  EXPECT_EQ(showHome, [homeButton isHidden]);
  EXPECT_NE(NSMinX(originalLocationBarFrame), NSMinX([locationBar frame]));
  EXPECT_NE(NSWidth(originalLocationBarFrame), NSWidth([locationBar frame]));
}

// Ensure that we don't toggle the buttons when we have a strip marked as not
// having the full toolbar. Also ensure that the location bar doesn't change
// size.
TEST_F(ToolbarControllerTest, DontToggleWhenNoToolbar) {
  [bar_ setHasToolbar:NO hasLocationBar:YES];
  NSView* homeButton = [[bar_ toolbarViews] objectAtIndex:kHomeIndex];
  NSView* locationBar = [[bar_ toolbarViews] objectAtIndex:kLocationIndex];
  NSRect locationBarFrame = [locationBar frame];
  EXPECT_EQ([homeButton isHidden], YES);
  [bar_ showOptionalHomeButton];
  EXPECT_EQ([homeButton isHidden], YES);
  NSRect newLocationBarFrame = [locationBar frame];
  EXPECT_NSEQ(locationBarFrame, newLocationBarFrame);
  newLocationBarFrame = [locationBar frame];
  EXPECT_NSEQ(locationBarFrame, newLocationBarFrame);
}

TEST_F(ToolbarControllerTest, BookmarkBubblePoint) {
  const NSPoint starPoint = [bar_ bookmarkBubblePoint];
  const NSRect barFrame =
      [[bar_ view] convertRect:[[bar_ view] bounds] toView:nil];

  // Make sure the star is completely inside the location bar.
  EXPECT_TRUE(NSPointInRect(starPoint, barFrame));
}

TEST_F(ToolbarControllerTest, TranslateBubblePoint) {
  LocationBarViewMac* locationBarBridge = [bar_ locationBarBridge];
  LocationBarDecoration* decoration = locationBarBridge->translate_decoration();
  const NSPoint translatePoint =
      locationBarBridge->GetBubblePointForDecoration(decoration);
  const NSRect barFrame =
      [[bar_ view] convertRect:[[bar_ view] bounds] toView:nil];
  EXPECT_TRUE(NSPointInRect(translatePoint, barFrame));
}

TEST_F(ToolbarControllerTest, HoverButtonForEvent) {
  base::scoped_nsobject<HitView> view(
      [[HitView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)]);
  NSView* toolbarView = [bar_ view];
  [bar_ setView:view];
  NSEvent* event = [NSEvent mouseEventWithType:NSMouseMoved
                                      location:NSMakePoint(10,10)
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:nil
                                   eventNumber:0
                                    clickCount:0
                                      pressure:0.0];

  // NOT a match.
  [view setHitTestReturn:bar_.get()];
  EXPECT_FALSE([bar_ hoverButtonForEvent:event]);

  // Not yet...
  base::scoped_nsobject<NSButton> button([[NSButton alloc] init]);
  [view setHitTestReturn:button];
  EXPECT_FALSE([bar_ hoverButtonForEvent:event]);

  // Now!
  base::scoped_nsobject<ImageButtonCell> cell(
      [[ImageButtonCell alloc] init]);
  [button setCell:cell.get()];
  EXPECT_TRUE([bar_ hoverButtonForEvent:nil]);

  // Restore the original view so that
  // -[ToolbarController browserWillBeDestroyed] will run correctly.
  [bar_ setView:toolbarView];
}

// Test that subviews are ordered left to right
TEST_F(ToolbarControllerTest, ElementOrder) {
  NSArray* views = [bar_ toolbarViews];
  for (size_t i = 1; i < [views count]; i++) {
    NSView* previousSubview = views[i - 1];
    NSView* subview = views[i];
    EXPECT_LE(NSMinX([previousSubview frame]), NSMinX([subview frame]));
  }
}

class ToolbarControllerRTLTest : public ToolbarControllerTest {
 public:
  ToolbarControllerRTLTest() {}

 private:
  cocoa_l10n_util::ScopedForceRTLMac rtl_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarControllerRTLTest);
};

// Test that subviews are ordered right to left
TEST_F(ToolbarControllerRTLTest, ElementOrder) {
  NSArray* views = [[[bar_ toolbarViews] reverseObjectEnumerator] allObjects];
  for (size_t i = 1; i < [views count]; i++) {
    NSView* previousSubview = views[i - 1];
    NSView* subview = views[i];
    EXPECT_LE(NSMinX([previousSubview frame]), NSMinX([subview frame]));
  }
}

class BrowserRemovedObserver : public BrowserListObserver {
 public:
  BrowserRemovedObserver() { BrowserList::AddObserver(this); }
  ~BrowserRemovedObserver() override { BrowserList::RemoveObserver(this); }
  void WaitUntilBrowserRemoved() { run_loop_.Run(); }
  void OnBrowserRemoved(Browser* browser) override { run_loop_.Quit(); }

 private:
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(BrowserRemovedObserver);
};

}  // namespace
