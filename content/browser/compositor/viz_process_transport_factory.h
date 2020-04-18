// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_VIZ_PROCESS_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_VIZ_PROCESS_TRANSPORT_FACTORY_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/gpu/context_lost_observer.h"
#include "components/viz/common/surfaces/frame_sink_id_allocator.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/browser/compositor/in_process_display_client.h"
#include "gpu/command_buffer/common/context_result.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/compositor/compositor.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace cc {
class SingleThreadTaskGraphRunner;
}

namespace gpu {
class GpuChannelEstablishFactory;
}

namespace ui {
class ContextProviderCommandBuffer;
}

namespace viz {
class CompositingModeReporterImpl;
class RasterContextProvider;
}

namespace content {

class ExternalBeginFrameControllerClientImpl;

// A replacement for GpuProcessTransportFactory to be used when running viz. In
// this configuration the display compositor is located in the viz process
// instead of in the browser process. Any interaction with the display
// compositor must happen over IPC.
class VizProcessTransportFactory : public ui::ContextFactory,
                                   public ui::ContextFactoryPrivate,
                                   public ImageTransportFactory,
                                   public viz::ContextLostObserver {
 public:
  VizProcessTransportFactory(
      gpu::GpuChannelEstablishFactory* gpu_channel_establish_factory,
      scoped_refptr<base::SingleThreadTaskRunner> resize_task_runner,
      viz::CompositingModeReporterImpl* compositing_mode_reporter);
  ~VizProcessTransportFactory() override;

  // Connects HostFrameSinkManager to FrameSinkManagerImpl in viz process.
  void ConnectHostFrameSinkManager();

  // ui::ContextFactory implementation.
  void CreateLayerTreeFrameSink(
      base::WeakPtr<ui::Compositor> compositor) override;
  scoped_refptr<viz::ContextProvider> SharedMainThreadContextProvider()
      override;
  void RemoveCompositor(ui::Compositor* compositor) override;
  double GetRefreshRate() const override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  cc::TaskGraphRunner* GetTaskGraphRunner() override;
  void AddObserver(ui::ContextFactoryObserver* observer) override;
  void RemoveObserver(ui::ContextFactoryObserver* observer) override;

  // ui::ContextFactoryPrivate implementation.
  std::unique_ptr<ui::Reflector> CreateReflector(ui::Compositor* source,
                                                 ui::Layer* target) override;
  void RemoveReflector(ui::Reflector* reflector) override;
  viz::FrameSinkId AllocateFrameSinkId() override;
  viz::HostFrameSinkManager* GetHostFrameSinkManager() override;
  void SetDisplayVisible(ui::Compositor* compositor, bool visible) override;
  void ResizeDisplay(ui::Compositor* compositor,
                     const gfx::Size& size) override;
  void SetDisplayColorMatrix(ui::Compositor* compositor,
                             const SkMatrix44& matrix) override;
  void SetDisplayColorSpace(ui::Compositor* compositor,
                            const gfx::ColorSpace& blending_color_space,
                            const gfx::ColorSpace& output_color_space) override;
  void SetAuthoritativeVSyncInterval(ui::Compositor* compositor,
                                     base::TimeDelta interval) override;
  void SetDisplayVSyncParameters(ui::Compositor* compositor,
                                 base::TimeTicks timebase,
                                 base::TimeDelta interval) override;
  void IssueExternalBeginFrame(ui::Compositor* compositor,
                               const viz::BeginFrameArgs& args) override;
  void SetOutputIsSecure(ui::Compositor* compositor, bool secure) override;
  viz::FrameSinkManagerImpl* GetFrameSinkManager() override;

  // ImageTransportFactory implementation.
  bool IsGpuCompositingDisabled() override;
  ui::ContextFactory* GetContextFactory() override;
  ui::ContextFactoryPrivate* GetContextFactoryPrivate() override;
  viz::GLHelper* GetGLHelper() override;

  // viz::ContextLostObserver implementation.
  void OnContextLost() override;

 private:
  struct CompositorData {
    CompositorData();
    CompositorData(CompositorData&& other);
    ~CompositorData();
    CompositorData& operator=(CompositorData&& other);

    // Privileged interface that controls the display for a root
    // CompositorFrameSink.
    viz::mojom::DisplayPrivateAssociatedPtr display_private;
    std::unique_ptr<InProcessDisplayClient> display_client;

    // Controls external BeginFrames for the display. Only set if external
    // BeginFrames are enabled for the compositor.
    std::unique_ptr<ExternalBeginFrameControllerClientImpl>
        external_begin_frame_controller_client;

   private:
    DISALLOW_COPY_AND_ASSIGN(CompositorData);
  };

  // Disables GPU compositing. This notifies UI and renderer compositors to drop
  // LayerTreeFrameSinks and request new ones. If fallback happens while
  // creating a new LayerTreeFrameSink for UI compositor it should be passed in
  // as |guilty_compositor| to avoid extra work and reentrancy problems.
  void DisableGpuCompositing(ui::Compositor* guilty_compositor);

  // Provided as a callback when the GPU process has crashed.
  void OnGpuProcessLost();

  // Finishes creation of LayerTreeFrameSink after GPU channel has been
  // established.
  void OnEstablishedGpuChannel(
      base::WeakPtr<ui::Compositor> compositor_weak_ptr,
      scoped_refptr<gpu::GpuChannelHost> gpu_channel);

  // Tries to create the raster and main thread ContextProviders. If the
  // ContextProviders already exist and haven't been lost then this will do
  // nothing. Also verifies |gpu_channel_host| and checks if GPU compositing is
  // blacklisted.
  //
  // Returns kSuccess if caller can use GPU compositing, kTransientFailure if
  // caller should try again or kFatalFailure if caller should fallback to
  // software compositing.
  gpu::ContextResult TryCreateContextsForGpuCompositing(
      scoped_refptr<gpu::GpuChannelHost> gpu_channel_host);

  void OnLostMainThreadSharedContext();

  gpu::GpuChannelEstablishFactory* const gpu_channel_establish_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> const resize_task_runner_;

  // Controls the compositing mode based on what mode the display compositors
  // are using.
  viz::CompositingModeReporterImpl* const compositing_mode_reporter_;

  base::flat_map<ui::Compositor*, CompositorData> compositor_data_map_;
  bool is_gpu_compositing_disabled_ = false;

  base::ObserverList<ui::ContextFactoryObserver> observer_list_;

  // ContextProvider used on worker threads for rasterization.
  scoped_refptr<viz::RasterContextProvider> worker_context_provider_;

  // ContextProvider used on the main thread. Shared by ui::Compositors and also
  // returned from GetSharedMainThreadContextProvider().
  scoped_refptr<ui::ContextProviderCommandBuffer> main_context_provider_;

  viz::FrameSinkIdAllocator frame_sink_id_allocator_;
  std::unique_ptr<cc::SingleThreadTaskGraphRunner> task_graph_runner_;
  const viz::RendererSettings renderer_settings_;

  base::WeakPtrFactory<VizProcessTransportFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VizProcessTransportFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_VIZ_PROCESS_TRANSPORT_FACTORY_H_
