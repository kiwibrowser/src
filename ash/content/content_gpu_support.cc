// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/content_gpu_support.h"

#include "base/containers/unique_ptr_adapters.h"
#include "content/public/browser/gpu_client.h"
#include "content/public/browser/gpu_service_registry.h"

namespace ash {

ContentGpuSupport::ContentGpuSupport() = default;

ContentGpuSupport::~ContentGpuSupport() = default;

scoped_refptr<base::SingleThreadTaskRunner>
ContentGpuSupport::GetGpuTaskRunner() {
  return content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::IO);
}

void ContentGpuSupport::BindDiscardableSharedMemoryManagerOnGpuTaskRunner(
    discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request) {
  content::BindInterfaceInGpuProcess(std::move(request));
}

void ContentGpuSupport::BindGpuRequestOnGpuTaskRunner(
    ui::mojom::GpuRequest request) {
  auto gpu_client = content::GpuClient::Create(
      std::move(request),
      base::BindOnce(&ContentGpuSupport::OnGpuClientConnectionError,
                     base::Unretained(this)));
  gpu_clients_.push_back(std::move(gpu_client));
}

void ContentGpuSupport::OnGpuClientConnectionError(content::GpuClient* client) {
  base::EraseIf(
      gpu_clients_,
      base::UniquePtrMatcher<content::GpuClient,
                             content::BrowserThread::DeleteOnIOThread>(client));
}

}  // namespace ash
