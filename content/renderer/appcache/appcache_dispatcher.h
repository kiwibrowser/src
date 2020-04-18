// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_APPCACHE_APPCACHE_DISPATCHER_H_
#define CONTENT_RENDERER_APPCACHE_APPCACHE_DISPATCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "content/common/appcache.mojom.h"
#include "content/common/appcache_interfaces.h"
#include "content/renderer/appcache/appcache_backend_proxy.h"
#include "ipc/ipc_listener.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// Dispatches appcache related messages sent to a child process from the
// main browser process. There is one instance per child process. Messages
// are dispatched on the main child thread. The ChildThread base class
// creates an instance and delegates calls to it.
class AppCacheDispatcher : public mojom::AppCacheFrontend {
 public:
  explicit AppCacheDispatcher(content::AppCacheFrontend* frontend);
  ~AppCacheDispatcher() override;

  void Bind(mojom::AppCacheFrontendRequest request);

  AppCacheBackendProxy* backend_proxy() { return &backend_proxy_; }


 private:
  // mojom::AppCacheFrontend
  void CacheSelected(int32_t host_id, mojom::AppCacheInfoPtr info) override;
  void StatusChanged(const std::vector<int32_t>& host_ids,
                     AppCacheStatus status) override;
  void EventRaised(const std::vector<int32_t>& host_ids,
                   AppCacheEventID event_id) override;
  void ProgressEventRaised(const std::vector<int32_t>& host_ids,
                           const GURL& url,
                           int32_t num_total,
                           int32_t num_complete) override;
  void ErrorEventRaised(const std::vector<int32_t>& host_ids,
                        mojom::AppCacheErrorDetailsPtr details) override;
  void LogMessage(int32_t host_id,
                  int32_t log_level,
                  const std::string& message) override;
  void ContentBlocked(int32_t host_id, const GURL& manifest_url) override;
  void SetSubresourceFactory(
      int32_t host_id,
      network::mojom::URLLoaderFactoryPtr url_loader_factory) override;

  AppCacheBackendProxy backend_proxy_;
  std::unique_ptr<content::AppCacheFrontend> frontend_;
  mojo::Binding<mojom::AppCacheFrontend> binding_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_APPCACHE_APPCACHE_DISPATCHER_H_
