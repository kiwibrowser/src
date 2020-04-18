// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/gpu_display_provider.h"

#include <utility>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/base/switches.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/display/display_scheduler.h"
#include "components/viz/service/display_embedder/external_begin_frame_controller_impl.h"
#include "components/viz/service/display_embedder/gl_output_surface.h"
#include "components/viz/service/display_embedder/in_process_gpu_memory_buffer_manager.h"
#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "components/viz/service/display_embedder/skia_output_surface_impl.h"
#include "components/viz/service/display_embedder/software_output_surface.h"
#include "components/viz/service/display_embedder/viz_process_context_provider.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "gpu/ipc/service/gpu_channel_manager_delegate.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory.h"
#include "ui/base/ui_base_switches.h"

#if defined(OS_WIN)
#include "components/viz/service/display_embedder/gl_output_surface_win.h"
#include "components/viz/service/display_embedder/software_output_device_win.h"
#endif

#if defined(OS_MACOSX)
#include "components/viz/service/display_embedder/gl_output_surface_mac.h"
#include "components/viz/service/display_embedder/software_output_device_mac.h"
#include "ui/base/cocoa/remote_layer_api.h"
#endif

#if defined(USE_X11)
#include "components/viz/service/display_embedder/software_output_device_x11.h"
#endif

#if defined(USE_OZONE)
#include "components/viz/service/display_embedder/gl_output_surface_ozone.h"
#include "components/viz/service/display_embedder/software_output_device_ozone.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/ozone/public/surface_ozone_canvas.h"
#endif

namespace {

gpu::ImageFactory* GetImageFactory(gpu::GpuChannelManager* channel_manager) {
  auto* buffer_factory = channel_manager->gpu_memory_buffer_factory();
  return buffer_factory ? buffer_factory->AsImageFactory() : nullptr;
}

}  // namespace

