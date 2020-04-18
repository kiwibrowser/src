// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"

#include <stddef.h>

#import "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/bookmark_stats.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils_desktop.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/chrome_pages.h"
#import "chrome/browser/ui/cocoa/background_gradient_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_bridge.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_view_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button_cell.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_context_menu_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_model_observer_for_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_name_folder_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/menu_button.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#import "chrome/browser/ui/cocoa/view_resizer.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

using base::UserMetricsAction;
using bookmarks::BookmarkBarLayout;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::BookmarkNodeData;
using content::OpenURLParams;
using content::Referrer;
using content::WebContents;

// Bookmark bar state changing and animations
//
// The bookmark bar has three real states: "showing" (a normal bar attached to
// the toolbar), "hidden", and "detached" (pretending to be part of the web
// content on the NTP). It can, or at least should be able to, animate between
// these states. There are several complications even without animation:
//  - The placement of the bookmark bar is done by the BWC, and it needs to know
//    the state in order to place the bookmark bar correctly (immediately below
//    the toolbar when showing, below the infobar when detached).
//  - The "divider" (a black line) needs to be drawn by either the toolbar (when
//    the bookmark bar is hidden or detached) or by the bookmark bar (when it is
//    showing). It should not be drawn by both.
//  - The toolbar needs to vertically "compress" when the bookmark bar is
//    showing. This ensures the proper display of both the bookmark bar and the
//    toolbar, and gives a padded area around the bookmark bar items for right
//    clicks, etc.
//
// Our model is that the BWC controls us and also the toolbar. We try not to
// talk to the browser nor the toolbar directly, instead centralizing control in
// the BWC. The key method by which the BWC controls us is
// |-updateState:ChangeType:|. This invokes state changes, and at appropriate
// times we request that the BWC do things for us via either the resize delegate
// or our general delegate. If the BWC needs any information about what it
// should do, or tell the toolbar to do, it can then query us back (e.g.,
// |-isShownAs...|, |-getDesiredToolbarHeightCompression|,
// |-toolbarDividerOpacity|, etc.).
//
// Animation-related complications:
//  - Compression of the toolbar is touchy during animation. It must not be
//    compressed while the bookmark bar is animating to/from showing (from/to
//    hidden), otherwise it would look like the bookmark bar's contents are
//    sliding out of the controls inside the toolbar. As such, we have to make
//    sure that the bookmark bar is shown at the right location and at the
//    right height (at various points in time).
//  - Showing the divider is also complicated during animation between hidden
//    and showing. We have to make sure that the toolbar does not show the
//    divider despite the fact that it's not compressed. The exception to this
//    is at the beginning/end of the animation when the toolbar is still
//    uncompressed but the bookmark bar has height 0. If we're not careful, we
//    get a flicker at this point.
//  - We have to ensure that we do the right thing if we're told to change state
//    while we're running an animation. The generic/easy thing to do is to jump
//    to the end state of our current animation, and (if the new state change
//    again involves an animation) begin the new animation. We can do better
//    than that, however, and sometimes just change the current animation to go
//    to the new end state (e.g., by "reversing" the animation in the showing ->
//    hidden -> showing case). We also have to ensure that demands to
//    immediately change state are always honoured.
//
// Pointers to animation logic:
//  - |-moveToState:withAnimation:| starts animations, deciding which ones we
//    know how to handle.
//  - |-doBookmarkBarAnimation| has most of the actual logic.
//  - |-getDesiredToolbarHeightCompression| and |-toolbarDividerOpacity| contain
//    related logic.
//  - The BWC's |-layoutSubviews| needs to know how to position things.
//  - The BWC should implement |-bookmarkBar:didChangeFromState:toState:| and
//    |-bookmarkBar:willAnimateFromState:toState:| in order to inform the
//    toolbar of required changes.
//
// Layout:
//
// Several events (initial load, changes to the bookmark model etc.) can
// require the bar layout to change. In most cases, this is accomplished
// by building a BookmarkBarLayout from the current state of the view,
// the bookmark model, and the managed bookmark service. If the calculated
// layout differs from the previous one, it's applied to the view
// via |applyLayout:animated:|. This is a cheap way to "coalesce" multiple
// potentially layout-changing events, since in practice, these events come
// in bursts and don't require a change.
//
// Temporary changes in layout during dragging are an exception to this,
// since the layout temporarily adjusts to the drag (for example, adding
// a placeholder space for a mid-drag button or removing the space previously
// taken up by a button which is being dragged off the bar). In this case,
// the original stored layout is maintained as the source of truth, and
// elements are laid out from a combination of the stored layout and the
// drag state. See |setDropInsertionPos:| for details.

namespace {

// Duration of the bookmark bar animations.
const NSTimeInterval kBookmarkBarAnimationDuration = 0.12;
const NSTimeInterval kDragAndDropAnimationDuration = 0.25;

const int kMaxReusePoolSize = 10;

// Min width for no item text field and import bookmarks button.
const CGFloat kNoItemElementMinWidth = 30;

void RecordAppLaunch(Profile* profile, GURL url) {
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)->
          enabled_extensions().GetAppByURL(url);
  if (!extension)
    return;

  extensions::RecordAppLaunchType(extension_misc::APP_LAUNCH_BOOKMARK_BAR,
                                  extension->GetType());
}

CGFloat GetBookmarkButtonHeightMinusPadding() {
  return GetCocoaLayoutConstant(BOOKMARK_BAR_HEIGHT) -
         bookmarks::kBookmarkVerticalPadding * 2;
}

}  // namespace

namespace bookmarks {

BookmarkBarLayout::BookmarkBarLayout()
    : visible_elements(0),
      apps_button_offset(0),
      managed_bookmarks_button_offset(0),
      off_the_side_button_offset(0),
      other_bookmarks_button_offset(0),
      no_item_textfield_offset(0),
      no_item_textfield_width(0),
      import_bookmarks_button_offset(0),
      import_bookmarks_button_width(0),
      max_x(0){};
BookmarkBarLayout::~BookmarkBarLayout(){};
BookmarkBarLayout::BookmarkBarLayout(BookmarkBarLayout&& other) = default;
BookmarkBarLayout& BookmarkBarLayout::operator=(BookmarkBarLayout&& other) =
    default;

bool operator==(const BookmarkBarLayout& lhs, const BookmarkBarLayout& rhs) {
  return std::tie(lhs.visible_elements, lhs.apps_button_offset,
                  lhs.managed_bookmarks_button_offset,
                  lhs.off_the_side_button_offset,
                  lhs.other_bookmarks_button_offset,
                  lhs.no_item_textfield_offset, lhs.no_item_textfield_width,
                  lhs.import_bookmarks_button_offset,
                  lhs.import_bookmarks_button_width, lhs.button_offsets,
                  lhs.max_x) ==
         std::tie(
             rhs.visible_elements, rhs.apps_button_offset,
             rhs.managed_bookmarks_button_offset,
             rhs.off_the_side_button_offset, rhs.other_bookmarks_button_offset,
             rhs.no_item_textfield_offset, rhs.no_item_textfield_width,
             rhs.import_bookmarks_button_offset,
             rhs.import_bookmarks_button_width, rhs.button_offsets, rhs.max_x);
}

bool operator!=(const BookmarkBarLayout& lhs, const BookmarkBarLayout& rhs) {
  return !(lhs == rhs);
}

}  // namespace bookmarks

@implementation BookmarkBarController {
  BookmarkBarLayout layout_;
  CGFloat originalNoItemTextFieldWidth_;
  CGFloat originalImportBookmarksButtonWidth_;
  CGFloat originalNoItemInterelementPadding_;
  BOOL didCreateExtraButtons_;

  // Maps bookmark node IDs to instantiated buttons for ease of lookup.
  std::unordered_map<int64_t, base::scoped_nsobject<BookmarkButton>>
      nodeIdToButtonMap_;

  // A place to stash bookmark buttons that have been removed from the bar
  // so that they can be reused instead of creating new ones.
  base::scoped_nsobject<NSMutableArray> unusedButtonPool_;
}

@synthesize currentState = currentState_;
@synthesize lastState = lastState_;
@synthesize isAnimationRunning = isAnimationRunning_;
@synthesize delegate = delegate_;
@synthesize stateAnimationsEnabled = stateAnimationsEnabled_;
@synthesize innerContentAnimationsEnabled = innerContentAnimationsEnabled_;

- (id)initWithBrowser:(Browser*)browser
         initialWidth:(CGFloat)initialWidth
             delegate:(id<BookmarkBarControllerDelegate>)delegate {
  if ((self = [super initWithNibName:nil bundle:nil])) {
    currentState_ = BookmarkBar::HIDDEN;
    lastState_ = BookmarkBar::HIDDEN;

    browser_ = browser;
    initialWidth_ = initialWidth;
    bookmarkModel_ =
        BookmarkModelFactory::GetForBrowserContext(browser_->profile());
    managedBookmarkService_ =
        ManagedBookmarkServiceFactory::GetForProfile(browser_->profile());
    buttons_.reset([[NSMutableArray alloc] init]);
    unusedButtonPool_.reset([[NSMutableArray alloc] init]);
    delegate_ = delegate;
    folderTarget_.reset(
        [[BookmarkFolderTarget alloc] initWithController:self
                                                 profile:browser_->profile()]);
    didCreateExtraButtons_ = NO;

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    folderImage_.reset(
        rb.GetNativeImageNamed(IDR_BOOKMARK_BAR_FOLDER).CopyNSImage());
    folderImageWhite_.reset(
        rb.GetNativeImageNamed(IDR_BOOKMARK_BAR_FOLDER_WHITE).CopyNSImage());

    const int kIconSize = 16;
    defaultImage_.reset([NSImageFromImageSkia(gfx::CreateVectorIcon(
        omnibox::kHttpIcon, kIconSize, gfx::kChromeIconGrey)) retain]);
    defaultImageIncognito_.reset([NSImageFromImageSkia(gfx::CreateVectorIcon(
        omnibox::kHttpIcon, kIconSize, SkColorSetA(SK_ColorWHITE, 0xCC)))
        retain]);

    innerContentAnimationsEnabled_ = YES;
    stateAnimationsEnabled_ = YES;

    // Register for theme changes, bookmark button pulsing, ...
    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(themeDidChangeNotification:)
                          name:kBrowserThemeDidChangeNotification
                        object:nil];

    contextMenuController_.reset(
        [[BookmarkContextMenuCocoaController alloc]
            initWithBookmarkBarController:self]);
  }
  return self;
}

- (Browser*)browser {
  return browser_;
}

- (BookmarkBarToolbarView*)controlledView {
  return base::mac::ObjCCastStrict<BookmarkBarToolbarView>([self view]);
}

- (BookmarkContextMenuCocoaController*)menuController {
  return contextMenuController_.get();
}

