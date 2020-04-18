// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_URL_REQUEST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_URL_REQUEST_H_

#include "content/browser/appcache/appcache_request.h"
#include "content/common/content_export.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

// AppCacheRequest wrapper for the URLRequest class.
class CONTENT_EXPORT AppCacheURLRequest : public AppCacheRequest {
 public:
  // Factory function to create an instance of the AppCacheURLRequest class.
  static std::unique_ptr<AppCacheURLRequest> Create(
      net::URLRequest* url_request);

  ~AppCacheURLRequest() override;

  // AppCacheRequest overrides.
  const GURL& GetURL() const override;
  const std::string& GetMethod() const override;
  const GURL& GetSiteForCookies() const override;
  const GURL GetReferrer() const override;
  bool IsSuccess() const override;
  bool IsCancelled() const override;
  bool IsError() const override;
  int GetResponseCode() const override;
  std::string GetResponseHeaderByName(const std::string& name) const override;
  net::URLRequest* GetURLRequest() override;
  AppCacheURLRequest* AsURLRequest() override;

 protected:
  explicit AppCacheURLRequest(net::URLRequest* url_request);

 private:
  net::URLRequest* url_request_;

  DISALLOW_COPY_AND_ASSIGN(AppCacheURLRequest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_REQUEST_H_
