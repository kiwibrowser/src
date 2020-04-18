// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDERER_WEBAPPLICATIONCACHEHOST_IMPL_H_
#define CONTENT_RENDERER_RENDERER_WEBAPPLICATIONCACHEHOST_IMPL_H_

#include "content/renderer/appcache/web_application_cache_host_impl.h"

namespace content {
class RenderViewImpl;

class RendererWebApplicationCacheHostImpl : public WebApplicationCacheHostImpl {
 public:
  RendererWebApplicationCacheHostImpl(
      RenderViewImpl* render_view,
      blink::WebApplicationCacheHostClient* client,
      AppCacheBackend* backend,
      int appcache_host_id,
      int frame_routing_id);

  // WebApplicationCacheHostImpl:
  void OnLogMessage(AppCacheLogLevel log_level,
                    const std::string& message) override;
  void OnContentBlocked(const GURL& manifest_url) override;
  void OnCacheSelected(const AppCacheInfo& info) override;

  void SetSubresourceFactory(
      network::mojom::URLLoaderFactoryPtr url_loader_factory) override;

 private:
  RenderViewImpl* GetRenderView();

  int routing_id_;
  int frame_routing_id_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDERER_WEBAPPLICATIONCACHEHOST_IMPL_H_
