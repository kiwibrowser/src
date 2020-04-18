// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/viz_process_transport_factory.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/debug/dump_without_crashing.h"
#include "base/single_thread_task_runner.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "components/viz/client/client_layer_tree_frame_sink.h"
#include "components/viz/client/hit_test_data_provider_draw_quad.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/gpu/raster_context_provider.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "components/viz/service/display_embedder/compositing_mode_reporter_impl.h"
#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/compositor/external_begin_frame_controller_client_impl.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/gpu_stream_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "ui/base/ui_base_features.h"
#include "ui/compositor/reflector.h"

#if defined(OS_WIN)
#include "ui/gfx/win/rendering_window_manager.h"
#endif

namespace content {
namespace {

// The client id for the browser process. It must not conflict with any
// child process client id.
constexpr uint32_t kBrowserClientId = 0u;

scoped_refptr<ui::ContextProviderCommandBuffer> CreateContextProviderImpl(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    bool support_locking,
    bool support_gles2_interface,
    bool support_raster_interface,
    bool support_grcontext,
    ui::command_buffer_metrics::ContextType type) {
  constexpr bool kAutomaticFlushes = false;

  gpu::ContextCreationAttribs attributes;
  attributes.alpha_size = -1;
  attributes.depth_size = 0;
  attributes.stencil_size = 0;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.bind_generates_resource = false;
  attributes.lose_context_when_out_of_memory = true;
  attributes.buffer_preserved = false;
  attributes.enable_gles2_interface = support_gles2_interface;
  attributes.enable_raster_interface = support_raster_interface;

  GURL url("chrome://gpu/VizProcessTransportFactory::CreateContextProvider");
  return base::MakeRefCounted<ui::ContextProviderCommandBuffer>(
      std::move(gpu_channel_host), gpu_memory_buffer_manager,
      kGpuStreamIdDefault, kGpuStreamPriorityUI, gpu::kNullSurfaceHandle,
      std::move(url), kAutomaticFlushes, support_locking, support_grcontext,
      gpu::SharedMemoryLimits(), attributes, type);
}

bool IsContextLost(viz::ContextProvider* context_provider) {
  return context_provider->ContextGL()->GetGraphicsResetStatusKHR() !=
         GL_NO_ERROR;
}

bool IsWorkerContextLost(viz::RasterContextProvider* context_provider) {
  viz::RasterContextProvider::ScopedRasterContextLock lock(context_provider);
  return lock.RasterInterface()->GetGraphicsResetStatusKHR() != GL_NO_ERROR;
}

// Provided as a callback to crash the GPU process.
void ReceivedBadMessageFromGpuProcess() {
  GpuProcessHost::CallOnIO(
      GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED, false /* force_create */,
      base::BindRepeating([](GpuProcessHost* host) {
        // There should always be a GpuProcessHost instance, and GPU process,
        // for running the compositor thread. The exception is during shutdown
        // the GPU process won't be restarted and GpuProcessHost::Get() can
        // return null.
        if (host)
          host->ForceShutdown();

        LOG(ERROR) << "Bad message received, terminating gpu process.";
        base::debug::DumpWithoutCrashing();
      }));
}

}  // namespace

VizProcessTransportFactory::VizProcessTransportFactory(
    gpu::GpuChannelEstablishFactory* gpu_channel_establish_factory,
    scoped_refptr<base::SingleThreadTaskRunner> resize_task_runner,
    viz::CompositingModeReporterImpl* compositing_mode_reporter)
    : gpu_channel_establish_factory_(gpu_channel_establish_factory),
      resize_task_runner_(std::move(resize_task_runner)),
      compositing_mode_reporter_(compositing_mode_reporter),
      frame_sink_id_allocator_(kBrowserClientId),
      task_graph_runner_(std::make_unique<cc::SingleThreadTaskGraphRunner>()),
      renderer_settings_(viz::CreateRendererSettings()),
      weak_ptr_factory_(this) {
  DCHECK(gpu_channel_establish_factory_);
  task_graph_runner_->Start("CompositorTileWorker1",
                            base::SimpleThread::Options());
  GetHostFrameSinkManager()->SetConnectionLostCallback(
      base::BindRepeating(&VizProcessTransportFactory::OnGpuProcessLost,
                          weak_ptr_factory_.GetWeakPtr()));
  GetHostFrameSinkManager()->SetBadMessageReceivedFromGpuCallback(
      base::BindRepeating(&ReceivedBadMessageFromGpuProcess));

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableGpu) ||
      command_line->HasSwitch(switches::kDisableGpuCompositing)) {
    DisableGpuCompositing(nullptr);
  }
}

