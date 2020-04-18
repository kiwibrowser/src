// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/geolocation/omnibox_geolocation_authorization_alert.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/ui/util/top_view_controller.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxGeolocationAuthorizationAlert
@synthesize delegate = _delegate;

- (instancetype)initWithDelegate:
        (id<OmniboxGeolocationAuthorizationAlertDelegate>)delegate {
  self = [super init];
  if (self) {
    _delegate = delegate;
  }
  return self;
}

- (instancetype)init {
  return [self initWithDelegate:nil];
}

- (void)showAuthorizationAlert {
  NSString* message =
      l10n_util::GetNSString(IDS_IOS_LOCATION_AUTHORIZATION_ALERT);
  NSString* cancel = l10n_util::GetNSString(IDS_NOT_NOW);
  NSString* ok = l10n_util::GetNSString(IDS_OK);

  // Use a UIAlertController to match the style of the iOS system location
  // alert.
  __weak OmniboxGeolocationAuthorizationAlert* weakSelf = self;
  UIAlertController* alert =
      [UIAlertController alertControllerWithTitle:nil
                                          message:message
                                   preferredStyle:UIAlertControllerStyleAlert];

  UIAlertAction* defaultAction = [UIAlertAction
      actionWithTitle:ok
                style:UIAlertActionStyleDefault
              handler:^(UIAlertAction* action) {
                OmniboxGeolocationAuthorizationAlert* strongSelf = weakSelf;
                if (strongSelf) {
                  [[strongSelf delegate]
                      authorizationAlertDidAuthorize:strongSelf];
                }
              }];
  [alert addAction:defaultAction];

  UIAlertAction* cancelAction = [UIAlertAction
      actionWithTitle:cancel
                style:UIAlertActionStyleCancel
              handler:^(UIAlertAction* action) {
                OmniboxGeolocationAuthorizationAlert* strongSelf = weakSelf;
                if (strongSelf) {
                  [[strongSelf delegate]
                      authorizationAlertDidCancel:strongSelf];
                }
              }];
  [alert addAction:cancelAction];

  // TODO(crbug.com/754642): Remove use of TopPresentedViewController().
  [top_view_controller::TopPresentedViewController() presentViewController:alert
                                                                  animated:YES
                                                                completion:nil];
}

@end
