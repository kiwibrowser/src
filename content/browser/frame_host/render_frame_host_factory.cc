// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/render_frame_host_factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"

namespace content {

// static
RenderFrameHostFactory* RenderFrameHostFactory::factory_ = nullptr;

// static
std::unique_ptr<RenderFrameHostImpl> RenderFrameHostFactory::Create(
    SiteInstance* site_instance,
    RenderViewHostImpl* render_view_host,
    RenderFrameHostDelegate* delegate,
    RenderWidgetHostDelegate* rwh_delegate,
    FrameTree* frame_tree,
    FrameTreeNode* frame_tree_node,
    int32_t routing_id,
    int32_t widget_routing_id,
    bool hidden,
    bool renderer_initiated_creation) {
  if (factory_) {
    return factory_->CreateRenderFrameHost(
        site_instance, render_view_host, delegate, rwh_delegate, frame_tree,
        frame_tree_node, routing_id, widget_routing_id, hidden,
        renderer_initiated_creation);
  }
  return base::WrapUnique(new RenderFrameHostImpl(
      site_instance, render_view_host, delegate, rwh_delegate, frame_tree,
      frame_tree_node, routing_id, widget_routing_id, hidden,
      renderer_initiated_creation));
}

// static
void RenderFrameHostFactory::RegisterFactory(RenderFrameHostFactory* factory) {
  DCHECK(!factory_) << "Can't register two factories at once.";
  factory_ = factory;
}

// static
void RenderFrameHostFactory::UnregisterFactory() {
  DCHECK(factory_) << "No factory to unregister.";
  factory_ = nullptr;
}

}  // namespace content
