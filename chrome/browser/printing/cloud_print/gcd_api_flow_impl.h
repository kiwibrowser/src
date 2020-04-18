// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_CLOUD_PRINT_GCD_API_FLOW_IMPL_H_
#define CHROME_BROWSER_PRINTING_CLOUD_PRINT_GCD_API_FLOW_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/printing/cloud_print/gcd_api_flow.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace cloud_print {

class GCDApiFlowImpl : public GCDApiFlow,
                       public net::URLFetcherDelegate,
                       public OAuth2TokenService::Consumer {
 public:
  // Create an OAuth2-based confirmation.
  GCDApiFlowImpl(net::URLRequestContextGetter* request_context,
                 OAuth2TokenService* token_service,
                 const std::string& account_id);
  ~GCDApiFlowImpl() override;

  // GCDApiFlow implementation:
  void Start(std::unique_ptr<Request> request) override;

  // net::URLFetcherDelegate implementation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // OAuth2TokenService::Consumer implementation:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

 private:
  void CreateRequest();

  std::unique_ptr<net::URLFetcher> url_fetcher_;
  std::unique_ptr<OAuth2TokenService::Request> oauth_request_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  OAuth2TokenService* token_service_;
  std::string account_id_;
  std::unique_ptr<Request> request_;

  DISALLOW_COPY_AND_ASSIGN(GCDApiFlowImpl);
};

}  // namespace cloud_print

#endif  // CHROME_BROWSER_PRINTING_CLOUD_PRINT_GCD_API_FLOW_IMPL_H_
