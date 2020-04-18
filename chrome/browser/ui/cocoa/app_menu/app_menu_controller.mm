// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/objc_release_properties.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/scoped_observer.h"
#include "base/strings/string16.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/accelerators_cocoa.h"
#import "chrome/browser/ui/cocoa/app_menu/menu_tracked_root_view.h"
#import "chrome/browser/ui/cocoa/app_menu/recent_tabs_menu_model_delegate.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_observer.h"
#include "chrome/grit/generated_resources.h"
#include "components/zoom/zoom_event_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/geometry/size.h"

namespace {
// Padding amounts on the left/right of a custom menu item (like the browser
// actions overflow container).
const int kLeftPadding = 16;
const int kRightPadding = 10;

// In *very* extreme cases, it's possible that there are so many overflowed
// actions, we won't be able to show them all. Cap the height so that the
// overflow won't make the menu larger than the height of the screen.
// Note: With this height, we can show 104 actions. Less than 0.0002% of our
// users will be affected.
const int kMaxOverflowContainerHeight = 416;
}

namespace app_menu_controller {
const CGFloat kAppMenuBubblePointOffsetY = 6;
}

using base::UserMetricsAction;

@interface AppMenuController (Private)
- (void)createModel;
- (void)adjustPositioning;
- (void)performCommandDispatch:(NSNumber*)tag;
- (NSButton*)zoomDisplay;
- (void)menu:(NSMenu*)menu willHighlightItem:(NSMenuItem*)item;
- (NSMenu*)recentTabsSubmenu;
- (RecentTabsSubMenuModel*)recentTabsMenuModel;
- (int)maxWidthForMenuModel:(ui::MenuModel*)model
                 modelIndex:(int)modelIndex;
@end

namespace AppMenuControllerInternal {

// A C++ delegate that handles the accelerators in the app menu.
class AcceleratorDelegate : public ui::AcceleratorProvider {
 public:
  bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* out_accelerator) const override {
    AcceleratorsCocoa* keymap = AcceleratorsCocoa::GetInstance();
    const ui::Accelerator* accelerator =
        keymap->GetAcceleratorForCommand(command_id);
    if (!accelerator)
      return false;
    *out_accelerator = *accelerator;
    return true;
  }
};

class ZoomLevelObserver {
 public:
  ZoomLevelObserver(AppMenuController* controller,
                    zoom::ZoomEventManager* manager)
      : controller_(controller) {
    subscription_ = manager->AddZoomLevelChangedCallback(
        base::Bind(&ZoomLevelObserver::OnZoomLevelChanged,
                   base::Unretained(this)));
  }

  ~ZoomLevelObserver() {}

 private:
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change) {
    AppMenuModel* appMenuModel = [controller_ appMenuModel];
    appMenuModel->UpdateZoomControls();
    const base::string16 level =
        appMenuModel->GetLabelForCommandId(IDC_ZOOM_PERCENT_DISPLAY);
    [[controller_ zoomDisplay] setTitle:base::SysUTF16ToNSString(level)];
  }

  std::unique_ptr<content::HostZoomMap::Subscription> subscription_;

  AppMenuController* controller_;  // Weak; owns this.

  DISALLOW_COPY_AND_ASSIGN(ZoomLevelObserver);
};

class ToolbarActionsBarObserverHelper : public ToolbarActionsBarObserver {
 public:
  ToolbarActionsBarObserverHelper(AppMenuController* controller,
                                  ToolbarActionsBar* toolbar_actions_bar)
      : controller_(controller),
        scoped_observer_(this),
        weak_ptr_factory_(this) {
    scoped_observer_.Add(toolbar_actions_bar);
  }
  ~ToolbarActionsBarObserverHelper() override {}

