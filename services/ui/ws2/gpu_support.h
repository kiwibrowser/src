// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_GPU_SUPPORT_H_
#define SERVICES_UI_WS2_GPU_SUPPORT_H_

#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "components/discardable_memory/public/interfaces/discardable_shared_memory_manager.mojom.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace ui {
namespace ws2 {

// GpuSupport provides the functions needed for gpu related functionality. It is
// expected this class is bound to functions in content that do this. Once Viz
// moves out of content, this class can be removed.
class COMPONENT_EXPORT(WINDOW_SERVICE) GpuSupport {
 public:
  virtual ~GpuSupport() {}

  // Returns the task runner used to bind gpu related interfaces on. This is
  // typically the io-thread.
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetGpuTaskRunner() = 0;

  // The Bind functions are called when a client requests a gpu related
  // interface.
  virtual void BindDiscardableSharedMemoryManagerOnGpuTaskRunner(
      discardable_memory::mojom::DiscardableSharedMemoryManagerRequest
          request) = 0;
  virtual void BindGpuRequestOnGpuTaskRunner(ui::mojom::GpuRequest request) = 0;
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_GPU_SUPPORT_H_
