// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_controller.h"

#include "base/ios/block_types.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service_helper.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#import "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/feature_engagement/tracker_util.h"
#import "ios/chrome/browser/metrics/tab_usage_recorder.h"
#include "ios/chrome/browser/sessions/ios_chrome_tab_restore_service_factory.h"
#include "ios/chrome/browser/sessions/session_util.h"
#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios.h"
#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_consumer.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"
#include "ios/chrome/browser/ui/commands/application_commands.h"
#include "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"
#import "ios/chrome/browser/ui/commands/show_signin_command.h"
#import "ios/chrome/browser/ui/keyboard/UIKeyCommand+Chrome.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_off_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_on_no_sessions_view.h"
#import "ios/chrome/browser/ui/settings/sync_utils/sync_presenter.h"
#import "ios/chrome/browser/ui/signin_interaction/public/signin_presenter.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_cache.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_header_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_controller.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_overlay_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_cell_data.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_transition_context.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_view.h"
#import "ios/chrome/browser/ui/tabs/requirements/tab_strip_fold_animation.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/referrer.h"
#import "ios/web/public/web_state/web_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Offsets for computing the panels' indexes in the TabSwitcherView.
const int kSignInPromoPanelIndex = 2;
const int kHeaderDistantSessionIndexOffset = 2;
const int kLocalTabsOffTheRecordPanelIndex = 0;
const int kLocalTabsOnTheRecordPanelIndex = 1;
// The duration of the tab switcher toggle animation.
const CGFloat kTransitionAnimationDuration = 0.25;
// The height of the browser view controller header.
const CGFloat kHeaderHeight = 39;

enum class TabModelType {
  MODEL_MAIN,
  MODEL_OTR,
};

enum class TransitionType {
  TRANSITION_PRESENT,
  TRANSITION_DISMISS,
};

enum class SnapshotViewOption {
  SNAPSHOT_VIEW,     // Try taking a snapshot view if possible.
  CLIENT_RENDERING,  // Use [UIView renderInContext:] to render a bitmap and set
                     // it as the backing store of a view's layer.
};

}  // namespace

@interface TabSwitcherController ()<SigninPresenter,
                                    SigninPromoViewConsumer,
                                    SyncPresenter,
                                    TabSwitcherModelDelegate,
                                    TabSwitcherViewDelegate,
                                    TabSwitcherHeaderViewDelegate,
                                    TabSwitcherHeaderViewDataSource,
                                    TabSwitcherPanelControllerDelegate,
                                    TabSwitcherPanelOverlayViewDelegate> {
  // weak.
  ios::ChromeBrowserState* _browserState;
  // weak.
  __weak id<TabSwitcherDelegate> _delegate;
  // The type of the model that was selected when the tab switcher was toggled.
  // Callers can retrieve the associated TabModel with |[self
  // onLoadActiveModel]|.
  TabModelType _onLoadActiveModelType;
  // The view this controller manages.
  TabSwitcherView* _tabSwitcherView;
  // The list of panels controllers for distant sessions.
  NSMutableArray* _controllersOfDistantSessions;
  // The panel controllers for the local sessions.
  TabSwitcherPanelController* _onTheRecordSession;
  TabSwitcherPanelController* _offTheRecordSession;
  // The model storing the state of what is shown by the tab switcher.
  TabSwitcherModel* _tabSwitcherModel;
  // Stores the current sign-in panel type.
  TabSwitcherSignInPanelsType _signInPanelType;
  // Stores the sign-in panel overlay view.
  TabSwitcherPanelOverlayView* _signInPanelOverlayView;
  // Cache for the panel's cells.
  TabSwitcherCache* _cache;
  // Stores the background color of the window when the tab switcher was
  // presented.
  UIColor* _initialWindowBackgroundColor;
  // Indicate whether a previous promo panel header cell should be removed or
  // added.
  BOOL _shouldRemovePromoPanelHeaderCell;
  BOOL _shouldAddPromoPanelHeaderCell;
  // Handles command dispatching.
  CommandDispatcher* _dispatcher;
  // Sign-in promo view mediator for the "Other Devices" tab.
  SigninPromoViewMediator* _signinPromoViewMediator;
}

// Updates the window background color to the tab switcher's background color.
// The original background color can be restored by calling
//  -restoreWindowBackgroundColor.
- (void)updateWindowBackgroundColor;

// Restores the tab switcher's window background color to the value it had
// before presenting it.
- (void)restoreWindowBackgroundColor;

// Performs the tab switcher transition according to the |transitionType| and
// call the completion block at the end of the transition. The transition will
// be animated according to the |animated| parameter. |completion| block must
// not be nil.
- (void)performTabSwitcherTransition:(TransitionType)transitionType
                           withModel:(TabModel*)tabModel
                            animated:(BOOL)animated
                      withCompletion:(ProceduralBlock)completion;

// Returns the index of the currently selected panel.
- (NSInteger)currentPanelIndex;

// Returns the session type of the panel and the given index.
- (TabSwitcherSessionType)sessionTypeForPanelIndex:(NSInteger)panelIndex;

// Returns the tab model corresponding to the given session type.
// There is no tab model for distant sessions so it returns nil for distant
// sessions type.
- (TabModel*)tabModelForSessionType:(TabSwitcherSessionType)sessionType;

// Returns the tab model of the currently selected tab.
- (TabModel*)currentSelectedModel;

// Returns the tab model that was active when the tab switcher was toggled.
- (TabModel*)onLoadActiveModel;

// Dismisses the tab switcher using the currently selected tab's tab model.
- (void)tabSwitcherDismissWithCurrentSelectedModel;

// Scrolls the scrollview to show the panel displaying the |selectedTabModel|.
- (void)selectPanelForTabModel:(TabModel*)selectedTabModel;

// Dismisses the tab switcher and create a new tab using the url, position and
// transition on the given tab model.
- (Tab*)dismissWithNewTabAnimation:(const GURL&)url
                           atIndex:(NSUInteger)position
                        transition:(ui::PageTransition)transition
                          tabModel:(TabModel*)tabModel;
// Add a promo panel corresponding to the panel type argument.
// Should only be called from inititalizer and signInPanelChangedTo:.
- (void)addPromoPanelForSignInPanelType:(TabSwitcherSignInPanelsType)panelType;

// Updates cells of local panels.
- (void)updateLocalPanelsCells;

@end

@implementation TabSwitcherController

