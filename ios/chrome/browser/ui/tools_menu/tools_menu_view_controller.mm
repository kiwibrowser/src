// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_controller.h"

#import <QuartzCore/QuartzCore.h>
#include <stdint.h>

#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_constants.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_menu_notification_delegate.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_menu_notifier.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/reading_list_menu_view_item.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_model.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_tools_cell.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/user_feedback/user_feedback_provider.h"
#import "ios/third_party/material_components_ios/src/components/Ink/src/MaterialInk.h"
#include "ios/web/public/user_agent.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ios::material::TimingFunction;

namespace {

// Time for ink to fade into view.
static const NSTimeInterval kMDCInkTouchDelayInterval = 0.15;

static const CGFloat kMenuItemHeight = 48;

static NSString* const kToolsItemCellID = @"ToolsItemCellID";


NS_INLINE void AnimateInViews(NSArray* views,
                              CGFloat initialX,
                              CGFloat initialY) {
  [views enumerateObjectsUsingBlock:^(UIView* view, NSUInteger index,
                                      BOOL* stop) {
    CGFloat beginTime = index * .035;
    CABasicAnimation* transformAnimation =
        [CABasicAnimation animationWithKeyPath:@"transform"];
    [transformAnimation
        setFromValue:[NSValue
                         valueWithCATransform3D:CATransform3DMakeTranslation(
                                                    initialX, initialY, 0)]];
    [transformAnimation
        setToValue:[NSValue valueWithCATransform3D:CATransform3DIdentity]];
    [transformAnimation setDuration:ios::material::kDuration1];
    [transformAnimation setBeginTime:beginTime];

    CAAnimation* fadeAnimation = OpacityAnimationMake(0, 1);
    [fadeAnimation setDuration:ios::material::kDuration1];
    [fadeAnimation
        setTimingFunction:TimingFunction(ios::material::CurveEaseOut)];
    [fadeAnimation setBeginTime:beginTime];

    [[view layer]
        addAnimation:AnimationGroupMake(@[ transformAnimation, fadeAnimation ])
              forKey:@"animateIn"];
  }];
}
}  // anonymous namespace

@interface ToolsMenuCollectionView : UICollectionView
@property(nonatomic, assign) CGPoint touchBeginPoint;
@property(nonatomic, assign) CGPoint touchEndPoint;
@end

@implementation ToolsMenuCollectionView

@synthesize touchBeginPoint = _touchBeginPoint;
@synthesize touchEndPoint = _touchEndPoint;

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  _touchBeginPoint = [[touches anyObject] locationInView:self];
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  _touchEndPoint = [[touches anyObject] locationInView:self];
  [super touchesEnded:touches withEvent:event];
}

@end

// Class Extension for private methods.
@interface ToolsMenuViewController ()<UICollectionViewDelegateFlowLayout,
                                      UICollectionViewDataSource,
                                      ReadingListMenuNotificationDelegate> {
  BOOL _waitForInk;
  // Weak pointer to ReadingListMenuNotifier, used to set the starting values
  // for the reading list badge.
  __weak ReadingListMenuNotifier* _readingListMenuNotifier;
  // YES if NSLayoutConstraits were added.
  BOOL _addedConstraints;
}

// Determines if the reading list should display a new feature badge. Defaults
// to |NO|.
@property(nonatomic, assign) BOOL showReadingListNewBadge;
// Indicates whether the New Incognito Tab cell should be highlighted. Defaults
// to |NO|.
@property(nonatomic, assign) BOOL highlightNewIncognitoTabCell;
// Tracks events for the purpose of in-product help. Does not take ownership of
// tracker. Tracker must not be destroyed during lifetime of
// ToolsMenuViewController. Defaults to |NULL|.
@property(nonatomic, assign) feature_engagement::Tracker* engagementTracker;
@property(nonatomic, strong) ToolsMenuCollectionView* menuView;
@property(nonatomic, strong) MDCInkView* touchFeedbackView;
@property(nonatomic, assign) ToolbarType toolbarType;
// Populated by the configuration object in |initializeMenuWithConfiguration:|
// stores the time this view controller was requested by the user for the
// reporting of metrics.
@property(nonatomic, assign) NSTimeInterval requestStartTime;

