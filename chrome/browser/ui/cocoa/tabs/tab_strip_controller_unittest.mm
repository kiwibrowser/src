// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/bind_helpers.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/new_tab_button.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/events/test/cocoa_test_event_utils.h"

using content::SiteInstance;
using content::WebContents;

@interface TabStripControllerForAlertTesting : TabStripController {
  // Keeps alert state of tabs in browser for testing purpose.
  std::map<content::WebContents*, TabAlertState> contentsAlertStateMaps_;
}
@end

@implementation TabStripControllerForAlertTesting
// Returns the alert state of each tab from the map we are keeping.
- (TabAlertState)alertStateForContents:(content::WebContents*)contents {
  return contentsAlertStateMaps_[contents];
}

- (void)setAlertStateForContents:(content::WebContents*)contents
                  withAlertState:(TabAlertState)alert_state {
  contentsAlertStateMaps_[contents] = alert_state;
}

@end

@interface TestTabStripControllerDelegate
    : NSObject<TabStripControllerDelegate> {
}
@end

@implementation TestTabStripControllerDelegate
- (void)onActivateTabWithContents:(WebContents*)contents {
}
- (void)onTabChanged:(TabChangeType)change withContents:(WebContents*)contents {
}
- (void)onTabDetachedWithContents:(WebContents*)contents {
}
- (void)onTabInsertedWithContents:(WebContents*)contents
                     inForeground:(BOOL)inForeground {
}
@end


// Helper class for invoking a base::Closure via
// -performSelector:withObject:afterDelay:.
@interface TestClosureRunner : NSObject {
 @private
  base::Closure closure_;
}
- (id)initWithClosure:(const base::Closure&)closure;
- (void)scheduleDelayedRun;
- (void)run;
@end

@implementation TestClosureRunner
- (id)initWithClosure:(const base::Closure&)closure {
  if (self) {
    closure_ = closure;
  }
  return self;
}
- (void)scheduleDelayedRun {
  [self performSelector:@selector(run) withObject:nil afterDelay:0];
}
- (void)run {
  closure_.Run();
}
@end

@interface TabStripController (Test)

- (void)mouseMoved:(NSEvent*)event;

@end

@implementation TabView (Test)

- (TabController*)controller {
  return controller_;
}

@end

namespace {

class TabStripControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    NSWindow* window = browser()->window()->GetNativeWindow();
    NSView* parent = [window contentView];
    NSRect content_frame = [parent frame];

    // Create the "switch view" (view that gets changed out when a tab
    // switches).
    NSRect switch_frame = NSMakeRect(0, 0, content_frame.size.width, 500);
    base::scoped_nsobject<NSView> switch_view(
        [[NSView alloc] initWithFrame:switch_frame]);
    switch_view_ = switch_view;
    [parent addSubview:switch_view_.get()];

    // Create the tab strip view. It's expected to have a child button in it
    // already as the "new tab" button so create that too.
    NSRect strip_frame = NSMakeRect(0, NSMaxY(switch_frame),
                                    content_frame.size.width, 30);
    tab_strip_.reset(
        [[TabStripView alloc] initWithFrame:strip_frame]);
    [parent addSubview:tab_strip_.get()];
    NSRect button_frame = NSMakeRect(0, 0, 15, 15);
    base::scoped_nsobject<NewTabButton> new_tab_button(
        [[NewTabButton alloc] initWithFrame:button_frame]);
    [tab_strip_ addSubview:new_tab_button.get()];
    [tab_strip_ setNewTabButton:new_tab_button.get()];

