// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace offline_pages {

// Asynchronously fetches the offline prefetch related data from the server.
class PrefetchRequestFetcher : public net::URLFetcherDelegate {
 public:
  using FinishedCallback = base::Callback<void(PrefetchRequestStatus status,
                                               const std::string& data)>;

  // Creates a fetcher that will sends a GET request to the server.
  static std::unique_ptr<PrefetchRequestFetcher> CreateForGet(
      const GURL& url,
      net::URLRequestContextGetter* request_context_getter,
      const FinishedCallback& callback);

  // Creates a fetcher that will sends a POST request to the server.
  static std::unique_ptr<PrefetchRequestFetcher> CreateForPost(
      const GURL& url,
      const std::string& message,
      net::URLRequestContextGetter* request_context_getter,
      const FinishedCallback& callback);

  ~PrefetchRequestFetcher() override;

  // URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // If |message| is empty, the GET request is sent. Otherwise, the POST request
  // is sent with |message| as post data.
  PrefetchRequestFetcher(const GURL& url,
                         const std::string& message,
                         net::URLRequestContextGetter* request_context_getter,
                         const FinishedCallback& callback);

  PrefetchRequestStatus ParseResponse(const net::URLFetcher* source,
                                      std::string* data);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;
  FinishedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchRequestFetcher);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_H_