 private:
  // ToolbarActionsBarObserver:
  void OnToolbarActionsBarDestroyed() override {
    scoped_observer_.RemoveAll();
  }
  void OnToolbarActionsBarDidStartResize() override {
    // No point in having multiple pending update calls.
    weak_ptr_factory_.InvalidateWeakPtrs();
    // Edge case: If the resize is caused by an action being added while the
    // menu is open, we need to wait for both toolbars to be updated. This can
    // happen if a user's data is synced with the menu open.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ToolbarActionsBarObserverHelper::UpdateSubmenu,
                              weak_ptr_factory_.GetWeakPtr()));
  }

  void UpdateSubmenu() {
    [controller_ updateBrowserActionsSubmenu];
  }

  AppMenuController* controller_;
  ScopedObserver<ToolbarActionsBar, ToolbarActionsBarObserver> scoped_observer_;
  base::WeakPtrFactory<ToolbarActionsBarObserverHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBarObserverHelper);
};

}  // namespace AppMenuControllerInternal

@implementation AppMenuController

- (id)initWithBrowser:(Browser*)browser {
  if ((self = [super init])) {
    browser_ = browser;
    acceleratorDelegate_.reset(
        new AppMenuControllerInternal::AcceleratorDelegate());
    [self createModel];
  }
  return self;
}

- (void)dealloc {
  [self browserWillBeDestroyed];
  [super dealloc];
}

- (void)browserWillBeDestroyed {
  // This method indicates imminent destruction. Destroy owned objects that hold
  // a weak Browser*, or pass this call onto reference counted objects.
  recentTabsMenuModelDelegate_.reset();
  [self setModel:nullptr];
  appMenuModel_.reset();
  bookmarkMenuBridge_ = nullptr;
  buttonViewController_.reset();

  // The observers should most likely already be destroyed (since they're reset
  // in -menuDidClose:), but sometimes shutdown can be funny, so make sure to
  // not leave any dangling observers.
  zoom_level_observer_.reset();
  toolbar_actions_bar_observer_.reset();

  [browserActionsController_ browserWillBeDestroyed];

  browser_ = nullptr;
}

- (void)addItemToMenu:(NSMenu*)menu
              atIndex:(NSInteger)index
            fromModel:(ui::MenuModel*)model {
  // Non-button item types should be built as normal items, with the exception
  // of the extensions overflow menu.
  int command_id = model->GetCommandIdAt(index);
  if (model->GetTypeAt(index) != ui::MenuModel::TYPE_BUTTON_ITEM &&
      command_id != IDC_EXTENSIONS_OVERFLOW_MENU) {
    [super addItemToMenu:menu
                 atIndex:index
               fromModel:model];
    return;
  }

  // Handle the special-cased menu items.
  base::scoped_nsobject<NSMenuItem> customItem(
      [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""]);
  MenuTrackedRootView* view = nil;
  switch (command_id) {
    case IDC_EXTENSIONS_OVERFLOW_MENU: {
      browserActionsMenuItem_ = customItem.get();
      view = [buttonViewController_ toolbarActionsOverflowItem];
      BrowserActionsContainerView* containerView =
          [buttonViewController_ overflowActionsContainerView];

      // The overflow browser actions container can't function properly without
      // a main counterpart, so if the browser window hasn't initialized, abort.
      // (This is fine because we re-populate the app menu each time before we
      // show it.)
      if (!browser_->window())
        break;

      BrowserActionsController* mainController =
          [[[BrowserWindowController browserWindowControllerForWindow:browser_->
              window()->GetNativeWindow()] toolbarController]
                  browserActionsController];
      toolbar_actions_bar_observer_.reset(
          new AppMenuControllerInternal::ToolbarActionsBarObserverHelper(
              self, [mainController toolbarActionsBar]));
      browserActionsController_.reset(
          [[BrowserActionsController alloc]
              initWithBrowser:browser_
                containerView:containerView
               mainController:mainController]);
      break;
    }
    case IDC_EDIT_MENU:
      view = [buttonViewController_ editItem];
      break;
    case IDC_ZOOM_MENU:
      view = [buttonViewController_ zoomItem];
      break;
    default:
      NOTREACHED();
      break;
  }
  DCHECK(view);
  [customItem setView:view];
  [view setMenuItem:customItem];
  [self adjustPositioning];
  [menu insertItem:customItem.get() atIndex:index];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  const BOOL enabled = [super validateUserInterfaceItem:item];

  NSMenuItem* menuItem = (id)item;
  ui::MenuModel* model =
      static_cast<ui::MenuModel*>(
          [[menuItem representedObject] pointerValue]);

  // The section headers in the recent tabs submenu should be bold and black if
  // a font list is specified for the items (bold is already applied in the
  // |MenuControllerCocoa| as the font list returned by |GetLabelFontListAt| is
  // bold).
  if (model && model == [self recentTabsMenuModel]) {
    if (model->GetLabelFontListAt([item tag])) {
      DCHECK([menuItem attributedTitle]);
      base::scoped_nsobject<NSMutableAttributedString> title(
          [[NSMutableAttributedString alloc]
              initWithAttributedString:[menuItem attributedTitle]]);
      [title addAttribute:NSForegroundColorAttributeName
                    value:[NSColor blackColor]
                    range:NSMakeRange(0, [title length])];
      [menuItem setAttributedTitle:title.get()];
    } else {
      // Not a section header. Add a tooltip with the title and the URL.
      std::string url;
      base::string16 title;
      if ([self recentTabsMenuModel]->GetURLAndTitleForItemAtIndex(
              [item tag], &url, &title)) {
        [menuItem setToolTip:
            cocoa_l10n_util::TooltipForURLAndTitle(
                base::SysUTF8ToNSString(url), base::SysUTF16ToNSString(title))];
       }
    }
  }

  return enabled;
}