    delegate_.reset(new TestTabStripModelDelegate());
    model_ = browser()->tab_strip_model();
    controller_delegate_.reset([TestTabStripControllerDelegate alloc]);
    controller_.reset([[TabStripController alloc]
                      initWithView:static_cast<TabStripView*>(tab_strip_.get())
                        switchView:switch_view.get()
                           browser:browser()
                          delegate:controller_delegate_.get()]);
  }

  void TearDown() override {
    // The call to CocoaTest::TearDown() deletes the Browser and TabStripModel
    // objects, so we first have to delete the controller, which refers to them.
    controller_.reset();
    model_ = NULL;
    CocoaProfileTest::TearDown();
  }

  // Return a derived TabStripController.
  TabStripControllerForAlertTesting* InitTabStripControllerForAlertTesting() {
    TabStripControllerForAlertTesting* c =
        [[TabStripControllerForAlertTesting alloc]
            initWithView:static_cast<TabStripView*>(tab_strip_.get())
              switchView:switch_view_.get()
                 browser:browser()
                delegate:controller_delegate_.get()];
    return c;
  }

  TabView* CreateTab() {
    std::unique_ptr<WebContents> web_contents =
        WebContents::Create(content::WebContents::CreateParams(
            profile(), SiteInstance::Create(profile())));
    model_->AppendWebContents(std::move(web_contents), true);
    const NSUInteger tab_count = [controller_.get() viewsCount];
    return static_cast<TabView*>([controller_.get() viewAtIndex:tab_count - 1]);
  }

  // Closes all tabs and unrefs the tabstrip and then posts a NSLeftMouseUp
  // event which should end the nested drag event loop.
  void CloseTabsAndEndDrag() {
    // Simulate a close of the browser window.
    model_->CloseAllTabs();
    controller_.reset();
    tab_strip_.reset();
    // Schedule a NSLeftMouseUp to end the nested drag event loop.
    NSEvent* event =
        cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp, 0);
    [NSApp postEvent:event atStart:NO];
  }

  std::unique_ptr<TestTabStripModelDelegate> delegate_;
  TabStripModel* model_;
  base::scoped_nsobject<TestTabStripControllerDelegate> controller_delegate_;
  base::scoped_nsobject<TabStripController> controller_;
  base::scoped_nsobject<TabStripView> tab_strip_;
  base::scoped_nsobject<NSView> switch_view_;
};

// Test adding and removing tabs and making sure that views get added to
// the tab strip.
TEST_F(TabStripControllerTest, AddRemoveTabs) {
  EXPECT_TRUE(model_->empty());
  CreateTab();
  EXPECT_EQ(model_->count(), 1);
}

// Clicking a selected (but inactive) tab should activate it.
TEST_F(TabStripControllerTest, ActivateSelectedButInactiveTab) {
  TabView* tab0 = CreateTab();
  TabView* tab1 = CreateTab();
  model_->ToggleSelectionAt(0);
  EXPECT_TRUE([[tab0 controller] selected]);
  EXPECT_TRUE([[tab1 controller] selected]);

  [controller_ selectTab:tab1];
  EXPECT_EQ(1, model_->active_index());
}

// Toggling (cmd-click) a selected (but inactive) tab should deselect it.
TEST_F(TabStripControllerTest, DeselectInactiveTab) {
  TabView* tab0 = CreateTab();
  TabView* tab1 = CreateTab();
  model_->ToggleSelectionAt(0);
  EXPECT_TRUE([[tab0 controller] selected]);
  EXPECT_TRUE([[tab1 controller] selected]);

  model_->ToggleSelectionAt(1);
  EXPECT_TRUE([[tab0 controller] selected]);
  EXPECT_FALSE([[tab1 controller] selected]);
}

TEST_F(TabStripControllerTest, SelectTab) {
  // TODO(pinkerton): Implement http://crbug.com/10899
}

TEST_F(TabStripControllerTest, RearrangeTabs) {
  // TODO(pinkerton): Implement http://crbug.com/10899
}

