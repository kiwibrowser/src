// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/test/test_image_factory.h"
#include "cc/test/test_task_graph_runner.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/surfaces/frame_sink_id_allocator.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "ui/compositor/compositor.h"

namespace viz {
class GLHelper;
class FrameSinkManagerImpl;
class TestFrameSinkManagerImpl;
}  // namespace viz

namespace content {

// Test implementation of ImageTransportFactory, ContextFactory and
// ContextFactoryPrivate. This class tries to do very little, mostly setting up
// HostFrameSinkManager and returning fake implementations where possible.
//
// This class will change behavior depending on the VizDisplayCompositor
// feature. With the feature enabled it behaves like VizProcessTransportFactory,
// otherwise it behaves like GpuProcessTransportFactory.
class TestImageTransportFactory : public ui::ContextFactory,
                                  public ui::ContextFactoryPrivate,
                                  public ImageTransportFactory {
 public:
  TestImageTransportFactory();
  ~TestImageTransportFactory() override;

  // ui::ContextFactory implementation.
  void CreateLayerTreeFrameSink(
      base::WeakPtr<ui::Compositor> compositor) override;
  scoped_refptr<viz::ContextProvider> SharedMainThreadContextProvider()
      override;
  void RemoveCompositor(ui::Compositor* compositor) override {}
  double GetRefreshRate() const override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  cc::TaskGraphRunner* GetTaskGraphRunner() override;
  void AddObserver(ui::ContextFactoryObserver* observer) override;
  void RemoveObserver(ui::ContextFactoryObserver* observer) override;

  // ui::ContextFactoryPrivate implementation.
  std::unique_ptr<ui::Reflector> CreateReflector(ui::Compositor* source,
                                                 ui::Layer* target) override;
  void RemoveReflector(ui::Reflector* reflector) override {}
  viz::FrameSinkId AllocateFrameSinkId() override;
  viz::HostFrameSinkManager* GetHostFrameSinkManager() override;
  void SetDisplayVisible(ui::Compositor* compositor, bool visible) override {}
  void ResizeDisplay(ui::Compositor* compositor,
                     const gfx::Size& size) override {}
  void SetDisplayColorMatrix(ui::Compositor* compositor,
                             const SkMatrix44& matrix) override {}
  void SetDisplayColorSpace(
      ui::Compositor* compositor,
      const gfx::ColorSpace& blending_color_space,
      const gfx::ColorSpace& output_color_space) override {}
  void SetAuthoritativeVSyncInterval(ui::Compositor* compositor,
                                     base::TimeDelta interval) override {}
  void SetDisplayVSyncParameters(ui::Compositor* compositor,
                                 base::TimeTicks timebase,
                                 base::TimeDelta interval) override {}
  void IssueExternalBeginFrame(ui::Compositor* compositor,
                               const viz::BeginFrameArgs& args) override {}
  void SetOutputIsSecure(ui::Compositor* compositor, bool secure) override {}
  viz::FrameSinkManagerImpl* GetFrameSinkManager() override;

  // ImageTransportFactory implementation.
  bool IsGpuCompositingDisabled() override;
  ui::ContextFactory* GetContextFactory() override;
  ui::ContextFactoryPrivate* GetContextFactoryPrivate() override;
  viz::GLHelper* GetGLHelper() override;

 private:
  const bool enable_viz_;

  cc::TestTaskGraphRunner task_graph_runner_;
  cc::TestImageFactory image_factory_;
  viz::TestGpuMemoryBufferManager gpu_memory_buffer_manager_;
  viz::RendererSettings renderer_settings_;
  viz::FrameSinkIdAllocator frame_sink_id_allocator_;
  scoped_refptr<viz::ContextProvider> shared_main_context_provider_;
  base::ObserverList<ui::ContextFactoryObserver> observer_list_;
  viz::HostFrameSinkManager host_frame_sink_manager_;

  // Objects that exist if |enable_viz_| is false.
  std::unique_ptr<viz::FrameSinkManagerImpl> frame_sink_manager_impl_;
  std::unique_ptr<viz::GLHelper> gl_helper_;

  // Objects that exist if |enable_viz_| is true.
  std::unique_ptr<viz::TestFrameSinkManagerImpl> test_frame_sink_manager_impl_;

  DISALLOW_COPY_AND_ASSIGN(TestImageTransportFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_