- (NSMenu*)bookmarkSubMenu {
  NSString* title = l10n_util::GetNSStringWithFixup(IDS_BOOKMARKS_MENU);
  return [[[self menu] itemWithTitle:title] submenu];
}

- (void)updateBookmarkSubMenu {
  NSMenu* bookmarkMenu = [self bookmarkSubMenu];
  if (!bookmarkMenu)
    return;  // Guest profiles have no bookmarks menu.

  // TODO(tapted): This should be cached and shared between browser windows in
  // the same profile at least. Better would be to key it to the profile and
  // share it with the main menu and the bookmarks toolbar as well. Sadly, it
  // can't even be easily cached on |self|. This is because the first 5 items in
  // the submenu are tied to a MenuControllerCocoa target via a raw pointer,
  // which can't be reused across menu invocations.
  bookmarkMenuBridge_ = std::make_unique<BookmarkMenuBridge>(
      [self appMenuModel]->browser()->profile(), bookmarkMenu);

  // Note |bookmarkMenuBridge_| is useless after the menu closes, but it must
  // exist when (and if) the menu action is sent. Unfortunately, AppKit sends
  // menu actions after menuDidClose: and NSMenuDidEndTrackingNotification so
  // |bookmarkMenuBridge_| is going to hang around until updateBookmarkSubMenu
  // is called again.
}

- (void)updateBrowserActionsSubmenu {
  MenuTrackedRootView* view =
      [buttonViewController_ toolbarActionsOverflowItem];
  BrowserActionsContainerView* containerView =
      [buttonViewController_ overflowActionsContainerView];
  [containerView setHidden:[browserActionsController_ buttonCount] == 0];

  // Find the preferred container size for the menu width.
  int menuWidth = [[self menu] size].width;
  int maxContainerWidth = menuWidth - kLeftPadding - kRightPadding;
  // Don't let the menu change sizes on us. (We lift this restriction every time
  // the menu updates, so if something changes, this won't leave us with an
  // awkward size.)
  [[self menu] setMinimumWidth:menuWidth];
  gfx::Size preferredContainerSize =
      [browserActionsController_ sizeForOverflowWidth:maxContainerWidth];

  // Set the origins and preferred size for the container.
  // View hierarchy is as follows (from parent > child):
  // |view| > |anonymous view| > containerView. We have to set the origin
  // and size of each for it display properly.
  // The parent views each have a size of the full width of the menu, so we can
  // properly position the container.
  NSSize parentSize = NSMakeSize(menuWidth,
                                 std::min(preferredContainerSize.height(),
                                          kMaxOverflowContainerHeight));
  [view setFrameSize:parentSize];
  [[containerView superview] setFrameSize:parentSize];

  // The container view gets its preferred size.
  [containerView setFrameSize:NSMakeSize(preferredContainerSize.width(),
                                         preferredContainerSize.height())];
  [browserActionsController_ update];

  [view setFrameOrigin:NSZeroPoint];
  [[containerView superview] setFrameOrigin:NSZeroPoint];
  [containerView setFrameOrigin:NSMakePoint(kLeftPadding, 0)];
}

