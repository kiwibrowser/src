// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/in_process_display_client.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"

#if defined(OS_MACOSX)
#include "ui/accelerated_widget_mac/ca_layer_frame_sink.h"
#endif

#if defined(OS_WIN)
#include <windows.h>

#include "components/viz/common/display/use_layered_window.h"
#include "components/viz/host/layered_window_updater_impl.h"
#include "ui/base/win/internal_constants.h"
#endif

namespace content {

InProcessDisplayClient::InProcessDisplayClient(gfx::AcceleratedWidget widget)
    : binding_(this) {
#if defined(OS_MACOSX) || defined(OS_WIN)
  widget_ = widget;
#endif
}

InProcessDisplayClient::~InProcessDisplayClient() {}

viz::mojom::DisplayClientPtr InProcessDisplayClient::GetBoundPtr(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  viz::mojom::DisplayClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr), task_runner);
  return ptr;
}

void InProcessDisplayClient::OnDisplayReceivedCALayerParams(
    const gfx::CALayerParams& ca_layer_params) {
#if defined(OS_MACOSX)
  ui::CALayerFrameSink* ca_layer_frame_sink =
      ui::CALayerFrameSink::FromAcceleratedWidget(widget_);
  if (ca_layer_frame_sink)
    ca_layer_frame_sink->UpdateCALayerTree(ca_layer_params);
  else
    DLOG(WARNING) << "Received frame for non-existent widget.";
#else
  DLOG(ERROR) << "Should not receive CALayer params on non-macOS platforms.";
#endif
}

void InProcessDisplayClient::DidSwapAfterSnapshotRequestReceived(
    const std::vector<ui::LatencyInfo>& latency_info) {
  RenderWidgetHostImpl::OnGpuSwapBuffersCompleted(latency_info);
}

void InProcessDisplayClient::CreateLayeredWindowUpdater(
    viz::mojom::LayeredWindowUpdaterRequest request) {
#if defined(OS_WIN)
  if (!viz::NeedsToUseLayerWindow(widget_)) {
    DLOG(ERROR) << "HWND shouldn't be using a layered window";
    return;
  }

  layered_window_updater_ = std::make_unique<viz::LayeredWindowUpdaterImpl>(
      widget_, std::move(request));
#else
// This should never happen on non-Windows platforms.
#endif
}

}  // namespace content
