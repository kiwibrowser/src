// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/test/test_toolbar_ui_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TestToolbarUIObserver
@synthesize broadcaster = _broadcaster;
@synthesize toolbarHeight = _toolbarHeight;

- (void)setBroadcaster:(ChromeBroadcaster*)broadcaster {
  [_broadcaster removeObserver:self
                   forSelector:@selector(broadcastToolbarHeight:)];
  _broadcaster = broadcaster;
  [_broadcaster addObserver:self
                forSelector:@selector(broadcastToolbarHeight:)];
}

- (void)broadcastToolbarHeight:(CGFloat)toolbarHeight {
  _toolbarHeight = toolbarHeight;
}

@end
