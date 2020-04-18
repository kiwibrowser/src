// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_APPEX_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_APPEX_CONSTANTS_H_

#import <UIKit/UIKit.h>

// Constants for interfacing to other iOS App Extensions.

namespace activity_services {

// Constants to communicate with Password Management App Extensions.

// Key to the App Extension version number. This is for the dictionary sent to
// App Extension.
extern NSString* const kPasswordAppExVersionNumberKey;
// Key to obtain the URL of the current page user was viewing. This is for the
// dictionary sent to App Extension.
extern NSString* const kPasswordAppExURLStringKey;
// Key to obtain the username (provided by App Extension) to sign in to the
// current page.
extern NSString* const kPasswordAppExUsernameKey;
// Key to obtain the password (provided by App Extension) to sign in to the
// current page.
extern NSString* const kPasswordAppExPasswordKey;

// Protocol version number.
extern NSNumber* const kPasswordAppExVersionNumber;

// String identifying the type of data being sent from host application to
// the App Extension.
extern NSString* const kUTTypeAppExtensionFindLoginAction;

}  // namespace activity_services

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_APPEX_CONSTANTS_H_
