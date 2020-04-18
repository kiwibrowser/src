// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_URL_LOADER_THROTTLE_H_
#define CONTENT_PUBLIC_COMMON_URL_LOADER_THROTTLE_H_

#include "base/strings/string_piece.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "net/base/request_priority.h"

class GURL;

namespace net {
struct RedirectInfo;
}

namespace network {
struct ResourceRequest;
struct ResourceResponseHead;
}

namespace content {

// A URLLoaderThrottle gets notified at various points during the process of
// loading a resource. At each stage, it has the opportunity to defer the
// resource load.
//
// Note that while a single throttle deferring a load at any given step will
// block the load from progressing further until a subsequent Delegate::Resume()
// call is made, it does NOT prevent subsequent throttles from processing the
// same step of the request if multiple throttles are affecting the load.
class CONTENT_EXPORT URLLoaderThrottle {
 public:
  // An interface for the throttle implementation to resume (when deferred) or
  // cancel the resource load. Please note that these methods could be called
  // in-band (i.e., inside URLLoaderThrottle notification methods such as
  // WillStartRequest), or out-of-band.
  //
  // It is guaranteed that throttles calling these methods won't be destroyed
  // synchronously.
  class CONTENT_EXPORT Delegate {
   public:
    // Cancels the resource load with the specified error code and an optional,
    // application-defined reason description.
    virtual void CancelWithError(int error_code,
                                 base::StringPiece custom_reason = nullptr) = 0;

    // Resumes the deferred resource load. It is a no-op if the resource load is
    // not deferred or has already been canceled.
    virtual void Resume() = 0;

    virtual void SetPriority(net::RequestPriority priority);

    // Pauses/resumes reading response body if the resource is fetched from
    // network.
    virtual void PauseReadingBodyFromNet();
    virtual void ResumeReadingBodyFromNet();

    // Replaces the URLLoader and URLLoaderClient endpoints held by the
    // ThrottlingURLLoader instance.
    virtual void InterceptResponse(
        network::mojom::URLLoaderPtr new_loader,
        network::mojom::URLLoaderClientRequest new_client_request,
        network::mojom::URLLoaderPtr* original_loader,
        network::mojom::URLLoaderClientRequest* original_client_request);

   protected:
    virtual ~Delegate();
  };

  virtual ~URLLoaderThrottle();

  // Detaches this object from the current sequence in preparation for a move to
  // a different sequence. If this method is called it must be before any of the
  // Will* methods below and may only be called once.
  virtual void DetachFromCurrentSequence();

  // Called before the resource request is started.
  virtual void WillStartRequest(network::ResourceRequest* request, bool* defer);

  // Called when the request was redirected.  |redirect_info| contains the
  // redirect responses's HTTP status code and some information about the new
  // request that will be sent if the redirect is followed, including the new
  // URL and new method.
  virtual void WillRedirectRequest(
      const net::RedirectInfo& redirect_info,
      const network::ResourceResponseHead& response_head,
      bool* defer);

  // Called when the response headers and meta data are available.
  // TODO(776312): Migrate this URL to ResourceResponseHead.
  virtual void WillProcessResponse(
      const GURL& response_url,
      const network::ResourceResponseHead& response_head,
      bool* defer);

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

 protected:
  URLLoaderThrottle();

  Delegate* delegate_ = nullptr;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_URL_LOADER_THROTTLE_H_
