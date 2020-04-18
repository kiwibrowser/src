// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_GCM_CHANNEL_STATUS_REQUEST_H_
#define COMPONENTS_GCM_DRIVER_GCM_CHANNEL_STATUS_REQUEST_H_

#include <memory>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/backoff_entry.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

namespace net {
class URLRequestContextGetter;
}

namespace gcm {

// Defines the request to talk with the server to determine if the GCM support
// should be enabled.
class GCMChannelStatusRequest : public net::URLFetcherDelegate {
 public:
  // Callback completing the channel status request.
  // |update_received|: use the existing values if it is false which means no
  //                    update is received.
  // |enabled|: indicates if GCM is enabled (allowed to run) or not.
  // |poll_interval_seconds|: the interval in seconds to start next poll
  //                          request.
  typedef base::Callback<void(bool update_received,
                              bool enabled,
                              int poll_interval_seconds)>
      GCMChannelStatusRequestCallback;

  GCMChannelStatusRequest(
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const std::string& channel_status_request_url,
      const std::string& user_agent,
      const GCMChannelStatusRequestCallback& callback);
  ~GCMChannelStatusRequest() override;

  void Start();

  static int default_poll_interval_seconds();
  static int min_poll_interval_seconds();

 private:
  FRIEND_TEST_ALL_PREFIXES(GCMChannelStatusRequestTest, RequestData);

  // Overridden from URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  bool ParseResponse(const net::URLFetcher* source);
  void RetryWithBackoff(bool update_backoff);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  const std::string channel_status_request_url_;
  const std::string user_agent_;
  GCMChannelStatusRequestCallback callback_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  net::BackoffEntry backoff_entry_;
  base::WeakPtrFactory<GCMChannelStatusRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GCMChannelStatusRequest);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_GCM_CHANNEL_STATUS_REQUEST_H_
