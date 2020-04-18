// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_PRINT_ACTIVITY_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_PRINT_ACTIVITY_H_

#import <UIKit/UIKit.h>

@protocol BrowserCommands;

// Activity that triggers the printing service.
@interface PrintActivity : UIActivity

// The dispatcher that handles when the activity is performed.
@property(nonatomic, weak) id<BrowserCommands> dispatcher;

// Identifier for the print activity.
+ (NSString*)activityIdentifier;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_PRINT_ACTIVITY_H_
