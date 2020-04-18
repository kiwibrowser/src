// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/extensions/test_extension_system.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_unittest_helper.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_view_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button_cell.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/base/cocoa/animation_utils.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/events/test/cocoa_test_event_utils.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image_skia.h"

using base::ASCIIToUTF16;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

// Unit tests don't need time-consuming asynchronous animations.
@interface BookmarkBarControllerTestable : BookmarkBarController {
}

@end

@implementation BookmarkBarControllerTestable

- (id)initWithBrowser:(Browser*)browser
         initialWidth:(CGFloat)initialWidth
             delegate:(id<BookmarkBarControllerDelegate>)delegate {
  if ((self = [super initWithBrowser:browser
                        initialWidth:initialWidth
                            delegate:delegate])) {
    [self setStateAnimationsEnabled:NO];
    [self setInnerContentAnimationsEnabled:NO];
  }
  return self;
}

@end

// Just like a BookmarkBarController but openURL: is stubbed out.
@interface BookmarkBarControllerNoOpen : BookmarkBarControllerTestable {
 @public
  std::vector<GURL> urls_;
  std::vector<WindowOpenDisposition> dispositions_;
}
@end

@implementation BookmarkBarControllerNoOpen
- (void)openURL:(GURL)url disposition:(WindowOpenDisposition)disposition {
  urls_.push_back(url);
  dispositions_.push_back(disposition);
}
- (void)clear {
  urls_.clear();
  dispositions_.clear();
}
@end


// NSCell that is pre-provided with a desired size that becomes the
// return value for -(NSSize)cellSize:.
@interface CellWithDesiredSize : NSCell {
 @private
  NSSize cellSize_;
}
#if !defined(MAC_OS_X_VERSION_10_10) || \
    MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10
// In the OSX 10.10 SDK, cellSize became an atomic property, so there is no
// need to redeclare it.
@property (nonatomic, readonly) NSSize cellSize;
#endif  // MAC_OS_X_VERSION_10_10
@end

@implementation CellWithDesiredSize

@synthesize cellSize = cellSize_;

- (id)initTextCell:(NSString*)string desiredSize:(NSSize)size {
  if ((self = [super initTextCell:string])) {
    cellSize_ = size;
  }
  return self;
}

@end

// Remember the number of times we've gotten a frameDidChange notification.
@interface BookmarkBarControllerTogglePong : BookmarkBarControllerNoOpen {
 @private
  int toggles_;
}
@property (nonatomic, readonly) int toggles;
@end

@implementation BookmarkBarControllerTogglePong

@synthesize toggles = toggles_;

- (void)frameDidChange {
  toggles_++;
}

@end

// Remembers if a notification callback was called.
@interface BookmarkBarControllerNotificationPong : BookmarkBarControllerNoOpen {
  BOOL windowWillCloseReceived_;
  BOOL windowDidResignKeyReceived_;
}
@property (nonatomic, readonly) BOOL windowWillCloseReceived;
@property (nonatomic, readonly) BOOL windowDidResignKeyReceived;
@end

@implementation BookmarkBarControllerNotificationPong
@synthesize windowWillCloseReceived = windowWillCloseReceived_;
@synthesize windowDidResignKeyReceived = windowDidResignKeyReceived_;

// Override NSNotificationCenter callback.
- (void)parentWindowWillClose:(NSNotification*)notification {
  windowWillCloseReceived_ = YES;
}

// NSNotificationCenter callback.
- (void)parentWindowDidResignKey:(NSNotification*)notification {
  windowDidResignKeyReceived_ = YES;
}
@end

// Remembers if and what kind of openAll was performed.
@interface BookmarkBarControllerOpenAllPong : BookmarkBarControllerNoOpen {
  WindowOpenDisposition dispositionDetected_;
}
@property (nonatomic) WindowOpenDisposition dispositionDetected;
@end

@implementation BookmarkBarControllerOpenAllPong
@synthesize dispositionDetected = dispositionDetected_;

// Intercede for the openAll:disposition: method.
- (void)openAll:(const BookmarkNode*)node
    disposition:(WindowOpenDisposition)disposition {
  [self setDispositionDetected:disposition];
}

@end

// Just like a BookmarkBarController but intercedes when providing
// pasteboard drag data.
@interface BookmarkBarControllerDragData : BookmarkBarControllerTestable {
  const BookmarkNode* dragDataNode_;  // Weak
}
- (void)setDragDataNode:(const BookmarkNode*)node;
@end

@implementation BookmarkBarControllerDragData

- (id)initWithBrowser:(Browser*)browser
         initialWidth:(CGFloat)initialWidth
             delegate:(id<BookmarkBarControllerDelegate>)delegate {
  if ((self = [super initWithBrowser:browser
                        initialWidth:initialWidth
                            delegate:delegate])) {
    dragDataNode_ = NULL;
  }
  return self;
}

- (void)setDragDataNode:(const BookmarkNode*)node {
  dragDataNode_ = node;
}

- (std::vector<const BookmarkNode*>)retrieveBookmarkNodeData {
  std::vector<const BookmarkNode*> dragDataNodes;
  if(dragDataNode_) {
    dragDataNodes.push_back(dragDataNode_);
  }
  return dragDataNodes;
}

@end


class FakeTheme : public ui::ThemeProvider {
 public:
  FakeTheme(NSColor* color) : color_(color) {}
  base::scoped_nsobject<NSColor> color_;

  bool UsingSystemTheme() const override { return true; }
  gfx::ImageSkia* GetImageSkiaNamed(int id) const override { return NULL; }
  SkColor GetColor(int id) const override { return SkColor(); }
  color_utils::HSL GetTint(int id) const override { return color_utils::HSL(); }
  int GetDisplayProperty(int id) const override { return -1; }
  bool ShouldUseNativeFrame() const override { return false; }
  bool HasCustomImage(int id) const override { return false; }
  base::RefCountedMemory* GetRawData(int id, ui::ScaleFactor scale_factor)
      const override {
    return NULL;
  }
  bool InIncognitoMode() const override { return false; }
  bool HasCustomColor(int id) const override { return false; }
  NSImage* GetNSImageNamed(int id) const override { return nil; }
  NSColor* GetNSImageColorNamed(int id) const override { return nil; }
  NSColor* GetNSColor(int id) const override { return color_.get(); }
  NSColor* GetNSColorTint(int id) const override { return nil; }
  NSGradient* GetNSGradient(int id) const override { return nil; }
  bool ShouldIncreaseContrast() const override { return false; }
};


@interface FakeDragInfo : NSObject {
 @public
  NSPoint dropLocation_;
  NSDragOperation sourceMask_;
}
@property (nonatomic, assign) NSPoint dropLocation;
- (void)setDraggingSourceOperationMask:(NSDragOperation)mask;
@end

@implementation FakeDragInfo

@synthesize dropLocation = dropLocation_;

- (id)init {
  if ((self = [super init])) {
    dropLocation_ = NSZeroPoint;
    sourceMask_ = NSDragOperationMove;
  }
  return self;
}

// NSDraggingInfo protocol functions.

- (id)draggingPasteboard {
  return self;
}

- (id)draggingSource {
  return self;
}

- (NSDragOperation)draggingSourceOperationMask {
  return sourceMask_;
}

- (NSPoint)draggingLocation {
  return dropLocation_;
}

// Other functions.

- (void)setDraggingSourceOperationMask:(NSDragOperation)mask {
  sourceMask_ = mask;
}

@end


namespace {

class BookmarkBarControllerTestBase : public CocoaProfileTest {
 public:
  base::scoped_nsobject<NSView> parent_view_;
  base::scoped_nsobject<ViewResizerPong> resizeDelegate_;

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(profile());

    base::FilePath extension_dir;
    static_cast<extensions::TestExtensionSystem*>(
        extensions::ExtensionSystem::Get(profile()))
        ->CreateExtensionService(base::CommandLine::ForCurrentProcess(),
                                 extension_dir, false);
    resizeDelegate_.reset([[ViewResizerPong alloc] init]);
    NSRect parent_frame = NSMakeRect(0, 0, 800, 50);
    parent_view_.reset([[NSView alloc] initWithFrame:parent_frame]);
    [parent_view_ setHidden:YES];
  }

  void InstallAndToggleBar(BookmarkBarController* bar) {
    // Loads the view.
    [[bar controlledView] setResizeDelegate:resizeDelegate_];
    // Awkwardness to look like we've been installed.
    for (NSView* subView in [parent_view_ subviews])
      [subView removeFromSuperview];
    [parent_view_ addSubview:[bar view]];
    NSRect frame = [[[bar view] superview] frame];
    frame.origin.y = 100;
    [[[bar view] superview] setFrame:frame];

    // Make sure it's on in a window so viewDidMoveToWindow is called
    NSView* contentView = [test_window() contentView];
    if (![parent_view_ isDescendantOf:contentView])
      [contentView addSubview:parent_view_];

    // Make sure it's open so certain things aren't no-ops.
    [bar updateState:BookmarkBar::SHOW
          changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  }
};

class BookmarkBarControllerTest : public BookmarkBarControllerTestBase {
 public:
  base::scoped_nsobject<BookmarkBarControllerNoOpen> bar_;