@synthesize animationDelegate = _animationDelegate;
@synthesize transitionContext = _transitionContext;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                        mainTabModel:(TabModel*)mainTabModel
                         otrTabModel:(TabModel*)otrTabModel
                      activeTabModel:(TabModel*)activeTabModel
          applicationCommandEndpoint:(id<ApplicationCommands>)endpoint {
  DCHECK(mainTabModel);
  DCHECK(otrTabModel);
  DCHECK(activeTabModel == otrTabModel || activeTabModel == mainTabModel);
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _browserState = browserState;

    _dispatcher = [[CommandDispatcher alloc] init];
    [_dispatcher startDispatchingToTarget:self
                              forProtocol:@protocol(BrowserCommands)];
    [_dispatcher startDispatchingToTarget:endpoint
                              forProtocol:@protocol(ApplicationCommands)];
    // -startDispatchingToTarget:forProtocol: doesn't pick up protocols the
    // passed protocol conforms to, so ApplicationSettingsCommands is explicitly
    // dispatched to the endpoint as well. Since this is potentially
    // fragile, DCHECK that it should still work (if the endpoint is nonnull).
    DCHECK(
        !endpoint ||
        [endpoint conformsToProtocol:@protocol(ApplicationSettingsCommands)]);
    [_dispatcher
        startDispatchingToTarget:endpoint
                     forProtocol:@protocol(ApplicationSettingsCommands)];

    // self.dispatcher shouldn't be used in this init method, so duplicate the
    // typecast to pass dispatcher into child objects.
    id<ApplicationCommands, BrowserCommands> passableDispatcher =
        static_cast<id<ApplicationCommands, BrowserCommands>>(_dispatcher);

    _onLoadActiveModelType = (activeTabModel == mainTabModel)
                                 ? TabModelType::MODEL_MAIN
                                 : TabModelType::MODEL_OTR;
    _cache = [[TabSwitcherCache alloc] init];
    [_cache setMainTabModel:mainTabModel otrTabModel:otrTabModel];
    _tabSwitcherModel =
        [[TabSwitcherModel alloc] initWithBrowserState:browserState
                                              delegate:self
                                          mainTabModel:mainTabModel
                                           otrTabModel:otrTabModel
                                             withCache:_cache];
    _controllersOfDistantSessions = [[NSMutableArray alloc] init];
    [self loadTabSwitcherView];
    _onTheRecordSession = [[TabSwitcherPanelController alloc]
                initWithModel:_tabSwitcherModel
        forLocalSessionOfType:TabSwitcherSessionType::REGULAR_SESSION
                    withCache:_cache
                 browserState:_browserState
                    presenter:self /* id<SigninPresenter, SyncPresenter> */
                   dispatcher:passableDispatcher];
    [_onTheRecordSession setDelegate:self];
    _offTheRecordSession = [[TabSwitcherPanelController alloc]
                initWithModel:_tabSwitcherModel
        forLocalSessionOfType:TabSwitcherSessionType::OFF_THE_RECORD_SESSION
                    withCache:_cache
                 browserState:_browserState
                    presenter:self /* id<SigninPresenter, SyncPresenter> */
                   dispatcher:passableDispatcher];
    [_offTheRecordSession setDelegate:self];
    [_tabSwitcherView addPanelView:[_offTheRecordSession view]
                           atIndex:kLocalTabsOffTheRecordPanelIndex];
    [_tabSwitcherView addPanelView:[_onTheRecordSession view]
                           atIndex:kLocalTabsOnTheRecordPanelIndex];
    [self addPromoPanelForSignInPanelType:[_tabSwitcherModel signInPanelType]];
    [[_tabSwitcherView headerView] reloadData];
    [_tabSwitcherModel syncedSessionsChanged];
    [self selectPanelForTabModel:[self onLoadActiveModel]];
  }
  return self;
}

#pragma mark - UIViewController

- (BOOL)prefersStatusBarHidden {
  return NO;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return UIStatusBarStyleLightContent;
}

- (CGRect)tabSwitcherInitialFrame {
  return [[UIScreen mainScreen] bounds];
}

- (void)loadTabSwitcherView {
  DCHECK(![_tabSwitcherView superview]);
  _tabSwitcherView =
      [[TabSwitcherView alloc] initWithFrame:[self tabSwitcherInitialFrame]];
  [_tabSwitcherView setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                        UIViewAutoresizingFlexibleHeight];
  [_tabSwitcherView setDelegate:self];
  [[_tabSwitcherView headerView] setDelegate:self];
  [[_tabSwitcherView headerView] setDataSource:self];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self.view setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                 UIViewAutoresizingFlexibleHeight];
  [_tabSwitcherView setFrame:self.view.bounds];
  [self.view addSubview:_tabSwitcherView];
}

#pragma mark - UIResponder

- (NSArray*)keyCommands {
  __weak TabSwitcherController* weakSelf = self;
  return @[
    [UIKeyCommand
        cr_keyCommandWithInput:@"t"
                 modifierFlags:UIKeyModifierCommand
                         title:l10n_util::GetNSStringWithFixup(
                                   IDS_IOS_TOOLS_MENU_NEW_TAB)
                        action:^{
                          TabSwitcherController* strongSelf = weakSelf;
                          if (!strongSelf)
                            return;
                          if ([strongSelf currentPanelIndex] ==
                              kLocalTabsOffTheRecordPanelIndex) {
                            [strongSelf openNewTabInPanelAtIndex:
                                            kLocalTabsOffTheRecordPanelIndex];
                          } else {
                            [strongSelf openNewTabInPanelAtIndex:
                                            kLocalTabsOnTheRecordPanelIndex];
                          }
                        }],
    [UIKeyCommand
        cr_keyCommandWithInput:@"n"
                 modifierFlags:UIKeyModifierCommand | UIKeyModifierShift
                         title:l10n_util::GetNSStringWithFixup(
                                   IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB)
                        action:^{
                          [weakSelf openNewTabInPanelAtIndex:
                                        kLocalTabsOffTheRecordPanelIndex];
                        }],
    [UIKeyCommand
        cr_keyCommandWithInput:@"t"
                 modifierFlags:UIKeyModifierCommand | UIKeyModifierShift
                         title:l10n_util::GetNSStringWithFixup(
                                   IDS_IOS_KEYBOARD_REOPEN_CLOSED_TAB)
                        action:^{
                          [weakSelf reopenClosedTab];
                        }],
    [UIKeyCommand
        cr_keyCommandWithInput:@"n"
                 modifierFlags:UIKeyModifierCommand
                         title:nil
                        action:^{
                          TabSwitcherController* strongSelf = weakSelf;
                          if (!strongSelf)
                            return;
                          if ([strongSelf currentPanelIndex] ==
                              kLocalTabsOffTheRecordPanelIndex) {
                            [strongSelf openNewTabInPanelAtIndex:
                                            kLocalTabsOffTheRecordPanelIndex];
                          } else {
                            [strongSelf openNewTabInPanelAtIndex:
                                            kLocalTabsOnTheRecordPanelIndex];
                          }
                        }],
  ];
}

#pragma mark - TabSwitcher protocol implementation

