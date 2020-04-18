// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/snackbar/snackbar_coordinator.h"

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/snackbar_commands.h"
#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SnackbarCoordinator
@synthesize dispatcher = _dispatcher;

- (void)start {
  [self.dispatcher startDispatchingToTarget:self
                                forProtocol:@protocol(SnackbarCommands)];
}

- (void)stop {
  [self.dispatcher stopDispatchingToTarget:self];
}

#pragma mark - SnackbarCommands

- (void)showSnackbarMessage:(MDCSnackbarMessage*)message {
  [MDCSnackbarManager showMessage:message];
}

@end
