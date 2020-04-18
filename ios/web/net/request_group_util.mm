// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/request_group_util.h"

#import <Foundation/Foundation.h>

#include "base/base64.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/sys_string_conversions.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Minimum length for a request group ID. A shorter string is considered as an
// invalid ID.
const int kMinimumIDLength = 5;
}

namespace web {

// Generates a request-group ID used to correlate web requests with the embedder
// that triggered it. It is important that the return value should not be unique
// for different users. See crbug/355613 for context.
NSString* GenerateNewRequestGroupID() {
  const unsigned int kGroupSize = 1000;
  static unsigned long count = 0;
  static unsigned int offset = base::RandInt(0, kGroupSize - 1);

  unsigned long current = count++;
  if (current < kGroupSize)
    current = (current + offset) % kGroupSize;

  // The returned string must have a minimum of kMinimumIDLength characters, and
  // no spaces.
  // TODO(blundell): Develop a long-term solution to this problem.
  // crbug.com/329243
  return [NSString stringWithFormat:@"%06lu", current];
}

NSString* ExtractRequestGroupIDFromUserAgent(NSString* user_agent) {
  if (![user_agent length])
    return nil;

  // The request_group_id is wrapped by parenthesis in the last space-delimited
  // fragment.
  NSString* fragment =
      [[user_agent componentsSeparatedByString:@" "] lastObject];
  NSString* request_group_id =
      [fragment hasPrefix:@"("] && [fragment hasSuffix:@")"]
          ? [fragment substringWithRange:NSMakeRange(1, [fragment length] - 2)]
          : nil;
  // GTLService constructs user agents that end with "(gzip)". To avoid these
  // getting treated as having the request_group_id "gzip", short-circuit out if
  // the request_group_id is not long enough to be a valid request_group_id (all
  // valid request_group_id are at least kMinimumIDLength characters long).
  // TODO(blundell): Develop a long-term solution to this problem.
  // crbug.com/329243
  if ([request_group_id length] < kMinimumIDLength)
    return nil;
  return request_group_id;
}

NSString* AddRequestGroupIDToUserAgent(NSString* base_user_agent,
                                       NSString* request_group_id) {
  if (!request_group_id)
    return base_user_agent;
  // TODO(blundell): Develop a long-term solution to this problem.
  // crbug.com/329243
  DCHECK([request_group_id length] >= kMinimumIDLength);
  return
      [NSString stringWithFormat:@"%@ (%@)", base_user_agent, request_group_id];
}

NSString* ExtractRequestGroupIDFromURL(NSURL* url) {
  GURL gurl = net::GURLWithNSURL(url);
  if (!gurl.has_username())
    return nil;

  std::string request_group_id_as_string;
  if (base::Base64Decode(gurl.username(), &request_group_id_as_string))
    return base::SysUTF8ToNSString(request_group_id_as_string);

  return nil;
}

NSURL* AddRequestGroupIDToURL(NSURL* base_url, NSString* request_group_id) {
  GURL url = net::GURLWithNSURL(base_url);
  std::string base64RequestGroupID;
  base::Base64Encode(base::SysNSStringToUTF8(request_group_id),
                     &base64RequestGroupID);
  GURL::Replacements replacements;
  replacements.SetUsernameStr(base64RequestGroupID);
  url = url.ReplaceComponents(replacements);
  return net::NSURLWithGURL(url);
}

NSString* ExtractRequestGroupIDFromRequest(NSURLRequest* request,
                                           NSString* application_scheme) {
  NSString* user_agent =
      [[request allHTTPHeaderFields] objectForKey:@"User-Agent"];
  NSString* request_group_id = ExtractRequestGroupIDFromUserAgent(user_agent);
  if (request_group_id)
    return request_group_id;
  if (application_scheme &&
      [[request.mainDocumentURL scheme]
          caseInsensitiveCompare:application_scheme] == NSOrderedSame) {
    return ExtractRequestGroupIDFromURL(request.mainDocumentURL);
  }
  return nil;
}

}  // namespace web
