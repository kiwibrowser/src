// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/fling_scheduler_mac.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "ui/compositor/compositor.h"

namespace content {

FlingSchedulerMac::FlingSchedulerMac(RenderWidgetHostImpl* host)
    : FlingScheduler(host) {}
FlingSchedulerMac::~FlingSchedulerMac() = default;

ui::Compositor* FlingSchedulerMac::GetCompositor() {
  if (!host_->GetView())
    return nullptr;

  // RWHV_child_frame doesn't have DelegatedFrameHost with ui::Compositor.
  if (host_->GetView()->IsRenderWidgetHostViewChildFrame())
    return nullptr;

  // TODO(sahel): Uncomment this once Viz is ready on Mac.
  // https://crbug.com/833985
  /* RenderWidgetHostViewMac* view =
      static_cast<RenderWidgetHostViewMac*>(host_->GetView());
  if (view->BrowserCompositor())
    return view->BrowserCompositor()->Compositor();
  } */

  return nullptr;
}

}  // namespace content
