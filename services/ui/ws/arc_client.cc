// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/arc_client.h"

#include "services/viz/privileged/interfaces/gl/gpu_service.mojom.h"

namespace ui {
namespace ws {

ArcClient::ArcClient(viz::mojom::GpuService* gpu_service)
    : gpu_service_(gpu_service) {}

ArcClient::~ArcClient() {}

// mojom::Arc overrides:
void ArcClient::CreateVideoDecodeAccelerator(
    arc::mojom::VideoDecodeAcceleratorRequest vda_request) {
  gpu_service_->CreateArcVideoDecodeAccelerator(std::move(vda_request));
}

void ArcClient::CreateVideoEncodeAccelerator(
    arc::mojom::VideoEncodeAcceleratorRequest vea_request) {
  gpu_service_->CreateArcVideoEncodeAccelerator(std::move(vea_request));
}

void ArcClient::CreateVideoProtectedBufferAllocator(
    arc::mojom::VideoProtectedBufferAllocatorRequest pba_request) {
  gpu_service_->CreateArcVideoProtectedBufferAllocator(std::move(pba_request));
}

void ArcClient::CreateProtectedBufferManager(
    arc::mojom::ProtectedBufferManagerRequest pbm_request) {
  gpu_service_->CreateArcProtectedBufferManager(std::move(pbm_request));
}

}  // namespace ws
}  // namespace ui
