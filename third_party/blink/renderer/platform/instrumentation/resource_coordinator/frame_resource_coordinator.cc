// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/frame_resource_coordinator.h"

#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

// static
FrameResourceCoordinator* FrameResourceCoordinator::Create(
    service_manager::InterfaceProvider* interface_provider) {
  return new FrameResourceCoordinator(interface_provider);
}

FrameResourceCoordinator::FrameResourceCoordinator(
    service_manager::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));
}

FrameResourceCoordinator::~FrameResourceCoordinator() = default;

void FrameResourceCoordinator::SetNetworkAlmostIdle(bool idle) {
  if (!service_)
    return;
  service_->SetNetworkAlmostIdle(idle);
}

void FrameResourceCoordinator::SetLifecycleState(
    resource_coordinator::mojom::LifecycleState state) {
  if (!service_)
    return;
  service_->SetLifecycleState(state);
}

void FrameResourceCoordinator::OnNonPersistentNotificationCreated() {
  if (!service_)
    return;
  service_->OnNonPersistentNotificationCreated();
}

}  // namespace blink