  void SetUp() override {
    BookmarkBarControllerTestBase::SetUp();
    ASSERT_TRUE(browser());
    AddCommandLineSwitches();

    bar_.reset([[BookmarkBarControllerNoOpen alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth([parent_view_ frame])
               delegate:nil]);
    InstallAndToggleBar(bar_.get());

    // AppKit methods are not guaranteed to complete synchronously. Some of them
    // have asynchronous side effects, such as invoking -[NSViewController
    // viewDidAppear]. Spinning the run loop until it's idle ensures that there
    // are no outstanding references to bar_, and that calling bar_.reset() will
    // synchronously destroy bar_.
    base::RunLoop().RunUntilIdle();
  }

  virtual void AddCommandLineSwitches() {}

  BookmarkBarControllerNoOpen* noOpenBar() {
    return (BookmarkBarControllerNoOpen*)bar_.get();
  }

  // Verifies that the i-th button has the title of the i-th child node of
  // |node|.
  void VerifyButtonTitles(const BookmarkNode* node) {
    ASSERT_LE((int)[[bar_ buttons] count], node->child_count());
    int numButtons = [[bar_ buttons] count];
    for (int i = 0; i < numButtons; i++) {
      EXPECT_NSEQ([[[bar_ buttons] objectAtIndex:i] title],
                  base::SysUTF16ToNSString(node->GetChild(i)->GetTitle()));
    }
  }

  // Verifies that |view|
  // * With a blank layout applied:
  //  - Is hidden
  //
  // * With |layout| applied:
  //  - Is visible
  //  - Has x origin |expected_x_origin|
  //  If |check_exact_width| is true:
  //   - has width |expected_width|
  //  Otherwise:
  //   - has width > 0 (|expected_width| is ignored in this case)
  void VerifyViewLayout(NSView* view,
                        bookmarks::BookmarkBarLayout& layout,
                        CGFloat expected_x_origin,
                        CGFloat expected_width,
                        bool check_exact_width) {
    ASSERT_TRUE(view);

    bookmarks::BookmarkBarLayout empty_layout = {};
    [bar_ applyLayout:empty_layout animated:NO];
    EXPECT_TRUE([view isHidden]);

    [bar_ applyLayout:layout animated:NO];
    EXPECT_CGFLOAT_EQ(NSMinX([view frame]), expected_x_origin);
    CGFloat actual_width = NSWidth([view frame]);
    if (check_exact_width) {
      EXPECT_CGFLOAT_EQ(actual_width, expected_width);
    } else {
      EXPECT_GT(actual_width, 0);
    }
  }
};

TEST_F(BookmarkBarControllerTest, ShowWhenShowBookmarkBarTrue) {
  [bar_ updateState:BookmarkBar::SHOW
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_TRUE([bar_ isInState:BookmarkBar::SHOW]);
  EXPECT_FALSE([bar_ isInState:BookmarkBar::DETACHED]);
  EXPECT_TRUE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);
  EXPECT_FALSE([[bar_ view] isHidden]);
  EXPECT_GT([resizeDelegate_ height], 0);
  EXPECT_GT([[bar_ view] frame].size.height, 0);
}

TEST_F(BookmarkBarControllerTest, HideWhenShowBookmarkBarFalse) {
  [bar_ updateState:BookmarkBar::HIDDEN
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_FALSE([bar_ isInState:BookmarkBar::SHOW]);
  EXPECT_FALSE([bar_ isInState:BookmarkBar::DETACHED]);
  EXPECT_FALSE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);
  EXPECT_TRUE([[bar_ view] isHidden]);
  EXPECT_EQ(0, [resizeDelegate_ height]);
  EXPECT_EQ(0, [[bar_ view] frame].size.height);
}

TEST_F(BookmarkBarControllerTest, HideWhenShowBookmarkBarTrueButDisabled) {
  [bar_ setBookmarkBarEnabled:NO];
  [bar_ updateState:BookmarkBar::SHOW
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_TRUE([bar_ isInState:BookmarkBar::SHOW]);
  EXPECT_FALSE([bar_ isInState:BookmarkBar::DETACHED]);
  EXPECT_FALSE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);
  EXPECT_TRUE([[bar_ view] isHidden]);
  EXPECT_EQ(0, [resizeDelegate_ height]);
  EXPECT_EQ(0, [[bar_ view] frame].size.height);
}

TEST_F(BookmarkBarControllerTest, ShowOnNewTabPage) {
  [bar_ updateState:BookmarkBar::DETACHED
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_FALSE([bar_ isInState:BookmarkBar::SHOW]);
  EXPECT_TRUE([bar_ isInState:BookmarkBar::DETACHED]);
  EXPECT_TRUE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);
  EXPECT_FALSE([[bar_ view] isHidden]);
  EXPECT_GT([resizeDelegate_ height], 0);
  EXPECT_GT([[bar_ view] frame].size.height, 0);
}

// Test whether |-updateState:...| sets currentState as expected. Make
// sure things don't crash.
TEST_F(BookmarkBarControllerTest, StateChanges) {
  // First, go in one-at-a-time cycle.
  [bar_ updateState:BookmarkBar::HIDDEN
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_EQ(BookmarkBar::HIDDEN, [bar_ currentState]);
  EXPECT_FALSE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);

  [bar_ updateState:BookmarkBar::SHOW
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_EQ(BookmarkBar::SHOW, [bar_ currentState]);
  EXPECT_TRUE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);

  [bar_ updateState:BookmarkBar::DETACHED
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_EQ(BookmarkBar::DETACHED, [bar_ currentState]);
  EXPECT_TRUE([bar_ isVisible]);
  EXPECT_FALSE([bar_ isAnimationRunning]);

  // Now try some "jumps".
  for (int i = 0; i < 2; i++) {
  [bar_ updateState:BookmarkBar::HIDDEN
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
    EXPECT_EQ(BookmarkBar::HIDDEN, [bar_ currentState]);
    EXPECT_FALSE([bar_ isVisible]);
    EXPECT_FALSE([bar_ isAnimationRunning]);

    [bar_ updateState:BookmarkBar::SHOW
           changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
    EXPECT_EQ(BookmarkBar::SHOW, [bar_ currentState]);
    EXPECT_TRUE([bar_ isVisible]);
    EXPECT_FALSE([bar_ isAnimationRunning]);
  }

  // Now try some "jumps".
  for (int i = 0; i < 2; i++) {
    [bar_ updateState:BookmarkBar::SHOW
           changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
    EXPECT_EQ(BookmarkBar::SHOW, [bar_ currentState]);
    EXPECT_TRUE([bar_ isVisible]);
    EXPECT_FALSE([bar_ isAnimationRunning]);

    [bar_ updateState:BookmarkBar::DETACHED
           changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
    EXPECT_EQ(BookmarkBar::DETACHED, [bar_ currentState]);
    EXPECT_TRUE([bar_ isVisible]);
    EXPECT_FALSE([bar_ isAnimationRunning]);
  }
}

// Make sure we're watching for frame change notifications.
TEST_F(BookmarkBarControllerTest, FrameChangeNotification) {
  base::scoped_nsobject<BookmarkBarControllerTogglePong> bar;
  bar.reset([[BookmarkBarControllerTogglePong alloc]
      initWithBrowser:browser()
         initialWidth:100  // arbitrary
             delegate:nil]);
  InstallAndToggleBar(bar.get());

  // Send a frame did change notification for the pong's view.
  [[NSNotificationCenter defaultCenter]
    postNotificationName:NSViewFrameDidChangeNotification
                  object:[bar view]];

  EXPECT_GT([bar toggles], 0);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutNoItemTextField) {
  NSTextField* text_field = [[bar_ buttonView] noItemTextField];

  bookmarks::BookmarkBarLayout layout = {};
  layout.visible_elements |= bookmarks::kVisibleElementsMaskNoItemTextField;
  layout.no_item_textfield_offset = 10;
  layout.no_item_textfield_width = 20;

  VerifyViewLayout(text_field, layout, 10, 20, true);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutImportBookmarksButton) {
  NSButton* button = [[bar_ buttonView] importBookmarksButton];

  bookmarks::BookmarkBarLayout layout = {};
  // This button never appears without the no item text field appearing, so
  // show that too.
  layout.visible_elements |= bookmarks::kVisibleElementsMaskNoItemTextField;
  layout.no_item_textfield_offset = 10;
  layout.no_item_textfield_width = 20;
  layout.visible_elements |=
      bookmarks::kVisibleElementsMaskImportBookmarksButton;
  layout.import_bookmarks_button_offset = 30;
  layout.import_bookmarks_button_width = 40;

  VerifyViewLayout(button, layout, 30, 40, true);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutManagedButton) {
  NSButton* button = [bar_ managedBookmarksButton];

  bookmarks::BookmarkBarLayout layout = {};
  layout.visible_elements |=
      bookmarks::kVisibleElementsMaskManagedBookmarksButton;
  layout.managed_bookmarks_button_offset = 50;

  VerifyViewLayout(button, layout, 50, CGFLOAT_MAX, false);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutOffTheSideButton) {
  NSButton* button = [bar_ offTheSideButton];

  bookmarks::BookmarkBarLayout layout = {};
  layout.visible_elements |= bookmarks::kVisibleElementsMaskOffTheSideButton;
  layout.off_the_side_button_offset = 100;

  VerifyViewLayout(button, layout, 100, CGFLOAT_MAX, false);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutOtherBookmarksButton) {
  NSButton* button = [bar_ otherBookmarksButton];

  bookmarks::BookmarkBarLayout layout = {};
  layout.visible_elements |=
      bookmarks::kVisibleElementsMaskOtherBookmarksButton;
  layout.other_bookmarks_button_offset = 110;

  VerifyViewLayout(button, layout, 110, CGFLOAT_MAX, false);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutAppsButton) {
  NSButton* button = [bar_ appsPageShortcutButton];

  bookmarks::BookmarkBarLayout layout = {};
  layout.visible_elements |= bookmarks::kVisibleElementsMaskAppsButton;
  layout.apps_button_offset = 5;

  VerifyViewLayout(button, layout, 5, CGFLOAT_MAX, false);
}

TEST_F(BookmarkBarControllerTest, ApplyLayoutBookmarkButtons) {
  // Add some bookmarks
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  GURL gurls[] = {
      GURL("http://www.google.com/a"), GURL("http://www.google.com/b"),
      GURL("http://www.google.com/c"), GURL("http://www.google.com/d")};

  base::string16 titles[] = {ASCIIToUTF16("a"), ASCIIToUTF16("b"),
                             ASCIIToUTF16("c"), ASCIIToUTF16("d")};
  for (size_t i = 0; i < arraysize(titles); i++)
    bookmarks::AddIfNotBookmarked(model, gurls[i], titles[i]);

  // Apply an empty layout. This should clear the buttons.
  bookmarks::BookmarkBarLayout layout = {};
  [bar_ applyLayout:layout animated:NO];
  EXPECT_EQ([[bar_ buttons] count], 0U);

  const BookmarkNode* parentNode = model->bookmark_bar_node();
  EXPECT_EQ(parentNode->child_count(), 4);
  // Ask the layout to show the first 2 nodes
  for (int i = 0; i < 3; i++) {
    const BookmarkNode* node = parentNode->GetChild(i);
    layout.button_offsets[node->id()] =
        bookmarks::kDefaultBookmarkWidth * (i + 1);
  }
  [bar_ applyLayout:layout animated:NO];
  EXPECT_EQ([[bar_ buttons] count], 3U);
  CGFloat lastMax = 0;
  for (int i = 0; i < 3; i++) {
    const BookmarkNode* node = parentNode->GetChild(i);
    BookmarkButton* button = [[bar_ buttons] objectAtIndex:i];
    EXPECT_EQ([button bookmarkNode], node);
    EXPECT_CGFLOAT_EQ(NSMinX([button frame]),
                      bookmarks::kDefaultBookmarkWidth * (i + 1));
    EXPECT_GT(NSWidth([button frame]), 0);
    EXPECT_EQ([button superview], [bar_ buttonView]);
    // Ensure buttons don't overlap.
    EXPECT_GT(NSMinX([button frame]), lastMax);
    lastMax = NSMaxX([button frame]);
  }
  VerifyButtonTitles(parentNode);
}
// TODO(lgrey): Add tests for RTL in a follow-up.
// crbug.com/648554

TEST_F(BookmarkBarControllerTest, LayoutNoBookmarks) {
  // With no apps button or managed button:
  profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar, false);
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  // Only the no item text field and import bookmarks button are showing.
  EXPECT_EQ(
      layout.visible_elements,
      (unsigned int)(bookmarks::kVisibleElementsMaskImportBookmarksButton |
                     bookmarks::kVisibleElementsMaskNoItemTextField));

  EXPECT_EQ(layout.VisibleButtonCount(), 0U);

  // The import bookmarks button is after the no item text field and they don't
  // overlap
  EXPECT_GT(layout.import_bookmarks_button_offset,
            layout.no_item_textfield_offset + layout.no_item_textfield_width);
  // And they both have a width
  EXPECT_GT(layout.no_item_textfield_width, 0);
  EXPECT_GT(layout.import_bookmarks_button_width, 0);
  // And start at the correct offset
  EXPECT_EQ(layout.no_item_textfield_offset,
            bookmarks::kBookmarkLeftMargin +
                bookmarks::kInitialNoItemTextFieldXOrigin);
  // With apps button:
  profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar, true);
  layout = [bar_ layoutFromCurrentState];

  // Only the apps button, no item text field and import bookmarks button are
  // showing.
  EXPECT_EQ(
      layout.visible_elements,
      (unsigned int)(bookmarks::kVisibleElementsMaskImportBookmarksButton |
                     bookmarks::kVisibleElementsMaskNoItemTextField |
                     bookmarks::kVisibleElementsMaskAppsButton));
  EXPECT_EQ(layout.VisibleButtonCount(), 0U);
  // The no item text field is after the apps button
  EXPECT_GT(layout.no_item_textfield_offset, layout.apps_button_offset);

  // And the apps button is at the correct offset
  EXPECT_EQ(layout.apps_button_offset, bookmarks::kBookmarkLeftMargin);
}

TEST_F(BookmarkBarControllerTest, LayoutManagedBookmarksButton) {
  PrefService* prefs = profile()->GetPrefs();
  // Doesn't show by default.
  bookmarks::ManagedBookmarkService* managedBookmarkService =
      ManagedBookmarkServiceFactory::GetForProfile(profile());
  EXPECT_TRUE(managedBookmarkService->managed_node()->empty());

  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsManagedBookmarksButtonVisible());

  // Add a managed bookmark.
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString("name", "Google");
  dict->SetString("url", "http://google.com");
  base::ListValue list;
  list.Append(std::move(dict));
  prefs->Set(bookmarks::prefs::kManagedBookmarks, list);
  EXPECT_FALSE(managedBookmarkService->managed_node()->empty());

  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsManagedBookmarksButtonVisible());

  // Toggle the button off via pref.
  prefs->SetBoolean(bookmarks::prefs::kShowManagedBookmarksInBookmarkBar,
                    false);

  layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsManagedBookmarksButtonVisible());

  // Toggle it back.
  prefs->SetBoolean(bookmarks::prefs::kShowManagedBookmarksInBookmarkBar, true);

  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsManagedBookmarksButtonVisible());
}

TEST_F(BookmarkBarControllerTest, LayoutManagedAppsButton) {
  // By default the pref is not managed and the apps shortcut is shown.
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile()->GetTestingPrefService();
  EXPECT_FALSE(prefs->IsManagedPreference(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar));
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsAppsButtonVisible());