// Returns the reading list cell.
- (ReadingListMenuViewCell*)readingListCell;
@end

@implementation ToolsMenuViewController

@synthesize showReadingListNewBadge = _showReadingListNewBadge;
@synthesize highlightNewIncognitoTabCell = _highlightNewIncognitoTabCell;
@synthesize engagementTracker = _engagementTracker;
@synthesize menuView = _menuView;
@synthesize isCurrentPageBookmarked = _isCurrentPageBookmarked;
@synthesize touchFeedbackView = _touchFeedbackView;
@synthesize isTabLoading = _isTabLoading;
@synthesize toolbarType = _toolbarType;
@synthesize menuItems = _menuItems;
@synthesize delegate = _delegate;
@synthesize dispatcher = _dispatcher;
@synthesize requestStartTime = _requestStartTime;

#pragma mark Public methods

- (CGFloat)optimalHeight:(CGFloat)suggestedHeight {
  NSInteger numberOfItems = [self.menuItems count];
  if (_toolbarType == ToolbarTypeWebiPhone) {
    // Account for the height of the first row, not included in |menuItems|.
    numberOfItems++;
  }
  CGFloat maxItems = suggestedHeight / kMenuItemHeight;
  if (maxItems >= numberOfItems) {
    return numberOfItems * kMenuItemHeight;
  } else {
    const CGFloat halfHeight = kMenuItemHeight / 2;
    return round(maxItems) * kMenuItemHeight - halfHeight;
  }
}

- (void)setItemEnabled:(BOOL)enabled withTag:(NSInteger)tag {
  for (ToolsMenuViewItem* item in _menuItems) {
    if ([item tag] == tag) {
      [item setActive:enabled];
      break;
    }
  }

  for (ToolsMenuViewCell* cell in [_menuView visibleCells]) {
    if (![cell isKindOfClass:[ToolsMenuViewCell class]])
      continue;

    if ([cell tag] != tag)
      continue;

    NSIndexPath* path = [_menuView indexPathForCell:cell];
    NSInteger itemIndex = [self dataIndexForIndexPath:path];
    [cell configureForMenuItem:[_menuItems objectAtIndex:itemIndex]];
  }
}

- (void)setIsCurrentPageBookmarked:(BOOL)value {
  _isCurrentPageBookmarked = value;

  ToolsMenuViewToolsCell* toolsCell = [self toolsCell];
  [[toolsCell starButton] setHidden:_isCurrentPageBookmarked];
  [[toolsCell starredButton] setHidden:!_isCurrentPageBookmarked];
}

- (void)setCanShowFindBar:(BOOL)enabled {
  [self setItemEnabled:enabled withTag:TOOLS_SHOW_FIND_IN_PAGE];
}

- (void)setCanShowShareMenu:(BOOL)enabled {
  ToolsMenuViewToolsCell* toolsCell = [self toolsCell];
  [[toolsCell shareButton] setEnabled:enabled];
}

- (UIButton*)toolsButton {
  UIButton* toolsButton = [[self toolsCell] toolsButton];
  [toolsButton addTarget:self
                  action:@selector(buttonPressed:)
        forControlEvents:UIControlEventTouchUpInside];
  [toolsButton setTranslatesAutoresizingMaskIntoConstraints:YES];
  [toolsButton setOpaque:NO];
  [toolsButton setBackgroundColor:[UIColor clearColor]];
  return toolsButton;
}

- (void)setIsTabLoading:(BOOL)isTabLoading {
  _isTabLoading = isTabLoading;

  ToolsMenuViewToolsCell* toolsCell = [self toolsCell];
  [[toolsCell stopButton] setHidden:!isTabLoading];
  [[toolsCell reloadButton] setHidden:isTabLoading];
}

