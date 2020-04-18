// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_SIGNIN_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_SIGNIN_H_

#import <Foundation/Foundation.h>

#import "cwv_web_view_configuration.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAuthenticationController;

@interface CWVWebViewConfiguration (Signin)

// This web view configuration's authentication controller.
// nil if CWVWebViewConfiguration is created with +incognitoConfiguration.
@property(nonatomic, readonly, nullable)
    CWVAuthenticationController* authenticationController;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_SIGNIN_H_
