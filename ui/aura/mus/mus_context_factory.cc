// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/mus_context_factory.h"

#include "base/command_line.h"
#include "cc/base/switches.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/window_tree_host.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/display/display_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_bindings.h"

namespace aura {

MusContextFactory::MusContextFactory(ui::Gpu* gpu)
    : gpu_(gpu),
      weak_ptr_factory_(this) {}

MusContextFactory::~MusContextFactory() {}

void MusContextFactory::OnEstablishedGpuChannel(
    base::WeakPtr<ui::Compositor> compositor,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  if (!compositor)
    return;
  WindowTreeHost* host =
      WindowTreeHost::GetForAcceleratedWidget(compositor->widget());
  WindowPortMus* window_port = WindowPortMus::Get(host->window());
  DCHECK(window_port);

  scoped_refptr<viz::ContextProvider> context_provider =
      gpu_->CreateContextProvider(std::move(gpu_channel));
  // If the binding fails, then we need to return early since the compositor
  // expects a successfully initialized/bound provider.
  if (context_provider->BindToCurrentThread() != gpu::ContextResult::kSuccess) {
    // TODO(danakj): We should retry if the result was not kFatalFailure.
    return;
  }
  std::unique_ptr<cc::LayerTreeFrameSink> layer_tree_frame_sink =
      window_port->RequestLayerTreeFrameSink(std::move(context_provider),
                                             gpu_->gpu_memory_buffer_manager());
  compositor->SetLayerTreeFrameSink(std::move(layer_tree_frame_sink));
}

void MusContextFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  gpu_->EstablishGpuChannel(
      base::BindOnce(&MusContextFactory::OnEstablishedGpuChannel,
                     weak_ptr_factory_.GetWeakPtr(), compositor));
}

scoped_refptr<viz::ContextProvider>
MusContextFactory::SharedMainThreadContextProvider() {
  if (!shared_main_thread_context_provider_) {
    scoped_refptr<gpu::GpuChannelHost> gpu_channel =
        gpu_->EstablishGpuChannelSync();
    shared_main_thread_context_provider_ =
        gpu_->CreateContextProvider(std::move(gpu_channel));
    if (shared_main_thread_context_provider_->BindToCurrentThread() !=
        gpu::ContextResult::kSuccess)
      shared_main_thread_context_provider_ = nullptr;
  }
  return shared_main_thread_context_provider_;
}

void MusContextFactory::RemoveCompositor(ui::Compositor* compositor) {
  // NOTIMPLEMENTED();
}

double MusContextFactory::GetRefreshRate() const {
  return 60.0;
}

gpu::GpuMemoryBufferManager* MusContextFactory::GetGpuMemoryBufferManager() {
  return gpu_->gpu_memory_buffer_manager();
}

cc::TaskGraphRunner* MusContextFactory::GetTaskGraphRunner() {
  return raster_thread_helper_.task_graph_runner();
}

}  // namespace aura
