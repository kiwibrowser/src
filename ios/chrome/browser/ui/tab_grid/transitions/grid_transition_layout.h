// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_LAYOUT_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_LAYOUT_H_

#import <UIKit/UIKit.h>

@class GridTransitionLayoutItem;

// An encapsulation of information for the layout of a grid of cells that will
// be used in an animated transition. The layout object is composed of layout
// items (see below).
@interface GridTransitionLayout : NSObject

// All of the items in the layout.
@property(nonatomic, copy, readonly) NSArray<GridTransitionLayoutItem*>* items;
// The item in the layout (if any) that's selected.
// Note that |selectedItem.cell.selected| doesn't need to be YES; the transition
// animation may set or unset that selection state as part of the animation.
@property(nonatomic, strong, readonly) GridTransitionLayoutItem* selectedItem;

// Creates a new layout object with |items|, and |selectedItem| selected.
// |items| should be non-nil, but it may be empty.
// |selectedItem| must either be nil, or one of the members of |items|.
+ (instancetype)layoutWithItems:(NSArray<GridTransitionLayoutItem*>*)items
                   selectedItem:(GridTransitionLayoutItem*)selectedItem;

@end

// An encapsulation of information for the layout of a single grid cell, in the
// form of UICollectionView classes.
@interface GridTransitionLayoutItem : NSObject

// A cell object with the desired appearance for the animation. This should
// correspond to an actual cell in the collection view involved in the trans-
// ition, but the value of thie property should not be in any view hierarchy
// when the layout item is created.
@property(nonatomic, strong, readonly) UICollectionViewCell* cell;
// The layout attributes for the cell in the collection view, normalized to
// UIWindow coordinates. It's the responsibility of the setter to do this
// normalization.
@property(nonatomic, strong, readonly)
    UICollectionViewLayoutAttributes* attributes;

// Creates a new layout item instance will |cell| and |attributes|, neither of
// which can be nil.
// It's an error if |cell| has a superview.
// The properties (size, etc) of |attributes| don't need to match the corres-
// ponding properties of |cell| when the item is created.
+ (instancetype)itemWithCell:(UICollectionViewCell*)cell
                  attributes:(UICollectionViewLayoutAttributes*)attributes;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_LAYOUT_H_
