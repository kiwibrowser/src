// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_mediator.h"

#import "ios/chrome/browser/ui/omnibox/omnibox_consumer.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxMediator
@synthesize consumer = _consumer;

#pragma mark - OmniboxLeftImageConsumer

- (void)setLeftImageId:(int)imageId {
  UIImage* image = [NativeImage(imageId)
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  [self.consumer updateAutocompleteIcon:image];
}

@end