VizProcessTransportFactory::~VizProcessTransportFactory() {
  if (main_context_provider_)
    main_context_provider_->RemoveObserver(this);

  task_graph_runner_->Shutdown();
}

void VizProcessTransportFactory::ConnectHostFrameSinkManager() {
  viz::mojom::FrameSinkManagerPtr frame_sink_manager;
  viz::mojom::FrameSinkManagerRequest frame_sink_manager_request =
      mojo::MakeRequest(&frame_sink_manager);
  viz::mojom::FrameSinkManagerClientPtr frame_sink_manager_client;
  viz::mojom::FrameSinkManagerClientRequest frame_sink_manager_client_request =
      mojo::MakeRequest(&frame_sink_manager_client);

  // Setup HostFrameSinkManager with interface endpoints.
  GetHostFrameSinkManager()->BindAndSetManager(
      std::move(frame_sink_manager_client_request), resize_task_runner_,
      std::move(frame_sink_manager));

  // Hop to the IO thread, then send the other side of interface to viz process.
  auto connect_on_io_thread =
      [](viz::mojom::FrameSinkManagerRequest request,
         viz::mojom::FrameSinkManagerClientPtrInfo client) {
        // There should always be a GpuProcessHost instance, and GPU process,
        // for running the compositor thread. The exception is during shutdown
        // the GPU process won't be restarted and GpuProcessHost::Get() can
        // return null.
        auto* gpu_process_host = GpuProcessHost::Get();
        if (gpu_process_host) {
          gpu_process_host->ConnectFrameSinkManager(std::move(request),
                                                    std::move(client));
        }
      };
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(connect_on_io_thread,
                     std::move(frame_sink_manager_request),
                     frame_sink_manager_client.PassInterface()));
}

void VizProcessTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
#if defined(OS_WIN)
  gfx::RenderingWindowManager::GetInstance()->UnregisterParent(
      compositor->widget());
#endif

  if (is_gpu_compositing_disabled_ || compositor->force_software_compositor()) {
    OnEstablishedGpuChannel(compositor, nullptr);
    return;
  }
  gpu_channel_establish_factory_->EstablishGpuChannel(
      base::BindOnce(&VizProcessTransportFactory::OnEstablishedGpuChannel,
                     weak_ptr_factory_.GetWeakPtr(), compositor));
}

scoped_refptr<viz::ContextProvider>
VizProcessTransportFactory::SharedMainThreadContextProvider() {
  if (is_gpu_compositing_disabled_)
    return nullptr;

  if (!main_context_provider_) {
    auto context_result = gpu::ContextResult::kTransientFailure;
    while (context_result == gpu::ContextResult::kTransientFailure) {
      context_result = TryCreateContextsForGpuCompositing(
          gpu_channel_establish_factory_->EstablishGpuChannelSync());

      if (context_result == gpu::ContextResult::kFatalFailure)
        DisableGpuCompositing(nullptr);
    }
    // On kFatalFailure |main_context_provider_| will be null.
  }

  return main_context_provider_;
}

void VizProcessTransportFactory::RemoveCompositor(ui::Compositor* compositor) {
#if defined(OS_WIN)
  // TODO(crbug.com/791660): Make sure that GpuProcessHost::SetChildSurface()
  // doesn't crash the GPU process after parent is unregistered.
  gfx::RenderingWindowManager::GetInstance()->UnregisterParent(
      compositor->widget());
#endif

  compositor_data_map_.erase(compositor);
}

