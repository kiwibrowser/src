// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_coordinator.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalSearchCoordinatorTest = PlatformTest;

// Checks that the dispatcher is reset when -disconnect is called.
TEST_F(ExternalSearchCoordinatorTest, DisconnectResetsDispatcher) {
  ExternalSearchCoordinator* coordinator =
      [[ExternalSearchCoordinator alloc] init];
  CommandDispatcher* dispatcher = [[CommandDispatcher alloc] init];
  coordinator.dispatcher = dispatcher;
  EXPECT_EQ(coordinator.dispatcher, dispatcher);

  [coordinator disconnect];

  EXPECT_EQ(coordinator.dispatcher, nil);
}