- (void)initializeMenuWithConfiguration:(ToolsMenuConfiguration*)configuration {
  self.requestStartTime = configuration.requestStartTime;
  self.showReadingListNewBadge = configuration.showReadingListNewBadge;
  self.engagementTracker = configuration.engagementTracker;
  self.highlightNewIncognitoTabCell =
      configuration.highlightNewIncognitoTabCell;

  if (configuration.readingListMenuNotifier) {
    _readingListMenuNotifier = configuration.readingListMenuNotifier;
    [configuration.readingListMenuNotifier setDelegate:self];
  }

  if (IsIPadIdiom()) {
    _toolbarType =
        configuration.hasNoOpenedTabs
            ? ToolbarTypeNoTabsiPad
            : (!IsCompactTablet() ? ToolbarTypeWebiPad : ToolbarTypeWebiPhone);
  } else {
    // kOptionInTabSwitcher option must be enabled on iPhone with
    // no opened tabs.
    DCHECK(!configuration.hasNoOpenedTabs || configuration.isInTabSwitcher);
    _toolbarType = configuration.isInTabSwitcher ? ToolbarTypeSwitcheriPhone
                                                 : ToolbarTypeWebiPhone;
  }

  // Build the menu, adding all relevant items.
  NSMutableArray* menu = [NSMutableArray array];

  for (size_t i = 0; i < arraysize(itemInfoList); ++i) {
    const MenuItemInfo& item = itemInfoList[i];
    if (!ToolsMenuItemShouldBeVisible(item, _toolbarType, configuration))
      continue;

    NSString* title = l10n_util::GetNSStringWithFixup(item.title_id);
    Class itemClass =
        item.item_class ? item.item_class : [ToolsMenuViewItem class];
    // Sanity check that the class is a useful one.
    DCHECK([itemClass
        respondsToSelector:@selector
        (menuItemWithTitle:accessibilityIdentifier:selector:command:)]);
    [menu addObject:[itemClass menuItemWithTitle:title
                         accessibilityIdentifier:item.accessibility_id
                                        selector:item.selector
                                         command:item.command_id]];
  }

#if !defined(NDEBUG)
  NSUserDefaults* standardDefaults = [NSUserDefaults standardUserDefaults];
  if ((_toolbarType & ToolbarTypeWebAll) &&
      [standardDefaults boolForKey:@"DevViewSource"]) {
    // Debug menu, not localized, only visible if turned on by a default.
    [menu addObject:[self createViewSourceItem]];
  }
#endif  // !defined(NDEBUG)

  [self setMenuItems:menu];

  // Decide the enabled state of the currently visible item between
  // "Request Desktop Site" and "Request Mobile Site".
  switch (configuration.userAgentType) {
    case web::UserAgentType::NONE:
      [self setItemEnabled:NO withTag:TOOLS_REQUEST_DESKTOP_SITE];
      break;
    case web::UserAgentType::MOBILE:
      [self setItemEnabled:YES withTag:TOOLS_REQUEST_DESKTOP_SITE];
      break;
    case web::UserAgentType::DESKTOP:
      [self setItemEnabled:YES withTag:TOOLS_REQUEST_MOBILE_SITE];
      break;
  }

  // Disable TOOLS_CLOSE_ALL_TABS menu item if on phone with no tabs.
  if (!IsIPadIdiom()) {
    [self setItemEnabled:!configuration.hasNoOpenedTabs
                 withTag:TOOLS_CLOSE_ALL_TABS];
  }
}

#if !defined(NDEBUG)
- (ToolsMenuViewItem*)createViewSourceItem {
  return [ToolsMenuViewItem menuItemWithTitle:@"View Source"
                      accessibilityIdentifier:@"View Source"
                                     selector:@selector(viewSource)
                                      command:TOOLS_VIEW_SOURCE];
}
#endif  // !defined(NDEBUG)

