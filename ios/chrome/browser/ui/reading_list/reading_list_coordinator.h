// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_controller.h"

namespace ios {
class ChromeBrowserState;
}

@class ReadingListMediator;
@protocol UrlLoader;

// Coordinator for Reading List, displaying the Reading List when starting.
@interface ReadingListCoordinator
    : ChromeCoordinator<ReadingListCollectionViewControllerDelegate>

// Mediator used by this coordinator. Reset when |-start| is called.
@property(nonatomic, strong, nullable) ReadingListMediator* mediator;

- (nullable instancetype)
initWithBaseViewController:(nullable UIViewController*)viewController
              browserState:(nonnull ios::ChromeBrowserState*)browserState
                    loader:(nullable id<UrlLoader>)loader
    NS_DESIGNATED_INITIALIZER;

- (nullable instancetype)initWithBaseViewController:
    (nullable UIViewController*)viewController NS_UNAVAILABLE;
- (nullable instancetype)
initWithBaseViewController:(nullable UIViewController*)viewController
              browserState:(nullable ios::ChromeBrowserState*)browserState
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COORDINATOR_H_
