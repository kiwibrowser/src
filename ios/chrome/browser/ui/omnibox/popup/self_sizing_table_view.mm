// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/popup/self_sizing_table_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SelfSizingTableView

#pragma mark - UITableView

- (void)setDelegate:(id<UITableViewDelegate>)delegate {
  [super setDelegate:delegate];
  [self invalidateIntrinsicContentSize];
}

- (void)reloadData {
  [super reloadData];
  [self invalidateIntrinsicContentSize];
}

- (void)performBatchUpdates:(void(NS_NOESCAPE ^)())updates
                 completion:(void (^)(BOOL))completion {
  __weak SelfSizingTableView* weakSelf = self;
  [super performBatchUpdates:updates
                  completion:^(BOOL complete) {
                    if (completion) {
                      completion(complete);
                    }
                    [weakSelf invalidateIntrinsicContentSize];
                  }];
}

#pragma mark section changes

- (void)reloadSections:(NSIndexSet*)sections
      withRowAnimation:(UITableViewRowAnimation)animation {
  [super reloadSections:sections withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

- (void)insertSections:(NSIndexSet*)sections
      withRowAnimation:(UITableViewRowAnimation)animation {
  [super insertSections:sections withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

- (void)deleteSections:(NSIndexSet*)sections
      withRowAnimation:(UITableViewRowAnimation)animation {
  [super deleteSections:sections withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

#pragma mark row changes

- (void)reloadRowsAtIndexPaths:(NSArray<NSIndexPath*>*)indexPaths
              withRowAnimation:(UITableViewRowAnimation)animation {
  [super reloadRowsAtIndexPaths:indexPaths withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

- (void)deleteRowsAtIndexPaths:(NSArray<NSIndexPath*>*)indexPaths
              withRowAnimation:(UITableViewRowAnimation)animation {
  [super deleteRowsAtIndexPaths:indexPaths withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

- (void)insertRowsAtIndexPaths:(NSArray<NSIndexPath*>*)indexPaths
              withRowAnimation:(UITableViewRowAnimation)animation {
  [super insertRowsAtIndexPaths:indexPaths withRowAnimation:animation];
  [self invalidateIntrinsicContentSize];
}

#pragma mark - UIView

- (CGSize)intrinsicContentSize {
  // Add height of every cell together.
  CGFloat height = 0;
  for (int section = 0; section < [self numberOfSections]; section++) {
    for (int row = 0; row < [self numberOfRowsInSection:section]; row++) {
      height += [self.delegate tableView:self
                 heightForRowAtIndexPath:[NSIndexPath indexPathForRow:row
                                                            inSection:section]];
    }
  }

  return CGSizeMake(0,
                    height + self.contentInset.top + self.contentInset.bottom);
}

@end