- (void)restoreInternalStateWithMainTabModel:(TabModel*)mainModel
                                 otrTabModel:(TabModel*)otrModel
                              activeTabModel:(TabModel*)activeModel {
  DCHECK(mainModel);
  DCHECK(otrModel);
  DCHECK(activeModel == mainModel || activeModel == otrModel);
  _onLoadActiveModelType = (activeModel == mainModel) ? TabModelType::MODEL_MAIN
                                                      : TabModelType::MODEL_OTR;
  [_cache setMainTabModel:mainModel otrTabModel:otrModel];
  [_tabSwitcherModel setMainTabModel:mainModel otrTabModel:otrModel];
  [self selectPanelForTabModel:activeModel];
}

- (UIViewController*)viewController {
  return self;
}

- (void)setOtrTabModel:(TabModel*)otrModel {
  [_cache setMainTabModel:[_cache mainTabModel] otrTabModel:otrModel];
  [_tabSwitcherModel setMainTabModel:[_tabSwitcherModel mainTabModel]
                         otrTabModel:otrModel];
}

- (void)showWithSelectedTabAnimation {
  [self updateWindowBackgroundColor];
  [self performTabSwitcherTransition:TransitionType::TRANSITION_PRESENT
                           withModel:[self onLoadActiveModel]
                            animated:YES
                      withCompletion:^{
                        [self.animationDelegate
                            tabSwitcherPresentationAnimationDidEnd:self];
                        [_tabSwitcherView wasShown];
                      }];
}

- (Tab*)dismissWithNewTabAnimationToModel:(TabModel*)targetModel
                                  withURL:(const GURL&)url
                                  atIndex:(NSUInteger)position
                               transition:(ui::PageTransition)transition {
  if (targetModel == [_tabSwitcherModel mainTabModel])
    [_tabSwitcherView selectPanelAtIndex:kLocalTabsOnTheRecordPanelIndex];
  else
    [_tabSwitcherView selectPanelAtIndex:kLocalTabsOffTheRecordPanelIndex];
  return [self dismissWithNewTabAnimation:url
                                  atIndex:position
                               transition:transition
                                 tabModel:targetModel];
}

- (void)setDelegate:(id<TabSwitcherDelegate>)delegate {
  _delegate = delegate;
  if (delegate == nullptr)
    [_tabSwitcherModel setMainTabModel:nil otrTabModel:nil];
}

- (id<TabSwitcherDelegate>)delegate {
  return _delegate;
}

- (id<ApplicationCommands, BrowserCommands>)dispatcher {
  return static_cast<id<ApplicationCommands, BrowserCommands>>(_dispatcher);
}

- (void)prepareForDisplayAtSize:(CGSize)size {
  // No-op.
}

#pragma mark - BrowserCommands

- (void)openNewTab:(OpenNewTabCommand*)command {
  // Ensure that the right mode is showing.
  NSInteger panelIndex = command.incognito ? kLocalTabsOffTheRecordPanelIndex
                                           : kLocalTabsOnTheRecordPanelIndex;
  [_tabSwitcherView selectPanelAtIndex:panelIndex];

  const TabSwitcherSessionType panelSessionType =
      (command.incognito) ? TabSwitcherSessionType::OFF_THE_RECORD_SESSION
                          : TabSwitcherSessionType::REGULAR_SESSION;

  TabModel* model = [self tabModelForSessionType:panelSessionType];

  // Either send or don't send the "New Tab Opened" or "Incognito Tab Opened" to
  // the feature_engageament::Tracker based on |command.userInitiated| and
  // |command.incognito|.
  feature_engagement::NotifyNewTabEventForCommand(model.browserState, command);

  [self dismissWithNewTabAnimation:GURL(kChromeUINewTabURL)
                           atIndex:NSNotFound
                        transition:ui::PAGE_TRANSITION_TYPED
                          tabModel:model];
}

#pragma mark - Private

- (void)updateWindowBackgroundColor {
  DCHECK(!_initialWindowBackgroundColor);
  _initialWindowBackgroundColor = self.view.window.backgroundColor;
  self.view.window.backgroundColor = [[MDCPalette greyPalette] tint900];
}

- (void)restoreWindowBackgroundColor {
  self.view.window.backgroundColor = _initialWindowBackgroundColor;
  _initialWindowBackgroundColor = nil;
}

- (UIView*)snapshotViewForView:(UIView*)inputView
                     withModel:(TabModel*)tabModel
                        option:(SnapshotViewOption)option {
  if (inputView) {
    if (option == SnapshotViewOption::SNAPSHOT_VIEW) {
      UIView* view = [inputView snapshotViewAfterScreenUpdates:NO];
      if (view)
        return view;
    }
    // If the view has just been created, it has not been rendered by Core
    // Animation and the snapshot view can't be generated. In that case we
    // trigger a client side rendering of the view and use the rendered image
    // as the backing store of a view layer.
    UIGraphicsBeginImageContextWithOptions(inputView.bounds.size,
                                           inputView.opaque, 0);
    [inputView.layer renderInContext:UIGraphicsGetCurrentContext()];
    UIImage* screenshot = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    UIView* view = [[UIView alloc] initWithFrame:CGRectZero];
    [view layer].contents = static_cast<id>(screenshot.CGImage);
    return view;
  } else {
    // When the input view is nil, we can't generate a snapshot so a placeholder
    // is returned.
    UIColor* backgroundColor = [tabModel isOffTheRecord]
                                   ? [[MDCPalette greyPalette] tint700]
                                   : [[MDCPalette greyPalette] tint100];
    UIView* placeholdView = [[UIView alloc] initWithFrame:CGRectZero];
    placeholdView.backgroundColor = backgroundColor;
    return placeholdView;
  }
}

- (TabSwitcherTransitionContextContent*)transitionContextContentForTabModel:
    (TabModel*)tabModel {
  if ([tabModel isOffTheRecord])
    return self.transitionContext.incognitoContent;
  else
    return self.transitionContext.regularContent;
}

- (UIImage*)updateScreenshotForCellIfNeeded:(TabSwitcherLocalSessionCell*)cell
                                   tabModel:(TabModel*)tabModel {
  if (cell.snapshot)
    return cell.snapshot;
  UIColor* backgroundColor = [tabModel isOffTheRecord]
                                 ? [[MDCPalette greyPalette] tint700]
                                 : [UIColor whiteColor];
  CGRect rect = CGRectMake(0, 0, 1, 1);
  UIGraphicsBeginImageContextWithOptions(rect.size, YES, 0);
  [backgroundColor setFill];
  CGContextFillRect(UIGraphicsGetCurrentContext(), rect);
  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return image;
}

