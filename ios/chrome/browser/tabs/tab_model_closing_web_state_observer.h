// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_CLOSING_WEB_STATE_OBSERVER_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_CLOSING_WEB_STATE_OBSERVER_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"

@class TabModel;
namespace sessions {
class TabRestoreService;
}

// Observes WebStateList events about closing WebState.
@interface TabModelClosingWebStateObserver : NSObject<WebStateListObserving>

- (instancetype)initWithTabModel:(TabModel*)tabModel
                  restoreService:(sessions::TabRestoreService*)restoreService
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_CLOSING_WEB_STATE_OBSERVER_H_
