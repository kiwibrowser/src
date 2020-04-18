// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_

#include <memory>

#include "content/browser/loader/resource_handler.h"
#include "content/common/content_export.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

class ResourceController;

// A ResourceHandler that simply delegates all calls to a next handler.  This
// class is intended to be subclassed.
class CONTENT_EXPORT LayeredResourceHandler : public ResourceHandler {
 public:
  LayeredResourceHandler(net::URLRequest* request,
                         std::unique_ptr<ResourceHandler> next_handler);
  ~LayeredResourceHandler() override;

  // ResourceHandler implementation:
  void SetDelegate(Delegate* delegate) override;
  void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillStart(const GURL& url,
                   std::unique_ptr<ResourceController> controller) override;
  void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                  int* buf_size,
                  std::unique_ptr<ResourceController> controller) override;
  void OnReadCompleted(int bytes_read,
                       std::unique_ptr<ResourceController> controller) override;
  void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) override;
  void OnDataDownloaded(int bytes_downloaded) override;

  std::unique_ptr<ResourceHandler> next_handler_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_