- (void)performTabSwitcherTransition:(TransitionType)transitionType
                           withModel:(TabModel*)tabModel
                            animated:(BOOL)animated
                      withCompletion:(ProceduralBlock)completion {
  switch (transitionType) {
    case TransitionType::TRANSITION_PRESENT:
      base::RecordAction(base::UserMetricsAction("MobileTabSwitcherPresented"));
      break;
    case TransitionType::TRANSITION_DISMISS:
      base::RecordAction(base::UserMetricsAction("MobileTabSwitcherDismissed"));
      break;
  }
  DCHECK(tabModel);
  DCHECK(completion);
  DCHECK([self transitionContext]);
  [[self view] setUserInteractionEnabled:NO];

  TabSwitcherTransitionContextContent* transitionContextContent =
      [self transitionContextContentForTabModel:tabModel];
  DCHECK(transitionContextContent);

  Tab* selectedTab = [tabModel currentTab];

  NSInteger selectedTabIndex = [tabModel indexOfTab:selectedTab];
  TabSwitcherLocalSessionCell* selectedCell = nil;
  CGRect selectedCellFrame = CGRectZero;
  TabSwitcherPanelController* selectedPanel =
      [self panelControllerForTabModel:tabModel];
  // Force invalidation and update of the tab switcher view and collection view
  // layout to take into account any changes in the size of the view controller
  // while the tab switcher was not shown.
  if (transitionType == TransitionType::TRANSITION_PRESENT) {
    [[self view] layoutIfNeeded];
    [[selectedPanel collectionView].collectionViewLayout invalidateLayout];
  }
  if (selectedTabIndex != NSNotFound) {
    selectedCell =
        [selectedPanel localSessionCellForTabAtIndex:selectedTabIndex];
    selectedCellFrame =
        [selectedPanel localSessionCellFrameForTabAtIndex:selectedTabIndex];

    const CGFloat collectionViewContentOffsetMinY =
        [selectedPanel collectionView].contentOffset.y;
    const CGFloat collectionViewContentOffsetMaxY =
        collectionViewContentOffsetMinY +
        [selectedPanel collectionView].bounds.size.height;
    const BOOL isCellTotallyHidden =
        CGRectGetMaxY(selectedCellFrame) < collectionViewContentOffsetMinY ||
        CGRectGetMinY(selectedCellFrame) > collectionViewContentOffsetMaxY;
    if (transitionType == TransitionType::TRANSITION_PRESENT ||
        isCellTotallyHidden) {
      [selectedPanel scrollTabIndexToVisible:selectedTabIndex
                               triggerLayout:YES];
    }
  } else {
    CGRect collectionViewFrame = [selectedPanel.collectionView frame];
    selectedCellFrame = CGRectMake(collectionViewFrame.size.width / 2,
                                   collectionViewFrame.size.height / 2, 0, 0);
  }
  // Compute initial and final tab frames.
  const CGFloat initialTabFrameOriginY = kHeaderHeight + StatusBarHeight();
  CGRect initialTabFrame =
      CGRectMake(0, initialTabFrameOriginY, self.view.bounds.size.width,
                 self.view.bounds.size.height - initialTabFrameOriginY);
  CGRect finalTabFrame =
      [[selectedPanel collectionView] convertRect:selectedCellFrame
                                           toView:self.view];

  // Compute initial and final toolbar screenshot frames.
  const CGRect initialToolbarFrame =
      [[self.delegate tabSwitcherTransitionToolbarOwner] toolbarFrame];
  CGRect initialToolbarScreenshotFrame = CGRectMake(
      0, 0, initialToolbarFrame.size.width, initialToolbarFrame.size.height);

  const CGFloat cellTopBarHeight = tabSwitcherLocalSessionCellTopBarHeight();
  CGRect finalToolbarScreenshotFrame =
      CGRectMake(0, 0, selectedCellFrame.size.width, cellTopBarHeight);

  UIImageView* tabScreenshotImageView =
      [[UIImageView alloc] initWithFrame:CGRectZero];

  __weak UIImageView* weakTabScreenshotImageView = tabScreenshotImageView;

  if ([self initialTabModelAndTabIDMatchesTabModel:tabModel
                                             tabID:selectedTab.tabId]) {
    tabScreenshotImageView.image = self.transitionContext.tabSnapshotImage;
  } else {
    // If transitioning to a different tab than the one animated in
    // from, a new snapshot should be generated instead of using the transition
    // context
    tabScreenshotImageView.image =
        [self updateScreenshotForCellIfNeeded:selectedCell tabModel:tabModel];
    if (selectedTab) {
      SnapshotTabHelper::FromWebState(selectedTab.webState)
          ->RetrieveColorSnapshot(^(UIImage* snapshot) {
            [weakTabScreenshotImageView setImage:snapshot];
          });
    }
  }

  const CGSize tabScreenshotImageSize = tabScreenshotImageView.image.size;

  CGRect initialTabScreenshotFrame = CGRectZero;
  const CGSize toolbarSize =
      [[self.delegate tabSwitcherTransitionToolbarOwner] toolbarFrame].size;
  CGSize initialTabTargetSize =
      CGSizeMake(initialTabFrame.size.width,
                 initialTabFrame.size.height - toolbarSize.height);
  CGSize revisedTargetSize = CGSizeZero;
  CalculateProjection(tabScreenshotImageSize, initialTabTargetSize,
                      ProjectionMode::kAspectFill, revisedTargetSize,
                      initialTabScreenshotFrame);
  initialTabScreenshotFrame.origin.y += toolbarSize.height;

  CGSize finalTabTargetSize =
      CGSizeMake(selectedCellFrame.size.width,
                 selectedCellFrame.size.height - cellTopBarHeight);
  CGRect finalTabScreenshotFrame;
  CalculateProjection(tabScreenshotImageSize, finalTabTargetSize,
                      ProjectionMode::kAspectFill, revisedTargetSize,
                      finalTabScreenshotFrame);
  finalTabScreenshotFrame.origin.y += cellTopBarHeight;

  UIView<TabStripFoldAnimation>* tabStripPlaceholderView =
      [transitionContextContent generateTabStripPlaceholderView];
  tabStripPlaceholderView.clipsToBounds = YES;
  tabStripPlaceholderView.backgroundColor = [UIColor clearColor];
  [self.view addSubview:tabStripPlaceholderView];

  CGRect tabStripInitialFrame =
      CGRectMake(0, StatusBarHeight(), self.view.bounds.size.width,
                 tabStripPlaceholderView.frame.size.height);
  CGRect tabStripFinalFrame =
      CGRectMake(finalTabFrame.origin.x,
                 finalTabFrame.origin.y - tabStripInitialFrame.size.height,
                 finalTabFrame.size.width, tabStripInitialFrame.size.height);
  CGRect shadowInitialFrame =
      CGRectMake(0, CGRectGetMaxY(initialToolbarScreenshotFrame),
                 initialToolbarScreenshotFrame.size.width, 2);
  CGRect shadowFinalFrame =
      CGRectMake(0, CGRectGetMaxY(finalToolbarScreenshotFrame),
                 finalToolbarScreenshotFrame.size.width, 2);

  if (transitionType == TransitionType::TRANSITION_DISMISS) {
    [tabStripPlaceholderView unfoldWithCompletion:nil];
    std::swap(initialTabFrame, finalTabFrame);
    std::swap(tabStripInitialFrame, tabStripFinalFrame);
    std::swap(shadowInitialFrame, shadowFinalFrame);
    std::swap(initialTabScreenshotFrame, finalTabScreenshotFrame);
    std::swap(initialToolbarScreenshotFrame, finalToolbarScreenshotFrame);
  } else {
    [tabStripPlaceholderView foldWithCompletion:^{
      [tabStripPlaceholderView removeFromSuperview];
    }];
  }

  tabStripPlaceholderView.frame = tabStripInitialFrame;

  // Create and setup placeholder view and subviews.
  UIView* placeholderView = [[UIView alloc] initWithFrame:initialTabFrame];
  [placeholderView setClipsToBounds:YES];
  [placeholderView setUserInteractionEnabled:NO];

  tabScreenshotImageView.frame = initialTabScreenshotFrame;
  tabScreenshotImageView.contentMode = UIViewContentModeScaleToFill;
  tabScreenshotImageView.autoresizingMask = UIViewAutoresizingNone;
  [placeholderView addSubview:tabScreenshotImageView];

  // Try using a snapshot view for dismissal animation because it's faster and
  // the collection view has already been rendered. Use a client rendering
  // otherwise.
  SnapshotViewOption snapshotOption = SnapshotViewOption::SNAPSHOT_VIEW;
  if (transitionType == TransitionType::TRANSITION_PRESENT)
    snapshotOption = SnapshotViewOption::CLIENT_RENDERING;

  UIView* finalToolbarScreenshotImageView =
      [self snapshotViewForView:selectedCell.topBar
                      withModel:tabModel
                         option:snapshotOption];
  finalToolbarScreenshotImageView.autoresizingMask = UIViewAutoresizingNone;
  finalToolbarScreenshotImageView.frame = initialToolbarScreenshotFrame;
  [placeholderView addSubview:finalToolbarScreenshotImageView];

  UIView* toolbarScreenshotImageView =
      transitionContextContent.toolbarSnapshotView;
  toolbarScreenshotImageView.autoresizingMask = UIViewAutoresizingNone;
  toolbarScreenshotImageView.frame = initialToolbarScreenshotFrame;
  [placeholderView addSubview:toolbarScreenshotImageView];

  UIImageView* toolbarShadowImageView =
      [[UIImageView alloc] initWithFrame:shadowInitialFrame];
  [toolbarShadowImageView setAutoresizingMask:UIViewAutoresizingNone];
  [toolbarShadowImageView setImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW)];
  [placeholderView addSubview:toolbarShadowImageView];

  [self.view addSubview:placeholderView];

  [selectedCell setHidden:YES];
  toolbarScreenshotImageView.alpha =
      (transitionType == TransitionType::TRANSITION_DISMISS) ? 0 : 1.0;

  __weak TabSwitcherController* weakSelf = self;
  void (^completionBlock)(BOOL) = ^(BOOL finished) {
    TabSwitcherController* strongSelf = weakSelf;

    [tabStripPlaceholderView removeFromSuperview];
    [toolbarScreenshotImageView removeFromSuperview];
    [selectedCell setHidden:NO];
    [placeholderView removeFromSuperview];

    if (transitionType == TransitionType::TRANSITION_DISMISS) {
      [strongSelf restoreWindowBackgroundColor];
      if (finished) {
        [strongSelf setTransitionContext:nil];
      }
    }
    [[strongSelf view] setUserInteractionEnabled:YES];
    completion();
  };

  ProceduralBlock animationBlock = ^{
    toolbarScreenshotImageView.alpha =
        transitionType == TransitionType::TRANSITION_DISMISS ? 1.0 : 0;
    tabStripPlaceholderView.frame = tabStripFinalFrame;
    toolbarShadowImageView.frame = shadowFinalFrame;
    placeholderView.frame = finalTabFrame;
    toolbarScreenshotImageView.frame = finalToolbarScreenshotFrame;
    finalToolbarScreenshotImageView.frame = finalToolbarScreenshotFrame;
    tabScreenshotImageView.frame = finalTabScreenshotFrame;
  };

  [UIView animateWithDuration:animated ? kTransitionAnimationDuration : 0
                        delay:0
                      options:UIViewAnimationCurveEaseInOut
                   animations:animationBlock
                   completion:completionBlock];
}

