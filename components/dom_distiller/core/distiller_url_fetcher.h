// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_DISTILLER_URL_FETCHER_H_
#define COMPONENTS_DOM_DISTILLER_CORE_DISTILLER_URL_FETCHER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

namespace dom_distiller {

class DistillerURLFetcher;

// Class for creating a DistillerURLFetcher.
class DistillerURLFetcherFactory {
 public:
  explicit DistillerURLFetcherFactory(
      net::URLRequestContextGetter* context_getter);
  virtual ~DistillerURLFetcherFactory() {}
  virtual DistillerURLFetcher* CreateDistillerURLFetcher() const;

 private:
  net::URLRequestContextGetter* context_getter_;
};

// This class fetches a URL, and notifies the caller when the operation
// completes or fails. If the request fails, an empty string will be returned.
class DistillerURLFetcher : public net::URLFetcherDelegate {
 public:
  explicit DistillerURLFetcher(net::URLRequestContextGetter* context_getter);
  ~DistillerURLFetcher() override;

  // Indicates when a fetch is done.
  typedef base::Callback<void(const std::string& data)> URLFetcherCallback;

  // Fetches a |url|. Notifies when the fetch is done via |callback|.
  virtual void FetchURL(const std::string& url,
                        const URLFetcherCallback& callback);

 protected:
  virtual std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      net::URLRequestContextGetter* context_getter,
      const std::string& url);

 private:
  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  std::unique_ptr<net::URLFetcher> url_fetcher_;
  URLFetcherCallback callback_;
  net::URLRequestContextGetter* context_getter_;
  DISALLOW_COPY_AND_ASSIGN(DistillerURLFetcher);
};

}  //  namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_DISTILLER_URL_FETCHER_H_