#pragma mark - Data handling utilities

- (ToolsMenuViewToolsCell*)toolsCell {
  for (ToolsMenuViewToolsCell* visibleCell in [_menuView visibleCells]) {
    if ([visibleCell isKindOfClass:[ToolsMenuViewToolsCell class]])
      return visibleCell;
  }

  return nil;
}

- (ReadingListMenuViewCell*)readingListCell {
  for (ReadingListMenuViewCell* visibleCell in [_menuView visibleCells]) {
    if ([visibleCell isKindOfClass:[ReadingListMenuViewCell class]])
      return visibleCell;
  }

  return nil;
}

- (NSInteger)dataIndexForIndexPath:(NSIndexPath*)path {
  NSInteger item = [path item];

  if (_toolbarType == ToolbarTypeWebiPhone)
    --item;

  return item;
}

#pragma mark - UIViewController Overrides

- (void)loadView {
  [super loadView];

  UIView* rootView = [self view];
  [rootView setAutoresizingMask:UIViewAutoresizingFlexibleHeight |
                                UIViewAutoresizingFlexibleWidth];
  [rootView setBackgroundColor:[UIColor whiteColor]];

  _touchFeedbackView = [[MDCInkView alloc] initWithFrame:CGRectZero];

  UICollectionViewFlowLayout* menuItemsLayout =
      [[UICollectionViewFlowLayout alloc] init];

  _menuView = [[ToolsMenuCollectionView alloc] initWithFrame:[rootView bounds]
                                        collectionViewLayout:menuItemsLayout];
  [_menuView setAccessibilityLabel:l10n_util::GetNSString(IDS_IOS_TOOLS_MENU)];
  [_menuView setAccessibilityIdentifier:kPopupMenuToolsMenuTableViewId];
  [_menuView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_menuView setBackgroundColor:[UIColor whiteColor]];
  [_menuView setDataSource:self];
  [_menuView setDelegate:self];
  [_menuView setOpaque:YES];

  [rootView addSubview:_menuView];
  [self updateViewConstraints];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [_menuView registerClass:[ToolsMenuViewToolsCell class]
      forCellWithReuseIdentifier:kToolsItemCellID];

  [_menuView registerClass:[ToolsMenuViewItem cellClass]
      forCellWithReuseIdentifier:[ToolsMenuViewItem cellID]];

  // Register each custom class.
  NSMutableSet* registeredClasses = [NSMutableSet set];
  for (size_t i = 0; i < arraysize(itemInfoList); ++i) {
    const MenuItemInfo& item = itemInfoList[i];
    if (!item.item_class || [registeredClasses containsObject:item.item_class])
      continue;
    [_menuView registerClass:[item.item_class cellClass]
        forCellWithReuseIdentifier:[item.item_class cellID]];
    [registeredClasses addObject:item.item_class];
  }
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];
  if (self.showReadingListNewBadge && self.engagementTracker) {
    self.engagementTracker->Dismissed(
        feature_engagement::kIPHBadgedReadingListFeature);
  }
}

- (void)updateViewConstraints {
  if (!_addedConstraints) {
    NSDictionary* view = @{@"menu" : _menuView};
    NSArray* constraints =
        @[ @"V:|-(0)-[menu]-(0)-|", @"H:|-(0)-[menu]-(0)-|" ];
    ApplyVisualConstraints(constraints, view);
    _addedConstraints = YES;
  }
  [super updateViewConstraints];
}

#pragma mark - Content Animation Stuff

