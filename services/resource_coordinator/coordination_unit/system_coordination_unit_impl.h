// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_SYSTEM_COORDINATION_UNIT_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_SYSTEM_COORDINATION_UNIT_IMPL_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"

namespace resource_coordinator {

class SystemCoordinationUnitImpl
    : public CoordinationUnitInterface<SystemCoordinationUnitImpl,
                                       mojom::SystemCoordinationUnit,
                                       mojom::SystemCoordinationUnitRequest> {
 public:
  static CoordinationUnitType Type() { return CoordinationUnitType::kSystem; }

  SystemCoordinationUnitImpl(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~SystemCoordinationUnitImpl() override;

  // mojom::SystemCoordinationUnit implementation:
  void OnProcessCPUUsageReady() override;
  void DistributeMeasurementBatch(
      mojom::ProcessResourceMeasurementBatchPtr measurement_batch) override;

 private:
  base::TimeTicks last_measurement_batch_time_;

  // CoordinationUnitInterface implementation:
  void OnEventReceived(mojom::Event event) override;
  void OnPropertyChanged(mojom::PropertyType property_type,
                         int64_t value) override;

  DISALLOW_COPY_AND_ASSIGN(SystemCoordinationUnitImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_SYSTEM_COORDINATION_UNIT_IMPL_H_
