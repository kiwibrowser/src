// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BOOKMARKS_CELLS_BOOKMARK_HOME_PROMO_ITEM_H_
#define IOS_CHROME_BROWSER_UI_BOOKMARKS_CELLS_BOOKMARK_HOME_PROMO_ITEM_H_

#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

#import <Foundation/Foundation.h>

@class SigninPromoViewMediator;

// BookmarkHomePromoItemDelegate supplies data needed by a
// BookmarkHomePromoItem.
@protocol BookmarkHomePromoItemDelegate<NSObject>

// The SigninPromoViewMediator to use for this item.
@property(nonatomic, readonly) SigninPromoViewMediator* signinPromoViewMediator;

@end

// BookmarkHomePromoItem provides the data for a table view row that displays a
// contextual sign-in promo.
@interface BookmarkHomePromoItem : TableViewItem

// This object's delegate.
@property(nonatomic, weak) id<BookmarkHomePromoItemDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_BOOKMARKS_CELLS_BOOKMARK_HOME_PROMO_ITEM_H_