namespace viz {

GpuDisplayProvider::GpuDisplayProvider(
    uint32_t restart_id,
    GpuServiceImpl* gpu_service_impl,
    scoped_refptr<gpu::InProcessCommandBuffer::Service> gpu_service,
    gpu::GpuChannelManager* gpu_channel_manager,
    bool headless,
    bool wait_for_all_pipeline_stages_before_draw)
    : restart_id_(restart_id),
      gpu_service_impl_(gpu_service_impl),
      gpu_service_(std::move(gpu_service)),
      gpu_channel_manager_delegate_(gpu_channel_manager->delegate()),
      gpu_memory_buffer_manager_(
          std::make_unique<InProcessGpuMemoryBufferManager>(
              gpu_channel_manager)),
      image_factory_(GetImageFactory(gpu_channel_manager)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      headless_(headless),
      wait_for_all_pipeline_stages_before_draw_(
          wait_for_all_pipeline_stages_before_draw) {
  DCHECK_NE(restart_id_, BeginFrameSource::kNotRestartableId);
}

GpuDisplayProvider::~GpuDisplayProvider() = default;

std::unique_ptr<Display> GpuDisplayProvider::CreateDisplay(
    const FrameSinkId& frame_sink_id,
    gpu::SurfaceHandle surface_handle,
    bool gpu_compositing,
    mojom::DisplayClient* display_client,
    ExternalBeginFrameControllerImpl* external_begin_frame_controller,
    const RendererSettings& renderer_settings,
    std::unique_ptr<SyntheticBeginFrameSource>* out_begin_frame_source) {
  BeginFrameSource* display_begin_frame_source = nullptr;
  std::unique_ptr<DelayBasedBeginFrameSource> synthetic_begin_frame_source;
  if (external_begin_frame_controller) {
    display_begin_frame_source =
        external_begin_frame_controller->begin_frame_source();
  } else {
    synthetic_begin_frame_source = std::make_unique<DelayBasedBeginFrameSource>(
        std::make_unique<DelayBasedTimeSource>(task_runner_.get()),
        restart_id_);
    display_begin_frame_source = synthetic_begin_frame_source.get();
  }

  // TODO(penghuang): Merge two output surfaces into one when GLRenderer and
  // software compositor is removed.
  std::unique_ptr<OutputSurface> output_surface;
  SkiaOutputSurface* skia_output_surface = nullptr;

  if (!gpu_compositing) {
    output_surface = std::make_unique<SoftwareOutputSurface>(
        CreateSoftwareOutputDeviceForPlatform(surface_handle, display_client));
  } else if (renderer_settings.use_skia_renderer &&
             renderer_settings.use_skia_deferred_display_list) {
#if defined(OS_MACOSX) || defined(OS_WIN)
    // TODO(penghuang): Support DDL for all platforms.
    NOTIMPLEMENTED();
    // Workaround compile error: private field 'gpu_service_impl_' is not used.
    ALLOW_UNUSED_LOCAL(gpu_service_impl_);
#else
    // Create an offscreen context_provider for SkiaOutputSurfaceImpl, because
    // SkiaRenderer still needs it to draw RenderPass into a texture.
    // TODO(penghuang): Remove this context_provider. https://crbug.com/825901
    auto context_provider = base::MakeRefCounted<VizProcessContextProvider>(
        gpu_service_, gpu::kNullSurfaceHandle, gpu_memory_buffer_manager_.get(),
        image_factory_, gpu_channel_manager_delegate_,
        gpu::SharedMemoryLimits());
    auto result = context_provider->BindToCurrentThread();
    CHECK_EQ(result, gpu::ContextResult::kSuccess);
    output_surface = std::make_unique<SkiaOutputSurfaceImpl>(
        gpu_service_impl_, surface_handle, std::move(context_provider),
        synthetic_begin_frame_source.get());
    skia_output_surface = static_cast<SkiaOutputSurface*>(output_surface.get());
#endif
  } else {
    scoped_refptr<VizProcessContextProvider> context_provider;

    // Retry creating and binding |context_provider| on transient failures.
    gpu::ContextResult context_result = gpu::ContextResult::kTransientFailure;
    while (context_result != gpu::ContextResult::kSuccess) {
      context_provider = base::MakeRefCounted<VizProcessContextProvider>(
          gpu_service_, surface_handle, gpu_memory_buffer_manager_.get(),
          image_factory_, gpu_channel_manager_delegate_,
          gpu::SharedMemoryLimits());
      context_result = context_provider->BindToCurrentThread();

      // TODO(crbug.com/819474): Don't crash here, instead fallback to software
      // compositing for fatal failures.
      CHECK_NE(context_result, gpu::ContextResult::kFatalFailure);
    }

    if (context_provider->ContextCapabilities().surfaceless) {
#if defined(USE_OZONE)
      output_surface = std::make_unique<GLOutputSurfaceOzone>(
          std::move(context_provider), surface_handle,
          synthetic_begin_frame_source.get(), gpu_memory_buffer_manager_.get(),
          GL_TEXTURE_2D, GL_BGRA_EXT);
#elif defined(OS_MACOSX)
      output_surface = std::make_unique<GLOutputSurfaceMac>(
          std::move(context_provider), surface_handle,
          synthetic_begin_frame_source.get(), gpu_memory_buffer_manager_.get(),
          renderer_settings.allow_overlays);
#else
      NOTREACHED();
#endif
    } else {
#if defined(OS_WIN)
      const auto& capabilities = context_provider->ContextCapabilities();
      const bool use_overlays =
          capabilities.dc_layers && capabilities.use_dc_overlays_for_video;
      output_surface = std::make_unique<GLOutputSurfaceWin>(
          std::move(context_provider), synthetic_begin_frame_source.get(),
          use_overlays);
#else
      output_surface = std::make_unique<GLOutputSurface>(
          std::move(context_provider), synthetic_begin_frame_source.get());
#endif
    }
  }

  int max_frames_pending = output_surface->capabilities().max_frames_pending;
  DCHECK_GT(max_frames_pending, 0);

  auto scheduler = std::make_unique<DisplayScheduler>(
      display_begin_frame_source, task_runner_.get(), max_frames_pending,
      wait_for_all_pipeline_stages_before_draw_);

  // The ownership of the BeginFrameSource is transferred to the caller.
  *out_begin_frame_source = std::move(synthetic_begin_frame_source);

  return std::make_unique<Display>(
      ServerSharedBitmapManager::current(), renderer_settings, frame_sink_id,
      std::move(output_surface), std::move(scheduler), task_runner_,
      skia_output_surface);
}

std::unique_ptr<SoftwareOutputDevice>
GpuDisplayProvider::CreateSoftwareOutputDeviceForPlatform(
    gpu::SurfaceHandle surface_handle,
    mojom::DisplayClient* display_client) {
  if (headless_)
    return std::make_unique<SoftwareOutputDevice>();

#if defined(OS_WIN)
  return CreateSoftwareOutputDeviceWinGpu(
      surface_handle, &output_device_backing_, display_client);
#elif defined(OS_MACOSX)
  return std::make_unique<SoftwareOutputDeviceMac>(task_runner_);
#elif defined(OS_ANDROID)
  // Android does not do software compositing, so we can't get here.
  NOTREACHED();
  return nullptr;
#elif defined(USE_OZONE)
  ui::SurfaceFactoryOzone* factory =
      ui::OzonePlatform::GetInstance()->GetSurfaceFactoryOzone();
  std::unique_ptr<ui::SurfaceOzoneCanvas> surface_ozone =
      factory->CreateCanvasForWidget(surface_handle);
  CHECK(surface_ozone);
  return std::make_unique<SoftwareOutputDeviceOzone>(std::move(surface_ozone));
#elif defined(USE_X11)
  return std::make_unique<SoftwareOutputDeviceX11>(surface_handle);
#endif
}

}  // namespace viz