- (BOOL)initialTabModelAndTabIDMatchesTabModel:(nonnull TabModel*)tabModel
                                         tabID:(nonnull NSString*)tabID {
  TabModel* initialTabModel = self.transitionContext.initialTabModel;
  NSString* initialTabID =
      [self transitionContextContentForTabModel:initialTabModel].initialTabID;
  return initialTabModel == tabModel && [initialTabID isEqualToString:tabID];
}

- (void)updateLocalPanelsCells {
  auto mainTabPanel =
      [self panelControllerForTabModel:[_tabSwitcherModel mainTabModel]];
  auto otrTabPanel =
      [self panelControllerForTabModel:[_tabSwitcherModel otrTabModel]];
  [mainTabPanel reload];
  [otrTabPanel reload];
}

- (NSInteger)currentPanelIndex {
  return [_tabSwitcherView currentPanelIndex];
}

- (int)offsetToDistantSessionPanels {
  if (_signInPanelType == TabSwitcherSignInPanelsType::NO_PANEL) {
    return kSignInPromoPanelIndex;
  }
  return kSignInPromoPanelIndex + 1;
}

- (TabSwitcherSessionType)sessionTypeForPanelIndex:(NSInteger)panelIndex {
  if (panelIndex == kLocalTabsOffTheRecordPanelIndex)
    return TabSwitcherSessionType::OFF_THE_RECORD_SESSION;
  if (panelIndex == kLocalTabsOnTheRecordPanelIndex)
    return TabSwitcherSessionType::REGULAR_SESSION;
  return TabSwitcherSessionType::DISTANT_SESSION;
}

- (TabModel*)tabModelForSessionType:(TabSwitcherSessionType)sessionType {
  switch (sessionType) {
    case TabSwitcherSessionType::REGULAR_SESSION:
      return [_tabSwitcherModel mainTabModel];
      break;
    case TabSwitcherSessionType::OFF_THE_RECORD_SESSION:
      return [_tabSwitcherModel otrTabModel];
      break;
    case TabSwitcherSessionType::DISTANT_SESSION:
      return nil;
      break;
  }
}

- (TabModel*)currentSelectedModel {
  const NSInteger currentPanelIndex = [self currentPanelIndex];
  const TabSwitcherSessionType sessionType =
      [self sessionTypeForPanelIndex:currentPanelIndex];
  TabModel* model = [self tabModelForSessionType:sessionType];
  if (!model)
    model = [self onLoadActiveModel];
  return model;
}