  // Hide the apps shortcut by policy, via the managed pref.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        std::make_unique<base::Value>(false));
  layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsAppsButtonVisible());

  // And try showing it via policy too.
  prefs->SetManagedPref(bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
                        std::make_unique<base::Value>(true));
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsAppsButtonVisible());
}

// This tests a formerly-pathological case where if there was a single
// bookmark on the bar and it was renamed, the button wouldn't update
// since the offset and number of buttons stayed the same.
// See crbug.com/724201
TEST_F(BookmarkBarControllerTest, RenameBookmark) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* barNode = model->bookmark_bar_node();
  const BookmarkNode* node = model->AddURL(barNode, 0, ASCIIToUTF16("Lorem"),
                                           GURL("http://example.com"));
  EXPECT_EQ([[bar_ buttons] count], 1U);
  EXPECT_NSEQ([[[bar_ buttons] firstObject] title], @"Lorem");

  model->SetTitle(node, ASCIIToUTF16("Ipsum"));
  [bar_ rebuildLayoutWithAnimated:NO];
  EXPECT_EQ([[bar_ buttons] count], 1U);
  EXPECT_NSEQ([[[bar_ buttons] firstObject] title], @"Ipsum");
}

TEST_F(BookmarkBarControllerTest, NoItemsResizing) {
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible() &&
              layout.IsImportBookmarksButtonVisible());
  CGFloat oldOffset = layout.import_bookmarks_button_offset;
  CGFloat oldWidth = layout.import_bookmarks_button_width;
  CGRect viewFrame = [[bar_ view] frame];
  CGFloat originalViewWidth = NSWidth(viewFrame);
  // Assert that the import bookmarks button fits.
  EXPECT_GT(originalViewWidth, oldOffset + oldWidth);
  // Resize the view to cut the import bookmarks button in half.
  viewFrame.size.width = oldOffset + oldWidth * 0.5;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible() &&
              layout.IsImportBookmarksButtonVisible());
  EXPECT_CGFLOAT_EQ(layout.import_bookmarks_button_offset, oldOffset);
  EXPECT_LT(layout.import_bookmarks_button_width, oldWidth);
  // Resize the view to cut off the import bookmarks button entirely.
  viewFrame.size.width = layout.import_bookmarks_button_offset - 1;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsImportBookmarksButtonVisible());
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible());
  // Now, cut the text field in half
  oldOffset = layout.no_item_textfield_offset;
  oldWidth = layout.no_item_textfield_width;
  viewFrame.size.width = oldOffset + oldWidth * 0.5;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible());
  EXPECT_CGFLOAT_EQ(layout.no_item_textfield_offset, oldOffset);
  EXPECT_LT(layout.no_item_textfield_width, oldWidth);
  // Make the view too small to fit either.
  viewFrame.size.width = bookmarks::kBookmarkLeftMargin;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsNoItemTextFieldVisible() ||
               layout.IsImportBookmarksButtonVisible());
  // Sanity check
  viewFrame.size.width = originalViewWidth;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible() &&
              layout.IsImportBookmarksButtonVisible());
}

TEST_F(BookmarkBarControllerTest, LayoutOtherBookmarks) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* otherBookmarks = model->other_node();
  model->AddURL(otherBookmarks, otherBookmarks->child_count(),
                ASCIIToUTF16("TheOther"), GURL("http://www.other.com"));
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsOtherBookmarksButtonVisible());
  EXPECT_EQ(layout.VisibleButtonCount(), 0U);
  EXPECT_GT(layout.other_bookmarks_button_offset, 0);
  CGFloat offsetFromMaxX =
      NSWidth([[bar_ view] frame]) - layout.other_bookmarks_button_offset;
  // Resize the view and ensure the button stays pinned to the right edge.
  CGRect viewFrame = [[bar_ view] frame];
  // Half of the original size
  viewFrame.size.width = std::ceil(NSWidth(viewFrame) * 0.5);
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsOtherBookmarksButtonVisible());
  EXPECT_GT(layout.other_bookmarks_button_offset, 0);
  EXPECT_CGFLOAT_EQ(NSWidth(viewFrame) - layout.other_bookmarks_button_offset,
                    offsetFromMaxX);
  // 150% of the original size
  viewFrame.size.width = NSWidth(viewFrame) * 3;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsOtherBookmarksButtonVisible());
  EXPECT_CGFLOAT_EQ(NSWidth(viewFrame) - layout.other_bookmarks_button_offset,
                    offsetFromMaxX);
}

TEST_F(BookmarkBarControllerTest, LayoutBookmarks) {
  // Apps button shows up by default, first test without it.
  profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar, false);

  // Add some bookmarks.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* barNode = model->bookmark_bar_node();
  GURL gurls[] = {GURL("http://www.google.com/a"),
                  GURL("http://www.google.com/b"),
                  GURL("http://www.google.com/c")};
  base::string16 titles[] = {ASCIIToUTF16("a"), ASCIIToUTF16("b"),
                             ASCIIToUTF16("c")};
  for (size_t i = 0; i < arraysize(titles); i++)
    bookmarks::AddIfNotBookmarked(model, gurls[i], titles[i]);

  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_EQ(layout.VisibleButtonCount(), (size_t)barNode->child_count());
  EXPECT_FALSE(layout.IsOffTheSideButtonVisible());
  EXPECT_EQ(layout.button_offsets.size(), (size_t)barNode->child_count());
  EXPECT_EQ(layout.button_offsets[barNode->GetChild(0)->id()],
            bookmarks::kBookmarkLeftMargin);
  // Assert that the buttons are in order.
  CGFloat lastOffset = 0;
  for (int i = 0; i < barNode->child_count(); i++) {
    CGFloat offset = layout.button_offsets[barNode->GetChild(i)->id()];
    EXPECT_GT(offset, lastOffset);
    lastOffset = offset;
  }
}

TEST_F(BookmarkBarControllerTest, NoItemUIHiddenWithBookmarks) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* barNode = model->bookmark_bar_node();

  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible() &&
              layout.IsImportBookmarksButtonVisible());

  const BookmarkNode* node =
      model->AddURL(barNode, barNode->child_count(), ASCIIToUTF16("title"),
                    GURL("http://www.google.com"));
  layout = [bar_ layoutFromCurrentState];
  EXPECT_FALSE(layout.IsNoItemTextFieldVisible() ||
               layout.IsImportBookmarksButtonVisible());

  model->Remove(node);
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsNoItemTextFieldVisible() &&
              layout.IsImportBookmarksButtonVisible());
}

