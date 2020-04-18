// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_NETWORK_INTERCEPTOR_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_NETWORK_INTERCEPTOR_H_

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/browser/devtools/protocol/network.h"
#include "content/public/common/resource_type.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/net_errors.h"

namespace content {

struct InterceptedRequestInfo {
  InterceptedRequestInfo();
  ~InterceptedRequestInfo();

  std::string interception_id;
  std::unique_ptr<protocol::Network::Request> network_request;
  base::UnguessableToken frame_id;
  ResourceType resource_type;
  bool is_navigation;
  protocol::Maybe<bool> is_download;
  protocol::Maybe<protocol::Object> redirect_headers;
  protocol::Maybe<int> redirect_status_code;
  protocol::Maybe<protocol::String> redirect_url;
  protocol::Maybe<protocol::Network::AuthChallenge> auth_challenge;
  int response_error_code;
  protocol::Maybe<int> http_response_status_code;
  protocol::Maybe<protocol::Object> response_headers;
};

class DevToolsNetworkInterceptor {
 public:
  virtual ~DevToolsNetworkInterceptor() = default;

  using RequestInterceptedCallback =
      base::RepeatingCallback<void(std::unique_ptr<InterceptedRequestInfo>)>;
  using ContinueInterceptedRequestCallback =
      protocol::Network::Backend::ContinueInterceptedRequestCallback;
  using GetResponseBodyForInterceptionCallback =
      protocol::Network::Backend::GetResponseBodyForInterceptionCallback;
  using TakeResponseBodyPipeCallback =
      base::OnceCallback<void(protocol::Response,
                              mojo::ScopedDataPipeConsumerHandle,
                              const std::string& mime_type)>;

  struct Modifications {
    Modifications();
    Modifications(base::Optional<net::Error> error_reason,
                  base::Optional<std::string> raw_response,
                  protocol::Maybe<std::string> modified_url,
                  protocol::Maybe<std::string> modified_method,
                  protocol::Maybe<std::string> modified_post_data,
                  protocol::Maybe<protocol::Network::Headers> modified_headers,
                  protocol::Maybe<protocol::Network::AuthChallengeResponse>
                      auth_challenge_response,
                  bool mark_as_canceled);
    ~Modifications();

    // If none of the following are set then the request will be allowed to
    // continue unchanged.
    base::Optional<net::Error> error_reason;   // Finish with error.
    base::Optional<std::string> raw_response;  // Finish with mock response.

    // Optionally modify before sending to network.
    protocol::Maybe<std::string> modified_url;
    protocol::Maybe<std::string> modified_method;
    protocol::Maybe<std::string> modified_post_data;
    protocol::Maybe<protocol::Network::Headers> modified_headers;

    // AuthChallengeResponse is mutually exclusive with the above.
    protocol::Maybe<protocol::Network::AuthChallengeResponse>
        auth_challenge_response;

    bool mark_as_canceled;
  };

  enum InterceptionStage {
    DONT_INTERCEPT = 0,
    REQUEST = (1 << 0),
    RESPONSE = (1 << 1),
    // Note: Both is not sent from front-end. It is used if both Request
    // and HeadersReceived was found it upgrades it to Both.
    BOTH = (REQUEST | RESPONSE),
  };

  struct Pattern {
   public:
    ~Pattern();
    Pattern(const Pattern& other);
    Pattern(const std::string& url_pattern,
            base::flat_set<ResourceType> resource_types,
            InterceptionStage interception_stage);

    bool Matches(const std::string& url, ResourceType resource_type) const;

    const std::string url_pattern;
    const base::flat_set<ResourceType> resource_types;
    const InterceptionStage interception_stage;
  };

  struct FilterEntry {
    FilterEntry(const base::UnguessableToken& target_id,
                std::vector<Pattern> patterns,
                RequestInterceptedCallback callback);
    FilterEntry(FilterEntry&&);
    ~FilterEntry();

    const base::UnguessableToken target_id;
    std::vector<Pattern> patterns;
    const RequestInterceptedCallback callback;

    DISALLOW_COPY_AND_ASSIGN(FilterEntry);
  };

  virtual void AddFilterEntry(std::unique_ptr<FilterEntry> entry) = 0;
  virtual void RemoveFilterEntry(const FilterEntry* entry) = 0;
  virtual void UpdatePatterns(FilterEntry* entry,
                              std::vector<Pattern> patterns) = 0;
  virtual void GetResponseBody(
      std::string interception_id,
      std::unique_ptr<GetResponseBodyForInterceptionCallback> callback) = 0;
  virtual void ContinueInterceptedRequest(
      std::string interception_id,
      std::unique_ptr<Modifications> modifications,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback) = 0;
};

inline DevToolsNetworkInterceptor::InterceptionStage& operator|=(
    DevToolsNetworkInterceptor::InterceptionStage& a,
    const DevToolsNetworkInterceptor::InterceptionStage& b) {
  a = static_cast<DevToolsNetworkInterceptor::InterceptionStage>(a | b);
  return a;
}

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_NETWORK_INTERCEPTOR_H_