- (void)animateContentIn {
  // Make sure that the collection view has laid out before trying to animate
  // the contents.
  [_menuView layoutIfNeeded];

  NSArray* visibleCells =
      [[_menuView visibleCells] sortedArrayUsingComparator:^NSComparisonResult(
                                    UIView* view1, UIView* view2) {
        CGPoint center1 = [view1 center];
        CGPoint center2 = [view2 center];

        if (center1.y < center2.y)
          return NSOrderedAscending;

        if (center1.y > center2.y)
          return NSOrderedDescending;

        return NSOrderedSame;
      }];

  ToolsMenuViewToolsCell* toolsCell = nil;
  if (_toolbarType == ToolbarTypeWebiPhone) {
    toolsCell = [visibleCells firstObject];
    if ([toolsCell isKindOfClass:[ToolsMenuViewToolsCell class]]) {
      visibleCells = [visibleCells
          subarrayWithRange:NSMakeRange(1, [visibleCells count] - 1)];
    } else {
      toolsCell = nil;
    }
  }

  [CATransaction begin];
  [CATransaction
      setAnimationTimingFunction:TimingFunction(ios::material::CurveEaseInOut)];
  [CATransaction setAnimationDuration:ios::material::kDuration5];
  [CATransaction setCompletionBlock:^{
    if (self.requestStartTime != 0) {
      UMA_HISTOGRAM_TIMES(
          "Toolbar.ShowToolsMenuResponsiveness",
          base::TimeDelta::FromSecondsD(
              [NSDate timeIntervalSinceReferenceDate] - self.requestStartTime));
      // Reset the start time to ensure that whatever happens, we only record
      // this once.
      self.requestStartTime = 0;
    }

  }];
  AnimateInViews([toolsCell allButtons], 10, 0);
  AnimateInViews(visibleCells, 0, -10);
  [CATransaction commit];

  // The number badge should be prioritized over the new feature badge, so only
  // show the new feature badge if number badge will not be shown.
  if (_readingListMenuNotifier.readingListUnreadCount == 0) {
    [[self readingListCell] updateShowTextBadge:_showReadingListNewBadge
                                       animated:YES];
  }
  [[self readingListCell]
      updateBadgeCount:_readingListMenuNotifier.readingListUnreadCount
              animated:YES];
  [[self readingListCell]
      updateSeenState:_readingListMenuNotifier.readingListUnseenItemsExist
             animated:YES];
  if (self.highlightNewIncognitoTabCell) {
    [self triggerNewIncognitoTabCellHighlight];
  }
}

- (void)hideContent {
  _menuView.alpha = 0;

  // Remove the target/action for touching the buttons. VoiceOver may hold on
  // to these buttons after |self| has been released, causing a crash if a
  // button is activated (see http://crbug.com/480974 ).
  UIButton* toolsButton = [[self toolsCell] toolsButton];
  [toolsButton removeTarget:self
                     action:@selector(buttonPressed:)
           forControlEvents:UIControlEventTouchUpInside];
  ToolsMenuViewToolsCell* toolsCell = [self toolsCell];
  for (UIButton* button in [toolsCell allButtons]) {
    [button removeTarget:self
                  action:@selector(buttonPressed:)
        forControlEvents:UIControlEventTouchUpInside];
  }
  [toolsCell.stopButton removeTarget:self.dispatcher
                              action:@selector(stopLoading)
                    forControlEvents:UIControlEventTouchUpInside];
  [toolsCell.reloadButton removeTarget:self.dispatcher
                                action:@selector(reload)
                      forControlEvents:UIControlEventTouchUpInside];
  [toolsCell.shareButton removeTarget:self.dispatcher
                               action:@selector(sharePage)
                     forControlEvents:UIControlEventTouchUpInside];
  [toolsCell.starButton removeTarget:self.dispatcher
                              action:@selector(bookmarkPage)
                    forControlEvents:UIControlEventTouchUpInside];
  [toolsCell.starredButton removeTarget:self.dispatcher
                                 action:@selector(bookmarkPage)
                       forControlEvents:UIControlEventTouchUpInside];
}

#pragma mark - Button event handling

- (void)buttonPressed:(id)sender {
  int commandID = [sender tag];
  // All command IDs should have been refactored to be < 0, and not use
  // ChromeExecuteCommand.
  DCHECK(commandID < 0);
  // Do any metrics logging for the command, and then close the menu.
  [_delegate commandWasSelected:commandID];
}