- (void)loadView {
  // Height is 0 because this is what the superview expects
  [self setView:[[[BookmarkBarToolbarView alloc]
                    initWithFrame:NSMakeRect(0, 0, initialWidth_, 0)]
                    autorelease]];
  [[self view] setHidden:YES];
  [[self view] setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  [[self controlledView] setController:self];
  [[self controlledView] setDelegate:self];

  buttonView_.reset([[BookmarkBarView alloc]
      initWithController:self
                   frame:NSMakeRect(0, -2, 584, 144)]);
  [buttonView_ setAutoresizingMask:NSViewWidthSizable | NSViewMaxXMargin];
  [[buttonView_ importBookmarksButton] setTarget:self];
  [[buttonView_ importBookmarksButton] setAction:@selector(importBookmarks:)];

  [self.view addSubview:buttonView_];

  // viewDidLoad became part of the API in 10.10.
  if (!base::mac::IsAtLeastOS10_10())
    [self viewDidLoadImpl];
}

- (BookmarkButton*)findAncestorButtonOnBarForNode:(const BookmarkNode*)node {
  // Find the closest parent that is visible on the bar.
  while (node) {
    // Check if we've reached one of the special buttons. Otherwise, if the next
    // parent is the boomark bar, find the corresponding button.
    if ([managedBookmarksButton_ bookmarkNode] == node)
      return managedBookmarksButton_;

    if ([otherBookmarksButton_ bookmarkNode] == node)
      return otherBookmarksButton_;

    if ([offTheSideButton_ bookmarkNode] == node)
      return offTheSideButton_;

    if (node->parent() == bookmarkModel_->bookmark_bar_node()) {
      for (BookmarkButton* button in [self buttons]) {
        if ([button bookmarkNode] == node) {
          [button setPulseIsStuckOn:YES];
          return button;
        }
      }
    }

    node = node->parent();
  }
  NOTREACHED();
  return nil;
}

- (void)startPulsingBookmarkNode:(const BookmarkNode*)node {
  [self stopPulsingBookmarkNode];

  pulsingButton_.reset([self findAncestorButtonOnBarForNode:node],
                       base::scoped_policy::RETAIN);
  if (!pulsingButton_)
    return;

  [pulsingButton_ setPulseIsStuckOn:YES];
  pulsingBookmarkObserver_.reset(
      new BookmarkModelObserverForCocoa(bookmarkModel_, ^{
        // Stop pulsing if anything happened to the node.
        [self stopPulsingBookmarkNode];
      }));
  pulsingBookmarkObserver_->StartObservingNode(node);
}

- (void)stopPulsingBookmarkNode {
  if (!pulsingButton_)
    return;

  [pulsingButton_ setPulseIsStuckOn:NO];
  pulsingButton_.reset();
  pulsingBookmarkObserver_.reset();
}

- (void)dealloc {
  [buttonView_ setController:nil];
  [[self controlledView] setController:nil];
  [[self controlledView] setDelegate:nil];
  [self browserWillBeDestroyed];
  [super dealloc];
}

- (void)browserWillBeDestroyed {
  // If |bridge_| is null it means -viewDidLoad has not yet been called, which
  // can only happen if the nib wasn't loaded. Retrieving it via -[self view]
  // would load it now, but it's too late for that, so let it be nil. Note this
  // should only happen in tests.
  BookmarkBarToolbarView* view = nil;
  if (bridge_)
    view = [self controlledView];

  // Clear delegate so it doesn't get called during stopAnimation.
  [view setResizeDelegate:nil];

  // We better stop any in-flight animation if we're being killed.
  [view stopAnimation];

  // Remove our view from its superview so it doesn't attempt to reference
  // it when the controller is gone.
  //TODO(dmaclach): Remove -- http://crbug.com/25845
  [view removeFromSuperview];

  // Be sure there is no dangling pointer.
  if ([view respondsToSelector:@selector(setController:)])
    [view performSelector:@selector(setController:) withObject:nil];

  // For safety, make sure the buttons can no longer call us.
  base::scoped_nsobject<NSMutableArray> buttons([buttons_ mutableCopy]);
  [buttons addObjectsFromArray:unusedButtonPool_];
  if (didCreateExtraButtons_) {
    [buttons addObjectsFromArray:@[
      appsPageShortcutButton_, managedBookmarksButton_, otherBookmarksButton_,
      offTheSideButton_
    ]];
  }

  for (BookmarkButton* button in buttons.get()) {
    [button setDelegate:nil];
    [button setTarget:nil];
    [button setAction:nil];
  }
  bridge_.reset(NULL);
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self watchForExitEvent:NO];
  browser_ = nullptr;
}

- (void)viewDidLoad {
  // This indirection allows the viewDidLoad implementation to be called from
  // elsewhere without triggering an availability warning.
  [self viewDidLoadImpl];
}

- (void)viewDidLoadImpl {
  // We are enabled by default.
  barIsEnabled_ = YES;

  // Remember the original sizes of the 'no items' and 'import bookmarks'
  // fields to aid in resizing when the window frame changes.
  NSRect noItemTextFieldFrame = [[buttonView_ noItemTextField] frame];
  NSRect noItemButtonFrame = [[buttonView_ importBookmarksButton] frame];
  originalNoItemTextFieldWidth_ = NSWidth(noItemTextFieldFrame);
  originalImportBookmarksButtonWidth_ = NSWidth(noItemButtonFrame);
  originalNoItemInterelementPadding_ =
      NSMinX(noItemButtonFrame) - NSMaxX(noItemTextFieldFrame);

  [[self view] setPostsFrameChangedNotifications:YES];
  [[NSNotificationCenter defaultCenter]
    addObserver:self
       selector:@selector(frameDidChange)
           name:NSViewFrameDidChangeNotification
         object:[self view]];

  // Watch for things going to or from fullscreen.
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(willEnterOrLeaveFullscreen:)
             name:NSWindowWillEnterFullScreenNotification
           object:nil];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(willEnterOrLeaveFullscreen:)
             name:NSWindowWillExitFullScreenNotification
           object:nil];
  [self layoutSubviews];

  // Don't pass ourself along (as 'self') until our init is completely
  // done.  Thus, this call is (almost) last.
  bridge_.reset(new BookmarkBarBridge(browser_->profile(), self,
                                      bookmarkModel_));
}

// Called by our main view (a BookmarkBarView) when it gets moved to a
// window.  We perform operations which need to know the relevant
// window (e.g. watch for a window close) so they can't be performed
// earlier (such as in awakeFromNib).
- (void)viewDidMoveToWindow {
  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];

  // Remove any existing notifications before registering for new ones.
  [defaultCenter removeObserver:self
                           name:NSWindowWillCloseNotification
                         object:nil];
  [defaultCenter removeObserver:self
                           name:NSWindowDidResignMainNotification
                         object:nil];

  [defaultCenter addObserver:self
                    selector:@selector(parentWindowWillClose:)
                        name:NSWindowWillCloseNotification
                      object:[[self view] window]];
  [defaultCenter addObserver:self
                    selector:@selector(parentWindowDidResignMain:)
                        name:NSWindowDidResignMainNotification
                      object:[[self view] window]];
}

// When going fullscreen we can run into trouble.  Our view is removed
// from the non-fullscreen window before the non-fullscreen window
// loses key, so our parentDidResignKey: callback never gets called.
// In addition, a bookmark folder controller needs to be autoreleased
// (in case it's in the event chain when closed), but the release
// implicitly needs to happen while it's connected to the original
// (non-fullscreen) window to "unlock bar visibility".  Such a
// contract isn't honored when going fullscreen with the menu option
// (not with the keyboard shortcut).  We fake it as best we can here.
// We have a similar problem leaving fullscreen.
- (void)willEnterOrLeaveFullscreen:(NSNotification*)notification {
  if (folderController_) {
    [self childFolderWillClose:folderController_];
    [self closeFolderAndStopTrackingMenus];
  }
}

// NSNotificationCenter callback.
- (void)parentWindowWillClose:(NSNotification*)notification {
  [self closeFolderAndStopTrackingMenus];
}

// NSNotificationCenter callback.
- (void)parentWindowDidResignMain:(NSNotification*)notification {
  [self closeFolderAndStopTrackingMenus];
}

- (void)layoutToFrame:(NSRect)frame {
  // The view should be pinned to the top of the window with a flexible width.
  DCHECK_EQ(NSViewWidthSizable | NSViewMinYMargin,
            [[self view] autoresizingMask]);
  [[self view] setFrame:frame];
  [self layoutSubviews];
  [self frameDidChange];
}

// Change the layout of the bookmark bar's subviews in response to a visibility
// change (e.g., show or hide the bar) or style change (attached or floating).
- (void)layoutSubviews {
  NSRect frame = [[self view] frame];
  NSRect buttonViewFrame = NSMakeRect(0, 0, NSWidth(frame), NSHeight(frame));

  // Add padding to the detached bookmark bar.
  // The state of our morph (if any); 1 is total bubble, 0 is the regular bar.
  CGFloat morph = [self detachedMorphProgress];
  CGFloat padding = 0;
  padding = GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_PADDING);
  buttonViewFrame =
      NSInsetRect(buttonViewFrame, morph * padding, morph * padding);
  [buttonView_ setFrame:buttonViewFrame];

  // Update bookmark button backgrounds.
  if ([self isAnimationRunning]) {
    for (NSButton* button in buttons_.get())
      [button setNeedsDisplay:YES];
    // Update the apps and other buttons explicitly, since they are not in the
    // buttons_ array.
    [appsPageShortcutButton_ setNeedsDisplay:YES];
    [managedBookmarksButton_ setNeedsDisplay:YES];
    [otherBookmarksButton_ setNeedsDisplay:YES];
  }
}

// We don't change a preference; we only change visibility. Preference changing
// (global state) is handled in |chrome::ToggleBookmarkBarWhenVisible()|. We
// simply update based on what we're told.
- (void)updateVisibility {
  [self showBookmarkBarWithAnimation:NO];
}

- (void)updateHiddenState {
  BOOL oldHidden = [[self view] isHidden];
  BOOL newHidden = ![self isVisible];
  if (oldHidden != newHidden)
    [[self view] setHidden:newHidden];
}

- (void)setBookmarkBarEnabled:(BOOL)enabled {
  if (enabled != barIsEnabled_) {
    barIsEnabled_ = enabled;
    [self updateVisibility];
  }
}

- (CGFloat)getDesiredToolbarHeightCompression {
  // Some special cases....
  if (!barIsEnabled_)
    return 0;

  if ([self isAnimationRunning]) {
    // No toolbar compression when animating between hidden and showing, nor
    // between showing and detached.
    if ([self isAnimatingBetweenState:BookmarkBar::HIDDEN
                             andState:BookmarkBar::SHOW] ||
        [self isAnimatingBetweenState:BookmarkBar::SHOW
                             andState:BookmarkBar::DETACHED])
      return 0;

    // If we ever need any other animation cases, code would go here.
  }

  return [self isInState:BookmarkBar::SHOW] ? bookmarks::kBookmarkBarOverlap
                                            : 0;
}

- (CGFloat)toolbarDividerOpacity {
  // Some special cases....
  if ([self isAnimationRunning]) {
    // In general, the toolbar shouldn't show a divider while we're animating
    // between showing and hidden. The exception is when our height is < 1, in
    // which case we can't draw it. It's all-or-nothing (no partial opacity).
    if ([self isAnimatingBetweenState:BookmarkBar::HIDDEN
                             andState:BookmarkBar::SHOW])
      return (NSHeight([[self view] frame]) < 1) ? 1 : 0;

    // The toolbar should show the divider when animating between showing and
    // detached (but opacity will vary).
    if ([self isAnimatingBetweenState:BookmarkBar::SHOW
                             andState:BookmarkBar::DETACHED])
      return static_cast<CGFloat>([self detachedMorphProgress]);

    // If we ever need any other animation cases, code would go here.
  }

  // In general, only show the divider when it's in the normal showing state.
  return [self isInState:BookmarkBar::SHOW] ? 0 : 1;
}

- (NSImage*)faviconForNode:(const BookmarkNode*)node
             forADarkTheme:(BOOL)forADarkTheme {
  if (!node)
    return forADarkTheme ? defaultImageIncognito_ : defaultImage_;

  if (forADarkTheme) {
    if (node == managedBookmarkService_->managed_node()) {
      ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      return rb.GetNativeImageNamed(
          IDR_BOOKMARK_BAR_FOLDER_MANAGED_WHITE).ToNSImage();
    }

    if (node->is_folder())
      return folderImageWhite_;
  } else {
    if (node == managedBookmarkService_->managed_node()) {
      // Most users never see this node, so the image is only loaded if needed.
      ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      return rb.GetNativeImageNamed(
          IDR_BOOKMARK_BAR_FOLDER_MANAGED).ToNSImage();
    }

    if (node->is_folder())
      return folderImage_;
  }

  const gfx::Image& favicon = bookmarkModel_->GetFavicon(node);
  if (!favicon.IsEmpty())
    return favicon.ToNSImage();

  return forADarkTheme ? defaultImageIncognito_ : defaultImage_;
}

- (void)closeFolderAndStopTrackingMenus {
  showFolderMenus_ = NO;
  [self closeAllBookmarkFolders];
}

- (BOOL)canEditBookmarks {
  PrefService* prefs = browser_->profile()->GetPrefs();
  return prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled);
}

- (BOOL)canEditBookmark:(const BookmarkNode*)node {
  // Don't allow edit/delete of the permanent nodes.
  if (node == nil || bookmarkModel_->is_permanent_node(node) ||
      !managedBookmarkService_->CanBeEditedByUser(node)) {
    return NO;
  }
  return YES;
}

#pragma mark Actions

// Helper methods called on the main thread by runMenuFlashThread.

- (void)setButtonFlashStateOn:(id)sender {
  [sender highlight:YES];
}

- (void)setButtonFlashStateOffAndCleanUp:(id)sender {
  [sender highlight:NO];
  [self closeFolderAndStopTrackingMenus];

  // Release the items retained by doMenuFlashOnSeparateThread.
  [sender release];
  [self release];
}