- (void)menuWillOpen:(NSMenu*)menu {
  [super menuWillOpen:menu];

  zoom_level_observer_.reset(new AppMenuControllerInternal::ZoomLevelObserver(
      self, zoom::ZoomEventManager::GetForBrowserContext(browser_->profile())));
  NSString* title = base::SysUTF16ToNSString(
      [self appMenuModel]->GetLabelForCommandId(IDC_ZOOM_PERCENT_DISPLAY));
  [[[buttonViewController_ zoomItem] viewWithTag:IDC_ZOOM_PERCENT_DISPLAY]
      setTitle:title];
  [[[[buttonViewController_ zoomItem]
      viewWithTag:IDC_ZOOM_MINUS] image]
          setAccessibilityDescription:l10n_util::GetNSString(
              IDS_TEXT_SMALLER_MAC)];
  [[[[buttonViewController_ zoomItem]
      viewWithTag:IDC_ZOOM_PLUS] image]
        setAccessibilityDescription:l10n_util::GetNSString(
              IDS_TEXT_BIGGER_MAC)];
  base::RecordAction(UserMetricsAction("ShowAppMenu"));

  NSImage* icon = [self appMenuModel]->browser()->window()->IsFullscreen()
                      ? [NSImage imageNamed:NSImageNameExitFullScreenTemplate]
                      : [NSImage imageNamed:NSImageNameEnterFullScreenTemplate];
  [[buttonViewController_ zoomFullScreen] setImage:icon];

  menuOpenTime_ = base::TimeTicks::Now();

  BrowserWindowController* bwc = [BrowserWindowController
      browserWindowControllerForWindow:browser_->window()->GetNativeWindow()];

  AppToolbarButton* appMenuButton =
      static_cast<AppToolbarButton*>([[bwc toolbarController] appMenuButton]);
  [appMenuButton animateIfPossibleWithDelay:NO];
}

- (void)menuDidClose:(NSMenu*)menu {
  [super menuDidClose:menu];

  // We don't need to observe changes to zoom or toolbar size when the menu is
  // closed, since we instantiate it with the proper value and recreate the menu
  // on each show. (We do this in -menuNeedsUpdate:, which is called when the
  // menu is about to be displayed at the start of a tracking session.)
  zoom_level_observer_.reset();
  toolbar_actions_bar_observer_.reset();
  // Make sure to reset() the BrowserActionsController since the view will also
  // be destroyed. If a new one's needed, it'll be created when we create the
  // model in -menuNeedsUpdate:.
  browserActionsController_.reset();
  UMA_HISTOGRAM_TIMES("Toolbar.AppMenuTimeToAction",
                      base::TimeTicks::Now() - menuOpenTime_);
  menuOpenTime_ = base::TimeTicks();
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
  // We should never have a BrowserActionsController before creating the menu.
  DCHECK(!browserActionsController_.get());

  // First empty out the menu and create a new model.
  [menu removeAllItems];
  [self createModel];
  [menu setMinimumWidth:0];

  // Create a new menu, which cannot be swapped because the tracking is about to
  // start, so simply copy the items.
  NSMenu* newMenu = [self menuFromModel:model_];
  NSArray* itemArray = [newMenu itemArray];
  [newMenu removeAllItems];
  for (NSMenuItem* item in itemArray) {
    [menu addItem:item];
  }

  [self updateRecentTabsSubmenu];
  [self updateBookmarkSubMenu];
  [self updateBrowserActionsSubmenu];
}