#pragma mark - UICollectionViewDelegate Implementation

- (BOOL)collectionView:(ToolsMenuCollectionView*)view
    shouldHighlightItemAtIndexPath:(NSIndexPath*)path {
  if (view.tracking)
    return NO;
  NSInteger item = [self dataIndexForIndexPath:path];
  return (item >= 0);
}

- (void)collectionView:(ToolsMenuCollectionView*)view
    didHighlightItemAtIndexPath:(NSIndexPath*)path {
  ToolsMenuViewCell* cell =
      (ToolsMenuViewCell*)[view cellForItemAtIndexPath:path];

  NSInteger item = [self dataIndexForIndexPath:path];
  DCHECK_GE(item, 0);
  ToolsMenuViewItem* menuItem = [_menuItems objectAtIndex:item];
  DCHECK(menuItem);
  if ([menuItem active]) {
    [_touchFeedbackView setFrame:cell.bounds];
    [cell addSubview:_touchFeedbackView];

    CGPoint touchPoint = [view touchBeginPoint];
    touchPoint = [view convertPoint:touchPoint toView:_touchFeedbackView];
    [_touchFeedbackView startTouchBeganAnimationAtPoint:touchPoint
                                             completion:nil];
  }
}

- (void)collectionView:(ToolsMenuCollectionView*)view
    didUnhighlightItemAtIndexPath:(NSIndexPath*)path {
  CGPoint touchPoint = [view touchEndPoint];
  touchPoint = [view convertPoint:touchPoint toView:_touchFeedbackView];
  __weak MDCInkView* inkView = _touchFeedbackView;
  _waitForInk = YES;
  [_touchFeedbackView startTouchEndedAnimationAtPoint:touchPoint
                                           completion:^{
                                             _waitForInk = NO;
                                             [inkView removeFromSuperview];
                                           }];
}

- (BOOL)collectionView:(UICollectionView*)view
    shouldSelectItemAtIndexPath:(NSIndexPath*)path {
  NSInteger item = [self dataIndexForIndexPath:path];
  if (item < 0)
    return NO;

  return [[_menuItems objectAtIndex:item] active];
}

- (void)collectionView:(UICollectionView*)view
    didSelectItemAtIndexPath:(NSIndexPath*)path {
  [view deselectItemAtIndexPath:path animated:YES];

  NSInteger item = [self dataIndexForIndexPath:path];
  if (item < 0)
    return;

  dispatch_time_t delayTime = dispatch_time(
      DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * kMDCInkTouchDelayInterval));
  dispatch_after(
      _waitForInk ? delayTime : 0, dispatch_get_main_queue(), ^(void) {
        ToolsMenuViewItem* menuItem = [_menuItems objectAtIndex:item];
        // Tag values > 0, and use of the ChromeExecuteCommand pattern from the
        // menu, is no longer supported.
        DCHECK([menuItem tag] < 0);
        [_delegate commandWasSelected:[menuItem tag]];

        // The menuItem will handle executing the command if it can.
        // Otherwise, the dispatching should have been handled by the preceding
        // -commandWasSelected: call on |_delegate|.
        // This is so that a baseViewController can be sent with the dispatch
        // command.
        if ([menuItem canExecuteCommand]) {
          [menuItem executeCommandWithDispatcher:self.dispatcher];
        }
      });
}

#pragma mark - UICollectionViewDataSource Implementation

