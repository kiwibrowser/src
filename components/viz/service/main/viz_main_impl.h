// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_MAIN_VIZ_MAIN_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_MAIN_VIZ_MAIN_IMPL_H_

#include "base/power_monitor/power_monitor.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "components/discardable_memory/client/client_discardable_shared_memory_manager.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/gpu_init.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/gl/gpu_service.mojom.h"
#include "services/viz/privileged/interfaces/viz_main.mojom.h"

namespace gpu {
class SyncPointManager;
}  // namespace gpu

namespace service_manager {
class Connector;
}

namespace ukm {
class MojoUkmRecorder;
}

namespace viz {
class DisplayProvider;
class FrameSinkManagerImpl;
class GpuServiceImpl;

class VizMainImpl : public gpu::GpuSandboxHelper, public mojom::VizMain {
 public:
  struct LogMessage {
    int severity;
    std::string header;
    std::string message;
  };
  using LogMessages = std::vector<LogMessage>;

  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnInitializationFailed() = 0;
    virtual void OnGpuServiceConnection(GpuServiceImpl* gpu_service) = 0;
    virtual void PostCompositorThreadCreated(
        base::SingleThreadTaskRunner* task_runner) = 0;
  };

  struct ExternalDependencies {
   public:
    ExternalDependencies();
    ExternalDependencies(ExternalDependencies&& other);
    ~ExternalDependencies();

    ExternalDependencies& operator=(ExternalDependencies&& other);

    bool create_display_compositor = false;
    gpu::SyncPointManager* sync_point_manager = nullptr;
    base::WaitableEvent* shutdown_event = nullptr;
    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner;
    service_manager::Connector* connector = nullptr;

   private:
    DISALLOW_COPY_AND_ASSIGN(ExternalDependencies);
  };

  VizMainImpl(Delegate* delegate,
              ExternalDependencies dependencies,
              std::unique_ptr<gpu::GpuInit> gpu_init = nullptr);
  // Destruction must happen on the GPU thread.
  ~VizMainImpl() override;

  void SetLogMessagesForHost(LogMessages messages);

  void Bind(mojom::VizMainRequest request);
  void BindAssociated(mojom::VizMainAssociatedRequest request);

  // mojom::VizMain implementation:
  void CreateGpuService(
      mojom::GpuServiceRequest request,
      mojom::GpuHostPtr gpu_host,
      discardable_memory::mojom::DiscardableSharedMemoryManagerPtr
          discardable_memory_manager,
      mojo::ScopedSharedBufferHandle activity_flags) override;
  void CreateFrameSinkManager(mojom::FrameSinkManagerParamsPtr params) override;

  GpuServiceImpl* gpu_service() { return gpu_service_.get(); }
  const GpuServiceImpl* gpu_service() const { return gpu_service_.get(); }

 private:
  // Initializes GPU's UkmRecorder if GPU is running in it's own process.
  void CreateUkmRecorderIfNeeded(service_manager::Connector* connector);

  void CreateFrameSinkManagerInternal(mojom::FrameSinkManagerParamsPtr params);
  void CreateFrameSinkManagerOnCompositorThread(
      mojom::FrameSinkManagerParamsPtr params);

  void TearDownOnCompositorThread();

  // gpu::GpuSandboxHelper:
  void PreSandboxStartup() override;
  bool EnsureSandboxInitialized(gpu::GpuWatchdogThread* watchdog_thread,
                                const gpu::GPUInfo* gpu_info,
                                const gpu::GpuPreferences& gpu_prefs) override;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner() const {
    return io_thread_ ? io_thread_->task_runner()
                      : dependencies_.io_thread_task_runner;
  }

  Delegate* const delegate_;

  const ExternalDependencies dependencies_;

  // The thread that handles IO events for Gpu (if one isn't already provided).
  // |io_thread_| must be ordered above GPU service related variables so it's
  // destroyed after they are.
  std::unique_ptr<base::Thread> io_thread_;

  LogMessages log_messages_;

  std::unique_ptr<gpu::GpuInit> gpu_init_;
  std::unique_ptr<GpuServiceImpl> gpu_service_;

  // The InCommandCommandBuffer::Service used by the frame sink manager.
  scoped_refptr<gpu::InProcessCommandBuffer::Service> gpu_command_service_;

  // If the gpu service is not yet ready then we stash pending
  // FrameSinkManagerParams.
  mojom::FrameSinkManagerParamsPtr pending_frame_sink_manager_params_;

  // Provides mojo interfaces for creating and managing FrameSinks. These live
  // on the compositor thread.
  std::unique_ptr<FrameSinkManagerImpl> frame_sink_manager_;
  std::unique_ptr<DisplayProvider> display_provider_;

  const scoped_refptr<base::SingleThreadTaskRunner> gpu_thread_task_runner_;

  // The main thread for the display compositor.
  std::unique_ptr<base::Thread> compositor_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_thread_task_runner_;

  std::unique_ptr<ukm::MojoUkmRecorder> ukm_recorder_;
  std::unique_ptr<base::PowerMonitor> power_monitor_;
  mojo::Binding<mojom::VizMain> binding_;
  mojo::AssociatedBinding<mojom::VizMain> associated_binding_;

  std::unique_ptr<discardable_memory::ClientDiscardableSharedMemoryManager>
      discardable_shared_memory_manager_;

  DISALLOW_COPY_AND_ASSIGN(VizMainImpl);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_MAIN_VIZ_MAIN_IMPL_H_