- (TabModel*)onLoadActiveModel {
  switch (_onLoadActiveModelType) {
    case TabModelType::MODEL_MAIN:
      return [_tabSwitcherModel mainTabModel];
    case TabModelType::MODEL_OTR:
      return [_tabSwitcherModel otrTabModel];
  }
}

- (void)selectPanelForTabModel:(TabModel*)selectedTabModel {
  DCHECK(selectedTabModel == [_tabSwitcherModel otrTabModel] ||
         selectedTabModel == [_tabSwitcherModel mainTabModel]);
  NSInteger selectedPanel =
      (selectedTabModel == [_tabSwitcherModel otrTabModel])
          ? kLocalTabsOffTheRecordPanelIndex
          : kLocalTabsOnTheRecordPanelIndex;
  [_tabSwitcherView selectPanelAtIndex:selectedPanel];
}

- (TabSwitcherPanelController*)panelControllerForTabModel:(TabModel*)tabModel {
  DCHECK(tabModel == [_tabSwitcherModel mainTabModel] ||
         tabModel == [_tabSwitcherModel otrTabModel]);
  if (tabModel == [_tabSwitcherModel mainTabModel])
    return _onTheRecordSession;
  if (tabModel == [_tabSwitcherModel otrTabModel])
    return _offTheRecordSession;
  return nil;
}

- (void)tabSwitcherDismissWithModel:(TabModel*)model animated:(BOOL)animated {
  DCHECK(model);
  [[self presentedViewController] dismissViewControllerAnimated:NO
                                                     completion:nil];
  [self.delegate tabSwitcher:self shouldFinishWithActiveModel:model];
  [self performTabSwitcherTransition:TransitionType::TRANSITION_DISMISS
                           withModel:model
                            animated:animated
                      withCompletion:^{
                        [self.animationDelegate
                            tabSwitcherDismissalAnimationDidEnd:self];
                        [self.delegate tabSwitcherDismissTransitionDidEnd:self];
                        [_tabSwitcherView wasHidden];
                      }];
}

- (void)tabSwitcherDismissWithCurrentSelectedModel {
  TabModel* model = [self currentSelectedModel];
  base::RecordAction(base::UserMetricsAction("MobileTabSwitcherClose"));
  [self tabSwitcherDismissWithModel:model animated:YES];
}

- (Tab*)dismissWithNewTabAnimation:(const GURL&)URL
                           atIndex:(NSUInteger)position
                        transition:(ui::PageTransition)transition
                          tabModel:(TabModel*)tabModel {
  NSUInteger tabIndex = position;
  if (position > tabModel.count)
    tabIndex = tabModel.count;

  web::NavigationManager::WebLoadParams loadParams(URL);
  loadParams.transition_type = transition;

  Tab* tab = [tabModel insertTabWithLoadParams:loadParams
                                        opener:nil
                                   openedByDOM:NO
                                       atIndex:tabIndex
                                  inBackground:NO];

  [self tabSwitcherDismissWithModel:tabModel animated:YES];

  return tab;
}

- (BOOL)isPanelIndexForLocalSession:(NSUInteger)panelIndex {
  return panelIndex == kLocalTabsOnTheRecordPanelIndex ||
         panelIndex == kLocalTabsOffTheRecordPanelIndex;
}

- (void)openTabWithContentOfDistantTab:
    (synced_sessions::DistantTab const*)distantTab {
  sync_sessions::OpenTabsUIDelegate* openTabs =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState)
          ->GetOpenTabsUIDelegate();
  const sessions::SessionTab* toLoad = nullptr;
  if (openTabs->GetForeignTab(distantTab->session_tag, distantTab->tab_id,
                              &toLoad)) {
    // Disable user interactions until the tab is inserted to prevent multiple
    // concurrent tab model updates.
    [_tabSwitcherView setUserInteractionEnabled:NO];

    TabModel* tabModel = [_tabSwitcherModel mainTabModel];
    WebStateList* webStateList = [tabModel webStateList];
    webStateList->InsertWebState(
        webStateList->count(),
        session_util::CreateWebStateWithNavigationEntries(
            [tabModel browserState], toLoad->current_navigation_index,
            toLoad->navigations),
        WebStateList::INSERT_FORCE_INDEX | WebStateList::INSERT_ACTIVATE,
        WebStateOpener());

    // Reenable touch events.
    [_tabSwitcherView setUserInteractionEnabled:YES];
    [self tabSwitcherDismissWithModel:tabModel animated:YES];
  }
}

- (void)removePromoPanelHeaderCellIfNeeded {
  if (_shouldRemovePromoPanelHeaderCell) {
    _shouldRemovePromoPanelHeaderCell = NO;
    [[_tabSwitcherView headerView]
        removeSessionsAtIndexes:@[ @(kSignInPromoPanelIndex) ]];
  }
}

- (void)addPromoPanelHeaderCellIfNeeded {
  if (_shouldAddPromoPanelHeaderCell) {
    _shouldAddPromoPanelHeaderCell = NO;
    [[_tabSwitcherView headerView]
        insertSessionsAtIndexes:@[ @(kSignInPromoPanelIndex) ]];
  }
}

- (void)reopenClosedTab {
  sessions::TabRestoreService* const tabRestoreService =
      IOSChromeTabRestoreServiceFactory::GetForBrowserState(_browserState);
  if (!tabRestoreService || tabRestoreService->entries().empty())
    return;

  const std::unique_ptr<sessions::TabRestoreService::Entry>& entry =
      tabRestoreService->entries().front();
  // Only handle the TAB type.
  if (entry->type != sessions::TabRestoreService::TAB)
    return;

  [self.dispatcher openNewTab:[OpenNewTabCommand command]];
  TabRestoreServiceDelegateImplIOS* const delegate =
      TabRestoreServiceDelegateImplIOSFactory::GetForBrowserState(
          _browserState);
  tabRestoreService->RestoreEntryById(delegate, entry->id,
                                      WindowOpenDisposition::CURRENT_TAB);
}

#pragma mark - TabSwitcherModelDelegate

- (void)distantSessionsRemovedAtSortedIndexes:(NSArray*)removedIndexes
                      insertedAtSortedIndexes:(NSArray*)insertedIndexes {
  // Update the panels
  int offset = [self offsetToDistantSessionPanels];

  // Iterate in reverse order so that indexes in |removedIndexes| stays synced
  // with the indexes in |_controllersOfDistantSessions|.
  for (NSNumber* objCIndex in [removedIndexes reverseObjectEnumerator]) {
    int index = [objCIndex intValue];
    [_tabSwitcherView removePanelViewAtIndex:index + offset];
    [_controllersOfDistantSessions removeObjectAtIndex:index];
  }

  for (NSNumber* objCIndex in insertedIndexes) {
    int index = [objCIndex intValue];
    std::string tag = [_tabSwitcherModel tagOfDistantSessionAtIndex:index];
    TabSwitcherPanelController* panelController = [
        [TabSwitcherPanelController alloc]
                   initWithModel:_tabSwitcherModel
        forDistantSessionWithTag:tag
                    browserState:_browserState
                       presenter:self /* id<SigninPresenter, SyncPresenter> */
                      dispatcher:self.dispatcher];
    [panelController setDelegate:self];
    [_tabSwitcherView addPanelView:[panelController view]
                           atIndex:index + offset];
    [_controllersOfDistantSessions insertObject:panelController atIndex:index];
  }

  // Update the header view.
  // The header view's content is created using the panel's content.
  [[_tabSwitcherView headerView] reloadData];
}

