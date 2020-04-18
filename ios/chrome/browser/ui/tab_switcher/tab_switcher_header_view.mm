// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_header_view.h"

#import "base/ios/block_types.h"
#include "base/logging.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/procedural_block_types.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_header_cell.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_cell_data.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kCollectionViewTopMargin = 39.0;
const CGFloat kCollectionViewHeight = 56.0;
const CGFloat kDismissButtonWidth = 46.0;
const CGFloat kDismissButtonHeight = 39.0;
const CGFloat kCollectionViewCellWidth = 238;
const CGFloat kActiveSpaceIndicatorHeight = 2;
enum PanelSelectionChangeDirection { RIGHT, LEFT };
}

@protocol AccessiblePanelSelectorDelegate
// Scrolls to the panel in the direction |direction|, if possible.
- (void)moveToPanelInDirection:(PanelSelectionChangeDirection)direction;
@end

// An invisible view that offers VoiceOver control of the panel selection
// UICollectionView.
// Notes:
// Directly subclassing UICollectionView resulted in a tons of unwanted
// interactions with the cells.
// Subclassing UIAccessibilityElement instead of UIView is not possible if
// we want the accessibilityFrame to resize itself using autoresizing masks.
@interface AccessiblePanelSelectorView : UIView {
  // The delegate which receives actions.
  __weak id<AccessiblePanelSelectorDelegate> _delegate;
}
- (void)setDelegate:(id<AccessiblePanelSelectorDelegate>)delegate;
@end

@implementation AccessiblePanelSelectorView

- (void)setDelegate:(id<AccessiblePanelSelectorDelegate>)delegate {
  _delegate = delegate;
}

- (UIAccessibilityTraits)accessibilityTraits {
  return [super accessibilityTraits] | UIAccessibilityTraitAdjustable |
         UIAccessibilityTraitCausesPageTurn;
}

- (BOOL)isAccessibilityElement {
  return YES;
}

- (void)accessibilityIncrement {
  [_delegate moveToPanelInDirection:RIGHT];
}

- (void)accessibilityDecrement {
  [_delegate moveToPanelInDirection:LEFT];
}

@end

@interface TabSwitcherHeaderView ()<UICollectionViewDataSource,
                                    UICollectionViewDelegate,
                                    AccessiblePanelSelectorDelegate> {
  UICollectionViewFlowLayout* _flowLayout;
  UICollectionView* _collectionView;
  AccessiblePanelSelectorView* _accessibilityView;
  UIButton* _dismissButton;
  UIView* _activeSpaceIndicatorView;

  BOOL _performingUpdate;
}

// Loads and initializes subviews.
- (void)loadSubviews;
// Performs layout of the collection view.
- (void)layoutCollectionView;

@end

@implementation TabSwitcherHeaderView

@synthesize delegate = _delegate;
@synthesize dataSource = _dataSource;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.backgroundColor = [[MDCPalette greyPalette] tint900];
    [self loadSubviews];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self layoutCollectionView];
}

- (void)selectItemAtIndex:(NSInteger)index {
  NSInteger selectedIndex = [self selectedIndex];
  if (selectedIndex != index) {
    [_collectionView
        selectItemAtIndexPath:[NSIndexPath indexPathForItem:index inSection:0]
                     animated:NO
               scrollPosition:UICollectionViewScrollPositionNone];
    [self updateSelectionAtIndex:index animated:YES];
  }
}

- (void)reloadData {
  [_collectionView reloadData];
  [_collectionView layoutIfNeeded];
}

- (void)performUpdate:(void (^)(TabSwitcherHeaderView* headerView))updateBlock {
  [self performUpdate:updateBlock completion:nil];
}

- (void)performUpdate:(void (^)(TabSwitcherHeaderView* headerView))updateBlock
           completion:(ProceduralBlock)completion {
  DCHECK(updateBlock);

  __weak TabSwitcherHeaderView* weakSelf = self;
  ProceduralBlock batchUpdates = ^{
    TabSwitcherHeaderView* strongSelf = weakSelf;
    if (!strongSelf)
      return;
    strongSelf->_performingUpdate = YES;
    updateBlock(strongSelf);
    strongSelf->_performingUpdate = NO;
  };
  ProceduralBlockWithBool batchCompletion = ^(BOOL finished) {
    // Reestablish selection after the update.
    const NSInteger selectedPanelIndex =
        [[weakSelf delegate] tabSwitcherHeaderViewSelectedPanelIndex];
    if (selectedPanelIndex != NSNotFound)
      [weakSelf selectItemAtIndex:selectedPanelIndex];
    if (completion)
      completion();
  };

  [_collectionView performBatchUpdates:batchUpdates completion:batchCompletion];
}

- (void)insertSessionsAtIndexes:(NSArray*)indexes {
  DCHECK(_performingUpdate);
  [_collectionView
      insertItemsAtIndexPaths:[self indexPathArrayWithIndexes:indexes]];
}

