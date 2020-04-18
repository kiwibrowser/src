// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_BRIDGE_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_BRIDGE_H_

#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"

@class TabModel;
@class TabModelObservers;

// Bridge WebStateListObserver events to TabModelObservers.
@interface TabModelObserversBridge : NSObject<WebStateListObserving>

- (instancetype)initWithTabModel:(TabModel*)tabModel
               tabModelObservers:(TabModelObservers*)tabModelObservers
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_BRIDGE_H_