TEST_F(TabStripControllerTest, CorrectMouseHoverBehavior) {
  TabView* tab1 = CreateTab();
  TabView* tab2 = CreateTab();

  EXPECT_FALSE([tab1 controller].selected);
  EXPECT_TRUE([tab2 controller].selected);

  // Check that there's no hovered tab yet.
  EXPECT_FALSE([controller_ hoveredTab]);

  // Set up mouse event on overlap of tab1 + tab2.
  const CGFloat min_y = NSMinY([tab_strip_.get() frame]) + 1;
  const CGFloat tab_overlap = [TabStripController tabOverlap];

  // Hover over overlap between tab 1 and 2.
  NSRect tab1_frame = [tab1 frame];
  NSPoint tab_overlap_point =
      NSMakePoint(NSMaxX(tab1_frame) - tab_overlap / 2, min_y);

  NSEvent* event = cocoa_test_event_utils::MouseEventAtPoint(tab_overlap_point,
                                                             NSMouseMoved,
                                                             0);
  [controller_.get() mouseMoved:event];
  EXPECT_EQ(tab2, [controller_ hoveredTab]);

  // Hover over tab 1.
  NSPoint hover_point = NSMakePoint(NSMidX(tab1_frame), min_y);
  event =
      cocoa_test_event_utils::MouseEventAtPoint(hover_point, NSMouseMoved, 0);
  [controller_.get() mouseMoved:event];
  EXPECT_EQ(tab1, [controller_ hoveredTab]);

  // Hover over tab 2.
  NSRect tab2_frame = [tab2 frame];
  hover_point = NSMakePoint(NSMidX(tab2_frame), min_y);
  event =
      cocoa_test_event_utils::MouseEventAtPoint(hover_point, NSMouseMoved, 0);
  [controller_.get() mouseMoved:event];
  EXPECT_EQ(tab2, [controller_ hoveredTab]);
}

TEST_F(TabStripControllerTest, CorrectTitleAndToolTipTextFromSetTabTitle) {
  using content::MediaStreamDevice;
  using content::MediaStreamDevices;
  using content::MediaStreamUI;

  TabView* const tab = CreateTab();
  TabController* const tabController = [tab controller];
  WebContents* const contents = model_->GetActiveWebContents();

  // For the duration of the test, assume the tab has been hovered. This adds a
  // subview containing the actual source of the tooltip.
  [controller_ setHoveredTab:tab];
  // Note -[NSView hitTest:] takes superview coordinates. Then, find a spot that
  // is outside the mask image, but inside the tab.
  NSPoint centerPoint = NSMakePoint(5, NSMidY([tab bounds]));
  NSPoint hitPoint = [tab convertPoint:centerPoint
                                toView:[tab_strip_ superview]];
  NSView* toolTipView = [tab_strip_ hitTest:hitPoint];
  EXPECT_TRUE(toolTipView);
  EXPECT_NE(toolTipView, tab);

  // Initially, tab title and tooltip text are equivalent.
  EXPECT_EQ(TabAlertState::NONE,
            chrome::GetTabAlertStateForContents(contents));
  [controller_ setTabTitle:tabController withContents:contents];
  NSString* const baseTitle = [tabController title];
  EXPECT_NSEQ(baseTitle, [tabController toolTip]);
  EXPECT_NSEQ([tabController toolTip], [toolTipView toolTip]);

  // Simulate the start of tab video capture.  Tab title remains the same, but
  // the tooltip text should include the following appended: 1) a line break;
  // 2) a non-empty string with a localized description of the alert state.
  scoped_refptr<MediaStreamCaptureIndicator> indicator =
      MediaCaptureDevicesDispatcher::GetInstance()->
          GetMediaStreamCaptureIndicator();
  const MediaStreamDevice dummyVideoCaptureDevice(
      content::MEDIA_TAB_VIDEO_CAPTURE, "dummy_id", "dummy name");
  std::unique_ptr<MediaStreamUI> streamUi(indicator->RegisterMediaStream(
      contents, MediaStreamDevices(1, dummyVideoCaptureDevice)));
  streamUi->OnStarted(base::DoNothing());
  EXPECT_EQ(TabAlertState::TAB_CAPTURING,
            chrome::GetTabAlertStateForContents(contents));
  [controller_ setTabTitle:tabController withContents:contents];
  EXPECT_NSEQ(baseTitle, [tabController title]);
  NSString* const toolTipText = [tabController toolTip];
  EXPECT_NSEQ(toolTipText, [toolTipView toolTip]);
  if ([baseTitle length] > 0) {
    EXPECT_TRUE(NSEqualRanges(NSMakeRange(0, [baseTitle length]),
                              [toolTipText rangeOfString:baseTitle]));
    EXPECT_TRUE(NSEqualRanges(NSMakeRange([baseTitle length], 1),
                              [toolTipText rangeOfString:@"\n"]));
    EXPECT_LT([baseTitle length] + 1, [toolTipText length]);
  } else {
    EXPECT_LT(0u, [toolTipText length]);
  }

  // Simulate the end of tab video capture.  Tab title and tooltip should become
  // equivalent again.
  streamUi.reset();
  EXPECT_EQ(TabAlertState::NONE,
            chrome::GetTabAlertStateForContents(contents));
  [controller_ setTabTitle:tabController withContents:contents];
  EXPECT_NSEQ(baseTitle, [tabController title]);
  EXPECT_NSEQ(baseTitle, [tabController toolTip]);
  EXPECT_NSEQ(baseTitle, [toolTipView toolTip]);
}

