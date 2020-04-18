// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/grid_view_controller.h"

#import "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/numerics/safe_conversions.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_cell.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_constants.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_image_data_source.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_item.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_layout.h"
#import "ios/chrome/browser/ui/tab_grid/transitions/grid_transition_layout.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kCellIdentifier = @"GridCellIdentifier";
// Creates an NSIndexPath with |index| in section 0.
NSIndexPath* CreateIndexPath(NSInteger index) {
  return [NSIndexPath indexPathForItem:index inSection:0];
}
}  // namespace

@interface GridViewController ()<GridCellDelegate,
                                 UICollectionViewDataSource,
                                 UICollectionViewDelegate>
// Bookkeeping based on |-viewDidAppear:| and |-viewDidDisappear:|.
// If the view is not appeared, there is no need to update the collection view.
@property(nonatomic, assign, getter=isViewAppeared) BOOL viewAppeared;
// A collection view of items in a grid format.
@property(nonatomic, weak) UICollectionView* collectionView;
// The local model backing the collection view.
@property(nonatomic, strong) NSMutableArray<GridItem*>* items;
// Identifier of the selected item. This value is disregarded if |self.items| is
// empty. This bookkeeping is done to set the correct selection on
// |-viewWillAppear:|.
@property(nonatomic, copy) NSString* selectedItemID;
// Index of the selected item in |items|.
@property(nonatomic, readonly) NSUInteger selectedIndex;
// The gesture recognizer used for interactive item reordering.
@property(nonatomic, strong)
    UILongPressGestureRecognizer* itemReorderRecognizer;
// Animator to show or hide the empty state.
@property(nonatomic, strong) UIViewPropertyAnimator* emptyStateAnimator;
@end

@implementation GridViewController
// Public properties.
@synthesize theme = _theme;
@synthesize delegate = _delegate;
@synthesize imageDataSource = _imageDataSource;
@synthesize emptyStateView = _emptyStateView;
// Private properties.
@synthesize viewAppeared = _viewAppeared;
@synthesize collectionView = _collectionView;
@synthesize items = _items;
@synthesize selectedItemID = _selectedItemID;
@synthesize itemReorderRecognizer = _itemReorderRecognizer;
@synthesize emptyStateAnimator = _emptyStateAnimator;

- (instancetype)init {
  if (self = [super init]) {
    _items = [[NSMutableArray<GridItem*> alloc] init];
  }
  return self;
}

- (void)loadView {
  GridLayout* layout = [[GridLayout alloc] init];
  UICollectionView* collectionView =
      [[UICollectionView alloc] initWithFrame:CGRectZero
                         collectionViewLayout:layout];
  [collectionView registerClass:[GridCell class]
      forCellWithReuseIdentifier:kCellIdentifier];
  collectionView.dataSource = self;
  collectionView.delegate = self;
  collectionView.backgroundView = [[UIView alloc] init];
  collectionView.backgroundView.backgroundColor =
      UIColorFromRGB(kGridBackgroundColor);
  if (@available(iOS 11, *))
    collectionView.contentInsetAdjustmentBehavior =
        UIScrollViewContentInsetAdjustmentAlways;
  self.itemReorderRecognizer = [[UILongPressGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(handleItemReorderingWithGesture:)];
  // The collection view cells will by default get touch events in parallel with
  // the reorder recognizer. When this happens, long-pressing on a non-selected
  // cell will cause the selected cell to briefly become unselected and then
  // selected again. To avoid this, the recognizer delays touchesBegan: calls
  // until it fails to recognize a long-press.
  self.itemReorderRecognizer.delaysTouchesBegan = YES;
  [collectionView addGestureRecognizer:self.itemReorderRecognizer];
  self.collectionView = collectionView;
  self.view = collectionView;
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self.collectionView reloadData];
  // Selection is invalid if there are no items.
  if (self.items.count == 0)
    return;
  [self.collectionView selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                                    animated:animated
                              scrollPosition:UICollectionViewScrollPositionTop];
  // Update the delegate, in case it wasn't set when |items| was populated.
  [self.delegate gridViewController:self didChangeItemCount:self.items.count];
  [self animateEmptyStateOut];
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  self.viewAppeared = YES;
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];
  self.viewAppeared = NO;
}

#pragma mark - Public

- (UIScrollView*)gridView {
  return self.collectionView;
}