- (void)distantSessionMayNeedUpdate:(std::string const&)tag {
  for (TabSwitcherPanelController* panel in _controllersOfDistantSessions) {
    DCHECK([panel isKindOfClass:[TabSwitcherPanelController class]]);
    if ([panel sessionTag] == tag) {
      [panel updateCollectionViewIfNeeded];
      return;
    }
  }
}

- (void)localSessionMayNeedUpdate:(TabSwitcherSessionType)type {
  if (type == TabSwitcherSessionType::REGULAR_SESSION) {
    [_onTheRecordSession updateCollectionViewIfNeeded];
  } else {
    DCHECK(type == TabSwitcherSessionType::OFF_THE_RECORD_SESSION);
    [_offTheRecordSession updateCollectionViewIfNeeded];
  }
}

- (void)signInPanelChangedTo:(TabSwitcherSignInPanelsType)newPanelType {
  if (_signInPanelType == newPanelType)
    return;

  // Remove promo panel that is no longer relevant, if any.
  if (_signInPanelType != TabSwitcherSignInPanelsType::NO_PANEL) {
    _shouldRemovePromoPanelHeaderCell = YES;
    BOOL updateScrollView =
        [_tabSwitcherView currentPanelIndex] != kSignInPromoPanelIndex;
    [_tabSwitcherView removePanelViewAtIndex:kSignInPromoPanelIndex
                            updateScrollView:updateScrollView];
    _signinPromoViewMediator.consumer = nil;
    [_signinPromoViewMediator signinPromoViewRemoved];
    _signinPromoViewMediator = nil;
    _signInPanelOverlayView = nil;
  } else {
    _shouldAddPromoPanelHeaderCell = YES;
  }
  [self addPromoPanelForSignInPanelType:newPanelType];
}

- (void)addPromoPanelForSignInPanelType:(TabSwitcherSignInPanelsType)panelType {
  _signInPanelType = panelType;
  DCHECK_EQ(nil, _signInPanelOverlayView);
  if (panelType != TabSwitcherSignInPanelsType::NO_PANEL) {
    _signInPanelOverlayView = [[TabSwitcherPanelOverlayView alloc]
        initWithFrame:CGRectZero
         browserState:_browserState
            presenter:self /* id<SigninPresenter, SyncPresenter> */
           dispatcher:self.dispatcher];
    [_signInPanelOverlayView
        setOverlayType:PanelOverlayTypeFromSignInPanelsType(panelType)];
    _signInPanelOverlayView.delegate = self;
    if (panelType == TabSwitcherSignInPanelsType::PANEL_USER_SIGNED_OUT) {
      DCHECK_NE(nil, _signInPanelOverlayView.signinPromoView);
      _signinPromoViewMediator = [[SigninPromoViewMediator alloc]
          initWithBrowserState:_browserState
                   accessPoint:signin_metrics::AccessPoint::
                                   ACCESS_POINT_TAB_SWITCHER
                     presenter:self];
      _signinPromoViewMediator.consumer = self;
      _signInPanelOverlayView.signinPromoView.delegate =
          _signinPromoViewMediator;
      [[_signinPromoViewMediator createConfigurator]
          configureSigninPromoView:_signInPanelOverlayView.signinPromoView];
    }
    [_tabSwitcherView addPanelView:_signInPanelOverlayView
                           atIndex:kSignInPromoPanelIndex];
  }
}

- (CGSize)sizeForItemAtIndex:(NSUInteger)index
                   inSession:(TabSwitcherSessionType)session {
  switch (session) {
    case TabSwitcherSessionType::OFF_THE_RECORD_SESSION:
      return [[_offTheRecordSession view] cellSize];
    case TabSwitcherSessionType::REGULAR_SESSION:
      return [[_onTheRecordSession view] cellSize];
    case TabSwitcherSessionType::DISTANT_SESSION:
      NOTREACHED();
      return {};
  }
}

- (BOOL)isSigninInProgress {
  return _signinPromoViewMediator.signinInProgress;
}

#pragma mark - TabSwitcherHeaderViewDelegate

- (void)tabSwitcherHeaderViewDismiss:(TabSwitcherHeaderView*)view {
  [self tabSwitcherDismissWithCurrentSelectedModel];
}

- (void)tabSwitcherHeaderViewDidSelectSessionAtIndex:(NSInteger)index {
  switch (index) {
    case kLocalTabsOffTheRecordPanelIndex:
      base::RecordAction(base::UserMetricsAction(
          "MobileTabSwitcherHeaderViewSelectIncognitoPanel"));
      break;
    case kLocalTabsOnTheRecordPanelIndex:
      base::RecordAction(base::UserMetricsAction(
          "MobileTabSwitcherHeaderViewSelectNonIncognitoPanel"));
      break;
    default:
      base::RecordAction(base::UserMetricsAction(
          "MobileTabSwitcherHeaderViewSelectDistantSessionPanel"));
      break;
  }
  [_tabSwitcherView selectPanelAtIndex:index];
}

#pragma mark - TabSwitcherHeaderViewDataSource

- (NSInteger)tabSwitcherHeaderViewSessionCount {
  NSInteger promoPanel = (![_tabSwitcherModel distantSessionCount]) ? 1 : 0;
  return [_tabSwitcherModel sessionCount] + promoPanel;
}

- (TabSwitcherSessionCellData*)sessionCellDataAtIndex:(NSUInteger)index {
  if (index == kLocalTabsOffTheRecordPanelIndex) {
    // If has incognito tabs return incognito cell data.
    return [TabSwitcherSessionCellData incognitoSessionCellData];
  } else if (index == kLocalTabsOnTheRecordPanelIndex) {
    return [TabSwitcherSessionCellData openTabSessionCellData];
  } else {
    if (![_tabSwitcherModel distantSessionCount]) {
      // Display promo panel cell if there is no distant sessions.
      return [TabSwitcherSessionCellData otherDevicesSessionCellData];
    } else {
      index -= kHeaderDistantSessionIndexOffset;

      sync_pb::SyncEnums::DeviceType deviceType =
          sync_pb::SyncEnums::TYPE_UNSET;
      NSString* cellTitle = nil;

      if (index < _controllersOfDistantSessions.count) {
        TabSwitcherPanelController* panel =
            [_controllersOfDistantSessions objectAtIndex:index];
        const synced_sessions::DistantSession* distantSession =
            [panel distantSession];
        deviceType = distantSession->device_type;
        cellTitle = base::SysUTF8ToNSString(distantSession->name);
      }
      TabSwitcherSessionCellType cellType;
      switch (deviceType) {
        case sync_pb::SyncEnums::TYPE_PHONE:
          cellType = kPhoneRemoteSessionCell;
          break;
        case sync_pb::SyncEnums::TYPE_TABLET:
          cellType = kTabletRemoteSessionCell;
          break;
        default:
          cellType = kLaptopRemoteSessionCell;
          break;
      }
      TabSwitcherSessionCellData* sessionData =
          [[TabSwitcherSessionCellData alloc] initWithSessionCellType:cellType];
      sessionData.title = cellTitle;
      return sessionData;
    }
  }
}