TEST_F(TabStripControllerTest, TabCloseDuringDrag) {
  TabController* tab;
  // The TabController gets autoreleased when created, but is owned by the
  // tab strip model. Use a ScopedNSAutoreleasePool to get a truly weak ref
  // to it to test that -maybeStartDrag:forTab: can handle that properly.
  {
    base::mac::ScopedNSAutoreleasePool pool;
    tab = [CreateTab() controller];
  }

  // Schedule a task to close all the tabs and stop the drag, before the call to
  // -maybeStartDrag:forTab:, which starts a nested event loop. This task will
  // run in that nested event loop, which shouldn't crash.
  base::scoped_nsobject<TestClosureRunner> runner([[TestClosureRunner alloc]
      initWithClosure:base::Bind(&TabStripControllerTest::CloseTabsAndEndDrag,
                                 base::Unretained(this))]);
  [runner scheduleDelayedRun];

  NSEvent* event =
      cocoa_test_event_utils::LeftMouseDownAtPoint(NSZeroPoint);
  [[controller_ dragController] maybeStartDrag:event forTab:tab];
}

TEST_F(TabStripControllerTest, ViewAccessibility_Contents) {
  NSArray* attrs = [tab_strip_ accessibilityAttributeNames];
  ASSERT_TRUE([attrs containsObject:NSAccessibilityContentsAttribute]);

  // Create two tabs and ensure they exist in the contents array.
  TabView* tab1 = CreateTab();
  TabView* tab2 = CreateTab();
  NSObject* contents =
      [tab_strip_ accessibilityAttributeValue:NSAccessibilityContentsAttribute];
  DCHECK([contents isKindOfClass:[NSArray class]]);
  NSArray* contentsArray = static_cast<NSArray*>(contents);
  ASSERT_TRUE([contentsArray containsObject:tab1]);
  ASSERT_TRUE([contentsArray containsObject:tab2]);
}

TEST_F(TabStripControllerTest, ViewAccessibility_Value) {
  NSArray* attrs = [tab_strip_ accessibilityAttributeNames];
  ASSERT_TRUE([attrs containsObject:NSAccessibilityValueAttribute]);

  // Create two tabs and ensure the active one gets returned.
  TabView* tab1 = CreateTab();
  TabView* tab2 = CreateTab();
  EXPECT_FALSE([tab1 controller].selected);
  EXPECT_TRUE([tab2 controller].selected);
  NSObject* value =
      [tab_strip_ accessibilityAttributeValue:NSAccessibilityValueAttribute];
  EXPECT_EQ(tab2, value);

  model_->ActivateTabAt(0, false);
  EXPECT_TRUE([tab1 controller].selected);
  EXPECT_FALSE([tab2 controller].selected);
  value =
      [tab_strip_ accessibilityAttributeValue:NSAccessibilityValueAttribute];
  EXPECT_EQ(tab1, value);
}