TEST_F(BookmarkBarControllerTest, LayoutBookmarksResize) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* barNode = model->bookmark_bar_node();
  for (int i = 0; i < 20; i++) {
    model->AddURL(barNode, barNode->child_count(), ASCIIToUTF16("title"),
                  GURL("http://www.google.com"));
  }
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  EXPECT_LT(layout.VisibleButtonCount(), 20U);
  size_t originalButtonCount = layout.VisibleButtonCount();
  EXPECT_TRUE(layout.IsOffTheSideButtonVisible());

  CGRect viewFrame = [[bar_ view] frame];
  // Increase the width of the view. More buttons should be visible.
  viewFrame.size.width += 200;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_GT(layout.VisibleButtonCount(), originalButtonCount);

  // Decrease the width of the view to below what it was originally.
  // Less buttons should be visible.
  viewFrame.size.width -= 400;
  [[bar_ view] setFrame:viewFrame];
  layout = [bar_ layoutFromCurrentState];
  EXPECT_LT(layout.VisibleButtonCount(), originalButtonCount);

  // NB: This part overlaps a little with |OffTheSideButtonHidden|
  // but we just want to check the layout here, whereas that test
  // ensures the folder is closed when the button goes away.
  //
  // Find the number of buttons at which the off-the-side button disappears.
  EXPECT_TRUE(layout.IsOffTheSideButtonVisible());
  bool crossoverPointFound = false;

  // Bound the number of iterations in case the button never
  // disappears.
  const int maxToRemove = 20;
  for (int i = 0; i < maxToRemove; i++) {
    if (!layout.IsOffTheSideButtonVisible()) {
      crossoverPointFound = true;
      break;
    }
    model->Remove(barNode->GetChild(barNode->child_count() - 1));
    layout = [bar_ layoutFromCurrentState];
  }

  // Either something is broken, or the test needs to be redone.
  ASSERT_TRUE(crossoverPointFound);
  EXPECT_FALSE(layout.IsOffTheSideButtonVisible());

  // Add a button and ensure the off the side button appears again.
  model->AddURL(barNode, barNode->child_count(), ASCIIToUTF16("title"),
                GURL("http://www.google.com"));
  layout = [bar_ layoutFromCurrentState];
  EXPECT_TRUE(layout.IsOffTheSideButtonVisible());

  // If the off-the-side button is visible, this means we have more
  // buttons than we can show, so adding another shouldn't increase
  // the button count.
  size_t buttonCountBeforeAdding = layout.VisibleButtonCount();
  model->AddURL(barNode, barNode->child_count(), ASCIIToUTF16("title"),
                GURL("http://www.google.com"));
  layout = [bar_ layoutFromCurrentState];
  EXPECT_EQ(layout.VisibleButtonCount(), buttonCountBeforeAdding);
}

// Confirm off the side button only enabled when reasonable.
TEST_F(BookmarkBarControllerTest, OffTheSideButtonHidden) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  [bar_ loaded:model];
  EXPECT_TRUE([[bar_ offTheSideButton] isHidden]);

  for (int i = 0; i < 2; i++) {
    bookmarks::AddIfNotBookmarked(
        model, GURL("http://www.foo.com"), ASCIIToUTF16("small"));
    EXPECT_TRUE([[bar_ offTheSideButton] isHidden]);
  }

  const BookmarkNode* parent = model->bookmark_bar_node();
  for (int i = 0; i < 20; i++) {
    model->AddURL(parent, parent->child_count(),
                  ASCIIToUTF16("super duper wide title"),
                  GURL("http://superfriends.hall-of-justice.edu"));
  }
  EXPECT_FALSE([[bar_ offTheSideButton] isHidden]);

  // Open the "off the side" and start deleting nodes.  Make sure
  // deletion of the last node in "off the side" causes the folder to
  // close.
  EXPECT_FALSE([[bar_ offTheSideButton] isHidden]);
  NSButton* offTheSideButton = [bar_ offTheSideButton];
  // Open "off the side" menu.
  [bar_ openOffTheSideFolderFromButton:offTheSideButton];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);
  [bbfc setIgnoreAnimations:YES];
  while (!parent->empty()) {
    // We've completed the job so we're done.
    if ([[bar_ offTheSideButton] isHidden])
      break;
    // Delete the last button.
    model->Remove(parent->GetChild(parent->child_count() - 1));
    // If last one make sure the menu is closed and the button is hidden.
    // Else make sure menu stays open.
    if ([[bar_ offTheSideButton] isHidden]) {
      EXPECT_FALSE([bar_ folderController]);
    } else {
      EXPECT_TRUE([bar_ folderController]);
    }
  }
}

// Tests that buttons in the reuse pool have their node pointer
// nulled out.
TEST_F(BookmarkBarControllerTest, ReusePoolNulled) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* barNode = model->bookmark_bar_node();
  for (int i = 0; i < 20; i++) {
    model->AddURL(barNode, barNode->child_count(), ASCIIToUTF16("title"),
                  GURL("http://www.google.com"));
  }
  NSMutableSet* oldButtons = [NSMutableSet setWithArray:[bar_ buttons]];
  // Resize the view so some buttons get removed and added to the
  // reuse pool.
  CGRect viewFrame = [[bar_ view] frame];
  viewFrame.size.width -= 200;
  [[bar_ view] setFrame:viewFrame];

  NSSet* newButtons = [NSSet setWithArray:[bar_ buttons]];
  [oldButtons minusSet:newButtons];

  // Make sure this test is actually testing something.
  ASSERT_GT([oldButtons count], 0U);

  for (BookmarkButton* button in oldButtons) {
    EXPECT_TRUE([button isHidden]);
    EXPECT_EQ([button bookmarkNode], nullptr);
  }
}

// http://crbug.com/46175 is a crash when deleting bookmarks from the
// off-the-side menu while it is open.  This test tries to bang hard
// in this area to reproduce the crash.
TEST_F(BookmarkBarControllerTest, DeleteFromOffTheSideWhileItIsOpen) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  [bar_ loaded:model];

  // Add a lot of bookmarks (per the bug).
  const BookmarkNode* parent = model->bookmark_bar_node();
  for (int i = 0; i < 100; i++) {
    std::ostringstream title;
    title << "super duper wide title " << i;
    model->AddURL(parent, parent->child_count(), ASCIIToUTF16(title.str()),
                  GURL("http://superfriends.hall-of-justice.edu"));
  }
  EXPECT_FALSE([[bar_ offTheSideButton] isHidden]);

  // Open "off the side" menu.
  NSButton* offTheSideButton = [bar_ offTheSideButton];
  [bar_ openOffTheSideFolderFromButton:offTheSideButton];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);
  [bbfc setIgnoreAnimations:YES];

  // Start deleting items; try and delete randomish ones in case it
  // makes a difference.
  int indices[] = { 2, 4, 5, 1, 7, 9, 2, 0, 10, 9 };
  while (!parent->empty()) {
    for (unsigned int i = 0; i < arraysize(indices); i++) {
      if (indices[i] < parent->child_count()) {
        // First we mouse-enter the button to make things harder.
        NSArray* buttons = [bbfc buttons];
        for (BookmarkButton* button in buttons) {
          if ([button bookmarkNode] == parent->GetChild(indices[i])) {
            [bbfc mouseEnteredButton:button event:nil];
            break;
          }
        }
        // Then we remove the node.  This triggers the button to get
        // deleted.
        model->Remove(parent->GetChild(indices[i]));
        // Force visual update which is otherwise delayed.
        [[bbfc window] displayIfNeeded];
      }
    }
  }
}

// Test whether |-dragShouldLockBarVisibility| returns NO iff the bar is
// detached.
TEST_F(BookmarkBarControllerTest, TestDragShouldLockBarVisibility) {
  [bar_ updateState:BookmarkBar::HIDDEN
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_TRUE([bar_ dragShouldLockBarVisibility]);

  [bar_ updateState:BookmarkBar::SHOW
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_TRUE([bar_ dragShouldLockBarVisibility]);

  [bar_ updateState:BookmarkBar::DETACHED
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_FALSE([bar_ dragShouldLockBarVisibility]);
}

// Confirm openBookmark: forwards the request to the controller's delegate
TEST_F(BookmarkBarControllerTest, OpenBookmark) {
  GURL gurl("http://walla.walla.ding.dong.com");
  std::unique_ptr<BookmarkNode> node(new BookmarkNode(gurl));

  base::scoped_nsobject<BookmarkButtonCell> cell(
      [[BookmarkButtonCell alloc] init]);
  [cell setBookmarkNode:node.get()];
  base::scoped_nsobject<BookmarkButton> button([[BookmarkButton alloc] init]);
  [button setCell:cell.get()];
  [cell setRepresentedObject:[NSValue valueWithPointer:node.get()]];

  [bar_ openBookmark:button];
  EXPECT_EQ(noOpenBar()->urls_[0], node->url());
  EXPECT_EQ(noOpenBar()->dispositions_[0], WindowOpenDisposition::CURRENT_TAB);
}

TEST_F(BookmarkBarControllerTest, TestAddRemoveAndClear) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  EXPECT_EQ(0U, [[bar_ buttons] count]);

  GURL gurl1("http://superfriends.hall-of-justice.edu");
  // Short titles increase the chances of this test succeeding if the view is
  // narrow.
  // TODO(viettrungluu): make the test independent of window/view size, font
  // metrics, button size and spacing, and everything else.
  base::string16 title1(ASCIIToUTF16("x"));
  bookmarks::AddIfNotBookmarked(model, gurl1, title1);
  EXPECT_EQ(1U, [[bar_ buttons] count]);

  GURL gurl2("http://legion-of-doom.gov");
  base::string16 title2(ASCIIToUTF16("y"));
  bookmarks::AddIfNotBookmarked(model, gurl2, title2);
  EXPECT_EQ(2U, [[bar_ buttons] count]);

  for (int i = 0; i < 3; i++) {
    bookmarks::RemoveAllBookmarks(model, gurl2);
    EXPECT_EQ(1U, [[bar_ buttons] count]);

    // and bring it back
    bookmarks::AddIfNotBookmarked(model, gurl2, title2);
    EXPECT_EQ(2U, [[bar_ buttons] count]);
  }

  // Explicit test of loaded: since this is a convenient spot
  [bar_ loaded:model];
  EXPECT_EQ(2U, [[bar_ buttons] count]);
}

// Make sure we don't create too many buttons; we only really need
// ones that will be visible.
TEST_F(BookmarkBarControllerTest, TestButtonLimits) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  EXPECT_EQ(0U, [[bar_ buttons] count]);
  // Add one; make sure we see it.
  const BookmarkNode* parent = model->bookmark_bar_node();
  model->AddURL(parent, parent->child_count(),
                ASCIIToUTF16("title"), GURL("http://www.google.com"));
  EXPECT_EQ(1U, [[bar_ buttons] count]);

  // Add 30 which we expect to be 'too many'.  Make sure we don't see
  // 30 buttons.
  model->Remove(parent->GetChild(0));
  EXPECT_EQ(0U, [[bar_ buttons] count]);
  for (int i=0; i<30; i++) {
    model->AddURL(parent, parent->child_count(),
                  ASCIIToUTF16("title"), GURL("http://www.google.com"));
  }
  int count = [[bar_ buttons] count];
  EXPECT_LT(count, 30L);

  // Add 10 more (to the front of the list so the on-screen buttons
  // would change) and make sure the count stays the same.
  for (int i=0; i<10; i++) {
    model->AddURL(parent, 0,  // index is 0, so front, not end
                  ASCIIToUTF16("title"), GURL("http://www.google.com"));
  }

  // Finally, grow the view and make sure the button count goes up.
  NSRect frame = [[bar_ view] frame];
  frame.size.width += 600;
  [[bar_ view] setFrame:frame];
  int finalcount = [[bar_ buttons] count];
  EXPECT_GT(finalcount, count);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(BookmarkBarControllerTest, Display) {
  [[bar_ view] display];
}

// Test that middle clicking on a bookmark button results in an open action,
// except for offTheSideButton, as it just opens its folder menu.
TEST_F(BookmarkBarControllerTest, MiddleClick) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  GURL gurl1("http://www.google.com/");
  base::string16 title1(ASCIIToUTF16("x"));
  bookmarks::AddIfNotBookmarked(model, gurl1, title1);

  EXPECT_EQ(1U, [[bar_ buttons] count]);
  NSButton* first = [[bar_ buttons] objectAtIndex:0];
  EXPECT_TRUE(first);

  [first otherMouseUp:
      cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp, 0)];
  EXPECT_EQ(noOpenBar()->urls_.size(), 1U);

  // Test for offTheSideButton.
  // Add more bookmarks so that offTheSideButton is visible.
  const BookmarkNode* parent = model->bookmark_bar_node();
  for (int i = 0; i < 20; i++) {
    model->AddURL(parent, parent->child_count(),
                  ASCIIToUTF16("super duper wide title"),
                  GURL("http://superfriends.hall-of-justice.edu"));
  }

  NSButton* offTheSideButton = [bar_ offTheSideButton];
  EXPECT_TRUE(offTheSideButton);
  EXPECT_FALSE([offTheSideButton isHidden]);
  [offTheSideButton otherMouseUp:
      cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp, 0)];

  // Middle click on offTheSideButton should not open any bookmarks under it,
  // therefore urls size should still be 1.
  EXPECT_EQ(noOpenBar()->urls_.size(), 1U);

  // Check that folderController should not be NULL since offTheSideButton
  // folder is currently open.
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);
  EXPECT_TRUE([bbfc parentButton] == offTheSideButton);

  // Middle clicking again on it should close the folder.
  [offTheSideButton otherMouseUp:
      cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp, 0)];
  bbfc = [bar_ folderController];
  EXPECT_FALSE(bbfc);
}

