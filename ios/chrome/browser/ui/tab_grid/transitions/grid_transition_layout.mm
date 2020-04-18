// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/transitions/grid_transition_layout.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "base/logging.h"

@interface GridTransitionLayout ()
@property(nonatomic, readwrite) NSArray<GridTransitionLayoutItem*>* items;
@property(nonatomic, readwrite) GridTransitionLayoutItem* selectedItem;
@end

@implementation GridTransitionLayout
@synthesize selectedItem = _selectedItem;
@synthesize items = _items;

+ (instancetype)layoutWithItems:(NSArray<GridTransitionLayoutItem*>*)items
                   selectedItem:(GridTransitionLayoutItem*)selectedItem {
  DCHECK(items);
  GridTransitionLayout* layout = [[GridTransitionLayout alloc] init];
  layout.items = items;
  layout.selectedItem = selectedItem;
  return layout;
}

- (void)setSelectedItem:(GridTransitionLayoutItem*)selectedItem {
  DCHECK(!selectedItem || [self.items containsObject:selectedItem]);
  _selectedItem = selectedItem;
}

@end

@interface GridTransitionLayoutItem ()
@property(nonatomic, readwrite) UICollectionViewCell* cell;
@property(nonatomic, readwrite) UICollectionViewLayoutAttributes* attributes;
@end

@implementation GridTransitionLayoutItem
@synthesize cell = _cell;
@synthesize attributes = _attributes;

+ (instancetype)itemWithCell:(UICollectionViewCell*)cell
                  attributes:(UICollectionViewLayoutAttributes*)attributes {
  DCHECK(cell);
  DCHECK(attributes);
  DCHECK(!cell.superview);
  GridTransitionLayoutItem* item = [[GridTransitionLayoutItem alloc] init];
  item.cell = cell;
  item.attributes = attributes;
  return item;
}

@end
