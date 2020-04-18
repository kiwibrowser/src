// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_provider_impl.h"
#include "services/resource_coordinator/coordination_unit/system_coordination_unit_impl.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace ukm {
class UkmEntryBuilder;
}  // namespace ukm

namespace resource_coordinator {

CoordinationUnitManager::CoordinationUnitManager() {
  CoordinationUnitBase::AssertNoActiveCoordinationUnits();
}

CoordinationUnitManager::~CoordinationUnitManager() {
  // TODO(oysteine): Keep the map of coordination units as a member of this
  // class, rather than statically inside CoordinationUnitBase, to avoid this
  // manual lifetime management.
  CoordinationUnitBase::ClearAllCoordinationUnits();
}

void CoordinationUnitManager::OnStart(
    service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>* registry,
    service_manager::ServiceContextRefFactory* service_ref_factory) {
  // Create the singleton CoordinationUnitProvider.
  provider_ =
      std::make_unique<CoordinationUnitProviderImpl>(service_ref_factory, this);
  registry->AddInterface(base::BindRepeating(
      &CoordinationUnitProviderImpl::Bind, base::Unretained(provider_.get())));

  // Create a singleton SystemCU instance. Ownership is taken by
  // CoordinationUnitBase. This interface is not directly registered to the
  // service, but rather clients can access it via CoordinationUnitProvider.
  CoordinationUnitID system_cu_id(CoordinationUnitType::kSystem, std::string());
  system_cu_ = SystemCoordinationUnitImpl::Create(
      system_cu_id, service_ref_factory->CreateRef());
}

void CoordinationUnitManager::RegisterObserver(
    std::unique_ptr<CoordinationUnitGraphObserver> observer) {
  observer->set_coordination_unit_manager(this);
  observers_.push_back(std::move(observer));
}

void CoordinationUnitManager::OnCoordinationUnitCreated(
    CoordinationUnitBase* coordination_unit) {
  for (auto& observer : observers_) {
    if (observer->ShouldObserve(coordination_unit)) {
      coordination_unit->AddObserver(observer.get());
      observer->OnCoordinationUnitCreated(coordination_unit);
    }
  }
}

void CoordinationUnitManager::OnBeforeCoordinationUnitDestroyed(
    CoordinationUnitBase* coordination_unit) {
  coordination_unit->BeforeDestroyed();
}

}  // namespace resource_coordinator
