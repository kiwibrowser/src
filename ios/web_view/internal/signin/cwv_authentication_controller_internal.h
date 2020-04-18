// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H_

#import "ios/web_view/public/cwv_authentication_controller.h"

NS_ASSUME_NONNULL_BEGIN

namespace ios_web_view {
class WebViewBrowserState;
}  // namespace ios_web_view

@interface CWVAuthenticationController ()

// |browserState| must outlive this class.
- (instancetype)initWithBrowserState:
    (ios_web_view::WebViewBrowserState*)browserState NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_CWV_AUTHENTICATION_CONTROLLER_INTERNAL_H_