// This call is invoked only by doMenuFlashOnSeparateThread below.
// It makes the selected BookmarkButton (which is masquerading as a menu item)
// flash once to give confirmation feedback, then it closes the menu.
// It spends all its time sleeping or scheduling UI work on the main thread.
- (void)runMenuFlashThread:(id)sender {

  // Check this is not running on the main thread, as it sleeps.
  DCHECK(![NSThread isMainThread]);

  // Duration of flash when the item is clicked on.
  const float kBBFlashTime = 0.08;

  [self performSelectorOnMainThread:@selector(setButtonFlashStateOn:)
                         withObject:sender
                      waitUntilDone:NO];
  [NSThread sleepForTimeInterval:kBBFlashTime];

  [self performSelectorOnMainThread:@selector(setButtonFlashStateOffAndCleanUp:)
                         withObject:sender
                      waitUntilDone:NO];
}

// Non-blocking call which starts the process to make the selected menu item
// flash a few times to give confirmation feedback, after which it closes the
// menu. The item is of course actually a BookmarkButton masquerading as a menu
// item).
- (void)doMenuFlashOnSeparateThread:(id)sender {

  // Ensure that self and sender don't go away before the animation completes.
  // These retains are balanced in cleanupAfterMenuFlashThread above.
  [self retain];
  [sender retain];
  [NSThread detachNewThreadSelector:@selector(runMenuFlashThread:)
                           toTarget:self
                         withObject:sender];
}

- (IBAction)openBookmark:(id)sender {
  BOOL isMenuItem = [[sender cell] isFolderButtonCell];
  BOOL animate = isMenuItem && innerContentAnimationsEnabled_;
  if (animate)
    [self doMenuFlashOnSeparateThread:sender];
  DCHECK([sender respondsToSelector:@selector(bookmarkNode)]);
  const BookmarkNode* node = [sender bookmarkNode];
  DCHECK(node);
  WindowOpenDisposition disposition =
      ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  RecordAppLaunch(browser_->profile(), node->url());
  [self openURL:node->url() disposition:disposition];

  if (!animate)
    [self closeFolderAndStopTrackingMenus];
  RecordBookmarkLaunch(node, [self bookmarkLaunchLocation]);
}

// Common function to open a bookmark folder of any type.
- (void)openBookmarkFolder:(id)sender {
  DCHECK([sender isKindOfClass:[BookmarkButton class]]);
  DCHECK([[sender cell] isKindOfClass:[BookmarkButtonCell class]]);

  // Only record the action if it's the initial folder being opened.
  if (!showFolderMenus_)
    RecordBookmarkFolderOpen([self bookmarkLaunchLocation]);
  showFolderMenus_ = !showFolderMenus_;

  // Middle click on chevron should not open bookmarks under it, instead just
  // open its folder menu.
  if (sender == offTheSideButton_.get()) {
    [[sender cell] setStartingChildIndex:layout_.VisibleButtonCount()];
    NSEvent* event = [NSApp currentEvent];
    if ([event type] == NSOtherMouseUp) {
      [self openOrCloseBookmarkFolderForOffTheSideButton];
      return;
    }
  }
  // Toggle presentation of bar folder menus.
  [folderTarget_ openBookmarkFolderFromButton:sender];
}

- (void)openOrCloseBookmarkFolderForOffTheSideButton {
  // If clicked on already opened folder, then close it and return.
  if ([folderController_ parentButton] == offTheSideButton_)
    [self closeBookmarkFolder:self];
  else
    [self addNewFolderControllerWithParentButton:offTheSideButton_];
}

// Click on a bookmark folder button.
- (void)openBookmarkFolderFromButton:(id)sender {
  [self openBookmarkFolder:sender];
}

// Click on the "off the side" button (chevron), which opens like a folder
// button but isn't exactly a parent folder.
- (void)openOffTheSideFolderFromButton:(id)sender {
  [self openBookmarkFolder:sender];
}

- (void)importBookmarks:(id)sender {
  chrome::ShowImportDialog(browser_);
}

- (NSButton*)appsPageShortcutButton {
  return appsPageShortcutButton_;
}

- (NSButton*)offTheSideButton {
  return offTheSideButton_;
}

- (NSImage*)offTheSideButtonImage:(BOOL)forDarkMode {
  const int kIconSize = 8;
  SkColor vectorIconColor = forDarkMode ? SkColorSetA(SK_ColorWHITE, 0xCC)
                                        : gfx::kChromeIconGrey;
  NSImage* image = NSImageFromImageSkia(
      gfx::CreateVectorIcon(kOverflowChevronIcon, kIconSize, vectorIconColor));
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    return cocoa_l10n_util::FlippedImage(image);
  else
    return image;
}

#pragma mark Private Methods

// Called after a theme change took place, possibly for a different profile.
- (void)themeDidChangeNotification:(NSNotification*)notification {
  [self updateTheme:[[[self view] window] themeProvider]];
}

- (BookmarkLaunchLocation)bookmarkLaunchLocation {
  return currentState_ == BookmarkBar::DETACHED ?
      BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR :
      BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR;
}

// Main menubar observation code, so we can know to close our fake menus if the
// user clicks on the actual menubar, as multiple unconnected menus sharing
// the screen looks weird.
// Needed because the local event monitor doesn't see the click on the menubar.

// Gets called when the menubar is clicked.
- (void)begunTracking:(NSNotification *)notification {
  [self closeFolderAndStopTrackingMenus];
}

// Install the callback.
- (void)startObservingMenubar {
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self
         selector:@selector(begunTracking:)
             name:NSMenuDidBeginTrackingNotification
           object:[NSApp mainMenu]];
}

// Remove the callback.
- (void)stopObservingMenubar {
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:self
                name:NSMenuDidBeginTrackingNotification
              object:[NSApp mainMenu]];
}

// End of menubar observation code.

// Begin (or end) watching for a click outside this window.  Unlike
// normal NSWindows, bookmark folder "fake menu" windows do not become
// key or main.  Thus, traditional notification (e.g. WillResignKey)
// won't work.  Our strategy is to watch (at the app level) for a
// "click outside" these windows to detect when they logically lose
// focus.
- (void)watchForExitEvent:(BOOL)watch {
  if (watch) {
    if (!exitEventTap_) {
      exitEventTap_ = [NSEvent
          addLocalMonitorForEventsMatchingMask:NSAnyEventMask
          handler:^NSEvent* (NSEvent* event) {
              if ([self isEventAnExitEvent:event])
                [self closeFolderAndStopTrackingMenus];
              return event;
          }];
      [self startObservingMenubar];
    }
  } else {
    if (exitEventTap_) {
      [NSEvent removeMonitor:exitEventTap_];
      exitEventTap_ = nil;
      [self stopObservingMenubar];
    }
  }
}

// Show/hide the bookmark bar.
// If |animate| is YES, the changes are made using the animator; otherwise they
// are made immediately.
- (void)showBookmarkBarWithAnimation:(BOOL)animate {
  if (animate && stateAnimationsEnabled_) {
    // If |-doBookmarkBarAnimation| does the animation, we're done.
    if ([self doBookmarkBarAnimation])
      return;

    // Else fall through and do the change instantly.
  }

  BookmarkBarToolbarView* view = [self controlledView];

  // Set our height immediately via -[AnimatableView setHeight:].
  [view setHeight:[self preferredHeight]];

  // Only show the divider if showing the normal bookmark bar.
  BOOL showsDivider = [self isInState:BookmarkBar::SHOW];
  [view setShowsDivider:showsDivider];

  // Make sure we're shown.
  [view setHidden:![self isVisible]];

  // Update everything else.
  [self layoutSubviews];
  [self frameDidChange];
}

// Handles animating the resize of the content view. Returns YES if it handled
// the animation, NO if not (and hence it should be done instantly).
- (BOOL)doBookmarkBarAnimation {
  BookmarkBarToolbarView* view = [self controlledView];
  if ([self isAnimatingFromState:BookmarkBar::HIDDEN
                         toState:BookmarkBar::SHOW]) {
    [view setShowsDivider:YES];
    [view setHidden:NO];
    // Height takes into account the extra height we have since the toolbar
    // only compresses when we're done.
    [view animateToNewHeight:(GetCocoaLayoutConstant(
                                  BOOKMARK_BAR_HEIGHT_NO_OVERLAP) -
                              bookmarks::kBookmarkBarOverlap)
                    duration:kBookmarkBarAnimationDuration];
  } else if ([self isAnimatingFromState:BookmarkBar::SHOW
                                toState:BookmarkBar::HIDDEN]) {
    [view setShowsDivider:YES];
    [view setHidden:NO];
    [view animateToNewHeight:0
                    duration:kBookmarkBarAnimationDuration];
  } else if ([self isAnimatingFromState:BookmarkBar::SHOW
                                toState:BookmarkBar::DETACHED]) {
    [view setShowsDivider:YES];
    [view setHidden:NO];
    [view animateToNewHeight:GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_HEIGHT)
                    duration:kBookmarkBarAnimationDuration];
  } else if ([self isAnimatingFromState:BookmarkBar::DETACHED
                                toState:BookmarkBar::SHOW]) {
    [view setShowsDivider:YES];
    [view setHidden:NO];
    // Height takes into account the extra height we have since the toolbar
    // only compresses when we're done.
    [view animateToNewHeight:(GetCocoaLayoutConstant(
                                  BOOKMARK_BAR_HEIGHT_NO_OVERLAP) -
                              bookmarks::kBookmarkBarOverlap)
                    duration:kBookmarkBarAnimationDuration];
  } else {
    // Oops! An animation we don't know how to handle.
    return NO;
  }

  return YES;
}

// Actually open the URL.  This is the last chance for a unit test to
// override.
- (void)openURL:(GURL)url disposition:(WindowOpenDisposition)disposition {
  OpenURLParams params(
      url, Referrer(), disposition, ui::PAGE_TRANSITION_AUTO_BOOKMARK,
      false);
  browser_->OpenURL(params);
}

- (int)preferredHeight {
  DCHECK(![self isAnimationRunning]);

  if (!barIsEnabled_)
    return 0;

  switch (currentState_) {
    case BookmarkBar::SHOW:
      return GetCocoaLayoutConstant(BOOKMARK_BAR_HEIGHT_NO_OVERLAP);
    case BookmarkBar::DETACHED:
      return GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_HEIGHT);
    case BookmarkBar::HIDDEN:
      return 0;
  }
}

// Return an appropriate width for the given bookmark button cell.
- (CGFloat)widthForBookmarkButtonCell:(NSCell*)cell {
  CGFloat width =
      [cell cellSize].width + [BookmarkButtonCell insetInView:buttonView_] * 2;
  return std::min(width, bookmarks::kDefaultBookmarkWidth);
}

- (BookmarkButton*)buttonForNode:(const BookmarkNode*)node {
  BookmarkButton* button = nil;
  int64_t nodeId = node->id();
  auto buttonIt = nodeIdToButtonMap_.find(nodeId);
  if (buttonIt != nodeIdToButtonMap_.end()) {
    button = (*buttonIt).second.get();
    [self updateTitleAndTooltipForButton:button];
  } else if ([unusedButtonPool_ count] > 0) {
    button = [[[unusedButtonPool_ firstObject] retain] autorelease];
    [unusedButtonPool_ removeObjectAtIndex:0];
    BOOL darkTheme = [[[self view] window] hasDarkTheme];
    [[button cell]
        setBookmarkNode:node
                  image:[self faviconForNode:node forADarkTheme:darkTheme]];
  } else {
    BookmarkButtonCell* cell = [self cellForBookmarkNode:node];
    NSRect frame = NSMakeRect(0, bookmarks::kBookmarkVerticalPadding, 0,
                              GetBookmarkButtonHeightMinusPadding());
    button = [[[BookmarkButton alloc] initWithFrame:frame] autorelease];
    [button setCell:cell];
    [buttonView_ addSubview:button];
    [button setDelegate:self];
  }
  DCHECK(button);

  // Do setup.
  nodeIdToButtonMap_.insert(
      {nodeId, base::scoped_nsobject<BookmarkButton>([button retain])});
  if (node->is_folder()) {
    [button setTarget:self];
    [button setAction:@selector(openBookmarkFolderFromButton:)];
    [[button draggableButton] setActsOnMouseDown:YES];
  } else {
    // Make the button do something.
    [button setTarget:self];
    [button setAction:@selector(openBookmark:)];
    [[button draggableButton] setActsOnMouseDown:NO];
  }
  [self updateTitleAndTooltipForButton:button];
  return button;
}

