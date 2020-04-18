// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_PANEL_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_PANEL_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

namespace ios {
class ChromeBrowserState;
}

@protocol ApplicationCommands;
@protocol UrlLoader;

// View controller for displaying the history panel.
@interface HistoryPanelViewController : UIViewController

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@end
#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_PANEL_VIEW_CONTROLLER_H_