- (NSInteger)collectionView:(UICollectionView*)view
     numberOfItemsInSection:(NSInteger)section {
  NSInteger numberOfItems = [_menuItems count];
  if (_toolbarType == ToolbarTypeWebiPhone)
    ++numberOfItems;

  return numberOfItems;
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)view
                 cellForItemAtIndexPath:(NSIndexPath*)path {
  NSInteger item = [self dataIndexForIndexPath:path];
  if (item < 0) {
    ToolsMenuViewToolsCell* cell =
        [view dequeueReusableCellWithReuseIdentifier:kToolsItemCellID
                                        forIndexPath:path];
    // Add specific target/action dispatch for buttons.
    // These need to be added *before* -buttonPressed:,
    // because -buttonPressed: closes the popup menu, which will usually
    // destroy the buttons before any other actions can be called.
    [cell.stopButton addTarget:self.dispatcher
                        action:@selector(stopLoading)
              forControlEvents:UIControlEventTouchUpInside];
    [cell.reloadButton addTarget:self.dispatcher
                          action:@selector(reload)
                forControlEvents:UIControlEventTouchUpInside];
    [cell.shareButton addTarget:self.dispatcher
                         action:@selector(sharePage)
               forControlEvents:UIControlEventTouchUpInside];
    [cell.starButton addTarget:self.dispatcher
                        action:@selector(bookmarkPage)
              forControlEvents:UIControlEventTouchUpInside];
    [cell.starredButton addTarget:self.dispatcher
                           action:@selector(bookmarkPage)
                 forControlEvents:UIControlEventTouchUpInside];
    for (UIButton* button in [cell allButtons]) {
      [button addTarget:self
                    action:@selector(buttonPressed:)
          forControlEvents:UIControlEventTouchUpInside];
    }

    return cell;
  }

  ToolsMenuViewItem* menuItem = [_menuItems objectAtIndex:item];
  ToolsMenuViewCell* menuItemCell =
      [view dequeueReusableCellWithReuseIdentifier:[[menuItem class] cellID]
                                      forIndexPath:path];
  [menuItemCell configureForMenuItem:menuItem];

  return menuItemCell;
}

#pragma mark - UICollectionViewDelegateFlowLayout Implementation

- (CGSize)collectionView:(UICollectionView*)view
                    layout:(UICollectionViewLayout*)collectionViewLayout
    sizeForItemAtIndexPath:(NSIndexPath*)path {
  return CGSizeMake(CGRectGetWidth([_menuView bounds]), kMenuItemHeight);
}

- (CGFloat)collectionView:(UICollectionView*)collectionView
                                 layout:(UICollectionViewLayout*)
                                            collectionViewLayout
    minimumLineSpacingForSectionAtIndex:(NSInteger)section {
  return 0;
}

#pragma mark - ReadingListMenuNotificationDelegate Implementation

- (void)unreadCountChanged:(NSInteger)unreadCount {
  [[self readingListCell] updateBadgeCount:unreadCount animated:YES];
}

- (void)unseenStateChanged:(BOOL)unseenItemsExist {
  [[self readingListCell] updateSeenState:unseenItemsExist animated:YES];
}

#pragma mark - New Incognito Tab in-product help promotion

- (void)triggerNewIncognitoTabCellHighlight {
  for (ToolsMenuViewCell* visibleCell in [_menuView visibleCells]) {
    if ([visibleCell.accessibilityIdentifier
            isEqualToString:kToolsMenuNewIncognitoTabId]) {
      // Set the label's background color to be clear so that the highlight is
      // is not covered by the label.
      visibleCell.title.backgroundColor = [UIColor clearColor];

      CABasicAnimation* highlightAnimation =
          [CABasicAnimation animationWithKeyPath:@"backgroundColor"];
      highlightAnimation.duration = ios::material::kDuration5;
      highlightAnimation.repeatCount = 2;
      highlightAnimation.autoreverses = YES;
      highlightAnimation.toValue =
          static_cast<id>([[MDCPalette cr_bluePalette] tint100].CGColor);
      highlightAnimation.timingFunction = [CAMediaTimingFunction
          functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
      [visibleCell.contentView.layer addAnimation:highlightAnimation
                                           forKey:nil];

      self.highlightNewIncognitoTabCell = NO;
      break;
    }
  }
}

@end
