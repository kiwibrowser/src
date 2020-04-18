// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/app_launcher_util.h"

#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* GetFormattedAbsoluteUrlWithSchemeRemoved(NSURL* url) {
  NSCharacterSet* char_set =
      [NSCharacterSet characterSetWithCharactersInString:@"/"];
  NSString* scheme = [NSString stringWithFormat:@"%@:", url.scheme];
  NSString* url_string = url.absoluteString;
  if (url_string.length <= scheme.length)
    return url_string;
  NSString* prompt = [[[url.absoluteString substringFromIndex:scheme.length]
      stringByTrimmingCharactersInSet:char_set]
      stringByRemovingPercentEncoding];
  // Returns original URL string if there's nothing interesting to display
  // other than the scheme itself.
  if (!prompt.length)
    return url_string;
  return prompt;
}

NSSet<NSString*>* GetItmsSchemes() {
  static NSSet<NSString*>* schemes;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    schemes = [NSSet<NSString*>
        setWithObjects:@"itms", @"itmss", @"itms-apps", @"itms-appss",
                       // There's no evidence that itms-bookss is actually
                       // supported, but over-inclusion costs less than
                       // under-inclusion.
                       @"itms-books", @"itms-bookss", nil];
  });
  return schemes;
}

bool UrlHasAppStoreScheme(const GURL& url) {
  return
      [GetItmsSchemes() containsObject:base::SysUTF8ToNSString(url.scheme())];
}

bool UrlHasPhoneCallScheme(const GURL& url) {
  return url.SchemeIs("tel") || url.SchemeIs("facetime") ||
         url.SchemeIs("facetime-audio");
}

NSString* GetPromptActionString(NSString* scheme) {
  if ([scheme isEqualToString:@"facetime"])
    return l10n_util::GetNSString(IDS_IOS_FACETIME_BUTTON);
  else if ([scheme isEqualToString:@"tel"] ||
           [scheme isEqualToString:@"facetime-audio"]) {
    return l10n_util::GetNSString(IDS_IOS_PHONE_CALL_BUTTON);
  }
  NOTREACHED();
  return @"";
}
