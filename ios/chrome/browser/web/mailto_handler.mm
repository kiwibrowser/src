// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler.h"

#include <set>

#include "base/strings/string_util.h"
#import "base/strings/sys_string_conversions.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Convenience function to add key with unescaped value to input URL.
GURL AppendQueryParameter(const GURL& input,
                          const std::string& key,
                          const std::string& value) {
  std::string query(input.query());
  if (!query.empty())
    query += "&";
  query += key + "=" + value;
  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return input.ReplaceComponents(replacements);
}

}  // namespace

@implementation MailtoHandler {
  std::set<std::string> _supportedHeaders;
}

@synthesize appName = _appName;
@synthesize appStoreID = _appStoreID;

- (instancetype)initWithName:(NSString*)appName
                  appStoreID:(NSString*)appStoreID {
  self = [super init];
  if (self) {
    _appName = [appName copy];
    _appStoreID = [appStoreID copy];
    // List of typical query parameter keys acceptable to mailto: URLs.
    _supportedHeaders =
        std::set<std::string>({"to", "cc", "bcc", "subject", "body"});
  }
  return self;
}

- (BOOL)isAvailable {
  NSURL* baseURL = [NSURL URLWithString:[self beginningScheme]];
  NSURL* testURL = [NSURL
      URLWithString:[NSString stringWithFormat:@"%@://", [baseURL scheme]]];
  return [[UIApplication sharedApplication] canOpenURL:testURL];
}

- (NSString*)beginningScheme {
  // Subclasses should override this method.
  return @"mailtohandler:/co?";
}

- (NSString*)rewriteMailtoURL:(const GURL&)gURL {
  if (!gURL.SchemeIs(url::kMailToScheme))
    return nil;
  GURL result(base::SysNSStringToUTF8([self beginningScheme]));
  if (gURL.path().length())
    result = AppendQueryParameter(result, "to", gURL.path());
  net::QueryIterator it(gURL);
  while (!it.IsAtEnd()) {
    // Normalizes the keys to all lowercase, but keeps the value unchanged.
    std::string key = base::ToLowerASCII(it.GetKey());
    if (_supportedHeaders.find(key) != _supportedHeaders.end()) {
      result = AppendQueryParameter(result, key, it.GetValue());
    }
    it.Advance();
  }
  return base::SysUTF8ToNSString(result.spec());
}

@end
