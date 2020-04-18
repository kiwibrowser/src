// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/grid_layout.h"

#import "ios/chrome/browser/ui/tab_grid/grid/grid_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface GridLayout ()
@property(nonatomic, strong) NSArray<NSIndexPath*>* indexPathsOfDeletingItems;
@end

@implementation GridLayout
@synthesize indexPathsOfDeletingItems = _indexPathsOfDeletingItems;

#pragma mark - UICollectionViewLayout

// This is called whenever the layout is invalidated, including during rotation.
// Resizes item, margins, and spacing to fit new size classes and width.
- (void)prepareLayout {
  [super prepareLayout];

  UIUserInterfaceSizeClass horizontalSizeClass =
      self.collectionView.traitCollection.horizontalSizeClass;
  UIUserInterfaceSizeClass verticalSizeClass =
      self.collectionView.traitCollection.verticalSizeClass;
  CGFloat width = CGRectGetWidth(self.collectionView.bounds);
  if (horizontalSizeClass == UIUserInterfaceSizeClassCompact &&
      verticalSizeClass == UIUserInterfaceSizeClassCompact) {
    self.itemSize = kGridCellSizeSmall;
    if (width < kGridLayoutCompactCompactLimitedWidth) {
      self.sectionInset = kGridLayoutInsetsCompactCompactLimitedWidth;
      self.minimumLineSpacing =
          kGridLayoutLineSpacingCompactCompactLimitedWidth;
    } else {
      self.sectionInset = kGridLayoutInsetsCompactCompact;
      self.minimumLineSpacing = kGridLayoutLineSpacingCompactCompact;
    }
  } else if (horizontalSizeClass == UIUserInterfaceSizeClassCompact &&
             verticalSizeClass == UIUserInterfaceSizeClassRegular) {
    if (width < kGridLayoutCompactRegularLimitedWidth) {
      self.itemSize = kGridCellSizeSmall;
      self.sectionInset = kGridLayoutInsetsCompactRegularLimitedWidth;
      self.minimumLineSpacing =
          kGridLayoutLineSpacingCompactRegularLimitedWidth;
    } else {
      self.itemSize = kGridCellSizeMedium;
      self.sectionInset = kGridLayoutInsetsCompactRegular;
      self.minimumLineSpacing = kGridLayoutLineSpacingCompactRegular;
    }
  } else if (horizontalSizeClass == UIUserInterfaceSizeClassRegular &&
             verticalSizeClass == UIUserInterfaceSizeClassCompact) {
    self.itemSize = kGridCellSizeSmall;
    self.sectionInset = kGridLayoutInsetsRegularCompact;
    self.minimumLineSpacing = kGridLayoutLineSpacingRegularCompact;
  } else {
    self.itemSize = kGridCellSizeLarge;
    self.sectionInset = kGridLayoutInsetsRegularRegular;
    self.minimumLineSpacing = kGridLayoutLineSpacingRegularRegular;
  }
}

- (void)prepareForCollectionViewUpdates:
    (NSArray<UICollectionViewUpdateItem*>*)updateItems {
  NSMutableArray<NSIndexPath*>* deletingItems =
      [NSMutableArray arrayWithCapacity:updateItems.count];
  for (UICollectionViewUpdateItem* item in updateItems) {
    if (item.updateAction == UICollectionUpdateActionDelete) {
      [deletingItems addObject:item.indexPathBeforeUpdate];
    }
  }
  self.indexPathsOfDeletingItems = [deletingItems copy];
}

- (UICollectionViewLayoutAttributes*)
initialLayoutAttributesForAppearingItemAtIndexPath:(NSIndexPath*)itemIndexPath {
  UICollectionViewLayoutAttributes* attributes =
      [super initialLayoutAttributesForAppearingItemAtIndexPath:itemIndexPath];
  // TODO(crbug.com/820410) : Polish the animation, and put constants where they
  // belong.
  // Cells being inserted start faded out, scaled down, and drop downwards
  // slightly.
  attributes.alpha = 0.0;
  CGAffineTransform transform =
      CGAffineTransformScale(attributes.transform, /*sx=*/0.9, /*sy=*/0.9);
  transform = CGAffineTransformTranslate(transform, /*tx=*/0,
                                         /*ty=*/attributes.size.height * 0.1);
  attributes.transform = transform;
  return attributes;
}

- (UICollectionViewLayoutAttributes*)
finalLayoutAttributesForDisappearingItemAtIndexPath:
    (NSIndexPath*)itemIndexPath {
  UICollectionViewLayoutAttributes* attributes =
      [super finalLayoutAttributesForDisappearingItemAtIndexPath:itemIndexPath];
  // Disappearing items that aren't being deleted just use the default
  // attributes.
  if (![self.indexPathsOfDeletingItems containsObject:itemIndexPath]) {
    return attributes;
  }
  // Cells being deleted fade out, are scaled down, and drop downwards slightly.
  attributes.alpha = 0.0;
  // Scaled down to 60%.
  CGAffineTransform transform =
      CGAffineTransformScale(attributes.transform, 0.6, 0.6);
  // Translated down (positive-y direction) by 50% of the cell cell size.
  transform =
      CGAffineTransformTranslate(transform, 0, attributes.size.height * 0.5);
  attributes.transform = transform;
  return attributes;
}

- (void)finalizeCollectionViewUpdates {
  self.indexPathsOfDeletingItems = @[];
}

@end