double VizProcessTransportFactory::GetRefreshRate() const {
  // TODO(kylechar): Delete this function from ContextFactoryPrivate.
  return 60.0;
}

gpu::GpuMemoryBufferManager*
VizProcessTransportFactory::GetGpuMemoryBufferManager() {
  return gpu_channel_establish_factory_->GetGpuMemoryBufferManager();
}

cc::TaskGraphRunner* VizProcessTransportFactory::GetTaskGraphRunner() {
  return task_graph_runner_.get();
}

void VizProcessTransportFactory::AddObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.AddObserver(observer);
}

void VizProcessTransportFactory::RemoveObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

std::unique_ptr<ui::Reflector> VizProcessTransportFactory::CreateReflector(
    ui::Compositor* source,
    ui::Layer* target) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTIMPLEMENTED();
  return nullptr;
}

void VizProcessTransportFactory::RemoveReflector(ui::Reflector* reflector) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTIMPLEMENTED();
}

viz::FrameSinkId VizProcessTransportFactory::AllocateFrameSinkId() {
  return frame_sink_id_allocator_.NextFrameSinkId();
}

viz::HostFrameSinkManager*
VizProcessTransportFactory::GetHostFrameSinkManager() {
  return BrowserMainLoop::GetInstance()->host_frame_sink_manager();
}

void VizProcessTransportFactory::SetDisplayVisible(ui::Compositor* compositor,
                                                   bool visible) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetDisplayVisible(visible);
}

void VizProcessTransportFactory::ResizeDisplay(ui::Compositor* compositor,
                                               const gfx::Size& size) {
  // Do nothing and resize when a CompositorFrame with a new size arrives.
}

void VizProcessTransportFactory::SetDisplayColorMatrix(
    ui::Compositor* compositor,
    const SkMatrix44& matrix) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetDisplayColorMatrix(gfx::Transform(matrix));
}

void VizProcessTransportFactory::SetDisplayColorSpace(
    ui::Compositor* compositor,
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& output_color_space) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetDisplayColorSpace(blending_color_space,
                                                     output_color_space);
}

void VizProcessTransportFactory::SetAuthoritativeVSyncInterval(
    ui::Compositor* compositor,
    base::TimeDelta interval) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetAuthoritativeVSyncInterval(interval);
}

void VizProcessTransportFactory::SetDisplayVSyncParameters(
    ui::Compositor* compositor,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetDisplayVSyncParameters(timebase, interval);
}

void VizProcessTransportFactory::IssueExternalBeginFrame(
    ui::Compositor* compositor,
    const viz::BeginFrameArgs& args) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;

  DCHECK(iter->second.external_begin_frame_controller_client);
  iter->second.external_begin_frame_controller_client->GetController()
      ->IssueExternalBeginFrame(args);
}

void VizProcessTransportFactory::SetOutputIsSecure(ui::Compositor* compositor,
                                                   bool secure) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end() || !iter->second.display_private)
    return;
  iter->second.display_private->SetOutputIsSecure(secure);
}

viz::FrameSinkManagerImpl* VizProcessTransportFactory::GetFrameSinkManager() {
  // When running with viz there is no FrameSinkManagerImpl in the browser
  // process. FrameSinkManagerImpl runs in the GPU process instead. Anything in
  // the browser process that relies FrameSinkManagerImpl or SurfaceManager
  // internal state needs to change. See https://crbug.com/787097 and
  // https://crbug.com/760181 for more context.
  NOTREACHED();
  return nullptr;
}

bool VizProcessTransportFactory::IsGpuCompositingDisabled() {
  return is_gpu_compositing_disabled_;
}

ui::ContextFactory* VizProcessTransportFactory::GetContextFactory() {
  return this;
}