TEST_F(BookmarkBarControllerTest, DisplaysHelpMessageOnEmpty) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  [bar_ loaded:model];
  EXPECT_FALSE([[[bar_ buttonView] noItemTextField] isHidden]);
}

TEST_F(BookmarkBarControllerTest, DisplaysImportBookmarksButtonOnEmpty) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  [bar_ loaded:model];
  EXPECT_FALSE([[[bar_ buttonView] importBookmarksButton] isHidden]);
}

TEST_F(BookmarkBarControllerTest, BookmarkButtonSizing) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  const BookmarkNode* parent = model->bookmark_bar_node();
  model->AddURL(parent, parent->child_count(),
                ASCIIToUTF16("title"), GURL("http://one.com"));

  [bar_ loaded:model];

  // Make sure the internal bookmark button also is the correct height.
  NSArray* buttons = [bar_ buttons];
  EXPECT_GT([buttons count], 0u);

  for (NSButton* button in buttons) {
    EXPECT_FLOAT_EQ((GetCocoaLayoutConstant(BOOKMARK_BAR_HEIGHT_NO_OVERLAP) +
                     bookmarks::kMaterialVisualHeightOffset) -
                        2 * bookmarks::kBookmarkVerticalPadding,
                    [button frame].size.height);
  }
}

TEST_F(BookmarkBarControllerTest, DropBookmarks) {
  const char* urls[] = {
    "http://qwantz.com",
    "http://xkcd.com",
    "javascript:alert('lolwut')",
    "file://localhost/tmp/local-file.txt"  // As if dragged from the desktop.
  };
  const char* titles[] = {
    "Philosophoraptor",
    "Can't draw",
    "Inspiration",
    "Frum stuf"
  };
  EXPECT_EQ(arraysize(urls), arraysize(titles));

  NSMutableArray* nsurls = [NSMutableArray array];
  NSMutableArray* nstitles = [NSMutableArray array];
  for (size_t i = 0; i < arraysize(urls); ++i) {
    [nsurls addObject:base::SysUTF8ToNSString(urls[i])];
    [nstitles addObject:base::SysUTF8ToNSString(titles[i])];
  }

  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* parent = model->bookmark_bar_node();
  [bar_ addURLs:nsurls withTitles:nstitles at:NSZeroPoint];
  EXPECT_EQ(4, parent->child_count());
  for (int i = 0; i < parent->child_count(); ++i) {
    GURL gurl = parent->GetChild(i)->url();
    if (gurl.scheme() == "http" ||
        gurl.scheme() == "javascript") {
      EXPECT_EQ(parent->GetChild(i)->url(), GURL(urls[i]));
    } else {
      // Be flexible if the scheme needed to be added.
      std::string gurl_string = gurl.spec();
      std::string my_string = parent->GetChild(i)->url().spec();
      EXPECT_NE(gurl_string.find(my_string), std::string::npos);
    }
    EXPECT_EQ(parent->GetChild(i)->GetTitle(), ASCIIToUTF16(titles[i]));
  }
}

TEST_F(BookmarkBarControllerTest, TestDragButton) {
  WithNoAnimation at_all;
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  GURL gurls[] = { GURL("http://www.google.com/a"),
                   GURL("http://www.google.com/b"),
                   GURL("http://www.google.com/c") };
  base::string16 titles[] = { ASCIIToUTF16("a"),
                              ASCIIToUTF16("b"),
                              ASCIIToUTF16("c") };
  for (unsigned i = 0; i < arraysize(titles); i++)
    bookmarks::AddIfNotBookmarked(model, gurls[i], titles[i]);

  EXPECT_EQ([[bar_ buttons] count], arraysize(titles));
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:0] title]);

  [bar_ dragButton:[[bar_ buttons] objectAtIndex:2]
                to:NSZeroPoint
              copy:NO];
  EXPECT_NSEQ(@"c", [[[bar_ buttons] objectAtIndex:0] title]);
  // Make sure a 'copy' did not happen.
  EXPECT_EQ([[bar_ buttons] count], arraysize(titles));

  [bar_ dragButton:[[bar_ buttons] objectAtIndex:1]
                to:NSMakePoint(1000, 0)
              copy:NO];
  EXPECT_NSEQ(@"c", [[[bar_ buttons] objectAtIndex:0] title]);
  EXPECT_NSEQ(@"b", [[[bar_ buttons] objectAtIndex:1] title]);
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:2] title]);
  EXPECT_EQ([[bar_ buttons] count], arraysize(titles));

  // A drop of the 1st between the next 2.
  CGFloat x = NSMinX([[[bar_ buttons] objectAtIndex:2] frame]);
  x += [[bar_ view] frame].origin.x;
  [bar_ dragButton:[[bar_ buttons] objectAtIndex:0]
                to:NSMakePoint(x, 0)
              copy:NO];
  EXPECT_NSEQ(@"b", [[[bar_ buttons] objectAtIndex:0] title]);
  EXPECT_NSEQ(@"c", [[[bar_ buttons] objectAtIndex:1] title]);
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:2] title]);
  EXPECT_EQ([[bar_ buttons] count], arraysize(titles));

  // A drop on a non-folder button.  (Shouldn't try and go in it.)
  x = NSMidX([[[bar_ buttons] objectAtIndex:0] frame]);
  x += [[bar_ view] frame].origin.x;
  [bar_ dragButton:[[bar_ buttons] objectAtIndex:2]
                to:NSMakePoint(x, 0)
              copy:NO];
  EXPECT_EQ(arraysize(titles), [[bar_ buttons] count]);

  // A drop on a folder button.
  const BookmarkNode* folder = model->AddFolder(
      model->bookmark_bar_node(), 0, ASCIIToUTF16("awesome folder"));
  DCHECK(folder);
  model->AddURL(folder, 0, ASCIIToUTF16("already"),
                GURL("http://www.google.com"));
  EXPECT_EQ(arraysize(titles) + 1, [[bar_ buttons] count]);
  EXPECT_EQ(1, folder->child_count());
  x = NSMidX([[[bar_ buttons] objectAtIndex:0] frame]);
  x += [[bar_ view] frame].origin.x;
  base::string16 title =
      [[[bar_ buttons] objectAtIndex:2] bookmarkNode]->GetTitle();
  [bar_ dragButton:[[bar_ buttons] objectAtIndex:2]
                to:NSMakePoint(x, 0)
              copy:NO];
  // Gone from the bar
  EXPECT_EQ(arraysize(titles), [[bar_ buttons] count]);
  // In the folder
  EXPECT_EQ(2, folder->child_count());
  // At the end
  EXPECT_EQ(title, folder->GetChild(1)->GetTitle());
}

TEST_F(BookmarkBarControllerTest, TestCopyButton) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  GURL gurls[] = { GURL("http://www.google.com/a"),
                   GURL("http://www.google.com/b"),
                   GURL("http://www.google.com/c") };
  base::string16 titles[] = { ASCIIToUTF16("a"),
                              ASCIIToUTF16("b"),
                              ASCIIToUTF16("c") };
  for (unsigned i = 0; i < arraysize(titles); i++)
    bookmarks::AddIfNotBookmarked(model, gurls[i], titles[i]);

  EXPECT_EQ([[bar_ buttons] count], arraysize(titles));
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:0] title]);

  // Drag 'a' between 'b' and 'c'.
  CGFloat x = NSMinX([[[bar_ buttons] objectAtIndex:2] frame]);
  x += [[bar_ view] frame].origin.x;
  [bar_ dragButton:[[bar_ buttons] objectAtIndex:0]
                to:NSMakePoint(x, 0)
              copy:YES];
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:0] title]);
  EXPECT_NSEQ(@"b", [[[bar_ buttons] objectAtIndex:1] title]);
  EXPECT_NSEQ(@"a", [[[bar_ buttons] objectAtIndex:2] title]);
  EXPECT_NSEQ(@"c", [[[bar_ buttons] objectAtIndex:3] title]);
  EXPECT_EQ([[bar_ buttons] count], 4U);
}

// Fake a theme with colored text.  Apply it and make sure bookmark
// buttons have the same colored text.  Repeat more than once.
TEST_F(BookmarkBarControllerTest, TestThemedButton) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  bookmarks::AddIfNotBookmarked(
      model, GURL("http://www.foo.com"), ASCIIToUTF16("small"));
  BookmarkButton* button = [[bar_ buttons] objectAtIndex:0];
  EXPECT_TRUE(button);

  NSArray* colors = [NSArray arrayWithObjects:[NSColor redColor],
                                              [NSColor blueColor],
                                              nil];
  for (NSColor* color in colors) {
    FakeTheme theme(color);
    [bar_ updateTheme:&theme];
    NSAttributedString* astr = [button attributedTitle];
    EXPECT_TRUE(astr);
    EXPECT_NSEQ(@"small", [astr string]);
    // Pick a char in the middle to test (index 3)
    NSDictionary* attributes = [astr attributesAtIndex:3 effectiveRange:NULL];
    NSColor* newColor =
        [attributes objectForKey:NSForegroundColorAttributeName];
    EXPECT_NSEQ(newColor, color);
  }
}

