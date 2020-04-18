// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_network_interceptor.h"
#include "base/strings/pattern.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "url/gurl.h"

namespace content {

InterceptedRequestInfo::InterceptedRequestInfo()
    : is_navigation(false), response_error_code(net::OK) {}

InterceptedRequestInfo::~InterceptedRequestInfo() = default;

DevToolsNetworkInterceptor::FilterEntry::FilterEntry(
    const base::UnguessableToken& target_id,
    std::vector<Pattern> patterns,
    RequestInterceptedCallback callback)
    : target_id(target_id),
      patterns(std::move(patterns)),
      callback(std::move(callback)) {}

DevToolsNetworkInterceptor::FilterEntry::FilterEntry(FilterEntry&&) {}
DevToolsNetworkInterceptor::FilterEntry::~FilterEntry() {}

DevToolsNetworkInterceptor::Modifications::Modifications()
    : mark_as_canceled(false) {}

DevToolsNetworkInterceptor::Modifications::Modifications(
    base::Optional<net::Error> error_reason,
    base::Optional<std::string> raw_response,
    protocol::Maybe<std::string> modified_url,
    protocol::Maybe<std::string> modified_method,
    protocol::Maybe<std::string> modified_post_data,
    protocol::Maybe<protocol::Network::Headers> modified_headers,
    protocol::Maybe<protocol::Network::AuthChallengeResponse>
        auth_challenge_response,
    bool mark_as_canceled)
    : error_reason(std::move(error_reason)),
      raw_response(std::move(raw_response)),
      modified_url(std::move(modified_url)),
      modified_method(std::move(modified_method)),
      modified_post_data(std::move(modified_post_data)),
      modified_headers(std::move(modified_headers)),
      auth_challenge_response(std::move(auth_challenge_response)),
      mark_as_canceled(mark_as_canceled) {}

DevToolsNetworkInterceptor::Modifications::~Modifications() {}

DevToolsNetworkInterceptor::Pattern::~Pattern() = default;

DevToolsNetworkInterceptor::Pattern::Pattern(const Pattern& other) = default;

DevToolsNetworkInterceptor::Pattern::Pattern(
    const std::string& url_pattern,
    base::flat_set<ResourceType> resource_types,
    InterceptionStage interception_stage)
    : url_pattern(url_pattern),
      resource_types(std::move(resource_types)),
      interception_stage(interception_stage) {}

bool DevToolsNetworkInterceptor::Pattern::Matches(
    const std::string& url,
    ResourceType resource_type) const {
  if (!resource_types.empty() &&
      resource_types.find(resource_type) == resource_types.end()) {
    return false;
  }
  return base::MatchPattern(url, url_pattern);
}

}  // namespace content
