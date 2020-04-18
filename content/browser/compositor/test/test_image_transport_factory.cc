// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/test/test_image_transport_factory.h"

#include <limits>
#include <utility>

#include "components/viz/common/features.h"
#include "components/viz/common/gl_helper.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/test/test_frame_sink_manager.h"
#include "content/browser/compositor/surface_utils.h"
#include "ui/compositor/reflector.h"
#include "ui/compositor/test/in_process_context_provider.h"

namespace content {
namespace {

// TODO(kylechar): Use the same client id for the browser everywhere.
constexpr uint32_t kDefaultClientId = std::numeric_limits<uint32_t>::max();

class FakeReflector : public ui::Reflector {
 public:
  FakeReflector() = default;
  ~FakeReflector() override = default;

  void OnMirroringCompositorResized() override {}
  void AddMirroringLayer(ui::Layer* layer) override {}
  void RemoveMirroringLayer(ui::Layer* layer) override {}
};

}  // namespace

TestImageTransportFactory::TestImageTransportFactory()
    : enable_viz_(
          base::FeatureList::IsEnabled(features::kVizDisplayCompositor)),
      frame_sink_id_allocator_(kDefaultClientId) {
  if (enable_viz_) {
    test_frame_sink_manager_impl_ =
        std::make_unique<viz::TestFrameSinkManagerImpl>();

    viz::mojom::FrameSinkManagerPtr frame_sink_manager;
    viz::mojom::FrameSinkManagerRequest frame_sink_manager_request =
        mojo::MakeRequest(&frame_sink_manager);
    viz::mojom::FrameSinkManagerClientPtr frame_sink_manager_client;
    viz::mojom::FrameSinkManagerClientRequest
        frame_sink_manager_client_request =
            mojo::MakeRequest(&frame_sink_manager_client);

    // Bind endpoints in HostFrameSinkManager.
    host_frame_sink_manager_.BindAndSetManager(
        std::move(frame_sink_manager_client_request), nullptr,
        std::move(frame_sink_manager));

    // Bind endpoints in TestFrameSinkManagerImpl. For non-tests there would be
    // a FrameSinkManagerImpl running in another process and these interface
    // endpoints would be bound there.
    test_frame_sink_manager_impl_->BindRequest(
        std::move(frame_sink_manager_request),
        std::move(frame_sink_manager_client));
  } else {
    frame_sink_manager_impl_ = std::make_unique<viz::FrameSinkManagerImpl>();
    surface_utils::ConnectWithLocalFrameSinkManager(
        &host_frame_sink_manager_, frame_sink_manager_impl_.get());
  }
}

TestImageTransportFactory::~TestImageTransportFactory() {
  std::unique_ptr<viz::GLHelper> lost_gl_helper = std::move(gl_helper_);

  for (auto& observer : observer_list_)
    observer.OnLostResources();
}

void TestImageTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  compositor->SetLayerTreeFrameSink(cc::FakeLayerTreeFrameSink::Create3d());
}

scoped_refptr<viz::ContextProvider>
TestImageTransportFactory::SharedMainThreadContextProvider() {
  if (shared_main_context_provider_ &&
      shared_main_context_provider_->ContextGL()->GetGraphicsResetStatusKHR() ==
          GL_NO_ERROR)
    return shared_main_context_provider_;

  constexpr bool kSupportsLocking = false;
  shared_main_context_provider_ = ui::InProcessContextProvider::CreateOffscreen(
      &gpu_memory_buffer_manager_, &image_factory_, kSupportsLocking);
  auto result = shared_main_context_provider_->BindToCurrentThread();
  if (result != gpu::ContextResult::kSuccess)
    shared_main_context_provider_ = nullptr;

  return shared_main_context_provider_;
}

double TestImageTransportFactory::GetRefreshRate() const {
  // The context factory created here is for unit tests, thus using a higher
  // refresh rate to spend less time waiting for BeginFrames.
  return 200.0;
}

gpu::GpuMemoryBufferManager*
TestImageTransportFactory::GetGpuMemoryBufferManager() {
  return &gpu_memory_buffer_manager_;
}

cc::TaskGraphRunner* TestImageTransportFactory::GetTaskGraphRunner() {
  return &task_graph_runner_;
}

void TestImageTransportFactory::AddObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.AddObserver(observer);
}

void TestImageTransportFactory::RemoveObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

std::unique_ptr<ui::Reflector> TestImageTransportFactory::CreateReflector(
    ui::Compositor* source,
    ui::Layer* target) {
  if (!enable_viz_)
    return std::make_unique<FakeReflector>();

  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTIMPLEMENTED();
  return nullptr;
}

viz::FrameSinkId TestImageTransportFactory::AllocateFrameSinkId() {
  return frame_sink_id_allocator_.NextFrameSinkId();
}

viz::HostFrameSinkManager*
TestImageTransportFactory::GetHostFrameSinkManager() {
  return &host_frame_sink_manager_;
}

viz::FrameSinkManagerImpl* TestImageTransportFactory::GetFrameSinkManager() {
  if (enable_viz_) {
    // Nothing should use FrameSinkManagerImpl with VizDisplayCompositor
    // enabled.
    NOTREACHED();
    return nullptr;
  }

  return frame_sink_manager_impl_.get();
}

bool TestImageTransportFactory::IsGpuCompositingDisabled() {
  return false;
}

ui::ContextFactory* TestImageTransportFactory::GetContextFactory() {
  return this;
}

ui::ContextFactoryPrivate*
TestImageTransportFactory::GetContextFactoryPrivate() {
  return this;
}

viz::GLHelper* TestImageTransportFactory::GetGLHelper() {
  if (enable_viz_) {
    // Nothing should use GLHelper with VizDisplayCompositor enabled.
    NOTREACHED();
    return nullptr;
  }

  if (!gl_helper_) {
    auto context_provider = SharedMainThreadContextProvider();
    gl_helper_ = std::make_unique<viz::GLHelper>(
        context_provider->ContextGL(), context_provider->ContextSupport());
  }
  return gl_helper_.get();
}

}  // namespace content
