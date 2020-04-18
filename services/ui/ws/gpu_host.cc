// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/gpu_host.h"

#include "base/command_line.h"
#include "base/memory/shared_memory.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "components/viz/host/server_gpu_memory_buffer_manager.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_shared_memory.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/ws/gpu_client.h"
#include "services/ui/ws/gpu_host_delegate.h"
#include "services/viz/public/interfaces/constants.mojom.h"
#include "ui/gfx/buffer_format_util.h"

#if defined(OS_WIN)
#include "ui/gfx/win/rendering_window_manager.h"
#endif

#if defined(OS_CHROMEOS)
#include "services/ui/ws/arc_client.h"
#endif

namespace ui {
namespace ws {

namespace {

// The client Id 1 is reserved for the frame sink manager.
const int32_t kInternalGpuChannelClientId = 2;

// TODO(crbug.com/620927): This should be removed once ozone-mojo is done.
bool HasSplitVizProcess() {
  constexpr char kEnableViz[] = "enable-viz";
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kEnableViz);
}

}  // namespace

DefaultGpuHost::DefaultGpuHost(
    GpuHostDelegate* delegate,
    service_manager::Connector* connector,
    discardable_memory::DiscardableSharedMemoryManager*
        discardable_shared_memory_manager)
    : delegate_(delegate),
      next_client_id_(kInternalGpuChannelClientId + 1),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      gpu_host_binding_(this),
      gpu_thread_("GpuThread") {
  DCHECK(discardable_shared_memory_manager);

  auto request = MakeRequest(&viz_main_);
  if (connector && HasSplitVizProcess()) {
    connector->BindInterface(viz::mojom::kVizServiceName, std::move(request));
  } else {
    // TODO(crbug.com/620927): This should be removed once ozone-mojo is done.
    gpu_thread_.Start();
    gpu_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&DefaultGpuHost::InitializeVizMain,
                                  base::Unretained(this),
                                  base::Passed(MakeRequest(&viz_main_))));
  }

  discardable_memory::mojom::DiscardableSharedMemoryManagerPtr
      discardable_manager_ptr;
  service_manager::BindSourceInfo source_info;
  discardable_shared_memory_manager->Bind(
      mojo::MakeRequest(&discardable_manager_ptr), source_info);

  viz::mojom::GpuHostPtr gpu_host_proxy;
  gpu_host_binding_.Bind(mojo::MakeRequest(&gpu_host_proxy));
  viz_main_->CreateGpuService(
      MakeRequest(&gpu_service_), std::move(gpu_host_proxy),
      std::move(discardable_manager_ptr), mojo::ScopedSharedBufferHandle());
  gpu_memory_buffer_manager_ =
      std::make_unique<viz::ServerGpuMemoryBufferManager>(
          gpu_service_.get(), next_client_id_++,
          std::make_unique<gpu::GpuMemoryBufferSupport>());
}

DefaultGpuHost::~DefaultGpuHost() {
  // TODO(crbug.com/620927): This should be removed once ozone-mojo is done.
  if (gpu_thread_.IsRunning()) {
    // Stop() will return after |viz_main_impl_| has been destroyed.
    gpu_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&DefaultGpuHost::DestroyVizMain,
                                  base::Unretained(this)));
    gpu_thread_.Stop();
  }
}

void DefaultGpuHost::Add(mojom::GpuRequest request) {
  AddInternal(std::move(request));
}

void DefaultGpuHost::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget) {
#if defined(OS_WIN)
  gfx::RenderingWindowManager::GetInstance()->RegisterParent(widget);
#endif
}

void DefaultGpuHost::OnAcceleratedWidgetDestroyed(
    gfx::AcceleratedWidget widget) {
#if defined(OS_WIN)
  gfx::RenderingWindowManager::GetInstance()->UnregisterParent(widget);
#endif
}

void DefaultGpuHost::CreateFrameSinkManager(
    viz::mojom::FrameSinkManagerParamsPtr params) {
  viz_main_->CreateFrameSinkManager(std::move(params));
}

#if defined(OS_CHROMEOS)
void DefaultGpuHost::AddArc(mojom::ArcRequest request) {
  arc_bindings_.AddBinding(std::make_unique<ArcClient>(gpu_service_.get()),
                           std::move(request));
}
#endif  // defined(OS_CHROMEOS)

GpuClient* DefaultGpuHost::AddInternal(mojom::GpuRequest request) {
  auto client(std::make_unique<GpuClient>(
      next_client_id_++, &gpu_info_, &gpu_feature_info_,
      gpu_memory_buffer_manager_.get(), gpu_service_.get()));
  GpuClient* client_ref = client.get();
  gpu_bindings_.AddBinding(std::move(client), std::move(request));
  return client_ref;
}

void DefaultGpuHost::OnBadMessageFromGpu() {
  // TODO(sad): Received some unexpected message from the gpu process. We
  // should kill the process and restart it.
  NOTIMPLEMENTED();
}

void DefaultGpuHost::InitializeVizMain(viz::mojom::VizMainRequest request) {
  viz::VizMainImpl::ExternalDependencies deps;
  deps.create_display_compositor = true;
  viz_main_impl_ = std::make_unique<viz::VizMainImpl>(nullptr, std::move(deps));
  viz_main_impl_->Bind(std::move(request));
}

void DefaultGpuHost::DestroyVizMain() {
  DCHECK(viz_main_impl_);
  viz_main_impl_.reset();
}

void DefaultGpuHost::DidInitialize(const gpu::GPUInfo& gpu_info,
                                   const gpu::GpuFeatureInfo& gpu_feature_info,
                                   const base::Optional<gpu::GPUInfo>&,
                                   const base::Optional<gpu::GpuFeatureInfo>&) {
  gpu_info_ = gpu_info;
  gpu_feature_info_ = gpu_feature_info;
  delegate_->OnGpuServiceInitialized();
}

void DefaultGpuHost::DidFailInitialize() {}

void DefaultGpuHost::DidCreateContextSuccessfully() {}

void DefaultGpuHost::DidCreateOffscreenContext(const GURL& url) {}

void DefaultGpuHost::DidDestroyOffscreenContext(const GURL& url) {}

void DefaultGpuHost::DidDestroyChannel(int32_t client_id) {}

void DefaultGpuHost::DidLoseContext(bool offscreen,
                                    gpu::error::ContextLostReason reason,
                                    const GURL& active_url) {}

void DefaultGpuHost::SetChildSurface(gpu::SurfaceHandle parent,
                                     gpu::SurfaceHandle child) {
#if defined(OS_WIN)
  // Verify that |parent| was created by the window server.
  DWORD process_id = 0;
  DWORD thread_id = GetWindowThreadProcessId(parent, &process_id);
  if (!thread_id || process_id != ::GetCurrentProcessId()) {
    OnBadMessageFromGpu();
    return;
  }

  // TODO(sad): Also verify that |child| was created by the mus-gpu process.

  if (!gfx::RenderingWindowManager::GetInstance()->RegisterChild(parent,
                                                                 child)) {
    OnBadMessageFromGpu();
  }
#else
  NOTREACHED();
#endif
}

void DefaultGpuHost::StoreShaderToDisk(int32_t client_id,
                                       const std::string& key,
                                       const std::string& shader) {}

void DefaultGpuHost::RecordLogMessage(int32_t severity,
                                      const std::string& header,
                                      const std::string& message) {}

}  // namespace ws
}  // namespace ui
