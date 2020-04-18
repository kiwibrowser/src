// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_HOST_LAYERED_WINDOW_UPDATER_IMPL_H_
#define COMPONENTS_VIZ_HOST_LAYERED_WINDOW_UPDATER_IMPL_H_

#include <windows.h>

#include <memory>

#include "base/macros.h"
#include "components/viz/host/viz_host_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/layered_window_updater.mojom.h"
#include "ui/gfx/geometry/size.h"

class SkCanvas;

namespace viz {

// Makes layered window drawing syscalls. Updates a layered window from shared
// memory backing buffer that was drawn into by the GPU process. This is
// required as UpdateLayeredWindow() syscall is blocked by the GPU sandbox.
class VIZ_HOST_EXPORT LayeredWindowUpdaterImpl
    : public mojom::LayeredWindowUpdater {
 public:
  LayeredWindowUpdaterImpl(HWND hwnd,
                           mojom::LayeredWindowUpdaterRequest request);
  ~LayeredWindowUpdaterImpl() override;

  // mojom::LayeredWindowUpdater implementation.
  void OnAllocatedSharedMemory(
      const gfx::Size& pixel_size,
      mojo::ScopedSharedBufferHandle scoped_buffer_handle) override;
  void Draw(DrawCallback draw_callback) override;

 private:
  const HWND hwnd_;
  mojo::Binding<mojom::LayeredWindowUpdater> binding_;
  std::unique_ptr<SkCanvas> canvas_;

  DISALLOW_COPY_AND_ASSIGN(LayeredWindowUpdaterImpl);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_HOST_LAYERED_WINDOW_UPDATER_IMPL_H_
