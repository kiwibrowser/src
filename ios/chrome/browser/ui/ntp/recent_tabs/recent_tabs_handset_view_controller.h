// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_RECENT_TABS_HANDSET_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_RECENT_TABS_HANDSET_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// Command protocol for the RecentTabsHandsetViewController.
@protocol RecentTabsHandsetViewControllerCommand

// Dismisses the recent tabs panel and calls |completion| once it is done.
- (void)dismissRecentTabsWithCompletion:(void (^)())completion;

@end

// UIViewController wrapper for RecentTabTableViewController for modal display.
@interface RecentTabsHandsetViewController : UIViewController

- (instancetype)initWithViewController:(UIViewController*)panelViewController
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@property(nonatomic, weak) id<RecentTabsHandsetViewControllerCommand>
    commandHandler;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_RECENT_TABS_RECENT_TABS_HANDSET_VIEW_CONTROLLER_H_
