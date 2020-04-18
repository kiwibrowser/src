// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/native_content_controller.h"

#import <UIKit/UIKit.h>

#include "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NativeContentController {
  GURL _url;
}

@synthesize view = _view;
@synthesize title = _title;
@synthesize url = _url;

- (instancetype)initWithNibName:(NSString*)nibName url:(const GURL&)url {
  self = [super init];
  if (self) {
    if (nibName.length) {
      [base::mac::FrameworkBundle() loadNibNamed:nibName
                                           owner:self
                                         options:nil];
    }
    _url = url;
  }
  return self;
}

- (instancetype)initWithURL:(const GURL&)url {
  return [self initWithNibName:nil url:url];
}

- (void)dealloc {
  [_view removeFromSuperview];
}

#pragma mark CRWNativeContent

- (BOOL)isViewAlive {
  return YES;
}

- (void)reload {
  // Not implemented in base class.
}

@end
