// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_INTERCEPTOR_H_
#define CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_INTERCEPTOR_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/common/single_request_url_loader_factory.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {

class ResourceContext;
struct ResourceRequest;
struct SubresourceLoaderParams;
class ThrottlingURLLoader;

// NavigationLoaderInterceptor is given a chance to create a URLLoader and
// intercept a navigation request before the request is handed off to the
// default URLLoader, e.g. the one from the network service.
// NavigationLoaderInterceptor is a per-request object and kept around during
// the lifetime of a navigation request (including multiple redirect legs).
class CONTENT_EXPORT NavigationLoaderInterceptor {
 public:
  NavigationLoaderInterceptor() = default;
  virtual ~NavigationLoaderInterceptor() = default;

  using LoaderCallback =
      base::OnceCallback<void(SingleRequestURLLoaderFactory::RequestHandler)>;

  // Asks this handler to handle this resource load request.
  // The handler must invoke |callback| eventually with either a non-null
  // RequestHandler indicating its willingness to handle the request, or a null
  // RequestHandler to indicate that someone else should handle the request.
  virtual void MaybeCreateLoader(
      const network::ResourceRequest& resource_request,
      ResourceContext* resource_context,
      LoaderCallback callback) = 0;

  // Returns a SubresourceLoaderParams if any to be used for subsequent URL
  // requests going forward. Subclasses who want to set-up custom loader for
  // subresource requests may want to override this.
  // Note that the handler can return a null callback to MaybeCreateLoader(),
  // and at the same time can return non-null SubresourceLoaderParams here if it
  // does NOT want to handle the specific request given to MaybeCreateLoader()
  // but wants to handle the subsequent resource requests.
  virtual base::Optional<SubresourceLoaderParams>
  MaybeCreateSubresourceLoaderParams();

  // Returns true if the handler creates a loader for the |response| passed.
  // An example of where this is used is AppCache, where the handler returns
  // fallback content for the response passed in.
  // The URLLoader interface pointer is returned in the |loader| parameter.
  // The interface request for the URLLoaderClient is returned in the
  // |client_request| parameter.
  // The |url_loader| points to the ThrottlingURLLoader that currently controls
  // the request. It can be optionally consumed to get the current
  // URLLoaderClient and URLLoader so that the implementation can rebind them to
  // intercept the inflight loading if necessary.  Note that the |url_loader|
  // will be reset after this method is called, which will also drop the
  // URLLoader held by |url_loader_| if it is not unbound yet.
  virtual bool MaybeCreateLoaderForResponse(
      const network::ResourceResponseHead& response,
      network::mojom::URLLoaderPtr* loader,
      network::mojom::URLLoaderClientRequest* client_request,
      ThrottlingURLLoader* url_loader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_NAVIGATION_LOADER_INTERCEPTOR_H_
