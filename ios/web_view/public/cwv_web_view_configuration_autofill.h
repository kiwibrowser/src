// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_AUTOFILL_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_AUTOFILL_H_

#import <Foundation/Foundation.h>

#import "cwv_web_view_configuration.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillDataManager;

@interface CWVWebViewConfiguration (Autofill)

// This web view configuration's autofill data manager.
// nil if CWVWebViewConfiguration is created with +incognitoConfiguration.
@property(nonatomic, readonly, nullable)
    CWVAutofillDataManager* autofillDataManager;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_AUTOFILL_H_
