// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_DETAILS_COLLECTION_VIEW_CONTROLLER_FOR_TESTING_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_DETAILS_COLLECTION_VIEW_CONTROLLER_FOR_TESTING_H_

#import "ios/chrome/browser/ui/settings/password_details_collection_view_controller.h"

@interface PasswordDetailsCollectionViewController (ForTesting)

// Allows to replace a |reauthenticationModule| for a fake one in integration
// tests, where the testing code cannot control the creation of the
// controller.
- (void)setReauthenticationModule:
    (id<ReauthenticationProtocol>)reauthenticationModule;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_DETAILS_COLLECTION_VIEW_CONTROLLER_FOR_TESTING_H_
