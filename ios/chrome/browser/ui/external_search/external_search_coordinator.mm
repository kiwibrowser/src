// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_coordinator.h"

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/external_search_commands.h"
#import "ios/chrome/browser/ui/external_search/external_search_mediator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ExternalSearchCoordinator ()

// The mediator registered for ExternalSearchCommands dispatching.
@property(nonatomic) ExternalSearchMediator* mediator;

@end

@implementation ExternalSearchCoordinator

@synthesize dispatcher = _dispatcher;
@synthesize mediator = _mediator;

- (instancetype)init {
  self = [super init];
  if (self) {
    _mediator = [[ExternalSearchMediator alloc] init];
  }
  return self;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher)
    return;
  if (self.dispatcher)
    [self.dispatcher stopDispatchingToTarget:self.mediator];
  [dispatcher startDispatchingToTarget:self.mediator
                           forProtocol:@protocol(ExternalSearchCommands)];
  _dispatcher = dispatcher;
}

- (void)disconnect {
  self.dispatcher = nil;
}

@end