// Used to dispatch commands from the App menu. The custom items within the
// menu cannot be hooked up directly to First Responder because the window in
// which the controls reside is not the BrowserWindowController, but a
// NSCarbonMenuWindow; this screws up the typical |-commandDispatch:| system.
- (IBAction)dispatchAppMenuCommand:(id)sender {
  NSInteger tag = [sender tag];
  if (sender == [buttonViewController_ zoomPlus] ||
      sender == [buttonViewController_ zoomMinus]) {
    // Do a direct dispatch rather than scheduling on the outermost run loop,
    // which would not get hit until after the menu had closed.
    [self performCommandDispatch:[NSNumber numberWithInt:tag]];

    // The zoom buttons should not close the menu if opened sticky.
    if ([sender respondsToSelector:@selector(isTracking)] &&
        [sender performSelector:@selector(isTracking)]) {
      [menu_ cancelTracking];
    }
  } else {
    // The custom views within the App menu are abnormal and keep the menu open
    // after a target-action.  Close the menu manually.
    [menu_ cancelTracking];

    // Executing certain commands from the nested run loop of the menu can lead
    // to wonky behavior (e.g. http://crbug.com/49716). To avoid this, schedule
    // the dispatch on the outermost run loop.
    [self performSelector:@selector(performCommandDispatch:)
               withObject:[NSNumber numberWithInt:tag]
               afterDelay:0.0];
  }
}

// Used to perform the actual dispatch on the outermost runloop.
- (void)performCommandDispatch:(NSNumber*)tag {
  [self appMenuModel]->ExecuteCommand([tag intValue], 0);
}

- (AppMenuModel*)appMenuModel {
  // Don't use |appMenuModel_| so that a test can override the generic one.
  return static_cast<AppMenuModel*>(model_);
}

- (void)updateRecentTabsSubmenu {
  ui::MenuModel* model = [self recentTabsMenuModel];
  if (model) {
    recentTabsMenuModelDelegate_.reset(
        new RecentTabsMenuModelDelegate(model, [self recentTabsSubmenu]));
  }
}

- (BrowserActionsController*)browserActionsController {
  return browserActionsController_.get();
}

- (void)createModel {
  DCHECK(browser_);
  recentTabsMenuModelDelegate_.reset();
  appMenuModel_.reset(new AppMenuModel(acceleratorDelegate_.get(), browser_));
  appMenuModel_->Init();
  [self setModel:appMenuModel_.get()];

  buttonViewController_.reset(
      [[AppMenuButtonViewController alloc] initWithController:self]);
  [buttonViewController_ view];

  // See comment in containerSuperviewFrameChanged:.
  NSView* containerSuperview =
      [[buttonViewController_ overflowActionsContainerView] superview];
  [containerSuperview setPostsFrameChangedNotifications:YES];
}

// Fit the localized strings into the Cut/Copy/Paste control, then resize the
// whole menu item accordingly.
- (void)adjustPositioning {
  const CGFloat kButtonPadding = 12;
  CGFloat delta = 0;

  // Go through the three buttons from right-to-left, adjusting the size to fit
  // the localized strings while keeping them all aligned on their horizontal
  // edges.
  NSButton* views[] = {
      [buttonViewController_ editPaste],
      [buttonViewController_ editCopy],
      [buttonViewController_ editCut]
  };
  for (size_t i = 0; i < arraysize(views); ++i) {
    NSButton* button = views[i];
    CGFloat originalWidth = NSWidth([button frame]);

    // Do not let |-sizeToFit| change the height of the button.
    NSSize size = [button frame].size;
    [button sizeToFit];
    size.width = [button frame].size.width + kButtonPadding;
    [button setFrameSize:size];

    CGFloat newWidth = size.width;
    delta += newWidth - originalWidth;

    NSRect frame = [button frame];
    frame.origin.x -= delta;
    [button setFrame:frame];
  }

  // Resize the menu item by the total amound the buttons changed so that the
  // spacing between the buttons and the title remains the same.
  NSRect itemFrame = [[buttonViewController_ editItem] frame];
  itemFrame.size.width += delta;
  [[buttonViewController_ editItem] setFrame:itemFrame];

  // Also resize the superview of the buttons, which is an NSView used to slide
  // when the item title is too big and GTM resizes it.
  NSRect parentFrame = [[[buttonViewController_ editCut] superview] frame];
  parentFrame.size.width += delta;
  parentFrame.origin.x -= delta;
  [[[buttonViewController_ editCut] superview] setFrame:parentFrame];
}

- (NSButton*)zoomDisplay {
  return [buttonViewController_ zoomDisplay];
}