- (void)removeSessionsAtIndexes:(NSArray*)indexes {
  DCHECK(_performingUpdate);
  [_collectionView
      deleteItemsAtIndexPaths:[self indexPathArrayWithIndexes:indexes]];
}

- (UIView*)dismissButton {
  return _dismissButton;
}

#pragma mark - Private

- (NSInteger)selectedIndex {
  NSInteger selectedIndex = NSNotFound;
  NSArray* selectedIndexPaths = [_collectionView indexPathsForSelectedItems];
  if (selectedIndexPaths.count) {
    NSIndexPath* selectedIndexPath = selectedIndexPaths[0];
    selectedIndex = selectedIndexPath.item;
  }
  return selectedIndex;
}

- (NSInteger)itemCount {
  return [_collectionView numberOfItemsInSection:0];
}

// The UICollectionViewFlowLayout enumerate indexes from right to left when the
// UI is configured in RTL mode. This method always returns the index from value
// for a left to right enumeration order.
- (NSInteger)leftToRightIndexForFlowLayoutIndex:(NSInteger)index {
  return UseRTLLayout() ? ([self itemCount] - 1) - index : index;
}

- (void)updateSelectionAtIndex:(NSInteger)index animated:(BOOL)animated {
  const CGRect cellRect = CGRectMake(
      [self leftToRightIndexForFlowLayoutIndex:index] *
          kCollectionViewCellWidth,
      0, kCollectionViewCellWidth, [_collectionView bounds].size.height);
  [_collectionView scrollRectToVisible:cellRect animated:animated];
  [self layoutActiveSpaceIndicatorAnimated:animated];
}

- (NSArray*)indexPathArrayWithIndexes:(NSArray*)indexes {
  NSMutableArray* array =
      [[NSMutableArray alloc] initWithCapacity:indexes.count];
  for (NSNumber* index in indexes) {
    [array
        addObject:[NSIndexPath indexPathForItem:[index intValue] inSection:0]];
  }
  return array;
}

