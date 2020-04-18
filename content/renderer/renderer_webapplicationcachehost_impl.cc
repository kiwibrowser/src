// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/renderer_webapplicationcachehost_impl.h"

#include "content/common/view_messages.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_view_impl.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"

using blink::WebApplicationCacheHostClient;
using blink::WebConsoleMessage;

namespace content {

RendererWebApplicationCacheHostImpl::RendererWebApplicationCacheHostImpl(
    RenderViewImpl* render_view,
    WebApplicationCacheHostClient* client,
    AppCacheBackend* backend,
    int appcache_host_id,
    int frame_routing_id)
    : WebApplicationCacheHostImpl(client, backend, appcache_host_id),
      routing_id_(render_view->GetRoutingID()),
      frame_routing_id_(frame_routing_id) {}

void RendererWebApplicationCacheHostImpl::OnLogMessage(
    AppCacheLogLevel log_level, const std::string& message) {
  if (RenderThreadImpl::current()->layout_test_mode())
    return;

  RenderViewImpl* render_view = GetRenderView();
  if (!render_view || !render_view->webview() ||
      !render_view->webview()->MainFrame())
    return;

  blink::WebFrame* frame = render_view->webview()->MainFrame();
  if (!frame->IsWebLocalFrame())
    return;
  // TODO(michaeln): Make app cache host per-frame and correctly report to the
  // involved frame.
  frame->ToWebLocalFrame()->AddMessageToConsole(
      WebConsoleMessage(static_cast<WebConsoleMessage::Level>(log_level),
                        blink::WebString::FromUTF8(message.c_str())));
}

void RendererWebApplicationCacheHostImpl::OnContentBlocked(
    const GURL& manifest_url) {
  RenderThreadImpl::current()->Send(new ViewHostMsg_AppCacheAccessed(
      routing_id_, manifest_url, true));
}

void RendererWebApplicationCacheHostImpl::OnCacheSelected(
    const AppCacheInfo& info) {
  if (!info.manifest_url.is_empty()) {
    RenderThreadImpl::current()->Send(new ViewHostMsg_AppCacheAccessed(
        routing_id_, info.manifest_url, false));
  }
  WebApplicationCacheHostImpl::OnCacheSelected(info);
}

void RendererWebApplicationCacheHostImpl::SetSubresourceFactory(
    network::mojom::URLLoaderFactoryPtr url_loader_factory) {
  RenderFrameImpl* render_frame =
      RenderFrameImpl::FromRoutingID(frame_routing_id_);
  if (render_frame) {
    render_frame->SetCustomURLLoaderFactory(std::move(url_loader_factory));
  }
}

RenderViewImpl* RendererWebApplicationCacheHostImpl::GetRenderView() {
  return RenderViewImpl::FromRoutingID(routing_id_);
}

}  // namespace content
