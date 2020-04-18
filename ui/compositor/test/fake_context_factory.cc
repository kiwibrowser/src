// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/test/fake_context_factory.h"

#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/base/switches.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/frame_sinks/delay_based_time_source.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/display/display_switches.h"
#include "ui/gfx/switches.h"

namespace ui {

FakeContextFactory::FakeContextFactory() {
#if defined(OS_WIN)
  renderer_settings_.finish_rendering_on_resize = true;
#elif defined(OS_MACOSX)
  renderer_settings_.release_overlay_resources_after_gpu_query = true;
#endif
}

FakeContextFactory::~FakeContextFactory() = default;

const viz::CompositorFrame& FakeContextFactory::GetLastCompositorFrame() const {
  return *frame_sink_->last_sent_frame();
}

void FakeContextFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  auto frame_sink = cc::FakeLayerTreeFrameSink::Create3d();
  frame_sink_ = frame_sink.get();
  compositor->SetLayerTreeFrameSink(std::move(frame_sink));
}

scoped_refptr<viz::ContextProvider>
FakeContextFactory::SharedMainThreadContextProvider() {
  return nullptr;
}

void FakeContextFactory::RemoveCompositor(ui::Compositor* compositor) {
  frame_sink_ = nullptr;
}

double FakeContextFactory::GetRefreshRate() const {
  return 200.0;
}

gpu::GpuMemoryBufferManager* FakeContextFactory::GetGpuMemoryBufferManager() {
  return &gpu_memory_buffer_manager_;
}

cc::TaskGraphRunner* FakeContextFactory::GetTaskGraphRunner() {
  return &task_graph_runner_;
}

}  // namespace ui
