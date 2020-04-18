// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLS_MENU_BUTTON_OBSERVER_BRIDGE_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLS_MENU_BUTTON_OBSERVER_BRIDGE_H_

#import <UIKit/UIKit.h>

#include "components/reading_list/ios/reading_list_model_bridge_observer.h"

@class ToolbarToolsMenuButton;

// Objective-C bridge informing a ToolbarToolsMenuButton whether the Reading
// List Model contains unseen items.
@interface ToolsMenuButtonObserverBridge
    : NSObject<ReadingListModelBridgeObserver>

// Creates the bridge to the ToolbarToolsMenuButton |button|.
- (instancetype)initWithModel:(ReadingListModel*)readingListModel
                toolbarButton:(ToolbarToolsMenuButton*)button
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLS_MENU_BUTTON_OBSERVER_BRIDGE_H_