// Adds |button| to the reuse pool. It remains a child of the bookmark
// bar to avoid the cost of readding it on reuse.
- (void)prepareButtonForReuse:(BookmarkButton*)button {
  // Dragged buttons unhide themselves, so position it off-screen
  // while it's in the reuse pool.
  CGRect buttonFrame = [button frame];
  buttonFrame.origin.x = -10000;
  [button setFrame:buttonFrame];

  // These buttons are still children of the bar view, so they're
  // subject to certain operations (theme update, for example) that
  // apply to all subviews. Ensure this doesn't try to reference
  // a stale node.
  auto* cell = base::mac::ObjCCastStrict<BookmarkButtonCell>([button cell]);
  [cell setBookmarkNode:nullptr];

  [unusedButtonPool_ addObject:button];
}

- (void)updateTitleAndTooltipForButton:(BookmarkButton*)button {
  const BookmarkNode* node = [button bookmarkNode];
  CGFloat buttonWidth = [self widthOfButtonForNode:node];
  NSString* buttonTitle = base::SysUTF16ToNSString(node->GetTitle());

  if (NSWidth([button frame]) == buttonWidth &&
      [[button title] isEqualToString:buttonTitle])
    return;

  CGRect frame = [button frame];
  frame.size.width = buttonWidth;
  [button setFrame:frame];
  [[button cell] setTitle:buttonTitle];
  NSString* tooltip = nil;

  // Folders show a tooltip iff the title is truncated.
  if (node->is_folder() && [buttonTitle length] > 0 &&
      [self widthForBookmarkButtonCell:[button cell]] < buttonWidth) {
    tooltip = buttonTitle;
  } else if (node->is_url()) {
    tooltip = [BookmarkMenuCocoaController tooltipForNode:node];
  }

  [button setToolTip:tooltip];
}

// Creates a bookmark bar button that does not correspond to a regular bookmark
// or folder. It is used by the "Other Bookmarks" and the "Apps" buttons.
- (BookmarkButton*)createCustomBookmarkButtonForCell:(NSCell*)cell {
  BookmarkButton* button = [[BookmarkButton alloc] init];
  [[button draggableButton] setDraggable:NO];
  [[button draggableButton] setActsOnMouseDown:YES];
  [button setCell:cell];
  [button setDelegate:self];
  [button setTarget:self];
  // Make sure this button, like all other BookmarkButtons, lives
  // until the end of the current event loop.
  [[button retain] autorelease];
  return button;
}

// Creates the button for "Managed Bookmarks", but does not position it.
- (void)createManagedBookmarksButton {
  if (managedBookmarksButton_.get()) {
    // The node's title might have changed if the user signed in or out.
    // Make sure it's up to date now.
    const BookmarkNode* node = managedBookmarkService_->managed_node();
    NSString* title = base::SysUTF16ToNSString(node->GetTitle());
    NSCell* cell = [managedBookmarksButton_ cell];
    [cell setTitle:title];

    return;
  }

  NSCell* cell =
      [self cellForBookmarkNode:managedBookmarkService_->managed_node()];
  managedBookmarksButton_.reset([self createCustomBookmarkButtonForCell:cell]);
  [managedBookmarksButton_ setAction:@selector(openBookmarkFolderFromButton:)];
  view_id_util::SetID(managedBookmarksButton_.get(), VIEW_ID_MANAGED_BOOKMARKS);
  NSRect frame = NSMakeRect(0, bookmarks::kBookmarkVerticalPadding,
                            [self widthForBookmarkButtonCell:cell],
                            GetBookmarkButtonHeightMinusPadding());
  [managedBookmarksButton_ setFrame:frame];
  [buttonView_ addSubview:managedBookmarksButton_.get()];

}

// Creates the button for "Other Bookmarks", but does not position it.
- (void)createOtherBookmarksButton {
  // Can't create this until the model is loaded, but only need to
  // create it once.
  if (otherBookmarksButton_.get()) {
    return;
  }

  NSCell* cell = [self cellForBookmarkNode:bookmarkModel_->other_node()];
  otherBookmarksButton_.reset([self createCustomBookmarkButtonForCell:cell]);
  [otherBookmarksButton_ setAction:@selector(openBookmarkFolderFromButton:)];
  NSRect frame = NSMakeRect(0, bookmarks::kBookmarkVerticalPadding,
                            [self widthForBookmarkButtonCell:cell],
                            GetBookmarkButtonHeightMinusPadding());
  [otherBookmarksButton_ setFrame:frame];
  view_id_util::SetID(otherBookmarksButton_.get(), VIEW_ID_OTHER_BOOKMARKS);
  [buttonView_ addSubview:otherBookmarksButton_.get()];
}

// Creates the button for "Apps", but does not position it.
- (void)createAppsPageShortcutButton {
  if (appsPageShortcutButton_.get()) {
    return;
  }

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSString* text = l10n_util::GetNSString(IDS_BOOKMARK_BAR_APPS_SHORTCUT_NAME);
  NSImage* image = rb.GetNativeImageNamed(
      IDR_BOOKMARK_BAR_APPS_SHORTCUT).ToNSImage();
  NSCell* cell = [self cellForCustomButtonWithText:text
                                             image:image];
  NSRect frame;
  frame.origin.y = bookmarks::kBookmarkVerticalPadding;
  frame.size = NSMakeSize([self widthForBookmarkButtonCell:cell],
                          GetBookmarkButtonHeightMinusPadding());
  appsPageShortcutButton_.reset([self createCustomBookmarkButtonForCell:cell]);
  [appsPageShortcutButton_ setFrame:frame];
  [[appsPageShortcutButton_ draggableButton] setActsOnMouseDown:NO];
  [appsPageShortcutButton_ setAction:@selector(openAppsPage:)];
  NSString* tooltip =
      l10n_util::GetNSString(IDS_BOOKMARK_BAR_APPS_SHORTCUT_TOOLTIP);
  [appsPageShortcutButton_ setToolTip:tooltip];
  [buttonView_ addSubview:appsPageShortcutButton_.get()];
}

// Creates the "off-the-side" (chevron/overflow) button but
// does not position it.
- (void)createOffTheSideButton {
  if (offTheSideButton_.get()) {
    return;
  }
  DCHECK(bookmarkModel_->loaded());
  offTheSideButton_.reset(
      [[BookmarkButton alloc] initWithFrame:NSMakeRect(0, 0, 20, 24)]);
  id offTheSideCell = [BookmarkButtonCell offTheSideButtonCell];
  [offTheSideCell setTag:kMaterialStandardButtonTypeWithLimitedClickFeedback];
  [offTheSideCell setImagePosition:NSImageOnly];

  [offTheSideCell setHighlightsBy:NSNoCellMask];
  [offTheSideCell setShowsBorderOnlyWhileMouseInside:YES];
  [offTheSideCell setBezelStyle:NSShadowlessSquareBezelStyle];
  [offTheSideCell setBookmarkNode:bookmarkModel_->bookmark_bar_node()];

  [offTheSideButton_ setCell:offTheSideCell];
  [offTheSideButton_ setImage:[self offTheSideButtonImage:NO]];
  [offTheSideButton_ setButtonType:NSMomentaryLightButton];

  [offTheSideButton_ setTarget:self];
  [offTheSideButton_ setAction:@selector(openOffTheSideFolderFromButton:)];
  [offTheSideButton_ setDelegate:self];
  [[offTheSideButton_ draggableButton] setDraggable:NO];
  [[offTheSideButton_ draggableButton] setActsOnMouseDown:YES];
  [offTheSideButton_ setHidden:YES];
  [buttonView_ addSubview:offTheSideButton_];
}

- (void)updateExtraButtonsVisibility {
  [self rebuildLayoutWithAnimated:NO];
}

- (void)createExtraButtons {
  DCHECK(!didCreateExtraButtons_);
  [self createManagedBookmarksButton];
  [self createOtherBookmarksButton];
  [self createAppsPageShortcutButton];
  [self createOffTheSideButton];
  didCreateExtraButtons_ = YES;
}

- (void)openAppsPage:(id)sender {
  WindowOpenDisposition disposition =
      ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  [self openURL:GURL(chrome::kChromeUIAppsURL) disposition:disposition];
  RecordBookmarkAppsPageOpen([self bookmarkLaunchLocation]);
}

// To avoid problems with sync, changes that may impact the current
// bookmark (e.g. deletion) make sure context menus are closed.  This
// prevents deleting a node which no longer exists.
- (void)cancelMenuTracking {
  [contextMenuController_ cancelTracking];
}

// Moves to the given next state (from the current state), possibly animating.
// If |animate| is NO, it will stop any running animation and jump to the given
// state. If YES, it may either (depending on implementation) jump to the end of
// the current animation and begin the next one, or stop the current animation
// mid-flight and animate to the next state.
- (void)moveToState:(BookmarkBar::State)nextState
      withAnimation:(BOOL)animate {
  BOOL isAnimationRunning = [self isAnimationRunning];

  // No-op if the next state is the same as the "current" one, subject to the
  // following conditions:
  //  - no animation is running; or
  //  - an animation is running and |animate| is YES ([*] if it's NO, we'd want
  //    to cancel the animation and jump to the final state).
  if ((nextState == currentState_) && (!isAnimationRunning || animate))
    return;

  // If an animation is running, we want to finalize it. Otherwise we'd have to
  // be able to animate starting from the middle of one type of animation. We
  // assume that animations that we know about can be "reversed".
  if (isAnimationRunning) {
    // Don't cancel if we're going to reverse the animation.
    if (nextState != lastState_) {
      [self stopCurrentAnimation];
      [self finalizeState];
    }

    // If we're in case [*] above, we can stop here.
    if (nextState == currentState_)
      return;
  }

  // Now update with the new state change.
  lastState_ = currentState_;
  currentState_ = nextState;
  isAnimationRunning_ = YES;

  // Animate only if told to and if bar is enabled.
  if (animate && stateAnimationsEnabled_ && barIsEnabled_) {
    [self closeAllBookmarkFolders];
    // Take care of any animation cases we know how to handle.

    // We know how to handle hidden <-> normal, normal <-> detached....
    if ([self isAnimatingBetweenState:BookmarkBar::HIDDEN
                             andState:BookmarkBar::SHOW] ||
        [self isAnimatingBetweenState:BookmarkBar::SHOW
                             andState:BookmarkBar::DETACHED]) {
      [delegate_ bookmarkBar:self
        willAnimateFromState:lastState_
                     toState:currentState_];
      [self showBookmarkBarWithAnimation:YES];
      return;
    }

    // If we ever need any other animation cases, code would go here.
    // Let any animation cases which we don't know how to handle fall through to
    // the unanimated case.
  }

  // Just jump to the state.
  [self finalizeState];
}

// N.B.: |-moveToState:...| will check if this should be a no-op or not.
- (void)updateState:(BookmarkBar::State)newState
         changeType:(BookmarkBar::AnimateChangeType)changeType {
  BOOL animate = changeType == BookmarkBar::ANIMATE_STATE_CHANGE &&
                 stateAnimationsEnabled_;
  [self moveToState:newState withAnimation:animate];
}

// Jump to final state (detached, attached, hidden, etc.) without animating,
// stopping a running animation if necessary.
- (void)finalizeState {
  // We promise that our delegate that the variables will be finalized before
  // the call to |-bookmarkBar:didChangeFromState:toState:|.
  BookmarkBar::State oldState = lastState_;
  lastState_ = currentState_;
  isAnimationRunning_ = NO;

  // Notify our delegate.
  [delegate_ bookmarkBar:self
      didChangeFromState:oldState
                 toState:currentState_];

  // Update ourselves visually.
  [self updateVisibility];
}

// Stops any current animation in its tracks (midway).
- (void)stopCurrentAnimation {
  [[self controlledView] stopAnimation];
}

// Delegate method for |AnimatableView| (a superclass of
// |BookmarkBarToolbarView|).
- (void)animationDidEnd:(NSAnimation*)animation {
  [self finalizeState];
}

