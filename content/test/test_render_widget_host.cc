// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_render_widget_host.h"

#include "base/run_loop.h"
#include "content/public/common/content_features.h"

namespace content {

std::unique_ptr<RenderWidgetHostImpl> TestRenderWidgetHost::Create(
    RenderWidgetHostDelegate* delegate,
    RenderProcessHost* process,
    int32_t routing_id,
    bool hidden) {
  mojom::WidgetPtr widget;
  std::unique_ptr<MockWidgetImpl> widget_impl =
      std::make_unique<MockWidgetImpl>(mojo::MakeRequest(&widget));
  return base::WrapUnique(new TestRenderWidgetHost(
      delegate, process, routing_id, std::move(widget_impl), std::move(widget),
      hidden));
}

TestRenderWidgetHost::TestRenderWidgetHost(
    RenderWidgetHostDelegate* delegate,
    RenderProcessHost* process,
    int32_t routing_id,
    std::unique_ptr<MockWidgetImpl> widget_impl,
    mojom::WidgetPtr widget,
    bool hidden)
    : RenderWidgetHostImpl(delegate,
                           process,
                           routing_id,
                           std::move(widget),
                           hidden),
      widget_impl_(std::move(widget_impl)) {}

TestRenderWidgetHost::~TestRenderWidgetHost() {}
mojom::WidgetInputHandler* TestRenderWidgetHost::GetWidgetInputHandler() {
  return widget_impl_->input_handler();
}

MockWidgetInputHandler* TestRenderWidgetHost::GetMockWidgetInputHandler() {
  return widget_impl_->input_handler();
}

}  // namespace content
