// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/mailto/mailto_handler_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

MailtoHandlerProvider::MailtoHandlerProvider() {}

MailtoHandlerProvider::~MailtoHandlerProvider() {}

void MailtoHandlerProvider::PrepareMailtoHandling(
    ios::ChromeBrowserState* browserState) {}

void MailtoHandlerProvider::PrepareMailtoHandling(
    SignedInIdentityBlock signed_in_identity_block,
    SignedInIdentitiesBlock signed_in_identities_block) {}

NSString* MailtoHandlerProvider::MailtoHandlerSettingsTitle() const {
  return nil;
}

UIViewController* MailtoHandlerProvider::MailtoHandlerSettingsController()
    const {
  return nil;
}

void MailtoHandlerProvider::DismissAllMailtoHandlerInterfaces() const {}

void MailtoHandlerProvider::HandleMailtoURL(NSURL* url) const {
  if ([[UIApplication sharedApplication]
          respondsToSelector:@selector(openURL:options:completionHandler:)]) {
    [[UIApplication sharedApplication] openURL:url
                                       options:@{}
                             completionHandler:nil];
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[UIApplication sharedApplication] openURL:url];
#pragma clang diagnostic pop
  }
}
