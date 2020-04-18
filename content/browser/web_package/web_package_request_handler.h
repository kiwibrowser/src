// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_REQUEST_HANDLER_H_
#define CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_REQUEST_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "content/browser/loader/navigation_loader_interceptor.h"
#include "content/public/common/resource_type.h"
#include "url/origin.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace content {

class URLLoaderThrottle;
class WebPackageLoader;

class WebPackageRequestHandler final : public NavigationLoaderInterceptor {
 public:
  using URLLoaderThrottlesGetter = base::RepeatingCallback<
      std::vector<std::unique_ptr<content::URLLoaderThrottle>>()>;

  static bool IsSupportedMimeType(const std::string& mime_type);

  WebPackageRequestHandler(
      url::Origin request_initiator,
      const GURL& url,
      uint32_t url_loader_options,
      int frame_tree_node_id,
      const base::UnguessableToken& devtools_navigation_token,
      bool report_raw_headers,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      URLLoaderThrottlesGetter url_loader_throttles_getter,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter);
  ~WebPackageRequestHandler() override;

  // NavigationLoaderInterceptor implementation
  void MaybeCreateLoader(const network::ResourceRequest& resource_request,
                         ResourceContext* resource_context,
                         LoaderCallback callback) override;
  bool MaybeCreateLoaderForResponse(
      const network::ResourceResponseHead& response,
      network::mojom::URLLoaderPtr* loader,
      network::mojom::URLLoaderClientRequest* client_request,
      ThrottlingURLLoader* url_loader) override;

 private:
  void StartResponse(network::mojom::URLLoaderRequest request,
                     network::mojom::URLLoaderClientPtr client);

  // Valid after MaybeCreateLoaderForResponse intercepts the request and until
  // the loader is re-bound to the new client for the redirected request in
  // StartResponse.
  std::unique_ptr<WebPackageLoader> web_package_loader_;

  url::Origin request_initiator_;
  GURL url_;
  const uint32_t url_loader_options_;
  const int frame_tree_node_id_;
  base::Optional<const base::UnguessableToken> devtools_navigation_token_;
  const bool report_raw_headers_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  URLLoaderThrottlesGetter url_loader_throttles_getter_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  base::WeakPtrFactory<WebPackageRequestHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageRequestHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_WEB_PACKAGE_REQUEST_HANDLER_H_
