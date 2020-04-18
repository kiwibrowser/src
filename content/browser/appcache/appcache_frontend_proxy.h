// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_FRONTEND_PROXY_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_FRONTEND_PROXY_H_

#include <string>
#include <vector>

#include "content/common/appcache.mojom.h"
#include "content/common/appcache_interfaces.h"

namespace content {

// Sends appcache related messages to a child process.
class AppCacheFrontendProxy : public AppCacheFrontend {
 public:
  explicit AppCacheFrontendProxy(int process_id);
  ~AppCacheFrontendProxy() override;

  // AppCacheFrontend methods
  void OnCacheSelected(int host_id, const AppCacheInfo& info) override;
  void OnStatusChanged(const std::vector<int>& host_ids,
                       AppCacheStatus status) override;
  void OnEventRaised(const std::vector<int>& host_ids,
                     AppCacheEventID event_id) override;
  void OnProgressEventRaised(const std::vector<int>& host_ids,
                             const GURL& url,
                             int num_total,
                             int num_complete) override;
  void OnErrorEventRaised(const std::vector<int>& host_ids,
                          const AppCacheErrorDetails& details) override;
  void OnLogMessage(int host_id,
                    AppCacheLogLevel log_level,
                    const std::string& message) override;
  void OnContentBlocked(int host_id, const GURL& manifest_url) override;
  void OnSetSubresourceFactory(
      int host_id,
      network::mojom::URLLoaderFactoryPtr url_loader_factory) override;

 private:
  mojom::AppCacheFrontend* GetAppCacheFrontend();

  const int process_id_;
  mojom::AppCacheFrontendPtr app_cache_renderer_ptr_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_FRONTEND_PROXY_H_
