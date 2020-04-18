// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_H_

#import "base/ios/crb_protocol_observers.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"

@interface TabModelObservers : CRBProtocolObservers<TabModelObserver>

+ (instancetype)observers;

@end

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_OBSERVERS_H_
