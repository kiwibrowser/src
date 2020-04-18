// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_
#define COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace translate {

// Downloads raw Translate data such as the Translate script and the language
// list.
class TranslateURLFetcher : public net::URLFetcherDelegate {
 public:
  // Callback type for Request().
  typedef base::Callback<void(int, bool, const std::string&)> Callback;

  // Represents internal state if the fetch is completed successfully.
  enum State {
    IDLE,        // No fetch request was issued.
    REQUESTING,  // A fetch request was issued, but not finished yet.
    COMPLETED,   // The last fetch request was finished successfully.
    FAILED,      // The last fetch request was finished with a failure.
  };

  explicit TranslateURLFetcher(int id);
  ~TranslateURLFetcher() override;

  int max_retry_on_5xx() {
    return max_retry_on_5xx_;
  }
  void set_max_retry_on_5xx(int count) {
    max_retry_on_5xx_ = count;
  }

  const std::string& extra_request_header() {
    return extra_request_header_;
  }
  void set_extra_request_header(const std::string& header) {
    extra_request_header_ = header;
  }

  // Requests to |url|. |callback| will be invoked when the function returns
  // true, and the request is finished asynchronously.
  // Returns false if the previous request is not finished, or the request
  // is omitted due to retry limitation.
  bool Request(const GURL& url, const Callback& callback);

  // Gets internal state.
  State state() { return state_; }

  // net::URLFetcherDelegate implementation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // URL to send the request.
  GURL url_;

  // ID which is assigned to the URLFetcher.
  const int id_;

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

  // An extra HTTP request header
  std::string extra_request_header_;

  DISALLOW_COPY_AND_ASSIGN(TranslateURLFetcher);
};

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_URL_FETCHER_H_
