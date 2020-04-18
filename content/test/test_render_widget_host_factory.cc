// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_render_widget_host_factory.h"

#include "content/test/test_render_widget_host.h"

namespace content {

TestRenderWidgetHostFactory::TestRenderWidgetHostFactory() {
  RenderWidgetHostFactory::RegisterFactory(this);
}

TestRenderWidgetHostFactory::~TestRenderWidgetHostFactory() {
  RenderWidgetHostFactory::UnregisterFactory();
}

RenderWidgetHostImpl* TestRenderWidgetHostFactory::CreateRenderWidgetHost(
    RenderWidgetHostDelegate* delegate,
    RenderProcessHost* process,
    int32_t routing_id,
    mojom::WidgetPtr widget_interface,
    bool hidden) {
  return TestRenderWidgetHost::Create(delegate, process, routing_id, hidden)
      .release();
}

}  // namespace content
