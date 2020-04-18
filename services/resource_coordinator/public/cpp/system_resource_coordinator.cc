// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/system_resource_coordinator.h"

namespace resource_coordinator {

SystemResourceCoordinator::SystemResourceCoordinator(
    service_manager::Connector* connector)
    : ResourceCoordinatorInterface() {
  CoordinationUnitID new_cu_id(CoordinationUnitType::kSystem, std::string());
  ResourceCoordinatorInterface::ConnectToService(connector, new_cu_id);
}

SystemResourceCoordinator::~SystemResourceCoordinator() = default;

void SystemResourceCoordinator::DistributeMeasurementBatch(
    mojom::ProcessResourceMeasurementBatchPtr batch) {
  if (!service_)
    return;
  service_->DistributeMeasurementBatch(std::move(batch));
}

void SystemResourceCoordinator::ConnectToService(
    mojom::CoordinationUnitProviderPtr& provider,
    const CoordinationUnitID& cu_id) {
  provider->GetSystemCoordinationUnit(mojo::MakeRequest(&service_));
}

}  // namespace resource_coordinator
