// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_ARC_CLIENT_H_
#define SERVICES_UI_WS_ARC_CLIENT_H_

#include "services/ui/public/interfaces/arc.mojom.h"

namespace viz {
namespace mojom {
class GpuService;
}  // namespace mojom
}  // namespace viz

namespace ui {
namespace ws {

// The implementation that relays requests from clients to the real
// service implementation in the GPU process over mojom.GpuService.
class ArcClient : public mojom::Arc {
 public:
  explicit ArcClient(viz::mojom::GpuService* gpu_service);
  ~ArcClient() override;

 private:
  // mojom::Arc overrides:
  void CreateVideoDecodeAccelerator(
      arc::mojom::VideoDecodeAcceleratorRequest vda_request) override;

  void CreateVideoEncodeAccelerator(
      arc::mojom::VideoEncodeAcceleratorRequest vea_request) override;

  void CreateVideoProtectedBufferAllocator(
      arc::mojom::VideoProtectedBufferAllocatorRequest pba_request) override;

  void CreateProtectedBufferManager(
      arc::mojom::ProtectedBufferManagerRequest pbm_request) override;

  // The objects these pointers refer to are owned by the GpuHost object.
  viz::mojom::GpuService* gpu_service_;

  DISALLOW_COPY_AND_ASSIGN(ArcClient);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_ARC_CLIENT_H_