TEST_F(TabStripControllerTest, CorrectWindowFromUpdateWindowAlertState) {
  controller_.reset(InitTabStripControllerForAlertTesting());
  NSWindow* window = [tab_strip_ window];
  BrowserWindowController* window_controller =
      [BrowserWindowController browserWindowControllerForWindow:window];
  TabStripControllerForAlertTesting* tabStripControllerForTesting =
      static_cast<TabStripControllerForAlertTesting*>(controller_);

  TabView* const tab1 = CreateTab();
  TabView* const tab2 = CreateTab();

  // tab2 should be the selected one.
  EXPECT_FALSE([tab1 controller].selected);
  EXPECT_TRUE([tab2 controller].selected);
  WebContents* const contents_at_tab1 = model_->GetActiveWebContents();

  [tabStripControllerForTesting
      setAlertStateForContents:contents_at_tab1
                withAlertState:TabAlertState::AUDIO_PLAYING];
  // Make sure the overriden from base controller correctly handles alert
  // status of tabs.
  EXPECT_EQ(TabAlertState::AUDIO_PLAYING,
            [controller_ alertStateForContents:contents_at_tab1]);
  [controller_ updateWindowAlertState:TabAlertState::AUDIO_PLAYING
                       forWebContents:contents_at_tab1];
  // Because we have one tab playing, and the other one's alert state is none,
  // window alert state should be AUDIO_PLAYING.
  EXPECT_EQ(TabAlertState::AUDIO_PLAYING, [window_controller alertState]);

  model_->ActivateTabAt(0, false);
  // tab1 should be the selected one now.
  EXPECT_TRUE([tab1 controller].selected);
  EXPECT_FALSE([tab2 controller].selected);
  WebContents* const contents_at_tab0 = model_->GetActiveWebContents();

  [tabStripControllerForTesting
      setAlertStateForContents:contents_at_tab0
                withAlertState:TabAlertState::AUDIO_MUTING];
  [controller_ updateWindowAlertState:TabAlertState::AUDIO_MUTING
                       forWebContents:contents_at_tab0];
  // We have two tabs. One is playing and the other one is muting. The window
  // alert state should be still AUDIO_PLAYING.
  EXPECT_EQ(TabAlertState::AUDIO_PLAYING, [window_controller alertState]);

  [tabStripControllerForTesting
      setAlertStateForContents:contents_at_tab1
                withAlertState:TabAlertState::AUDIO_MUTING];
  [controller_ updateWindowAlertState:TabAlertState::AUDIO_MUTING
                       forWebContents:contents_at_tab1];
  // Now both tabs are muting, the window alert state should be AUDIO_MUTING.
  EXPECT_EQ(TabAlertState::AUDIO_MUTING, [window_controller alertState]);

  [tabStripControllerForTesting
      setAlertStateForContents:contents_at_tab0
                withAlertState:TabAlertState::AUDIO_PLAYING];
  [controller_ updateWindowAlertState:TabAlertState::AUDIO_PLAYING
                       forWebContents:contents_at_tab0];
  // Among those tabs which were muting, one is started playing, the window
  // alert state should be playing.
  EXPECT_EQ(TabAlertState::AUDIO_PLAYING, [window_controller alertState]);

  // Mute it again for further testing.
  [tabStripControllerForTesting
      setAlertStateForContents:contents_at_tab0
                withAlertState:TabAlertState::AUDIO_MUTING];
  [controller_ updateWindowAlertState:TabAlertState::AUDIO_MUTING
                       forWebContents:contents_at_tab0];

  [tabStripControllerForTesting setAlertStateForContents:contents_at_tab1
                                          withAlertState:TabAlertState::NONE];
  [controller_ updateWindowAlertState:TabAlertState::NONE
                       forWebContents:contents_at_tab1];
  // One of the tabs is muting, the other one is none. So window alert state
  // should be MUTING.
  EXPECT_EQ(TabAlertState::AUDIO_MUTING, [window_controller alertState]);

  [tabStripControllerForTesting setAlertStateForContents:contents_at_tab0
                                          withAlertState:TabAlertState::NONE];
  [controller_ updateWindowAlertState:TabAlertState::NONE
                       forWebContents:contents_at_tab0];
  // Neither of tabs playing nor muting, so the window alert state should be
  // NONE.
  EXPECT_EQ(TabAlertState::NONE, [window_controller alertState]);
}

}  // namespace