- (void)setEmptyStateView:(UIView*)emptyStateView {
  if (_emptyStateView)
    [_emptyStateView removeFromSuperview];
  _emptyStateView = emptyStateView;
  emptyStateView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.collectionView.backgroundView addSubview:emptyStateView];
  [NSLayoutConstraint activateConstraints:@[
    [self.collectionView.backgroundView.centerXAnchor
        constraintEqualToAnchor:emptyStateView.centerXAnchor],
    [self.collectionView.backgroundView.centerYAnchor
        constraintEqualToAnchor:emptyStateView.centerYAnchor]
  ]];
}

- (BOOL)isGridEmpty {
  return self.items.count == 0;
}

- (BOOL)isSelectedCellVisible {
  // The collection view's selected item may not have updated yet, so use the
  // selected index.
  NSUInteger selectedIndex = self.selectedIndex;
  if (selectedIndex == NSNotFound)
    return NO;
  NSIndexPath* selectedIndexPath = CreateIndexPath(selectedIndex);
  return [self.collectionView.indexPathsForVisibleItems
      containsObject:selectedIndexPath];
}

- (GridTransitionLayout*)transitionLayout {
  [self.collectionView layoutIfNeeded];
  NSMutableArray<GridTransitionLayoutItem*>* items =
      [[NSMutableArray alloc] init];
  GridTransitionLayoutItem* selectedItem;
  for (NSIndexPath* path in self.collectionView.indexPathsForVisibleItems) {
    GridCell* cell = base::mac::ObjCCastStrict<GridCell>(
        [self.collectionView cellForItemAtIndexPath:path]);
    UICollectionViewLayoutAttributes* attributes =
        [self.collectionView layoutAttributesForItemAtIndexPath:path];
    // Normalize frame to window coordinates. The attributes class applies this
    // change to the other properties such as center, bounds, etc.
    attributes.frame =
        [self.collectionView convertRect:attributes.frame toView:nil];
    GridTransitionLayoutItem* item =
        [GridTransitionLayoutItem itemWithCell:[cell proxyForTransitions]
                                    attributes:attributes];
    [items addObject:item];
    if (cell.selected) {
      selectedItem = item;
    }
  }
  return [GridTransitionLayout layoutWithItems:items selectedItem:selectedItem];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  return base::checked_cast<NSInteger>(self.items.count);
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  GridCell* cell = base::mac::ObjCCastStrict<GridCell>([collectionView
      dequeueReusableCellWithReuseIdentifier:kCellIdentifier
                                forIndexPath:indexPath]);
  cell.accessibilityIdentifier =
      [NSString stringWithFormat:@"%@%ld", kGridCellIdentifierPrefix,
                                 base::checked_cast<long>(indexPath.item)];
  GridItem* item = self.items[indexPath.item];
  [self configureCell:cell withItem:item];
  return cell;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    canMoveItemAtIndexPath:(NSIndexPath*)indexPath {
  return indexPath && self.items.count > 1;
}

- (void)collectionView:(UICollectionView*)collectionView
    moveItemAtIndexPath:(NSIndexPath*)sourceIndexPath
            toIndexPath:(NSIndexPath*)destinationIndexPath {
  NSUInteger source = base::checked_cast<NSUInteger>(sourceIndexPath.item);
  NSUInteger destination =
      base::checked_cast<NSUInteger>(destinationIndexPath.item);
  // Update |items| before informing the delegate, so the state of the UI
  // is correctly represented before any updates occur.
  GridItem* item = self.items[source];
  [self.items removeObjectAtIndex:source];
  [self.items insertObject:item atIndex:destination];

  [self.delegate gridViewController:self
                  didMoveItemWithID:item.identifier
                            toIndex:destination];
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  NSUInteger index = base::checked_cast<NSUInteger>(indexPath.item);
  DCHECK_LT(index, self.items.count);
  NSString* itemID = self.items[index].identifier;
  [self.delegate gridViewController:self didSelectItemWithID:itemID];
}

#pragma mark - GridCellDelegate

- (void)closeButtonTappedForCell:(GridCell*)cell {
  NSUInteger index = base::checked_cast<NSUInteger>(
      [self.collectionView indexPathForCell:cell].item);
  DCHECK_LT(index, self.items.count);
  NSString* itemID = self.items[index].identifier;
  [self.delegate gridViewController:self didCloseItemWithID:itemID];
}

#pragma mark - GridConsumer

- (void)populateItems:(NSArray<GridItem*>*)items
       selectedItemID:(NSString*)selectedItemID {
#ifndef NDEBUG
  // Consistency check: ensure no IDs are duplicated.
  NSMutableSet<NSString*>* identifiers = [[NSMutableSet alloc] init];
  for (GridItem* item in items) {
    [identifiers addObject:item.identifier];
  }
  CHECK_EQ(identifiers.count, items.count);
#endif

  self.items = [items mutableCopy];
  self.selectedItemID = selectedItemID;
  if ([self isViewAppeared]) {
    [self.collectionView reloadData];
    [self.collectionView
        selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                     animated:YES
               scrollPosition:UICollectionViewScrollPositionTop];
  }
  // Whether the view is visible or not, the delegate must be updated.
  [self.delegate gridViewController:self didChangeItemCount:self.items.count];
}

- (void)insertItem:(GridItem*)item
           atIndex:(NSUInteger)index
    selectedItemID:(NSString*)selectedItemID {
  // Consistency check: |item|'s ID is not in |items|.
  // (using DCHECK rather than DCHECK_EQ to avoid a checked_cast on NSNotFound).
  DCHECK([self indexOfItemWithID:item.identifier] == NSNotFound);
  auto performDataSourceUpdates = ^{
    [self.items insertObject:item atIndex:index];
    self.selectedItemID = selectedItemID;
  };
  if (![self isViewAppeared]) {
    performDataSourceUpdates();
    [self.delegate gridViewController:self didChangeItemCount:self.items.count];
    return;
  }
  auto performAllUpdates = ^{
    performDataSourceUpdates();
    [self animateEmptyStateOut];
    [self.collectionView insertItemsAtIndexPaths:@[ CreateIndexPath(index) ]];
  };
  auto completion = ^(BOOL finished) {
    [self.collectionView
        selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                     animated:YES
               scrollPosition:UICollectionViewScrollPositionNone];
    [self.delegate gridViewController:self didChangeItemCount:self.items.count];
  };
  [self.collectionView performBatchUpdates:performAllUpdates
                                completion:completion];
}

- (void)removeItemWithID:(NSString*)removedItemID
          selectedItemID:(NSString*)selectedItemID {
  NSUInteger index = [self indexOfItemWithID:removedItemID];
  auto performDataSourceUpdates = ^{
    [self.items removeObjectAtIndex:index];
    self.selectedItemID = selectedItemID;
  };
  if (![self isViewAppeared]) {
    performDataSourceUpdates();
    [self.delegate gridViewController:self didChangeItemCount:self.items.count];
    return;
  }
  auto performAllUpdates = ^{
    performDataSourceUpdates();
    [self.collectionView deleteItemsAtIndexPaths:@[ CreateIndexPath(index) ]];
    if (self.items.count == 0) {
      [self animateEmptyStateIn];
    }
  };
  auto completion = ^(BOOL finished) {
    if (self.items.count > 0) {
      [self.collectionView
          selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                       animated:YES
                 scrollPosition:UICollectionViewScrollPositionNone];
    }
    [self.delegate gridViewController:self didChangeItemCount:self.items.count];
  };
  [self.collectionView performBatchUpdates:performAllUpdates
                                completion:completion];
}

- (void)selectItemWithID:(NSString*)selectedItemID {
  self.selectedItemID = selectedItemID;
  if (![self isViewAppeared])
    return;
  [self.collectionView
      selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                   animated:YES
             scrollPosition:UICollectionViewScrollPositionNone];
}

- (void)replaceItemID:(NSString*)itemID withItem:(GridItem*)item {
  // Consistency check: |item|'s ID is either |itemID| or not in |items|.
  DCHECK([item.identifier isEqualToString:itemID] ||
         [self indexOfItemWithID:item.identifier] == NSNotFound);
  NSUInteger index = [self indexOfItemWithID:itemID];
  self.items[index] = item;
  if (![self isViewAppeared])
    return;
  GridCell* cell = base::mac::ObjCCastStrict<GridCell>(
      [self.collectionView cellForItemAtIndexPath:CreateIndexPath(index)]);
  [self configureCell:cell withItem:item];
}

- (void)moveItemWithID:(NSString*)itemID toIndex:(NSUInteger)toIndex {
  NSUInteger fromIndex = [self indexOfItemWithID:itemID];
  // If this move would be a no-op, early return and avoid spurious UI updates.
  if (fromIndex == toIndex)
    return;

  auto performDataSourceUpdates = ^{
    GridItem* item = self.items[fromIndex];
    [self.items removeObjectAtIndex:fromIndex];
    [self.items insertObject:item atIndex:toIndex];
  };
  // If the view isn't visible, there's no need for the collection view to
  // update.
  if (![self isViewAppeared]) {
    performDataSourceUpdates();
    return;
  }
  auto performAllUpdates = ^{
    performDataSourceUpdates();
    [self.collectionView moveItemAtIndexPath:CreateIndexPath(fromIndex)
                                 toIndexPath:CreateIndexPath(toIndex)];
  };
  auto completion = ^(BOOL finished) {
    [self.collectionView
        selectItemAtIndexPath:CreateIndexPath(self.selectedIndex)
                     animated:YES
               scrollPosition:UICollectionViewScrollPositionNone];
  };
  [self.collectionView performBatchUpdates:performAllUpdates
                                completion:completion];
}

#pragma mark - Private properties

- (NSUInteger)selectedIndex {
  return [self indexOfItemWithID:self.selectedItemID];
}

#pragma mark - Private

// Returns the index in |self.items| of the first item whose identifier is
// |identifier|.
- (NSUInteger)indexOfItemWithID:(NSString*)identifier {
  auto selectedTest = ^BOOL(GridItem* item, NSUInteger index, BOOL* stop) {
    return [item.identifier isEqualToString:identifier];
  };
  return [self.items indexOfObjectPassingTest:selectedTest];
}

// Configures |cell|'s title synchronously, and favicon and snapshot
// asynchronously with information from |item|. Updates the |cell|'s theme to
// this view controller's theme. This view controller becomes the delegate for
// the cell.
- (void)configureCell:(GridCell*)cell withItem:(GridItem*)item {
  DCHECK(cell);
  DCHECK(item);
  cell.delegate = self;
  cell.theme = self.theme;
  cell.itemIdentifier = item.identifier;
  cell.title = item.title;
  NSString* itemIdentifier = item.identifier;
  [self.imageDataSource faviconForIdentifier:itemIdentifier
                                  completion:^(UIImage* icon) {
                                    // Only update the icon if the cell is not
                                    // already reused for another item.
                                    if (cell.itemIdentifier == itemIdentifier)
                                      cell.icon = icon;
                                  }];
  [self.imageDataSource snapshotForIdentifier:itemIdentifier
                                   completion:^(UIImage* snapshot) {
                                     // Only update the icon if the cell is not
                                     // already reused for another item.
                                     if (cell.itemIdentifier == itemIdentifier)
                                       cell.snapshot = snapshot;
                                   }];
}

// Animates the empty state into view.
- (void)animateEmptyStateIn {
  // TODO(crbug.com/820410) : Polish the animation, and put constants where they
  // belong.
  [self.emptyStateAnimator stopAnimation:YES];
  self.emptyStateAnimator = [[UIViewPropertyAnimator alloc]
      initWithDuration:1.0 - self.emptyStateView.alpha
          dampingRatio:1.0
            animations:^{
              self.emptyStateView.alpha = 1.0;
              self.emptyStateView.transform = CGAffineTransformIdentity;
            }];
  [self.emptyStateAnimator startAnimation];
}

// Animates the empty state out of view.
- (void)animateEmptyStateOut {
  // TODO(crbug.com/820410) : Polish the animation, and put constants where they
  // belong.
  [self.emptyStateAnimator stopAnimation:YES];
  self.emptyStateAnimator = [[UIViewPropertyAnimator alloc]
      initWithDuration:self.emptyStateView.alpha
          dampingRatio:1.0
            animations:^{
              self.emptyStateView.alpha = 0.0;
              self.emptyStateView.transform = CGAffineTransformScale(
                  CGAffineTransformIdentity, /*sx=*/0.9, /*sy=*/0.9);
            }];
  [self.emptyStateAnimator startAnimation];
}

// Handle the long-press gesture used to reorder cells in the collection view.
- (void)handleItemReorderingWithGesture:(UIGestureRecognizer*)gesture {
  DCHECK(gesture == self.itemReorderRecognizer);
  CGPoint location = [gesture locationInView:self.collectionView];
  switch (gesture.state) {
    case UIGestureRecognizerStateBegan: {
      NSIndexPath* path =
          [self.collectionView indexPathForItemAtPoint:location];
      BOOL moving =
          [self.collectionView beginInteractiveMovementForItemAtIndexPath:path];
      if (!moving) {
        gesture.enabled = NO;
      }
      break;
    }
    case UIGestureRecognizerStateChanged:
      [self.collectionView updateInteractiveMovementTargetPosition:location];
      break;
    case UIGestureRecognizerStateEnded:
      [self.collectionView endInteractiveMovement];
      break;
    case UIGestureRecognizerStateCancelled:
      [self.collectionView cancelInteractiveMovement];
      // Re-enable cancelled gesture.
      gesture.enabled = YES;
      break;
    case UIGestureRecognizerStatePossible:
    case UIGestureRecognizerStateFailed:
      NOTREACHED() << "Unexpected long-press recognizer state";
  }
}

@end
