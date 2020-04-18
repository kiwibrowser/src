// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_home_promo_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"
#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_table_signin_promo_cell.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BookmarkHomePromoItem
@synthesize delegate = _delegate;

- (instancetype)initWithType:(NSInteger)type {
  if ((self = [super initWithType:type])) {
    self.cellClass = [BookmarkTableSigninPromoCell class];
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)cell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:cell withStyler:styler];
  BookmarkTableSigninPromoCell* signinPromoCell =
      base::mac::ObjCCastStrict<BookmarkTableSigninPromoCell>(cell);
  SigninPromoViewMediator* mediator = self.delegate.signinPromoViewMediator;

  signinPromoCell.signinPromoView.delegate = mediator;
  [[mediator createConfigurator]
      configureSigninPromoView:signinPromoCell.signinPromoView];
  signinPromoCell.selectionStyle = UITableViewCellSelectionStyleNone;
  [mediator signinPromoViewVisible];
}

@end
