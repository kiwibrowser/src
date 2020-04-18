// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NET_REQUEST_GROUP_UTIL_H_
#define IOS_WEB_NET_REQUEST_GROUP_UTIL_H_

@class NSString;
@class NSURL;
@class NSURLRequest;

// Request group IDs are internally used by the web layer to associate network
// requests to RequestTracker instances, and in turn, to WebState instances.
// The ID can be added to the user-agent string by UIWebView in most cases and
// then extracted by the network layer.
// However, when using non-standard schemes, UIWebView does not add the
// "User-Agent" HTTP header to the requests. The workaround for this case is to
// add the ID directly in the URL of the main request (which is the only request
// accessible from the UIWebView delegate).

namespace web {

// Generates a request-group ID.
NSString* GenerateNewRequestGroupID();

// Extracts the requestGroupID embedded in a User-Agent string or nil if a
// requestGroupID cannot be located.
NSString* ExtractRequestGroupIDFromUserAgent(NSString* user_agent);

// Returns a new user agent, which is the result of the encoding of
// |request_group_id| in |base_user_agent|. The request group ID can be later
// extracted with ExtractRequestGroupIDFromUserAgent().
// If request_group_id is nil, returns base_user_agent.
NSString* AddRequestGroupIDToUserAgent(NSString* base_user_agent,
                                       NSString* request_group_id);

// Extracts the requestGroupID embedded in a NSURL or nil if a requestGroupID
// cannot be located.
NSString* ExtractRequestGroupIDFromURL(NSURL* url);

// Returns a new user agent, which is the result of the encoding of
// |request_group_id| in |base_url|. The request group ID can be later extracted
// with ExtractRequestGroupIDFromURL().
NSURL* AddRequestGroupIDToURL(NSURL* base_url, NSString* request_group_id);

// Extracts the request group ID from |request| by retrieving it from the
// user-agent if possible, and from the parent URL otherwise.
// The ID can only be retrived from the parent URL if its scheme is
// |application_scheme|.
// The reason why the |application_scheme| case is different is because
// UIWebView does not provide a "User-Agent" HTTP header for these requests.
// The ID is then encoded in the URL of the main request, and thus sub-requests
// have to look in their parent URL for the ID.
NSString* ExtractRequestGroupIDFromRequest(NSURLRequest* request,
                                           NSString* application_scheme);

}  // namespace web

#endif  // IOS_WEB_NET_REQUEST_GROUP_UTIL_H_
