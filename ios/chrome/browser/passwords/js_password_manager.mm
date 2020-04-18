// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/js_password_manager.h"

#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Sanitizes |JSONString| and wraps it in quotes so it can be injected safely in
// JavaScript.
NSString* JSONEscape(NSString* JSONString) {
  return base::SysUTF8ToNSString(
      base::GetQuotedJSONString(base::SysNSStringToUTF8(JSONString)));
}
}  // namespace

@implementation JsPasswordManager {
  // The injection receiver used to evaluate JavaScript.
  CRWJSInjectionReceiver* _receiver;
}

- (instancetype)initWithReceiver:(CRWJSInjectionReceiver*)receiver {
  DCHECK(receiver);
  self = [super init];
  if (self) {
    _receiver = receiver;
  }
  return self;
}

- (void)findPasswordFormsWithCompletionHandler:
    (void (^)(NSString*))completionHandler {
  DCHECK(completionHandler);
  [_receiver executeJavaScript:@"__gCrWeb.passwords.findPasswordForms()"
             completionHandler:^(id result, NSError*) {
               completionHandler(base::mac::ObjCCastStrict<NSString>(result));
             }];
}

- (void)extractForm:(NSString*)formName
      completionHandler:(void (^)(NSString*))completionHandler {
  DCHECK(completionHandler);
  NSString* extra = [NSString
      stringWithFormat:@"__gCrWeb.passwords.getPasswordFormDataAsString(%@)",
                       JSONEscape(formName)];
  [_receiver executeJavaScript:extra
             completionHandler:^(id result, NSError*) {
               completionHandler(base::mac::ObjCCastStrict<NSString>(result));
             }];
}

- (void)fillPasswordForm:(NSString*)JSONString
            withUsername:(NSString*)username
                password:(NSString*)password
       completionHandler:(void (^)(BOOL))completionHandler {
  DCHECK(completionHandler);
  NSString* script = [NSString
      stringWithFormat:@"__gCrWeb.passwords.fillPasswordForm(%@, %@, %@)",
                       JSONString, JSONEscape(username), JSONEscape(password)];
  [_receiver executeJavaScript:script
             completionHandler:^(id result, NSError*) {
               completionHandler([result isEqual:@YES]);
             }];
}


@end