// Test that delegates and targets of buttons are cleared on dealloc.
TEST_F(BookmarkBarControllerTest, TestClearOnDealloc) {
  // Make some bookmark buttons.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  GURL gurls[] = { GURL("http://www.foo.com/"),
                   GURL("http://www.bar.com/"),
                   GURL("http://www.baz.com/") };
  base::string16 titles[] = { ASCIIToUTF16("a"),
                              ASCIIToUTF16("b"),
                              ASCIIToUTF16("c") };
  for (size_t i = 0; i < arraysize(titles); i++)
    bookmarks::AddIfNotBookmarked(model, gurls[i], titles[i]);

  // Get and retain the buttons so we can examine them after dealloc.
  base::scoped_nsobject<NSArray> buttons([[bar_ buttons] retain]);
  EXPECT_EQ([buttons count], arraysize(titles));

  // Make sure that everything is set.
  for (BookmarkButton* button in buttons.get()) {
    ASSERT_TRUE([button isKindOfClass:[BookmarkButton class]]);
    EXPECT_TRUE([button delegate]);
    EXPECT_TRUE([button target]);
    EXPECT_TRUE([button action]);
  }

  [bar_ browserWillBeDestroyed];

  // Make sure that everything is cleared.
  for (BookmarkButton* button in buttons.get()) {
    EXPECT_FALSE([button delegate]);
    EXPECT_FALSE([button target]);
    EXPECT_FALSE([button action]);
  }
}

TEST_F(BookmarkBarControllerTest, TestFolders) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  // Create some folder buttons.
  const BookmarkNode* parent = model->bookmark_bar_node();
  const BookmarkNode* folder = model->AddFolder(parent,
                                                parent->child_count(),
                                                ASCIIToUTF16("folder"));
  model->AddURL(folder, folder->child_count(),
                ASCIIToUTF16("f1"), GURL("http://framma-lamma.com"));
  folder = model->AddFolder(parent, parent->child_count(),
                            ASCIIToUTF16("empty"));

  EXPECT_EQ([[bar_ buttons] count], 2U);

  // First confirm mouseEntered does nothing if "menus" aren't active.
  NSEvent* event =
      cocoa_test_event_utils::MouseEventWithType(NSOtherMouseUp, 0);
  [bar_ mouseEnteredButton:[[bar_ buttons] objectAtIndex:0] event:event];
  EXPECT_FALSE([bar_ folderController]);

  // Make one active.  Entering it is now a no-op.
  [bar_ openBookmarkFolderFromButton:[[bar_ buttons] objectAtIndex:0]];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);
  [bar_ mouseEnteredButton:[[bar_ buttons] objectAtIndex:0] event:event];
  EXPECT_EQ(bbfc, [bar_ folderController]);

  // Enter a different one; a new folderController is active.
  [bar_ mouseEnteredButton:[[bar_ buttons] objectAtIndex:1] event:event];
  EXPECT_NE(bbfc, [bar_ folderController]);

  // Confirm exited is a no-op.
  [bar_ mouseExitedButton:[[bar_ buttons] objectAtIndex:1] event:event];
  EXPECT_NE(bbfc, [bar_ folderController]);

  // Clean up.
  [bar_ closeBookmarkFolder:nil];
}

// Verify that the folder menu presentation properly tracks mouse movements
// over the bar. Until there is a click no folder menus should show. After a
// click on a folder folder menus should show until another click on a folder
// button, and a click outside the bar and its folder menus.
TEST_F(BookmarkBarControllerTest, TestFolderButtons) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2b ] 3b 4f:[ 4f1b 4f2b ] ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model and that we do not have a folder controller.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);
  EXPECT_FALSE([bar_ folderController]);

  // Add a real bookmark so we can click on it.
  const BookmarkNode* folder = root->GetChild(3);
  model->AddURL(folder, folder->child_count(), ASCIIToUTF16("CLICK ME"),
                GURL("http://www.google.com/"));

  // Click on a folder button.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"4f"];
  EXPECT_TRUE(button);
  [bar_ openBookmarkFolderFromButton:button];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);

  // Make sure a 2nd click on the same button closes things.
  [bar_ openBookmarkFolderFromButton:button];
  EXPECT_FALSE([bar_ folderController]);

  // Next open is a different button.
  button = [bar_ buttonWithTitleEqualTo:@"2f"];
  EXPECT_TRUE(button);
  [bar_ openBookmarkFolderFromButton:button];
  EXPECT_TRUE([bar_ folderController]);

  // Mouse over a non-folder button and confirm controller has gone away.
  button = [bar_ buttonWithTitleEqualTo:@"1b"];
  EXPECT_TRUE(button);
  NSEvent* event = cocoa_test_event_utils::MouseEventAtPoint([button center],
                                                             NSMouseMoved, 0);
  [bar_ mouseEnteredButton:button event:event];
  EXPECT_FALSE([bar_ folderController]);

  // Mouse over the original folder and confirm a new controller.
  button = [bar_ buttonWithTitleEqualTo:@"2f"];
  EXPECT_TRUE(button);
  [bar_ mouseEnteredButton:button event:event];
  BookmarkBarFolderController* oldBBFC = [bar_ folderController];
  EXPECT_TRUE(oldBBFC);

  // 'Jump' over to a different folder and confirm a new controller.
  button = [bar_ buttonWithTitleEqualTo:@"4f"];
  EXPECT_TRUE(button);
  [bar_ mouseEnteredButton:button event:event];
  BookmarkBarFolderController* newBBFC = [bar_ folderController];
  EXPECT_TRUE(newBBFC);
  EXPECT_NE(oldBBFC, newBBFC);
}

// Make sure the "off the side" folder looks like a bookmark folder
// but only contains "off the side" items.
TEST_F(BookmarkBarControllerTest, OffTheSideFolder) {

  // It starts hidden.
  EXPECT_TRUE([[bar_ offTheSideButton] isHidden]);

  // Create some buttons.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* parent = model->bookmark_bar_node();
  for (int x = 0; x < 30; x++) {
    model->AddURL(parent, parent->child_count(),
                  ASCIIToUTF16("medium-size-title"),
                  GURL("http://framma-lamma.com"));
  }
  // Add a couple more so we can delete one and make sure its button goes away.
  model->AddURL(parent, parent->child_count(),
                ASCIIToUTF16("DELETE_ME"), GURL("http://ashton-tate.com"));
  model->AddURL(parent, parent->child_count(),
                ASCIIToUTF16("medium-size-title"),
                GURL("http://framma-lamma.com"));

  // Should no longer be hidden.
  EXPECT_FALSE([[bar_ offTheSideButton] isHidden]);

  // Open it; make sure we have a folder controller.
  EXPECT_FALSE([bar_ folderController]);
  [bar_ openOffTheSideFolderFromButton:[bar_ offTheSideButton]];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  EXPECT_TRUE(bbfc);

  // Confirm the contents are only buttons which fell off the side by
  // making sure that none of the nodes in the off-the-side folder are
  // found in bar buttons.  Be careful since not all the bar buttons
  // may be currently displayed.
  NSArray* folderButtons = [bbfc buttons];
  NSArray* barButtons = [bar_ buttons];
  for (BookmarkButton* folderButton in folderButtons) {
    for (BookmarkButton* barButton in barButtons) {
      if ([barButton superview]) {
        EXPECT_NE([folderButton bookmarkNode], [barButton bookmarkNode]);
      }
    }
  }

  // Delete a bookmark in the off-the-side and verify it's gone.
  BookmarkButton* button = [bbfc buttonWithTitleEqualTo:@"DELETE_ME"];
  EXPECT_TRUE(button);
  model->Remove(parent->GetChild(parent->child_count() - 2));
  button = [bbfc buttonWithTitleEqualTo:@"DELETE_ME"];
  EXPECT_FALSE(button);
}

TEST_F(BookmarkBarControllerTest, EventToExitCheck) {
  NSEvent* event = cocoa_test_event_utils::MouseEventWithType(NSMouseMoved, 0);
  EXPECT_FALSE([bar_ isEventAnExitEvent:event]);

  BookmarkBarFolderWindow* folderWindow = [[[BookmarkBarFolderWindow alloc]
                                             init] autorelease];
  [[[bar_ view] window] addChildWindow:folderWindow
                               ordered:NSWindowAbove];
  event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(NSMakePoint(1,1),
                                                               folderWindow);
  EXPECT_FALSE([bar_ isEventAnExitEvent:event]);

  event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(100,100), test_window());
  EXPECT_TRUE([bar_ isEventAnExitEvent:event]);

  // Many components are arbitrary (e.g. location, keycode).
  event = [NSEvent keyEventWithType:NSKeyDown
                           location:NSMakePoint(1,1)
                      modifierFlags:0
                          timestamp:0
                       windowNumber:0
                            context:nil
                         characters:@"x"
        charactersIgnoringModifiers:@"x"
                          isARepeat:NO
                            keyCode:87];
  EXPECT_FALSE([bar_ isEventAnExitEvent:event]);

  [[[bar_ view] window] removeChildWindow:folderWindow];
}

TEST_F(BookmarkBarControllerTest, DropDestination) {
  // Make some buttons.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* parent = model->bookmark_bar_node();
  model->AddFolder(parent, parent->child_count(), ASCIIToUTF16("folder 1"));
  model->AddFolder(parent, parent->child_count(), ASCIIToUTF16("folder 2"));
  EXPECT_EQ([[bar_ buttons] count], 2U);

  // Confirm "off to left" and "off to right" match nothing.
  NSPoint p = NSMakePoint(-1, 2);
  EXPECT_FALSE([bar_ buttonForDroppingOnAtPoint:p]);
  EXPECT_TRUE([bar_ shouldShowIndicatorShownForPoint:p]);
  p = NSMakePoint(50000, 10);
  EXPECT_FALSE([bar_ buttonForDroppingOnAtPoint:p]);
  EXPECT_TRUE([bar_ shouldShowIndicatorShownForPoint:p]);

  // Confirm "right in the center" (give or take a pixel) is a match,
  // and confirm "just barely in the button" is not.  Anything more
  // specific seems likely to be tweaked.
  CGFloat viewFrameXOffset = [[bar_ view] frame].origin.x;
  for (BookmarkButton* button in [bar_ buttons]) {
    CGFloat x = NSMidX([button frame]) + viewFrameXOffset;
    // Somewhere near the center: a match
    EXPECT_EQ(button,
              [bar_ buttonForDroppingOnAtPoint:NSMakePoint(x-1, 10)]);
    EXPECT_EQ(button,
              [bar_ buttonForDroppingOnAtPoint:NSMakePoint(x+1, 10)]);
    EXPECT_FALSE([bar_ shouldShowIndicatorShownForPoint:NSMakePoint(x, 10)]);;

    // On the very edges: NOT a match
    x = NSMinX([button frame]) + viewFrameXOffset;
    EXPECT_NE(button,
              [bar_ buttonForDroppingOnAtPoint:NSMakePoint(x, 9)]);
    x = NSMaxX([button frame]) + viewFrameXOffset;
    EXPECT_NE(button,
              [bar_ buttonForDroppingOnAtPoint:NSMakePoint(x, 11)]);
  }
}