- (void)loadSubviews {
  UICollectionViewFlowLayout* flowLayout =
      [[UICollectionViewFlowLayout alloc] init];
  [flowLayout setMinimumLineSpacing:0];
  [flowLayout setMinimumInteritemSpacing:0];
  const CGSize cellSize =
      CGSizeMake(kCollectionViewCellWidth, kCollectionViewHeight);
  [flowLayout setItemSize:cellSize];
  [flowLayout setScrollDirection:UICollectionViewScrollDirectionHorizontal];
  _flowLayout = flowLayout;

  _collectionView =
      [[UICollectionView alloc] initWithFrame:[self collectionViewFrame]
                         collectionViewLayout:flowLayout];
  [_collectionView setDelegate:self];
  [_collectionView setDataSource:self];
  [_collectionView registerClass:[TabSwitcherHeaderCell class]
      forCellWithReuseIdentifier:[TabSwitcherHeaderCell identifier]];
  [_collectionView setShowsVerticalScrollIndicator:NO];
  [_collectionView setShowsHorizontalScrollIndicator:NO];
  [_collectionView setBackgroundColor:[[MDCPalette greyPalette] tint900]];
  [_collectionView setAllowsMultipleSelection:NO];
  [_collectionView setAllowsSelection:YES];
  [_collectionView setIsAccessibilityElement:NO];
  [_collectionView setAccessibilityElementsHidden:YES];
  [self addSubview:_collectionView];

  _accessibilityView = [[AccessiblePanelSelectorView alloc]
      initWithFrame:[self collectionViewFrame]];
  [_accessibilityView
      setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin |
                          UIViewAutoresizingFlexibleWidth];
  [_accessibilityView setDelegate:self];
  [_accessibilityView setUserInteractionEnabled:NO];
  [self addSubview:_accessibilityView];

  _dismissButton = [[UIButton alloc] initWithFrame:CGRectZero];
  UIImage* dismissImage =
      [UIImage imageNamed:@"tabswitcher_tab_switcher_button"];
  dismissImage =
      [dismissImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  [_dismissButton setContentMode:UIViewContentModeCenter];
  [_dismissButton setBackgroundColor:[UIColor clearColor]];
  [_dismissButton setTintColor:[UIColor whiteColor]];
  [_dismissButton setImage:dismissImage forState:UIControlStateNormal];
  [_dismissButton
      setAccessibilityLabel:l10n_util::GetNSString(
                                IDS_IOS_TAB_STRIP_LEAVE_TAB_SWITCHER)];

  [_dismissButton addTarget:self
                     action:@selector(dismissButtonTouchUpInside:)
           forControlEvents:UIControlEventTouchUpInside];
  [_dismissButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self addSubview:_dismissButton];

  NSArray* constraints = @[
    @"V:|-0-[dismissButton(==buttonHeight)]",
    @"H:[dismissButton(==buttonWidth)]-0-|",
  ];
  NSDictionary* viewsDictionary = @{
    @"dismissButton" : _dismissButton,
  };
  NSDictionary* metrics = @{
    @"buttonHeight" : @(kDismissButtonHeight),
    @"buttonWidth" : @(kDismissButtonWidth),
  };
  ApplyVisualConstraintsWithMetrics(constraints, viewsDictionary, metrics);

  UIView* activeSpaceIndicatorView = [[UIView alloc] initWithFrame:CGRectZero];
  [activeSpaceIndicatorView
      setBackgroundColor:[[MDCPalette cr_bluePalette] tint500]];
  [activeSpaceIndicatorView
      setFrame:CGRectMake(
                   0, self.bounds.size.height - kActiveSpaceIndicatorHeight,
                   kCollectionViewCellWidth, kActiveSpaceIndicatorHeight)];
  [self addSubview:activeSpaceIndicatorView];
  _activeSpaceIndicatorView = activeSpaceIndicatorView;
}

- (void)layoutCollectionView {
  NSInteger selectedIndex = [self selectedIndex];
  [_collectionView setFrame:[self collectionViewFrame]];
  if (selectedIndex != NSNotFound)
    [self updateSelectionAtIndex:selectedIndex animated:YES];
}

- (CGRect)collectionViewFrame {
  return CGRectMake(0, kCollectionViewTopMargin, self.bounds.size.width,
                    kCollectionViewHeight);
}

- (void)layoutActiveSpaceIndicatorAnimated:(BOOL)animated {
  [self setPanelSelectorAccessibility];
  NSInteger selectedIndex = [self selectedIndex];
  if (selectedIndex == NSNotFound)
    return;
  CGRect indicatorFrame = [_activeSpaceIndicatorView bounds];
  indicatorFrame.origin.y =
      self.bounds.size.height - kActiveSpaceIndicatorHeight;
  indicatorFrame.origin.x =
      kCollectionViewCellWidth *
          [self leftToRightIndexForFlowLayoutIndex:selectedIndex] -
      [_collectionView contentOffset].x;
  if (animated)
    [UIView beginAnimations:nil context:NULL];
  [_activeSpaceIndicatorView setFrame:indicatorFrame];
  if (animated)
    [UIView commitAnimations];
}

- (void)dismissButtonTouchUpInside:(UIButton*)button {
  [self.delegate tabSwitcherHeaderViewDismiss:self];
}

- (void)setPanelSelectorAccessibility {
  NSInteger index = [self selectedIndex];
  if (index != NSNotFound)
    [_accessibilityView setAccessibilityLabel:[self panelTitleAtIndex:index]];
}

- (NSString*)panelTitleAtIndex:(NSInteger)index {
  NSIndexPath* indexPath = [NSIndexPath indexPathForItem:index inSection:0];
  TabSwitcherSessionCellData* sessionCellData =
      [[self dataSource] sessionCellDataAtIndex:indexPath.row];
  return sessionCellData.title;
}

#pragma mark - AccessiblePanelSelectorDelegate

- (void)moveToPanelInDirection:(PanelSelectionChangeDirection)direction {
  NSInteger indexDelta = direction == RIGHT ? 1 : -1;
  NSInteger newIndex = [self selectedIndex] + indexDelta;
  newIndex = std::max<NSInteger>(newIndex, 0);
  newIndex = std::min<NSInteger>(
      newIndex,
      [self collectionView:_collectionView numberOfItemsInSection:0] - 1);
  NSIndexPath* newIndexPath =
      [NSIndexPath indexPathForItem:newIndex inSection:0];
  [_collectionView
      selectItemAtIndexPath:newIndexPath
                   animated:NO
             scrollPosition:UICollectionViewScrollPositionCenteredHorizontally];
  [self updateSelectionAtIndex:newIndexPath.item animated:NO];
  [[self delegate]
      tabSwitcherHeaderViewDidSelectSessionAtIndex:newIndexPath.item];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  DCHECK([self dataSource]);
  DCHECK(section == 0);
  return [[self dataSource] tabSwitcherHeaderViewSessionCount];
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  TabSwitcherHeaderCell* headerCell = [collectionView
      dequeueReusableCellWithReuseIdentifier:[TabSwitcherHeaderCell identifier]
                                forIndexPath:indexPath];
  TabSwitcherSessionCellData* sessionCellData =
      [[self dataSource] sessionCellDataAtIndex:indexPath.row];
  [headerCell loadSessionCellData:sessionCellData];
  return headerCell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [self updateSelectionAtIndex:indexPath.item animated:YES];
  [[self delegate] tabSwitcherHeaderViewDidSelectSessionAtIndex:indexPath.item];
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  [self layoutActiveSpaceIndicatorAnimated:NO];
}

@end
