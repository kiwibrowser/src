// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/app/password_test_util.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/settings/password_details_collection_view_controller_for_testing.h"
#import "ios/chrome/browser/ui/settings/save_passwords_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/settings_navigation_controller.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MockReauthenticationModule

@synthesize localizedReasonForAuthentication =
    _localizedReasonForAuthentication;
@synthesize shouldSucceed = _shouldSucceed;
@synthesize canAttempt = _canAttempt;

- (void)setShouldSucceed:(BOOL)shouldSucceed {
  _canAttempt = YES;
  _shouldSucceed = shouldSucceed;
}

- (BOOL)canAttemptReauth {
  return _canAttempt;
}

- (void)attemptReauthWithLocalizedReason:(NSString*)localizedReason
                    canReusePreviousAuth:(BOOL)canReusePreviousAuth
                                 handler:(void (^)(BOOL success))
                                             showCopyPasswordsHandler {
  self.localizedReasonForAuthentication = localizedReason;
  showCopyPasswordsHandler(_shouldSucceed);
}

@end

namespace chrome_test_util {

// Replace the reauthentication module in
// PasswordDetailsCollectionViewController with a fake one to avoid being
// blocked with a reauth prompt, and return the fake reauthentication module.
MockReauthenticationModule* SetUpAndReturnMockReauthenticationModule() {
  MockReauthenticationModule* mock_reauthentication_module =
      [[MockReauthenticationModule alloc] init];
  // TODO(crbug.com/754642): Stop using TopPresentedViewController();
  SettingsNavigationController* settings_navigation_controller =
      base::mac::ObjCCastStrict<SettingsNavigationController>(
          top_view_controller::TopPresentedViewController());
  PasswordDetailsCollectionViewController*
      password_details_collection_view_controller =
          base::mac::ObjCCastStrict<PasswordDetailsCollectionViewController>(
              settings_navigation_controller.topViewController);
  [password_details_collection_view_controller
      setReauthenticationModule:mock_reauthentication_module];
  return mock_reauthentication_module;
}

// Replace the reauthentication module in
// PasswordExporter with a fake one to avoid being
// blocked with a reauth prompt, and return the fake reauthentication module.
MockReauthenticationModule*
SetUpAndReturnMockReauthenticationModuleForExport() {
  MockReauthenticationModule* mock_reauthentication_module =
      [[MockReauthenticationModule alloc] init];
  // TODO(crbug.com/754642): Stop using TopPresentedViewController();
  SettingsNavigationController* settings_navigation_controller =
      base::mac::ObjCCastStrict<SettingsNavigationController>(
          top_view_controller::TopPresentedViewController());
  SavePasswordsCollectionViewController*
      save_passwords_collection_view_controller =
          base::mac::ObjCCastStrict<SavePasswordsCollectionViewController>(
              settings_navigation_controller.topViewController);
  [save_passwords_collection_view_controller
      setReauthenticationModuleForExporter:mock_reauthentication_module];
  return mock_reauthentication_module;
}

}  // namespace
