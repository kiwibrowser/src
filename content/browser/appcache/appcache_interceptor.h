// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_INTERCEPTOR_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_INTERCEPTOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request_interceptor.h"

class GURL;

namespace net {
class URLRequest;
}

namespace content {
class AppCacheHost;
class AppCacheRequestHandler;
class AppCacheServiceImpl;

// An interceptor to hijack requests and potentially service them out of
// the appcache.
class CONTENT_EXPORT AppCacheInterceptor : public net::URLRequestInterceptor {
 public:
  // Must be called to make a request eligible for retrieval from an appcache.
  static void SetExtraRequestInfo(net::URLRequest* request,
                                  AppCacheServiceImpl* service,
                                  int process_id,
                                  int host_id,
                                  ResourceType resource_type,
                                  bool should_reset_appcache);

  // PlzNavigate
  // Must be called to make a request eligible for retrieval from an appcache.
  static void SetExtraRequestInfoForHost(net::URLRequest* request,
                                         AppCacheHost* host,
                                         ResourceType resource_type,
                                         bool should_reset_appcache);

  // May be called after response headers are complete to retrieve extra
  // info about the response.
  static void GetExtraResponseInfo(net::URLRequest* request,
                                   int64_t* cache_id,
                                   GURL* manifest_url);

  AppCacheInterceptor();
  ~AppCacheInterceptor() override;

 protected:
  // Override from net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;

 private:
  static void SetHandler(net::URLRequest* request,
                         std::unique_ptr<AppCacheRequestHandler> handler);
  static AppCacheRequestHandler* GetHandler(net::URLRequest* request);

  DISALLOW_COPY_AND_ASSIGN(AppCacheInterceptor);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_INTERCEPTOR_H_
