// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/renderer_resource_coordinator.h"

#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"

namespace blink {

namespace {

RendererResourceCoordinator* g_renderer_resource_coordinator = nullptr;

}  // namespace

// static
void RendererResourceCoordinator::Initialize() {
  blink::Platform* platform = Platform::Current();
  DCHECK(IsMainThread());
  DCHECK(platform);
  g_renderer_resource_coordinator = new RendererResourceCoordinator(
      platform->GetConnector(), platform->GetBrowserServiceName());
}

// static
void RendererResourceCoordinator::
    SetCurrentRendererResourceCoordinatorForTesting(
        RendererResourceCoordinator* renderer_resource_coordinator) {
  g_renderer_resource_coordinator = renderer_resource_coordinator;
}

// static
RendererResourceCoordinator& RendererResourceCoordinator::Get() {
  DCHECK(g_renderer_resource_coordinator);
  return *g_renderer_resource_coordinator;
}

RendererResourceCoordinator::RendererResourceCoordinator(
    service_manager::Connector* connector,
    const std::string& service_name) {
  connector->BindInterface(service_name, &service_);
}

RendererResourceCoordinator::RendererResourceCoordinator() = default;

RendererResourceCoordinator::~RendererResourceCoordinator() = default;

void RendererResourceCoordinator::SetExpectedTaskQueueingDuration(
    base::TimeDelta duration) {
  if (!service_)
    return;
  service_->SetExpectedTaskQueueingDuration(duration);
}

void RendererResourceCoordinator::SetMainThreadTaskLoadIsLow(
    bool main_thread_task_load_is_low) {
  if (!service_)
    return;
  service_->SetMainThreadTaskLoadIsLow(main_thread_task_load_is_low);
}

}  // namespace blink
