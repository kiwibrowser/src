// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ASSIST_RANKER_RANKER_URL_FETCHER_H_
#define COMPONENTS_ASSIST_RANKER_RANKER_URL_FETCHER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace assist_ranker {

// Downloads Ranker models.
class RankerURLFetcher : public net::URLFetcherDelegate {
 public:
  // Callback type for Request().
  typedef base::Callback<void(bool, const std::string&)> Callback;

  // Represents internal state if the fetch is completed successfully.
  enum State {
    IDLE,        // No fetch request was issued.
    REQUESTING,  // A fetch request was issued, but not finished yet.
    COMPLETED,   // The last fetch request was finished successfully.
    FAILED,      // The last fetch request was finished with a failure.
  };

  RankerURLFetcher();
  ~RankerURLFetcher() override;

  int max_retry_on_5xx() { return max_retry_on_5xx_; }
  void set_max_retry_on_5xx(int count) { max_retry_on_5xx_ = count; }

  // Requests to |url|. |callback| will be invoked when the function returns
  // true, and the request is finished asynchronously.
  // Returns false if the previous request is not finished, or the request
  // is omitted due to retry limitation.
  bool Request(const GURL& url,
               const Callback& callback,
               net::URLRequestContextGetter* request_context);

  // Gets internal state.
  State state() { return state_; }

  // net::URLFetcherDelegate implementation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // URL to send the request.
  GURL url_;

  // Internal state.
  enum State state_;

  // URLFetcher instance.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // Callback passed at Request(). It will be invoked when an asynchronous
  // fetch operation is finished.
  Callback callback_;

  // Counts how many times did it try to fetch the language list.
  int retry_count_;

  // Max number how many times to retry on the server error
  int max_retry_on_5xx_;

  DISALLOW_COPY_AND_ASSIGN(RankerURLFetcher);
};

}  // namespace assist_ranker

#endif  // COMPONENTS_ASSIST_RANKER_RANKER_URL_FETCHER_H_
