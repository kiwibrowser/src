// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_system_mail.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MailtoHandlerSystemMail

- (instancetype)init {
  NSString* name = l10n_util::GetNSString(IDS_IOS_SYSTEM_MAIL_APP);
  self =
      [super initWithName:name appStoreID:[MailtoHandlerManager systemMailApp]];
  return self;
}

- (BOOL)isAvailable {
  // System Mail client app is always available.
  return YES;
}

- (NSString*)rewriteMailtoURL:(const GURL&)gURL {
  if (!gURL.SchemeIs(url::kMailToScheme))
    return nil;
  return base::SysUTF8ToNSString(gURL.spec());
}

@end
