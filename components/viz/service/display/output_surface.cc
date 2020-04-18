// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/output_surface.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/vulkan/buildflags.h"
#include "ui/gfx/swap_result.h"

namespace viz {

OutputSurface::OutputSurface(scoped_refptr<ContextProvider> context_provider)
    : context_provider_(std::move(context_provider)) {
  DCHECK(context_provider_);
}

OutputSurface::OutputSurface(
    std::unique_ptr<SoftwareOutputDevice> software_device)
    : software_device_(std::move(software_device)) {
  DCHECK(software_device_);
}

#if BUILDFLAG(ENABLE_VULKAN)
OutputSurface::OutputSurface(
    scoped_refptr<VulkanContextProvider> vulkan_context_provider)
    : vulkan_context_provider_(std::move(vulkan_context_provider)) {
  DCHECK(vulkan_context_provider_);
}
#endif

OutputSurface::~OutputSurface() = default;

void OutputSurface::UpdateLatencyInfoOnSwap(
    const gfx::SwapResponse& response,
    std::vector<ui::LatencyInfo>* latency_info) {
  for (auto& latency : *latency_info) {
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, response.swap_start, 1);
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0,
        response.swap_end, 1);
  }
}

bool OutputSurface::LatencyInfoHasSnapshotRequest(
    const std::vector<ui::LatencyInfo>& latency_info) {
  for (const auto& latency : latency_info) {
    if (latency.Snapshots().size())
      return true;
  }
  return false;
}

}  // namespace viz
