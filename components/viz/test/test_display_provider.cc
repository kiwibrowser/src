// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/test_display_provider.h"

#include "base/threading/thread_task_runner_handle.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display_scheduler.h"
#include "components/viz/service/display/software_output_device.h"
#include "components/viz/test/fake_output_surface.h"

namespace viz {

TestDisplayProvider::TestDisplayProvider() = default;

TestDisplayProvider::~TestDisplayProvider() = default;

std::unique_ptr<Display> TestDisplayProvider::CreateDisplay(
    const FrameSinkId& frame_sink_id,
    gpu::SurfaceHandle surface_handle,
    bool gpu_compositing,
    mojom::DisplayClient* display_client,
    ExternalBeginFrameControllerImpl* external_begin_frame_controller,
    const RendererSettings& renderer_settings,
    std::unique_ptr<SyntheticBeginFrameSource>* out_begin_frame_source) {
  auto task_runner = base::ThreadTaskRunnerHandle::Get();
  DCHECK(task_runner);

  // This could use a fake time source if tests wanted to send begin frames.
  std::unique_ptr<SyntheticBeginFrameSource> begin_frame_source =
      std::make_unique<DelayBasedBeginFrameSource>(
          std::make_unique<DelayBasedTimeSource>(task_runner.get()),
          BeginFrameSource::kNotRestartableId);

  std::unique_ptr<OutputSurface> output_surface;
  if (gpu_compositing) {
    output_surface = FakeOutputSurface::Create3d();
  } else {
    output_surface = FakeOutputSurface::CreateSoftware(
        std::make_unique<SoftwareOutputDevice>());
  }

  auto scheduler = std::make_unique<DisplayScheduler>(
      begin_frame_source.get(), task_runner.get(),
      output_surface->capabilities().max_frames_pending);

  // Give ownership of |begin_frame_source| to caller.
  *out_begin_frame_source = std::move(begin_frame_source);

  return std::make_unique<Display>(&shared_bitmap_manager_, renderer_settings,
                                   frame_sink_id, std::move(output_surface),
                                   std::move(scheduler), task_runner);
}

}  // namespace viz
