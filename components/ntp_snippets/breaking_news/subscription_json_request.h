// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_JSON_REQUEST_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_JSON_REQUEST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "components/ntp_snippets/status.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace ntp_snippets {

namespace internal {

// A single request to subscribe for breaking news via GCM. The Request has to
// stay alive in order to be successfully completed.
class SubscriptionJsonRequest : public net::URLFetcherDelegate {
 public:
  // A client can expect a message in the status only, if there was any error
  // during the subscription. In successful cases, it will be an empty string.
  using CompletedCallback = base::OnceCallback<void(const Status& status)>;

  // Builds non-authenticated and authenticated SubscriptionJsonRequests.
  class Builder {
   public:
    Builder();
    Builder(Builder&&);
    ~Builder();

    // Builds a Request object that contains all data to fetch new snippets.
    std::unique_ptr<SubscriptionJsonRequest> Build() const;

    Builder& SetToken(const std::string& token);
    Builder& SetUrl(const GURL& url);
    Builder& SetUrlRequestContextGetter(
        const scoped_refptr<net::URLRequestContextGetter>& context_getter);
    Builder& SetAuthenticationHeader(const std::string& auth_header);

    // The application language represented as an IETF language tag, defined in
    // BCP 47, e.g. "de", "de-AT".
    Builder& SetLocale(const std::string& locale);

    // The device country represented as lowercase ISO 3166-1 alpha-2, e.g.
    // "us", "in".
    // TODO(vitaliii): Use CLDR. Currently this is not possible, because the
    // variations permanent country is not provided in CLDR.
    Builder& SetCountryCode(const std::string& country_code);

   private:
    std::string BuildHeaders() const;
    std::string BuildBody() const;
    std::unique_ptr<net::URLFetcher> BuildURLFetcher(
        net::URLFetcherDelegate* request,
        const std::string& headers,
        const std::string& body) const;

    // GCM subscription token obtained from GCM driver (instanceID::getToken()).
    std::string token_;

    std::string locale_;
    std::string country_code_;

    GURL url_;
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
    std::string auth_header_;

    DISALLOW_COPY_AND_ASSIGN(Builder);
  };

  ~SubscriptionJsonRequest() override;

  // Starts an async request. The callback is invoked when the request succeeds
  // or fails. The callback is not called if the request is destroyed.
  void Start(CompletedCallback callback);

 private:
  friend class Builder;
  SubscriptionJsonRequest();
  // URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // The fetcher for subscribing.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // The callback to notify when URLFetcher finished and results are available.
  // When the request is finished/aborted/destroyed, it's called in the dtor!
  CompletedCallback request_completed_callback_;

  DISALLOW_COPY_AND_ASSIGN(SubscriptionJsonRequest);
};

}  // namespace internal

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_JSON_REQUEST_H_
