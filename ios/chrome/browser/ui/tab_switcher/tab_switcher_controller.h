// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"

@protocol ApplicationCommands;

namespace ios {
class ChromeBrowserState;
}

@interface TabSwitcherController : UIViewController<TabSwitcher>

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                        mainTabModel:(TabModel*)mainTabModel
                         otrTabModel:(TabModel*)otrTabModel
                      activeTabModel:(TabModel*)activeTabModel
          applicationCommandEndpoint:(id<ApplicationCommands>)endpoint;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CONTROLLER_H_
