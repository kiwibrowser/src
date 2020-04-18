// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ash_service.h"

#include "ash/mojo_interface_factory.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/service_manager/embedder/embedded_service_info.h"

namespace ash {
namespace {

std::unique_ptr<service_manager::Service> CreateAshService() {
  return std::make_unique<AshService>();
}

}  // namespace

AshService::AshService() = default;

AshService::~AshService() = default;

// static
service_manager::EmbeddedServiceInfo AshService::CreateEmbeddedServiceInfo() {
  service_manager::EmbeddedServiceInfo info;
  info.factory = base::BindRepeating(&CreateAshService);
  info.task_runner = base::ThreadTaskRunnerHandle::Get();
  return info;
}

void AshService::OnStart() {
  mojo_interface_factory::RegisterInterfaces(
      &registry_, base::ThreadTaskRunnerHandle::Get());
}

void AshService::OnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle) {
  registry_.BindInterface(interface_name, std::move(handle));
}

}  // namespace ash