// Bookmark button menu items that open a new window (e.g., open in new window,
// open in incognito, edit, etc.) cause us to lose a mouse-exited event
// on the button, which leaves it in a hover state.
// Since the showsBorderOnlyWhileMouseInside uses a tracking area, simple
// tricks (e.g. sending an extra mouseExited: to the button) don't
// fix the problem.
// http://crbug.com/129338
- (void)unhighlightBookmark:(const BookmarkNode*)node {
  // Only relevant if context menu was opened from a button on the
  // bookmark bar.
  const BookmarkNode* parent = node->parent();
  BookmarkNode::Type parentType = parent->type();
  if (parentType == BookmarkNode::BOOKMARK_BAR) {
    int index = parent->GetIndexOf(node);
    if ((index >= 0) && (static_cast<NSUInteger>(index) < [buttons_ count])) {
      NSButton* button =
          [buttons_ objectAtIndex:static_cast<NSUInteger>(index)];
      if ([button showsBorderOnlyWhileMouseInside]) {
        [button setShowsBorderOnlyWhileMouseInside:NO];
        [button setShowsBorderOnlyWhileMouseInside:YES];
      }
    }
  }
}

- (void)addButtonForNode:(const bookmarks::BookmarkNode*)node
                 atIndex:(NSInteger)buttonIndex {
  [self rebuildLayoutWithAnimated:NO];
}

#pragma mark Private Methods Exposed for Testing

- (BookmarkBarView*)buttonView {
  return buttonView_;
}

- (NSMutableArray*)buttons {
  return buttons_.get();
}

- (BookmarkButton*)otherBookmarksButton {
  return otherBookmarksButton_.get();
}

- (BookmarkButton*)managedBookmarksButton {
  return managedBookmarksButton_.get();
}

- (BookmarkBarFolderController*)folderController {
  return folderController_;
}

- (id)folderTarget {
  return folderTarget_.get();
}

// Return an autoreleased NSCell suitable for a bookmark button.
// TODO(jrg): move much of the cell config into the BookmarkButtonCell class.
- (BookmarkButtonCell*)cellForBookmarkNode:(const BookmarkNode*)node {
  BOOL darkTheme = [[[self view] window] hasDarkTheme];
  NSImage* image = node ? [self faviconForNode:node forADarkTheme:darkTheme]
                        : nil;
  BookmarkButtonCell* cell =
      [BookmarkButtonCell buttonCellForNode:node
                                       text:nil
                                      image:image
                             menuController:contextMenuController_];
  [cell setTag:kMaterialStandardButtonTypeWithLimitedClickFeedback];

  // Note: a quirk of setting a cell's text color is that it won't work
  // until the cell is associated with a button, so we can't theme the cell yet.

  return cell;
}

// Return an autoreleased NSCell suitable for a special button displayed on the
// bookmark bar that is not attached to any bookmark node.
// TODO(jrg): move much of the cell config into the BookmarkButtonCell class.
- (BookmarkButtonCell*)cellForCustomButtonWithText:(NSString*)text
                                             image:(NSImage*)image {
  BookmarkButtonCell* cell =
      [BookmarkButtonCell buttonCellWithText:text
                                       image:image
                              menuController:contextMenuController_];
  [cell setTag:kMaterialStandardButtonTypeWithLimitedClickFeedback];
  [cell setHighlightsBy:NSNoCellMask];

  // Note: a quirk of setting a cell's text color is that it won't work
  // until the cell is associated with a button, so we can't theme the cell yet.

  return cell;
}

// Called when our controlled frame has changed size.
- (void)frameDidChange {
  [self rebuildLayoutWithAnimated:NO];
}

// Adapt appearance of buttons to the current theme. Called after
// theme changes, or when our view is added to the view hierarchy.
// Oddly, the view pings us instead of us pinging our view.  This is
// because our trigger is an [NSView viewWillMoveToWindow:], which the
// controller doesn't normally know about.  Otherwise we don't have
// access to the theme before we know what window we will be on.
- (void)updateTheme:(const ui::ThemeProvider*)themeProvider {
  if (!themeProvider)
    return;
  NSColor* color =
      themeProvider->GetNSColor(ThemeProperties::COLOR_BOOKMARK_TEXT);
  for (BookmarkButton* button in buttons_.get()) {
    BookmarkButtonCell* cell = [button cell];
    [cell setTextColor:color];
  }
  [[managedBookmarksButton_ cell] setTextColor:color];
  [[otherBookmarksButton_ cell] setTextColor:color];
  [[appsPageShortcutButton_ cell] setTextColor:color];
}

// Return YES if the event indicates an exit from the bookmark bar
// folder menus.  E.g. "click outside" of the area we are watching.
// At this time we are watching the area that includes all popup
// bookmark folder windows.
- (BOOL)isEventAnExitEvent:(NSEvent*)event {
  NSWindow* eventWindow = [event window];
  NSWindow* myWindow = [[self view] window];
  switch ([event type]) {
    case NSLeftMouseDown:
    case NSRightMouseDown:
      // If the click is in my window but NOT in the bookmark bar, consider
      // it a click 'outside'. Clicks directly on an active button (i.e. one
      // that is a folder and for which its folder menu is showing) are 'in'.
      // All other clicks on the bookmarks bar are counted as 'outside'
      // because they should close any open bookmark folder menu.
      if (eventWindow == myWindow) {
        NSView* hitView =
            [[eventWindow contentView] hitTest:[event locationInWindow]];
        if (hitView == [folderController_ parentButton])
          return NO;
        if (![hitView isDescendantOf:[self view]] ||
            hitView == buttonView_.get())
          return YES;
      }
      // If a click in a bookmark bar folder window and that isn't
      // one of my bookmark bar folders, YES is click outside.
      if (![eventWindow isKindOfClass:[BookmarkBarFolderWindow
                                       class]]) {
        return YES;
      }
      break;
    case NSKeyDown: {
      // Event hooks often see the same keydown event twice due to the way key
      // events get dispatched and redispatched, so ignore if this keydown
      // event has the EXACT same timestamp as the previous keydown.
      static NSTimeInterval lastKeyDownEventTime;
      NSTimeInterval thisTime = [event timestamp];
      if (lastKeyDownEventTime != thisTime) {
        lastKeyDownEventTime = thisTime;
        // Ignore all modifiers like Cmd - keyboard shortcuts should not work
        // while a bookmark folder window (essentially a menu) is open.
        return [folderController_ handleInputText:[event characters]];
      }
      return NO;
    }
    case NSKeyUp:
      return NO;
    case NSLeftMouseDragged:
      // We can get here with the following sequence:
      // - open a bookmark folder
      // - right-click (and unclick) on it to open context menu
      // - move mouse to window titlebar then click-drag it by the titlebar
      // http://crbug.com/49333
      return NO;
    default:
      break;
  }
  return NO;
}

#pragma mark Drag & Drop

// Find something like std::is_between<T>?  I can't believe one doesn't exist.
static BOOL ValueInRangeInclusive(CGFloat low, CGFloat value, CGFloat high) {
  return ((value >= low) && (value <= high));
}

// Return the proposed drop target for a hover open button from the
// given array, or nil if none.  We use this for distinguishing
// between a hover-open candidate or drop-indicator draw.
// Helper for buttonForDroppingOnAtPoint:.
- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point
                                    fromArray:(NSArray*)array {
  // Ensure buttons are scanned left to right.
  id<NSFastEnumeration> buttonsToCheck =
      cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
          ? [array reverseObjectEnumerator]
          : [array objectEnumerator];
  for (BookmarkButton* button in buttonsToCheck) {
    // Hidden buttons can overlap valid visible buttons, just ignore.
    if ([button isHidden])
      continue;
    // Break early if we've gone too far.
    if ((NSMinX([button frame]) > point.x) || (![button superview]))
      return nil;
    // Careful -- this only applies to the bar with horiz buttons.
    // Intentionally NOT using NSPointInRect() so that scrolling into
    // a submenu doesn't cause it to be closed.
    if (ValueInRangeInclusive(NSMinX([button frame]),
                              point.x,
                              NSMaxX([button frame]))) {
      // Over a button but let's be a little more specific (make sure
      // it's over the middle half, not just over it).
      NSRect frame = [button frame];
      NSRect middleHalfOfButton = NSInsetRect(frame, frame.size.width / 4, 0);
      if (ValueInRangeInclusive(NSMinX(middleHalfOfButton),
                                point.x,
                                NSMaxX(middleHalfOfButton))) {
        // It makes no sense to drop on a non-folder; there is no hover.
        if (![button isFolder])
          return nil;
        // Got it!
        return button;
      } else {
        // Over a button but not over the middle half.
        return nil;
      }
    }
  }
  // Not hovering over a button.
  return nil;
}

// Return the proposed drop target for a hover open button, or nil if
// none.  Works with both the bookmark buttons and the "Other
// Bookmarks" button.  Point is in [self view] coordinates.
- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point {
  point = [[self view] convertPoint:point
                           fromView:[[[self view] window] contentView]];

  // If there's a hover button, return it if the point is within its bounds.
  // Since the logic in -buttonForDroppingOnAtPoint:fromArray: only matches a
  // button when the point is over the middle half, this is needed to prevent
  // the button's folder being closed if the mouse temporarily leaves the
  // middle half but is still within the button bounds.
  if (hoverButton_ && NSPointInRect(point, [hoverButton_ frame]))
    return hoverButton_.get();

  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point
                                                  fromArray:buttons_.get()];
  // One more chance -- try "Other Bookmarks" and "off the side" (if visible).
  // This is different than BookmarkBarFolderController.
  if (!button) {
    NSMutableArray* array = [NSMutableArray array];
    if (layout_.IsOffTheSideButtonVisible())
      [array addObject:offTheSideButton_];
    [array addObject:otherBookmarksButton_];
    button = [self buttonForDroppingOnAtPoint:point
                                    fromArray:array];
  }
  return button;
}

// Returns the index in the model for a drag to the location given by
// |point|. This is determined by finding the first button before the center
// of which |point| falls, scanning front to back. Note that, currently, only
// the x-coordinate of |point| is considered. Though not currently implemented,
// we may check for errors, in which case this would return negative value;
// callers should check for this.
- (int)indexForDragToPoint:(NSPoint)point {
  NSPoint dropLocation =
      [[self view] convertPoint:point
                       fromView:[[[self view] window] contentView]];

  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  for (size_t i = 0; i < layout_.VisibleButtonCount(); i++) {
    BookmarkButton* button = [buttons_ objectAtIndex:i];
    CGFloat midpoint = NSMidX([button frame]);
    if (isRTL ? dropLocation.x >= midpoint : dropLocation.x <= midpoint) {
      return i;
    }
  }
  // No buttons left to insert the dragged button before, so it
  // goes at the end.
  return layout_.VisibleButtonCount();
}

// Informs the bar that something representing the bookmark at
// |sourceNode| was dragged into the bar.
// |point| is in the base coordinate system of the destination window;
// it comes from an id<NSDraggingInfo>. |copy| is YES if a copy is to be
// made and inserted into the new location while leaving the bookmark in
// the old location, otherwise move the bookmark by removing from its old
// location and inserting into the new location.
// Returns whether the drag was allowed.
// TODO(mrossetti,jrg): Yet more duplicated code.
// http://crbug.com/35966
- (BOOL)dragBookmark:(const BookmarkNode*)sourceNode
                  to:(NSPoint)point
                copy:(BOOL)copy {
  DCHECK(sourceNode);
  // Drop destination.
  const BookmarkNode* destParent = NULL;
  int destIndex = 0;

  // First check if we're dropping on a button.  If we have one, and
  // it's a folder, drop in it.
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];
  if ([button isFolder]) {
    destParent = [button bookmarkNode];
    // Drop it at the end.
    destIndex = [button bookmarkNode]->child_count();
  } else {
    // Else we're dropping somewhere on the bar, so find the right spot.
    destParent = bookmarkModel_->bookmark_bar_node();
    destIndex = [self indexForDragToPoint:point];
  }

  if (!managedBookmarkService_->CanBeEditedByUser(destParent))
    return NO;
  if (!managedBookmarkService_->CanBeEditedByUser(sourceNode))
    copy = YES;

  // Be sure we don't try and drop a folder into itself.
  if (sourceNode != destParent) {
    if (copy)
      bookmarkModel_->Copy(sourceNode, destParent, destIndex);
    else
      bookmarkModel_->Move(sourceNode, destParent, destIndex);
  }

  [self closeFolderAndStopTrackingMenus];

  // Movement of a node triggers observers (like us) to rebuild the
  // bar so we don't have to do so explicitly.

  return YES;
}

- (void)draggingEnded:(id<NSDraggingInfo>)info {
  [self closeFolderAndStopTrackingMenus];
  layout_ = {};  // Force layout.
  [self rebuildLayoutWithAnimated:YES];
}

