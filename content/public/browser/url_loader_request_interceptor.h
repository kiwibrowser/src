// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_URL_LOADER_REQUEST_INTERCEPTOR_H_
#define CONTENT_PUBLIC_BROWSER_URL_LOADER_REQUEST_INTERCEPTOR_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace network {
struct ResourceRequest;
}

namespace content {

class ResourceContext;

// URLLoaderRequestInterceptor is given a chance to create a URLLoader and
// intercept a navigation request before the request is handed off to the
// default URLLoader, e.g. the one from the network service.
// URLLoaderRequestInterceptor is a per-request object and kept around during
// the lifetime of a navigation request (including multiple redirect legs).
class URLLoaderRequestInterceptor {
 public:
  URLLoaderRequestInterceptor() = default;
  virtual ~URLLoaderRequestInterceptor() = default;

  using RequestHandler =
      base::OnceCallback<void(network::mojom::URLLoaderRequest,
                              network::mojom::URLLoaderClientPtr)>;
  using LoaderCallback = base::OnceCallback<void(RequestHandler)>;

  // Asks this handler to handle this resource load request.
  // The handler must invoke |callback| eventually with either a non-null
  // RequestHandler indicating its willingness to handle the request, or a null
  // RequestHandler to indicate that someone else should handle the request.
  virtual void MaybeCreateLoader(
      const network::ResourceRequest& resource_request,
      ResourceContext* resource_context,
      LoaderCallback callback) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_URL_LOADER_REQUEST_INTERCEPTOR_H_
