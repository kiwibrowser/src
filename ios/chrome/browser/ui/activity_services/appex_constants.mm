// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/appex_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace activity_services {

// Values for these constants to communicate with Password Management iOS App
// Extensions came from https://github.com/AgileBits/onepassword-app-extension
NSString* const kPasswordAppExVersionNumberKey = @"version_number";
NSString* const kPasswordAppExURLStringKey = @"url_string";
NSString* const kPasswordAppExUsernameKey = @"username";
NSString* const kPasswordAppExPasswordKey = @"password";

NSNumber* const kPasswordAppExVersionNumber = @110;

NSString* const kUTTypeAppExtensionFindLoginAction =
    @"org.appextension.chrome-password-action";

}  // namespace activity_services
