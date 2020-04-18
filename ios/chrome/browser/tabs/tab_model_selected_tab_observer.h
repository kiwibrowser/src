// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_SELECTED_TAB_OBSERVER_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_SELECTED_TAB_OBSERVER_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"

@class TabModel;

// Listen to WebStateList active WebState change events and then save the
// old Tab state, fire notification and save the session if necessary.
@interface TabModelSelectedTabObserver : NSObject<WebStateListObserving>

- (instancetype)initWithTabModel:(TabModel*)tabModel NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_SELECTED_TAB_OBSERVER_H_
