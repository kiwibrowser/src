// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_GCD_REST_CLIENT_H_
#define REMOTING_HOST_GCD_REST_CLIENT_H_

#include <memory>
#include <queue>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/clock.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "remoting/base/oauth_token_getter.h"
#include "remoting/base/url_request_context_getter.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace remoting {

// A client for calls to the GCD REST API.
class GcdRestClient : public net::URLFetcherDelegate {
 public:
  // Result of a GCD call.
  enum Result {
    SUCCESS,
    NETWORK_ERROR,
    NO_SUCH_HOST,
    OTHER_ERROR,
  };

  typedef base::Callback<void(Result result)> ResultCallback;

  // Note: |token_getter| must outlive this object.
  GcdRestClient(const std::string& gcd_base_url,
                const std::string& gcd_device_id,
                const scoped_refptr<net::URLRequestContextGetter>&
                    url_request_context_getter,
                OAuthTokenGetter* token_getter);

  ~GcdRestClient() override;

  // Tests whether is object is currently running a request.  Only one
  // request at a time may be pending.
  bool HasPendingRequest() { return !!url_fetcher_; }

  // Sends a 'patchState' request to the GCD API.  Constructs and
  // sends an appropriate JSON message M where |patch_details| becomes
  // the value of M.patches[0].patch.
  void PatchState(std::unique_ptr<base::DictionaryValue> patch_details,
                  const GcdRestClient::ResultCallback& callback);

  void SetClockForTest(base::Clock* clock);

 private:
  void OnTokenReceived(OAuthTokenGetter::Status status,
                       const std::string& user_email,
                       const std::string& access_token);
  void FinishCurrentRequest(Result result);

  // URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  std::string gcd_base_url_;
  std::string gcd_device_id_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  OAuthTokenGetter* token_getter_;
  base::Clock* clock_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  ResultCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(GcdRestClient);
};

}  // namespace remoting

#endif  // REMOTING_HOST_GCD_REST_CLIENT_H_