// Set insertionPos_ and hasInsertionPos_, and make insertion space for a
// hypothetical drop with the new button having a left edge of |where|.
- (void)setDropInsertionPos:(CGFloat)where {
  if (hasInsertionPos_ && where == insertionPos_) {
    return;
  }
  insertionPos_ = where;
  hasInsertionPos_ = YES;
  CGFloat paddingWidth = bookmarks::kDefaultBookmarkWidth;
  BookmarkButton* draggedButton = [BookmarkButton draggedButton];
  BOOL draggedButtonIsOnBar = NO;
  int64_t draggedButtonNodeId;
  CGFloat draggedButtonOffset;
  if (draggedButton) {
    paddingWidth = std::min(bookmarks::kDefaultBookmarkWidth,
                            NSWidth([draggedButton frame]));
    draggedButtonNodeId = [draggedButton bookmarkNode]->id();
    auto offsetIt = layout_.button_offsets.find(draggedButtonNodeId);
    if (offsetIt != layout_.button_offsets.end()) {
      draggedButtonIsOnBar = YES;
      draggedButtonOffset = (*offsetIt).second;
    }
  }
  paddingWidth += bookmarks::kBookmarkHorizontalPadding;
  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();

  // If the button being dragged is not currently on the bar
  // (for example, a drag from the URL bar, a link on the desktop,
  // a button inside a menu, etc.), buttons in front of the drag position
  // (to the right in LTR, to the left in RTL), should make room for it.
  // Otherwise:
  // If a given button was to the left of the dragged button's original
  // position, but is to the right of the drag position, or
  // if it was to the right of the dragged button's original position and
  // is to the left of the drag position, make room.
  // TODO(lgrey): Extract ScopedNSAnimationContextGroup
  // from the tab strip controller and use it on all of these.
  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext]
      setDuration:kDragAndDropAnimationDuration];

  for (BookmarkButton* button in buttons_.get()) {
    CGRect buttonFrame = [button frame];
    int64_t nodeId = [button bookmarkNode]->id();
    CGFloat offset = layout_.button_offsets[nodeId];
    CGFloat buttonEdge = isRTL ? NSWidth([buttonView_ frame]) - offset : offset;

    if (draggedButtonIsOnBar) {
      if (nodeId == draggedButtonNodeId)
        continue;
      BOOL wasBefore = offset < draggedButtonOffset;
      BOOL isBefore = isRTL ? buttonEdge > where : buttonEdge < where;
      if (isBefore && !wasBefore) {
        offset -= paddingWidth;
      } else if (!isBefore && wasBefore) {
        offset += paddingWidth;
      }
    } else {
      if (isRTL ? (where > buttonEdge)
                : (where < offset + NSWidth(buttonFrame))) {
        offset += paddingWidth;
      }
    }

    [self applyXOffset:offset
              toButton:button
              animated:innerContentAnimationsEnabled_];
  }

  [NSAnimationContext endGrouping];
}

- (BookmarkBarLayout)layoutFromCurrentState {
  BookmarkBarLayout layout = {};

  const BookmarkNode* node = bookmarkModel_->bookmark_bar_node();
  CGFloat viewWidth = NSWidth([buttonView_ frame]);
  CGFloat xOffset = bookmarks::kBookmarkLeftMargin;
  CGFloat maxX = viewWidth - bookmarks::kBookmarkHorizontalPadding;

  layout.visible_elements = bookmarks::kVisibleElementsMaskNone;
  layout.max_x = maxX;

  // Lay out "extra" buttons (apps, managed, "Other").
  if (chrome::ShouldShowAppsShortcutInBookmarkBar(browser_->profile())) {
    layout.visible_elements |= bookmarks::kVisibleElementsMaskAppsButton;
    layout.apps_button_offset = xOffset;
    xOffset += NSWidth([appsPageShortcutButton_ frame]) +
               bookmarks::kBookmarkHorizontalPadding;
  }
  if ([self shouldShowManagedBookmarksButton]) {
    layout.visible_elements |=
        bookmarks::kVisibleElementsMaskManagedBookmarksButton;
    layout.managed_bookmarks_button_offset = xOffset;
    xOffset += NSWidth([managedBookmarksButton_ frame]) +
               bookmarks::kBookmarkHorizontalPadding;
  }
  if (!bookmarkModel_->other_node()->empty()) {
    layout.visible_elements |=
        bookmarks::kVisibleElementsMaskOtherBookmarksButton;
    maxX -= NSWidth([otherBookmarksButton_ frame]);
    layout.other_bookmarks_button_offset = maxX;
  }

  // Lay out empty state ("no items" label, import bookmarks button.)
  if (node->empty()) {
    CGFloat roomForTextField =
        maxX - xOffset + bookmarks::kInitialNoItemTextFieldXOrigin;
    if (roomForTextField >= kNoItemElementMinWidth) {
      layout.visible_elements |= bookmarks::kVisibleElementsMaskNoItemTextField;
      xOffset += bookmarks::kInitialNoItemTextFieldXOrigin;
      layout.no_item_textfield_offset = xOffset;
      layout.no_item_textfield_width =
          std::min(maxX - xOffset, originalNoItemTextFieldWidth_);
      xOffset +=
          layout.no_item_textfield_width + originalNoItemInterelementPadding_;

      // Does the "import bookmarks" button fit?
      if (maxX - xOffset >= kNoItemElementMinWidth) {
        layout.visible_elements |=
            bookmarks::kVisibleElementsMaskImportBookmarksButton;
        layout.import_bookmarks_button_offset = xOffset;
        layout.import_bookmarks_button_width =
            std::min(maxX - xOffset, originalImportBookmarksButtonWidth_);
      }
    }
  } else {
    // Lay out bookmark buttons and "off-the-side" chevron.
    CGFloat offTheSideButtonWidth = NSWidth([offTheSideButton_ frame]);
    int buttonCount = node->child_count();
    for (int i = 0; i < buttonCount; i++) {
      const BookmarkNode* buttonNode = node->GetChild(i);
      CGFloat widthOfButton = [self widthOfButtonForNode:buttonNode];
      // If it's the last button, we just need to ensure that it can fit.
      // If not, we need to be able to fit both it and the chevron.
      CGFloat widthToCheck = i == buttonCount - 1
                                 ? widthOfButton
                                 : widthOfButton + offTheSideButtonWidth;
      if (xOffset + widthToCheck > maxX) {
        layout.visible_elements |=
            bookmarks::kVisibleElementsMaskOffTheSideButton;
        layout.off_the_side_button_offset = maxX - offTheSideButtonWidth;
        break;
      }
      layout.button_offsets.insert({buttonNode->id(), xOffset});
      xOffset += widthOfButton + bookmarks::kBookmarkHorizontalPadding;
    }
  }
  return layout;
}

- (void)rebuildLayoutWithAnimated:(BOOL)animated {
  if (!bookmarkModel_->loaded())
    return;

  BookmarkBarLayout layout = [self layoutFromCurrentState];
  if (layout_ != layout) {
    layout_ = std::move(layout);
    [self applyLayout:layout_ animated:animated];
  }
}

- (void)applyLayout:(const BookmarkBarLayout&)layout animated:(BOOL)animated {
  if (!bookmarkModel_->loaded())
    return;

  if (folderController_)
    [self closeAllBookmarkFolders];
  [self stopPulsingBookmarkNode];

  // Hide or show "extra" buttons and position if necessary.
  [self applyXOffset:layout.apps_button_offset
            toButton:appsPageShortcutButton_
            animated:NO];
  [appsPageShortcutButton_ setHidden:!layout.IsAppsButtonVisible()];

  [self applyXOffset:layout.managed_bookmarks_button_offset
            toButton:managedBookmarksButton_
            animated:NO];
  [managedBookmarksButton_ setHidden:!layout.IsManagedBookmarksButtonVisible()];

  [self applyXOffset:layout.other_bookmarks_button_offset
            toButton:otherBookmarksButton_
            animated:NO];
  [otherBookmarksButton_ setHidden:!layout.IsOtherBookmarksButtonVisible()];

  [self applyXOffset:layout.off_the_side_button_offset
            toButton:offTheSideButton_
            animated:NO];
  [offTheSideButton_ setHidden:!layout.IsOffTheSideButtonVisible()];
  if (layout.IsOffTheSideButtonVisible()) {
    [[offTheSideButton_ cell]
        setStartingChildIndex:layout_.VisibleButtonCount()];
  }

  // Hide or show empty state and position if necessary.
  [[buttonView_ noItemTextField] setHidden:!layout.IsNoItemTextFieldVisible()];
  [[buttonView_ importBookmarksButton]
      setHidden:!layout.IsImportBookmarksButtonVisible()];

  if (layout.IsNoItemTextFieldVisible()) {
    NSTextField* noItemTextField = [buttonView_ noItemTextField];
    [noItemTextField setHidden:NO];
    NSRect textFieldFrame = [noItemTextField frame];
    textFieldFrame.size.width = layout.no_item_textfield_width;
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
      textFieldFrame.origin.x = NSWidth([buttonView_ frame]) -
                                layout.no_item_textfield_offset -
                                layout.no_item_textfield_width;
    } else {
      textFieldFrame.origin.x = layout.no_item_textfield_offset;
    }
    [noItemTextField setFrame:textFieldFrame];

    if (layout.IsImportBookmarksButtonVisible()) {
      NSButton* importBookmarksButton = [buttonView_ importBookmarksButton];
      NSRect buttonFrame = [importBookmarksButton frame];
      buttonFrame.size.width = layout.import_bookmarks_button_width;
      if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
        buttonFrame.origin.x = NSWidth([buttonView_ frame]) -
                               layout.import_bookmarks_button_offset -
                               layout.import_bookmarks_button_width;
      } else {
        buttonFrame.origin.x = layout.import_bookmarks_button_offset;
      }
      [importBookmarksButton setFrame:buttonFrame];
    }
  }
  // 1) Hide all buttons initially.
  // 2) Show all buttons in the layout.
  // 3) Remove any buttons that are still hidden (so: not in the layout)
  //    from the node ID -> button map and add them to the reuse pool if
  //    there's room.
  for (BookmarkButton* button in buttons_.get()) {
    [button setHidden:YES];
  }
  [buttons_ removeAllObjects];

  const BookmarkNode* parentNode = bookmarkModel_->bookmark_bar_node();
  for (size_t i = 0; i < layout.VisibleButtonCount(); i++) {
    const BookmarkNode* node = parentNode->GetChild(i);
    DCHECK(node);
    BookmarkButton* button = [self buttonForNode:node];
    CGFloat offset = layout.button_offsets.at(node->id());
    [self applyXOffset:offset
              toButton:button
              animated:animated && innerContentAnimationsEnabled_];
    [buttons_ addObject:button];
    [button setHidden:NO];
  }

  auto item = nodeIdToButtonMap_.begin();
  while (item != nodeIdToButtonMap_.end()) {
    if ([item->second isHidden]) {
      if ([unusedButtonPool_ count] < kMaxReusePoolSize) {
        [self prepareButtonForReuse:item->second.get()];
      } else {
        [item->second setDelegate:nil];
        [item->second removeFromSuperview];
      }
      item = nodeIdToButtonMap_.erase(item);
    } else {
      ++item;
    }
  }
  const ui::ThemeProvider* themeProvider = [[[self view] window] themeProvider];
  [self updateTheme:themeProvider];
}

- (void)applyXOffset:(CGFloat)offset
            toButton:(BookmarkButton*)button
            animated:(BOOL)animated {
  CGRect frame = [button frame];
  BOOL wasOffscreen = frame.origin.x < -200;
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    frame.origin.x = NSWidth([buttonView_ frame]) - offset - NSWidth(frame);
  } else {
    frame.origin.x = offset;
  }

  // Buttons fresh from the reuse pool are kept offscreen. Animation should
  // be disabled for them so they don't slide in.
  if (animated && !wasOffscreen) {
    [[button animator] setFrame:frame];
  } else {
    [button setFrame:frame];
  }
}

- (CGFloat)widthOfButtonForNode:(const BookmarkNode*)node {
  // TODO(lgrey): Can we get this information without an actual image?
  NSImage* image = [self faviconForNode:node forADarkTheme:NO];
  CGFloat width = [BookmarkButtonCell cellWidthForNode:node image:image] +
                  [BookmarkButtonCell insetInView:buttonView_] * 2;
  return std::min(width, bookmarks::kDefaultBookmarkWidth);
}