TEST_F(BookmarkBarControllerTest, CloseFolderOnAnimate) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  [bar_ setStateAnimationsEnabled:YES];
  const BookmarkNode* parent = model->bookmark_bar_node();
  const BookmarkNode* folder = model->AddFolder(parent,
                                                parent->child_count(),
                                                ASCIIToUTF16("folder"));
  model->AddFolder(parent, parent->child_count(),
                  ASCIIToUTF16("sibbling folder"));
  model->AddURL(folder, folder->child_count(), ASCIIToUTF16("title a"),
                GURL("http://www.google.com/a"));
  model->AddURL(folder, folder->child_count(),
      ASCIIToUTF16("title super duper long long whoa momma title you betcha"),
      GURL("http://www.google.com/b"));
  BookmarkButton* button = [[bar_ buttons] objectAtIndex:0];
  EXPECT_FALSE([bar_ folderController]);
  [bar_ openBookmarkFolderFromButton:button];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  // The following tells us that the folder menu is showing. We want to make
  // sure the folder menu goes away if the bookmark bar is hidden.
  EXPECT_TRUE(bbfc);
  EXPECT_TRUE([bar_ isVisible]);

  // Hide the bookmark bar.
  [bar_ updateState:BookmarkBar::DETACHED
         changeType:BookmarkBar::ANIMATE_STATE_CHANGE];
  EXPECT_TRUE([bar_ isAnimationRunning]);

  // Now that we've closed the bookmark bar (with animation) the folder menu
  // should have been closed thus releasing the folderController.
  EXPECT_FALSE([bar_ folderController]);

  // Stop the pending animation to tear down cleanly.
  [bar_ updateState:BookmarkBar::DETACHED
         changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  EXPECT_FALSE([bar_ isAnimationRunning]);
}

class BookmarkBarControllerOpenAllTest : public BookmarkBarControllerTest {
public:
 void SetUp() override {
    BookmarkBarControllerTest::SetUp();
    ASSERT_TRUE(profile());

    resizeDelegate_.reset([[ViewResizerPong alloc] init]);
    NSRect parent_frame = NSMakeRect(0, 0, 800, 50);
    bar_.reset([[BookmarkBarControllerOpenAllPong alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth(parent_frame)
               delegate:nil]);
    // Forces loading of the nib.
    [[bar_ controlledView] setResizeDelegate:resizeDelegate_];
    // Awkwardness to look like we've been installed.
    [parent_view_ addSubview:[bar_ view]];
    NSRect frame = [[[bar_ view] superview] frame];
    frame.origin.y = 100;
    [[[bar_ view] superview] setFrame:frame];

    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    parent_ = model->bookmark_bar_node();
    // { one, { two-one, two-two }, three }
    model->AddURL(parent_, parent_->child_count(), ASCIIToUTF16("title"),
                  GURL("http://one.com"));
    folder_ = model->AddFolder(parent_, parent_->child_count(),
                               ASCIIToUTF16("folder"));
    model->AddURL(folder_, folder_->child_count(),
                  ASCIIToUTF16("title"), GURL("http://two-one.com"));
    model->AddURL(folder_, folder_->child_count(),
                  ASCIIToUTF16("title"), GURL("http://two-two.com"));
    model->AddURL(parent_, parent_->child_count(),
                  ASCIIToUTF16("title"), GURL("https://three.com"));
  }
  const BookmarkNode* parent_;  // Weak
  const BookmarkNode* folder_;  // Weak
};

// Command-click on a folder should open all the bookmarks in it.
TEST_F(BookmarkBarControllerOpenAllTest, CommandClickOnFolder) {
  NSButton* first = [[bar_ buttons] objectAtIndex:0];
  EXPECT_TRUE(first);

  // Create the right kind of event; mock NSApp so [NSApp
  // currentEvent] finds it.
  NSEvent* commandClick =
      cocoa_test_event_utils::MouseEventAtPoint(NSZeroPoint,
                                                NSLeftMouseDown,
                                                NSCommandKeyMask);
  id fakeApp = [OCMockObject partialMockForObject:NSApp];
  [[[fakeApp stub] andReturn:commandClick] currentEvent];
  id oldApp = NSApp;
  NSApp = fakeApp;
  size_t originalDispositionCount = noOpenBar()->dispositions_.size();

  // Click!
  [first performClick:first];

  size_t dispositionCount = noOpenBar()->dispositions_.size();
  EXPECT_EQ(originalDispositionCount + 1, dispositionCount);
  EXPECT_EQ(noOpenBar()->dispositions_[dispositionCount - 1],
            WindowOpenDisposition::NEW_BACKGROUND_TAB);

  // Replace NSApp
  NSApp = oldApp;
}

class BookmarkBarControllerNotificationTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    resizeDelegate_.reset([[ViewResizerPong alloc] init]);
    NSRect parent_frame = NSMakeRect(0, 0, 800, 50);
    parent_view_.reset([[NSView alloc] initWithFrame:parent_frame]);
    [parent_view_ setHidden:YES];
    bar_.reset([[BookmarkBarControllerNotificationPong alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth(parent_frame)
               delegate:nil]);

    // Loads the view
    [[bar_ controlledView] setResizeDelegate:resizeDelegate_];
    // Awkwardness to look like we've been installed.
    [parent_view_ addSubview:[bar_ view]];
    NSRect frame = [[[bar_ view] superview] frame];
    frame.origin.y = 100;
    [[[bar_ view] superview] setFrame:frame];

    // Do not add the bar to a window, yet.
  }

  base::scoped_nsobject<NSView> parent_view_;
  base::scoped_nsobject<ViewResizerPong> resizeDelegate_;
  base::scoped_nsobject<BookmarkBarControllerNotificationPong> bar_;
};

TEST_F(BookmarkBarControllerNotificationTest, DeregistersForNotifications) {
  NSWindow* window = [[CocoaTestHelperWindow alloc] init];
  [window setReleasedWhenClosed:YES];

  // First add the bookmark bar to the temp window, then to another window.
  [[window contentView] addSubview:parent_view_];
  [[test_window() contentView] addSubview:parent_view_];

  // Post a fake windowDidResignKey notification for the temp window and make
  // sure the bookmark bar controller wasn't listening.
  [[NSNotificationCenter defaultCenter]
      postNotificationName:NSWindowDidResignKeyNotification
                    object:window];
  EXPECT_FALSE([bar_ windowDidResignKeyReceived]);

  // Close the temp window and make sure no notification was received.
  [window close];
  EXPECT_FALSE([bar_ windowWillCloseReceived]);
}


// TODO(jrg): draggingEntered: and draggingExited: trigger timers so
// they are hard to test.  Factor out "fire timers" into routines
// which can be overridden to fire immediately to make behavior
// confirmable.

// TODO(jrg): add unit test to make sure "Other Bookmarks" responds
// properly to a hover open.

// TODO(viettrungluu): figure out how to test animations.

class BookmarkBarControllerDragDropTest : public BookmarkBarControllerTestBase {
 public:
  base::scoped_nsobject<BookmarkBarControllerDragData> bar_;

  void SetUp() override {
    BookmarkBarControllerTestBase::SetUp();
    ASSERT_TRUE(browser());

    bar_.reset([[BookmarkBarControllerDragData alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth([parent_view_ frame])
               delegate:nil]);
    InstallAndToggleBar(bar_.get());
  }
};

TEST_F(BookmarkBarControllerDragDropTest, DragMoveBarBookmarkToOffTheSide) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1bWithLongName 2fWithLongName:[ "
      "2f1bWithLongName 2f2fWithLongName:[ 2f2f1bWithLongName "
      "2f2f2bWithLongName 2f2f3bWithLongName 2f4b ] 2f3bWithLongName ] "
      "3bWithLongName 4bWithLongName 5bWithLongName 6bWithLongName "
      "7bWithLongName 8bWithLongName 9bWithLongName 10bWithLongName "
      "11bWithLongName 12bWithLongName 13b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Ensure that the off-the-side button is showing.
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  ASSERT_TRUE(layout.IsOffTheSideButtonVisible());

  // Remember how many buttons are showing and are available.
  int oldDisplayedButtons = layout.VisibleButtonCount();
  int oldChildCount = root->child_count();
  // Pop up the off-the-side menu.
  BookmarkButton* otsButton = (BookmarkButton*)[bar_ offTheSideButton];
  ASSERT_TRUE(otsButton);
  [[otsButton target] performSelector:@selector(openOffTheSideFolderFromButton:)
                           withObject:otsButton];
  BookmarkBarFolderController* otsController = [bar_ folderController];
  EXPECT_TRUE(otsController);
  NSWindow* toWindow = [otsController window];
  EXPECT_TRUE(toWindow);
  BookmarkButton* draggedButton =
      [bar_ buttonWithTitleEqualTo:@"3bWithLongName"];
  ASSERT_TRUE(draggedButton);
  int oldOTSCount = (int)[[otsController buttons] count];
  EXPECT_EQ(oldOTSCount, oldChildCount - oldDisplayedButtons);
  BookmarkButton* targetButton = [[otsController buttons] objectAtIndex:0];
  ASSERT_TRUE(targetButton);
  [otsController dragButton:draggedButton
                         to:[targetButton center]
                       copy:YES];
  // Close
  [[otsButton target] performSelector:@selector(openOffTheSideFolderFromButton:)
                           withObject:otsButton];
  // Open
  [[otsButton target] performSelector:@selector(openOffTheSideFolderFromButton:)
                           withObject:otsButton];

  // There should still be the same number of buttons in the bar
  // and off-the-side should have one more.
  layout = [bar_ layoutFromCurrentState];
  int newDisplayedButtons = layout.VisibleButtonCount();
  int newChildCount = root->child_count();
  int newOTSCount = (int)[[[bar_ folderController] buttons] count];
  EXPECT_EQ(oldDisplayedButtons, newDisplayedButtons);
  EXPECT_EQ(oldChildCount + 1, newChildCount);
  EXPECT_EQ(oldOTSCount + 1, newOTSCount);
  EXPECT_EQ(newOTSCount, newChildCount - newDisplayedButtons);
}

TEST_F(BookmarkBarControllerDragDropTest, DragOffTheSideToOther) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1bWithLongName 2bWithLongName "
      "3bWithLongName 4bWithLongName 5bWithLongName 6bWithLongName "
      "7bWithLongName 8bWithLongName 9bWithLongName 10bWithLongName "
      "11bWithLongName 12bWithLongName 13bWithLongName 14bWithLongName "
      "15bWithLongName 16bWithLongName 17bWithLongName 18bWithLongName "
      "19bWithLongName 20bWithLongName ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  const BookmarkNode* other = model->other_node();
  const std::string other_string("1other 2other 3other ");
  bookmarks::test::AddNodesFromModelString(model, other, other_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);
  std::string actualOtherString = bookmarks::test::ModelStringFromNode(other);
  EXPECT_EQ(other_string, actualOtherString);