- (NSInteger)tabSwitcherHeaderViewSelectedPanelIndex {
  return [_tabSwitcherView currentPanelIndex];
}

#pragma mark - TabSwitcherViewDelegate

- (void)openNewTabInPanelAtIndex:(NSInteger)panelIndex {
  CHECK(panelIndex >= 0);
  DCHECK([self isPanelIndexForLocalSession:panelIndex]);
  BOOL incognito = !(panelIndex == kLocalTabsOnTheRecordPanelIndex);
  if (incognito) {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherCreateIncognitoTab"));
  } else {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherCreateNonIncognitoTab"));
  }
  // Create and execute command to create the tab.
  [self.dispatcher
      openNewTab:[OpenNewTabCommand commandWithIncognito:incognito]];
}

- (NewTabButtonStyle)buttonStyleForPanelAtIndex:(NSInteger)panelIndex {
  CHECK(panelIndex >= 0);
  switch (panelIndex) {
    case kLocalTabsOnTheRecordPanelIndex:
      if ([_onTheRecordSession shouldShowNewTabButton]) {
        return NewTabButtonStyle::BLUE;
      } else {
        return NewTabButtonStyle::HIDDEN;
      }
    case kLocalTabsOffTheRecordPanelIndex:
      if ([_offTheRecordSession shouldShowNewTabButton]) {
        return NewTabButtonStyle::GRAY;
      } else {
        return NewTabButtonStyle::HIDDEN;
      }
    default:
      return NewTabButtonStyle::HIDDEN;
  }
}

- (BOOL)shouldShowDismissButtonForPanelAtIndex:(NSInteger)panelIndex {
  CHECK(panelIndex >= 0);
  switch (panelIndex) {
    case kLocalTabsOnTheRecordPanelIndex:
      return [[_tabSwitcherModel mainTabModel] count] != 0;
    case kLocalTabsOffTheRecordPanelIndex:
      return [[_tabSwitcherModel otrTabModel] count] != 0;
    default:
      return [[self currentSelectedModel] count] != 0;
  }
}

- (void)tabSwitcherViewDelegateDismissTabSwitcher:(TabSwitcherView*)view {
  [self tabSwitcherDismissWithCurrentSelectedModel];
}

#pragma mark - TabSwitcherPanelControllerDelegate

- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
               didSelectDistantTab:(synced_sessions::DistantTab*)tab {
  DCHECK(tab);
  [self openTabWithContentOfDistantTab:tab];
  base::RecordAction(
      base::UserMetricsAction("MobileTabSwitcherOpenDistantTab"));
}

- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
                 didSelectLocalTab:(Tab*)tab {
  DCHECK(tab);
  const TabSwitcherSessionType panelSessionType =
      tabSwitcherPanelController.sessionType;
  TabModel* tabModel = [self tabModelForSessionType:panelSessionType];
  [tabModel setCurrentTab:tab];
  [self tabSwitcherDismissWithModel:tabModel animated:YES];
  if (panelSessionType == TabSwitcherSessionType::OFF_THE_RECORD_SESSION) {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherOpenIncognitoTab"));
  } else {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherOpenNonIncognitoTab"));
  }
}

- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
                  didCloseLocalTab:(Tab*)tab {
  DCHECK(tab);
  const TabSwitcherSessionType panelSessionType =
      tabSwitcherPanelController.sessionType;
  TabModel* tabModel =
      panelSessionType == TabSwitcherSessionType::OFF_THE_RECORD_SESSION
          ? [_tabSwitcherModel otrTabModel]
          : [_tabSwitcherModel mainTabModel];
  DCHECK_NE(NSNotFound, static_cast<NSInteger>([tabModel indexOfTab:tab]));
  [tabModel closeTab:tab];

  if (panelSessionType == TabSwitcherSessionType::OFF_THE_RECORD_SESSION) {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherCloseIncognitoTab"));
  } else {
    base::RecordAction(
        base::UserMetricsAction("MobileTabSwitcherCloseNonIncognitoTab"));
  }
}

- (void)tabSwitcherPanelControllerDidUpdateOverlayViewVisibility:
    (TabSwitcherPanelController*)tabSwitcherPanelController {
  [_tabSwitcherView updateOverlayButtonState];
}

#pragma mark - SyncPresenter

- (void)showReauthenticateSignin {
  [self.dispatcher
              showSignin:
                  [[ShowSigninCommand alloc]
                      initWithOperation:AUTHENTICATION_OPERATION_REAUTHENTICATE
                            accessPoint:signin_metrics::AccessPoint::
                                            ACCESS_POINT_UNKNOWN]
      baseViewController:self];
}

- (void)showSyncSettings {
  [self.dispatcher showSyncSettingsFromViewController:self];
}

- (void)showSyncPassphraseSettings {
  [self.dispatcher showSyncPassphraseSettingsFromViewController:self];
}

#pragma mark - SigninPresenter

- (void)showSignin:(ShowSigninCommand*)command {
  [self.dispatcher showSignin:command baseViewController:self];
}

#pragma mark - SigninPromoViewConsumer

- (void)configureSigninPromoWithConfigurator:
            (SigninPromoViewConfigurator*)configurator
                             identityChanged:(BOOL)identityChanged {
  DCHECK_NE(nil, _signInPanelOverlayView);
  DCHECK_NE(nil, _signInPanelOverlayView.signinPromoView);
  [configurator
      configureSigninPromoView:_signInPanelOverlayView.signinPromoView];
}

- (void)signinDidFinish {
  [_tabSwitcherModel syncedSessionsChanged];
}

#pragma mark - TabSwitcherPanelOverlayViewDelegate

- (void)tabSwitcherPanelOverlViewWasShown:
    (TabSwitcherPanelOverlayView*)tabSwitcherPanelOverlayView {
  [_signinPromoViewMediator signinPromoViewVisible];
}

- (void)tabSwitcherPanelOverlViewWasHidden:
    (TabSwitcherPanelOverlayView*)tabSwitcherPanelOverlayView {
  [_signinPromoViewMediator signinPromoViewHidden];
}

@end
