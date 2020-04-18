// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_MUS_CONTEXT_FACTORY_H_
#define UI_AURA_MUS_MUS_CONTEXT_FACTORY_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/gpu/context_provider.h"
#include "services/ui/public/cpp/raster_thread_helper.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "ui/aura/aura_export.h"
#include "ui/compositor/compositor.h"

namespace gpu {
class GpuChannelHost;
}

namespace ui {
class Gpu;
}

namespace aura {

// ContextFactory implementation that can be used with Mus.
class AURA_EXPORT MusContextFactory : public ui::ContextFactory {
 public:
  explicit MusContextFactory(ui::Gpu* gpu);
  ~MusContextFactory() override;

 private:
  // Callback function for Gpu::EstablishGpuChannel().
  void OnEstablishedGpuChannel(base::WeakPtr<ui::Compositor> compositor,
                               scoped_refptr<gpu::GpuChannelHost> gpu_channel);

  // ContextFactory:
  void CreateLayerTreeFrameSink(
      base::WeakPtr<ui::Compositor> compositor) override;
  scoped_refptr<viz::ContextProvider> SharedMainThreadContextProvider()
      override;
  void RemoveCompositor(ui::Compositor* compositor) override;
  double GetRefreshRate() const override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  cc::TaskGraphRunner* GetTaskGraphRunner() override;
  void AddObserver(ui::ContextFactoryObserver* observer) override {}
  void RemoveObserver(ui::ContextFactoryObserver* observer) override {}

  ui::RasterThreadHelper raster_thread_helper_;
  ui::Gpu* gpu_;
  scoped_refptr<viz::ContextProvider> shared_main_thread_context_provider_;

  base::WeakPtrFactory<MusContextFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MusContextFactory);
};

}  // namespace aura

#endif  // UI_AURA_MUS_MUS_CONTEXT_FACTORY_H_