  // Ensure that the off-the-side button is showing.
  bookmarks::BookmarkBarLayout layout = [bar_ layoutFromCurrentState];
  ASSERT_TRUE(layout.IsOffTheSideButtonVisible());

  // Remember how many buttons are showing and are available.
  int oldDisplayedButtons = layout.VisibleButtonCount();
  int oldRootCount = root->child_count();
  int oldOtherCount = other->child_count();

  // Pop up the off-the-side menu.
  BookmarkButton* otsButton = (BookmarkButton*)[bar_ offTheSideButton];
  ASSERT_TRUE(otsButton);
  [[otsButton target] performSelector:@selector(openOffTheSideFolderFromButton:)
                           withObject:otsButton];
  BookmarkBarFolderController* otsController = [bar_ folderController];
  EXPECT_TRUE(otsController);
  int oldOTSCount = (int)[[otsController buttons] count];
  EXPECT_EQ(oldOTSCount, oldRootCount - oldDisplayedButtons);

  // Pick an off-the-side button and drag it to the other bookmarks.
  BookmarkButton* draggedButton =
      [otsController buttonWithTitleEqualTo:@"20bWithLongName"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton = [bar_ otherBookmarksButton];
  ASSERT_TRUE(targetButton);
  [bar_ dragButton:draggedButton to:[targetButton center] copy:NO];

  // There should one less button in the bar, one less in off-the-side,
  // and one more in other bookmarks.
  int newRootCount = root->child_count();
  int newOTSCount = (int)[[otsController buttons] count];
  int newOtherCount = other->child_count();
  EXPECT_EQ(oldRootCount - 1, newRootCount);
  EXPECT_EQ(oldOTSCount - 1, newOTSCount);
  EXPECT_EQ(oldOtherCount + 1, newOtherCount);
}

TEST_F(BookmarkBarControllerDragDropTest, DragBookmarkData) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                  "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);
  const BookmarkNode* other = model->other_node();
  const std::string other_string("O1b O2b O3f:[ O3f1b O3f2f ] "
                                 "O4f:[ O4f1b O4f2f ] 05b ");
  bookmarks::test::AddNodesFromModelString(model, other, other_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);
  actual = bookmarks::test::ModelStringFromNode(other);
  EXPECT_EQ(other_string, actual);

  // Remember the little ones.
  int oldChildCount = root->child_count();

  BookmarkButton* targetButton = [bar_ buttonWithTitleEqualTo:@"3b"];
  ASSERT_TRUE(targetButton);

  // Gen up some dragging data.
  const BookmarkNode* newNode = other->GetChild(2);
  [bar_ setDragDataNode:newNode];
  base::scoped_nsobject<FakeDragInfo> dragInfo([[FakeDragInfo alloc] init]);
  [dragInfo setDropLocation:[targetButton center]];
  [bar_ dragBookmarkData:(id<NSDraggingInfo>)dragInfo.get()];

  // There should one more button in the bar.
  int newChildCount = root->child_count();
  EXPECT_EQ(oldChildCount + 1, newChildCount);
  // Verify the model.
  const std::string expected("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                             "2f3b ] O3f:[ O3f1b O3f2f ] 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);
  oldChildCount = newChildCount;

  // Now do it over a folder button.
  targetButton = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(targetButton);
  NSPoint targetPoint = [targetButton center];
  newNode = other->GetChild(2);  // Should be O4f.
  EXPECT_EQ(newNode->GetTitle(), ASCIIToUTF16("O4f"));
  [bar_ setDragDataNode:newNode];
  [dragInfo setDropLocation:targetPoint];
  [bar_ dragBookmarkData:(id<NSDraggingInfo>)dragInfo.get()];

  newChildCount = root->child_count();
  EXPECT_EQ(oldChildCount, newChildCount);
  // Verify the model.
  const std::string expected1("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                              "2f3b O4f:[ O4f1b O4f2f ] ] O3f:[ O3f1b O3f2f ] "
                              "3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected1, actual);
}

TEST_F(BookmarkBarControllerDragDropTest, AddURLs) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                 "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);

  // Remember the children.
  int oldChildCount = root->child_count();

  BookmarkButton* targetButton = [bar_ buttonWithTitleEqualTo:@"3b"];
  ASSERT_TRUE(targetButton);

  NSArray* urls = [NSArray arrayWithObjects: @"http://www.a.com/",
                   @"http://www.b.com/", nil];
  NSArray* titles = [NSArray arrayWithObjects: @"SiteA", @"SiteB", nil];
  [bar_ addURLs:urls withTitles:titles at:[targetButton center]];

  // There should two more nodes in the bar.
  int newChildCount = root->child_count();
  EXPECT_EQ(oldChildCount + 2, newChildCount);
  // Verify the model.
  const std::string expected("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                             "2f3b ] SiteA SiteB 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);
}

TEST_F(BookmarkBarControllerDragDropTest, ControllerForNode) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2b ] 3b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Find the main bar controller.
  const void* expectedController = bar_;
  const void* actualController = [bar_ controllerForNode:root];
  EXPECT_EQ(expectedController, actualController);
}

TEST_F(BookmarkBarControllerDragDropTest, DropPositionIndicator) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2b 2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Hide the apps shortcut.
  profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar, false);
  ASSERT_TRUE([[bar_ appsPageShortcutButton] isHidden]);

  // Validate initial model.
  std::string actualModel = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModel);

  // Test a series of points starting at the right edge of the bar.
  BookmarkButton* targetButton = [bar_ buttonWithTitleEqualTo:@"1b"];
  ASSERT_TRUE(targetButton);
  NSPoint targetPoint = [targetButton left];
  CGFloat leftMarginIndicatorPosition = bookmarks::kBookmarkLeftMargin - 0.5 *
                                        bookmarks::kBookmarkHorizontalPadding;
  const CGFloat baseOffset = targetPoint.x;
  CGFloat expected = leftMarginIndicatorPosition;
  CGFloat actual = [bar_ indicatorPosForDragToPoint:targetPoint];
  EXPECT_CGFLOAT_EQ(expected, actual);
  targetButton = [bar_ buttonWithTitleEqualTo:@"2f"];
  actual = [bar_ indicatorPosForDragToPoint:[targetButton right]];
  targetButton = [bar_ buttonWithTitleEqualTo:@"3b"];
  expected = [targetButton left].x - baseOffset + leftMarginIndicatorPosition;
  EXPECT_CGFLOAT_EQ(expected, actual);
  targetButton = [bar_ buttonWithTitleEqualTo:@"4b"];
  targetPoint = [targetButton right];
  targetPoint.x += 100;  // Somewhere off to the right.
  CGFloat xDelta = 0.5 * bookmarks::kBookmarkHorizontalPadding;
  expected = NSMaxX([targetButton frame]) + xDelta;
  actual = [bar_ indicatorPosForDragToPoint:targetPoint];
  EXPECT_CGFLOAT_EQ(expected, actual);
}

TEST_F(BookmarkBarControllerDragDropTest, PulseButton) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  GURL gurl("http://www.google.com");
  const BookmarkNode* node = model->AddURL(root, root->child_count(),
                                           ASCIIToUTF16("title"), gurl);

  BookmarkButton* button = [[bar_ buttons] objectAtIndex:0];
  EXPECT_FALSE([button isPulseStuckOn]);
  [bar_ startPulsingBookmarkNode:node];
  EXPECT_TRUE([button isPulseStuckOn]);
  [bar_ stopPulsingBookmarkNode];
  EXPECT_FALSE([button isPulseStuckOn]);

  // Pulsing a node within a folder should pulse the folder button.
  const BookmarkNode* folder =
      model->AddFolder(root, root->child_count(), ASCIIToUTF16("folder"));
  const BookmarkNode* inner =
      model->AddURL(folder, folder->child_count(), ASCIIToUTF16("inner"), gurl);

  BookmarkButton* folder_button = [[bar_ buttons] objectAtIndex:1];
  EXPECT_FALSE([folder_button isPulseStuckOn]);
  [bar_ startPulsingBookmarkNode:inner];
  EXPECT_TRUE([folder_button isPulseStuckOn]);
  [bar_ stopPulsingBookmarkNode];
  EXPECT_FALSE([folder_button isPulseStuckOn]);

  // Stop pulsing if the node moved.
  [bar_ startPulsingBookmarkNode:inner];
  EXPECT_TRUE([folder_button isPulseStuckOn]);
  const BookmarkNode* folder2 =
      model->AddFolder(root, root->child_count(), ASCIIToUTF16("folder2"));
  model->Move(inner, folder2, 0);
  EXPECT_FALSE([folder_button isPulseStuckOn]);

  // Removing a pulsing folder is allowed.
  [bar_ startPulsingBookmarkNode:inner];
  BookmarkButton* folder2_button = [[bar_ buttons] objectAtIndex:2];
  EXPECT_TRUE([folder2_button isPulseStuckOn]);
  model->Remove(folder2);
  EXPECT_FALSE([folder2_button isPulseStuckOn]);
  [bar_ stopPulsingBookmarkNode];  // Should not crash.
}

TEST_F(BookmarkBarControllerDragDropTest, DragBookmarkDataToTrash) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                  "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);

  int oldChildCount = root->child_count();

  // Drag a button to the trash.
  BookmarkButton* buttonToDelete = [bar_ buttonWithTitleEqualTo:@"3b"];
  ASSERT_TRUE(buttonToDelete);
  EXPECT_TRUE([bar_ canDragBookmarkButtonToTrash:buttonToDelete]);
  [bar_ didDragBookmarkToTrash:buttonToDelete];

  // There should be one less button in the bar.
  int newChildCount = root->child_count();
  EXPECT_EQ(oldChildCount - 1, newChildCount);
  // Verify the model.
  const std::string expected("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                             "2f3b ] 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);

  // Verify that the other bookmark folder can't be deleted.
  BookmarkButton *otherButton = [bar_ otherBookmarksButton];
  EXPECT_FALSE([bar_ canDragBookmarkButtonToTrash:otherButton]);
}

}  // namespace
