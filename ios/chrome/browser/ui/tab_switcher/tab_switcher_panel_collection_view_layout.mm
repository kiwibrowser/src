// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_collection_view_layout.h"

#include <algorithm>

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat minWidthOfTab = 200;
const CGFloat kInterTabSpacing = 16;
const UIEdgeInsets kCollectionViewEdgeInsets = {16, 16, 16, 16};
const CGFloat kMaxSizeAsAFactorOfBounds = 0.5;
const CGFloat kMinCellHeightWidthRatio = 0.6;
const CGFloat kMaxCellHeightWidthRatio = 1.8;
}

@implementation TabSwitcherPanelCollectionViewLayout {
  // Keeps track of the inserted and deleted index paths.
  NSMutableArray* _deletedIndexPaths;
  NSMutableArray* _insertedIndexPaths;
}

- (int)maxRowCountWithColumnCount:(int)columnCount inBounds:(CGSize)boundsSize {
  int cellWidth = (boundsSize.width / columnCount) - (kInterTabSpacing * 2.0);
  int minCellHeight = cellWidth * kMinCellHeightWidthRatio;
  return boundsSize.height / (minCellHeight + (kInterTabSpacing * 2.0));
}

- (void)updateLayoutWithBounds:(CGSize)boundsSize {
  // Ignore initial call to |updateLayoutWithBounds| when the frame of the
  // collection view is CGRectZero, because it creates very small cells with
  // broken constraints.
  if (boundsSize.height == 0 && boundsSize.width == 0)
    return;
  int tabCount = [[self collectionView] numberOfItemsInSection:0];
  // Early return because there's nothing to layout.
  if (tabCount == 0)
    return;

  int numberOfColumns = 0;
  int numberOfRows = 0;

  int maxNumberOfColums = static_cast<int>(
      floor(boundsSize.width / (minWidthOfTab + kInterTabSpacing * 2.0)));
  // No need to have more columns than tabs.
  maxNumberOfColums = std::min(maxNumberOfColums, tabCount);

  if ([self maxRowCountWithColumnCount:maxNumberOfColums inBounds:boundsSize] *
          maxNumberOfColums <
      tabCount) {
    // It is impossible for all the tabs to be shown on screen at once.
    // Layout the tabs using the highest density possible, i.e. using the
    // maximum number of columns.
    numberOfColumns = maxNumberOfColums;
    numberOfRows =
        [self maxRowCountWithColumnCount:maxNumberOfColums inBounds:boundsSize];
  } else {
    // Find the most squarish configuration that allows showing all the tabs.
    // A squarish configuration is a layout were the number of rows and columns
    // are roughly equal.

    // |bestScore| contains abs(rowCount - columnCount).
    // The lower |bestScore| is, the better the configuration is.
    int bestScore = INT_MAX;

    int loopStart;
    int loopEnd;
    int loopDirection;
    if (boundsSize.width > boundsSize.height) {
      // In landscape, consider in priority layouts with a large number of
      // columns.
      loopStart = maxNumberOfColums;
      loopEnd = 0;
      loopDirection = -1;
    } else {
      // In landscape, consider in priority layouts with a large number of rows,
      // i.e. a small number of columns.
      loopStart = 1;
      loopEnd = maxNumberOfColums + 1;
      loopDirection = 1;
    }

    int columnCountIterator = loopStart;
    while (columnCountIterator != loopEnd) {
      // Find the minimum number of rows needed to show |tabCount| tab in
      // |columnCount| columns.
      int maxRowCount = [self maxRowCountWithColumnCount:columnCountIterator
                                                inBounds:boundsSize];
      int idealRowCount = static_cast<int>(
          ceil(static_cast<float>(tabCount) / columnCountIterator));
      if (idealRowCount <= maxRowCount) {
        int score = abs(idealRowCount - columnCountIterator);
        if (score < bestScore) {
          bestScore = score;
          numberOfColumns = columnCountIterator;
          numberOfRows = idealRowCount;
        }
      }
      columnCountIterator += loopDirection;
    }
    DCHECK_NE(bestScore, INT_MAX);
  }

  DCHECK_NE(numberOfColumns, 0);
  DCHECK_NE(numberOfRows, 0);

  // Compute the size of the cells.
  CGFloat horizontalFreeSpace =
      boundsSize.width - (kInterTabSpacing * 2 * (numberOfColumns - 1)) -
      kCollectionViewEdgeInsets.left - kCollectionViewEdgeInsets.right;
  CGFloat verticalFreeSpace =
      boundsSize.height - (kInterTabSpacing * 2 * (numberOfRows - 1)) -
      kCollectionViewEdgeInsets.top - kCollectionViewEdgeInsets.bottom;
  CGSize newCellSize = CGSizeMake(horizontalFreeSpace / numberOfColumns,
                                  verticalFreeSpace / numberOfRows);

  // The cells must not be larger than half of the bounds because the @1x
  // snapshots would look blurry on retina screens.
  newCellSize.width =
      std::min(newCellSize.width, boundsSize.width * kMaxSizeAsAFactorOfBounds);
  newCellSize.height = std::min(newCellSize.height,
                                boundsSize.height * kMaxSizeAsAFactorOfBounds);

  // Avoid having cells be too narrow.
  newCellSize.height = std::min(newCellSize.height,
                                newCellSize.width * kMaxCellHeightWidthRatio);

  [self setItemSize:newCellSize];

  [self setMinimumInteritemSpacing:kInterTabSpacing];
  [self setMinimumLineSpacing:kInterTabSpacing * 2];

  bool forceVerticalCentering = numberOfRows == 1;
  if (forceVerticalCentering) {
    UIEdgeInsets insets = kCollectionViewEdgeInsets;
    insets.top = (boundsSize.height - newCellSize.height) / 2.0;
    insets.bottom = (boundsSize.height - newCellSize.height) / 2.0;
    [self setSectionInset:insets];
  } else {
    [self setSectionInset:kCollectionViewEdgeInsets];
  }
}

- (void)prepareLayout {
  [super prepareLayout];
  [self updateLayoutWithBounds:[[self collectionView] bounds].size];
}

@end
