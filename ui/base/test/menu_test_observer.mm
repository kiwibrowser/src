// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/test/menu_test_observer.h"

#include "base/logging.h"
#import "base/mac/objc_release_properties.h"

@implementation MenuTestObserver

@synthesize menu = menu_;
@synthesize isOpen = isOpen_;
@synthesize didOpen = didOpen_;
@synthesize closeAfterOpening = closeAfterOpening_;
@synthesize openCallback = openCallback_;

- (instancetype)initWithMenu:(NSMenu*)menu {
  if ((self = [super init])) {
    menu_ = menu;

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(menuDidBeginTracking:)
                   name:NSMenuDidBeginTrackingNotification
                 object:menu_];
    [center addObserver:self
               selector:@selector(menuDidEndTracking:)
                   name:NSMenuDidEndTrackingNotification
                 object:menu_];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  base::mac::ReleaseProperties(self);
  [super dealloc];
}

- (void)menuDidBeginTracking:(NSNotification*)notif {
  DCHECK_EQ(menu_, [notif object]);
  isOpen_ = YES;
  didOpen_ = YES;

  // Post the callback to the runloop, since in this notification callback,
  // the menu may not be fully in its tracking mode yet.
  NSArray* modes = @[ NSEventTrackingRunLoopMode, NSDefaultRunLoopMode ];
  [self performSelector:@selector(performOpenTasks)
             withObject:nil
             afterDelay:0
                inModes:modes];
}

- (void)menuDidEndTracking:(NSNotification*)notif {
  DCHECK_EQ(menu_, [notif object]);
  isOpen_ = NO;
}

- (void)performOpenTasks {
  DCHECK(isOpen_);

  if (openCallback_)
    openCallback_(self);

  if (closeAfterOpening_)
    [menu_ cancelTracking];
}

@end