ui::ContextFactoryPrivate*
VizProcessTransportFactory::GetContextFactoryPrivate() {
  return this;
}

viz::GLHelper* VizProcessTransportFactory::GetGLHelper() {
  NOTREACHED();  // Readback happens in the GPU process and this isn't used.
  return nullptr;
}

void VizProcessTransportFactory::OnContextLost() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&VizProcessTransportFactory::OnLostMainThreadSharedContext,
                     weak_ptr_factory_.GetWeakPtr()));
}

void VizProcessTransportFactory::DisableGpuCompositing(
    ui::Compositor* guilty_compositor) {
  DLOG(ERROR) << "Switching to software compositing.";

  // Change the result of IsGpuCompositingDisabled() before notifying anything.
  is_gpu_compositing_disabled_ = true;

  compositing_mode_reporter_->SetUsingSoftwareCompositing();

  // Consumers of the shared main thread context aren't CompositingModeWatchers,
  // so inform them about the compositing mode switch by acting like the context
  // was lost. This also destroys the contexts since they aren't created when
  // gpu compositing isn't being used.
  OnLostMainThreadSharedContext();

  // Drop our reference on the gpu contexts for the compositors.
  worker_context_provider_ = nullptr;
  if (main_context_provider_) {
    main_context_provider_->RemoveObserver(this);
    main_context_provider_ = nullptr;
  }

  // Here we remove the FrameSink from every compositor that needs to fall back
  // to software compositing.
  //
  // Releasing the FrameSink from the compositor will remove it from
  // |compositor_data_map_|, so we can't do that while iterating though the
  // collection.
  std::vector<ui::Compositor*> to_release;
  to_release.reserve(compositor_data_map_.size());
  for (auto& pair : compositor_data_map_) {
    ui::Compositor* compositor = pair.first;
    // The |guilty_compositor| is in the process of setting up its FrameSink
    // so removing it from |compositor_data_map_| would be both pointless and
    // the cause of a crash.
    // Compositors with force_software_compositor() do not follow the global
    // compositing mode, so they do not need to changed.
    if (compositor != guilty_compositor &&
        !compositor->force_software_compositor()) {
      to_release.push_back(compositor);
    }
  }
  for (ui::Compositor* compositor : to_release) {
    // Compositor expects to be not visible when releasing its FrameSink.
    bool visible = compositor->IsVisible();
    compositor->SetVisible(false);
    gfx::AcceleratedWidget widget = compositor->ReleaseAcceleratedWidget();
    compositor->SetAcceleratedWidget(widget);
    if (visible)
      compositor->SetVisible(true);
  }

  GpuDataManagerImpl::GetInstance()->NotifyGpuInfoUpdate();
}

void VizProcessTransportFactory::OnGpuProcessLost() {
  // Reconnect HostFrameSinkManager to new GPU process.
  ConnectHostFrameSinkManager();
}

