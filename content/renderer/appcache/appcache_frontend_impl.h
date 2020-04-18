// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_APPCACHE_APPCACHE_FRONTEND_IMPL_H_
#define CONTENT_RENDERER_APPCACHE_APPCACHE_FRONTEND_IMPL_H_

#include "content/common/appcache_interfaces.h"

namespace content {

class AppCacheFrontendImpl : public AppCacheFrontend {
 public:
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
};

}  // namespace content

#endif  // CONTENT_RENDERER_APPCACHE_APPCACHE_FRONTEND_IMPL_H_
