// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NewTabPageView
@synthesize contentView = _contentView;
@synthesize contentCollectionView = _contentCollectionView;
@synthesize tabBar = tabBar_;
@synthesize safeAreaInsetForToolbar = _safeAreaInsetForToolbar;

- (instancetype)initWithFrame:(CGRect)frame
                    andTabBar:(NewTabPageBar*)tabBar {
  self = [super initWithFrame:frame];
  if (self) {
    [self addSubview:tabBar];
    tabBar_ = tabBar;
  }
  return self;
}

- (void)safeAreaInsetsDidChange {
  self.safeAreaInsetForToolbar = self.safeAreaInsets;
  [super safeAreaInsetsDidChange];
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

#pragma mark - Properties

- (void)setSafeAreaInsetForToolbar:(UIEdgeInsets)safeAreaInsetForToolbar {
  _safeAreaInsetForToolbar = safeAreaInsetForToolbar;
  self.tabBar.safeAreaInsetFromNTPView = safeAreaInsetForToolbar;
}

- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];

  // This should never be needed in autolayout.
  if (self.translatesAutoresizingMaskIntoConstraints) {
    // Trigger a layout.  The |-layoutIfNeeded| call is required because
    // sometimes  |-layoutSubviews| is not successfully triggered when
    // |-setNeedsLayout| is called after frame changes due to autoresizing
    // masks.
    [self setNeedsLayout];
    [self layoutIfNeeded];
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];

  // TODO(crbug.com/807330) Completely remove tabbar once
  // IsUIRefreshPhase1Enabled is defaulted on.
  self.tabBar.hidden = !self.tabBar.items.count || IsUIRefreshPhase1Enabled();
  if (self.tabBar.hidden) {
    self.contentView.frame = self.bounds;
  } else {
    CGSize barSize = [self.tabBar sizeThatFits:self.bounds.size];
    self.tabBar.frame = CGRectMake(CGRectGetMinX(self.bounds),
                                   CGRectGetMaxY(self.bounds) - barSize.height,
                                   barSize.width, barSize.height);
    CGRect previousContentFrame = self.contentView.frame;
    self.contentView.frame = CGRectMake(
        CGRectGetMinX(self.bounds), CGRectGetMinY(self.bounds),
        CGRectGetWidth(self.bounds), CGRectGetMinY(self.tabBar.frame));
    if (!CGRectEqualToRect(previousContentFrame, self.contentView.frame)) {
      [self.contentCollectionView.collectionViewLayout invalidateLayout];
    }
  }

  // When using a new_tab_page_view in autolayout -setFrame is never called,
  // which means all the logic to keep the selected scroll index set is never
  // called.  Rather than refactor away all of this, just make sure -setFrame is
  // called when loaded in autolayout.
  if (!self.translatesAutoresizingMaskIntoConstraints) {
    [self setFrame:self.frame];
  }
}

@end