void VizProcessTransportFactory::OnEstablishedGpuChannel(
    base::WeakPtr<ui::Compositor> compositor_weak_ptr,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host) {
  ui::Compositor* compositor = compositor_weak_ptr.get();
  if (!compositor)
    return;

  bool gpu_compositing =
      !is_gpu_compositing_disabled_ && !compositor->force_software_compositor();

  if (gpu_compositing) {
    auto context_result =
        TryCreateContextsForGpuCompositing(std::move(gpu_channel_host));
    if (context_result == gpu::ContextResult::kTransientFailure) {
      // Get a new GpuChannelHost and retry context creation.
      gpu_channel_establish_factory_->EstablishGpuChannel(
          base::BindOnce(&VizProcessTransportFactory::OnEstablishedGpuChannel,
                         weak_ptr_factory_.GetWeakPtr(), compositor_weak_ptr));
      return;
    } else if (context_result == gpu::ContextResult::kFatalFailure) {
      DisableGpuCompositing(compositor);
      gpu_compositing = false;
    }
  }

#if defined(OS_WIN)
  gfx::RenderingWindowManager::GetInstance()->RegisterParent(
      compositor->widget());
#endif

  auto& compositor_data = compositor_data_map_[compositor];

  auto root_params = viz::mojom::RootCompositorFrameSinkParams::New();

  // Create interfaces for a root CompositorFrameSink.
  viz::mojom::CompositorFrameSinkAssociatedPtrInfo sink_info;
  root_params->compositor_frame_sink = mojo::MakeRequest(&sink_info);
  viz::mojom::CompositorFrameSinkClientRequest client_request =
      mojo::MakeRequest(&root_params->compositor_frame_sink_client);
  root_params->display_private =
      mojo::MakeRequest(&compositor_data.display_private);
  compositor_data.display_client =
      std::make_unique<InProcessDisplayClient>(compositor->widget());
  root_params->display_client =
      compositor_data.display_client->GetBoundPtr(resize_task_runner_)
          .PassInterface();

#if defined(GPU_SURFACE_HANDLE_IS_ACCELERATED_WINDOW)
  gpu::SurfaceHandle surface_handle = compositor->widget();
#else
  // TODO(kylechar): Fix this when we support macOS.
  gpu::SurfaceHandle surface_handle = gpu::kNullSurfaceHandle;
#endif

  // Initialize ExternalBeginFrameController client if enabled.
  compositor_data.external_begin_frame_controller_client.reset();
  if (compositor->external_begin_frames_enabled()) {
    compositor_data.external_begin_frame_controller_client =
        std::make_unique<ExternalBeginFrameControllerClientImpl>(compositor);
    root_params->external_begin_frame_controller =
        compositor_data.external_begin_frame_controller_client
            ->GetControllerRequest();
    root_params->external_begin_frame_controller_client =
        compositor_data.external_begin_frame_controller_client->GetBoundPtr()
            .PassInterface();
  }

  root_params->frame_sink_id = compositor->frame_sink_id();
  root_params->widget = surface_handle;
  root_params->gpu_compositing = gpu_compositing;
  root_params->renderer_settings = renderer_settings_;

  // Connects the viz process end of CompositorFrameSink message pipes. The
  // browser compositor may request a new CompositorFrameSink on context loss,
  // which will destroy the existing CompositorFrameSink.
  GetHostFrameSinkManager()->CreateRootCompositorFrameSink(
      std::move(root_params));

  // Create LayerTreeFrameSink with the browser end of CompositorFrameSink.
  viz::ClientLayerTreeFrameSink::InitParams params;
  params.compositor_task_runner = compositor->task_runner();
  params.gpu_memory_buffer_manager = GetGpuMemoryBufferManager();
  params.pipes.compositor_frame_sink_associated_info = std::move(sink_info);
  params.pipes.client_request = std::move(client_request);
  params.local_surface_id_provider =
      std::make_unique<viz::DefaultLocalSurfaceIdProvider>();
  params.enable_surface_synchronization = true;
  params.hit_test_data_provider =
      std::make_unique<viz::HitTestDataProviderDrawQuad>(
          /*should_ask_for_child_region=*/false);

  scoped_refptr<viz::ContextProvider> compositor_context;
  scoped_refptr<viz::RasterContextProvider> worker_context;
  if (gpu_compositing) {
    // Only pass the contexts to the compositor if it will use gpu compositing.
    compositor_context = main_context_provider_;
    worker_context = worker_context_provider_;
  }
  compositor->SetLayerTreeFrameSink(
      std::make_unique<viz::ClientLayerTreeFrameSink>(
          std::move(compositor_context), std::move(worker_context), &params));

#if defined(OS_WIN)
  gfx::RenderingWindowManager::GetInstance()->DoSetParentOnChild(
      compositor->widget());
#endif
}

