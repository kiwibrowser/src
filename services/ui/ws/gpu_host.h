// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_GPU_HOST_H_
#define SERVICES_UI_WS_GPU_HOST_H_

#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "components/viz/service/main/viz_main_impl.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_info.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "services/ui/public/interfaces/gpu.mojom.h"
#include "services/viz/privileged/interfaces/gl/gpu_host.mojom.h"
#include "services/viz/privileged/interfaces/gl/gpu_service.mojom.h"

#if defined(OS_CHROMEOS)
#include "services/ui/public/interfaces/arc.mojom.h"
#endif  // defined(OS_CHROMEOS)

namespace discardable_memory {
class DiscardableSharedMemoryManager;
}

namespace service_manager {
class Connector;
}

namespace viz {
class ServerGpuMemoryBufferManager;
}

namespace ui {

namespace ws {

class GpuClient;

namespace test {
class GpuHostTest;
}  // namespace test

class GpuHostDelegate;

// GpuHost sets up connection from clients to the real service implementation in
// the GPU process.
class GpuHost {
 public:
  GpuHost() = default;
  virtual ~GpuHost() = default;

  virtual void Add(mojom::GpuRequest request) = 0;
  virtual void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget) = 0;
  virtual void OnAcceleratedWidgetDestroyed(gfx::AcceleratedWidget widget) = 0;

  // Requests a viz::mojom::FrameSinkManager interface from viz.
  virtual void CreateFrameSinkManager(
      viz::mojom::FrameSinkManagerParamsPtr params) = 0;

#if defined(OS_CHROMEOS)
  virtual void AddArc(mojom::ArcRequest request) = 0;
#endif  // defined(OS_CHROMEOS)
};

class DefaultGpuHost : public GpuHost, public viz::mojom::GpuHost {
 public:
  DefaultGpuHost(GpuHostDelegate* delegate,
                 service_manager::Connector* connector,
                 discardable_memory::DiscardableSharedMemoryManager*
                     discardable_shared_memory_manager);
  ~DefaultGpuHost() override;

 private:
  friend class test::GpuHostTest;

  GpuClient* AddInternal(mojom::GpuRequest request);
  void OnBadMessageFromGpu();

  // TODO(crbug.com/611505): this goes away after the gpu proces split in mus.
  void InitializeVizMain(viz::mojom::VizMainRequest request);
  void DestroyVizMain();

  // GpuHost:
  void Add(mojom::GpuRequest request) override;
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget) override;
  void OnAcceleratedWidgetDestroyed(gfx::AcceleratedWidget widget) override;
  void CreateFrameSinkManager(
      viz::mojom::FrameSinkManagerParamsPtr params) override;
#if defined(OS_CHROMEOS)
  void AddArc(mojom::ArcRequest request) override;
#endif  // defined(OS_CHROMEOS)

  // viz::mojom::GpuHost:
  void DidInitialize(const gpu::GPUInfo& gpu_info,
                     const gpu::GpuFeatureInfo& gpu_feature_info,
                     const base::Optional<gpu::GPUInfo>&,
                     const base::Optional<gpu::GpuFeatureInfo>&) override;
  void DidFailInitialize() override;
  void DidCreateContextSuccessfully() override;
  void DidCreateOffscreenContext(const GURL& url) override;
  void DidDestroyOffscreenContext(const GURL& url) override;
  void DidDestroyChannel(int32_t client_id) override;
  void DidLoseContext(bool offscreen,
                      gpu::error::ContextLostReason reason,
                      const GURL& active_url) override;
  void SetChildSurface(gpu::SurfaceHandle parent,
                       gpu::SurfaceHandle child) override;
  void StoreShaderToDisk(int32_t client_id,
                         const std::string& key,
                         const std::string& shader) override;
  void RecordLogMessage(int32_t severity,
                        const std::string& header,
                        const std::string& message) override;

  GpuHostDelegate* const delegate_;
  int32_t next_client_id_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  viz::mojom::GpuServicePtr gpu_service_;
  mojo::Binding<viz::mojom::GpuHost> gpu_host_binding_;
  gpu::GPUInfo gpu_info_;
  gpu::GpuFeatureInfo gpu_feature_info_;

  std::unique_ptr<viz::ServerGpuMemoryBufferManager> gpu_memory_buffer_manager_;

  viz::mojom::VizMainPtr viz_main_;

  // TODO(crbug.com/620927): This should be removed once ozone-mojo is done.
  base::Thread gpu_thread_;
  std::unique_ptr<viz::VizMainImpl> viz_main_impl_;

  mojo::StrongBindingSet<mojom::Gpu> gpu_bindings_;
#if defined(OS_CHROMEOS)
  mojo::StrongBindingSet<mojom::Arc> arc_bindings_;
#endif  // defined(OS_CHROMEOS)

  DISALLOW_COPY_AND_ASSIGN(DefaultGpuHost);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_GPU_HOST_H_
