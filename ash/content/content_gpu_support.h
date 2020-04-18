// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_CONTENT_CONTENT_GPU_SUPPORT_H_
#define ASH_CONTENT_CONTENT_GPU_SUPPORT_H_

#include <memory>
#include <vector>

#include "ash/content/ash_with_content_export.h"
#include "base/macros.h"
#include "content/public/browser/browser_thread.h"
#include "services/ui/ws2/gpu_support.h"

namespace content {
class GpuClient;
}

namespace ash {

// An implementation of GpuSupport that forwards to the Gpu implementation in
// content.
class ASH_WITH_CONTENT_EXPORT ContentGpuSupport : public ui::ws2::GpuSupport {
 public:
  ContentGpuSupport();
  ~ContentGpuSupport() override;

  // ui::ws2::GpuSupport:
  scoped_refptr<base::SingleThreadTaskRunner> GetGpuTaskRunner() override;
  void BindDiscardableSharedMemoryManagerOnGpuTaskRunner(
      discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request)
      override;
  void BindGpuRequestOnGpuTaskRunner(ui::mojom::GpuRequest request) override;

 private:
  void OnGpuClientConnectionError(content::GpuClient* client);

  std::vector<std::unique_ptr<content::GpuClient,
                              content::BrowserThread::DeleteOnIOThread>>
      gpu_clients_;

  DISALLOW_COPY_AND_ASSIGN(ContentGpuSupport);
};

}  // namespace ash

#endif  // ASH_CONTENT_CONTENT_GPU_SUPPORT_H_