gpu::ContextResult
VizProcessTransportFactory::TryCreateContextsForGpuCompositing(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host) {
  DCHECK(!is_gpu_compositing_disabled_);

  // Fallback to software compositing if there is no IPC channel.
  if (!gpu_channel_host)
    return gpu::ContextResult::kFatalFailure;

  // Fallback to software compositing if GPU compositing is blacklisted.
  auto gpu_compositing_status =
      gpu_channel_host->gpu_feature_info()
          .status_values[gpu::GPU_FEATURE_TYPE_GPU_COMPOSITING];
  if (gpu_compositing_status != gpu::kGpuFeatureStatusEnabled)
    return gpu::ContextResult::kFatalFailure;

  if (worker_context_provider_ &&
      IsWorkerContextLost(worker_context_provider_.get()))
    worker_context_provider_ = nullptr;

  if (!worker_context_provider_) {
    constexpr bool kSharedWorkerContextSupportsLocking = true;
    constexpr bool kSharedWorkerContextSupportsRaster = true;
    const bool kSharedWorkerContextSupportsGLES2 =
        features::IsUiGpuRasterizationEnabled();
    const bool kSharedWorkerContextSupportsGrContext =
        features::IsUiGpuRasterizationEnabled();

    worker_context_provider_ = CreateContextProviderImpl(
        gpu_channel_host, GetGpuMemoryBufferManager(),
        kSharedWorkerContextSupportsLocking, kSharedWorkerContextSupportsGLES2,
        kSharedWorkerContextSupportsRaster,
        kSharedWorkerContextSupportsGrContext,
        ui::command_buffer_metrics::BROWSER_WORKER_CONTEXT);

    // Don't observer context loss on |worker_context_provider_| here, that is
    // already observered by LayerTreeFrameSink. The lost context will be caught
    // when recreating LayerTreeFrameSink(s).
    auto context_result = worker_context_provider_->BindToCurrentThread();
    if (context_result != gpu::ContextResult::kSuccess) {
      worker_context_provider_ = nullptr;
      return context_result;
    }
  }

  if (main_context_provider_ && IsContextLost(main_context_provider_.get())) {
    main_context_provider_->RemoveObserver(this);
    main_context_provider_ = nullptr;
  }

  if (!main_context_provider_) {
    constexpr bool kCompositorContextSupportsLocking = false;
    constexpr bool kCompositorContextSupportsGLES2 = true;
    constexpr bool kCompositorContextSupportsRaster = false;
    constexpr bool kCompositorContextSupportsGrContext = true;

    main_context_provider_ = CreateContextProviderImpl(
        std::move(gpu_channel_host), GetGpuMemoryBufferManager(),
        kCompositorContextSupportsLocking, kCompositorContextSupportsGLES2,
        kCompositorContextSupportsRaster, kCompositorContextSupportsGrContext,
        ui::command_buffer_metrics::UI_COMPOSITOR_CONTEXT);
    main_context_provider_->SetDefaultTaskRunner(resize_task_runner_);

    auto context_result = main_context_provider_->BindToCurrentThread();
    if (context_result != gpu::ContextResult::kSuccess) {
      worker_context_provider_ = nullptr;
      main_context_provider_ = nullptr;
      return context_result;
    }

    main_context_provider_->AddObserver(this);
  }

  return gpu::ContextResult::kSuccess;
}

void VizProcessTransportFactory::OnLostMainThreadSharedContext() {
  // It's possible that |main_context_provider_| was already reset in
  // OnEstablishedGpuChannel(), so check if it's lost before resetting here.
  if (main_context_provider_ && IsContextLost(main_context_provider_.get())) {
    main_context_provider_->RemoveObserver(this);
    main_context_provider_ = nullptr;
  }

  for (auto& observer : observer_list_)
    observer.OnLostResources();
}

VizProcessTransportFactory::CompositorData::CompositorData() = default;
VizProcessTransportFactory::CompositorData::CompositorData(
    CompositorData&& other) = default;
VizProcessTransportFactory::CompositorData::~CompositorData() = default;
VizProcessTransportFactory::CompositorData&
VizProcessTransportFactory::CompositorData::operator=(CompositorData&& other) =
    default;

}  // namespace content
