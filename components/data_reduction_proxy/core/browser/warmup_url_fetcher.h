// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_WARMUP_URL_FETCHER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_WARMUP_URL_FETCHER_H_

#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "net/url_request/url_fetcher_delegate.h"

class GURL;

namespace net {

class ProxyServer;
class URLFetcher;
class URLRequestContextGetter;

}  // namespace net

namespace data_reduction_proxy {

// URLFetcherDelegate for fetching the warmup URL.
class WarmupURLFetcher : public net::URLFetcherDelegate {
 public:
  enum class FetchResult { kFailed, kSuccessful, kTimedOut };

  // The proxy server that was used to fetch the request, and whether the fetch
  // was successful.
  typedef base::RepeatingCallback<void(const net::ProxyServer&, FetchResult)>
      WarmupURLFetcherCallback;

  WarmupURLFetcher(const scoped_refptr<net::URLRequestContextGetter>&
                       url_request_context_getter,
                   WarmupURLFetcherCallback callback);

  ~WarmupURLFetcher() override;

  // Creates and starts a URLFetcher that fetches the warmup URL.
  // |previous_attempt_counts| is the count of fetch attempts that have been
  // made to the proxy which is being probed. The fetching may happen after some
  // delay depending on |previous_attempt_counts|.
  void FetchWarmupURL(size_t previous_attempt_counts);

  // Returns true if a warmup URL fetch is currently in-flight.
  bool IsFetchInFlight() const;

 protected:
  // Sets |warmup_url_with_query_params| to the warmup URL. Attaches random
  // query params to the warmup URL.
  void GetWarmupURLWithQueryParam(GURL* warmup_url_with_query_params) const;

  // Returns the time for which the fetching of the warmup URL probe should be
  // delayed.
  virtual base::TimeDelta GetFetchWaitTime() const;

  // Returns the timeout value for fetching the secure proxy URL. Virtualized
  // for testing.
  virtual base::TimeDelta GetFetchTimeout() const;

  // Called when the fetch timeouts.
  void OnFetchTimeout();

  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // The URLFetcher being used for fetching the warmup URL. Protected for
  // testing.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // Timer used to delay the fetching of the warmup probe URL.
  base::OneShotTimer fetch_delay_timer_;

  // Timer to enforce timeout of fetching the warmup URL.
  base::OneShotTimer fetch_timeout_timer_;

  // True if the fetcher for warmup URL is in-flight.
  bool is_fetch_in_flight_;

 private:
  // Creates and immediately starts a URLFetcher that fetches the warmup URL.
  void FetchWarmupURLNow();

  // Resets the variable after the fetching of the warmup URL has completed or
  // timed out. Must be called after |callback_| has been run.
  void CleanupAfterFetch();

  // Count of fetch attempts that have been made to the proxy which is being
  // probed.
  size_t previous_attempt_counts_;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Callback that should be executed when the fetching of the warmup URL is
  // completed.
  WarmupURLFetcherCallback callback_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(WarmupURLFetcher);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_WARMUP_URL_FETCHER_H_