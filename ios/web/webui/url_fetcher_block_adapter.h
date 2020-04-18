// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_
#define IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "base/mac/scoped_block.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace web {

// Class for use of URLFetcher from Objective-C with a completion handler block.
class URLFetcherBlockAdapter;
// Block type for URLFetcherBlockAdapter callbacks.
typedef void (^URLFetcherBlockAdapterCompletion)(NSData*,
                                                 URLFetcherBlockAdapter*);

// Class to manage retrieval of WebUI resources.
class URLFetcherBlockAdapter : public net::URLFetcherDelegate {
 public:
  // Creates URLFetcherBlockAdapter for resource at |url| with
  // |request_context|.
  // |completion_handler| is called with results of the fetch.
  URLFetcherBlockAdapter(
      const GURL& url,
      net::URLRequestContextGetter* request_context,
      web::URLFetcherBlockAdapterCompletion completion_handler);
  ~URLFetcherBlockAdapter() override;
  // Starts the fetch.
  virtual void Start();

 protected:
  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // The URL to fetch.
  const GURL url_;
  // The request context.
  net::URLRequestContextGetter* request_context_;
  // Callback for resource load.
  base::mac::ScopedBlock<web::URLFetcherBlockAdapterCompletion>
      completion_handler_;
  // URLFetcher for retrieving data from net stack.
  std::unique_ptr<net::URLFetcher> fetcher_;
};

}  // namespace web

#endif  // IOS_WEB_WEBUI_URL_FETCHER_BLOCK_ADAPTER_H_
