// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_container/browser_container_view_controller.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BrowserContainerViewController () {
  // Weak reference to content view, so old _contentView can be removed from
  // superview when new one is added.
  __weak UIView* _contentView;
}
@end

@implementation BrowserContainerViewController

- (void)dealloc {
  DCHECK(![_contentView superview] || [_contentView superview] == self.view);
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

#pragma mark - Public

- (void)displayContentView:(UIView*)contentView {
  if (_contentView == contentView)
    return;

  DCHECK(![_contentView superview] || [_contentView superview] == self.view);
  [_contentView removeFromSuperview];
  _contentView = contentView;

  if (contentView)
    [self.view addSubview:contentView];
}

@end
