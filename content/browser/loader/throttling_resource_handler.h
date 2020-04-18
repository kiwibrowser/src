// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_THROTTLING_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_THROTTLING_RESOURCE_HANDLER_H_

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/common/content_export.h"
#include "content/public/browser/resource_throttle.h"
#include "net/url_request/redirect_info.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace network {
struct ResourceResponse;
}

namespace content {

// Used to apply a list of ResourceThrottle instances to an URLRequest.
class CONTENT_EXPORT ThrottlingResourceHandler
    : public LayeredResourceHandler,
      public ResourceThrottle::Delegate {
 public:
  ThrottlingResourceHandler(
      std::unique_ptr<ResourceHandler> next_handler,
      net::URLRequest* request,
      std::vector<std::unique_ptr<ResourceThrottle>> throttles);
  ~ThrottlingResourceHandler() override;

  // LayeredResourceHandler overrides:
  void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillStart(const GURL& url,
                   std::unique_ptr<ResourceController> controller) override;

  // ResourceThrottle::Delegate implementation:
  void Cancel() override;
  void CancelWithError(int error_code) override;
  void Resume() override;

 private:
  void ResumeStart();
  void ResumeRedirect();
  void ResumeResponse();

  // Called when the throttle at |throttle_index| defers a request.  Logs the
  // name of the throttle that delayed the request.
  void OnRequestDeferred(int throttle_index);

  enum DeferredStage {
    DEFERRED_NONE,
    DEFERRED_START,
    DEFERRED_REDIRECT,
    DEFERRED_RESPONSE
  };
  DeferredStage deferred_stage_;

  std::vector<std::unique_ptr<ResourceThrottle>> throttles_;
  size_t next_index_;

  GURL deferred_url_;
  net::RedirectInfo deferred_redirect_;
  scoped_refptr<network::ResourceResponse> deferred_response_;

  bool cancelled_by_resource_throttle_;

  DISALLOW_COPY_AND_ASSIGN(ThrottlingResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_THROTTLING_RESOURCE_HANDLER_H_
