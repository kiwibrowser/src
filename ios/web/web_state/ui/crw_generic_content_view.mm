// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/ui/crw_generic_content_view.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWGenericContentView () {
  // The size of the view's bounds at the last call to |-layoutSubviews|.
  CGSize _lastLayoutSize;
  // Backing objectect for |self.scrollView|.
  UIScrollView* _scrollView;
  // Backing object for |self.view|.
  UIView* _view;
}

@end

@implementation CRWGenericContentView

- (instancetype)initWithView:(UIView*)view {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK(view);
    _lastLayoutSize = CGSizeZero;
    _view = view;
    _scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
    [self addSubview:_scrollView];
    [_scrollView addSubview:_view];
    [_scrollView setBackgroundColor:[_view backgroundColor]];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)decoder {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

#pragma mark Accessors

- (UIScrollView*)scrollView {
  if (!_scrollView) {
    _scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
  }
  return _scrollView;
}

- (UIEdgeInsets)contentInset {
  return self.scrollView.contentInset;
}

- (void)setContentInset:(UIEdgeInsets)contentInset {
  self.scrollView.contentInset = contentInset;
}

- (UIView*)view {
  return _view;
}

#pragma mark Layout

- (void)layoutSubviews {
  [super layoutSubviews];

  // Early return if the bounds' size hasn't changed since the last layout.
  if (CGSizeEqualToSize(_lastLayoutSize, self.bounds.size))
    return;
  _lastLayoutSize = self.bounds.size;

  // scrollView layout.
  self.scrollView.frame = self.bounds;

  // view layout.
  CGRect contentRect =
      UIEdgeInsetsInsetRect(self.bounds, self.scrollView.contentInset);
  CGSize viewSize = [self.view sizeThatFits:contentRect.size];
  self.view.frame = CGRectMake(0.0, 0.0, viewSize.width, viewSize.height);

  // UIScrollViews only scroll vertically if the content size's height is
  // greater than that of its bounds.
  if (viewSize.height <= _lastLayoutSize.height) {
    CGFloat singlePixel = 1.0f / [[UIScreen mainScreen] scale];
    viewSize.height = _lastLayoutSize.height + singlePixel;
  }
  self.scrollView.contentSize = viewSize;
}

- (BOOL)isViewAlive {
  return YES;
}

@end
