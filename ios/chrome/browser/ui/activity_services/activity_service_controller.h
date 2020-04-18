// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_CONTROLLER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/activity_services/share_protocol.h"

@class ShareToData;

// Controller to show the built-in services (e.g. Copy, Printing) and services
// offered by iOS App Extensions (Share, Action).
@interface ActivityServiceController : NSObject<ShareProtocol>

// Return the singleton ActivityServiceController.
+ (instancetype)sharedInstance;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_CONTROLLER_H_
