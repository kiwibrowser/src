// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_AUTHENTICATION_UI_UTIL_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_AUTHENTICATION_UI_UTIL_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"

@class AlertCoordinator;

// Returns the sign in alert coordinator for |error|. |dismissAction| is called
// when the dialog is dismissed (the user taps on the Ok button) or cancelled
// (the alert coordinator is cancelled programatically).
AlertCoordinator* ErrorCoordinator(NSError* error,
                                   ProceduralBlock dismissAction,
                                   UIViewController* viewController);

// Returns a message to display, as an error, to the user. This message
// contains:
//  * localized description (if any)
//  * domain name
//  * error code
//  * underlying errors recursively (only the domain name and the error code)
NSString* DialogMessageFromError(NSError* error);

// Returns the sign in alert coordinator for |error|, with no associated
// action. An action should be added before starting it.
AlertCoordinator* ErrorCoordinatorNoItem(NSError* error,
                                         UIViewController* viewController);

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_AUTHENTICATION_UI_UTIL_H_