// Clear insertion flag, remove insertion space and put all visible bookmark
// bar buttons in their normal locations.
// Gets called only by our view.
// TODO(lgrey): Is there a sane way to dedupe this with |setDropInsertionPos:|?
- (void)clearDropInsertionPos {
  if (!hasInsertionPos_) {
    return;
  }
  hasInsertionPos_ = NO;
  BookmarkButton* draggedButton = [BookmarkButton draggedButton];
  if (!draggedButton || [draggedButton bookmarkNode] == nullptr) {
    [self applyLayout:layout_ animated:YES];
    return;
  }
  int64_t draggedButtonNodeId = [draggedButton bookmarkNode]->id();
  if (layout_.button_offsets.find(draggedButtonNodeId) ==
      layout_.button_offsets.end()) {
    [self applyLayout:layout_ animated:YES];
    return;
  }
  // The dragged button came from the bar, but is being dragged outside
  // of it now, so the rest of the buttons should be laid out as if it
  // were removed.
  CGFloat draggedButtonOffset = layout_.button_offsets[draggedButtonNodeId];
  CGFloat spaceForDraggedButton =
      NSWidth([draggedButton frame]) + bookmarks::kBookmarkHorizontalPadding;
  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext]
      setDuration:kDragAndDropAnimationDuration];

  for (BookmarkButton* button in buttons_.get()) {
    int64_t nodeId = [button bookmarkNode]->id();
    CGFloat offset = layout_.button_offsets[nodeId];

    if (nodeId == draggedButtonNodeId)
      continue;

    if (offset > draggedButtonOffset) {
      offset -= spaceForDraggedButton;
      [self applyXOffset:offset
                toButton:button
                animated:innerContentAnimationsEnabled_];
    }
  }

  [NSAnimationContext endGrouping];
}

#pragma mark Bridge Notification Handlers

- (void)loaded:(BookmarkModel*)model {
  DCHECK(model == bookmarkModel_);
  if (!model->loaded())
    return;
  // Make sure there are no stale pointers in the pasteboard.  This
  // can be important if a bookmark is deleted (via bookmark sync)
  // while in the middle of a drag.  The "drag completed" code
  // (e.g. [BookmarkBarView performDragOperationForBookmarkButton:]) is
  // careful enough to bail if there is no data found at "drop" time.
  [[NSPasteboard pasteboardWithName:NSDragPboard] clearContents];

  if (!didCreateExtraButtons_) {
    [self createExtraButtons];
  }

  // If this is a rebuild request while we have a folder open, close it.
  if (folderController_)
    [self closeAllBookmarkFolders];
  [self rebuildLayoutWithAnimated:NO];
}
- (void)nodeAdded:(BookmarkModel*)model
           parent:(const BookmarkNode*)newParent index:(int)newIndex {
  // If a context menu is open, close it.
  [self cancelMenuTracking];

  const BookmarkNode* newNode = newParent->GetChild(newIndex);
  id<BookmarkButtonControllerProtocol> newController =
      [self controllerForNode:newParent];
  [newController addButtonForNode:newNode atIndex:newIndex];
  [self rebuildLayoutWithAnimated:NO];
}

- (void)nodeChanged:(BookmarkModel*)model
               node:(const BookmarkNode*)node {
  // Invalidate the layout if the changed node is visible. This ensures
  // the button is updated if the button offsets don't change but the
  // title does.
  if (nodeIdToButtonMap_.find(node->id()) != nodeIdToButtonMap_.end())
    layout_ = {};
  [self loaded:model];
}

- (void)nodeMoved:(BookmarkModel*)model
        oldParent:(const BookmarkNode*)oldParent oldIndex:(int)oldIndex
        newParent:(const BookmarkNode*)newParent newIndex:(int)newIndex {
  const BookmarkNode* movedNode = newParent->GetChild(newIndex);
  id<BookmarkButtonControllerProtocol> oldController =
      [self controllerForNode:oldParent];
  id<BookmarkButtonControllerProtocol> newController =
      [self controllerForNode:newParent];
  if (newController == oldController) {
    [oldController moveButtonFromIndex:oldIndex toIndex:newIndex];
  } else {
    [oldController removeButton:oldIndex animate:NO];
    [newController addButtonForNode:movedNode atIndex:newIndex];
  }
  [self rebuildLayoutWithAnimated:NO];
}

- (void)nodeRemoved:(BookmarkModel*)model
             parent:(const BookmarkNode*)oldParent index:(int)index {
  // If a context menu is open, close it.
  [self cancelMenuTracking];

  // Locate the parent node. The parent may not be showing, in which case
  // we do nothing.
  id<BookmarkButtonControllerProtocol> parentController =
      [self controllerForNode:oldParent];
  [parentController removeButton:index animate:YES];
  [self rebuildLayoutWithAnimated:NO];
}

// TODO(jrg): if the bookmark bar is open on launch, we see the
// buttons all placed, then "scooted over" as the favicons load.  If
// this looks bad I may need to change widthForBookmarkButtonCell to
// add space for an image even if not there on the assumption that
// favicons will eventually load.
- (void)nodeFaviconLoaded:(BookmarkModel*)model
                     node:(const BookmarkNode*)node {
  auto buttonIt = nodeIdToButtonMap_.find(node->id());
  if (buttonIt != nodeIdToButtonMap_.end()) {
    BookmarkButton* button = (*buttonIt).second;
    BOOL darkTheme = [[[self view] window] hasDarkTheme];
    NSImage* theImage = [self faviconForNode:node forADarkTheme:darkTheme];
    [[button cell] setBookmarkCellText:[button title] image:theImage];
  }
  [self rebuildLayoutWithAnimated:NO];
  if (folderController_)
    [folderController_ faviconLoadedForNode:node];
}

// TODO(jrg): for now this is brute force.
- (void)nodeChildrenReordered:(BookmarkModel*)model
                         node:(const BookmarkNode*)node {
  [self loaded:model];
}

#pragma mark BookmarkBarState Protocol

// (BookmarkBarState protocol)
- (BOOL)isVisible {
  return barIsEnabled_ && (currentState_ == BookmarkBar::SHOW ||
                           currentState_ == BookmarkBar::DETACHED ||
                           lastState_ == BookmarkBar::SHOW ||
                           lastState_ == BookmarkBar::DETACHED);
}

// (BookmarkBarState protocol)
- (BOOL)isInState:(BookmarkBar::State)state {
  return currentState_ == state && ![self isAnimationRunning];
}

// (BookmarkBarState protocol)
- (BOOL)isAnimatingToState:(BookmarkBar::State)state {
  return currentState_ == state && [self isAnimationRunning];
}

// (BookmarkBarState protocol)
- (BOOL)isAnimatingFromState:(BookmarkBar::State)state {
  return lastState_ == state && [self isAnimationRunning];
}

// (BookmarkBarState protocol)
- (BOOL)isAnimatingFromState:(BookmarkBar::State)fromState
                     toState:(BookmarkBar::State)toState {
  return lastState_ == fromState &&
         currentState_ == toState &&
         [self isAnimationRunning];
}

// (BookmarkBarState protocol)
- (BOOL)isAnimatingBetweenState:(BookmarkBar::State)fromState
                       andState:(BookmarkBar::State)toState {
  return [self isAnimatingFromState:fromState toState:toState] ||
         [self isAnimatingFromState:toState toState:fromState];
}

// (BookmarkBarState protocol)
- (CGFloat)detachedMorphProgress {
  if ([self isInState:BookmarkBar::DETACHED]) {
    return 1;
  }
  if ([self isAnimatingToState:BookmarkBar::DETACHED]) {
    return static_cast<CGFloat>(
        [[self controlledView] currentAnimationProgress]);
  }
  if ([self isAnimatingFromState:BookmarkBar::DETACHED]) {
    return static_cast<CGFloat>(
        1 - [[self controlledView] currentAnimationProgress]);
  }
  return 0;
}

#pragma mark BookmarkBarToolbarViewController Protocol

- (int)currentTabContentsHeight {
  BrowserWindowController* browserController =
      [BrowserWindowController browserWindowControllerForView:[self view]];
  return NSHeight([[browserController tabContentArea] frame]);
}

- (Profile*)profile {
  return browser_->profile();
}

#pragma mark BookmarkButtonDelegate Protocol

- (NSPasteboardItem*)pasteboardItemForDragOfButton:(BookmarkButton*)button {
  return [[self folderTarget] pasteboardItemForDragOfButton:button];
}

// BookmarkButtonDelegate protocol implementation.  When menus are
// "active" (e.g. you clicked to open one), moving the mouse over
// another folder button should close the 1st and open the 2nd (like
// real menus).  We detect and act here.
- (void)mouseEnteredButton:(id)sender event:(NSEvent*)event {
  DCHECK([sender isKindOfClass:[BookmarkButton class]]);

  // If folder menus are not being shown, do nothing.  This is different from
  // BookmarkBarFolderController's implementation because the bar should NOT
  // automatically open folder menus when the mouse passes over a folder
  // button while the BookmarkBarFolderController DOES automatically open
  // a subfolder menu.
  if (!showFolderMenus_)
    return;

  // From here down: same logic as BookmarkBarFolderController.
  // TODO(jrg): find a way to share these 4 non-comment lines?
  // http://crbug.com/35966
  // If already opened, then we exited but re-entered the button, so do nothing.
  if ([folderController_ parentButton] == sender)
    return;
  // Else open a new one if it makes sense to do so.
  const BookmarkNode* node = [sender bookmarkNode];
  if (node && node->is_folder()) {
    // Update |hoverButton_| so that it corresponds to the open folder.
    hoverButton_.reset([sender retain]);
    [folderTarget_ openBookmarkFolderFromButton:sender];
  } else {
    // We're over a non-folder bookmark so close any old folders.
    [folderController_ close];
    folderController_ = nil;
  }
}

// BookmarkButtonDelegate protocol implementation.
- (void)mouseExitedButton:(id)sender event:(NSEvent*)event {
  // Don't care; do nothing.
  // This is different behavior that the folder menus.
}

- (NSWindow*)browserWindow {
  return [[self view] window];
}

- (BOOL)canDragBookmarkButtonToTrash:(BookmarkButton*)button {
  return [self canEditBookmarks] &&
         [self canEditBookmark:[button bookmarkNode]];
}

- (void)didDragBookmarkToTrash:(BookmarkButton*)button {
  if ([self canDragBookmarkButtonToTrash:button]) {
    const BookmarkNode* node = [button bookmarkNode];
    if (node)
      bookmarkModel_->Remove(node);
  }
}

- (void)bookmarkDragDidEnd:(BookmarkButton*)button
                 operation:(NSDragOperation)operation {
  [self rebuildLayoutWithAnimated:YES];
}


#pragma mark BookmarkButtonControllerProtocol

// Close all bookmark folders.  "Folder" here is the fake menu for
// bookmark folders, not a button context menu.
- (void)closeAllBookmarkFolders {
  [self watchForExitEvent:NO];

  // Grab the parent button under to make sure that the highlighting that was
  // applied while revealing the menu is turned off.
  BookmarkButton* parentButton = [folderController_ parentButton];
  [folderController_ close];
  [parentButton setNeedsDisplay:YES];
  folderController_ = nil;
}

- (void)closeBookmarkFolder:(id)sender {
  // We're the top level, so close one means close them all.
  [self closeAllBookmarkFolders];
}

- (BookmarkModel*)bookmarkModel {
  return bookmarkModel_;
}

- (BOOL)draggingAllowed:(id<NSDraggingInfo>)info {
  return [self canEditBookmarks];
}