- (void)menu:(NSMenu*)menu willHighlightItem:(NSMenuItem*)item {
  if (browserActionsController_.get()) {
    [browserActionsController_ setFocusedInOverflow:
        (item == browserActionsMenuItem_)];
  }
}

- (NSMenu*)recentTabsSubmenu {
  NSString* title = l10n_util::GetNSStringWithFixup(IDS_RECENT_TABS_MENU);
  return [[[self menu] itemWithTitle:title] submenu];
}

// The recent tabs menu model is recognized by the existence of either the
// kRecentlyClosedHeaderCommandId or the kDisabledRecentlyClosedHeaderCommandId.
- (RecentTabsSubMenuModel*)recentTabsMenuModel {
  int index = 0;
  // Start searching at the app menu model level, |model| will be updated only
  // if the command we're looking for is found in one of the [sub]menus.
  ui::MenuModel* model = [self appMenuModel];
  if (ui::MenuModel::GetModelAndIndexForCommandId(
          RecentTabsSubMenuModel::kRecentlyClosedHeaderCommandId, &model,
          &index)) {
    return static_cast<RecentTabsSubMenuModel*>(model);
  }
  if (ui::MenuModel::GetModelAndIndexForCommandId(
          RecentTabsSubMenuModel::kDisabledRecentlyClosedHeaderCommandId,
          &model, &index)) {
    return static_cast<RecentTabsSubMenuModel*>(model);
  }
  return NULL;
}

// This overrdies the parent class to return a custom width for recent tabs
// menu.
- (int)maxWidthForMenuModel:(ui::MenuModel*)model
                 modelIndex:(int)modelIndex {
  RecentTabsSubMenuModel* recentTabsMenuModel = [self recentTabsMenuModel];
  if (recentTabsMenuModel && recentTabsMenuModel == model) {
    return recentTabsMenuModel->GetMaxWidthForItemAtIndex(modelIndex);
  }
  return -1;
}

@end  // @implementation AppMenuController

////////////////////////////////////////////////////////////////////////////////

@interface AppMenuButtonViewController ()
- (void)containerSuperviewFrameChanged:(NSNotification*)notification;
@end

@implementation AppMenuButtonViewController

@synthesize editItem = editItem_;
@synthesize editCut = editCut_;
@synthesize editCopy = editCopy_;
@synthesize editPaste = editPaste_;
@synthesize zoomItem = zoomItem_;
@synthesize zoomPlus = zoomPlus_;
@synthesize zoomDisplay = zoomDisplay_;
@synthesize zoomMinus = zoomMinus_;
@synthesize zoomFullScreen = zoomFullScreen_;
@synthesize toolbarActionsOverflowItem = toolbarActionsOverflowItem_;
@synthesize overflowActionsContainerView = overflowActionsContainerView_;

- (id)initWithController:(AppMenuController*)controller {
  if ((self = [super initWithNibName:@"AppMenu"
                              bundle:base::mac::FrameworkBundle()])) {
    controller_ = controller;
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(containerSuperviewFrameChanged:)
               name:NSViewFrameDidChangeNotification
             object:[overflowActionsContainerView_ superview]];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  base::mac::ReleaseProperties(self);
  [super dealloc];
}

- (IBAction)dispatchAppMenuCommand:(id)sender {
  [controller_ dispatchAppMenuCommand:sender];
}

- (void)containerSuperviewFrameChanged:(NSNotification*)notification {
  // AppKit menus were probably never designed with a view like the browser
  // actions container in mind, and, as a result, we come across a few oddities.
  // One of these is that the container's superview will, on some versions of
  // OSX, change frame position sometime after the the menu begins tracking
  // (and thus, after all our ability to adjust it normally). Throw in the
  // towel, and simply don't let the frame move from where it's supposed to be.
  // TODO(devlin): Yet another Cocoa hack. It'd be good to find a workaround,
  // but unlikely unless we replace the Cocoa menu implementation.
  NSView* containerSuperview = [overflowActionsContainerView_ superview];
  if (NSMinX([containerSuperview frame]) != 0)
    [containerSuperview setFrameOrigin:NSZeroPoint];
}

@end  // @implementation AppMenuButtonViewController
