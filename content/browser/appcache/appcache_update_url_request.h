// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_REQUEST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_REQUEST_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/appcache/appcache_update_request_base.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request.h"

namespace content {

// URLRequest subclass for the UpdateRequestBase class. Provides functionality
// to update the AppCache using functionality provided by the URLRequest class.
class AppCacheUpdateJob::UpdateURLRequest
    : public AppCacheUpdateJob::UpdateRequestBase,
      public net::URLRequest::Delegate {
 public:
  ~UpdateURLRequest() override;

  // UpdateRequestBase overrides.
  void Start() override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  GURL GetURL() const override;
  void SetLoadFlags(int flags) override;
  int GetLoadFlags() const override;
  std::string GetMimeType() const override;
  void SetSiteForCookies(const GURL& site_for_cookies) override;
  void SetInitiator(const base::Optional<url::Origin>& initiator) override;
  net::HttpResponseHeaders* GetResponseHeaders() const override;
  int GetResponseCode() const override;
  const net::HttpResponseInfo& GetResponseInfo() const override;
  void Read() override;
  int Cancel() override;

  // URLRequest::Delegate overrides
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

 private:
  UpdateURLRequest(net::URLRequestContext* request_context,
                   const GURL& url,
                   int buffer_size,
                   URLFetcher* fetcher);

  friend class AppCacheUpdateJob::UpdateRequestBase;

  std::unique_ptr<net::URLRequest> request_;
  URLFetcher* fetcher_;
  scoped_refptr<net::IOBuffer> buffer_;
  int buffer_size_;

  base::WeakPtrFactory<AppCacheUpdateJob::UpdateURLRequest> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpdateURLRequest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_URL_REQUEST_H_