// TODO(jrg): much of this logic is duped with
// [BookmarkBarFolderController draggingEntered:] except when noted.
// http://crbug.com/35966
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  NSPoint point = [info draggingLocation];
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];

  // Don't allow drops that would result in cycles.
  if (button) {
    NSData* data = [[info draggingPasteboard]
        dataForType:ui::ClipboardUtil::UTIForPasteboardType(
                        kBookmarkButtonDragType)];
    if (data && [info draggingSource]) {
      BookmarkButton* sourceButton = nil;
      [data getBytes:&sourceButton length:sizeof(sourceButton)];
      const BookmarkNode* sourceNode = [sourceButton bookmarkNode];
      const BookmarkNode* destNode = [button bookmarkNode];
      if (destNode->HasAncestor(sourceNode))
        button = nil;
    }
  }

  if ([button isFolder]) {
    if (hoverButton_ == button) {
      return NSDragOperationMove;  // already open or timed to open
    }
    if (hoverButton_) {
      // Oops, another one triggered or open.
      [NSObject cancelPreviousPerformRequestsWithTarget:[hoverButton_
                                                         target]];
      // Unlike BookmarkBarFolderController, we do not delay the close
      // of the previous one.  Given the lack of diagonal movement,
      // there is no need, and it feels awkward to do so.  See
      // comments about kDragHoverCloseDelay in
      // bookmark_bar_folder_controller.mm for more details.
      [[hoverButton_ target] closeBookmarkFolder:hoverButton_];
      hoverButton_.reset();
    }
    hoverButton_.reset([button retain]);
    DCHECK([[hoverButton_ target]
            respondsToSelector:@selector(openBookmarkFolderFromButton:)]);
    [[hoverButton_ target]
     performSelector:@selector(openBookmarkFolderFromButton:)
     withObject:hoverButton_
     afterDelay:bookmarks::kDragHoverOpenDelay
     inModes:[NSArray arrayWithObject:NSRunLoopCommonModes]];
  }
  if (!button) {
    if (hoverButton_) {
      [NSObject cancelPreviousPerformRequestsWithTarget:[hoverButton_ target]];
      [[hoverButton_ target] closeBookmarkFolder:hoverButton_];
      hoverButton_.reset();
    }
  }

  // Thrown away but kept to be consistent with the draggingEntered: interface.
  return NSDragOperationMove;
}

- (void)draggingExited:(id<NSDraggingInfo>)info {
  // Only close the folder menu if the user dragged up past the BMB. If the user
  // dragged to below the BMB, they might be trying to drop a link into the open
  // folder menu.
  // TODO(asvitkine): Need a way to close the menu if the user dragged below but
  //                  not into the menu.
  NSRect bounds = [[self view] bounds];
  NSPoint origin = [[self view] convertPoint:bounds.origin toView:nil];
  if ([info draggingLocation].y > origin.y + bounds.size.height)
    [self closeFolderAndStopTrackingMenus];

  // NOT the same as a cancel --> we may have moved the mouse into the submenu.
  if (hoverButton_) {
    [NSObject cancelPreviousPerformRequestsWithTarget:[hoverButton_ target]];
    hoverButton_.reset();
  }
}

- (BOOL)dragShouldLockBarVisibility {
  return ![self isInState:BookmarkBar::DETACHED] &&
  ![self isAnimatingToState:BookmarkBar::DETACHED];
}

// TODO(mrossetti,jrg): Yet more code dup with BookmarkBarFolderController.
// http://crbug.com/35966
- (BOOL)dragButton:(BookmarkButton*)sourceButton
                to:(NSPoint)point
              copy:(BOOL)copy {
  DCHECK([sourceButton isKindOfClass:[BookmarkButton class]]);
  const BookmarkNode* sourceNode = [sourceButton bookmarkNode];
  return [self dragBookmark:sourceNode to:point copy:copy];
}

- (BOOL)dragBookmarkData:(id<NSDraggingInfo>)info {
  BOOL dragged = NO;
  std::vector<const BookmarkNode*> nodes([self retrieveBookmarkNodeData]);
  if (nodes.size()) {
    BOOL copy = !([info draggingSourceOperationMask] & NSDragOperationMove);
    NSPoint dropPoint = [info draggingLocation];
    for (std::vector<const BookmarkNode*>::const_iterator it = nodes.begin();
         it != nodes.end(); ++it) {
      const BookmarkNode* sourceNode = *it;
      dragged = [self dragBookmark:sourceNode to:dropPoint copy:copy];
    }
  }
  return dragged;
}

- (std::vector<const BookmarkNode*>)retrieveBookmarkNodeData {
  std::vector<const BookmarkNode*> dragDataNodes;
  BookmarkNodeData dragData;
  if (dragData.ReadFromClipboard(ui::CLIPBOARD_TYPE_DRAG)) {
    std::vector<const BookmarkNode*> nodes(
        dragData.GetNodes(bookmarkModel_, browser_->profile()->GetPath()));
    dragDataNodes.assign(nodes.begin(), nodes.end());
  }
  return dragDataNodes;
}

// Return YES if we should show the drop indicator, else NO.
- (BOOL)shouldShowIndicatorShownForPoint:(NSPoint)point {
  return ![self buttonForDroppingOnAtPoint:point];
}

// Return the x position for a drop indicator.
- (CGFloat)indicatorPosForDragToPoint:(NSPoint)point {
  CGFloat x = 0;
  CGFloat halfHorizontalPadding = 0.5 * bookmarks::kBookmarkHorizontalPadding;
  int destIndex = [self indexForDragToPoint:point];
  int numButtons = layout_.VisibleButtonCount();

  CGFloat leadingOffset;
  if (layout_.IsManagedBookmarksButtonVisible()) {
    leadingOffset =
        layout_.managed_bookmarks_button_offset + halfHorizontalPadding;
  } else if (layout_.IsAppsButtonVisible()) {
    leadingOffset = layout_.apps_button_offset + halfHorizontalPadding;
  } else {
    leadingOffset = bookmarks::kBookmarkLeftMargin - halfHorizontalPadding;
  }

  // If it's a drop strictly between existing buttons ...
  if (destIndex == 0) {
    x = leadingOffset;
  } else if (destIndex > 0 && destIndex < numButtons) {
    // ... put the indicator right between the buttons.
    int64_t nodeId =
        bookmarkModel_->bookmark_bar_node()->GetChild(destIndex)->id();
    x = layout_.button_offsets[nodeId] - halfHorizontalPadding;
    // If it's a drop at the end (past the last button, if there are any) ...
  } else if (destIndex == numButtons) {
    // and if it's past the last button ...
    if (numButtons > 0) {
      // ... find the last button, and put the indicator after it.
      const BookmarkNode* node =
          bookmarkModel_->bookmark_bar_node()->GetChild(destIndex - 1);
      int64_t nodeId = node->id();
      x = layout_.button_offsets[nodeId] + [self widthOfButtonForNode:node] +
          halfHorizontalPadding;
      // Otherwise, put it right at the beginning.
    } else {
      x = leadingOffset;
    }
  } else {
    NOTREACHED();
  }

  return cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
             ? NSWidth([buttonView_ frame]) - x
             : x;
}

- (void)childFolderWillShow:(id<BookmarkButtonControllerProtocol>)child {
  // If the bookmarkbar is not in detached mode, lock bar visibility, forcing
  // the overlay to stay open when in fullscreen mode.
  if (![self isInState:BookmarkBar::DETACHED] &&
      ![self isAnimatingToState:BookmarkBar::DETACHED]) {
    BrowserWindowController* browserController =
        [BrowserWindowController browserWindowControllerForView:[self view]];
    [browserController lockToolbarVisibilityForOwner:child withAnimation:NO];
  }
}

- (void)childFolderWillClose:(id<BookmarkButtonControllerProtocol>)child {
  // Release bar visibility, allowing the overlay to close if in fullscreen
  // mode.
  BrowserWindowController* browserController =
      [BrowserWindowController browserWindowControllerForView:[self view]];
  [browserController releaseToolbarVisibilityForOwner:child withAnimation:NO];
}

// Add a new folder controller as triggered by the given folder button.
- (void)addNewFolderControllerWithParentButton:(BookmarkButton*)parentButton {
  // If doing a close/open, make sure the fullscreen chrome doesn't
  // have a chance to begin animating away in the middle of things.
  BrowserWindowController* browserController =
      [BrowserWindowController browserWindowControllerForView:[self view]];
  // Confirm we're not re-locking with ourself as an owner before locking.
  DCHECK([browserController isToolbarVisibilityLockedForOwner:self] == NO);
  [browserController lockToolbarVisibilityForOwner:self withAnimation:NO];

  if (folderController_)
    [self closeAllBookmarkFolders];

  // Folder controller, like many window controllers, owns itself.
  folderController_ =
      [[BookmarkBarFolderController alloc]
          initWithParentButton:parentButton
              parentController:nil
                 barController:self
                       profile:browser_->profile()];
  [folderController_ showWindow:self];

  // Only BookmarkBarController has this; the
  // BookmarkBarFolderController does not.
  [self watchForExitEvent:YES];

  // No longer need to hold the lock; the folderController_ now owns it.
  [browserController releaseToolbarVisibilityForOwner:self withAnimation:NO];
}

- (void)openAll:(const BookmarkNode*)node
    disposition:(WindowOpenDisposition)disposition {
  [self closeFolderAndStopTrackingMenus];
  chrome::OpenAll([[self view] window], browser_, node, disposition,
                  browser_->profile());
}

// TODO(mrossetti): Duplicate code with BookmarkBarFolderController.
// http://crbug.com/35966
- (BOOL)addURLs:(NSArray*)urls withTitles:(NSArray*)titles at:(NSPoint)point {
  DCHECK([urls count] == [titles count]);
  BOOL nodesWereAdded = NO;
  // Figure out where these new bookmarks nodes are to be added.
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];
  const BookmarkNode* destParent = NULL;
  int destIndex = 0;
  if ([button isFolder]) {
    destParent = [button bookmarkNode];
    // Drop it at the end.
    destIndex = [button bookmarkNode]->child_count();
  } else {
    // Else we're dropping somewhere on the bar, so find the right spot.
    destParent = bookmarkModel_->bookmark_bar_node();
    destIndex = [self indexForDragToPoint:point];
  }

  if (!managedBookmarkService_->CanBeEditedByUser(destParent))
    return NO;

  // Don't add the bookmarks if the destination index shows an error.
  if (destIndex >= 0) {
    // Create and add the new bookmark nodes.
    size_t urlCount = [urls count];
    for (size_t i = 0; i < urlCount; ++i) {
      GURL gurl;
      const char* string = [[urls objectAtIndex:i] UTF8String];
      if (string)
        gurl = GURL(string);
      // We only expect to receive valid URLs.
      DCHECK(gurl.is_valid());
      if (gurl.is_valid()) {
        bookmarkModel_->AddURL(destParent,
                               destIndex++,
                               base::SysNSStringToUTF16(
                                  [titles objectAtIndex:i]),
                               gurl);
        nodesWereAdded = YES;
      }
    }
  }
  return nodesWereAdded;
}

- (void)moveButtonFromIndex:(NSInteger)fromIndex toIndex:(NSInteger)toIndex {
  int buttonCount = layout_.VisibleButtonCount();
  BOOL isMoveWithinOffTheSideMenu =
      (toIndex >= buttonCount) && (fromIndex >= buttonCount);
  if (isMoveWithinOffTheSideMenu) {
    fromIndex -= buttonCount;
    toIndex -= buttonCount;
    [folderController_ moveButtonFromIndex:fromIndex toIndex:toIndex];
  } else {
    [self rebuildLayoutWithAnimated:NO];
  }
}

- (void)removeButton:(NSInteger)buttonIndex animate:(BOOL)animate {
  if ((size_t)buttonIndex < layout_.VisibleButtonCount()) {
    // The button being removed is showing in the bar.
    BookmarkButton* oldButton = buttons_[buttonIndex];
    if (animate && innerContentAnimationsEnabled_ && [self isVisible] &&
        [[self browserWindow] isMainWindow]) {
      NSPoint poofPoint = [oldButton screenLocationForRemoveAnimation];
      NSShowAnimationEffect(NSAnimationEffectDisappearingItemDefault, poofPoint,
                            NSZeroSize, nil, nil, nil);
    }
    [self rebuildLayoutWithAnimated:YES];
  } else if (folderController_ &&
             [folderController_ parentButton] == offTheSideButton_) {
    // The button being removed is in the OTS (off-the-side) and the OTS
    // menu is showing so we need to remove the button.
    NSInteger index = buttonIndex - layout_.VisibleButtonCount();
    [folderController_ removeButton:index animate:animate];
  }
}

- (id<BookmarkButtonControllerProtocol>)controllerForNode:
    (const BookmarkNode*)node {
  // See if it's in the bar, then if it is in the hierarchy of visible
  // folder menus.
  if (bookmarkModel_->bookmark_bar_node() == node)
    return self;
  return [folderController_ controllerForNode:node];
}

// For testing.
- (const BookmarkBarLayout&)currentLayout {
  return layout_;
}

- (BOOL)shouldShowManagedBookmarksButton {
  PrefService* prefs = browser_->profile()->GetPrefs();
  bool prefIsSet =
      prefs->GetBoolean(bookmarks::prefs::kShowManagedBookmarksInBookmarkBar);
  return prefIsSet && !managedBookmarkService_->managed_node()->empty();
}

@end
