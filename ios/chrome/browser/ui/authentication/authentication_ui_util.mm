// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/authentication_ui_util.h"

#include "base/format_macros.h"
#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

AlertCoordinator* ErrorCoordinator(NSError* error,
                                   ProceduralBlock dismissAction,
                                   UIViewController* viewController) {
  DCHECK(error);

  AlertCoordinator* alertCoordinator =
      ErrorCoordinatorNoItem(error, viewController);

  NSString* okButtonLabel = l10n_util::GetNSString(IDS_OK);
  [alertCoordinator addItemWithTitle:okButtonLabel
                              action:dismissAction
                               style:UIAlertActionStyleDefault];

  [alertCoordinator setCancelAction:dismissAction];

  return alertCoordinator;
}

NSString* DialogMessageFromError(NSError* error) {
  NSMutableString* errorMessage = [[NSMutableString alloc] init];
  if (error.userInfo[NSLocalizedDescriptionKey]) {
    [errorMessage appendString:error.localizedDescription];
  } else {
    [errorMessage appendString:@"--"];
  }
  [errorMessage appendString:@" ("];
  NSError* errorCursor = error;
  for (int errorDepth = 0; errorDepth < 3 && errorCursor; ++errorDepth) {
    if (errorDepth > 0) {
      [errorMessage appendString:@", "];
    }
    [errorMessage
        appendFormat:@"%@: %" PRIdNS, errorCursor.domain, errorCursor.code];
    errorCursor = errorCursor.userInfo[NSUnderlyingErrorKey];
  }
  [errorMessage appendString:@")"];
  return [errorMessage copy];
}

AlertCoordinator* ErrorCoordinatorNoItem(NSError* error,
                                         UIViewController* viewController) {
  DCHECK(error);

  NSString* title = l10n_util::GetNSString(
      IDS_IOS_SYNC_AUTHENTICATION_ERROR_ALERT_VIEW_TITLE);
  NSString* errorMessage;
  if ([NSURLErrorDomain isEqualToString:error.domain] &&
      error.code == kCFURLErrorCannotConnectToHost) {
    errorMessage =
        l10n_util::GetNSString(IDS_IOS_SYNC_ERROR_INTERNET_DISCONNECTED);
  } else {
    errorMessage = DialogMessageFromError(error);
  }
  AlertCoordinator* alertCoordinator =
      [[AlertCoordinator alloc] initWithBaseViewController:viewController
                                                     title:title
                                                   message:errorMessage];
  return alertCoordinator;
}
